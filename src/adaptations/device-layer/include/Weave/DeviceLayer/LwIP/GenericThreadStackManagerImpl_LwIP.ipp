/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Contains non-inline method definitions for the
 *          GenericThreadStackManagerImpl_LwIP<> template.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/LwIP/GenericThreadStackManagerImpl_LwIP.h>
#include <openthread/ip6.h>

#define NRF_LOG_MODULE_NAME weave
#include "nrf_log.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_LwIP<ImplClass>::InitThreadNetIf()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    static struct netif sThreadNetIf;

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Initialize a LwIP netif structure for the OpenThread interface and
    // add it to the list of interfaces known to LwIP.
    mNetIf = netif_add(&sThreadNetIf,
#if LWIP_IPV4
                       NULL, NULL, NULL,
#endif // LWIP_IPV4
                       NULL, DoInitThreadNetIf, tcpip_input);

    // Unkock LwIP stack
    UNLOCK_TCPIP_CORE();

    VerifyOrExit(mNetIf != NULL, err = INET_ERROR_INTERFACE_INIT_FAILURE);

    // Lock OpenThread
    Impl()->LockThreadStack();

    // Arrange for OpenThread to call our ReceivePacket() method whenever an
    // IPv6 packet is received.
    otIp6SetReceiveCallback(Impl()->OTInstance(), ReceivePacket, NULL);

    // Unlock OpenThread
    Impl()->UnlockThreadStack();

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_LwIP<ImplClass>::UpdateNetIfAddresses()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err_t lwipErr = ERR_OK;
    const otNetifAddress * otAddrs;
    bool addrSet[LWIP_IPV6_NUM_ADDRESSES];
    struct netif * threadNetIf = ThreadStackMgrImpl().ThreadNetIf();

    memset(addrSet, 0, sizeof(addrSet));

    // Lock LwIP stack first, then OpenThread.
    LOCK_TCPIP_CORE();
    Impl()->LockThreadStack();

    // Get the list of active unicast IPv6 addresses known to OpenThread.
    otAddrs = otIp6GetUnicastAddresses(Impl()->OTInstance());

    // For each such address that is in the valid state...
    for (const otNetifAddress * otAddr = otAddrs; otAddr != NULL; otAddr = otAddr->mNext)
    {
        if (otAddr->mValid)
        {
            const struct ip6_addr * addr = reinterpret_cast<const struct ip6_addr *>(otAddr->mAddress.mFields.m32);
            s8_t addrIdx;

            // TODO: filter to ignore anything except link-local, mesh-local and Weave fabric addresses.

            // If the address is a link-local, and the primary link-local address fpr the LwIP netif has not been
            // set already, then set the current link-local address as the primary.  (The primary link-local address
            // is the address with an index of zero).
            //
            // NOTE: This special case is required because netif_add_ip6_address() will never set the primary
            // link-local address.
            //
            if (ip6_addr_islinklocal(addr) && !addrSet[0])
            {
                netif_ip6_addr_set(threadNetIf, 0, addr);
                addrIdx = 0;
            }
            else
            {
                lwipErr = netif_add_ip6_address(threadNetIf, addr, &addrIdx);
                if (lwipErr == ERR_VAL)
                {
                    break;
                }
                VerifyOrExit(lwipErr == ERR_OK, err = nl::Weave::System::MapErrorLwIP(lwipErr));
            }

            netif_ip6_addr_set_state(threadNetIf, addrIdx, (otAddr->mPreferred) ? IP6_ADDR_PREFERRED : IP6_ADDR_DEPRECATED);
            addrSet[addrIdx] = true;
        }
    }

    // For each address associated with the netif that was *not* set able, remove the address
    // from the netif by setting its state to INVALID.
    for (u8_t addrIdx = 0; addrIdx < LWIP_IPV6_NUM_ADDRESSES; addrIdx++)
    {
        if (!addrSet[addrIdx])
        {
            netif_ip6_addr_set_state(threadNetIf, addrIdx, IP6_ADDR_INVALID);
        }
    }

exit:

    Impl()->UnlockThreadStack();
    UNLOCK_TCPIP_CORE();

    return err;
}

template<class ImplClass>
err_t GenericThreadStackManagerImpl_LwIP<ImplClass>::DoInitThreadNetIf(struct netif * netif)
{
    netif->name[0] = WEAVE_DEVICE_CONFIG_LWIP_THREAD_IF_NAME[0];
    netif->name[1] = WEAVE_DEVICE_CONFIG_LWIP_THREAD_IF_NAME[1];
    netif->output_ip6 = SendPacket;
#if LWIP_IPV4 || LWIP_VERSION_MAJOR < 2
    netif->output = NULL;
#endif /* LWIP_IPV4 || LWIP_VERSION_MAJOR < 2 */
    netif->linkoutput = NULL;
    netif->flags = NETIF_FLAG_UP|NETIF_FLAG_LINK_UP|NETIF_FLAG_BROADCAST|NETIF_FLAG_MLD6;
    netif->mtu = WEAVE_DEVICE_CONFIG_THREAD_IF_MTU;
    return ERR_OK;
}

/**
 * Send an outbound packet via the LwIP Thread interface.
 *
 * This method is called by LwIP, via a pointer in the netif structure, whenever
 * an IPv6 packet is to be sent out the Thread interface.
 *
 * NB: This method is called with the LwIP TCPIP core lock held.
 */
template<class ImplClass>
#if LWIP_VERSION_MAJOR < 2
err_t GenericThreadStackManagerImpl_LwIP<ImplClass>::SendPacket(struct netif * netif, struct pbuf * pkt, struct ip6_addr * ipaddr)
#else
err_t GenericThreadStackManagerImpl_LwIP<ImplClass>::SendPacket(struct netif * netif, struct pbuf * pkt, const struct ip6_addr * ipaddr)
#endif
{
    err_t lwipErr = ERR_OK;
    otError otErr;
    otMessage * otMsg = NULL;
    uint16_t remainingLen;

    // Lock the OpenThread stack.
    // Note that at this point the LwIP core lock is also held.
    ThreadStackMgrImpl().LockThreadStack();

// TODO: remove me
//    if (ipaddr->addr[0] == 0xFF020000)
//        ExitNow();

    // Allocate an OpenThread message
    otMsg = otIp6NewMessage(ThreadStackMgrImpl().OTInstance(), NULL);
    VerifyOrExit(otMsg != NULL, lwipErr = ERR_MEM);

    // Copy data from LwIP's packet buffer chain into the OpenThread message.
    remainingLen = pkt->tot_len;
    for (struct pbuf * partialPkt = pkt; partialPkt != NULL; partialPkt = partialPkt->next)
    {
        VerifyOrExit(partialPkt->len <= remainingLen, lwipErr = ERR_VAL);

        otErr = otMessageAppend(otMsg, partialPkt->payload, partialPkt->len);
        VerifyOrExit(otErr == OT_ERROR_NONE, lwipErr = ERR_MEM);

        remainingLen -= partialPkt->len;
    }
    VerifyOrExit(remainingLen == 0, lwipErr = ERR_VAL);

    // Pass the packet to OpenThread to be sent.  Note that OpenThread takes care of releasing
    // the otMessage object regardless of whether otIp6Send() succeeds or fails.
    otErr = otIp6Send(ThreadStackMgrImpl().OTInstance(), otMsg);
    otMsg = NULL;
    VerifyOrExit(otErr == OT_ERROR_NONE, lwipErr = ERR_IF);

exit:

    NRF_LOG_DEBUG("Thread packet sent (len %u, err %d)", pkt->tot_len, lwipErr);

    if (otMsg != NULL)
    {
        otMessageFree(otMsg);
    }

    // Unlock the OpenThread stack.
    ThreadStackMgrImpl().UnlockThreadStack();

    return lwipErr;
}

/**
 * Receive an inbound packet from the LwIP Thread interface.
 *
 * This method is called by OpenThread whenever an IPv6 packet has been received destined
 * to the local node has been received from the Thread interface.
 *
 * NB: This method is called with the OpenThread stack lock held.  To ensure proper
 * lock ordering, it must *not* attempt to acquire the LwIP TCPIP core lock, or the
 * OpenWeave stack lock.
 */
template<class ImplClass>
void GenericThreadStackManagerImpl_LwIP<ImplClass>::ReceivePacket(otMessage * pkt, void *)
{
    struct pbuf * pbuf = NULL;
    err_t lwipErr = ERR_OK;
    uint16_t pktLen = otMessageGetLength(pkt);
    struct netif * threadNetIf = ThreadStackMgrImpl().ThreadNetIf();

    // Allocate an LwIP pbuf to hold the inbound packet.
    pbuf = pbuf_alloc(PBUF_RAW, pktLen, PBUF_POOL);
    VerifyOrExit(pbuf != NULL, lwipErr = ERR_MEM);

    // Copy the packet data from the OpenThread message object to the pbuf.
    if (otMessageRead(pkt, 0, pbuf->payload, pktLen) != pktLen)
    {
        ExitNow(lwipErr = ERR_IF);
    }

    // Deliver the packet to the input function associated with the LwIP netif.
    // NOTE: The input function posts the inbound packet to LwIP's TCPIP thread.
    // Thus there's no need to acquire the LwIP TCPIP core lock here.
    lwipErr = threadNetIf->input(pbuf, threadNetIf);

    NRF_LOG_DEBUG("Thread packet received (len %u, err %d)", pktLen, lwipErr);

exit:

    // Always free the OpenThread message.
    otMessageFree(pkt);

    if (lwipErr != ERR_OK)
    {
        // If an error occurred, make sure the pbuf gets freed.
        if (pbuf != NULL)
        {
            pbuf_free(pbuf);
        }

        // TODO: log packet reception error
        // TODO: deliver Weave platform event signaling loss of inbound packet.
    }
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

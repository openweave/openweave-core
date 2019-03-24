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
#include <Weave/Support/logging/DecodedIPPacket.h>
#include <Weave/Core/WeaveEncoding.h>

#include <openthread/ip6.h>
#include <openthread/icmp6.h>

#define NRF_LOG_MODULE_NAME weave
#include "nrf_log.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

namespace {

void LogPacket(const char * direction, otMessage * pkt)
{
    char srcStr[50], destStr[50], typeBuf[20];
    const char * type = typeBuf;
    IPAddress addr;
    uint8_t headerData[44];
    uint16_t pktLen;

    const uint8_t & IPv6_NextHeader = headerData[6];
    const uint8_t * const IPv6_SrcAddr = headerData + 8;
    const uint8_t * const IPv6_DestAddr = headerData + 24;
    const uint8_t * const IPv6_SrcPort = headerData + 40;
    const uint8_t * const IPv6_DestPort = headerData + 42;
    const uint8_t & ICMPv6_Type = headerData[40];
    const uint8_t & ICMPv6_Code = headerData[41];

    constexpr uint8_t kIPProto_UDP = 17;
    constexpr uint8_t kIPProto_TCP = 6;
    constexpr uint8_t kIPProto_ICMPv6 = 58;

    constexpr uint8_t kICMPType_EchoRequest = 128;
    constexpr uint8_t kICMPType_EchoResponse = 129;

    pktLen = otMessageGetLength(pkt);

    if (pktLen >= sizeof(headerData))
    {
        otMessageRead(pkt, 0, headerData, sizeof(headerData));

        memcpy(addr.Addr, IPv6_SrcAddr, 16);
        addr.ToString(srcStr, sizeof(srcStr));

        memcpy(addr.Addr, IPv6_DestAddr, 16);
        addr.ToString(destStr, sizeof(destStr));

        if (IPv6_NextHeader == kIPProto_UDP)
        {
            type = "UDP";
        }
        else if (IPv6_NextHeader == kIPProto_TCP)
        {
            type = "TCP";
        }
        else if (IPv6_NextHeader == kIPProto_ICMPv6)
        {
            if (ICMPv6_Type == kICMPType_EchoRequest)
            {
                type = "ICMPv6 Echo Request";
            }
            else if (ICMPv6_Type == kICMPType_EchoResponse)
            {
                type = "ICMPv6 Echo Response";
            }
            else
            {
                snprintf(typeBuf, sizeof(typeBuf), "ICMPv6 %" PRIu8 ",%" PRIu8, ICMPv6_Type, ICMPv6_Code);
            }
        }
        else
        {
            snprintf(typeBuf, sizeof(typeBuf), "IP proto %" PRIu8, IPv6_NextHeader);
        }

        if (IPv6_NextHeader == kIPProto_UDP || IPv6_NextHeader == kIPProto_TCP)
        {
            snprintf(srcStr + strlen(srcStr), 13, ", port %" PRIu16, Encoding::BigEndian::Get16(IPv6_SrcPort));
            snprintf(destStr + strlen(destStr), 13, ", port %" PRIu16, Encoding::BigEndian::Get16(IPv6_DestPort));
        }

        WeaveLogDetail(DeviceLayer, "Thread packet %s: %s, len %" PRIu16, direction, type, pktLen);
        WeaveLogDetail(DeviceLayer, "    src  %s", srcStr);
        WeaveLogDetail(DeviceLayer, "    dest %s", destStr);
    }
    else
    {
        WeaveLogDetail(DeviceLayer, "Thread packet %s: %s, len %" PRIu16, direction, "(decode error)", pktLen);
    }
}

}

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

    // Disable automatic echo mode in OpenThread.
    otIcmp6SetEchoMode(Impl()->OTInstance(), OT_ICMP6_ECHO_HANDLER_DISABLED);

    // Enable the receive filter for Thread control traffic.
    otIp6SetReceiveFilterEnabled(Impl()->OTInstance(), true);

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

    WeaveLogDetail(DeviceLayer, "Updating Thread interface addresses");

    // Lock LwIP stack first, then OpenThread.
    LOCK_TCPIP_CORE();
    Impl()->LockThreadStack();

    // Get the list of active unicast IPv6 addresses known to OpenThread.
    otAddrs = otIp6GetUnicastAddresses(Impl()->OTInstance());

    // For each such address that is in the valid state...
    for (const otNetifAddress * otAddr = otAddrs; otAddr != NULL; otAddr = otAddr->mNext)
    {
        if (!otAddr->mValid)
        {
            continue;
        }

        const IPAddress * addr = reinterpret_cast<const struct IPAddress *>(otAddr->mAddress.mFields.m32);
        bool isLinkLocal = addr->IsIPv6LinkLocal();
        bool isMeshLocal;
        s8_t addrIdx;
        u8_t addrState = (otAddr->mPreferred) ? IP6_ADDR_PREFERRED : IP6_ADDR_VALID;

        // If the address is a link-local, and the primary link-local address for the LwIP netif has not been
        // set already, then set the current link-local address as the primary.  (The primary link-local address
        // is the address with an index of zero).
        //
        // NOTE: This special case is required because netif_add_ip6_address() will never set the primary
        // link-local address.
        //
        if (isLinkLocal && !addrSet[0])
        {
            ip_addr_t lwipAddr = addr->ToLwIPAddr();
            netif_ip6_addr_set(threadNetIf, 0, ip_2_ip6(&lwipAddr));
            addrIdx = 0;
        }
        else
        {
            isMeshLocal = IsOpenThreadMeshLocalAddress(Impl()->OTInstance(), *addr);

            // If the address is not a link-local, mesh-local or Weave fabric address, ignore it.
            if (!isLinkLocal && !isMeshLocal && !FabricState.IsFabricAddress(*addr))
            {
                continue;
            }

            // Ignore the address if it is a mesh-local RLOC address.  These are used internally
            // by Thread, but are not used by applications.
            if (otAddr->mRloc)
            {
                continue;
            }

            // Add the address to the LwIP netif.  netif_add_ip6_address() returns the index
            // of the address within the netif address table.
            ip_addr_t lwipAddr = addr->ToLwIPAddr();
            lwipErr = netif_add_ip6_address(threadNetIf, ip_2_ip6(&lwipAddr), &addrIdx);
            if (lwipErr == ERR_VAL)
            {
                break;
            }
            VerifyOrExit(lwipErr == ERR_OK, err = nl::Weave::System::MapErrorLwIP(lwipErr));
        }

        // Set the address state to PREFERRED or ACTIVE depending on the state in OpenThread.
        netif_ip6_addr_set_state(threadNetIf, addrIdx, addrState);
        addrSet[addrIdx] = true;

#if WEAVE_DETAIL_LOGGING

        {
            char addrStr[50];
            addr->ToString(addrStr, sizeof(addrStr));
            const char * typeStr;
            if (isLinkLocal)
                typeStr = "link-local";
            else if (isMeshLocal)
                typeStr = "mesh-local";
            else
                typeStr = "fabric";
            WeaveLogDetail(DeviceLayer, "   %s (#%d, %s%s)", addrStr, addrIdx, typeStr,
                    (addrState == IP6_ADDR_PREFERRED) ? ", preferred" : "");
        }

#endif // WEAVE_DETAIL_LOGGING
    }

    // For each address associated with the netif that was *not* set, remove the address
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
err_t GenericThreadStackManagerImpl_LwIP<ImplClass>::SendPacket(struct netif * netif, struct pbuf * pktPBuf, struct ip6_addr * ipaddr)
#else
err_t GenericThreadStackManagerImpl_LwIP<ImplClass>::SendPacket(struct netif * netif, struct pbuf * pktPBuf, const struct ip6_addr * ipaddr)
#endif
{
    err_t lwipErr = ERR_OK;
    otError otErr;
    otMessage * pktMsg = NULL;
    uint16_t remainingLen;

    // Lock the OpenThread stack.
    // Note that at this point the LwIP core lock is also held.
    ThreadStackMgrImpl().LockThreadStack();

    // Allocate an OpenThread message
    pktMsg = otIp6NewMessage(ThreadStackMgrImpl().OTInstance(), true);
    VerifyOrExit(pktMsg != NULL, lwipErr = ERR_MEM);

    // Copy data from LwIP's packet buffer chain into the OpenThread message.
    remainingLen = pktPBuf->tot_len;
    for (struct pbuf * partialPkt = pktPBuf; partialPkt != NULL; partialPkt = partialPkt->next)
    {
        VerifyOrExit(partialPkt->len <= remainingLen, lwipErr = ERR_VAL);

        otErr = otMessageAppend(pktMsg, partialPkt->payload, partialPkt->len);
        VerifyOrExit(otErr == OT_ERROR_NONE, lwipErr = ERR_MEM);

        remainingLen -= partialPkt->len;
    }
    VerifyOrExit(remainingLen == 0, lwipErr = ERR_VAL);

    LogPacket("SENT", pktMsg);

    // Pass the packet to OpenThread to be sent.  Note that OpenThread takes care of releasing
    // the otMessage object regardless of whether otIp6Send() succeeds or fails.
    otErr = otIp6Send(ThreadStackMgrImpl().OTInstance(), pktMsg);
    pktMsg = NULL;
    VerifyOrExit(otErr == OT_ERROR_NONE, lwipErr = ERR_IF);

exit:

    if (pktMsg != NULL)
    {
        otMessageFree(pktMsg);
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

    LogPacket("RCVD", pkt);

    // Deliver the packet to the input function associated with the LwIP netif.
    // NOTE: The input function posts the inbound packet to LwIP's TCPIP thread.
    // Thus there's no need to acquire the LwIP TCPIP core lock here.
    lwipErr = threadNetIf->input(pbuf, threadNetIf);

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

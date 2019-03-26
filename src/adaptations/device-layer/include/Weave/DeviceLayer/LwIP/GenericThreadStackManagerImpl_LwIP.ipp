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

#include <openthread/thread.h>
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
WEAVE_ERROR GenericThreadStackManagerImpl_LwIP<ImplClass>::InitThreadNetIf(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    static struct netif sThreadNetIf;

    memset(mAddrAssigned, 0, sizeof(mAddrAssigned));

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Initialize a LwIP netif structure for the OpenThread interface and
    // add it to the list of interfaces known to LwIP.
    mNetIf = netif_add(&sThreadNetIf,
#if LWIP_IPV4
                       NULL, NULL, NULL,
#endif // LWIP_IPV4
                       NULL, DoInitThreadNetIf, tcpip_input);

    // Start with the interface in the down state.
    netif_set_link_down(mNetIf);

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
WEAVE_ERROR GenericThreadStackManagerImpl_LwIP<ImplClass>::UpdateThreadNetIfState(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err_t lwipErr = ERR_OK;
    otDeviceRole curRole;
    bool isAttached;
    bool addrAssigned[LWIP_IPV6_NUM_ADDRESSES];

    memset(addrAssigned, 0, sizeof(addrAssigned));

    // Lock LwIP stack first, then OpenThread.
    LOCK_TCPIP_CORE();
    Impl()->LockThreadStack();

    // Determine the current OpenThread device role and whether the device is attached to a Thread network.
    curRole = otThreadGetDeviceRole(Impl()->OTInstance());
    isAttached = (curRole != OT_DEVICE_ROLE_DISABLED && curRole != OT_DEVICE_ROLE_DETACHED);

    // If needed, adjust the link state of the LwIP netif to reflect the state of the OpenThread stack.
    if (isAttached != (bool)netif_is_link_up(mNetIf))
    {
        WeaveLogDetail(DeviceLayer, "LwIP Thread interface %s", (isAttached) ? "UP" : "DOWN");

        if (isAttached)
        {
            netif_set_link_up(mNetIf);
        }
        else
        {
            netif_set_link_down(mNetIf);
        }

        // Post an event signaling the change in Thread connectivity state.
        {
            WeaveDeviceEvent event;
            event.Type = DeviceEventType::kThreadConnectivityChange;
            event.ThreadConnectivityChange.Result = (isAttached) ? kConnectivity_Established : kConnectivity_Lost;
            PlatformMgr().PostEvent(&event);
        }
    }

    // If attached to a Thread network, adjust the set of addresses on the LwIP netif to match those
    // configured in the Thread stack...
    if (isAttached)
    {
        // Enumerate the list of unicast IPv6 addresses known to OpenThread...
        const otNetifAddress * otAddrs = otIp6GetUnicastAddresses(Impl()->OTInstance());
        for (const otNetifAddress * otAddr = otAddrs; otAddr != NULL; otAddr = otAddr->mNext)
        {
            IPAddress addr = ToIPAddress(otAddr->mAddress);

            // Assign the following OpenThread addresses to LwIP's address table:
            //   - link-local addresses.
            //   - mesh-local addresses that are NOT RLOC addresses.
            //   - global unicast addresses.
            //
            // This logic purposefully leaves out Weave fabric ULAs, as well as other non-fabric ULAs that the
            // Thread stack assigns due to Thread SLAAC.
            //
            // Assignments of Weave fabric ULAs to the netif address table are handled separately by the WARM module.
            //
            // Non-fabric ULAs are ignored entirely as they are presumed to not be of interest to Weave-enabled
            // devices, and would otherwise consume slots in the LwIP address table, potentially leading to
            // starvation.
            if (otAddr->mValid &&
                !otAddr->mRloc &&
                (!addr.IsIPv6ULA() || IsOpenThreadMeshLocalAddress(Impl()->OTInstance(), addr)))
            {
                ip_addr_t lwipAddr = addr.ToLwIPAddr();
                s8_t addrIdx;

                // Add the address to the LwIP netif.  If the address is a link-local, and the primary
                // link-local address* for the LwIP netif has not been set already, then use netif_ip6_addr_set()
                // to set the primary address.  Otherwise use netif_add_ip6_address().
                //
                // This special case is required because LwIP's netif_add_ip6_address() will never set
                // the primary link-local address.
                //
                // * -- The primary link-local address always appears in the first slot in the netif address table.
                //
                if (addr.IsIPv6LinkLocal() && !addrAssigned[0])
                {
                    netif_ip6_addr_set(mNetIf, 0, ip_2_ip6(&lwipAddr));
                    addrIdx = 0;
                }
                else
                {
                    lwipErr = netif_add_ip6_address(mNetIf, ip_2_ip6(&lwipAddr), &addrIdx);
                    if (lwipErr == ERR_VAL)
                    {
                        break;
                    }
                    VerifyOrExit(lwipErr == ERR_OK, err = nl::Weave::System::MapErrorLwIP(lwipErr));
                }

                // Set the address state to PREFERRED or ACTIVE depending on the state in OpenThread.
                netif_ip6_addr_set_state(mNetIf, addrIdx, (otAddr->mPreferred) ? IP6_ADDR_PREFERRED : IP6_ADDR_VALID);

                // Record that the netif address slot was assigned during this loop.
                addrAssigned[addrIdx] = true;
            }
        }
    }

    WeaveLogDetail(DeviceLayer, "LwIP Thread interface addresses %s", (isAttached) ? "updated" : "cleared");

    // For each address associated with the netif that was *not* assigned above, remove the address
    // from the netif if the address is one that was previously assigned by this method.
    // In the case where the device is no longer attached to a Thread network, remove all addresses
    // from the netif.
    for (u8_t addrIdx = 0; addrIdx < LWIP_IPV6_NUM_ADDRESSES; addrIdx++)
    {
        if (!isAttached || (mAddrAssigned[addrIdx] && !addrAssigned[addrIdx]))
        {
            // Remove the address from the netif by setting its state to INVALID
            netif_ip6_addr_set_state(mNetIf, addrIdx, IP6_ADDR_INVALID);
        }

#if WEAVE_DETAIL_LOGGING
        else
        {
            uint8_t state = netif_ip6_addr_state(mNetIf, addrIdx);
            if (state != IP6_ADDR_INVALID)
            {
                IPAddress addr = IPAddress::FromLwIPAddr(*netif_ip6_addr(mNetIf, addrIdx));
                char addrStr[50];
                addr.ToString(addrStr, sizeof(addrStr));
                const char * typeStr;
                if (IsOpenThreadMeshLocalAddress(Impl()->OTInstance(), addr))
                    typeStr = "Thread mesh-local address";
                else
                    typeStr = CharacterizeIPv6Address(addr);
                WeaveLogDetail(DeviceLayer, "   %s (%s%s)", addrStr, typeStr,
                        (state == IP6_ADDR_PREFERRED) ? ", preferred" : "");
            }
        }
#endif // WEAVE_DETAIL_LOGGING
    }

    // Remember the set of assigned addresses.
    memcpy(mAddrAssigned, addrAssigned, sizeof(mAddrAssigned));

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

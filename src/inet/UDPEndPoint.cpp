/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2013-2018 Nest Labs, Inc.
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
 *      This file implements the <tt>nl::Inet::UDPEndPoint</tt>
 *      class, where the Nest Inet Layer encapsulates methods for
 *      interacting with UDP transport endpoints (SOCK_DGRAM sockets
 *      on Linux and BSD-derived systems) or LwIP UDP protocol
 *      control blocks, as the system is configured accordingly.
 *
 */

#define __APPLE_USE_RFC_3542

#include <string.h>

#include <InetLayer/UDPEndPoint.h>
#include <InetLayer/InetLayer.h>
#include <InetLayer/InetFaultInjection.h>
#include <SystemLayer/SystemFaultInjection.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/udp.h>
#include <lwip/tcpip.h>
#include <lwip/ip.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <sys/select.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif // HAVE_SYS_SOCKET_H
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#include "arpa-inet-compatibility.h"

// SOCK_CLOEXEC not defined on all platforms, e.g. iOS/MacOS:
#ifdef SOCK_CLOEXEC
#define SOCK_FLAGS SOCK_CLOEXEC
#else
#define SOCK_FLAGS 0
#endif

namespace nl {
namespace Inet {

using Weave::System::PacketBuffer;

Weave::System::ObjectPool<UDPEndPoint, INET_CONFIG_NUM_UDP_ENDPOINTS> UDPEndPoint::sPool;

INET_ERROR UDPEndPoint::Bind(IPAddressType addrType, IPAddress addr, uint16_t port, InterfaceId intfId)
{
    INET_ERROR res = INET_NO_ERROR;

    if (mState != kState_Ready && mState != kState_Bound)
        return INET_ERROR_INCORRECT_STATE;

    if (addr != IPAddress::Any && addr.Type() != kIPAddressType_Any && addr.Type() != addrType)
        return INET_ERROR_WRONG_ADDRESS_TYPE;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB.
    res = GetPCB(addrType);

    // Bind the PCB to the specified address/port.
    if (res == INET_NO_ERROR)
    {
#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
        ip_addr_t ipAddr = addr.ToLwIPAddr();
#if INET_CONFIG_ENABLE_IPV4
        lwip_ip_addr_type lType = IPAddress::ToLwIPAddrType(addrType);
        IP_SET_TYPE_VAL(ipAddr, lType);
#endif // INET_CONFIG_ENABLE_IPV4
        res = Weave::System::MapErrorLwIP(udp_bind(mUDP, &ipAddr, port));
#else // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
        if (addrType == kIPAddressType_IPv6)
        {
            ip6_addr_t ipv6Addr = addr.ToIPv6();
            res = Weave::System::MapErrorLwIP(udp_bind_ip6(mUDP, &ipv6Addr, port));
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4)
        {
            ip4_addr_t ipv4Addr = addr.ToIPv4();
            res = Weave::System::MapErrorLwIP(udp_bind(mUDP, &ipv4Addr, port));
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else
            res = INET_ERROR_WRONG_ADDRESS_TYPE;
#endif // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
    }

    if (res == INET_NO_ERROR)
    {
        if (!IsInterfaceIdPresent(intfId))
            mUDP->intf_filter = NULL;
        else
        {
            struct netif *p;
            for (p = netif_list; p != NULL && p != intfId; p = p->next);
            if (p == NULL)
                res = INET_ERROR_UNKNOWN_INTERFACE;
            else
                mUDP->intf_filter = p;
        }
    }

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // Make sure we have the appropriate type of socket.
    res = GetSocket(addrType);

    if (res == INET_NO_ERROR)
    {
        res = IPEndPointBasis::Bind(addrType, addr, port, intfId);

        if (res == INET_NO_ERROR)
        {
            mBoundPort = port;
            mBoundIntfId = intfId;
        }
    }

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (res == INET_NO_ERROR)
        mState = kState_Bound;

    return res;
}

INET_ERROR UDPEndPoint::Listen(void)
{
    INET_ERROR res = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    Weave::System::Layer& lSystemLayer = SystemLayer();
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (mState == kState_Listening)
        return INET_NO_ERROR;

    if (mState != kState_Bound)
    {
        return INET_ERROR_INCORRECT_STATE;
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
    udp_recv(mUDP, LwIPReceiveUDPMessage, this);
#else // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
    if (PCB_ISIPV6(mUDP))
        udp_recv_ip6(mUDP, LwIPReceiveUDPMessage, this);
    else
        udp_recv(mUDP, LwIPReceiveUDPMessage, this);
#endif // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // Wake the thread calling select so that it starts selecting on the new socket.
    lSystemLayer.WakeSelect();

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (res == INET_NO_ERROR)
        mState = kState_Listening;

    return res;
}

void UDPEndPoint::Close(void)
{
    if (mState != kState_Closed)
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP

        // Lock LwIP stack
        LOCK_TCPIP_CORE();

        // Since UDP PCB is released synchronously here, but UDP endpoint itself might have to wait
        // for destruction asynchronously, there could be more allocated UDP endpoints than UDP PCBs.
        if (mUDP != NULL)
            udp_remove(mUDP);
        mUDP = NULL;

        // Unlock LwIP stack
        UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

        if (mSocket != INET_INVALID_SOCKET_FD)
        {
            Weave::System::Layer& lSystemLayer = SystemLayer();

            // Wake the thread calling select so that it recognizes the socket is closed.
            lSystemLayer.WakeSelect();

            close(mSocket);
            mSocket = INET_INVALID_SOCKET_FD;
        }

        // Clear any results from select() that indicate pending I/O for the socket.
        mPendingIO.Clear();

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

        mState = kState_Closed;
    }
}

void UDPEndPoint::Free(void)
{
    Close();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    DeferredFree(kReleaseDeferralErrorTactic_Die);
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
    Release();
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP
}

INET_ERROR UDPEndPoint::SendTo(IPAddress addr, uint16_t port, Weave::System::PacketBuffer *msg, uint16_t sendFlags)
{
    return SendTo(addr, port, INET_NULL_INTERFACEID, msg, sendFlags);
}

INET_ERROR UDPEndPoint::SendTo(IPAddress addr, uint16_t port, InterfaceId intfId, Weave::System::PacketBuffer *msg, uint16_t sendFlags)
{
    INET_ERROR res = INET_NO_ERROR;

    INET_FAULT_INJECT(FaultInjection::kFault_Send,
            if ((sendFlags & kSendFlag_RetainBuffer) == 0)
                PacketBuffer::Free(msg);
            return INET_ERROR_UNKNOWN_INTERFACE;
            );
    INET_FAULT_INJECT(FaultInjection::kFault_SendNonCritical,
            if ((sendFlags & kSendFlag_RetainBuffer) == 0)
                PacketBuffer::Free(msg);
            return INET_ERROR_NO_MEMORY;
            );

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    if (sendFlags & kSendFlag_RetainBuffer)
    {
        // when retaining a buffer, the caller expects the msg to be
        // unmodified.  LwIP stack will normally prepend the packet
        // headers as the packet traverses the UDP/IP/netif layers,
        // which normally modifies the packet.  We prepend a small
        // pbuf to the beginning of the pbuf chain, s.t. all headers
        // are added to the temporary space, just large enough to hold
        // the transport headers. Careful reader will note:
        //
        // * we're actually oversizing the reserved space, the
        //   transport header is large enough for the TCP header which
        //   is larger than the UDP header, but it seemed cleaner than
        //   the combination of PBUF_IP for reserve space, UDP_HLEN
        //   for payload, and post allocation adjustment of the header
        //   space).
        //
        // * the code deviates from the existing PacketBuffer
        //   abstractions and needs to reach into the underlying pbuf
        //   code.  The code in PacketBuffer also forces us to perform
        //   (effectively) a reinterpret_cast rather than a
        //   static_cast.  JIRA WEAV-811 is filed to track the
        //   re-architecting of the memory management.

        pbuf *msgCopy = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_RAM);

        if (msgCopy == NULL)
        {
            return INET_ERROR_NO_MEMORY;
        }

        pbuf_chain(msgCopy, (pbuf *) msg);
        msg = (PacketBuffer *)msgCopy;
    }

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB based on the destination address.
    res = GetPCB(addr.Type());

    // Send the message to the specified address/port.
    if (res == INET_NO_ERROR)
    {
        err_t lwipErr = ERR_VAL;

#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
        ip_addr_t ipAddr = addr.ToLwIPAddr();
        if (intfId != INET_NULL_INTERFACEID)
            lwipErr = udp_sendto_if(mUDP, (pbuf *)msg, &ipAddr, port, intfId);
        else
            lwipErr = udp_sendto(mUDP, (pbuf *)msg, &ipAddr, port);
#else // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
        if (PCB_ISIPV6(mUDP))
        {
            ip6_addr_t ipv6Addr = addr.ToIPv6();
            if (intfId != INET_NULL_INTERFACEID)
                lwipErr = udp_sendto_if_ip6(mUDP, (pbuf *)msg, &ipv6Addr, port, intfId);
            else
                lwipErr = udp_sendto_ip6(mUDP, (pbuf *)msg, &ipv6Addr, port);
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            ip4_addr_t ipv4Addr = addr.ToIPv4();
            if (intfId != INET_NULL_INTERFACEID)
                lwipErr = udp_sendto_if(mUDP, (pbuf *)msg, &ipv4Addr, port, intfId);
            else
                lwipErr = udp_sendto(mUDP, (pbuf *)msg, &ipv4Addr, port);
        }
#endif // INET_CONFIG_ENABLE_IPV4
#endif // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5

        if (lwipErr != ERR_OK)
            res = Weave::System::MapErrorLwIP(lwipErr);
    }

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

    PacketBuffer::Free(msg);

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    // Make sure we have the appropriate type of socket based on the destination address.
    res = GetSocket(addr.Type());

    if (res == INET_NO_ERROR)
    {
        res = IPEndPointBasis::SendTo(addr, port, intfId, msg, sendFlags);
    }

    if ((sendFlags & kSendFlag_RetainBuffer) == 0)
        PacketBuffer::Free(msg);

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    WEAVE_SYSTEM_FAULT_INJECT_ASYNC_EVENT();

    return res;
}

//A lock is required because the LwIP thread may be referring to intf_filter,
//while this code running in the Inet application is potentially modifying it.
//NOTE: this only supports LwIP interfaces whose number is no bigger than 9.
INET_ERROR UDPEndPoint::BindInterface(IPAddressType addrType, InterfaceId intfId)
{
    INET_ERROR err = INET_NO_ERROR;

    if (mState != kState_Ready && mState != kState_Bound)
        return INET_ERROR_INCORRECT_STATE;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB.
    err = GetPCB(addrType);
    if (err == INET_NO_ERROR)
    {
        if ( !IsInterfaceIdPresent(intfId) )
        { //Stop interface-based filtering.
            mUDP->intf_filter = NULL;
        }
        else
        {
            struct netif *netifPtr;
            for (netifPtr = netif_list; netifPtr != NULL; netifPtr = netifPtr->next)
            {
                if (netifPtr == intfId)
                {
                    break;
                }
            }
            if (netifPtr == NULL)
            {
                err = INET_ERROR_UNKNOWN_INTERFACE;
            }
            else
            {
                mUDP->intf_filter = netifPtr;
            }
        }
    }

    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    // Make sure we have the appropriate type of socket.
    err = GetSocket(addrType);

    if (err == INET_NO_ERROR)
        err = IPEndPointBasis::BindInterface(addrType, intfId);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (err == INET_NO_ERROR)
        mState = kState_Bound;

    return err;
}

void UDPEndPoint::Init(InetLayer *inetLayer)
{
    IPEndPointBasis::Init(inetLayer);
}

InterfaceId UDPEndPoint::GetBoundInterface (void)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    return mUDP->intf_filter;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    return mBoundIntfId;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

void UDPEndPoint::HandleDataReceived(PacketBuffer *msg)
{
    if (mState == kState_Listening && OnMessageReceived != NULL)
    {
        IPPacketInfo *pktInfo = GetPacketInfo(msg);
        if (pktInfo != NULL)
        {
            IPPacketInfo pktInfoCopy = *pktInfo; // copy the address info so that the app can free the
                                                  // PacketBuffer without affecting access to address info.
            OnMessageReceived(this, msg, &pktInfoCopy);
        }
        else
        {
            if (OnReceiveError != NULL)
                OnReceiveError(this, INET_ERROR_INBOUND_MESSAGE_TOO_BIG, NULL);
            PacketBuffer::Free(msg);
        }
    }
    else
        PacketBuffer::Free(msg);
}

INET_ERROR UDPEndPoint::GetPCB(IPAddressType addrType)
{
    // IMMPORTANT: This method MUST be called with the LwIP stack LOCKED!

#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
    if (mUDP == NULL)
    {
        switch (addrType)
        {
        case kIPAddressType_IPv6:
#if INET_CONFIG_ENABLE_IPV4
        case kIPAddressType_IPv4:
#endif // INET_CONFIG_ENABLE_IPV4
            mUDP = udp_new_ip_type(IPAddress::ToLwIPAddrType(addrType));
            break;

        default:
            return INET_ERROR_WRONG_ADDRESS_TYPE;
        }

        if (mUDP == NULL)
        {
            WeaveLogError(Inet, "udp_new failed");
            return INET_ERROR_NO_MEMORY;
        }
    }
    else
    {
        switch (IP_GET_TYPE(&mUDP->local_ip))
        {
        case IPADDR_TYPE_V6:
            if (addrType != kIPAddressType_IPv6)
                return INET_ERROR_WRONG_ADDRESS_TYPE;
            break;

#if INET_CONFIG_ENABLE_IPV4
        case IPADDR_TYPE_V4:
            if (addrType != kIPAddressType_IPv4)
                return INET_ERROR_WRONG_ADDRESS_TYPE;
            break;
#endif // INET_CONFIG_ENABLE_IPV4

        default:
            break;
        }
    }
#else // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
    if (mUDP == NULL)
    {
        if (addrType == kIPAddressType_IPv6)
        {
            mUDP = udp_new_ip6();
            if (mUDP != NULL)
                ip_set_option(mUDP, SOF_REUSEADDR);
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4) {
            mUDP = udp_new();
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else
            return INET_ERROR_WRONG_ADDRESS_TYPE;

        if (mUDP == NULL) {
            WeaveLogError(Inet, "udp_new failed");
            return INET_ERROR_NO_MEMORY;
        }
    }
    else
    {
#if INET_CONFIG_ENABLE_IPV4
        const IPAddressType pcbType = PCB_ISIPV6(mUDP) ? kIPAddressType_IPv6 : kIPAddressType_IPv4;
#else // !INET_CONFIG_ENABLE_IPV4
        const IPAddressType pcbType = kIPAddressType_IPv6;
#endif // !INET_CONFIG_ENABLE_IPV4
        if (addrType != pcbType) {
            return INET_ERROR_WRONG_ADDRESS_TYPE;
        }
    }
#endif // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5

    return INET_NO_ERROR;
}

IPPacketInfo *UDPEndPoint::GetPacketInfo(PacketBuffer *buf)
{
    // When using LwIP information about the packet is 'hidden' in the reserved space before the start of
    // the data in the packet buffer.  This is necessary because the events in dolomite can only have two
    // arguments, which in this case are used to convey the pointer to the end point and the pointer to the
    // buffer.
    //
    // In most cases this trick of storing information before the data works because the first buffer in
    // an LwIP UDP message contains the space that was used for the Ethernet/IP/UDP headers. However, given
    // the current size of the IPPacketInfo structure (40 bytes), it is possible for there to not be enough
    // room to store the structure along with the payload in a single pbuf. In practice, this should only
    // happen for extremely large IPv4 packets that arrive without an Ethernet header.
    //
    if (!buf->EnsureReservedSize(sizeof(IPPacketInfo) + 3))
        return NULL;
    uintptr_t start = (uintptr_t)buf->Start();
    uintptr_t pktInfoStart = start - sizeof(IPPacketInfo);
    pktInfoStart = pktInfoStart & ~3;// align to 4-byte boundary
    return (IPPacketInfo *)pktInfoStart;
}

#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
void UDPEndPoint::LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
#else // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
void UDPEndPoint::LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
#endif // LWIP_VERSION_MAJOR <= 1 || LWIP_VERSION_MINOR >= 5
{
    UDPEndPoint*            ep              = static_cast<UDPEndPoint*>(arg);
    PacketBuffer*           buf             = reinterpret_cast<PacketBuffer*>(static_cast<void*>(p));
    Weave::System::Layer&   lSystemLayer    = ep->SystemLayer();

    IPPacketInfo *pktInfo = GetPacketInfo(buf);
    if (pktInfo != NULL)
    {
#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
        pktInfo->SrcAddress = IPAddress::FromLwIPAddr(*addr);
        pktInfo->DestAddress = IPAddress::FromLwIPAddr(*ip_current_dest_addr());
#else // LWIP_VERSION_MAJOR <= 1
        if (PCB_ISIPV6(pcb))
        {
            pktInfo->SrcAddress = IPAddress::FromIPv6(*(ip6_addr_t *)addr);
            pktInfo->DestAddress = IPAddress::FromIPv6(*ip6_current_dest_addr());
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            pktInfo->SrcAddress = IPAddress::FromIPv4(*addr);
            pktInfo->DestAddress = IPAddress::FromIPv4(*ip_current_dest_addr());
        }
#endif // INET_CONFIG_ENABLE_IPV4
#endif // LWIP_VERSION_MAJOR <= 1

        pktInfo->Interface = ip_current_netif();
        pktInfo->SrcPort = port;
        pktInfo->DestPort = pcb->local_port;
    }

    if (lSystemLayer.PostEvent(*ep, kInetEvent_UDPDataReceived, (uintptr_t)buf) != INET_NO_ERROR)
        PacketBuffer::Free(buf);
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
INET_ERROR UDPEndPoint::GetSocket(IPAddressType aAddressType)
{
    const int lType = (SOCK_DGRAM | SOCK_FLAGS);
    const int lProtocol = 0;

    return (IPEndPointBasis::GetSocket(aAddressType, lType, lProtocol));
}

SocketEvents UDPEndPoint::PrepareIO(void)
{
    SocketEvents res;

    if (mState == kState_Listening && OnMessageReceived != NULL)
        res.SetRead();

    return res;
}

void UDPEndPoint::HandlePendingIO(void)
{
    INET_ERROR      lStatus = INET_NO_ERROR;
    PacketBuffer *  lBuffer = NULL;
    IPPacketInfo    lPacketInfo;

    if (mState == kState_Listening && OnMessageReceived != NULL && mPendingIO.IsReadable())
    {
        const uint16_t lPort = mBoundPort;

        lStatus = IPEndPointBasis::HandlePendingIO(lPort, lBuffer, lPacketInfo);

        if (lStatus == INET_NO_ERROR)
            OnMessageReceived(this, lBuffer, &lPacketInfo);
        else
        {
            PacketBuffer::Free(lBuffer);
            if (OnReceiveError != NULL
                && lStatus != Weave::System::MapErrorPOSIX(EAGAIN)
            )
                OnReceiveError(this, lStatus, NULL);
        }
    }

    mPendingIO.Clear();
}

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl

/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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

// TODO: remove me
#include <stdio.h>

#include <InetLayer/UDPEndPoint.h>
#include <InetLayer/InetLayer.h>
#include <InetLayer/InetFaultInjection.h>

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
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

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
#if LWIP_VERSION_MAJOR > 1
        ip_addr_t ipAddr = addr.ToLwIPAddr();
        res = Weave::System::MapErrorLwIP(udp_bind(mUDP, &ipAddr, port));
#else // LWIP_VERSION_MAJOR <= 1
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
#endif // LWIP_VERSION_MAJOR <= 1
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
        if (addrType == kIPAddressType_IPv6)
        {
            struct sockaddr_in6 sa;
            memset(&sa, 0, sizeof(sa));
            sa.sin6_family = AF_INET6;
            sa.sin6_port = htons(port);
            sa.sin6_flowinfo = 0;
            sa.sin6_addr = addr.ToIPv6();
            sa.sin6_scope_id = intfId;

            if (res == INET_NO_ERROR && bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof(sa)) != 0)
                res = Weave::System::MapErrorPOSIX(errno);

            // Instruct the kernel that any messages to multicast destinations should be
            // sent down the interface specified by the caller.
#ifdef IPV6_MULTICAST_IF
            if (res == INET_NO_ERROR)
                setsockopt(mSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &intfId, sizeof(intfId));
#endif // defined(IPV6_MULTICAST_IF)
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4)
        {
            struct sockaddr_in sa;
            memset(&sa, 0, sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_port = htons(port);
            sa.sin_addr = addr.ToIPv4();

            if (bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof(sa)) != 0)
                res = Weave::System::MapErrorPOSIX(errno);

            // Instruct the kernel that any messages to multicast destinations should be
            // sent down the interface to which the specified IPv4 address is bound.
#ifdef IP_MULTICAST_IF
            if (res == INET_NO_ERROR)
                setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_IF, &sa, sizeof(sa));
#endif // defined(IP_MULTICAST_IF)
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else
            res = INET_ERROR_WRONG_ADDRESS_TYPE;

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

INET_ERROR UDPEndPoint::Listen()
{
    INET_ERROR res = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    Weave::System::Layer& lSystemLayer = SystemLayer();
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (mState == kState_Listening)
        return INET_NO_ERROR;

    if (mState != kState_Bound)
        return INET_ERROR_INCORRECT_STATE;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

#if LWIP_VERSION_MAJOR > 1
    udp_recv(mUDP, LwIPReceiveUDPMessage, this);
#else // LWIP_VERSION_MAJOR <= 1
    if (PCB_ISIPV6(mUDP))
        udp_recv_ip6(mUDP, LwIPReceiveUDPMessage, this);
    else
        udp_recv(mUDP, LwIPReceiveUDPMessage, this);
#endif // LWIP_VERSION_MAJOR <= 1

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // Wake the thread calling select so that it recognizes the new socket.
    lSystemLayer.WakeSelect();

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (res == INET_NO_ERROR)
        mState = kState_Listening;

    return res;
}

void UDPEndPoint::Close()
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

# if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

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

void UDPEndPoint::Free()
{
    Close();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    DeferredFree(kReleaseDeferralErrorTactic_Die);
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
    Release();
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP
}

INET_ERROR UDPEndPoint::SendTo(IPAddress addr, uint16_t port, PacketBuffer *msg, uint16_t sendFlags)
{
    return SendTo(addr, port, INET_NULL_INTERFACEID, msg, sendFlags);
}

INET_ERROR UDPEndPoint::SendTo(IPAddress addr, uint16_t port, InterfaceId intfId, PacketBuffer *msg, uint16_t sendFlags)
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

#if LWIP_VERSION_MAJOR > 1
        ip_addr_t ipAddr = addr.ToLwIPAddr();
        if (intfId != INET_NULL_INTERFACEID)
            lwipErr = udp_sendto_if(mUDP, (pbuf *)msg, &ipAddr, port, intfId);
        else
            lwipErr = udp_sendto(mUDP, (pbuf *)msg, &ipAddr, port);
#else // LWIP_VERSION_MAJOR <= 1
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
#endif // LWIP_VERSION_MAJOR <= 1

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

    // For now the entire message must fit within a single buffer.
    if (res == INET_NO_ERROR && msg->Next() != NULL)
        res = INET_ERROR_MESSAGE_TOO_LONG;

    if (res == INET_NO_ERROR)
    {
        struct iovec msgIOV;
        union
        {
            sockaddr any;
            sockaddr_in in;
            sockaddr_in6 in6;
        } peerSockAddr;
        uint8_t controlData[256];
        struct msghdr msgHeader;

        memset(&msgHeader, 0, sizeof(msgHeader));

        msgIOV.iov_base = msg->Start();
        msgIOV.iov_len = msg->DataLength();
        msgHeader.msg_iov = &msgIOV;
        msgHeader.msg_iovlen = 1;

        memset(&peerSockAddr, 0, sizeof(peerSockAddr));
        msgHeader.msg_name = &peerSockAddr;
        if (mAddrType == kIPAddressType_IPv6)
        {
            peerSockAddr.in6.sin6_family = AF_INET6;
            peerSockAddr.in6.sin6_port = htons(port);
            peerSockAddr.in6.sin6_flowinfo = 0;
            peerSockAddr.in6.sin6_addr = addr.ToIPv6();
            peerSockAddr.in6.sin6_scope_id = intfId;
            msgHeader.msg_namelen = sizeof(sockaddr_in6);
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            peerSockAddr.in.sin_family = AF_INET;
            peerSockAddr.in.sin_port = htons(port);
            peerSockAddr.in.sin_addr = addr.ToIPv4();
            msgHeader.msg_namelen = sizeof(sockaddr_in);
        }
#endif // INET_CONFIG_ENABLE_IPV4

        // If the endpoint has been bound to a particular interface, and the caller didn't supply
        // a specific interface to send on, use the bound interface. This appears to be necessary
        // for messages to multicast addresses, which under linux don't seem to get sent out the
        // correct inferface, despite the socket being bound.
        if (intfId == INET_NULL_INTERFACEID)
            intfId = mBoundIntfId;

        if (intfId != INET_NULL_INTERFACEID && false)
        {
#if defined(IP_PKTINFO) || defined(IPV6_PKTINFO)
            memset(controlData, 0, sizeof(controlData));
            msgHeader.msg_control = controlData;
            msgHeader.msg_controllen = sizeof(controlData);

            struct cmsghdr *controlHdr = CMSG_FIRSTHDR(&msgHeader);

#if INET_CONFIG_ENABLE_IPV4
#if defined(IP_PKTINFO)
            if (mAddrType == kIPAddressType_IPv4)
            {
                controlHdr->cmsg_level = IPPROTO_IP;
                controlHdr->cmsg_type = IP_PKTINFO;
                controlHdr->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));

                struct in_pktinfo *pktInfo = (struct in_pktinfo *)CMSG_DATA(controlHdr);
                pktInfo->ipi_ifindex = intfId;

                msgHeader.msg_controllen = CMSG_SPACE(sizeof(in_pktinfo));
            }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#if defined(IPV6_PKTINFO)
            if (mAddrType == kIPAddressType_IPv6)
            {
                controlHdr->cmsg_level = IPPROTO_IP;
                controlHdr->cmsg_type = IPV6_PKTINFO;
                controlHdr->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));

                struct in6_pktinfo *pktInfo = (struct in6_pktinfo *)CMSG_DATA(controlHdr);
                pktInfo->ipi6_ifindex = intfId;

                msgHeader.msg_controllen = CMSG_SPACE(sizeof(in6_pktinfo));
            }
#endif // defined(IPV6_PKTINFO)
#else // !(defined(IP_PKTINFO) && defined(IPV6_PKTINFO))
            res = INET_ERROR_NOT_IMPLEMENTED
#endif // !(defined(IP_PKTINFO) && defined(IPV6_PKTINFO))
        }

        if (res == INET_NO_ERROR)
        {
            // Send UDP packet.
//            ssize_t lenSent = sendmsg(mSocket, &msgHeader, 0);

            ssize_t lenSent = sendto(mSocket, msgHeader.msg_iov[0].iov_base, msgHeader.msg_iov[0].iov_len, 0, &peerSockAddr.any, msgHeader.msg_namelen);
            if (lenSent == -1)
                res = Weave::System::MapErrorPOSIX(errno);
            else if (lenSent != msg->DataLength())
                res = INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED;
        }
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
INET_ERROR UDPEndPoint::BindInterface(IPAddressType addrType, InterfaceId intf)
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
        if ( !IsInterfaceIdPresent(intf) )
        { //Stop interface-based filtering.
            mUDP->intf_filter = NULL;
        }
        else
        {
            struct netif *netifPtr;
            for (netifPtr = netif_list; netifPtr != NULL; netifPtr = netifPtr->next)
            {
                if (netifPtr == intf)
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
#if HAVE_SO_BINDTODEVICE

    // Make sure we have the appropriate type of socket.
    err = GetSocket(addrType);

    if (err == INET_NO_ERROR)
    {
        if (intf == INET_NULL_INTERFACEID)
        {//Stop interface-based filtering.
            if (setsockopt(mSocket, SOL_SOCKET, SO_BINDTODEVICE, "", 0) == -1)
            {
                err = Weave::System::MapErrorPOSIX(errno);
            }
        }
        else
        {    //Start filtering on the passed interface.
            char intfName[IF_NAMESIZE];
            if (if_indextoname(intf, intfName) == NULL)
            {
                err = Weave::System::MapErrorPOSIX(errno);
            }

            if (err == INET_NO_ERROR && setsockopt(mSocket, SOL_SOCKET, SO_BINDTODEVICE, intfName, strlen(intfName)) == -1)
            {
                err = Weave::System::MapErrorPOSIX(errno);
            }
        }
    }

    if (err == INET_NO_ERROR)
        mBoundIntfId = intf;

#else // !HAVE_SO_BINDTODEVICE

    err = INET_ERROR_NOT_IMPLEMENTED;

#endif // HAVE_SO_BINDTODEVICE
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (err == INET_NO_ERROR)
        mState = kState_Bound;

    return err;
}

void UDPEndPoint::Init(InetLayer *inetLayer)
{
    InitEndPointBasis(*inetLayer);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    mBoundIntfId = INET_NULL_INTERFACEID;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
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

#if LWIP_VERSION_MAJOR > 1
    if (mUDP == NULL)
    {
        switch (addrType)
        {
        case kIPAddressType_IPv6:
#if INET_CONFIG_ENABLE_IPV4
        case kIPAddressType_IPv4:
#endif // INET_CONFIG_ENABLE_IPV4
            mUDP = udp_new();
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
#else // LWIP_VERSION_MAJOR <= 1
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
#endif // LWIP_VERSION_MAJOR <= 1

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

#if LWIP_VERSION_MAJOR > 1
void UDPEndPoint::LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
#else // LWIP_VERSION_MAJOR <= 1
void UDPEndPoint::LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
#endif // LWIP_VERSION_MAJOR <= 1
{
    UDPEndPoint*            ep              = static_cast<UDPEndPoint*>(arg);
    PacketBuffer*           buf             = reinterpret_cast<PacketBuffer*>(static_cast<void*>(p));
    Weave::System::Layer&   lSystemLayer    = ep->SystemLayer();

    IPPacketInfo *pktInfo = GetPacketInfo(buf);
    if (pktInfo != NULL)
    {
#if LWIP_VERSION_MAJOR > 1
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

INET_ERROR UDPEndPoint::GetSocket(IPAddressType addrType)
{
    if (mSocket == INET_INVALID_SOCKET_FD)
    {
        int one = 1;
        int family;
        int res;

        if (addrType == kIPAddressType_IPv6)
            family = PF_INET6;
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4)
            family = PF_INET;
#endif // INET_CONFIG_ENABLE_IPV4
        else
            return INET_ERROR_WRONG_ADDRESS_TYPE;

        mSocket = ::socket(family, SOCK_DGRAM | SOCK_FLAGS, 0);
        if (mSocket == -1)
            return Weave::System::MapErrorPOSIX(errno);
        mAddrType = addrType;

        //
        // NOTE WELL: the errors returned by setsockopt() here are not returned as Inet layer
        // Weave::System::MapErrorPOSIX(errno) codes because they are normally expected to fail on some
        // platforms where the socket option code is defined in the header files but not [yet]
        // implemented. Certainly, there is room to improve this by connecting the build
        // configuration logic up to check for implementations of these options and to provide
        // appropriate HAVE_xxxxx definitions accordingly.
        //

        res = setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR,  (void*)&one, sizeof(one));
        static_cast<void>(res);

#ifdef SO_REUSEPORT
        res = setsockopt(mSocket, SOL_SOCKET, SO_REUSEPORT,  (void*)&one, sizeof(one));
        if (res != 0)
        {
            WeaveLogError(Inet, "SO_REUSEPORT: %d", errno);
        }
#endif // defined(SO_REUSEPORT)

        // If creating an IPv6 socket, tell the kernel that it will be IPv6 only.  This makes it
        // posible to bind two sockets to the same port, one for IPv4 and one for IPv6.
#ifdef IPV6_V6ONLY
#if INET_CONFIG_ENABLE_IPV4
        if (addrType == kIPAddressType_IPv6)
#endif // INET_CONFIG_ENABLE_IPV4
        {
            res = setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void *) &one, sizeof(one));
            if (res != 0)
            {
                WeaveLogError(Inet, "IPV6_V6ONLY: %d", errno);
            }
        }
#endif // defined(IPV6_V6ONLY)

#if INET_CONFIG_ENABLE_IPV4
#ifdef IP_PKTINFO
        res = setsockopt(mSocket, IPPROTO_IP, IP_PKTINFO, (void *) &one, sizeof(one));
        if (res != 0)
        {
            WeaveLogError(Inet, "IP_PKTINFO: %d", errno);
        }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#ifdef IPV6_RECVPKTINFO
        res = setsockopt(mSocket, IPPROTO_IPV6, IPV6_RECVPKTINFO, (void *) &one, sizeof(one));
        if (res != 0)
        {
            WeaveLogError(Inet, "IPV6_PKTINFO: %d", errno);
        }
#endif // defined(IPV6_RECVPKTINFO)

        // On systems that support it, disable the delivery of SIGPIPE signals when writing to a closed
        // socket.  This is mostly needed on iOS which has the peculiar habit of sending SIGPIPEs on
        // unconnected UDP sockets.
#ifdef SO_NOSIGPIPE
        {
            one = 1;
            res = setsockopt(mSocket, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
            if (res != 0)
            {
                WeaveLogError(Inet, "SO_NOSIGPIPE: %d", errno);
            }
        }
#endif // defined(SO_NOSIGPIPE)

    }
    else if (mAddrType != addrType)
        return INET_ERROR_INCORRECT_STATE;

    return INET_NO_ERROR;
}

SocketEvents UDPEndPoint::PrepareIO()
{
    SocketEvents res;

    if (mState == kState_Listening && OnMessageReceived != NULL)
        res.SetRead();

    return res;
}

void UDPEndPoint::HandlePendingIO()
{
    INET_ERROR err = INET_NO_ERROR;

    if (mState == kState_Listening && OnMessageReceived != NULL && mPendingIO.IsReadable())
    {
        IPPacketInfo pktInfo;
        pktInfo.Clear();
        pktInfo.DestPort = mBoundPort;

        PacketBuffer *buf = PacketBuffer::New(0);

        if (buf != NULL)
        {
            struct iovec msgIOV;
            union
            {
                sockaddr any;
                sockaddr_in in;
                sockaddr_in6 in6;
            } peerSockAddr;
            uint8_t controlData[256];
            struct msghdr msgHeader;

            msgIOV.iov_base = buf->Start();
            msgIOV.iov_len = buf->AvailableDataLength();

            memset(&peerSockAddr, 0, sizeof(peerSockAddr));

            memset(&msgHeader, 0, sizeof(msgHeader));
            msgHeader.msg_name = &peerSockAddr;
            msgHeader.msg_namelen = sizeof(peerSockAddr);
            msgHeader.msg_iov = &msgIOV;
            msgHeader.msg_iovlen = 1;
            msgHeader.msg_control = controlData;
            msgHeader.msg_controllen = sizeof(controlData);

            ssize_t rcvLen = recvmsg(mSocket, &msgHeader, MSG_DONTWAIT);

            if (rcvLen < 0)
                err = Weave::System::MapErrorPOSIX(errno);

            else if (rcvLen > buf->AvailableDataLength())
                err = INET_ERROR_INBOUND_MESSAGE_TOO_BIG;

            else
            {
                buf->SetDataLength((uint16_t) rcvLen);

                if (peerSockAddr.any.sa_family == AF_INET6)
                {
                    pktInfo.SrcAddress = IPAddress::FromIPv6(peerSockAddr.in6.sin6_addr);
                    pktInfo.SrcPort = ntohs(peerSockAddr.in6.sin6_port);
                }
#if INET_CONFIG_ENABLE_IPV4
                else if (peerSockAddr.any.sa_family == AF_INET)
                {
                    pktInfo.SrcAddress = IPAddress::FromIPv4(peerSockAddr.in.sin_addr);
                    pktInfo.SrcPort = ntohs(peerSockAddr.in.sin_port);
                }
#endif // INET_CONFIG_ENABLE_IPV4
                else
                    err = INET_ERROR_INCORRECT_STATE;
            }

            if (err == INET_NO_ERROR)
            {
                for (struct cmsghdr *controlHdr = CMSG_FIRSTHDR(&msgHeader);
                     controlHdr != NULL;
                     controlHdr = CMSG_NXTHDR(&msgHeader, controlHdr))
                {
#if INET_CONFIG_ENABLE_IPV4
#ifdef IP_PKTINFO
                    if (controlHdr->cmsg_level == IPPROTO_IP && controlHdr->cmsg_type == IP_PKTINFO)
                    {
                        struct in_pktinfo *inPktInfo = (struct in_pktinfo *)CMSG_DATA(controlHdr);
                        pktInfo.Interface = inPktInfo->ipi_ifindex;
                        pktInfo.DestAddress = IPAddress::FromIPv4(inPktInfo->ipi_addr);
                        continue;
                    }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#ifdef IPV6_PKTINFO
                    if (controlHdr->cmsg_level == IPPROTO_IPV6 && controlHdr->cmsg_type == IPV6_PKTINFO)
                    {
                        struct in6_pktinfo *in6PktInfo = (struct in6_pktinfo *)CMSG_DATA(controlHdr);
                        pktInfo.Interface = in6PktInfo->ipi6_ifindex;
                        pktInfo.DestAddress = IPAddress::FromIPv6(in6PktInfo->ipi6_addr);
                        continue;
                    }
#endif // defined(IPV6_PKTINFO)
                }
            }
        }

        else
            err = INET_ERROR_NO_MEMORY;

        if (err == INET_NO_ERROR)
            OnMessageReceived(this, buf, &pktInfo);
        else
        {
            PacketBuffer::Free(buf);
            if (OnReceiveError != NULL
                && err != Weave::System::MapErrorPOSIX(EAGAIN)
            )
                OnReceiveError(this, err, NULL);
        }
    }

    mPendingIO.Clear();
}

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl

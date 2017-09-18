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
 *      This file implements the <tt>nl::Inet::RawEndPoint</tt> class,
 *      where the Nest Inet Layer encapsulates methods for interacting
 *      with PF_RAW sockets (on Linux and BSD-derived systems) or LwIP
 *      raw protocol control blocks, as the system is configured
 *      accordingly.
 *
 */

#include <string.h>
#include <stdio.h>

#include <InetLayer/RawEndPoint.h>
#include <InetLayer/InetLayer.h>

#include <Weave/Support/CodeUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/ip.h>
#include <lwip/tcpip.h>
#include <lwip/raw.h>
#include <lwip/netif.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#if HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif // HAVE_NETINET_ICMP6_H
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif // HAVE_SYS_SOCKET_H
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

// SOCK_CLOEXEC not defined on all platforms, e.g. iOS/MacOS:
#ifdef SOCK_CLOEXEC
#define SOCK_FLAGS SOCK_CLOEXEC
#else
#define SOCK_FLAGS 0
#endif

namespace nl {
namespace Inet {

class SenderInfo
{
public:
    IPAddress Address;
};

Weave::System::ObjectPool<RawEndPoint, INET_CONFIG_NUM_RAW_ENDPOINTS> RawEndPoint::sPool;

/**
 * Bind the raw endpoint to an IP address of the specified type.
 *
 * @param[in]   addrType    An IPAddressType to identify the type of the address.
 *
 * @param[in]   addr        An IPAddress object required to be of the specified type.
 *
 * @return INET_NO_ERROR on success, or a mapped OS error on failure. An invalid
 * parameter list can result in INET_ERROR_WRONG_ADDRESS_TYPE. If the raw endpoint
 * is already bound or is listening, then returns INET_ERROR_INCORRECT_STATE.
 */
INET_ERROR RawEndPoint::Bind(IPAddressType addrType, IPAddress addr)
{
    INET_ERROR res;
    IPAddressType addrTypeInfer;

    if (mState != kState_Closed)
        return INET_ERROR_INCORRECT_STATE;

    addrTypeInfer = addr.Type();
    if (addrTypeInfer != kIPAddressType_Any && addrType != addrTypeInfer)
        return INET_ERROR_WRONG_ADDRESS_TYPE;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB.
    res = GetPCB();

    // Bind the PCB to the specified address.
    if (res == INET_NO_ERROR)
    {
#if LWIP_VERSION_MAJOR > 1
        ip_addr_t ipAddr = addr.ToLwIPAddr();

        if (IP_GET_TYPE(&ipAddr) == IPADDR_TYPE_ANY)
            res = INET_ERROR_WRONG_ADDRESS_TYPE;
        else
            res = Weave::System::MapErrorLwIP(raw_bind(mRaw, &ipAddr));
#else // LWIP_VERSION_MAJOR <= 1
        if (addrType == kIPAddressType_IPv6)
        {
            ip6_addr_t ipv6Addr = addr.ToIPv6();
            res = Weave::System::MapErrorLwIP(raw_bind_ip6(mRaw, &ipv6Addr));
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4)
        {
            ip_addr_t ipv4Addr = addr.ToIPv4();
            res = Weave::System::MapErrorLwIP(raw_bind(mRaw, &ipv4Addr));
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else
            res = INET_ERROR_WRONG_ADDRESS_TYPE;
#endif // LWIP_VERSION_MAJOR <= 1
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
            sa.sin6_family = AF_INET6;
            sa.sin6_flowinfo = 0;
            sa.sin6_addr = addr.ToIPv6();
            sa.sin6_scope_id = 0;

            if (bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof(sa)) != 0)
                res = Weave::System::MapErrorPOSIX(errno);
        }
#if INET_CONFIG_ENABLE_IPV4
        else if (addrType == kIPAddressType_IPv4)
        {
            struct sockaddr_in sa;
            sa.sin_family = AF_INET;
            sa.sin_addr = addr.ToIPv4();

            if (bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof(sa)) != 0)
                res = Weave::System::MapErrorPOSIX(errno);
        }
#endif // INET_CONFIG_ENABLE_IPV4
        else
            res = INET_ERROR_WRONG_ADDRESS_TYPE;
    }

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (res == INET_NO_ERROR)
        mState = kState_Bound;

    return res;
}

/**
 * Bind the raw endpoint to an IPv6 link-local scope address at the specified
 * interface index.  Also sets various IPv6 socket options appropriate for
 * transmitting packets to and from on-link destinations.
 *
 * @param[in]   intf    An InterfaceId to identify the scope of the address.
 *
 * @param[in]   addr    An IPv6 link-local scope IPAddress object.
 *
 * @return INET_NO_ERROR on success, or a mapped OS error on failure. An invalid
 * parameter list can result in INET_ERROR_WRONG_ADDRESS_TYPE. If the raw endpoint
 * is already bound or is listening, then returns INET_ERROR_INCORRECT_STATE.
 */
INET_ERROR RawEndPoint::BindIPv6LinkLocal(InterfaceId intf, IPAddress addr)
{
    INET_ERROR res = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    const int lIfIndex = static_cast<int>(intf);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (!addr.IsIPv6LinkLocal())
    {
        res = INET_ERROR_WRONG_ADDRESS_TYPE;
        goto ret;
    }

    if (mState != kState_Closed)
    {
        res = INET_ERROR_INCORRECT_STATE;
        goto ret;
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB.
    res = GetPCB();

    // Bind the PCB to the specified address.
    if (res == INET_NO_ERROR)
    {
#if LWIP_VERSION_MAJOR > 1
        ip_addr_t ipAddr = addr.ToLwIPAddr();
        res = Weave::System::MapErrorLwIP(raw_bind(mRaw, &ipAddr));
#else // LWIP_VERSION_MAJOR <= 1
        ip6_addr_t ipv6Addr = addr.ToIPv6();
        res = Weave::System::MapErrorLwIP(raw_bind_ip6(mRaw, &ipv6Addr));
#endif // LWIP_VERSION_MAJOR <= 1

        if (res != INET_NO_ERROR)
        {
            raw_remove(mRaw);
            mRaw = NULL;
        }
    }

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    static const int sInt255 = 255;

    // Make sure we have the appropriate type of socket.
    res = GetSocket(kIPAddressType_IPv6);
    if (res != INET_NO_ERROR)
    {
        goto ret;
    }

    if (::setsockopt(mSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &lIfIndex, sizeof(lIfIndex)) != 0)
    {
        goto optfail;
    }

    if (::setsockopt(mSocket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &sInt255, sizeof(sInt255)) != 0)
    {
        goto optfail;
    }

    if (::setsockopt(mSocket, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &sInt255, sizeof(sInt255)) != 0)
    {
        goto optfail;
    }

    mAddrType = kIPAddressType_IPv6;
    goto ret;

optfail:
    res = Weave::System::MapErrorPOSIX(errno);
    ::close(mSocket);
    mSocket = INET_INVALID_SOCKET_FD;
    mAddrType = kIPAddressType_Unknown;

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

ret:
    if (res == INET_NO_ERROR)
    {
        mState = kState_Bound;
    }

    return res;
}

INET_ERROR RawEndPoint::Listen()
{
    INET_ERROR res = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    Weave::System::Layer& lSystemLayer = SystemLayer();
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (mState == kState_Listening)
        return INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

#if LWIP_VERSION_MAJOR > 1
    raw_recv(mRaw, LwIPReceiveRawMessage, this);
#else // LWIP_VERSION_MAJOR <= 1
    if (PCB_ISIPV6(mRaw))
        raw_recv_ip6(mRaw, LwIPReceiveRawMessage, this);
    else
        raw_recv(mRaw, LwIPReceiveRawMessage, this);
#endif // LWIP_VERSION_MAJOR <= 1

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

void RawEndPoint::Close()
{
    if (mState != kState_Closed)
    {
#if WEAVE_SYSTEM_CONFIG_USE_LWIP

        // Lock LwIP stack
        LOCK_TCPIP_CORE();

        if (mRaw != NULL)
        raw_remove(mRaw);
        mRaw = NULL;

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

void RawEndPoint::Free()
{
    Close();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    DeferredFree(kReleaseDeferralErrorTactic_Release);
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
    Release();
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP
}

INET_ERROR RawEndPoint::SendTo(IPAddress addr, Weave::System::PacketBuffer *msg)
{
    INET_ERROR res = INET_NO_ERROR;

#if INET_CONFIG_ENABLE_IPV4
    if (IPVer == kIPVersion_4 && addr.Type() != kIPAddressType_IPv4)
    {
        res = INET_ERROR_WRONG_ADDRESS_TYPE;
        goto finalize;
    }
#endif // INET_CONFIG_ENABLE_IPV4

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Lock LwIP stack
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB based on the destination address.
    res = GetPCB();

    // Send the message to the specified address.
    if (res == INET_NO_ERROR)
    {
        err_t lwipErr = ERR_OK;

#if LWIP_VERSION_MAJOR > 1
        ip_addr_t ipAddr = addr.ToLwIPAddr();
        lwipErr = raw_sendto(mRaw, (pbuf *)msg, &ipAddr);
#else // LWIP_VERSION_MAJOR <= 1
        if (PCB_ISIPV6(mRaw))
        {
            ip6_addr_t ipv6Addr = addr.ToIPv6();
            lwipErr = raw_sendto_ip6(mRaw, (pbuf *)msg, &ipv6Addr);
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            ip_addr_t ipv4Addr = addr.ToIPv4();
            lwipErr = raw_sendto(mRaw, (pbuf *)msg, &ipv4Addr);
        }
#endif // INET_CONFIG_ENABLE_IPV4
#endif // LWIP_VERSION_MAJOR <= 1

        if (lwipErr != ERR_OK)
            res = Weave::System::MapErrorLwIP(lwipErr);
    }

    // Unlock LwIP stack
    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // Make sure we have the appropriate type of socket based on the destination address.
    res = GetSocket(addr.Type());

    // For now the entire message must fit within a single buffer.
    if (res == INET_NO_ERROR && msg->Next() != NULL)
        res = INET_ERROR_MESSAGE_TOO_LONG;

    if (res == INET_NO_ERROR)
    {
        union
        {
            sockaddr any;
            sockaddr_in in;
            sockaddr_in6 in6;
        } sa;

        if (mAddrType == kIPAddressType_IPv6)
        {
            sa.in6.sin6_family = AF_INET6;
            sa.in6.sin6_port = 0; //FIXME
            sa.in6.sin6_flowinfo = 0;
            sa.in6.sin6_addr = addr.ToIPv6();
            sa.in6.sin6_scope_id = 0;
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            sa.in.sin_family = AF_INET;
            sa.in.sin_port = 0; //FIXME
            sa.in.sin_addr = addr.ToIPv4();
        }
#endif // INET_CONFIG_ENABLE_IPV4

        uint16_t msgLen = msg->DataLength();

        // Send Raw packet.
        ssize_t lenSent = sendto(mSocket, msg->Start(), msgLen, 0, &sa.any, sizeof(sa));
        if (lenSent == -1)
            res = Weave::System::MapErrorPOSIX(errno);
        else if (lenSent != msgLen)
            res = INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED;
    }

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if INET_CONFIG_ENABLE_IPV4
finalize:
#endif // INET_CONFIG_ENABLE_IPV4

    Weave::System::PacketBuffer::Free(msg);
    return res;
}

INET_ERROR RawEndPoint::SetICMPFilter(uint8_t numICMPTypes, const uint8_t * aICMPTypes)
{
    INET_ERROR err;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if !(HAVE_NETINET_ICMP6_H && HAVE_ICMP6_FILTER)
    err = INET_ERROR_NOT_IMPLEMENTED;
    ExitNow();
#endif //!(HAVE_NETINET_ICMP6_H && HAVE_ICMP6_FILTER)
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    VerifyOrExit(IPVer == kIPVersion_6, err = INET_ERROR_WRONG_ADDRESS_TYPE);
    VerifyOrExit(IPProto == kIPProtocol_ICMPv6, err = INET_ERROR_WRONG_PROTOCOL_TYPE);
    VerifyOrExit((numICMPTypes == 0 && aICMPTypes == NULL) || (numICMPTypes != 0 && aICMPTypes != NULL), err =
        INET_ERROR_BAD_ARGS);

    err = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    LOCK_TCPIP_CORE();
    NumICMPTypes = numICMPTypes;
    ICMPTypes = aICMPTypes;
    UNLOCK_TCPIP_CORE();
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if HAVE_NETINET_ICMP6_H && HAVE_ICMP6_FILTER
    struct icmp6_filter filter;
    if (numICMPTypes > 0)
    {
        ICMP6_FILTER_SETBLOCKALL(&filter);
        for (int j = 0; j < numICMPTypes; ++j)
        {
            ICMP6_FILTER_SETPASS(aICMPTypes[j], &filter);
        }
    }
    else
    {
        ICMP6_FILTER_SETPASSALL(&filter);
    }
    if (setsockopt(mSocket, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter)) == -1)
    {
        err = Weave::System::MapErrorPOSIX(errno);
    }
#endif // HAVE_NETINET_ICMP6_H && HAVE_ICMP6_FILTER
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

exit:
    return err;
}

//A lock is required because the LwIP thread may be referring to intf_filter,
//while this code running in the Inet application is potentially modifying it.
//NOTE: this only supports LwIP interfaces whose number is no bigger than 9.
INET_ERROR RawEndPoint::BindInterface(InterfaceId intf)
{
    INET_ERROR err = INET_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    LOCK_TCPIP_CORE();

    // Make sure we have the appropriate type of PCB.
    err = GetPCB();
    if (err == INET_NO_ERROR)
    {
        if ( !IsInterfaceIdPresent(intf) )
        { //Stop interface-based filtering.
            mRaw->intf_filter = NULL;
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
                mRaw->intf_filter = netifPtr;
            }
        }
    }

    UNLOCK_TCPIP_CORE();

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if HAVE_SO_BINDTODEVICE

    if ( !IsInterfaceIdPresent(intf) )
    {//Stop interface-based filtering.
        if (setsockopt(mSocket, SOL_SOCKET, SO_BINDTODEVICE, "", 0) == -1)
        {
            err = Weave::System::MapErrorPOSIX(errno);
        }
    }
    else
    {//Start filtering on the passed interface.
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

#else // !HAVE_SO_BINDTODEVICE

    err = INET_ERROR_NOT_IMPLEMENTED;

#endif // HAVE_SO_BINDTODEVICE
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (err == INET_NO_ERROR)
        mState = kState_Bound;

    return err;
}

void RawEndPoint::Init(InetLayer *inetLayer, IPVersion ipVer, IPProtocol ipProto)
{
    InitEndPointBasis(*inetLayer);

    IPVer = ipVer;
    IPProto = ipProto;
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP

void RawEndPoint::HandleDataReceived(Weave::System::PacketBuffer *msg)
{
    if (mState == kState_Listening && OnMessageReceived != NULL)
    {
        SenderInfo *senderInfo = GetSenderInfo(msg);
        OnMessageReceived(this, msg, senderInfo->Address);
    }
    else
        Weave::System::PacketBuffer::Free(msg);
}

INET_ERROR RawEndPoint::GetPCB()
{
    // IMPORTANT: This method MUST be called with the LwIP stack LOCKED!

    if (mRaw == NULL)
    {
        if (IPVer == kIPVersion_6)
            mRaw = raw_new_ip6(IPProto);
#if INET_CONFIG_ENABLE_IPV4
        else if (IPVer == kIPVersion_4)
            mRaw = raw_new(IPProto);
#endif // INET_CONFIG_ENABLE_IPV4
        else
            return INET_ERROR_WRONG_ADDRESS_TYPE;
        if (mRaw == NULL)
            return INET_ERROR_NO_MEMORY;
    }

    return INET_NO_ERROR;
}

SenderInfo *RawEndPoint::GetSenderInfo(Weave::System::PacketBuffer *buf)
{
    // When using LwIP information about the sender is 'hidden' in the reserved space before the start of
    // the data in the packet buffer.  This is necessary because the events in dolomite can only have two
    // arguments, which in this case are used  to convey the pointer to the end point and the pointer to the
    // buffer.  This trick of storing information before the data works because the first buffer in an LwIP
    // Raw message contains the space that was used for the IP headers, which is always bigger than the
    // SenderInfo structure.
    uintptr_t p = (uintptr_t)buf->Start();
    p = p - sizeof(SenderInfo);
    p = p & ~7;// align to 8-byte boundary
    return (SenderInfo *)p;
}

/* This function is executed when a raw_pcb is listening and an IP datagram (v4 or v6) is received.
 * NOTE: currently ICMPv4 filtering is currently not implemented, but it can easily be added later.
 * This fn() may be executed concurrently with SetICMPFilter()
 * - this fn() runs in the LwIP thread (and the lock has already been taken)
 * - SetICMPFilter() runs in the Inet thread.
 */
#if LWIP_VERSION_MAJOR > 1
u8_t RawEndPoint::LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
#else // LWIP_VERSION_MAJOR <= 1
u8_t RawEndPoint::LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)
#endif // LWIP_VERSION_MAJOR <= 1
{
    RawEndPoint *ep = (RawEndPoint *)arg;
    Weave::System::PacketBuffer *buf = (Weave::System::PacketBuffer *)p;
    uint8_t enqueue = 1;

    //Filtering based on the saved ICMP6 types (the only protocol currently supported.)
    if ((ep->IPVer == kIPVersion_6) &&
        (ep->IPProto == kIPProtocol_ICMPv6))
    {
        if (ep->NumICMPTypes > 0)
        { //When no filter is defined, let all ICMPv6 packets pass
          //The type is the first 8 bits field of an ICMP (v4 or v6) packet
            uint8_t icmp_type = *(buf->Start() + ip_current_header_tot_len());
            uint8_t icmp_type_found = 0;
            for (int j = 0; j < ep->NumICMPTypes; ++j)
            {
                if (ep->ICMPTypes[j] == icmp_type)
                {
                    icmp_type_found = 1;
                    break;
                }
            }
            if ( !icmp_type_found )
            {
                enqueue = 0;            //do not eat it
            }
        }
    }
    if (enqueue)
    {
        Weave::System::Layer& lSystemLayer = ep->SystemLayer();

        buf->SetStart(buf->Start() + ip_current_header_tot_len());
        SenderInfo *senderInfo = GetSenderInfo(buf);

#if LWIP_VERSION_MAJOR > 1
        senderInfo->Address = IPAddress::FromLwIPAddr(*addr);
#else // LWIP_VERSION_MAJOR <= 1
        if (PCB_ISIPV6(pcb))
        {
            senderInfo->Address = IPAddress::FromIPv6(*(ip6_addr_t *)addr);
        }
#if INET_CONFIG_ENABLE_IPV4
        else
        {
            senderInfo->Address = IPAddress::FromIPv4(*addr);
        }
#endif // INET_CONFIG_ENABLE_IPV4
#endif // LWIP_VERSION_MAJOR <= 1

        if (lSystemLayer.PostEvent(*ep, kInetEvent_RawDataReceived, (uintptr_t)buf) != INET_NO_ERROR)
            Weave::System::PacketBuffer::Free(buf);
    }
    return enqueue;
}

#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

INET_ERROR RawEndPoint::GetSocket(IPAddressType addrType)
{
    int sock, pf, proto;

    if (mState == kState_Closed)
    {
        switch (addrType)
        {
        case kIPAddressType_IPv6:
            pf = PF_INET6;
            proto = IPPROTO_ICMPV6;
            break;

#if INET_CONFIG_ENABLE_IPV4
        case kIPAddressType_IPv4:
            pf = PF_INET;
            proto = IPPROTO_ICMP;
            break;
#endif // INET_CONFIG_ENABLE_IPV4

        default:
            return INET_ERROR_WRONG_ADDRESS_TYPE;
        }

        sock = ::socket(pf, SOCK_RAW | SOCK_FLAGS, proto);
        if (sock == -1)
            return Weave::System::MapErrorPOSIX(errno);

        mSocket = sock;
        mAddrType = addrType;
    }

    return INET_NO_ERROR;
}

SocketEvents RawEndPoint::PrepareIO()
{
    SocketEvents res;

    if (mState == kState_Listening && OnMessageReceived != NULL)
        res.SetRead();

    return res;
}

void RawEndPoint::HandlePendingIO()
{
    INET_ERROR err = INET_NO_ERROR;

    if (mState == kState_Listening && OnMessageReceived != NULL && mPendingIO.IsReadable())
    {
        IPAddress senderAddr = IPAddress::Any;

        Weave::System::PacketBuffer *buf = Weave::System::PacketBuffer::New(0);

        if (buf != NULL)
        {
            union
            {
                sockaddr any;
                sockaddr_in in;
                sockaddr_in6 in6;
            } sa;
            memset(&sa, 0, sizeof(sa));
            socklen_t saLen = sizeof(sa);

            ssize_t rcvLen = recvfrom(mSocket, buf->Start(), buf->AvailableDataLength(), 0, &sa.any, &saLen);
            if (rcvLen < 0)
                err = Weave::System::MapErrorPOSIX(errno);

            else if (rcvLen > buf->AvailableDataLength())
                err = INET_ERROR_INBOUND_MESSAGE_TOO_BIG;

            else
            {
                buf->SetDataLength((uint16_t) rcvLen);

                if (sa.any.sa_family == AF_INET6)
                {
                    senderAddr = IPAddress::FromIPv6(sa.in6.sin6_addr);
                }
#if INET_CONFIG_ENABLE_IPV4
                else if (sa.any.sa_family == AF_INET)
                {
                    senderAddr = IPAddress::FromIPv4(sa.in.sin_addr);
                }
#endif // INET_CONFIG_ENABLE_IPV4
                else
                    err = INET_ERROR_INCORRECT_STATE;
            }
        }

        else
            err = INET_ERROR_NO_MEMORY;

        if (err == INET_NO_ERROR)
            OnMessageReceived(this, buf, senderAddr); //FIXME
        else
        {
            Weave::System::PacketBuffer::Free(buf);
            if (OnReceiveError != NULL)
                OnReceiveError(this, err, senderAddr); //FIXME
        }
    }

    mPendingIO.Clear();
}

#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl

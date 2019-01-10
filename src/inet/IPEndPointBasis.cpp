/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This header file implements the <tt>nl::Inet::IPEndPointBasis</tt>
 *      class, an intermediate, non-instantiable basis class
 *      supporting other IP-based end points.
 *
 */

#include <InetLayer/IPEndPointBasis.h>

#include <string.h>

#include <InetLayer/EndPointBasis.h>
#include <InetLayer/InetInterface.h>
#include <InetLayer/InetLayer.h>

#include <Weave/Support/CodeUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#include <lwip/ip.h>
#include <lwip/netif.h>
#include <lwip/udp.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif // HAVE_SYS_SOCKET_H
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

namespace nl {
namespace Inet {

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
union PeerSockAddr
{
    sockaddr     any;
    sockaddr_in  in;
    sockaddr_in6 in6;
};
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

using Weave::System::PacketBuffer;

void IPEndPointBasis::Init(InetLayer *aInetLayer)
{
    InitEndPointBasis(*aInetLayer);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    mBoundIntfId = INET_NULL_INTERFACEID;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
void IPEndPointBasis::HandleDataReceived(PacketBuffer *aBuffer)
{
    if ((mState == kState_Listening) && (OnMessageReceived != NULL))
    {
        const IPPacketInfo *pktInfo = GetPacketInfo(aBuffer);

        if (pktInfo != NULL)
        {
            const IPPacketInfo pktInfoCopy = *pktInfo;  // copy the address info so that the app can free the
                                                        // PacketBuffer without affecting access to address info.
            OnMessageReceived(this, aBuffer, &pktInfoCopy);
        }
        else
        {
            if (OnReceiveError != NULL)
                OnReceiveError(this, INET_ERROR_INBOUND_MESSAGE_TOO_BIG, NULL);
            PacketBuffer::Free(aBuffer);
        }
    }
    else
    {
        PacketBuffer::Free(aBuffer);
    }
}

/**
 *  @brief Get LwIP IP layer source and destination addressing information.
 *
 *  @param[in]   aBuffer       the packet buffer containing the IP message
 *
 *  @returns  a pointer to the address information on success; otherwise,
 *            NULL if there is insufficient space in the packet for
 *            the address information.
 *
 *  @details
 *     When using LwIP information about the packet is 'hidden' in the
 *     reserved space before the start of the data in the packet
 *     buffer. This is necessary because the system layer events only
 *     have two arguments, which in this case are used to convey the
 *     pointer to the end point and the pointer to the buffer.
 *
 *     In most cases this trick of storing information before the data
 *     works because the first buffer in an LwIP IP message contains
 *     the space that was used for the Ethernet/IP/UDP headers. However,
 *     given the current size of the IPPacketInfo structure (40 bytes),
 *     it is possible for there to not be enough room to store the
 *     structure along with the payload in a single packet buffer. In
 *     practice, this should only happen for extremely large IPv4
 *     packets that arrive without an Ethernet header.
 *
 */
IPPacketInfo *IPEndPointBasis::GetPacketInfo(PacketBuffer *aBuffer)
{
    uintptr_t       lStart;
    uintptr_t       lPacketInfoStart;
    IPPacketInfo *  lPacketInfo = NULL;

    if (!aBuffer->EnsureReservedSize(sizeof (IPPacketInfo) + 3))
        goto done;

    lStart           = (uintptr_t)aBuffer->Start();
    lPacketInfoStart = lStart - sizeof (IPPacketInfo);

    // Align to a 4-byte boundary

    lPacketInfo      = reinterpret_cast<IPPacketInfo *>(lPacketInfoStart & ~(sizeof (uint32_t) - 1));

 done:
    return (lPacketInfo);
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
INET_ERROR IPEndPointBasis::Bind(IPAddressType aAddressType, IPAddress aAddress, uint16_t aPort, InterfaceId aInterfaceId)
{
    INET_ERROR lRetval = INET_NO_ERROR;

    if (aAddressType == kIPAddressType_IPv6)
    {
        struct sockaddr_in6 sa;

        memset(&sa, 0, sizeof (sa));

        sa.sin6_family   = AF_INET6;
        sa.sin6_port     = htons(aPort);
        sa.sin6_flowinfo = 0;
        sa.sin6_addr     = aAddress.ToIPv6();
        sa.sin6_scope_id = aInterfaceId;

        if (bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof (sa)) != 0)
            lRetval = Weave::System::MapErrorPOSIX(errno);

        // Instruct the kernel that any messages to multicast destinations should be
        // sent down the interface specified by the caller.
#ifdef IPV6_MULTICAST_IF
        if (lRetval == INET_NO_ERROR)
            setsockopt(mSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &aInterfaceId, sizeof (aInterfaceId));
#endif // defined(IPV6_MULTICAST_IF)

        // Instruct the kernel that any messages to multicast destinations should be
        // set with the configured hop limit value.
#ifdef IPV6_MULTICAST_HOPS
        int hops = INET_CONFIG_IP_MULTICAST_HOP_LIMIT;
        setsockopt(mSocket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof (hops));
#endif // defined(IPV6_MULTICAST_HOPS)
    }
#if INET_CONFIG_ENABLE_IPV4
    else if (aAddressType == kIPAddressType_IPv4)
    {
        struct sockaddr_in sa;
        int enable = 1;

        memset(&sa, 0, sizeof (sa));

        sa.sin_family = AF_INET;
        sa.sin_port   = htons(aPort);
        sa.sin_addr   = aAddress.ToIPv4();

        if (bind(mSocket, (const sockaddr *) &sa, (unsigned) sizeof (sa)) != 0)
            lRetval = Weave::System::MapErrorPOSIX(errno);

        // Instruct the kernel that any messages to multicast destinations should be
        // sent down the interface to which the specified IPv4 address is bound.
#ifdef IP_MULTICAST_IF
        if (lRetval == INET_NO_ERROR)
            setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_IF, &sa, sizeof (sa));
#endif // defined(IP_MULTICAST_IF)

        // Instruct the kernel that any messages to multicast destinations should be
        // set with the configured hop limit value.
#ifdef IP_MULTICAST_TTL
        int ttl = INET_CONFIG_IP_MULTICAST_HOP_LIMIT;
        setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof (ttl));
#endif // defined(IP_MULTICAST_TTL)

        // Allow socket transmitting broadcast packets.
        if (lRetval == INET_NO_ERROR)
            setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof (enable));
    }
#endif // INET_CONFIG_ENABLE_IPV4
    else
        lRetval = INET_ERROR_WRONG_ADDRESS_TYPE;

    return (lRetval);
}

INET_ERROR IPEndPointBasis::BindInterface(IPAddressType aAddressType, InterfaceId aInterfaceId)
{
    INET_ERROR lRetval = INET_NO_ERROR;

#if HAVE_SO_BINDTODEVICE
    if (aInterfaceId == INET_NULL_INTERFACEID)
    {
        //Stop interface-based filtering.
        if (setsockopt(mSocket, SOL_SOCKET, SO_BINDTODEVICE, "", 0) == -1)
        {
            lRetval = Weave::System::MapErrorPOSIX(errno);
        }
    }
    else
    {
        //Start filtering on the passed interface.
        char lInterfaceName[IF_NAMESIZE];

        if (if_indextoname(aInterfaceId, lInterfaceName) == NULL)
        {
            lRetval = Weave::System::MapErrorPOSIX(errno);
        }

        if (lRetval == INET_NO_ERROR && setsockopt(mSocket, SOL_SOCKET, SO_BINDTODEVICE, lInterfaceName, strlen(lInterfaceName)) == -1)
        {
            lRetval = Weave::System::MapErrorPOSIX(errno);
        }
    }

    if (lRetval == INET_NO_ERROR)
        mBoundIntfId = aInterfaceId;

#else // !HAVE_SO_BINDTODEVICE
    lRetval = INET_ERROR_NOT_IMPLEMENTED;
#endif // HAVE_SO_BINDTODEVICE

    return (lRetval);
}

INET_ERROR IPEndPointBasis::SendTo(const IPAddress &aAddress, uint16_t aPort, InterfaceId aInterfaceId, Weave::System::PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    INET_ERROR     lRetval = INET_NO_ERROR;
    PeerSockAddr   lPeerSockAddr;
    struct iovec   msgIOV;
    uint8_t        controlData[256];
    struct msghdr  msgHeader;

    // For now the entire message must fit within a single buffer.

    VerifyOrExit(aBuffer->Next() == NULL, lRetval = INET_ERROR_MESSAGE_TOO_LONG);

    memset(&msgHeader, 0, sizeof (msgHeader));

    msgIOV.iov_base      = aBuffer->Start();
    msgIOV.iov_len       = aBuffer->DataLength();
    msgHeader.msg_iov    = &msgIOV;
    msgHeader.msg_iovlen = 1;

    memset(&lPeerSockAddr, 0, sizeof (lPeerSockAddr));

    msgHeader.msg_name = &lPeerSockAddr;

    if (mAddrType == kIPAddressType_IPv6)
    {
        lPeerSockAddr.in6.sin6_family   = AF_INET6;
        lPeerSockAddr.in6.sin6_port     = htons(aPort);
        lPeerSockAddr.in6.sin6_flowinfo = 0;
        lPeerSockAddr.in6.sin6_addr     = aAddress.ToIPv6();
        lPeerSockAddr.in6.sin6_scope_id = aInterfaceId;
        msgHeader.msg_namelen           = sizeof (sockaddr_in6);
    }
#if INET_CONFIG_ENABLE_IPV4
    else
    {
        lPeerSockAddr.in.sin_family     = AF_INET;
        lPeerSockAddr.in.sin_port       = htons(aPort);
        lPeerSockAddr.in.sin_addr       = aAddress.ToIPv4();
        msgHeader.msg_namelen           = sizeof (sockaddr_in);
    }
#endif // INET_CONFIG_ENABLE_IPV4

    // If the endpoint has been bound to a particular interface,
    // and the caller didn't supply a specific interface to send
    // on, use the bound interface. This appears to be necessary
    // for messages to multicast addresses, which under Linux
    // don't seem to get sent out the correct inferface, despite
    // the socket being bound.

    if (aInterfaceId == INET_NULL_INTERFACEID)
        aInterfaceId = mBoundIntfId;

    if (aInterfaceId != INET_NULL_INTERFACEID && false)
    {
#if defined(IP_PKTINFO) || defined(IPV6_PKTINFO)
        memset(controlData, 0, sizeof (controlData));
        msgHeader.msg_control = controlData;
        msgHeader.msg_controllen = sizeof (controlData);

        struct cmsghdr *controlHdr = CMSG_FIRSTHDR(&msgHeader);

#if INET_CONFIG_ENABLE_IPV4
#if defined(IP_PKTINFO)
        if (mAddrType == kIPAddressType_IPv4)
        {
            controlHdr->cmsg_level = IPPROTO_IP;
            controlHdr->cmsg_type  = IP_PKTINFO;
            controlHdr->cmsg_len   = CMSG_LEN(sizeof (in_pktinfo));

            struct in_pktinfo *pktInfo = (struct in_pktinfo *)CMSG_DATA(controlHdr);
            pktInfo->ipi_ifindex = aInterfaceId;

            msgHeader.msg_controllen = CMSG_SPACE(sizeof (in_pktinfo));
        }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#if defined(IPV6_PKTINFO)
        if (mAddrType == kIPAddressType_IPv6)
        {
            controlHdr->cmsg_level = IPPROTO_IP;
            controlHdr->cmsg_type  = IPV6_PKTINFO;
            controlHdr->cmsg_len   = CMSG_LEN(sizeof (in6_pktinfo));

            struct in6_pktinfo *pktInfo = (struct in6_pktinfo *)CMSG_DATA(controlHdr);
            pktInfo->ipi6_ifindex = aInterfaceId;

            msgHeader.msg_controllen = CMSG_SPACE(sizeof (in6_pktinfo));
        }
#endif // defined(IPV6_PKTINFO)
#else // !(defined(IP_PKTINFO) && defined(IPV6_PKTINFO))
        lRetval = INET_ERROR_NOT_IMPLEMENTED
#endif // !(defined(IP_PKTINFO) && defined(IPV6_PKTINFO))
    }

    if (lRetval == INET_NO_ERROR)
    {
        // Send IP packet.

        const ssize_t lenSent = sendto(mSocket, msgHeader.msg_iov[0].iov_base, msgHeader.msg_iov[0].iov_len, 0, &lPeerSockAddr.any, msgHeader.msg_namelen);

        if (lenSent == -1)
            lRetval = Weave::System::MapErrorPOSIX(errno);
        else if (lenSent != aBuffer->DataLength())
            lRetval = INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED;
    }

exit:
    return (lRetval);
}

INET_ERROR IPEndPointBasis::GetSocket(IPAddressType aAddressType, int aType, int aProtocol)
{
    INET_ERROR res = INET_NO_ERROR;

    if (mSocket == INET_INVALID_SOCKET_FD)
    {
        const int one = 1;
        int family;

        switch (aAddressType)
        {
        case kIPAddressType_IPv6:
            family = PF_INET6;
            break;

#if INET_CONFIG_ENABLE_IPV4
        case kIPAddressType_IPv4:
            family = PF_INET;
            break;
#endif // INET_CONFIG_ENABLE_IPV4

        default:
            return INET_ERROR_WRONG_ADDRESS_TYPE;
        }

        mSocket = ::socket(family, aType, aProtocol);
        if (mSocket == -1)
            return Weave::System::MapErrorPOSIX(errno);

        mAddrType = aAddressType;

        // NOTE WELL: the errors returned by setsockopt() here are not
        // returned as Inet layer Weave::System::MapErrorPOSIX(errno)
        // codes because they are normally expected to fail on some
        // platforms where the socket option code is defined in the
        // header files but not [yet] implemented. Certainly, there is
        // room to improve this by connecting the build configuration
        // logic up to check for implementations of these options and
        // to provide appropriate HAVE_xxxxx definitions accordingly.

        res = setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR,  (void*)&one, sizeof (one));
        static_cast<void>(res);

#ifdef SO_REUSEPORT
        res = setsockopt(mSocket, SOL_SOCKET, SO_REUSEPORT,  (void*)&one, sizeof (one));
        if (res != 0)
        {
            WeaveLogError(Inet, "SO_REUSEPORT: %d", errno);
        }
#endif // defined(SO_REUSEPORT)

        // If creating an IPv6 socket, tell the kernel that it will be
        // IPv6 only.  This makes it posible to bind two sockets to
        // the same port, one for IPv4 and one for IPv6.

#ifdef IPV6_V6ONLY
#if INET_CONFIG_ENABLE_IPV4
        if (aAddressType == kIPAddressType_IPv6)
#endif // INET_CONFIG_ENABLE_IPV4
        {
            res = setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void *) &one, sizeof (one));
            if (res != 0)
            {
                WeaveLogError(Inet, "IPV6_V6ONLY: %d", errno);
            }
        }
#endif // defined(IPV6_V6ONLY)

#if INET_CONFIG_ENABLE_IPV4
#ifdef IP_PKTINFO
        res = setsockopt(mSocket, IPPROTO_IP, IP_PKTINFO, (void *) &one, sizeof (one));
        if (res != 0)
        {
            WeaveLogError(Inet, "IP_PKTINFO: %d", errno);
        }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#ifdef IPV6_RECVPKTINFO
        res = setsockopt(mSocket, IPPROTO_IPV6, IPV6_RECVPKTINFO, (void *) &one, sizeof (one));
        if (res != 0)
        {
            WeaveLogError(Inet, "IPV6_PKTINFO: %d", errno);
        }
#endif // defined(IPV6_RECVPKTINFO)

        // On systems that support it, disable the delivery of SIGPIPE
        // signals when writing to a closed socket.  This is mostly
        // needed on iOS which has the peculiar habit of sending
        // SIGPIPEs on unconnected UDP sockets.
#ifdef SO_NOSIGPIPE
        {
            res = setsockopt(mSocket, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof (one));
            if (res != 0)
            {
                WeaveLogError(Inet, "SO_NOSIGPIPE: %d", errno);
            }
        }
#endif // defined(SO_NOSIGPIPE)

    }
    else if (mAddrType != aAddressType)
    {
        return INET_ERROR_INCORRECT_STATE;
    }

    return INET_NO_ERROR;
}

SocketEvents IPEndPointBasis::PrepareIO(void)
{
    SocketEvents res;

    if (mState == kState_Listening && OnMessageReceived != NULL)
        res.SetRead();

    return res;
}

void IPEndPointBasis::HandlePendingIO(uint16_t aPort)
{
    INET_ERROR      lStatus = INET_NO_ERROR;
    IPPacketInfo    lPacketInfo;
    PacketBuffer *  lBuffer;

    lPacketInfo.Clear();
    lPacketInfo.DestPort = aPort;

    lBuffer = PacketBuffer::New(0);

    if (lBuffer != NULL)
    {
        struct iovec msgIOV;
        PeerSockAddr lPeerSockAddr;
        uint8_t controlData[256];
        struct msghdr msgHeader;

        msgIOV.iov_base = lBuffer->Start();
        msgIOV.iov_len = lBuffer->AvailableDataLength();

        memset(&lPeerSockAddr, 0, sizeof (lPeerSockAddr));

        memset(&msgHeader, 0, sizeof (msgHeader));

        msgHeader.msg_name = &lPeerSockAddr;
        msgHeader.msg_namelen = sizeof (lPeerSockAddr);
        msgHeader.msg_iov = &msgIOV;
        msgHeader.msg_iovlen = 1;
        msgHeader.msg_control = controlData;
        msgHeader.msg_controllen = sizeof (controlData);

        ssize_t rcvLen = recvmsg(mSocket, &msgHeader, MSG_DONTWAIT);

        if (rcvLen < 0)
        {
            lStatus = Weave::System::MapErrorPOSIX(errno);
        }
        else if (rcvLen > lBuffer->AvailableDataLength())
        {
            lStatus = INET_ERROR_INBOUND_MESSAGE_TOO_BIG;
        }
        else
        {
            lBuffer->SetDataLength((uint16_t) rcvLen);

            if (lPeerSockAddr.any.sa_family == AF_INET6)
            {
                lPacketInfo.SrcAddress = IPAddress::FromIPv6(lPeerSockAddr.in6.sin6_addr);
                lPacketInfo.SrcPort = ntohs(lPeerSockAddr.in6.sin6_port);
            }
#if INET_CONFIG_ENABLE_IPV4
            else if (lPeerSockAddr.any.sa_family == AF_INET)
            {
                lPacketInfo.SrcAddress = IPAddress::FromIPv4(lPeerSockAddr.in.sin_addr);
                lPacketInfo.SrcPort = ntohs(lPeerSockAddr.in.sin_port);
            }
#endif // INET_CONFIG_ENABLE_IPV4
            else
            {
                lStatus = INET_ERROR_INCORRECT_STATE;
            }
        }

        if (lStatus == INET_NO_ERROR)
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
                    lPacketInfo.Interface = inPktInfo->ipi_ifindex;
                    lPacketInfo.DestAddress = IPAddress::FromIPv4(inPktInfo->ipi_addr);
                    continue;
                }
#endif // defined(IP_PKTINFO)
#endif // INET_CONFIG_ENABLE_IPV4

#ifdef IPV6_PKTINFO
                if (controlHdr->cmsg_level == IPPROTO_IPV6 && controlHdr->cmsg_type == IPV6_PKTINFO)
                {
                    struct in6_pktinfo *in6PktInfo = (struct in6_pktinfo *)CMSG_DATA(controlHdr);
                    lPacketInfo.Interface = in6PktInfo->ipi6_ifindex;
                    lPacketInfo.DestAddress = IPAddress::FromIPv6(in6PktInfo->ipi6_addr);
                    continue;
                }
#endif // defined(IPV6_PKTINFO)
            }
        }
    }
    else
    {
        lStatus = INET_ERROR_NO_MEMORY;
    }

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

    return;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl

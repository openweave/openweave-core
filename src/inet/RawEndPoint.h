/*
 *
 *    Copyright (c) 2018 Google LLC
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
 *      This header file defines the <tt>nl::Inet::RawEndPoint</tt>
 *      class, where the Nest Inet Layer encapsulates methods for
 *      interacting with IP network endpoints (SOCK_RAW sockets
 *      on Linux and BSD-derived systems) or LwIP raw protocol
 *      control blocks, as the system is configured accordingly.
 */

#ifndef RAWENDPOINT_H
#define RAWENDPOINT_H

#include <InetLayer/IPEndPointBasis.h>
#include <InetLayer/IPAddress.h>

#include <SystemLayer/SystemPacketBuffer.h>

namespace nl {
namespace Inet {

class InetLayer;
class IPPacketInfo;

/**
 * @brief   Objects of this class represent raw IP network endpoints.
 *
 * @details
 *  Nest Inet Layer encapsulates methods for interacting with IP network
 *  endpoints (SOCK_RAW sockets on Linux and BSD-derived systems) or LwIP
 *  raw protocol control blocks, as the system is configured accordingly.
 */
class NL_DLL_EXPORT RawEndPoint : public IPEndPointBasis
{
    friend class InetLayer;

public:
    /**
     * @brief   Version of the Internet protocol
     *
     * @details
     *  While this field is a mutable class variable, it is an invariant of the
     *  class that it not be modified.
     */
    IPVersion IPVer;   // This data member is read-only

    /**
     * @brief   version of the Internet Control Message Protocol (ICMP)
     *
     * @details
     *  While this field is a mutable class variable, it is an invariant of the
     *  class that it not be modified.
     */
    IPProtocol IPProto; // This data member is read-only

    /**
     * @brief   Bind the endpoint to an interface IP address.
     *
     * @param[in]   addrType    the protocol version of the IP address
     * @param[in]   addr        the IP address (must be an interface address)
     * @param[in]   intfId      an optional network interface indicator
     *
     * @retval  INET_NO_ERROR               success: endpoint bound to address
     * @retval  INET_ERROR_INCORRECT_STATE  endpoint has been bound previously
     * @retval  INET_NO_MEMORY              insufficient memory for endpoint
     *
     * @retval  INET_ERROR_UNKNOWN_INTERFACE
     *      On some platforms, the optionally specified interface is not
     *      present.
     *
     * @retval  INET_ERROR_WRONG_PROTOCOL_TYPE
     *      \c addrType does not match \c IPVer.
     *
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *      \c addrType is \c kIPAddressType_Any, or the type of \c addr is not
     *      equal to \c addrType.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *  Binds the endpoint to the specified network interface IP address.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired.
     */
    INET_ERROR Bind(IPAddressType addrType, IPAddress addr, InterfaceId intfId = INET_NULL_INTERFACEID);

    /**
     * @brief   Bind the endpoint to an interface IPv6 link-local address.
     *
     * @param[in]   intf    the indicator of the network interface
     * @param[in]   addr    the IP address (must be an interface address)
     *
     * @retval  INET_NO_ERROR               success: endpoint bound to address
     * @retval  INET_ERROR_INCORRECT_STATE  endpoint has been bound previously
     * @retval  INET_NO_MEMORY              insufficient memory for endpoint
     *
     * @retval  INET_ERROR_WRONG_PROTOCOL_TYPE
     *      \c addrType does not match \c IPVer.
     *
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *      \c addr is not an IPv6 link-local address or \c intf is
     *      \c INET_NULL_INTERFACEID.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *  Binds the endpoint to the IPv6 link-local address \c addr on the
     *  network interface indicated by \c intf.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired.
     */
    INET_ERROR BindIPv6LinkLocal(InterfaceId intf, IPAddress addr);

    /**
     * @brief   Prepare the endpoint to receive ICMP messages.
     *
     * @retval  INET_NO_ERROR   always returned.
     *
     * @details
     *  If \c mState is already \c kState_Listening, then no operation is
     *  performed, otherwise the \c mState is set to \c kState_Listening and
     *  the endpoint is prepared to received ICMPv6 messages, according to the
     *  semantics of the platform.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired
     */
    INET_ERROR Listen(void);

    /**
     *  A synonym for <tt>SendTo(addr, INET_NULL_INTERFACEID, msg,
     *  sendFlags)</tt>.
     */
    INET_ERROR SendTo(IPAddress addr, Weave::System::PacketBuffer *msg, uint16_t sendFlags = 0);

    /**
     * @brief   Send an ICMP message to the specified destination address.
     *
     * @param[in]   addr        the destination IP address
     * @param[in]   intfId      an optional network interface indicator
     * @param[in]   msg         the packet buffer containing the UDP message
     * @param[in]   sendFlags   optional transmit option flags
     *
     * @retval  INET_NO_ERROR       success: \c msg is queued for transmit.
     * @retval  INET_ERROR_NOT_IMPLEMENTED  system implementation not complete.
     *
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *      the destination address and the bound interface address do not
     *      have matching protocol versions or address type.
     *
     * @retval  INET_ERROR_MESSAGE_TOO_LONG
     *      \c msg does not contain the whole ICMP message.
     *
     * @retval  INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED
     *      On some platforms, only a truncated portion of \c msg was queued
     *      for transmit.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *      If possible, then this method sends the ICMP message \c msg to the
     *      destination \c addr with the transmit option flags encoded
     *      in \c sendFlags.
     *
     *      Where <tt>(sendFlags & kSendFlag_RetainBuffer) != 0</tt>, calls
     *      <tt>Weave::System::PacketBuffer::Free</tt> on behalf of the caller, otherwise this
     *      method deep-copies \c msg into a fresh object, and queues that for
     *      transmission, leaving the original \c msg available after return.
     */
    INET_ERROR SendTo(IPAddress addr, InterfaceId intfId, Weave::System::PacketBuffer *msg, uint16_t sendFlags = 0);

    /**
     * Get the bound interface on this endpoint.
     *
     * @return InterfaceId   The bound interface id.
     */
    InterfaceId GetBoundInterface(void);

    /**
     * @brief   Set the ICMP6 filter parameters in the network stack.
     *
     * @param[in]   numICMPTypes    length of array at \c aICMPTypes
     * @param[in]   aICMPTypes      the set of ICMPv6 type codes to filter.
     *
     * @retval  INET_NO_ERROR                   success: filter parameters set
     * @retval  INET_ERROR_NOT_IMPLEMENTED      system does not implement
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE   endpoint not IPv6 type
     * @retval  INET_ERROR_WRONG_PROTOCOL_TYPE  endpoint not ICMP6 type
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *  Apply the ICMPv6 filtering parameters for the codes in \c aICMPTypes to
     *  the underlying endpoint in the system networking stack.
     */
    INET_ERROR SetICMPFilter(uint8_t numICMPTypes, const uint8_t * aICMPTypes);

    /**
     * @brief   Bind the endpoint to a network interface.
     *
     * @param[in]   addrType    the protocol version of the IP address.
	 *
     * @param[in]   intf        indicator of the network interface.
     *
     * @retval  INET_NO_ERROR               success: endpoint bound to address
     * @retval  INET_NO_MEMORY              insufficient memory for endpoint
     * @retval  INET_ERROR_NOT_IMPLEMENTED  system implementation not complete.
     *
     * @retval  INET_ERROR_UNKNOWN_INTERFACE
     *      On some platforms, the interface is not present.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *  Binds the endpoint to the specified network interface IP address.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired.
     */
    INET_ERROR BindInterface(IPAddressType addrType, InterfaceId intf);

    /**
     * @brief   Close the endpoint.
     *
     * @details
     *  If <tt>mState != kState_Closed</tt>, then closes the endpoint, removing
     *  it from the set of endpoints eligible for communication events.
     *
     *  On LwIP systems, this method must not be called with the LwIP stack
     *  lock already acquired.
     */
    void Close(void);

    /**
     * @brief   Close the endpoint and recycle its memory.
     *
     * @details
     *  Invokes the \c Close method, then invokes the
     *  <tt>InetLayerBasis::Release</tt> method to return the object to its
     *  memory pool.
     *
     *  On LwIP systems, this method must not be called with the LwIP stack
     *  lock already acquired.
     */
    void Free(void);

private:
    RawEndPoint(void);                          // not defined
    RawEndPoint(const RawEndPoint&);            // not defined
    ~RawEndPoint(void);                         // not defined

    static Weave::System::ObjectPool<RawEndPoint, INET_CONFIG_NUM_RAW_ENDPOINTS> sPool;

    void Init(InetLayer *inetLayer, IPVersion ipVer, IPProtocol ipProto);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    uint8_t NumICMPTypes;
    const uint8_t *ICMPTypes;

    void HandleDataReceived(Weave::System::PacketBuffer *msg);
    INET_ERROR GetPCB(IPAddressType addrType);

#if LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
    static u8_t LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr);
#else // LWIP_VERSION_MAJOR <= 1 && LWIP_VERSION_MINOR < 5
    static u8_t LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr);
#endif // LWIP_VERSION_MAJOR > 1 || LWIP_VERSION_MINOR >= 5
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    INET_ERROR GetSocket(IPAddressType addrType);
    SocketEvents PrepareIO(void);
    void HandlePendingIO(void);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
};

} // namespace Inet
} // namespace nl

#endif // !defined(RAWENDPOINT_H)

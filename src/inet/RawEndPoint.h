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

/**                                                                           .
 *    @file
 *      This header file defines the <tt>nl::Inet::RawEndPoint</tt>
 *      class, where the Nest Inet Layer encapsulates methods for
 *      interacting with PF_RAW sockets (on Linux and BSD-derived
 *      systems) or LwIP raw protocol control blocks, as the system
 *      is configured accordingly.
 */

#ifndef RAWENDPOINT_H
#define RAWENDPOINT_H

#include <InetLayer/EndPointBasis.h>
#include <SystemLayer/SystemPacketBuffer.h>

namespace nl {
namespace Inet {

class InetLayer;
class SenderInfo;

/**                                                                           .
 * @brief   Objects of this class represent raw IP protocol endpoints.
 *
 * @details
 *  Nest Inet Layer encapsulates methods for interacting with PF_RAW
 *  sockets (on Linux and BSD-derived systems) or LwIP raw protocol control
 *  blocks, as the system is configured accordingly.
 */
class NL_DLL_EXPORT RawEndPoint : public EndPointBasis
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
     * @brief   Basic dynamic state of the underlying endpoint.
     *
     * @details
     *  Objects are initialized in the "closed" state, proceed to the "bound"
     *  state after binding to a local interface address, then proceed to the
     *  "listening" state when they have continuations registered for handling
     *  events for reception of ICMP messages.
     */
    enum {
        kState_Closed = kBasisState_Closed, /**< Endpoint initialized, but not bound. */
        kState_Bound,                       /**< Endpoint bound, but not listening. */
        kState_Listening                    /**< Endpoint receiving ICMP messages. */
    } mState;

    /**
     * @brief   Bind the endpoint to an interface IP address.
     *
     * @param[in]   addrType    the protocol version of the IP address
     * @param[in]   addr        the IP address (must be an interface address)
     *
     * @retval  INET_NO_ERROR               success: endpoint bound to address
     * @retval  INET_ERROR_INCORRECT_STATE  endpoint has been bound previously
     * @retval  INET_NO_MEMORY              insufficient memory for endpoint
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
    INET_ERROR Bind(IPAddressType addrType, IPAddress addr);

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
     * @brief   Send an ICMP message to the specified destination address.
     *
     * @param[in]   addr    the destination address
     * @param[in]   msg     the packet buffer containing the ICMP message
     *
     * @retval  INET_NO_ERROR       success: \msg is queued for transmit.
     *
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *      the destination address and the bound interface address do not
     *      have matching protocol versions or address type.
     *
     * @retval  INET_ERROR_MESSAGE_TOO_LONG
     *      Returned on some platforms, \c msg does not contain the whole ICMP
     *      message.
     *
     * @retval  INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED
     *      Returned on some platforms, only a truncated portion of \c msg was
     *      queued for transmit.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *      Sends the ICMP message if possible. Always calls
     *      <tt>Weave::System::PacketBuffer::Free</tt> on behalf of the caller.
     */
    INET_ERROR SendTo(IPAddress addr, Weave::System::PacketBuffer *msg);

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
     * @param[in]   intf    indicator of the network interface.
     *
     * @retval  INET_NO_ERROR               success: endpoint bound to address
     * @retval  INET_NO_MEMORY              insufficient memory for endpoint
     *
     * @retval  INET_ERROR_UNKNOWN_INTERFACE
     *      On LwIP systems, the interface is not present.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *  Binds the endpoint to the specified network interface IP address.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired.
     */
    INET_ERROR BindInterface(InterfaceId intf);

    /**
     * @brief   Close the endpoint and recycle its memory.
     *
     * @details
     *  Invokes the \c Close method, then invokes the
     *  <tt>InetLayerBasis::Release</tt> method to return the object to its
     *  memory pool.
     *
     *  On LwIP systems, an event handler is dispatched to release the object
     *  within the context of the the LwIP thread. This method must not be
     *  called with the LwIP stack lock already acquired.
     */
    void Free(void);

    /**
     * @brief   Type of message text reception event handling function.
     *
     * @param[in]   endPoint    The raw endpoint associated with the event.
     * @param[in]   msg         The message text received.
     * @param[in]   senderAddr  The IP address of the sender.
     *
     * @details
     *  Provide a function of this type to the \c OnMessageReceived delegate
     *  member to process message text reception events on \c endPoint where
     *  \c msg is the message text received from the sender at \c senderAddr.
     */
    typedef void (*OnMessageReceivedFunct)(RawEndPoint *endPoint, Weave::System::PacketBuffer *msg, IPAddress senderAddr);

    /** The endpoint's message reception event handling function delegate. */
    OnMessageReceivedFunct OnMessageReceived;

    /**
     * @brief   Type of reception error event handling function.
     *
     * @param[in]   endPoint    The raw endpoint associated with the event.
     * @param[in]   err         The reason for the error.
     *
     * @details
     *  Provide a function of this type to the \c OnReceiveError delegate
     *  member to process reception error events on \c endPoint. The \c err
     *  argument provides specific detail about the type of the error.
     */
    typedef void (*OnReceiveErrorFunct)(RawEndPoint *endPoint, INET_ERROR err, IPAddress senderAddr);

    /** The endpoint's receive error event handling function delegate. */
    OnReceiveErrorFunct OnReceiveError;

private:
    RawEndPoint(void);                          // not defined
    RawEndPoint(const RawEndPoint&);            // not defined
    ~RawEndPoint(void);                         // not defined

    static Weave::System::ObjectPool<RawEndPoint, INET_CONFIG_NUM_RAW_ENDPOINTS> sPool;

    void Init(InetLayer *inetLayer, IPVersion ipVer, IPProtocol ipProto);
    void Close(void);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    INET_ERROR GetSocket(IPAddressType addrType);
    SocketEvents PrepareIO(void);
    void HandlePendingIO(void);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    uint8_t NumICMPTypes;
    const uint8_t *ICMPTypes;

    void HandleDataReceived(Weave::System::PacketBuffer *msg);
    INET_ERROR GetPCB(void);
    static SenderInfo *GetSenderInfo(Weave::System::PacketBuffer *buf);

#if LWIP_VERSION_MAJOR > 1
    static u8_t LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr);
#else // LWIP_VERSION_MAJOR <= 1
    static u8_t LwIPReceiveRawMessage(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr);
#endif // LWIP_VERSION_MAJOR <= 1
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
};

} // namespace Inet
} // namespace nl

#endif // !defined(RAWENDPOINT_H)

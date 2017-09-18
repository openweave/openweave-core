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
 *      This header file defines the <tt>nl::Inet::UDPEndPoint</tt>
 *      class, where the Nest Inet Layer encapsulates methods for
 *      interacting with UDP transport endpoints (SOCK_DGRAM sockets
 *      on Linux and BSD-derived systems) or LwIP UDP protocol
 *      control blocks, as the system is configured accordingly.
 */

#ifndef UDPENDPOINT_H
#define UDPENDPOINT_H

#include <InetLayer/EndPointBasis.h>
#include <InetLayer/IPAddress.h>

#include <SystemLayer/SystemPacketBuffer.h>

namespace nl {
namespace Inet {

class InetLayer;
class IPPacketInfo;

/**                                                                           .
 * @brief   Objects of this class represent UDP transport endpoints.
 *
 * @details
 *  Nest Inet Layer encapsulates methods for interacting with UDP transport
 *  endpoints (SOCK_DGRAM sockets on Linux and BSD-derived systems) or LwIP
 *  UDP protocol control blocks, as the system is configured accordingly.
 */
class NL_DLL_EXPORT UDPEndPoint : public EndPointBasis
{
    friend class InetLayer;

public:
    /**
     * @brief   Basic dynamic state of the underlying endpoint.
     *
     * @details
     *  Objects are initialized in the "ready" state, proceed to the "bound"
     *  state after binding to a local interface address, then proceed to the
     *  "listening" state when they have continuations registered for handling
     *  events for reception of UDP messages.
     *
     * @note
     *  The \c kBasisState_Closed state enumeration is mapped to \c kState_Ready for historical binary-compatibility reasons. The
     *  existing \c kState_Closed exists to identify separately the distinction between "not opened yet" and "previously opened now
     *  closed" that existed previously in the \c kState_Ready and \c kState_Closed states.
     */
    enum {
        kState_Ready        = kBasisState_Closed,   /**< Endpoint initialized, but not open. */
        kState_Bound        = 1,                    /**< Endpoint bound, but not listening. */
        kState_Listening    = 2,                    /**< Endpoint receiving datagrams. */
        kState_Closed       = 3                     /**< Endpoint closed, ready for release. */
    } mState;

    /**
     * @brief   Transmit option flags for the \c SendTo methods.
     */
    enum {
        /** Do not destructively queue the message directly. Queue a copy. */
        kSendFlag_RetainBuffer = 0x0040
    };

    /**
     * @brief   Bind the endpoint to an interface IP address.
     *
     * @param[in]   addrType    the protocol version of the IP address
     * @param[in]   addr        the IP address (must be an interface address)
     * @param[in]   port        the UDP port
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
     *  already acquired.
     */
    INET_ERROR Bind(IPAddressType addrType, IPAddress addr, uint16_t port, InterfaceId intfId = INET_NULL_INTERFACEID);

    /**
     * @brief   Prepare the endpoint to receive UDP messages.
     *
     * @retval  INET_NO_ERROR   success: endpoint ready to receive messages.
     * @retval  INET_ERROR_INCORRECT_STATE  endpoint is already listening.
     *
     * @details
     *  If \c State is already \c kState_Listening, then no operation is
     *  performed, otherwise the \c State is set to \c kState_Listening and
     *  the endpoint is prepared to received UDP messages, according to the
     *  semantics of the platform.
     *
     *  On LwIP, this method must not be called with the LwIP stack lock
     *  already acquired
     */
    INET_ERROR Listen(void);

    /**
     *  A synonym for <tt>SendTo(addr, port, INET_NULL_INTERFACEID, msg,
     *  sendFlags)</tt>.
     */
    INET_ERROR SendTo(IPAddress addr, uint16_t port, Weave::System::PacketBuffer *msg, uint16_t sendFlags = 0);

    /**
     * @brief   Send a UDP message to the specified destination address.
     *
     * @param[in]   addr        the destination IP address
     * @param[in]   port        the destination UDP port
     * @param[in]   intfId      an optional network interface indicator
     * @param[in]   msg         the packet buffer containing the UDP message
     * @param[in]   sendFlags   optional transmit option flags
     *
     * @retval  INET_NO_ERROR       success: \msg is queued for transmit.
     * @retval  INET_ERROR_NOT_IMPLEMENTED  system implementation not complete.
     *
     * @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *      the destination address and the bound interface address do not
     *      have matching protocol versions or address type.
     *
     * @retval  INET_ERROR_MESSAGE_TOO_LONG
     *      \c msg does not contain the whole UDP message.
     *
     * @retval  INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED
     *      On some platforms, only a truncated portion of \c msg was queued
     *      for transmit.
     *
     * @retval  other                   another system or platform error
     *
     * @details
     *      If possible, then this method sends the UDP message \c msg to the
     *      destination \c addr (with \c intfId used as the scope
     *      identifier for IPv6 link-local destinations) and \c port with the
     *      transmit option flags encoded in \c sendFlags.
     *
     *      Where <tt>(sendFlags & kSendFlag_RetainBuffer) != 0</tt>, calls
     *      <tt>Weave::System::PacketBuffer::Free</tt> on behalf of the caller, otherwise this
     *      method deep-copies \c msg into a fresh object, and queues that for
     *      transmission, leaving the original \c msg available after return.
     */
    INET_ERROR SendTo(IPAddress addr, uint16_t port, InterfaceId intfId, Weave::System::PacketBuffer *msg, uint16_t sendFlags = 0);

    /**
     * Get the bound interface on this endpoint.
     *
     * @return InterfaceId   The bound interface id.
     */
    InterfaceId GetBoundInterface(void);

    /**
     * @brief   Bind the endpoint to a network interface.
     *
     * @param[in]   addrType    the protocol version of the IP address
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

    /**
     * @brief   Type of message text reception event handling function.
     *
     * @param[in]   endPoint    The UDP endpoint associated with the event.
     * @param[in]   msg         The message text received.
     * @param[in]   pktInfo     The ancillary packet information.
     *
     * @details
     *  Provide a function of this type to the \c OnMessageReceived delegate
     *  member to process message text reception events on \c endPoint where
     *  \c msg is the message text and \c pktInfo is the ancillary packet
     *  information for it.
     */
    typedef void (*OnMessageReceivedFunct)(UDPEndPoint *endPoint, Weave::System::PacketBuffer *msg, const IPPacketInfo *pktInfo);

    /** The endpoint's message reception event handling function delegate. */
    OnMessageReceivedFunct OnMessageReceived;

    /**
     * @brief   Type of reception error event handling function.
     *
     * @param[in]   endPoint    The raw endpoint associated with the event.
     * @param[in]   err         The reason for the error.
     * @param[in]   pktInfo     The ancillary packet information.
     *
     * @details
     *  Provide a function of this type to the \c OnReceiveError delegate
     *  member to process reception acceptance error events on \c endPoint. The
     *  \c err argument provides specific detail about the type of the error,
     *  and \c pktInfo is the ancillary packet information associated with the
     *  reception event.
     */
    typedef void (*OnReceiveErrorFunct)(UDPEndPoint *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo);

    /** The endpoint's receive error event handling function delegate. */
    OnReceiveErrorFunct OnReceiveError;

private:
    UDPEndPoint(void);                                  // not defined
    UDPEndPoint(const UDPEndPoint&);                    // not defined
    ~UDPEndPoint(void);                                 // not defined

    static Weave::System::ObjectPool<UDPEndPoint, INET_CONFIG_NUM_UDP_ENDPOINTS> sPool;

    void Init(InetLayer *inetLayer);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    void HandleDataReceived(Weave::System::PacketBuffer *msg);
    INET_ERROR GetPCB(IPAddressType addrType4);
    static IPPacketInfo *GetPacketInfo(Weave::System::PacketBuffer *buf);
#if LWIP_VERSION_MAJOR > 1
    static void LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
#else // LWIP_VERSION_MAJOR <= 1
    static void LwIPReceiveUDPMessage(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);
#endif // LWIP_VERSION_MAJOR <= 1
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    uint16_t mBoundPort;
    InterfaceId mBoundIntfId;

    INET_ERROR GetSocket(IPAddressType addrType);
    SocketEvents PrepareIO(void);
    void HandlePendingIO(void);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
};

} // namespace Inet
} // namespace nl

#endif // !defined(UDPENDPOINT_H)

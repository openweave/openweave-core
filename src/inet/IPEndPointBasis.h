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
 *      This header file defines the <tt>nl::Inet::IPEndPointBasis</tt>
 *      class, an intermediate, non-instantiable basis class
 *      supporting other IP-based end points.
 *
 */

#ifndef IPENDPOINTBASIS_H
#define IPENDPOINTBASIS_H

#include <InetLayer/EndPointBasis.h>

#include <SystemLayer/SystemPacketBuffer.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#include <lwip/netif.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

namespace nl {
namespace Inet {

class InetLayer;
class IPPacketInfo;

/**
 * @class IPEndPointBasis
 *
 * @brief Objects of this class represent non-instantiable IP protocol
 *        endpoints.
 *
 */
class NL_DLL_EXPORT IPEndPointBasis : public EndPointBasis
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
     *  events for reception of ICMP messages.
     *
     * @note
     *  The \c kBasisState_Closed state enumeration is mapped to \c
     *  kState_Ready for historical binary-compatibility reasons. The
     *  existing \c kState_Closed exists to identify separately the
     *  distinction between "not opened yet" and "previously opened
     *  now closed" that existed previously in the \c kState_Ready and
     *  \c kState_Closed states.
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
     * @brief   Type of message text reception event handling function.
     *
     * @param[in]   endPoint    The endpoint associated with the event.
     * @param[in]   msg         The message text received.
     * @param[in]   senderAddr  The IP address of the sender.
     *
     * @details
     *  Provide a function of this type to the \c OnMessageReceived delegate
     *  member to process message text reception events on \c endPoint where
     *  \c msg is the message text received from the sender at \c senderAddr.
     */
    typedef void (*OnMessageReceivedFunct)(IPEndPointBasis *endPoint, Weave::System::PacketBuffer *msg, const IPPacketInfo *pktInfo);

    /** The endpoint's message reception event handling function delegate. */
    OnMessageReceivedFunct OnMessageReceived;

    /**
     * @brief   Type of reception error event handling function.
     *
     * @param[in]   endPoint    The endpoint associated with the event.
     * @param[in]   err         The reason for the error.
     *
     * @details
     *  Provide a function of this type to the \c OnReceiveError delegate
     *  member to process reception error events on \c endPoint. The \c err
     *  argument provides specific detail about the type of the error.
     */
    typedef void (*OnReceiveErrorFunct)(IPEndPointBasis *endPoint, INET_ERROR err, const IPPacketInfo *pktInfo);

    /** The endpoint's receive error event handling function delegate. */
    OnReceiveErrorFunct OnReceiveError;

    /**
     *  @brief Set whether IP multicast traffic should be looped back.
     *
     *  @param[in]   aIPVersion
     *
     *  @param[in]   aLoop
     *
     *  @retval  INET_NO_ERROR
     *       success: multicast loopback behavior set
     *  @retval  other
     *       another system or platform error
     *
     *  @details
     *     Set whether or not IP multicast traffic should be looped back
     *     to this endpoint.
     *
     */
    INET_ERROR SetMulticastLoopback(IPVersion aIPVersion, bool aLoopback);

    /**
     *  @brief Join an IP multicast group.
     *
     *  @param[in]   aInterfaceId  the indicator of the network interface to
     *                             add to the multicast group
     *
     *  @param[in]   aAddress      the multicast group to add the
     *                             interface to
     *
     *  @retval  INET_NO_ERROR
     *       success: multicast group removed
     *
     *  @retval  INET_ERROR_UNKNOWN_INTERFACE
     *       unknown network interface, \c aInterfaceId
     *
     *  @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *       \c aAddress is not \c kIPAddressType_IPv4 or
     *       \c kIPAddressType_IPv6 or is not multicast
     *
     *  @retval  other
     *       another system or platform error
     *
     *  @details
     *     Join the endpoint to the supplied multicast group on the
     *     specified interface.
     *
     */
    INET_ERROR JoinMulticastGroup(InterfaceId aInterfaceId, const IPAddress &aAddress);

    /**
     *  @brief Leave an IP multicast group.
     *
     *  @param[in]   aInterfaceId  the indicator of the network interface to
     *                             remove from the multicast group
     *
     *  @param[in]   aAddress      the multicast group to remove the
     *                             interface from
     *
     *  @retval  INET_NO_ERROR
     *       success: multicast group removed
     *
     *  @retval  INET_ERROR_UNKNOWN_INTERFACE
     *       unknown network interface, \c aInterfaceId
     *
     *  @retval  INET_ERROR_WRONG_ADDRESS_TYPE
     *       \c aAddress is not \c kIPAddressType_IPv4 or
     *       \c kIPAddressType_IPv6 or is not multicast
     *
     *  @retval  other
     *       another system or platform error
     *
     *  @details
     *     Remove the endpoint from the supplied multicast group on the
     *     specified interface.
     *
     */
    INET_ERROR LeaveMulticastGroup(InterfaceId aInterfaceId, const IPAddress &aAddress);

protected:
    void Init(InetLayer *aInetLayer);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
public:
    static inline struct netif *FindNetifFromInterfaceId(InterfaceId aInterfaceId)
    {
            struct netif *lRetval = NULL;

#if LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 0 && defined(NETIF_FOREACH)
            NETIF_FOREACH(lRetval)
            {
                if (lRetval == aInterfaceId)
                    break;
            }
#else // LWIP_VERSION_MAJOR < 2 || !defined(NETIF_FOREACH)
            for (lRetval = netif_list; lRetval != NULL && lRetval != aInterfaceId; lRetval = lRetval->next);
#endif // LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 0 && defined(NETIF_FOREACH)

            return (lRetval);
    }

protected:
    void HandleDataReceived(Weave::System::PacketBuffer *aBuffer);

    static IPPacketInfo *GetPacketInfo(Weave::System::PacketBuffer *buf);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
protected:
    InterfaceId mBoundIntfId;

    INET_ERROR Bind(IPAddressType aAddressType, IPAddress aAddress, uint16_t aPort, InterfaceId aInterfaceId);
    INET_ERROR BindInterface(IPAddressType aAddressType, InterfaceId aInterfaceId);
    INET_ERROR SendTo(const IPAddress &aAddress, uint16_t aPort, InterfaceId aInterfaceId, Weave::System::PacketBuffer *aBuffer, uint16_t aSendFlags);
    INET_ERROR GetSocket(IPAddressType aAddressType, int aType, int aProtocol);
    SocketEvents PrepareIO(void);
    void HandlePendingIO(uint16_t aPort);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

private:
    IPEndPointBasis(void);                     // not defined
    IPEndPointBasis(const IPEndPointBasis &);  // not defined
    ~IPEndPointBasis(void);                    // not defined
};

} // namespace Inet
} // namespace nl

#endif // !defined(IPENDPOINT_H)

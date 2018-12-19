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
                if (lRetval == aInterfaceid)
                    break;
            }
#else // LWIP_VERSION_MAJOR < 2 || !defined(NETIF_FOREACH)
            for (lRetval = netif_list; lRetval != NULL && lRetval != aInterfaceId; lRetval = lRetval->next);
#endif // LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 0 && defined(NETIF_FOREACH)

            return (lRetval);
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
protected:
    InterfaceId mBoundIntfId;

    INET_ERROR Bind(IPAddressType aAddressType, IPAddress aAddress, uint16_t aPort, InterfaceId aInterfaceId);
    INET_ERROR BindInterface(IPAddressType aAddressType, InterfaceId aInterfaceId);
    INET_ERROR SendTo(const IPAddress &aAddress, uint16_t aPort, InterfaceId aInterfaceId, Weave::System::PacketBuffer *aBuffer, uint16_t aSendFlags);
    INET_ERROR GetSocket(IPAddressType aAddressType, int aType, int aProtocol);
    INET_ERROR HandlePendingIO(uint16_t aPort, Weave::System::PacketBuffer *&aBuffer, IPPacketInfo &aPacketInfo);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

private:
    IPEndPointBasis(void);                     // not defined
    IPEndPointBasis(const IPEndPointBasis &);  // not defined
    ~IPEndPointBasis(void);                    // not defined
};

} // namespace Inet
} // namespace nl

#endif // !defined(IPENDPOINT_H)

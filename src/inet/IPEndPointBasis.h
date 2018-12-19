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
     * @brief   Transmit option flags for the \c SendTo methods.
     */
    enum {
        /** Do not destructively queue the message directly. Queue a copy. */
        kSendFlag_RetainBuffer = 0x0040
    };

protected:
    void Init(InetLayer *aInetLayer);

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

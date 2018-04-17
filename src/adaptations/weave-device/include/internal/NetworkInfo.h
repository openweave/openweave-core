/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

#include <internal/WeaveDeviceInternal.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class NetworkInfo
{
public:
    typedef ::nl::Weave::Profiles::NetworkProvisioning::NetworkType NetworkType_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiMode WiFiMode_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiRole WiFiRole_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType WiFiSecurityType_t;

    enum
    {
        kMaxWiFiSSIDLength = 32,
        kMaxWiFiKeyLength = 64
    };

    NetworkType_t NetworkType;              /**< The type of network. */
    uint32_t NetworkId;                     /**< The network id assigned to the network by the device. */
    bool NetworkIdPresent;                  /**< True if the NetworkId field is present. */

    // ---- WiFi-specific Fields ----
    char WiFiSSID[kMaxWiFiSSIDLength + 1];  /**< The WiFi SSID as a NULL-terminated string. */
    WiFiMode_t WiFiMode;                    /**< The operating mode of the WiFi network.*/
    WiFiRole_t WiFiRole;                    /**< The role played by the device on the WiFi network. */
    WiFiSecurityType_t WiFiSecurityType;    /**< The WiFi security type. */
    uint8_t WiFiKey[kMaxWiFiKeyLength];     /**< The WiFi key (NOT NULL-terminated). */
    uint8_t WiFiKeyLen;                     /**< The length in bytes of the WiFi key. */

    // ---- General Fields ----
    int16_t WirelessSignalStrength;         /**< The signal strength of the network, or INT16_MIN if not
                                             *   available/applicable. */

    void Reset();
    WEAVE_ERROR Decode(::nl::Weave::TLV::TLVReader & reader);
    WEAVE_ERROR Encode(::nl::Weave::TLV::TLVWriter & writer) const;
    WEAVE_ERROR MergeTo(NetworkInfo & dest);
    static WEAVE_ERROR EncodeArray(nl::Weave::TLV::TLVWriter & writer, const NetworkInfo * elems, size_t count);
};

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // NETWORK_INFO_H

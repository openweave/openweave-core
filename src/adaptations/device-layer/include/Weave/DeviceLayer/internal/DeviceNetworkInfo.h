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

#ifndef DEVICE_NETWORK_INFO_H
#define DEVICE_NETWORK_INFO_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Ids for well-known network provision types.
 */
enum
{
    kThreadNetworkId                                = 1,
    kWiFiStationNetworkId                           = 2,
};

class DeviceNetworkInfo
{
public:
    typedef ::nl::Weave::Profiles::NetworkProvisioning::NetworkType NetworkType_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiMode WiFiMode_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiRole WiFiRole_t;
    typedef ::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType WiFiSecurityType_t;

    enum
    {
        // ---- WiFi-specific Limits ----
        kMaxWiFiSSIDLength                  = 32,
        kMaxWiFiKeyLength                   = 64,

        // ---- Thread-specific Limits ----
        kMaxThreadNetworkNameLength         = 16,
        kThreadExtendedPANIdLength          = 8,
        kThreadMeshPrefixLength             = 8,
        kThreadNetworkKeyLength             = 16,
        kThreadPSKcLength                   = 16,
    };

    NetworkType_t NetworkType;              /**< The type of network. */
    uint32_t NetworkId;                     /**< The network id assigned to the network by the device. */

    // ---- WiFi-specific Fields ----
#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
    char WiFiSSID[kMaxWiFiSSIDLength + 1];  /**< The WiFi SSID as a NULL-terminated string. */
    WiFiMode_t WiFiMode;                    /**< The operating mode of the WiFi network.*/
    WiFiRole_t WiFiRole;                    /**< The role played by the device on the WiFi network. */
    WiFiSecurityType_t WiFiSecurityType;    /**< The WiFi security type. */
    uint8_t WiFiKey[kMaxWiFiKeyLength];     /**< The WiFi key (NOT NULL-terminated). */
    uint8_t WiFiKeyLen;                     /**< The length in bytes of the WiFi key. */
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION

    // ---- Thread-specific Fields ----
#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
    char ThreadNetworkName[kMaxThreadNetworkNameLength + 1];
                                            /**< The Thread network name as a NULL-terminated string. */
    uint8_t ThreadExtendedPANId[kThreadExtendedPANIdLength];
                                            /**< The Thread extended PAN ID. */
    uint8_t ThreadMeshPrefix[kThreadMeshPrefixLength];
                                            /**< The Thread mesh prefix. */
    uint8_t ThreadNetworkKey[kThreadNetworkKeyLength];
                                            /**< The Thread master network key (NOT NULL-terminated). */
    uint8_t ThreadPSKc[kThreadPSKcLength];
                                            /**< The Thread pre-shared commissioner key (NOT NULL-terminated). */
    uint32_t ThreadPANId;                   /**< The 16-bit Thread PAN ID, or kThreadPANId_NotSpecified */
    uint8_t ThreadChannel;                  /**< The Thread channel (currently [11..26]), or kThreadChannel_NotSpecified */
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD

    // ---- General Fields ----
    int16_t WirelessSignalStrength;         /**< The signal strength of the network, or INT16_MIN if not
                                                 available/applicable. */
    struct
    {
        bool NetworkId : 1;                 /**< True if the NetworkId field is present. */
        bool ThreadExtendedPANId : 1;       /**< True if the ThreadExtendedPANId field is present. */
        bool ThreadMeshPrefix : 1;          /**< True if the ThreadMeshPrefix field is present. */
        bool ThreadNetworkKey : 1;          /**< True if the ThreadNetworkKey field is present. */
        bool ThreadPSKc : 1;                /**< True if the ThreadPSKc field is present. */
    } FieldPresent;

    void Reset();
    WEAVE_ERROR Decode(::nl::Weave::TLV::TLVReader & reader);
    WEAVE_ERROR Encode(::nl::Weave::TLV::TLVWriter & writer) const;
    WEAVE_ERROR MergeTo(DeviceNetworkInfo & dest);
    static WEAVE_ERROR EncodeArray(nl::Weave::TLV::TLVWriter & writer, const DeviceNetworkInfo * elems, size_t count);
};

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // DEVICE_NETWORK_INFO_H

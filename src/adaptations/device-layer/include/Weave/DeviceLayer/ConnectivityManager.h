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

#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelConnectionMgr.h>
#include <Weave/Support/FlagUtils.hpp>
#include "esp_event.h"

namespace nl {
namespace Inet {
class IPAddress;
} // namespace Inet
} // namespace nl

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

class NetworkInfo;
class NetworkProvisioningServerImpl;
template<class ImplClass> class GenericNetworkProvisioningServerImpl;

extern const char *CharacterizeIPv6Address(const ::nl::Inet::IPAddress & ipAddr);

} // namespace Internal

class ConnectivityManager
{
public:

    enum WiFiStationMode
    {
        kWiFiStationMode_NotSupported               = -1,
        kWiFiStationMode_ApplicationControlled      = 0,
        kWiFiStationMode_Disabled                   = 1,
        kWiFiStationMode_Enabled                    = 2,
    };

    enum WiFiAPMode
    {
        kWiFiAPMode_NotSupported                    = -1,
        kWiFiAPMode_ApplicationControlled           = 0,
        kWiFiAPMode_Disabled                        = 1,
        kWiFiAPMode_Enabled                         = 2,
        kWiFiAPMode_OnDemand                        = 3,
        kWiFiAPMode_OnDemand_NoStationProvision     = 4,
    };

    enum ServiceTunnelMode
    {
        kServiceTunnelMode_NotSupported             = -1,
        kServiceTunnelMode_Disabled                 = 0,
        kServiceTunnelMode_Enabled                  = 1,
    };

    enum WoBLEServiceMode
    {
        kWoBLEServiceMode_NotSupported              = -1,
        kWoBLEServiceMode_Enabled                   = 0,
        kWoBLEServiceMode_Disabled                  = 1,
    };

    // WiFi station methods
    WiFiStationMode GetWiFiStationMode(void);
    WEAVE_ERROR SetWiFiStationMode(WiFiStationMode val);
    bool IsWiFiStationEnabled(void);
    bool IsWiFiStationApplicationControlled(void) const;
    bool IsWiFiStationConnected(void) const;
    uint32_t GetWiFiStationReconnectIntervalMS(void) const;
    WEAVE_ERROR SetWiFiStationReconnectIntervalMS(uint32_t val) const;
    bool IsWiFiStationProvisioned(void) const;
    void ClearWiFiStationProvision(void);

    // WiFi AP methods
    WiFiAPMode GetWiFiAPMode(void) const;
    WEAVE_ERROR SetWiFiAPMode(WiFiAPMode val);
    bool IsWiFiAPActive(void) const;
    bool IsWiFiAPApplicationControlled(void) const;
    void DemandStartWiFiAP(void);
    void StopOnDemandWiFiAP(void);
    void MaintainOnDemandWiFiAP(void);
    uint32_t GetWiFiAPIdleTimeoutMS(void) const;
    void SetWiFiAPIdleTimeoutMS(uint32_t val);

    // Internet connectivity methods
    bool HaveIPv4InternetConnectivity(void) const;
    bool HaveIPv6InternetConnectivity(void) const;

    // Service tunnel methods
    ServiceTunnelMode GetServiceTunnelMode(void) const;
    WEAVE_ERROR SetServiceTunnelMode(ServiceTunnelMode val);
    bool IsServiceTunnelConnected(void);
    bool IsServiceTunnelRestricted(void);

    // Service connectivity methods
    bool HaveServiceConnectivity(void);

    // WoBLE service methods
    WoBLEServiceMode GetWoBLEServiceMode(void);
    WEAVE_ERROR SetWoBLEServiceMode(WoBLEServiceMode val);
    bool IsBLEAdvertisingEnabled(void);
    WEAVE_ERROR SetBLEAdvertisingEnabled(bool val);
    bool IsBLEFastAdvertisingEnabled(void);
    WEAVE_ERROR SetBLEFastAdvertisingEnabled(bool val);
    WEAVE_ERROR GetBLEDeviceName(char * buf, size_t bufSize);
    WEAVE_ERROR SetBLEDeviceName(const char * deviceName);
    uint16_t NumBLEConnections(void);

private:

    // NOTE: These members are for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManager;
    friend class ::nl::Weave::DeviceLayer::Internal::NetworkProvisioningServerImpl;
    template<class ImplClass> friend class ::nl::Weave::DeviceLayer::Internal::GenericNetworkProvisioningServerImpl;

    WEAVE_ERROR Init(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);
    bool CanStartWiFiScan();
    void OnWiFiScanDone();
    void OnWiFiStationProvisionChange();

private:

    // NOTE: These members are private to the class and should not be used by friends.

    enum WiFiStationState
    {
        kWiFiStationState_NotConnected,
        kWiFiStationState_Connecting,
        kWiFiStationState_Connecting_Succeeded,
        kWiFiStationState_Connecting_Failed,
        kWiFiStationState_Connected,
        kWiFiStationState_Disconnecting,
    };

    enum WiFiAPState
    {
        kWiFiAPState_NotActive,
        kWiFiAPState_Activating,
        kWiFiAPState_Active,
        kWiFiAPState_Deactivating,
    };

    enum Flags
    {
        kFlag_HaveIPv4InternetConnectivity      = 0x0001,
        kFlag_HaveIPv6InternetConnectivity      = 0x0002,
        kFlag_ServiceTunnelStarted              = 0x0004,
        kFlag_ServiceTunnelUp                   = 0x0008,
        kFlag_AwaitingConnectivity              = 0x0010,
    };

    uint64_t mLastStationConnectFailTime;
    uint64_t mLastAPDemandTime;
    WiFiStationMode mWiFiStationMode;
    WiFiStationState mWiFiStationState;
    WiFiAPMode mWiFiAPMode;
    WiFiAPState mWiFiAPState;
    ServiceTunnelMode mServiceTunnelMode;
    uint32_t mWiFiStationReconnectIntervalMS;
    uint32_t mWiFiAPIdleTimeoutMS;
    uint16_t mFlags;

    void DriveStationState(void);
    void OnStationConnected(void);
    void OnStationDisconnected(void);
    void ChangeWiFiStationState(WiFiStationState newState);
    static void DriveStationState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

    void DriveAPState(void);
    WEAVE_ERROR ConfigureWiFiAP(void);
    void ChangeWiFiAPState(WiFiAPState newState);
    static void DriveAPState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

    void UpdateInternetConnectivityState(void);
    void OnStationIPv4AddressAvailable(const system_event_sta_got_ip_t & got_ip);
    void OnStationIPv4AddressLost(void);
    void OnIPv6AddressAvailable(const system_event_got_ip6_t & got_ip);

    void DriveServiceTunnelState(void);
    static void DriveServiceTunnelState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

    static const char * WiFiStationModeToStr(WiFiStationMode mode);
    static const char * WiFiStationStateToStr(WiFiStationState state);
    static const char * WiFiAPModeToStr(WiFiAPMode mode);
    static const char * WiFiAPStateToStr(WiFiAPState state);
    static void RefreshMessageLayer(void);
    static void HandleServiceTunnelNotification(::nl::Weave::Profiles::WeaveTunnel::WeaveTunnelConnectionMgr::TunnelConnNotifyReasons reason,
            WEAVE_ERROR err, void *appCtxt);
};

inline bool ConnectivityManager::IsWiFiStationApplicationControlled(void) const
{
    return mWiFiStationMode == kWiFiStationMode_ApplicationControlled;
}

inline bool ConnectivityManager::IsWiFiStationConnected(void) const
{
    return mWiFiStationState == kWiFiStationState_Connected;
}

inline bool ConnectivityManager::IsWiFiAPApplicationControlled(void) const
{
    return mWiFiAPMode == kWiFiAPMode_ApplicationControlled;
}

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void) const
{
    return mWiFiStationReconnectIntervalMS;
}

inline ConnectivityManager::WiFiAPMode ConnectivityManager::GetWiFiAPMode(void) const
{
    return mWiFiAPMode;
}

inline bool ConnectivityManager::IsWiFiAPActive(void) const
{
    return mWiFiAPState == kWiFiAPState_Active;
}

inline uint32_t ConnectivityManager::GetWiFiAPIdleTimeoutMS(void) const
{
    return mWiFiAPIdleTimeoutMS;
}

inline bool ConnectivityManager::HaveIPv4InternetConnectivity(void) const
{
    return ::nl::GetFlag(mFlags, kFlag_HaveIPv4InternetConnectivity);
}

inline bool ConnectivityManager::HaveIPv6InternetConnectivity(void) const
{
    return ::nl::GetFlag(mFlags, kFlag_HaveIPv6InternetConnectivity);
}

inline ConnectivityManager::ServiceTunnelMode ConnectivityManager::GetServiceTunnelMode(void) const
{
    return mServiceTunnelMode;
}

inline bool ConnectivityManager::CanStartWiFiScan()
{
    return mWiFiStationState != kWiFiStationState_Connecting;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONNECTIVITY_MANAGER_H

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

/**
 *    @file
 *          Defines the public interface for the Device Layer ConnectivityManager object.
 */

#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace Internal {
class NetworkProvisioningServerImpl;
template<class> class GenericNetworkProvisioningServerImpl;
} // namespace Internal

class ConnectivityManagerImpl;

/**
 * Provides control of network connectivity for a Weave device.
 */
class ConnectivityManager
{
    using ImplClass = ::nl::Weave::DeviceLayer::ConnectivityManagerImpl;

public:

    // ===== Members that define the public interface of the ConnectivityManager

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
    bool IsWiFiStationApplicationControlled(void);
    bool IsWiFiStationConnected(void);
    uint32_t GetWiFiStationReconnectIntervalMS(void);
    WEAVE_ERROR SetWiFiStationReconnectIntervalMS(uint32_t val);
    bool IsWiFiStationProvisioned(void);
    void ClearWiFiStationProvision(void);

    // WiFi AP methods
    WiFiAPMode GetWiFiAPMode(void);
    WEAVE_ERROR SetWiFiAPMode(WiFiAPMode val);
    bool IsWiFiAPActive(void);
    bool IsWiFiAPApplicationControlled(void);
    void DemandStartWiFiAP(void);
    void StopOnDemandWiFiAP(void);
    void MaintainOnDemandWiFiAP(void);
    uint32_t GetWiFiAPIdleTimeoutMS(void);
    void SetWiFiAPIdleTimeoutMS(uint32_t val);

    // Internet connectivity methods
    bool HaveIPv4InternetConnectivity(void);
    bool HaveIPv6InternetConnectivity(void);

    // Service tunnel methods
    ServiceTunnelMode GetServiceTunnelMode(void);
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

    // ===== Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManagerImpl;
    friend class ::nl::Weave::DeviceLayer::Internal::NetworkProvisioningServerImpl;
    template<class> friend class ::nl::Weave::DeviceLayer::Internal::GenericNetworkProvisioningServerImpl;

    WEAVE_ERROR Init(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);
    bool CanStartWiFiScan(void);
    void OnWiFiScanDone(void);
    void OnWiFiStationProvisionChange(void);
};

/**
 * Returns a reference to the public interface of the ConnectivityManager singleton object.
 *
 * Weave applications should use this to access features of the ConnectivityManager object
 * that are common to all platforms.
 */
extern ConnectivityManager & ConnectivityMgr(void);

/**
 * Returns the platform-specific implementation of the ConnectivityManager singleton object.
 *
 * Weave applications can use this to gain access to features of the ConnectivityManager
 * that are specific to the selected platform.
 */
extern ConnectivityManagerImpl & ConnectivityMgrImpl(void);

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


/* Include a header file containing the implementation of the ConfigurationManager
 * object for the selected platform.
 */
#ifdef EXTERNAL_CONNECTIVITYMANAGERIMPL_HEADER
#include EXTERNAL_CONNECTIVITYMANAGERIMPL_HEADER
#else
#define CONNECTIVITYMANAGERIMPL_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/ConnectivityManagerImpl.h>
#include CONNECTIVITYMANAGERIMPL_HEADER
#endif


namespace nl {
namespace Weave {
namespace DeviceLayer {

inline ConnectivityManager::WiFiStationMode ConnectivityManager::GetWiFiStationMode(void)
{
    return static_cast<ImplClass*>(this)->_GetWiFiStationMode();
}

inline WEAVE_ERROR ConnectivityManager::SetWiFiStationMode(WiFiStationMode val)
{
    return static_cast<ImplClass*>(this)->_SetWiFiStationMode(val);
}

inline bool ConnectivityManager::IsWiFiStationEnabled(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiStationEnabled();
}

inline bool ConnectivityManager::IsWiFiStationApplicationControlled(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiStationApplicationControlled();
}

inline bool ConnectivityManager::IsWiFiStationConnected(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiStationConnected();
}

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void)
{
    return static_cast<ImplClass*>(this)->_GetWiFiStationReconnectIntervalMS();
}

inline WEAVE_ERROR ConnectivityManager::SetWiFiStationReconnectIntervalMS(uint32_t val)
{
    return static_cast<ImplClass*>(this)->_SetWiFiStationReconnectIntervalMS(val);
}

inline bool ConnectivityManager::IsWiFiStationProvisioned(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiStationProvisioned();
}

inline void ConnectivityManager::ClearWiFiStationProvision(void)
{
    static_cast<ImplClass*>(this)->_ClearWiFiStationProvision();
}

inline ConnectivityManager::WiFiAPMode ConnectivityManager::GetWiFiAPMode(void)
{
    return static_cast<ImplClass*>(this)->_GetWiFiAPMode();
}

inline WEAVE_ERROR ConnectivityManager::SetWiFiAPMode(WiFiAPMode val)
{
    return static_cast<ImplClass*>(this)->_SetWiFiAPMode(val);
}

inline bool ConnectivityManager::IsWiFiAPActive(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiAPActive();
}

inline bool ConnectivityManager::IsWiFiAPApplicationControlled(void)
{
    return static_cast<ImplClass*>(this)->_IsWiFiAPApplicationControlled();
}

inline void ConnectivityManager::DemandStartWiFiAP(void)
{
    static_cast<ImplClass*>(this)->_DemandStartWiFiAP();
}

inline void ConnectivityManager::StopOnDemandWiFiAP(void)
{
    static_cast<ImplClass*>(this)->_StopOnDemandWiFiAP();
}

inline void ConnectivityManager::MaintainOnDemandWiFiAP(void)
{
    static_cast<ImplClass*>(this)->_MaintainOnDemandWiFiAP();
}

inline uint32_t ConnectivityManager::GetWiFiAPIdleTimeoutMS(void)
{
    return static_cast<ImplClass*>(this)->_GetWiFiAPIdleTimeoutMS();
}

inline void ConnectivityManager::SetWiFiAPIdleTimeoutMS(uint32_t val)
{
    static_cast<ImplClass*>(this)->_SetWiFiAPIdleTimeoutMS(val);
}

inline bool ConnectivityManager::HaveIPv4InternetConnectivity(void)
{
    return static_cast<ImplClass*>(this)->_HaveIPv4InternetConnectivity();
}

inline bool ConnectivityManager::HaveIPv6InternetConnectivity(void)
{
    return static_cast<ImplClass*>(this)->_HaveIPv6InternetConnectivity();
}

inline ConnectivityManager::ServiceTunnelMode ConnectivityManager::GetServiceTunnelMode(void)
{
    return static_cast<ImplClass*>(this)->_GetServiceTunnelMode();
}

inline WEAVE_ERROR ConnectivityManager::SetServiceTunnelMode(ServiceTunnelMode val)
{
    return static_cast<ImplClass*>(this)->_SetServiceTunnelMode(val);
}

inline bool ConnectivityManager::IsServiceTunnelConnected(void)
{
    return static_cast<ImplClass*>(this)->_IsServiceTunnelConnected();
}

inline bool ConnectivityManager::IsServiceTunnelRestricted(void)
{
    return static_cast<ImplClass*>(this)->_IsServiceTunnelRestricted();
}

inline bool ConnectivityManager::HaveServiceConnectivity(void)
{
    return static_cast<ImplClass*>(this)->_HaveServiceConnectivity();
}

inline ConnectivityManager::WoBLEServiceMode ConnectivityManager::GetWoBLEServiceMode(void)
{
    return static_cast<ImplClass*>(this)->_GetWoBLEServiceMode();
}

inline WEAVE_ERROR ConnectivityManager::SetWoBLEServiceMode(WoBLEServiceMode val)
{
    return static_cast<ImplClass*>(this)->_SetWoBLEServiceMode(val);
}

inline bool ConnectivityManager::IsBLEAdvertisingEnabled(void)
{
    return static_cast<ImplClass*>(this)->_IsBLEAdvertisingEnabled();
}

inline WEAVE_ERROR ConnectivityManager::SetBLEAdvertisingEnabled(bool val)
{
    return static_cast<ImplClass*>(this)->_SetBLEAdvertisingEnabled(val);
}

inline bool ConnectivityManager::IsBLEFastAdvertisingEnabled(void)
{
    return static_cast<ImplClass*>(this)->_IsBLEFastAdvertisingEnabled();
}

inline WEAVE_ERROR ConnectivityManager::SetBLEFastAdvertisingEnabled(bool val)
{
    return static_cast<ImplClass*>(this)->_SetBLEFastAdvertisingEnabled(val);
}

inline WEAVE_ERROR ConnectivityManager::GetBLEDeviceName(char * buf, size_t bufSize)
{
    return static_cast<ImplClass*>(this)->_GetBLEDeviceName(buf, bufSize);
}

inline WEAVE_ERROR ConnectivityManager::SetBLEDeviceName(const char * deviceName)
{
    return static_cast<ImplClass*>(this)->_SetBLEDeviceName(deviceName);
}

inline uint16_t ConnectivityManager::NumBLEConnections(void)
{
    return static_cast<ImplClass*>(this)->_NumBLEConnections();
}

inline WEAVE_ERROR ConnectivityManager::Init(void)
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline void ConnectivityManager::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    static_cast<ImplClass*>(this)->_OnPlatformEvent(event);
}

inline bool ConnectivityManager::CanStartWiFiScan(void)
{
    return static_cast<ImplClass*>(this)->_CanStartWiFiScan();
}

inline void ConnectivityManager::OnWiFiScanDone(void)
{
    static_cast<ImplClass*>(this)->_OnWiFiScanDone();
}

inline void ConnectivityManager::OnWiFiStationProvisionChange(void)
{
    static_cast<ImplClass*>(this)->_OnWiFiStationProvisionChange();
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONNECTIVITY_MANAGER_H

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

#ifndef CONNECTIVITY_MANAGER_IMPL_H
#define CONNECTIVITY_MANAGER_IMPL_H

#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Support/FlagUtils.hpp>

namespace nl {
namespace Inet {
class IPAddress;
} // namespace Inet
} // namespace nl

namespace nl {
namespace Weave {
namespace DeviceLayer {

class PlatformManagerImpl;

namespace Internal {

class NetworkProvisioningServerImpl;
template<class ImplClass> class GenericNetworkProvisioningServerImpl;

} // namespace Internal

/**
 * Concrete implementation of the ConnectivityManager singleton object for the ESP32 platform.
 */
class ConnectivityManagerImpl final
    : public ConnectivityManager
{
    // Allow the ConnectivityManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend class ConnectivityManager;

private:

    // ===== Members that implement the ConnectivityManager abstract interface.

    WiFiStationMode _GetWiFiStationMode(void);
    WEAVE_ERROR _SetWiFiStationMode(WiFiStationMode val);
    bool _IsWiFiStationEnabled(void);
    bool _IsWiFiStationApplicationControlled(void);
    bool _IsWiFiStationConnected(void);
    uint32_t _GetWiFiStationReconnectIntervalMS(void);
    WEAVE_ERROR _SetWiFiStationReconnectIntervalMS(uint32_t val);
    bool _IsWiFiStationProvisioned(void);
    void _ClearWiFiStationProvision(void);
    WiFiAPMode _GetWiFiAPMode(void);
    WEAVE_ERROR _SetWiFiAPMode(WiFiAPMode val);
    bool _IsWiFiAPActive(void);
    bool _IsWiFiAPApplicationControlled(void);
    void _DemandStartWiFiAP(void);
    void _StopOnDemandWiFiAP(void);
    void _MaintainOnDemandWiFiAP(void);
    uint32_t _GetWiFiAPIdleTimeoutMS(void);
    void _SetWiFiAPIdleTimeoutMS(uint32_t val);
    bool _HaveIPv4InternetConnectivity(void);
    bool _HaveIPv6InternetConnectivity(void);
    ServiceTunnelMode _GetServiceTunnelMode(void);
    WEAVE_ERROR _SetServiceTunnelMode(ServiceTunnelMode val);
    bool _IsServiceTunnelConnected(void);
    bool _IsServiceTunnelRestricted(void);
    bool _HaveServiceConnectivity(void);
    WoBLEServiceMode _GetWoBLEServiceMode(void);
    WEAVE_ERROR _SetWoBLEServiceMode(WoBLEServiceMode val);
    bool _IsBLEAdvertisingEnabled(void);
    WEAVE_ERROR _SetBLEAdvertisingEnabled(bool val);
    bool _IsBLEFastAdvertisingEnabled(void);
    WEAVE_ERROR _SetBLEFastAdvertisingEnabled(bool val);
    WEAVE_ERROR _GetBLEDeviceName(char * buf, size_t bufSize);
    WEAVE_ERROR _SetBLEDeviceName(const char * deviceName);
    uint16_t _NumBLEConnections(void);
    WEAVE_ERROR _Init(void);
    void _OnPlatformEvent(const WeaveDeviceEvent * event);
    bool _CanStartWiFiScan();
    void _OnWiFiScanDone();
    void _OnWiFiStationProvisionChange();
    static const char * _WiFiStationModeToStr(WiFiStationMode mode);
    static const char * _WiFiAPModeToStr(WiFiAPMode mode);
    static const char * _ServiceTunnelModeToStr(ServiceTunnelMode mode);
    static const char * _WoBLEServiceModeToStr(WoBLEServiceMode mode);

    // ===== Members for internal use by the following friends.

    friend ConnectivityManager & ConnectivityMgr(void);
    friend ConnectivityManagerImpl & ConnectivityMgrImpl(void);

    static ConnectivityManagerImpl sInstance;

    // ===== Private members reserved for use by this class only.

    enum Flags
    {
        kFlag_HaveIPv6InternetConnectivity      = 0x0002,
        kFlag_AwaitingConnectivity              = 0x0010,
    };

    uint16_t mFlags;
};

inline bool ConnectivityManagerImpl::_IsWiFiStationApplicationControlled(void)
{
    return false;
}

inline WEAVE_ERROR ConnectivityManagerImpl::_SetWiFiStationMode(WiFiStationMode val)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

inline bool ConnectivityManagerImpl::_IsWiFiStationEnabled(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_IsWiFiStationConnected(void)
{
    return false;
}

inline uint32_t ConnectivityManagerImpl::_GetWiFiStationReconnectIntervalMS(void)
{
    return 0;
}

inline WEAVE_ERROR ConnectivityManagerImpl::_SetWiFiStationReconnectIntervalMS(uint32_t val)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

inline bool ConnectivityManagerImpl::_IsWiFiStationProvisioned(void)
{
    return false;
}

inline void ConnectivityManagerImpl::_ClearWiFiStationProvision(void)
{
}

inline ConnectivityManager::WiFiAPMode ConnectivityManagerImpl::_GetWiFiAPMode(void)
{
    return kWiFiAPMode_NotSupported;
}

inline WEAVE_ERROR ConnectivityManagerImpl::_SetWiFiAPMode(WiFiAPMode val)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

inline bool ConnectivityManagerImpl::_IsWiFiAPActive(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_IsWiFiAPApplicationControlled(void)
{
    return false;
}

inline void ConnectivityManagerImpl::_DemandStartWiFiAP(void)
{
}

inline void ConnectivityManagerImpl::_StopOnDemandWiFiAP(void)
{
}

inline void ConnectivityManagerImpl::_MaintainOnDemandWiFiAP(void)
{
}

inline uint32_t ConnectivityManagerImpl::_GetWiFiAPIdleTimeoutMS(void)
{
    return 0;
}

inline void ConnectivityManagerImpl::_SetWiFiAPIdleTimeoutMS(uint32_t val)
{
}

inline bool ConnectivityManagerImpl::_HaveIPv4InternetConnectivity(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_HaveIPv6InternetConnectivity(void)
{
    return ::nl::GetFlag(mFlags, kFlag_HaveIPv6InternetConnectivity);
}

inline ConnectivityManager::ServiceTunnelMode ConnectivityManagerImpl::_GetServiceTunnelMode(void)
{
    return kServiceTunnelMode_NotSupported;
}

inline WEAVE_ERROR ConnectivityManagerImpl::_SetServiceTunnelMode(ServiceTunnelMode val)
{
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
}

inline bool ConnectivityManagerImpl::_IsServiceTunnelConnected(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_IsServiceTunnelRestricted(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_HaveServiceConnectivity(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_CanStartWiFiScan()
{
    return false;
}

inline void ConnectivityManagerImpl::_OnWiFiScanDone()
{
}

inline void ConnectivityManagerImpl::_OnWiFiStationProvisionChange()
{
}

inline const char * ConnectivityManagerImpl::_WiFiStationModeToStr(WiFiStationMode mode)
{
    return NULL;
}

inline const char * ConnectivityManagerImpl::_WiFiAPModeToStr(WiFiAPMode mode)
{
    return NULL;
}

inline const char * ConnectivityManagerImpl::_ServiceTunnelModeToStr(ServiceTunnelMode mode)
{
    return NULL;
}

inline const char * ConnectivityManagerImpl::_WoBLEServiceModeToStr(WoBLEServiceMode mode)
{
    return NULL;
}


/**
 * Returns the public interface of the ConnectivityManager singleton object.
 *
 * Weave applications should use this to access features of the ConnectivityManager object
 * that are common to all platforms.
 */
inline ConnectivityManager & ConnectivityMgr(void)
{
    return ConnectivityManagerImpl::sInstance;
}

/**
 * Returns the platform-specific implementation of the ConnectivityManager singleton object.
 *
 * Weave applications can use this to gain access to features of the ConnectivityManager
 * that are specific to the ESP32 platform.
 */
inline ConnectivityManagerImpl & ConnectivityMgrImpl(void)
{
    return ConnectivityManagerImpl::sInstance;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CONNECTIVITY_MANAGER_IMPL_H

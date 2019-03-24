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

#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
#include <Weave/DeviceLayer/internal/GenericConnectivityManagerImpl_BLE.h>
#else
#include <Weave/DeviceLayer/internal/GenericConnectivityManagerImpl_NoBLE.h>
#endif
#include <Weave/DeviceLayer/internal/GenericConnectivityManagerImpl_NoWiFi.h>
#include <Weave/DeviceLayer/internal/GenericConnectivityManagerImpl_NoTunnel.h>
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
 * Concrete implementation of the ConnectivityManager singleton object for Nordic nRF52 platforms.
 */
class ConnectivityManagerImpl final
    : public ConnectivityManager,
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
      public Internal::GenericConnectivityManagerImpl_BLE<ConnectivityManagerImpl>,
#else
      public Internal::GenericConnectivityManagerImpl_NoBLE<ConnectivityManagerImpl>,
#endif
      public Internal::GenericConnectivityManagerImpl_NoWiFi<ConnectivityManagerImpl>,
      public Internal::GenericConnectivityManagerImpl_NoTunnel<ConnectivityManagerImpl>
{
    // Allow the ConnectivityManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend class ConnectivityManager;

private:

    // ===== Members that implement the ConnectivityManager abstract interface.

    bool _HaveIPv4InternetConnectivity(void);
    bool _HaveIPv6InternetConnectivity(void);
    bool _HaveServiceConnectivity(void);
    WEAVE_ERROR _Init(void);
    void _OnPlatformEvent(const WeaveDeviceEvent * event);

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

inline bool ConnectivityManagerImpl::_HaveIPv4InternetConnectivity(void)
{
    return false;
}

inline bool ConnectivityManagerImpl::_HaveIPv6InternetConnectivity(void)
{
    return ::nl::GetFlag(mFlags, kFlag_HaveIPv6InternetConnectivity);
}

inline bool ConnectivityManagerImpl::_HaveServiceConnectivity(void)
{
    // TODO: implement this
    return true;
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

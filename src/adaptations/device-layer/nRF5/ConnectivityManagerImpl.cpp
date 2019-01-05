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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ConnectivityManager.h>
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/NetworkInfo.h>
#include <Weave/DeviceLayer/internal/ServiceTunnelAgent.h>
#include <Weave/DeviceLayer/internal/BLEManager.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Warm/Warm.h>

#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/nd6.h>
#include <lwip/dns.h>

#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;
using namespace ::nl::Weave::DeviceLayer::Internal;

using Profiles::kWeaveProfile_Common;
using Profiles::kWeaveProfile_NetworkProvisioning;

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace {

inline ConnectivityChange GetConnectivityChange(bool prevState, bool newState)
{
    if (prevState == newState)
        return kConnectivity_NoChange;
    else if (newState)
        return kConnectivity_Established;
    else
        return kConnectivity_Lost;
}

} // unnamed namespace


ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;

ConnectivityManager::WoBLEServiceMode ConnectivityManagerImpl::_GetWoBLEServiceMode(void)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().GetWoBLEServiceMode();
#else
    return kWoBLEServiceMode_NotSupported;
#endif
}

WEAVE_ERROR ConnectivityManagerImpl::_SetWoBLEServiceMode(WoBLEServiceMode val)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().SetWoBLEServiceMode(val);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
}

bool ConnectivityManagerImpl::_IsBLEAdvertisingEnabled(void)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().IsAdvertisingEnabled();
#else
    return false;
#endif
}

WEAVE_ERROR ConnectivityManagerImpl::_SetBLEAdvertisingEnabled(bool val)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().SetAdvertisingEnabled(val);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
}

bool ConnectivityManagerImpl::_IsBLEFastAdvertisingEnabled(void)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().IsFastAdvertisingEnabled();
#else
    return false;
#endif
}

WEAVE_ERROR ConnectivityManagerImpl::_SetBLEFastAdvertisingEnabled(bool val)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().SetFastAdvertisingEnabled(val);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
}

WEAVE_ERROR ConnectivityManagerImpl::_GetBLEDeviceName(char * buf, size_t bufSize)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().GetDeviceName(buf, bufSize);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
}

WEAVE_ERROR ConnectivityManagerImpl::_SetBLEDeviceName(const char * deviceName)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().SetDeviceName(deviceName);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
}

uint16_t ConnectivityManagerImpl::_NumBLEConnections(void)
{
#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
    return BLEMgr().NumConnections();
#else
    return 0;
#endif
}

// ==================== ConnectivityManager Platform Internal Methods ====================

WEAVE_ERROR ConnectivityManagerImpl::_Init()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // TODO: implement me
    mFlags = 0;

    return err;
}

void ConnectivityManagerImpl::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // TODO: implement me
}

// ==================== ConnectivityManager Private Methods ====================

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

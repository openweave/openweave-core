/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      Defines the Weave Device Layer Network Telemetry Manager object.
 *
 */

#include <Weave/DeviceLayer/WeaveDeviceConfig.h>
#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

#if WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

#ifndef NETWORK_TELEMETRY_MANAGER_H
#define NETWORK_TELEMETRY_MANAGER_H

namespace nl {
namespace Weave {
namespace DeviceLayer {

extern nl::Weave::System::Layer SystemLayer;

namespace Internal {
template<class> class GenericPlatformManagerImpl;
}

/**
 * Manages network telemetry logging for a Weave device.
 */
class NetworkTelemetryManager final
{
public:
    NetworkTelemetryManager(void);

    void EnableTimer(void);
    void DisableTimer(void);
    bool IsTimerEnabled(void) const;
    void SetPollingInterval(uint32_t aIntervalMsec);
    uint32_t GetPollingInterval(void) const;

    void GetAndLogNetworkTelemetry(void);

private:

    // ===== Members for internal use by the following friends.

    template<class> friend class Internal::GenericPlatformManagerImpl;
    friend NetworkTelemetryManager & NetworkTelemetryMgr(void);

    static NetworkTelemetryManager sInstance;

    void Init(void);

    // ===== Private members for use by this class only.

    bool mTimerEnabled;
    uint32_t mPollingInterval;

    void StartPollingTimer(void);
    void StopPollingTimer(void);
    void HandleTimer(void);
    static void sHandleTimer(nl::Weave::System::Layer * aLayer, void * aAppState, WEAVE_ERROR aError);

#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
    WEAVE_ERROR GetAndLogTunnelTelemetryStats(void);
#endif
};

inline bool NetworkTelemetryManager::IsTimerEnabled(void) const
{
    return mTimerEnabled;
}

inline void NetworkTelemetryManager::SetPollingInterval(uint32_t aIntervalMsec)
{
    mPollingInterval = aIntervalMsec;
}

inline uint32_t NetworkTelemetryManager::GetPollingInterval(void) const
{
    return mPollingInterval;
}

inline void NetworkTelemetryManager::StartPollingTimer(void)
{
    nl::Weave::DeviceLayer::SystemLayer.StartTimer(mPollingInterval, sHandleTimer, this);
}

inline void NetworkTelemetryManager::StopPollingTimer(void)
{
    nl::Weave::DeviceLayer::SystemLayer.CancelTimer(sHandleTimer, this);
}

inline void NetworkTelemetryManager::sHandleTimer(nl::Weave::System::Layer * aLayer, void * aAppState, WEAVE_ERROR aError)
{
    static_cast<NetworkTelemetryManager *>(aAppState)->HandleTimer();
}

/**
 * Returns a reference to the NetworkTelemetryManager singleton object.
 */
inline NetworkTelemetryManager & NetworkTelemetryMgr(void)
{
    return NetworkTelemetryManager::sInstance;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NETWORK_TELEMETRY_MANAGER_H

#endif // WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

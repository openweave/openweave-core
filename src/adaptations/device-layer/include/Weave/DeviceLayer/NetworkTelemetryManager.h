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

#if WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

#ifndef NETWORK_TELEMETRY_MANAGER_H
#define NETWORK_TELEMETRY_MANAGER_H

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

#include <SystemLayer/SystemLayer.h>

/**
 * This is a base class that handles network telemetry functions
 * for different networks.
 */
class WeaveTelemetryBase
{
public:
    WeaveTelemetryBase();

    void Init(uint32_t aIntervalMsec);

    void Enable(void);
    void Disable(void);
    bool IsEnabled(void) const;
    void SetPollingInterval(uint32_t aIntervalMsec);
    uint32_t GetPollingInterval(void) const;

private:
    void StartPollingTimer(void);
    void StopPollingTimer(void);
    void HandleTimer(void);
    static void sHandleTimer(nl::Weave::System::Layer * aLayer, void * aAppState, WEAVE_ERROR aError);
    virtual void GetTelemetryStatsAndLogEvent(void) = 0;

private:
    bool mEnabled;
    uint32_t mInterval;
};

inline bool WeaveTelemetryBase::IsEnabled(void) const
{
    return mEnabled;
}

inline void WeaveTelemetryBase::SetPollingInterval(uint32_t aIntervalMsec)
{
    mInterval = aIntervalMsec;
}

inline uint32_t WeaveTelemetryBase::GetPollingInterval(void) const
{
    return mInterval;
}

inline void WeaveTelemetryBase::sHandleTimer(nl::Weave::System::Layer * aLayer, void * aAppState, WEAVE_ERROR aError)
{
    static_cast<WeaveTelemetryBase *>(aAppState)->HandleTimer();
}


#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY
class WiFiTelemetry : public WeaveTelemetryBase
{
protected:
    virtual void GetTelemetryStatsAndLogEvent(void);
};
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY


#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY
class ThreadTelemetry : public WeaveTelemetryBase
{
protected:
    virtual void GetTelemetryStatsAndLogEvent(void);
};

class ThreadTopology : public WeaveTelemetryBase
{
protected:
    virtual void GetTelemetryStatsAndLogEvent(void);
};
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY


#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
class TunnelTelemetry : public WeaveTelemetryBase
{
protected:
    virtual void GetTelemetryStatsAndLogEvent(void);
};
#endif // WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY


class NetworkTelemetryManager
{
public:
    NetworkTelemetryManager(void);

    WEAVE_ERROR Init(void);

#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY
    WiFiTelemetry mWiFiTelemetry;
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY
    ThreadTelemetry mThreadTelemetry;
    ThreadTopology mThreadTopology;
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
    TunnelTelemetry mTunnelTelemetry;
#endif

private:
    friend NetworkTelemetryManager & NetworkTelemetryMgr(void);

    static NetworkTelemetryManager sInstance;
};

/**
 * Returns a reference to the NetworkTelemetryManager singleton object.
 */
inline NetworkTelemetryManager & NetworkTelemetryMgr(void)
{
    return NetworkTelemetryManager::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NETWORK_TELEMETRY_MANAGER_H

#endif // WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

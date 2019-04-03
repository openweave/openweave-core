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
 *      Implementation for the Weave Device Layer Network Telemetry object.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

#if WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

#include <Weave/DeviceLayer/NetworkTelemetryManager.h>

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY
#include <Weave/DeviceLayer/ThreadStackManager.h>
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
#include <Weave/Support/TraitEventUtils.h>
#include <Weave/DeviceLayer/internal/ServiceTunnelAgent.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <weave/trait/telemetry/tunnel/TelemetryTunnelTrait.h>
#endif

using namespace nl::Weave::DeviceLayer;
using namespace nl::Weave::DeviceLayer::Internal;

WeaveTelemetryBase::WeaveTelemetryBase()
{
}

void WeaveTelemetryBase::Init(uint32_t aIntervalMsec)
{
    SetPollingInterval(aIntervalMsec);
    Enable();
}

void WeaveTelemetryBase::Enable(void)
{
    mEnabled = true;
    StartPollingTimer();
}

void WeaveTelemetryBase::Disable(void)
{
    mEnabled = false;
    StopPollingTimer();
}

void WeaveTelemetryBase::StartPollingTimer(void)
{
    SystemLayer.StartTimer(mInterval, sHandleTimer, this);
}

void WeaveTelemetryBase::StopPollingTimer(void)
{
    SystemLayer.CancelTimer(sHandleTimer, this);
}

void WeaveTelemetryBase::HandleTimer(void)
{
    GetTelemetryStatsAndLogEvent();

    StartPollingTimer();
}

NetworkTelemetryManager::NetworkTelemetryManager(void)
{
}

WEAVE_ERROR NetworkTelemetryManager::Init(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceLayer, "Initiating Network Telemetry & Topology\n");

#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY
    mWiFiTelemetry.Init(WEAVE_DEVICE_CONFIG_DEFAULT_TELEMETRY_INTERVAL_MS);
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY
    mThreadTelemetry.Init(WEAVE_DEVICE_CONFIG_DEFAULT_TELEMETRY_INTERVAL_MS);
    mThreadTopology.Init(WEAVE_DEVICE_CONFIG_DEFAULT_TELEMETRY_INTERVAL_MS);
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
    mTunnelTelemetry.Init(WEAVE_DEVICE_CONFIG_DEFAULT_TUNNEL_TELEMETRY_INTERVAL_MS);
#endif

    return err;
}


#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY
void WiFiTelemetry::GetTelemetryStatsAndLogEvent(void)
{
    WEAVE_ERROR err;

    err = ConnectivityMgr().GetAndLogWifiStatsCounters();
    SuccessOrExit(err);

exit:
    return;
}
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_TELEMETRY


#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY
void ThreadTelemetry::GetTelemetryStatsAndLogEvent(void)
{
    WEAVE_ERROR err;

    err = ThreadStackMgr().GetAndLogThreadStatsCounters();
    SuccessOrExit(err);

exit:
    return;
}

void ThreadTopology::GetTelemetryStatsAndLogEvent(void)
{
    WEAVE_ERROR err;

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY_FULL
    err = ThreadStackMgr().GetAndLogThreadTopologyFull();
    SuccessOrExit(err);
#else
    err = ThreadStackMgr().GetAndLogThreadTopologyMinimal();
    SuccessOrExit(err);
#endif

exit:
    return;
}
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD_TELEMETRY


#if WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY
void TunnelTelemetry::GetTelemetryStatsAndLogEvent(void)
{
    nl::Weave::Profiles::DataManagement_Current::event_id_t eventId;
    nl::Weave::Profiles::WeaveTunnel::WeaveTunnelStatistics tunnelStats;
    Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TelemetryTunnelStatsEvent statsEvent;

    ServiceTunnelAgent.GetWeaveTunnelStatistics(tunnelStats);

    statsEvent.txBytesToService       = tunnelStats.mPrimaryStats.mTxBytesToService;
    statsEvent.rxBytesFromService     = tunnelStats.mPrimaryStats.mRxBytesFromService;
    statsEvent.txMessagesToService    = tunnelStats.mPrimaryStats.mTxMessagesToService;
    statsEvent.rxMessagesFromService  = tunnelStats.mPrimaryStats.mRxMessagesFromService;
    statsEvent.tunnelDownCount        = tunnelStats.mPrimaryStats.mTunnelDownCount;
    statsEvent.tunnelConnAttemptCount = tunnelStats.mPrimaryStats.mTunnelConnAttemptCount;

    statsEvent.lastTimeTunnelWentDown    = tunnelStats.mPrimaryStats.mLastTimeTunnelWentDown;
    statsEvent.lastTimeTunnelEstablished = tunnelStats.mPrimaryStats.mLastTimeTunnelEstablished;

    statsEvent.droppedMessagesCount = tunnelStats.mDroppedMessagesCount;

    switch (ServiceTunnelAgent.GetWeaveTunnelAgentState())
    {
    case nl::Weave::Profiles::WeaveTunnel::WeaveTunnelAgent::kState_Initialized_NoTunnel:
        statsEvent.currentTunnelState = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_STATE_NO_TUNNEL;
        break;
    case nl::Weave::Profiles::WeaveTunnel::WeaveTunnelAgent::kState_PrimaryTunModeEstablished:
        statsEvent.currentTunnelState = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_STATE_PRIMARY_ESTABLISHED;
        break;
    case nl::Weave::Profiles::WeaveTunnel::WeaveTunnelAgent::kState_BkupOnlyTunModeEstablished:
        statsEvent.currentTunnelState =
            Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_STATE_BACKUP_ONLY_ESTABLISHED;
        break;
    case nl::Weave::Profiles::WeaveTunnel::WeaveTunnelAgent::kState_PrimaryAndBkupTunModeEstablished:
        statsEvent.currentTunnelState =
            Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_STATE_PRIMARY_AND_BACKUP_ESTABLISHED;
        break;
    default:
        break;
    }

    switch (tunnelStats.mCurrentActiveTunnel)
    {
    case nl::Weave::Profiles::WeaveTunnel::kType_TunnelUnknown:
        statsEvent.currentActiveTunnel = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_TYPE_NONE;
        break;
    case nl::Weave::Profiles::WeaveTunnel::kType_TunnelPrimary:
        statsEvent.currentActiveTunnel = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_TYPE_PRIMARY;
        break;
    case nl::Weave::Profiles::WeaveTunnel::kType_TunnelBackup:
        statsEvent.currentActiveTunnel = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_TYPE_BACKUP;
        break;
    case nl::Weave::Profiles::WeaveTunnel::kType_TunnelShortcut:
        statsEvent.currentActiveTunnel = Schema::Weave::Trait::Telemetry::Tunnel::TelemetryTunnelTrait::TUNNEL_TYPE_SHORTCUT;
        break;
    default:
        break;
    }

    WeaveLogProgress(DeviceLayer,
                     "Weave Tunnel Counters\n"
                     "Tx Messages:                   %d\n"
                     "Rx Messages:                   %d\n"
                     "Tunnel Down Count:             %d\n"
                     "Tunnel Conn Attempt Count:     %d\n"
                     "Tunnel State:                  %d\n"
                     "CurrentActiveTunnel:           %d\n",
                     statsEvent.txMessagesToService, statsEvent.rxMessagesFromService, statsEvent.tunnelDownCount, statsEvent.tunnelConnAttemptCount,
                     statsEvent.currentTunnelState, statsEvent.currentActiveTunnel);

    WeaveLogProgress(DeviceLayer,
                     "Weave Tunnel Time Stamps\n"
                     "LastTime TunnelWentDown:       %" PRIu64 "\n"
                     "LastTime TunnelEstablished:    %" PRIu64 "\n",
                     statsEvent.lastTimeTunnelWentDown, statsEvent.lastTimeTunnelEstablished);

    eventId = nl::LogEvent(&statsEvent);
    WeaveLogProgress(DeviceLayer, "Weave Tunnel Tolopoly Stats Event Id: %u\n", eventId);

    return;
}
#endif // WEAVE_DEVICE_CONFIG_ENABLE_TUNNEL_TELEMETRY

#endif // WEAVE_DEVICE_CONFIG_ENABLE_NETWORK_TELEMETRY

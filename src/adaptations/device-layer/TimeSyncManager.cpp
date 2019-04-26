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
 *      Implementation for the Weave Device Layer TimeSyncManager object.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/ServiceDirectoryManager.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/TimeUtils.h>

#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Time;
using namespace ::nl::Weave::DeviceLayer::Internal;

#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC && !WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#error "CONFIG ERROR: WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC requires WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY to be enabled"
#endif

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace {

#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
SingleSourceTimeSyncClient TimeSyncClient;
#endif

} // unnamed namespace


TimeSyncManager TimeSyncManager::sInstance;

WEAVE_ERROR TimeSyncManager::SetMode(TimeSyncMode newMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(newMode > kTimeSyncMode_NotSupported && newMode < kTimeSyncMode_Max, err = WEAVE_ERROR_INVALID_ARGUMENT);

#if !WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC && !WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
    VerifyOrExit(newMode != kTimeSyncMode_Service, err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);
#endif

    mMode = newMode;

    DriveTimeSync();

exit:
    return err;
}

void TimeSyncManager::SetSyncInterval(uint32_t intervalSec)
{
    mSyncIntervalSec = intervalSec;
    DriveTimeSync();
}

bool TimeSyncManager::IsTimeSynchronized()
{
    uint64_t t;
    return System::Layer::GetClock_RealTime(t) == WEAVE_NO_ERROR;
}

WEAVE_ERROR TimeSyncManager::Init()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
    mServiceDirTimeSyncStartUS = 0;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC

#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
    new (&TimeSyncClient) SingleSourceTimeSyncClient();

    err = TimeSyncClient.Init(NULL, &ExchangeMgr);
    SuccessOrExit(err);

    mLastSyncTimeMS = 0;
    mSyncIntervalSec = WEAVE_DEVICE_CONFIG_DEFAULT_TIME_SYNC_INTERVAL;
    mTimeSyncBinding = NULL;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC || WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
    mMode = kTimeSyncMode_Service;
#else
    mMode = kTimeSyncMode_Disabled;
#endif

    ExitNow(err = WEAVE_NO_ERROR); // Avoids warning about unused label

exit:
    return err;
}

void TimeSyncManager::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    if (event->Type == DeviceEventType::kServiceProvisioningChange ||
        (event->Type == DeviceEventType::kServiceConnectivityChange &&
         event->ServiceConnectivityChange.Result != kConnectivity_NoChange))
    {
        sInstance.DriveTimeSync();
    }
}

#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC

void TimeSyncManager::MarkServiceDirRequestStart()
{
    // Mark the local start time of the directory query request using the hi-res clock.
    sInstance.mServiceDirTimeSyncStartUS = System::Layer::GetClock_MonotonicHiRes();
}

void TimeSyncManager::ProcessServiceDirTimeData(uint64_t serverRealTimeMS, uint32_t serverProcessingTimeMS)
{
    // If synchronizing time with the service, and a service directory time sync is in progress...
    if (sInstance.mMode == kTimeSyncMode_Service && sInstance.mServiceDirTimeSyncStartUS != 0)
    {
        // Mark the end time of the request using the hi-res clock.
        uint64_t timeSyncEndUS = System::Layer::GetClock_MonotonicHiRes();

        WeaveLogProgress(DeviceLayer, "Time sync with service directory complete");

        // Use the information from the directory server response to compute an approximation of
        // the current real time.
        uint64_t twoWayTripTimeUS = timeSyncEndUS - sInstance.mServiceDirTimeSyncStartUS - (serverProcessingTimeMS * 1000);
        uint64_t avgOneWayTripTimeUS = twoWayTripTimeUS >> 1;
        uint64_t syncedRealTimeUS = (serverRealTimeMS * 1000) + avgOneWayTripTimeUS;

        // Update the system's real-time clock with the synchronized value.
        sInstance.ApplySynchronizedTime(syncedRealTimeUS);

        // If Weave time server synchronization is also enabled, restart the time server sync
        // interval from this point.
        sInstance.DriveTimeSync();
    }
}

#endif // WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC

void TimeSyncManager::DriveTimeSync()
{
    WEAVE_ERROR err;

#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

    // If synchronizing time with the service...
    //    AND the system has been service provisioned...
    //    AND the system has service connectivity...
    if (mMode == kTimeSyncMode_Service &&
        ConfigurationMgr().IsServiceProvisioned() &&
        ConnectivityMgr().HaveServiceConnectivity())
    {
        // Compute the amount of time until the next synchronization event.
        uint64_t timeToNextSyncMS = 0;
        if (mLastSyncTimeMS != 0)
        {
            uint64_t nextSyncTimeMS = mLastSyncTimeMS + (mSyncIntervalSec * 1000);
            uint64_t nowMS = System::Layer::GetClock_MonotonicMS();
            if (nowMS < nextSyncTimeMS)
            {
                timeToNextSyncMS = nextSyncTimeMS - nowMS;
            }
        }

        // If synchronization should happen now...
        if (timeToNextSyncMS == 0)
        {
            // Make sure there's no time sync in progress.
            CancelTimeSync();

            WeaveLogProgress(DeviceLayer, "Starting time sync with Weave time server");

            // Create and prepare a binding for talking to the time server endpoint.
            // This will result in a callback to HandleTimeServerSyncBindingEvent when
            // the binding is ready to be used.
            mTimeSyncBinding = ExchangeMgr.NewBinding(TimeServiceSync_HandleBindingEvent, NULL);
            VerifyOrExit(mTimeSyncBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);
            err = mTimeSyncBinding->BeginConfiguration()
                    .Target_ServiceEndpoint(WEAVE_DEVICE_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID)
                    .Transport_UDP_WRM()
                    .Exchange_ResponseTimeoutMsec(WEAVE_DEVICE_CONFIG_TIME_SYNC_TIMEOUT)
                    .Security_SharedCASESession()
                    .PrepareBinding();
            SuccessOrExit(err);
        }

        // Otherwise initiate synchronization after an appropriate delay.
        else
        {
            SystemLayer.StartTimer((uint32_t)timeToNextSyncMS, DriveTimeSync, NULL);
        }
    }

    // Otherwise stop any time sync that might be in progress and cancel the interval timer.
    else
    {
        CancelTimeSync();
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

    ExitNow(err = WEAVE_NO_ERROR); // suppress warning about unused label.

exit:
    if (err != WEAVE_NO_ERROR)
    {
        sInstance.TimeSyncFailed(err, NULL);
    }
}

void TimeSyncManager::CancelTimeSync()
{
#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
    sInstance.mServiceDirTimeSyncStartUS = 0;
#endif

#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
    SystemLayer.CancelTimer(DriveTimeSync, NULL);
    TimeSyncClient.Abort();
    if (mTimeSyncBinding != NULL)
    {
        mTimeSyncBinding->Close();
        mTimeSyncBinding = NULL;
    }
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
}

void TimeSyncManager::ApplySynchronizedTime(uint64_t syncedRealTimeUS)
{
    WEAVE_ERROR err;

    // Only change the system clock if the final time value is valid...
    if (syncedRealTimeUS > (((uint64_t)WEAVE_SYSTEM_CONFIG_VALID_REAL_TIME_THRESHOLD) * kMicrosecondsPerSecond))
    {
        bool wasSynchronized = sInstance.IsTimeSynchronized();

        // Attempt to set the system's real time clock.  If successful...
        err = System::Layer::SetClock_RealTime(syncedRealTimeUS);
        if (err == WEAVE_NO_ERROR)
        {
            // If this is the first point at which time is synchronized, post a Time Sync change event.
            if (!wasSynchronized)
            {
                WeaveDeviceEvent event;
                event.Type = DeviceEventType::kTimeSyncChange;
                event.TimeSyncChange.IsTimeSynchronized = true;
                PlatformMgr().PostEvent(&event);
            }
        }
        else
        {
            WeaveLogError(DeviceLayer, "SetClock_RealTime() failed: %s", ErrorStr(err));
        }
    }

    // Update the time from which the next sync interval should be counted.
    sInstance.mLastSyncTimeMS = System::Layer::GetClock_MonotonicMS();
}

void TimeSyncManager::TimeSyncFailed(WEAVE_ERROR reason, Profiles::StatusReporting::StatusReport * statusReport)
{
    WeaveLogError(DeviceLayer, "Time sync failed: %s",
             (reason == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
             ? StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode)
             : ErrorStr(reason));

    // Update the time from which the next sync interval should be counted.
    sInstance.mLastSyncTimeMS = System::Layer::GetClock_MonotonicMS();

    // Arrange to try again at the next synchronization interval.
    sInstance.DriveTimeSync();
}

void TimeSyncManager::DriveTimeSync(System::Layer * /* unused */, void * /* unused */, System::Error /* unused */)
{
    sInstance.DriveTimeSync();
}

#if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

void TimeSyncManager::TimeServiceSync_HandleBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
            const ::nl::Weave::Binding::InEventParam & inParam, ::nl::Weave::Binding::OutEventParam & outParam)
{
    WEAVE_ERROR err;
    Binding * binding = inParam.Source;

    // If the binding is ready, send a Time Sync request to the timer server.
    if (eventType == Binding::kEvent_BindingReady)
    {
        err = TimeSyncClient.Sync(binding, TimeServiceSync_HandleSyncComplete);
        if (err != WEAVE_NO_ERROR)
        {
            sInstance.TimeSyncFailed(err, NULL);
        }
    }

    // Otherwise handle any failure that occurred during binding preparation.
    else if (eventType == Binding::kEvent_PrepareFailed)
    {
        sInstance.TimeSyncFailed(inParam.PrepareFailed.Reason, inParam.PrepareFailed.StatusReport);
    }

    // Pass all other events to the default handler.
    else
    {
        binding->DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

void TimeSyncManager::TimeServiceSync_HandleSyncComplete(void * context, WEAVE_ERROR result, int64_t syncedRealTimeUS)
{
    if (result == WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceLayer, "Time sync with time service complete");
        sInstance.ApplySynchronizedTime((uint64_t)syncedRealTimeUS);
        sInstance.DriveTimeSync();
    }
    else
    {
        sInstance.TimeSyncFailed(result, NULL);
    }
}

#endif // WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file contains implementation of the TimeSyncCoordinator class used in Time Services
 *      WEAVE_CONFIG_TIME must be defined if Time Services are needed
 *
 */

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

using namespace nl::Weave::Profiles::Time;
using namespace nl::Inet;

WEAVE_ERROR TimeSyncNode::InitCoordinator(nl::Weave::WeaveExchangeManager *aExchangeMgr, const uint8_t aEncryptionType,
    const uint16_t aKeyId, const int32_t aSyncPeriod_msec
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    ,
    const int32_t aNominalDiscoveryPeriod_msec,
    const int32_t aShortestDiscoveryPeriod_msec
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // initialize general data
    err = InitState(kTimeSyncRole_Coordinator, this, aExchangeMgr);
    SuccessOrExit(err);

    // initialize Server-specific data
    err = _InitServer(false);
    SuccessOrExit(err);
    // declare our existence. note we're using the same encryption and key id as the client
    MulticastTimeChangeNotification(mEncryptionType, mKeyId);

    // initialize Client-specific data
    err = _InitClient(aEncryptionType, aKeyId
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        , TimeSyncRequest::kLikelihoodForResponse_Min
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );
    SuccessOrExit(err);
    OnSyncSucceeded = TimeSyncNode::_OnSyncSucceeded;

    err = EnableAutoSync(aSyncPeriod_msec
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        , aNominalDiscoveryPeriod_msec, aShortestDiscoveryPeriod_msec
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncNode::_ShutdownCoordinator(void)
{
    (void)_ShutdownServer();
    (void)_ShutdownClient();

    return WEAVE_NO_ERROR;
}

bool TimeSyncNode::_OnSyncSucceeded(void * const aApp, const timesync_t aOffsetUsec, const bool aIsReliable,
    const bool aIsServer, const uint8_t aNumContributor)
{
    bool shouldUpdate = true;
    TimeSyncNode * const coordinator = reinterpret_cast<TimeSyncNode *>(aApp);

    coordinator->RegisterLocalSyncOperation(aNumContributor);

    // register that we've just received correction from external reliable source
    if (aIsServer)
    {
        coordinator->RegisterCorrectionFromServerOrNtp();

        if ((WEAVE_CONFIG_TIME_CLIENT_MIN_OFFSET_FROM_SERVER_USEC > aOffsetUsec)
            && (-WEAVE_CONFIG_TIME_CLIENT_MIN_OFFSET_FROM_SERVER_USEC < aOffsetUsec))
        {
            // the change is pretty small
            // ignore small changes from server
            shouldUpdate = false;
        }
    }

    if (shouldUpdate)
    {
        // declare that we're about to significantly change our clock
        if ((aOffsetUsec > WEAVE_CONFIG_TIME_COORDINATOR_THRESHOLD_TO_SEND_NOTIFICATION_USEC)
            || (aOffsetUsec < -WEAVE_CONFIG_TIME_COORDINATOR_THRESHOLD_TO_SEND_NOTIFICATION_USEC))
        {
            coordinator->MulticastTimeChangeNotification(coordinator->mEncryptionType,
                coordinator->mKeyId);
        }
    }

    return shouldUpdate;
}

#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
#endif // WEAVE_CONFIG_TIME

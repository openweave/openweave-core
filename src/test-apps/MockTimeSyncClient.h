/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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

#ifndef MOCKTIMESYNCCLIENT_H_
#define MOCKTIMESYNCCLIENT_H_

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT

#include <Weave/Profiles/time/WeaveTime.h>

using namespace nl::Weave;

class MockTimeSyncClient
{
public:
    MockTimeSyncClient();

    WEAVE_ERROR Init(
        nl::Weave::WeaveExchangeManager *exchangeMgr,
        OperatingMode mode,
        uint64_t serviceNodeId = kAnyNodeId,
        const char * serviceNodeAddr = NULL,
        const uint8_t encryptionType = nl::Weave::kWeaveEncryptionType_None,
        const uint16_t keyId = nl::Weave::WeaveKeyId::kNone);

    WEAVE_ERROR Shutdown(void);

private:
    nl::Weave::Profiles::Time::TimeSyncNode mClient;

    OperatingMode mOperatingMode;

    nl::Weave::Profiles::Time::ServingNode mContacts[7];
    void SetupContacts(void);

    nl::Weave::Profiles::Time::ServingNode mServiceContact;
    void SetupServiceContact(uint64_t serviceNodeId, const char * serviceNodeAddr);

    static void HandleSyncTimer(nl::Weave::System::Layer* aSystemLayer, void* aAppState, nl::Weave::System::Error aError);

    static void OnResponseReadyForCalculation(void * const aApp,
        nl::Weave::Profiles::Time::Contact aContact[], const int aSize);
    static void OnTimeChangeNotificationReceived(void * const aApp, const uint64_t aNodeId,
        const nl::Inet::IPAddress & aNodeAddr);
    static bool OnSyncSucceeded(void * const aApp, const nl::Weave::Profiles::Time::timesync_t aOffsetUsec,
        const bool aIsReliable, const bool aIsServer, const uint8_t aNumContributor);
    static void OnSyncFailed(void * const aApp, const WEAVE_ERROR aErrorCode);

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    nl::Weave::WeaveConnection * mConnectionToService;

    void SetupConnectionToService(void);
    void CloseConnectionToService(void);

    static void HandleConnectionComplete(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleConnectionClosed(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
};

class MockSingleSourceTimeSyncClient
{
public:
    MockSingleSourceTimeSyncClient();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager * const aExchangeMgr, const uint64_t aPublisherNodeId, const uint16_t aSubnetId);
    WEAVE_ERROR Shutdown(void);

private:
    nl::Weave::Profiles::Time::SingleSourceTimeSyncClient mClient;
    nl::Weave::Binding * mBinding;
    nl::Weave::WeaveExchangeManager * mExchangeMgr;

    static void BindingEventCallback (void * const apAppState,
            const nl::Weave::Binding::EventType aEvent,
            const nl::Weave::Binding::InEventParam & aInParam,
            nl::Weave::Binding::OutEventParam & aOutParam);

    static void HandleSyncTimer(nl::Weave::System::Layer* aSystemLayer, void* aAppState, nl::Weave::System::Error aError);

    static void HandleTimeChangeNotificationReceived(void * const aApp, nl::Weave::ExchangeContext * aEC);
    static void HandleSyncCompleted(void * const aApp, const WEAVE_ERROR aErrorCode, const nl::Weave::Profiles::Time::timesync_t aCorrectedSystemTime);
};

#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
#endif /* MOCKTIMESYNCCLIENT_H_ */

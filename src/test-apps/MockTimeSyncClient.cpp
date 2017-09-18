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

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "MockTimeSyncUtil.h"
#include "MockTimeSyncClient.h"

#if WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_TIME_ENABLE_CLIENT

using namespace nl::Weave::Platform::Time;
using namespace nl::Weave::Profiles::Time;
using namespace nl::Inet;
using nl::Weave::Binding;

const uint32_t RESPONSE_TIMEOUT_MSEC = 5000;

MockSingleSourceTimeSyncClient::MockSingleSourceTimeSyncClient(void)
{
    mBinding = NULL;
    mExchangeMgr = NULL;
}

void MockSingleSourceTimeSyncClient::BindingEventCallback (void * const apAppState, const Binding::EventType aEvent,
        const Binding::InEventParam & aInParam, Binding::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(TimeService, "%s: Event(%d)", __func__, aEvent);

    MockSingleSourceTimeSyncClient * const mockClient = reinterpret_cast<MockSingleSourceTimeSyncClient *>(apAppState);

    switch (aEvent)
    {
    case Binding::kEvent_BindingReady  :

        WeaveLogDetail(TimeService, "Arming sync timer", __func__, aEvent);

        mockClient->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(RESPONSE_TIMEOUT_MSEC * 2 + 1000, HandleSyncTimer, mockClient);

        err = mockClient->mClient.Sync(mockClient->mBinding, HandleSyncCompleted);
        SuccessOrExit(err);
        break;

    default:
        Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }

exit:
    WeaveLogFunctError(err);
}

WEAVE_ERROR MockSingleSourceTimeSyncClient::Init(nl::Weave::WeaveExchangeManager * const aExchangeMgr, const uint64_t aPublisherNodeId, const uint16_t aSubnetId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mExchangeMgr = aExchangeMgr;

    mClient.Init(this, mExchangeMgr);
    mClient.OnTimeChangeNotificationReceived = HandleTimeChangeNotificationReceived;

    mBinding = mExchangeMgr->NewBinding(BindingEventCallback, this);
    VerifyOrExit(NULL != mBinding, err = WEAVE_ERROR_NO_MEMORY);

    {
        Binding::Configuration bindingConfig = mBinding->BeginConfiguration()
            .Target_NodeId(aPublisherNodeId)
            .Transport_UDP()
            // (default) max num of msec between any outgoing message and next incoming message (could be a response to it)
            .Exchange_ResponseTimeoutMsec(RESPONSE_TIMEOUT_MSEC)
            .Security_None();

        if (nl::Weave::kWeaveSubnetId_NotSpecified != aSubnetId)
            bindingConfig.TargetAddress_WeaveFabric(aSubnetId);

        err = bindingConfig.PrepareBinding();
        SuccessOrExit(err);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR MockSingleSourceTimeSyncClient::Shutdown(void)
{
    mBinding->Release();
    mBinding = NULL;

    return WEAVE_NO_ERROR;
}

void MockSingleSourceTimeSyncClient::HandleSyncTimer(nl::Weave::System::Layer* aSystemLayer, void* aAppState, nl::Weave::System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    MockSingleSourceTimeSyncClient * const mockClient = reinterpret_cast<MockSingleSourceTimeSyncClient *>(aAppState);

    WeaveLogProgress(TimeService, "--------------- Sync Timer -----------------------------");

    err = mockClient->mClient.Sync(mockClient->mBinding, HandleSyncCompleted);
    WeaveLogFunctError(err);

    err = mockClient->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(RESPONSE_TIMEOUT_MSEC * 2 + 1000, HandleSyncTimer, mockClient);
    WeaveLogFunctError(err);
}

void MockSingleSourceTimeSyncClient::HandleTimeChangeNotificationReceived(void * const aApp, nl::Weave::ExchangeContext * aEC)
{
    MockSingleSourceTimeSyncClient * const mockClient = reinterpret_cast<MockSingleSourceTimeSyncClient *>(aApp);

    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    WeaveLogProgress(TimeService, "++++  OnTimeChangeNotificationReceived  ++++");
    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");

    (void)mockClient->mClient.Sync(mockClient->mBinding, HandleSyncCompleted);
}

void MockSingleSourceTimeSyncClient::HandleSyncCompleted(void * const aApp, const WEAVE_ERROR aErrorCode, const nl::Weave::Profiles::Time::timesync_t aCorrectedSystemTime)
{
    if ((WEAVE_NO_ERROR == aErrorCode) && (TIMESYNC_INVALID != aCorrectedSystemTime))
    {
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
        WeaveLogProgress(TimeService, "++++           Sync Succeeded           ++++");
        WeaveLogProgress(TimeService, "++++   Corrected time: %" PRId64 " usec", aCorrectedSystemTime);
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");

        (void)nl::Weave::Platform::Time::SetSystemTime(aCorrectedSystemTime);
    }
    else
    {
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
        WeaveLogProgress(TimeService, "++++   Sync Completed with no results   ++++");
        WeaveLogProgress(TimeService, "++++   Error code: %d", aErrorCode);
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    }
}

MockTimeSyncClient::MockTimeSyncClient()
{
    memset(mContacts, 0, sizeof(mContacts));
}

void MockTimeSyncClient::SetupContacts(void)
{
    // contacts for manually run
    /*
    mContacts[0].mNodeId = 4;
    IPAddress::FromString("fd00:0:1:1::4", mContacts[0].mNodeAddr);

    mContacts[1].mNodeId = 3;
    IPAddress::FromString("fd00:0:1:1::3", mContacts[1].mNodeAddr);

    mContacts[2].mNodeId = 2;
    IPAddress::FromString("fd00:0:1:1::2", mContacts[2].mNodeAddr);

    mContacts[3].mNodeId = 5;
    IPAddress::FromString("fd00:0:1:1::5", mContacts[3].mNodeAddr);

    mContacts[4].mNodeId = 6;
    IPAddress::FromString("fd00:0:1:1::6", mContacts[4].mNodeAddr);

    mContacts[5].mNodeId = 7;
    IPAddress::FromString("fd00:0:1:1::7", mContacts[5].mNodeAddr);

    mContacts[6].mNodeId = 8;
    IPAddress::FromString("fd00:0:1:1::8", mContacts[6].mNodeAddr);
    */

    // contacts for automatic run with Happy
    /*
    mContacts[0].mNodeId = 0xFFFE000003;
    IPAddress::FromString("fd00:0:fab1:6:200:ff:fe00:3", mContacts[0].mNodeAddr);

    mContacts[1].mNodeId = 0xFFFE000002;
    IPAddress::FromString("fd00:0:fab1:6:200:ff:fe00:2", mContacts[1].mNodeAddr);
    */

    /* server node (node03 in three_nodes_on_thread_weave.json) */
    mContacts[0].mNodeId = 0x18B430000000000A;
    IPAddress::FromString("fd00:0000:fab1:0006:1ab4:3000:0000:000A", mContacts[0].mNodeAddr);

    /* client node (node01 in three_nodes_on_thread_weave.json) */
    mContacts[1].mNodeId = 0x18B4300000000004;
    IPAddress::FromString("fd00:0000:fab1:0006:1ab4:3000:0000:0004", mContacts[1].mNodeAddr);
}

void MockTimeSyncClient::SetupServiceContact(uint64_t serviceNodeId, const char * serviceNodeAddr)
{
    mServiceContact.mNodeId = serviceNodeId;
    IPAddress::FromString(serviceNodeAddr, mServiceContact.mNodeAddr);
}

WEAVE_ERROR MockTimeSyncClient::Init(
    nl::Weave::WeaveExchangeManager *exchangeMgr,
    OperatingMode mode,
    uint64_t serviceNodeId,
    const char * serviceNodeAddr,
    const uint8_t encryptionType,
    const uint16_t keyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    if (mode == kOperatingMode_ServiceOverTunnel) {
        if ((serviceNodeId == kAnyNodeId) || (serviceNodeAddr == NULL)) {
            printf("--ts-server-node-id and --ts-server-node-addr are MANDATORY when using --time-sync-mode-service-over-tunnel\n");
            ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        }
        SetupServiceContact(serviceNodeId, serviceNodeAddr);
    }
#endif

    SetupContacts();

    err = mClient.InitClient(this, exchangeMgr, encryptionType, keyId);
    SuccessOrExit(err);

    mOperatingMode = mode;
    mClient.OnTimeChangeNotificationReceived = OnTimeChangeNotificationReceived;
    mClient.OnSyncSucceeded = OnSyncSucceeded;
    mClient.OnSyncFailed = OnSyncFailed;
    mClient.FilterTimeCorrectionContributor = OnResponseReadyForCalculation;

    WeaveLogProgress(TimeService, "--------------------------------------------");

    switch (mOperatingMode)
    {
    case kOperatingMode_Auto:
        // Sync period: 20 seconds
        // Discovery period: 120 seconds
        // Discovery period in the existence of communication error: 10 seconds
        err = mClient.EnableAutoSync(20000
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
            , 120000, 10000
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
            );
        SuccessOrExit(err);
        break;
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    case kOperatingMode_Service:
        // periodically sync to another node using TCP connection
        err = mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(20 * 1000, HandleSyncTimer, this);
        SuccessOrExit(err);
        SetupConnectionToService();
        break;
    case kOperatingMode_ServiceOverTunnel:
        // periodically sync to another node using WRM over a Weave Tunnel
        err = mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(20 * 1000, HandleSyncTimer, this);
        SuccessOrExit(err);
        err = mClient.SyncWithNodes(1, &mServiceContact);
        SuccessOrExit(err);
        break;
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    case kOperatingMode_AssignedLocalNodes:
        // periodically sync with local nodes using UDP connection
        err = mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(20 * 1000, HandleSyncTimer, this);
        SuccessOrExit(err);
        err = mClient.SyncWithNodes(1, mContacts);
        SuccessOrExit(err);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        break;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR MockTimeSyncClient::Shutdown(void)
{
    // this function doesn't have error result
    mClient.GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleSyncTimer, &mClient);

    return mClient.Shutdown();
}

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
void MockTimeSyncClient::HandleConnectionComplete(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr)
{
    MockTimeSyncClient * const mockClient = reinterpret_cast<MockTimeSyncClient *>(con->AppState);
    WeaveLogProgress(TimeService, "Connection to service completed");
    mockClient->mClient.SyncWithService(con);
}

void MockTimeSyncClient::HandleConnectionClosed(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr)
{
    MockTimeSyncClient * const mockClient = reinterpret_cast<MockTimeSyncClient *>(con->AppState);
    WeaveLogProgress(TimeService, "Connection to service closed");
    mockClient->mConnectionToService = NULL;
}

void MockTimeSyncClient::CloseConnectionToService(void)
{
    if (NULL != mConnectionToService)
    {
        WeaveLogProgress(TimeService, "App closing connection to service");

        mConnectionToService->Close();
        mConnectionToService = NULL;
    }
}

void MockTimeSyncClient::SetupConnectionToService(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    CloseConnectionToService();

    mConnectionToService = nl::Weave::MessageLayer.NewConnection();
    if (NULL == mConnectionToService)
    {
        WeaveLogError(TimeService, "Cannot acquire new connection object");
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }
    mConnectionToService->AppState = this;

    err = mConnectionToService->Connect(mContacts[0].mNodeId, mContacts[0].mNodeAddr);
    SuccessOrExit(err);

    mConnectionToService->OnConnectionClosed = HandleConnectionClosed;
    mConnectionToService->OnConnectionComplete = HandleConnectionComplete;

exit:
    WeaveLogFunctError(err);

    return;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

void MockTimeSyncClient::HandleSyncTimer(nl::Weave::System::Layer* aSystemLayer, void* aAppState, nl::Weave::System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    MockTimeSyncClient * const client = reinterpret_cast<MockTimeSyncClient *>(aAppState);

    WeaveLogProgress(TimeService, "--------------------------------------------");

    err = client->mClient.Abort();
    SuccessOrExit(err);

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    client->CloseConnectionToService();
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    switch (client->mOperatingMode)
    {

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    case kOperatingMode_Service:
        err = client->mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(15000,
            HandleSyncTimer, &client->mClient);
        SuccessOrExit(err);
        client->SetupConnectionToService();
        break;
    case kOperatingMode_ServiceOverTunnel:
        err = client->mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(15000,
            HandleSyncTimer, &client->mClient);
        SuccessOrExit(err);
        err = client->mClient.SyncWithNodes(1, &(client->mServiceContact));
        SuccessOrExit(err);
        break;
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    case kOperatingMode_AssignedLocalNodes:
        err = client->mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(30000,
            HandleSyncTimer, &client->mClient);
        SuccessOrExit(err);
        err = client->mClient.SyncWithNodes(1, client->mContacts);
        SuccessOrExit(err);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        break;
    }

exit:
    WeaveLogFunctError(err);

    return;
}

void MockTimeSyncClient::OnTimeChangeNotificationReceived(void * const aApp, const uint64_t aNodeId,
    const IPAddress & aNodeAddr)
{
    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    WeaveLogProgress(TimeService, "++++ OnTiomeChangeNotificationReceived  ++++");
    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");

    // stop whatever we're doing and begin sync
    // this is just a demo of what can be done inside this callback
    // it is still recommended to start a new sync from a clean stack
    MockTimeSyncClient * const client = reinterpret_cast<MockTimeSyncClient *>(aApp);
    if (kOperatingMode_AssignedLocalNodes == client->mOperatingMode)
    {
        WeaveLogProgress(TimeService, "Leave whatever we're doing and sync again");

        client->mClient.Abort();

        // cancel existing timer
        client->mClient.GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(
            HandleSyncTimer, &client->mClient);

        // start a new timer
        client->mClient.GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(30000,
                    HandleSyncTimer, &client->mClient);

        // sync based on known contacts
        // note the originator of this notification would natually be used!
        client->mClient.Sync();
    }
}

bool MockTimeSyncClient::OnSyncSucceeded(void * const aApp, const timesync_t aOffsetUsec, const bool aIsReliable,
    const bool aIsServer, const uint8_t aNumContributor)
{
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    MockTimeSyncClient * const mockClient = reinterpret_cast<MockTimeSyncClient *>(aApp);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    if (aNumContributor > 0)
    {
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
        WeaveLogProgress(TimeService, "++++           Sync Succeeded           ++++");
        WeaveLogProgress(TimeService, "++++ Reliable: %c, # Contributors: %2d    ++++", aIsReliable ? 'Y' : 'N',
            aNumContributor);
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    }
    else
    {
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
        WeaveLogProgress(TimeService, "++++   Sync Completed with no results   ++++");
        WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    }

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    mockClient->CloseConnectionToService();
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    return true;
}

void MockTimeSyncClient::OnSyncFailed(void * const aApp, const WEAVE_ERROR aErrorCode)
{
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    MockTimeSyncClient * const mockClient = reinterpret_cast<MockTimeSyncClient *>(aApp);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    WeaveLogProgress(TimeService, "/////////////////////////////////////////////////////////////////");
    WeaveLogProgress(TimeService, "////                         Sync Failed                     ////");
    WeaveLogProgress(TimeService, "/////////////////////////////////////////////////////////////////");

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    mockClient->CloseConnectionToService();
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
}

void MockTimeSyncClient::OnResponseReadyForCalculation(void * const aApp,
    Contact aContact[], const int aSize)
{
    timesync_t unadjTimestamp_usec;

    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");
    WeaveLogProgress(TimeService, "++++           Capacity: %3d            ++++", aSize);
    WeaveLogProgress(TimeService, "++++++++++++++++++++++++++++++++++++++++++++");

    (void)GetMonotonicRawTime(&unadjTimestamp_usec);

    for (int i = 0; i < aSize; ++i)
    {
        if ((aContact[i].mCommState == uint8_t(TimeSyncNode::kCommState_Completed)) &&
            (aContact[i].mResponseStatus != uint8_t(TimeSyncNode::kResponseStatus_Invalid)))
        {
            const timesync_t correctedRemoteSystemTime_usec = (aContact[i].mRemoteTimestamp_usec
                + aContact[i].mFlightTime_usec)
                + (unadjTimestamp_usec - aContact[i].mUnadjTimestampLastContact_usec);

            WeaveLogDetail(TimeService, "Node %llu Role:%d corrected system time:%f",
                static_cast<unsigned long long>(aContact[i].mNodeId),
                aContact[i].mRole,
                correctedRemoteSystemTime_usec * 1e-6);
        }
    }
}

#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
#endif // WEAVE_CONFIG_TIME

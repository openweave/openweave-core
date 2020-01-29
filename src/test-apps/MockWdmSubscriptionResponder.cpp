/*
 *
 *    Copyright (c) 2019 Google, LLC.
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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
 *      This file implements the Weave Data Management mock subscription responder.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS
#include <inttypes.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Profiles/security/ApplicationKeysTraitDataSink.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/ASN1.h>
#include "MockWdmTestVerifier.h"
#include "MockWdmSubscriptionResponder.h"
#include "MockSinkTraits.h"
#include "MockSourceTraits.h"
#include "MockWdmTestVerifier.h"

#include "MockPlatformClocks.h"

using nl::Weave::System::PacketBuffer;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Weave::Trait::Auth::ApplicationKeysTrait;

const nl::Weave::ExchangeContext::Timeout kResponseTimeoutMsec = 15000;
const nl::Weave::ExchangeContext::Timeout kWRMPActiveRetransTimeoutMsec = 3000;
const nl::Weave::ExchangeContext::Timeout kWRMPInitialRetransTimeoutMsec = 3000;
const uint16_t kWRMPMaxRetrans = 3;
const uint16_t kWRMPAckTimeoutMsec = 200;
const nl::Weave::Profiles::Time::timesync_t kCommandTimeoutMicroSecs = 30*nl::kMicrosecondsPerSecond;

static int gNumDataChangeBeforeCancellation;
static int gFinalStatus;
static SubscriptionHandler *gSubscriptionHandler = NULL;
static int gTimeBetweenDataChangeMsec = 0;
static bool gEnableDataFlip = true;
static nl::Weave::Binding * gBinding = NULL;
static bool gClearDataSink = false;
static bool gCleanStatus = true;
static nl::Weave::WRMPConfig gWRMPConfig = { kWRMPInitialRetransTimeoutMsec, kWRMPActiveRetransTimeoutMsec, kWRMPAckTimeoutMsec, kWRMPMaxRetrans };


static void TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    // There is no proper Weave logging routine for us to use here
    vprintf(aFormat, args);

    va_end(args);
}

static WEAVE_ERROR DebugPrettyPrint(nl::Weave::TLV::TLVReader & aReader)
{
    return nl::Weave::TLV::Debug::Dump(aReader, TLVPrettyPrinter);
}

nl::Weave::Profiles::DataManagement::SubscriptionEngine * nl::Weave::Profiles::DataManagement::SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine gWdmSubscriptionEngine;

    return &gWdmSubscriptionEngine;
}

struct VersionNode
{
    uint64_t versionInfo;
    VersionNode * next;
};


class WdmResponderState
{
public:
    int mDataflipCount;
    int mClientStateCount;
    int mPublisherStateCount;
    void init(void)
    {
        mDataflipCount = 1;
        mClientStateCount = 1;
        mPublisherStateCount = 1;
    }
};

static WdmResponderState gResponderState;


class MockWdmSubscriptionResponderImpl: public MockWdmSubscriptionResponder
{
public:

    MockWdmSubscriptionResponderImpl ();

    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr,
                              const MockWdmNodeOptions &aConfig);
    void PrintVersionsLog();
    virtual void ClearDataSinkState(void);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    static void ClearDataSinkIterator(void *aTraitInstance, TraitDataHandle aHandle, void *aContext);

    bool mIsMutualSubscription;
    int mTestCaseId;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    MockWdmNodeOptions::WdmUpdateTiming mUpdateTiming;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
    // publisher side
    uint32_t mTimeBetweenLivenessCheckSec;
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[10];

    // source traits
    LocaleSettingsTraitDataSource mLocaleSettingsDataSource;
    LocaleCapabilitiesTraitDataSource mLocaleCapabilitiesDataSource;
    TestATraitDataSource mTestADataSource0;
    TestATraitDataSource mTestADataSource1;
    TestBTraitDataSource mTestBDataSource;
    TestBLargeTraitDataSource mTestBLargeDataSource;
    BoltLockSettingTraitDataSource mBoltLockSettingDataSource;
    ApplicationKeysTraitDataSource mApplicationKeysTraitDataSource;
    TestCTraitDataSource mTestCDataSource;

    static void EngineEventCallback (void * const aAppState, SubscriptionEngine::EventID aEvent,
        const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam);

    static void PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam, SubscriptionHandler::OutEventParam & aOutParam);

    // client side
    TestATraitDataSink mTestADataSink0;
    TestATraitDataSink mTestADataSink1;
    TestBTraitDataSink mTestBDataSink;
    LocaleCapabilitiesTraitDataSink mLocaleCapabilitiesDataSink;
    SingleResourceSinkTraitCatalog mSinkCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[5];
    nl::Weave::Profiles::DataManagement_Current::TraitSchemaEngine::IGetDataDelegate* mSinkAddressList[4];

    enum
    {
        kTestADataSink0Index = 0,
        kTestADataSink1Index,
        kTestBDataSinkIndex,
        kLocaleCapabilitiesTraitSinkIndex,
        kLocaleSettingsTraitSinkIndex,

        kTestATraitSource0Index,
        kTestATraitSource1Index,
        kTestBTraitSourceIndex,
        kTestBLargeTraitSourceIndex,
        kLocaleSettingsTraitSourceIndex,
        kBoltLockSettingTraitSourceIndex,
        kApplicationKeysTraitSourceIndex,
        kTestCTraitSourceIndex,
        kLocaleCapabilitiesTraitSourceIndex,

        kNumTraitHandles,
    };

    enum
    {
        kClientCancel = 0,
        kPublisherCancel,
        kClientAbort,
        kPublisherAbort,
        kIdle,
    };

    TraitDataHandle mTraitHandleSet[kNumTraitHandles];

    enum
    {
        // subscribe LocaleSettings, TestA(two instances) and TestB traits in initiator
        // publish TestA(two instances) and TestB traits in initiator
        kTestCase_TestTrait                         = 1,

        // subscribe Locale Setting, ApplicationKeys traits in initiator
        // publish Locale Capabilities traits in responder
        kTestCase_IntegrationTrait                  = 2,

        //Reject Incoming subscribe request
        kTestCase_RejectIncomingSubscribeRequest    = 3,

        // subscribe oversize TestB, TestA(two instances) traits and LocaleSettings in initiator
        // publish TestA(two instances) and oversize TestB traits in initiator
        kTestCase_TestOversizeTrait1 = 4,

        // subscribe oversize LocaleSettings, TestB, and TestA(two instances) traits in initiator
        // publish TestA(two instances) and oversize TestB traits in initiator
        kTestCase_TestOversizeTrait2 = 5,

        kTestCase_CompatibleVersionedRequest = 6,

        kTestCase_ForwardCompatibleVersionedRequest = 7,

        kTestCase_IncompatibleVersionedRequest = 8,

        kTestCase_IncompatibleVersionedCommandRequest = 9,

        kTestCase_TestUpdatableTrait = 10,
    };

    enum
    {
        kMonitorCurrentStateCnt = 160,
        kMonitorCurrentStateInterval = 120 //msec
    };

    TraitPath mTraitPaths[4];
    VersionedTraitPath mVersionedTraitPaths[4];
    uint32_t mNumPaths;

    VersionNode mTraitVersionSet[kNumTraitHandles];

    SubscriptionClient * mSubscriptionClient;

    void AddNewVersion (int aTraitDataSinkIndex);

    void DumpClientTraitChecksum(int TraitDataSinkIndex);
    void DumpClientTraits(void);

    void DumpPublisherTraitChecksum(int TraitDataSounceIndex);
    void DumpPublisherTraits(void);

    static void ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
            const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam);

    static void HandleDataFlipTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState, ::nl::Weave::System::Error aErr);

    static void HandleClientComplete(void *aAppState);

    static void HandleClientRelease(void *aAppState);

    static void HandlePublisherComplete();

    static void HandlePublisherRelease();

    static void MonitorPublisherCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr);

    static void MonitorClientCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr);

    enum CustomCommandState
    {
        kCmdState_Idle          = 0,    ///< No active command
        kCmdState_Requesting    = 1,    ///< Command has been sent but we haven't heard anything back
        kCmdState_Operating     = 2,    ///< We have received In-Progress message but are still waiting for response
    };
    CustomCommandState mCmdState;
    nl::Weave::ExchangeContext * mEcCommand;
    void Command_Send (void);
    void Command_End (const bool aAbort = false);
    static void OnMessageReceivedForCustomCommand (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
        const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId,
        uint8_t aMsgType, PacketBuffer *aPayload);
    static void HandleCustomCommandTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState, ::nl::Weave::System::Error aErr);
};

static MockWdmSubscriptionResponderImpl gWdmSubscriptionResponder;

MockWdmSubscriptionResponder * MockWdmSubscriptionResponder::GetInstance ()
{
    return &gWdmSubscriptionResponder;
}

MockWdmSubscriptionResponderImpl::MockWdmSubscriptionResponderImpl() :
    mTimeBetweenLivenessCheckSec(30),
    mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, sizeof(mSourceCatalogStore) / sizeof(mSourceCatalogStore[0])),
    mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0])),
    mCmdState(kCmdState_Idle),
    mEcCommand(NULL)
{
}

WEAVE_ERROR MockWdmSubscriptionResponderImpl::Init (nl::Weave::WeaveExchangeManager *aExchangeMgr,
                                                    const MockWdmNodeOptions &aConfig)
{
    gResponderState.init();
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Test Case ID: %s", (aConfig.mTestCaseId == NULL) ? "NULL" : aConfig.mTestCaseId);

    if (NULL != aConfig.mNumDataChangeBeforeCancellation)
    {
        gNumDataChangeBeforeCancellation = atoi(aConfig.mNumDataChangeBeforeCancellation);
        if (gNumDataChangeBeforeCancellation < -1)
        {
            gNumDataChangeBeforeCancellation = -1;
        }
    }
    else
    {
        gNumDataChangeBeforeCancellation = -1;
    }

    if (NULL != aConfig.mFinalStatus)
    {
        gFinalStatus = atoi(aConfig.mFinalStatus);
    }
    else
    {
        gFinalStatus = 1;
    }

    if (NULL != aConfig.mTestCaseId)
    {
        mTestCaseId = atoi(aConfig.mTestCaseId);
    }
    else
    {
        mTestCaseId = kTestCase_TestTrait;
    }

    if (NULL != aConfig.mTimeBetweenDataChangeMsec)
    {
        gTimeBetweenDataChangeMsec = atoi(aConfig.mTimeBetweenDataChangeMsec);
    }
    else
    {
        gTimeBetweenDataChangeMsec = 15000;
    }

    gEnableDataFlip = aConfig.mEnableDataFlip;

    if (NULL != aConfig.mTimeBetweenLivenessCheckSec)
    {
        mTimeBetweenLivenessCheckSec = atoi(aConfig.mTimeBetweenLivenessCheckSec);
    }
    else
    {
        mTimeBetweenLivenessCheckSec = 30;
    }

    mTestADataSource0.mTraitTestSet = 0;
    mTestADataSource1.mTraitTestSet = 1;

    mSinkCatalog.Add(1, &mTestADataSink0, mTraitHandleSet[kTestADataSink0Index]);
    mSinkCatalog.Add(2, &mTestADataSink1, mTraitHandleSet[kTestADataSink1Index]);
    mSinkCatalog.Add(1, &mTestBDataSink, mTraitHandleSet[kTestBDataSinkIndex]);
    mSinkCatalog.Add(0, &mLocaleCapabilitiesDataSink, mTraitHandleSet[kLocaleCapabilitiesTraitSinkIndex]);

    mSourceCatalog.Add(0, &mTestADataSource0, mTraitHandleSet[kTestATraitSource0Index]);
    mSourceCatalog.Add(1, &mTestADataSource1, mTraitHandleSet[kTestATraitSource1Index]);

    switch (mTestCaseId)
    {
    case kTestCase_TestOversizeTrait1:
    case kTestCase_TestOversizeTrait2:
         mSourceCatalog.Add(0, &mTestBLargeDataSource, mTraitHandleSet[kTestBLargeTraitSourceIndex]);
         break;
    default:
        mSourceCatalog.Add(0, &mTestBDataSource, mTraitHandleSet[kTestBTraitSourceIndex]);
        break;
    }

    mSourceCatalog.Add(0, &mLocaleSettingsDataSource, mTraitHandleSet[kLocaleSettingsTraitSourceIndex]);
    mSourceCatalog.Add(0, &mBoltLockSettingDataSource, mTraitHandleSet[kBoltLockSettingTraitSourceIndex]);
    mSourceCatalog.Add(0, &mApplicationKeysTraitDataSource, mTraitHandleSet[kApplicationKeysTraitSourceIndex]);
    mSourceCatalog.Add(0, &mTestCDataSource, mTraitHandleSet[kTestCTraitSourceIndex]);
    mSourceCatalog.Add(0, &mLocaleCapabilitiesDataSource, mTraitHandleSet[kLocaleCapabilitiesTraitSourceIndex]);

    switch (mTestCaseId)
    {
    case kTestCase_IntegrationTrait:
        WeaveLogDetail(DataManagement, "kTestCase_IntegrationTrait");
        break;

    case kTestCase_RejectIncomingSubscribeRequest:
        WeaveLogDetail(DataManagement, "kTestCase_RejectIncomingSubscribeRequest");
        break;

    case kTestCase_TestTrait:
        WeaveLogDetail(DataManagement, "kTestCase_TestTrait");
        break;

    case kTestCase_TestOversizeTrait1:
    case kTestCase_TestOversizeTrait2:
        WeaveLogDetail(DataManagement, "kTestCase_TestOversizeTrait %d", mTestCaseId);
        break;

    case kTestCase_CompatibleVersionedRequest:
        WeaveLogDetail(DataManagement, "kTestCase_CompatibleVersionedRequest");
        break;

    case kTestCase_ForwardCompatibleVersionedRequest:
        WeaveLogDetail(DataManagement, "kTestCase_ForwardCompatibleVersionedRequest");
        break;

    case kTestCase_IncompatibleVersionedRequest:
        WeaveLogDetail(DataManagement, "kTestCase_IncompatibleVersionedRequest");
        break;

    case kTestCase_IncompatibleVersionedCommandRequest:
        WeaveLogDetail(DataManagement, "kTestCase_IncompatibleVersionedCommandRequest");
        break;

    case kTestCase_TestUpdatableTrait:
        WeaveLogDetail(DataManagement, "kTestCase_TestUpdatableTrait");
        break;

    default:
        WeaveLogDetail(DataManagement, "kTestCase_TestTrait");
        break;
    }

    #if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    mUpdateTiming = aConfig.mWdmUpdateTiming;
    #endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    mIsMutualSubscription = aConfig.mEnableMutualSubscription;

    mSubscriptionClient = NULL;
    mExchangeMgr = aExchangeMgr;

    err = SubscriptionEngine::GetInstance()->Init(mExchangeMgr, this, EngineEventCallback);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    mTraitVersionSet[kTestADataSink0Index].versionInfo = mTestADataSink0.GetVersion();
    mTraitVersionSet[kTestADataSink0Index].next = NULL;
    mTraitVersionSet[kTestADataSink1Index].versionInfo = mTestADataSink1.GetVersion();
    mTraitVersionSet[kTestADataSink1Index].next = NULL;
    mTraitVersionSet[kTestBDataSinkIndex].versionInfo = mTestBDataSink.GetVersion();
    mTraitVersionSet[kTestBDataSinkIndex].next = NULL;
    mTraitVersionSet[kLocaleCapabilitiesTraitSinkIndex].versionInfo = mLocaleCapabilitiesDataSink.GetVersion();
    mTraitVersionSet[kLocaleCapabilitiesTraitSinkIndex].next = NULL;

    mSinkAddressList[kTestADataSink0Index] = &mTestADataSink0;
    mSinkAddressList[kTestADataSink1Index] = &mTestADataSink1;
    mSinkAddressList[kTestBDataSinkIndex] = &mTestBDataSink;
    mSinkAddressList[kLocaleCapabilitiesTraitSinkIndex] = &mLocaleCapabilitiesDataSink;

    Command_End();

exit:
    return err;
}

void MockWdmSubscriptionResponderImpl::DumpPublisherTraitChecksum(int inTraitDataSourceIndex)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSource *dataSource;
    err = mSourceCatalog.Locate(mTraitHandleSet[inTraitDataSourceIndex], &dataSource);
    SuccessOrExit(err);

    ::DumpPublisherTraitChecksum(dataSource);
exit:
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionResponderImpl::DumpClientTraitChecksum(int inTraitDataSinkIndex)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSink *dataSink;
    TraitSchemaEngine::IGetDataDelegate *dataSource;

    dataSource = mSinkAddressList[inTraitDataSinkIndex];
    err = mSinkCatalog.Locate(mTraitHandleSet[inTraitDataSinkIndex], &dataSink);
    SuccessOrExit(err);

    ::DumpClientTraitChecksum(dataSink->GetSchemaEngine(), dataSource);
exit:
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionResponderImpl::DumpClientTraits(void)
{
    switch (mTestCaseId)
    {
        case kTestCase_IntegrationTrait:
            DumpClientTraitChecksum(kLocaleCapabilitiesTraitSinkIndex);
            break;
        case kTestCase_TestTrait:
        case kTestCase_CompatibleVersionedRequest:
        case kTestCase_ForwardCompatibleVersionedRequest:
        case kTestCase_IncompatibleVersionedCommandRequest:
            DumpClientTraitChecksum(kTestADataSink0Index);
            DumpClientTraitChecksum(kTestADataSink1Index);
            DumpClientTraitChecksum(kTestBDataSinkIndex);
            break;
        case kTestCase_TestOversizeTrait1:
        case kTestCase_TestOversizeTrait2:
            DumpClientTraitChecksum(kTestADataSink0Index);
            DumpClientTraitChecksum(kTestADataSink1Index);
            break;
        case kTestCase_TestUpdatableTrait:
            break;
    }
}

void MockWdmSubscriptionResponderImpl::DumpPublisherTraits(void)
{
    switch (mTestCaseId)
    {
        case kTestCase_IntegrationTrait:
            DumpPublisherTraitChecksum(kLocaleSettingsTraitSourceIndex);
            DumpPublisherTraitChecksum(kApplicationKeysTraitSourceIndex);
            break;
        case kTestCase_TestTrait:
        case kTestCase_CompatibleVersionedRequest:
        case kTestCase_ForwardCompatibleVersionedRequest:
        case kTestCase_IncompatibleVersionedCommandRequest:
            DumpPublisherTraitChecksum(kTestATraitSource0Index);
            DumpPublisherTraitChecksum(kTestATraitSource1Index);
            DumpPublisherTraitChecksum(kTestBTraitSourceIndex);
            DumpPublisherTraitChecksum(kLocaleSettingsTraitSourceIndex);
            break;
        case kTestCase_TestOversizeTrait1:
            DumpPublisherTraitChecksum(kTestATraitSource0Index);
            DumpPublisherTraitChecksum(kTestATraitSource1Index);
            DumpPublisherTraitChecksum(kLocaleSettingsTraitSourceIndex);
            break;
        case kTestCase_TestOversizeTrait2:
            DumpPublisherTraitChecksum(kLocaleSettingsTraitSourceIndex);
            DumpPublisherTraitChecksum(kTestATraitSource0Index);
            DumpPublisherTraitChecksum(kTestATraitSource1Index);
            break;
        case kTestCase_TestUpdatableTrait:
            break;
    }
}

void MockWdmSubscriptionResponderImpl::EngineEventCallback (void * const aAppState,
    SubscriptionEngine::EventID aEvent,
    const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    switch (aEvent)
    {
    case SubscriptionEngine::kEvent_OnIncomingSubscribeRequest:
        WeaveLogDetail(DataManagement, "Engine->kEvent_OnIncomingSubscribeRequest peer = 0x%" PRIX64, aInParam.mIncomingSubscribeRequest.mEC->PeerNodeId);
        aOutParam.mIncomingSubscribeRequest.mHandlerAppState = responder;
        aOutParam.mIncomingSubscribeRequest.mHandlerEventCallback = MockWdmSubscriptionResponderImpl::PublisherEventCallback;
        aOutParam.mIncomingSubscribeRequest.mRejectRequest = false;

        aInParam.mIncomingSubscribeRequest.mBinding->SetDefaultResponseTimeout(kResponseTimeoutMsec);
        aInParam.mIncomingSubscribeRequest.mBinding->SetDefaultWRMPConfig(gWRMPConfig);

        break;
    default:
        SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

void MockWdmSubscriptionResponderImpl::AddNewVersion(int aTraitDataSinkIndex)
{
    VersionNode * curr = &mTraitVersionSet[aTraitDataSinkIndex];
    while (curr->next != NULL)
    {
        curr = curr->next;
    }

    if (curr->versionInfo != mSinkCatalogStore[aTraitDataSinkIndex].mItem->GetVersion())
    {
        VersionNode * updatingVersion = (VersionNode *)malloc(sizeof(VersionNode));
        WeaveLogDetail(DataManagement, "Trait %u version is changed %" PRIu64 " ---> %" PRIu64, aTraitDataSinkIndex, curr->versionInfo, mSinkCatalogStore[aTraitDataSinkIndex].mItem->GetVersion());
        updatingVersion->versionInfo = mSinkCatalogStore[aTraitDataSinkIndex].mItem->GetVersion();
        updatingVersion->next = NULL;
        curr->next = updatingVersion;
    }
}

void MockWdmSubscriptionResponderImpl::PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam, SubscriptionHandler::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    switch (aEvent)
    {
    case SubscriptionHandler::kEvent_OnSubscribeRequestParsed:
        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscribeRequestParsed");

        // ideally this number should be set to something for cloud service, and something else for everyone else
        // we can potentially copy this from the client side, but it would take considerable amount of code to be generic enough
        // setting to some constant here seems to be easier

        aInParam.mSubscribeRequestParsed.mHandler->GetBinding()->SetDefaultResponseTimeout(kResponseTimeoutMsec);
        aInParam.mSubscribeRequestParsed.mHandler->GetBinding()->SetDefaultWRMPConfig(gWRMPConfig);

        switch (responder->mTestCaseId)
        {
        case kTestCase_RejectIncomingSubscribeRequest:
            // reject right here and release the resources associated with this incoming request
            aInParam.mSubscribeRequestParsed.mHandler->EndSubscription(nl::Weave::Profiles::kWeaveProfile_Common,
                nl::Weave::Profiles::Common::kStatus_Canceled);
            break;
        default:
            WeaveLogDetail(DataManagement, "Liveness check range provided by client %u - %u sec. Set to %u sec",
                    aInParam.mSubscribeRequestParsed.mTimeoutSecMin,
                    aInParam.mSubscribeRequestParsed.mTimeoutSecMax,
                    responder->mTimeBetweenLivenessCheckSec);
            aInParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest(responder->mTimeBetweenLivenessCheckSec);
        }
        break;

    case SubscriptionHandler::kEvent_OnExchangeStart:
        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnExchangeStart");
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionEstablished:
        if (true == gClearDataSink || true == gCleanStatus)
        {
            responder->DumpPublisherTraits();
            gCleanStatus = false;
        }

        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscriptionEstablished");
        gSubscriptionHandler = aInParam.mSubscriptionEstablished.mHandler;
        gBinding = gSubscriptionHandler->GetBinding();
        gBinding->AddRef();

        if (responder->mIsMutualSubscription)
        {
            if (NULL != responder->mSubscriptionClient)
            {
                WeaveLogDetail(DataManagement, "Skip mutual subscription setup, for we only have one client");
            }
            else
            {
                WeaveLogDetail(DataManagement, "Creating mutual subscription");

                switch (responder->mTestCaseId)
                {
                case kTestCase_TestTrait:
                case kTestCase_TestUpdatableTrait:
                    responder->mNumPaths = 3;
                    responder->mTraitPaths[0].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink0Index];
                    responder->mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mTraitPaths[1].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink1Index];
                    responder->mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mTraitPaths[2].mTraitDataHandle = responder->mTraitHandleSet[kTestBDataSinkIndex];
                    responder->mTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;
                    break;
                case kTestCase_IntegrationTrait:
                    responder->mNumPaths = 1;
                    responder->mTraitPaths[0].mTraitDataHandle = responder->mTraitHandleSet[kLocaleCapabilitiesTraitSinkIndex];
                    responder->mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;
                    break;
                case kTestCase_TestOversizeTrait1:
                case kTestCase_TestOversizeTrait2:
                    responder->mNumPaths = 3;
                    responder->mTraitPaths[0].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink0Index];
                    responder->mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mTraitPaths[1].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink1Index];
                    responder->mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mTraitPaths[2].mTraitDataHandle = responder->mTraitHandleSet[kTestBDataSinkIndex];
                    responder->mTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;
                    break;

                case kTestCase_CompatibleVersionedRequest:
                case kTestCase_ForwardCompatibleVersionedRequest:
                case kTestCase_IncompatibleVersionedRequest:
                case kTestCase_IncompatibleVersionedCommandRequest:

                    responder->mNumPaths = 3;

                    for (int i = 0; i < 3; i++) {
                        if (responder->mTestCaseId == kTestCase_CompatibleVersionedRequest) {
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 1;
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 1;
                        }
                        else if (responder->mTestCaseId == kTestCase_ForwardCompatibleVersionedRequest) {
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 1;
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 4;
                        }
                        else if (responder->mTestCaseId == kTestCase_IncompatibleVersionedRequest) {
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 2;
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 4;
                        }
                        else if (responder->mTestCaseId == kTestCase_IncompatibleVersionedCommandRequest) {
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 1;
                            responder->mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 4;
                        }
                    }

                    responder->mVersionedTraitPaths[0].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink0Index];
                    responder->mVersionedTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mVersionedTraitPaths[1].mTraitDataHandle = responder->mTraitHandleSet[kTestADataSink1Index];
                    responder->mVersionedTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

                    responder->mVersionedTraitPaths[2].mTraitDataHandle = responder->mTraitHandleSet[kTestBDataSinkIndex];
                    responder->mVersionedTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;
                    break;
                }

                err = SubscriptionEngine::GetInstance()->NewClient(&(responder->mSubscriptionClient),
                    aInParam.mSubscriptionEstablished.mHandler->GetBinding(),
                    responder, ClientEventCallback,
                    &(responder->mSinkCatalog), kResponseTimeoutMsec * 2);
                VerifyOrExit(err == WEAVE_NO_ERROR, gBinding->Release());

                // TODO: EVENT-DEMO
                responder->mSubscriptionClient->InitiateCounterSubscription(
                    responder->mTimeBetweenLivenessCheckSec);
            }
        }
        else
        {
            if (gNumDataChangeBeforeCancellation != 0)
            {
                // alter data every gTimeBetweenDataChangeMsec milliseconds
                responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, aAppState);
            }
            else
            {
                if (gFinalStatus != kIdle)
                {
                    switch (gFinalStatus)
                    {
                    case kPublisherCancel:
                    case kPublisherAbort:
                        responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, responder);
                        break;
                    case kClientCancel:
                    case kClientAbort:
                        responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, responder);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscriptionTerminated. peer = 0x%" PRIX64 ", %s: %s",
                aInParam.mSubscriptionTerminated.mHandler->GetPeerNodeId(),
                (aInParam.mSubscriptionTerminated.mIsStatusCodeValid) ? "Status Report" : "Error",
                (aInParam.mSubscriptionTerminated.mIsStatusCodeValid)
                    ? ::nl::StatusReportStr(aInParam.mSubscriptionTerminated.mStatusProfileId, aInParam.mSubscriptionTerminated.mStatusCode)
                    : ::nl::ErrorStr(aInParam.mSubscriptionTerminated.mReason));
        switch (gFinalStatus)
        {
        case kPublisherCancel:
        case kPublisherAbort:
            responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorPublisherCurrentState, responder);
            break;
        case kClientCancel:
        case kClientAbort:
            responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorClientCurrentState, responder);
            break;
        case kIdle:
        default:
            break;
        }

        if (gNumDataChangeBeforeCancellation != 0)
        {
            //responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleDataFlipTimeout, aAppState);
        }
        HandlePublisherRelease();
        gResponderState.init();
        responder->onCompleteTest();
        break;

    default:
        SubscriptionHandler::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR && gSubscriptionHandler)
    {
        // tell the handler to cancel
        (void)gSubscriptionHandler->EndSubscription();
    }

    WeaveLogFunctError(err);
}

void MockWdmSubscriptionResponderImpl::PrintVersionsLog()
{
    for (int i = 0; i< kNumTraitHandles; i++)
    {
        VersionNode * pre = &mTraitVersionSet[i];
        VersionNode * curr = mTraitVersionSet[i].next;

        printf("Responder's trait %u versions log is : %" PRIu64, i, pre->versionInfo);
        while (curr != NULL)
        {
            pre = curr;
            curr = curr->next;
            printf(" ==> %" PRIu64, pre->versionInfo);
        }
        printf("\n");
        mTraitVersionSet[i].next = NULL;
    }
}

void MockWdmSubscriptionResponderImpl::ClearDataSinkState(void)
{
    gClearDataSink = true;
}

void MockWdmSubscriptionResponderImpl::ClearDataSinkIterator(void *aTraitInstance, TraitDataHandle aHandle, void *aContext)
{
    MockTraitDataSink *traitInstance = static_cast<MockTraitDataSink *>(aTraitInstance);
    traitInstance->ResetDataSink();
}


void MockWdmSubscriptionResponderImpl::ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
        const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    switch (aEvent)
    {
    case SubscriptionClient::kEvent_OnExchangeStart:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnExchangeStart");
        break;

    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded");
        if (gSubscriptionHandler->GetSubscriptionId(&(aOutParam.mSubscribeRequestPrepareNeeded.mSubscriptionId))
                != WEAVE_NO_ERROR)
        {
            WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded invalid state");
            HandleClientRelease(aAppState);
        }
        else
        {
            if (responder->mTestCaseId >= kTestCase_CompatibleVersionedRequest && responder->mTestCaseId <= kTestCase_IncompatibleVersionedCommandRequest) {
                aOutParam.mSubscribeRequestPrepareNeeded.mVersionedPathList = responder->mVersionedTraitPaths;
            }
            else {
                aOutParam.mSubscribeRequestPrepareNeeded.mPathList = responder->mTraitPaths;
            }

            aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize = responder->mNumPaths;
            aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = true;
            aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
            aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
        }

        break;

    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionEstablished");
        WeaveLogDetail(DataManagement, "Liveness Timeout: %u msec", aInParam.mSubscriptionEstablished.mClient->GetLivenessTimeoutMsec());
        if (gNumDataChangeBeforeCancellation != 0)
        {
            // alter data every gTimeBetweenDataChangeMsec milliseconds
            responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, aAppState);
        }
        else
        {
            if (gFinalStatus != kIdle)
            {
                switch (gFinalStatus)
                {
                case kPublisherCancel:
                case kPublisherAbort:
                    responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, responder);
                    break;
                case kClientCancel:
                case kClientAbort:
                    responder->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, responder);
                    break;
                default:
                    break;
                }
            }
        }
        break;
    case SubscriptionClient::kEvent_OnNotificationRequest:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationRequest");
        break;
    case SubscriptionClient::kEvent_OnNotificationProcessed:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationProcessed");
        switch (responder->mTestCaseId)
        {
        case kTestCase_IntegrationTrait:
            responder->AddNewVersion(responder->kLocaleCapabilitiesTraitSinkIndex);
            break;
        case kTestCase_TestTrait:
        case kTestCase_CompatibleVersionedRequest:
        case kTestCase_ForwardCompatibleVersionedRequest:
        case kTestCase_IncompatibleVersionedCommandRequest:
            responder->AddNewVersion(responder->kTestADataSink0Index);
            responder->AddNewVersion(responder->kTestADataSink1Index);
            responder->AddNewVersion(responder->kTestBDataSinkIndex);
            break;
        case kTestCase_TestOversizeTrait1:
        case kTestCase_TestOversizeTrait2:
            responder->AddNewVersion(responder->kTestADataSink0Index);
            responder->AddNewVersion(responder->kTestADataSink1Index);
            break;
        }
        responder->DumpClientTraits();
        break;
    case SubscriptionClient::kEvent_OnSubscriptionTerminated:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated, Reason: %u, peer = 0x%" PRIX64 "\n",
                aInParam.mSubscriptionTerminated.mReason,
                aInParam.mSubscriptionTerminated.mClient->GetPeerNodeId());

        switch (gFinalStatus)
        {
        case kPublisherCancel:
        case kPublisherAbort:
            responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorPublisherCurrentState, responder);
            break;
        case kClientCancel:
        case kClientAbort:
            responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorClientCurrentState, responder);
            break;
        case kIdle:
        default:
            break;
        }
        if (gNumDataChangeBeforeCancellation != 0)
        {
            responder->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleDataFlipTimeout, responder);
        }
        if (gClearDataSink)
        {
            responder->mSinkCatalog.Iterate(MockWdmSubscriptionResponderImpl::ClearDataSinkIterator, NULL);
        }
        HandleClientRelease(responder);
        HandlePublisherRelease();
        gResponderState.init();
        responder->onCompleteTest();
        break;
    default:
        SubscriptionClient::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

void MockWdmSubscriptionResponderImpl::HandlePublisherComplete()
{
    if (NULL != gSubscriptionHandler)
    {
        if (gFinalStatus == kPublisherCancel)
        {
            (void)gSubscriptionHandler->EndSubscription();
        }
        if (gFinalStatus == kPublisherAbort)
        {
            (void)gSubscriptionHandler->AbortSubscription();
        }
    }
}

void MockWdmSubscriptionResponderImpl::HandleClientRelease(void *aAppState)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    if (NULL != responder->mSubscriptionClient)
    {
        responder->mSubscriptionClient->Free();
        responder->mSubscriptionClient = NULL;
    }
}

void MockWdmSubscriptionResponderImpl::HandleClientComplete(void *aAppState)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    if (NULL != responder->mSubscriptionClient)
    {
        if (gFinalStatus == kClientCancel)
        {
            (void)responder->mSubscriptionClient->EndSubscription();
        }
        if (gFinalStatus == kClientAbort)
        {
            responder->mSubscriptionClient->AbortSubscription();
            responder->mSubscriptionClient->Free();
            responder->mSubscriptionClient = NULL;
        }
    }
}

void MockWdmSubscriptionResponderImpl::HandlePublisherRelease()
{

    gSubscriptionHandler = NULL;

    if (NULL != gBinding)
    {
        gBinding->Release();
        gBinding = NULL;
    }

}

void MockWdmSubscriptionResponderImpl::Command_End(const bool aAbort)
{
    WeaveLogDetail(DataManagement, "Responder %s: state: %d", __func__, mCmdState);

    (void)mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleCustomCommandTimeout, this);

    mCmdState = kCmdState_Idle;
    if (NULL != mEcCommand)
    {
        // this might be needed, for the test infrastructure re-init this object multiple times
        // ExchangeContext::Close would gracefully close this exchange, while ExchangeContext::Abort
        // would forcefully reclaim all resources
        if (aAbort)
        {
            mEcCommand->Abort();
        }
        else
        {
            mEcCommand->Close();
        }
        mEcCommand = NULL;
    }
}

void MockWdmSubscriptionResponderImpl::HandleCustomCommandTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState, ::nl::Weave::System::Error aErr)
{
    MockWdmSubscriptionResponderImpl * const pResponder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);

    WeaveLogDetail(DataManagement, "Responder %s", __func__);

    pResponder->Command_End(true);
}

void MockWdmSubscriptionResponderImpl::OnMessageReceivedForCustomCommand (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
        const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId,
        uint8_t aMsgType, PacketBuffer *aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockWdmSubscriptionResponderImpl * pResponder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aEC->AppState);
    nl::Weave::Profiles::StatusReporting::StatusReport status;
    enum
    {
        kMsgIdentified_Unknown        = 0,
        kMsgIdentified_StatusReport   = 1,
        kMsgIdentified_Response       = 2,
        kMsgIdentified_InProgress     = 3,
    } messageType = kMsgIdentified_Unknown;

    WeaveLogDetail(DataManagement, "Responder %s: state: %d", __func__, pResponder->mCmdState);

    VerifyOrExit(aEC == pResponder->mEcCommand, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(kCmdState_Idle != pResponder->mCmdState, err = WEAVE_ERROR_INCORRECT_STATE);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) && (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        // Note that payload is not freed in this call to parse
        err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);
        messageType = kMsgIdentified_StatusReport;
        WeaveLogError(DataManagement, "Received Status Report 0x%" PRIX32 " : 0x%" PRIX16, status.mProfileId, status.mStatusCode);
    }
    else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_CustomCommandResponse == aMsgType))
    {
        // command response, implies the request succeeded to some degree
        messageType = kMsgIdentified_Response;
    }
    else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_InProgress == aMsgType))
    {
        messageType = kMsgIdentified_InProgress;
    }
    else
    {
        messageType = kMsgIdentified_Unknown;
    }

    switch (pResponder->mCmdState)
    {
    case kCmdState_Requesting:
        if (kMsgIdentified_InProgress == messageType)
        {
            // in progress, indicates the other side has received the request and is working on it
            // note we could have different time out settings after receiving this in progress message.
            // it's up to each individual trait.
            WeaveLogDetail(DataManagement, "Received In Progress message. Waiting for a response");
            // there is no payload in "in progress" message, so there is nothing to parse
            pResponder->mCmdState = kCmdState_Operating;
            // do not close this EC when we leave this function
            aEC = NULL;
            break;
        }
        // fall through intentional
    case kCmdState_Operating:
        if (kMsgIdentified_Response == messageType)
        {
            // command response, implies the request succeeded to some degree
            WeaveLogDetail(DataManagement, "Response message, end");

            nl::Weave::TLV::TLVReader reader;
            reader.Init(aPayload);

            err = reader.Next();
            SuccessOrExit(err);

            CustomCommandResponse::Parser response;
            err = response.Init(reader);
            SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
            // this function only print out all recognized properties in the response
            // check the parser class to see what else is available
            err = response.CheckSchemaValidity();
            SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        }
        else
        {
            if (kMsgIdentified_StatusReport == messageType)
            {
                // status report, implies the request failed
                WeaveLogDetail(DataManagement, "Status Report, end");
            }
            else
            {
                // unexpected message
                WeaveLogDetail(DataManagement, "Unexpected message, end");
            }
        }
        // reset state and close the exchange
        pResponder->Command_End();
        // aEC has been closed/aborted as part of Command_End
        aEC = NULL;
        break;
    default:
        // this is probably dead code
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    if (WEAVE_NO_ERROR != err)
    {
        pResponder->Command_End(true);
        // aEC has been closed/aborted as part of Command_End
        aEC = NULL;
    }

    if (NULL != aEC)
    {
        aEC->Abort();
        aEC = NULL;
    }

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }
}

void MockWdmSubscriptionResponderImpl::Command_Send(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = PacketBuffer::New();
    static uint32_t commandType = 1;
    static uint32_t signatureType = 0; // static dummy signature.  Values: 0 == None, 1 -- Weave Signature, 2 -- Group Key Signature

    WeaveLogDetail(DataManagement, "Responder %s: state: %d", __func__, mCmdState);

    printf("<<< TestCaseId %u >>>\n", mTestCaseId);

    VerifyOrExit((NULL != gBinding) && (kCmdState_Idle == mCmdState) && (NULL == mEcCommand), err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    err = gBinding->NewExchangeContext(mEcCommand);
    SuccessOrExit(err);

    mEcCommand->AppState = this;
    mEcCommand->OnMessageReceived = OnMessageReceivedForCustomCommand;
    mEcCommand->OnResponseTimeout = NULL;
    mEcCommand->OnSendError = NULL;
    mEcCommand->OnAckRcvd = NULL;

    {
        nl::Weave::TLV::TLVWriter writer;
        CustomCommand::Builder request;
        uint64_t nowMicroSecs, deadline;

        writer.Init(msgBuf);
        err = request.Init(&writer);
        SuccessOrExit(err);

        Path::Builder & path = request.CreatePathBuilder();

        if (mTestCaseId == kTestCase_ForwardCompatibleVersionedRequest) {
            path.ProfileID(Schema::Nest::Test::Trait::TestATrait::kWeaveProfileId, SchemaVersionRange(4, 1)).InstanceID(1).EndOfPath();
        }
        else if (mTestCaseId == kTestCase_IncompatibleVersionedCommandRequest) {
            path.ProfileID(Schema::Nest::Test::Trait::TestATrait::kWeaveProfileId, SchemaVersionRange(4, 2)).InstanceID(1).EndOfPath();
        }
        else {
            path.ProfileID(Schema::Nest::Test::Trait::TestATrait::kWeaveProfileId).InstanceID(1).EndOfPath();
        }

        SuccessOrExit(path.GetError());

        // Command Type: alternating
        commandType = (1 == commandType) ? 2 : 1;
        request.CommandType(commandType);
        request.MustBeVersion(mTestADataSink1.GetVersion());

        // It's safe to bail out after a series of operation, for
        // SubscriptionRequest::Builder would internally turn to NOP after error is logged
        SuccessOrExit(err = request.GetError());

        err = System::Layer::GetClock_RealTime(nowMicroSecs);
        SuccessOrExit(err);

        deadline = nowMicroSecs + kCommandTimeoutMicroSecs;
        request.ExpiryTimeMicroSecond((int64_t)deadline);

        SuccessOrExit(err = request.GetError());

        // Add arguments here
        {
            uint32_t dummyUInt = 7;
            bool dummyBool = false;
            nl::Weave::TLV::TLVType dummyType = nl::Weave::TLV::kTLVType_NotSpecified;
            err = writer.StartContainer(nl::Weave::TLV::ContextTag(CustomCommand::kCsTag_Argument), nl::Weave::TLV::kTLVType_Structure, dummyType);
            SuccessOrExit(err);

            err = writer.Put(nl::Weave::TLV::ContextTag(1), dummyUInt);
            SuccessOrExit(err);
            err = writer.PutBoolean(nl::Weave::TLV::ContextTag(2), dummyBool);
            SuccessOrExit(err);

            err = writer.EndContainer(dummyType);
            SuccessOrExit(err);
        }

        switch (signatureType)
        {
            case 0: // dummy signature -- No signature
                WeaveLogDetail(DataManagement, "Command with no authenticator" );
                break;
            case 1: // dummy signature -- Weave signature
                {
                    nl::Weave::TLV::TLVType dummyType =  nl::Weave::TLV::kTLVType_NotSpecified;

                    WeaveLogDetail(DataManagement, "Command with dummy WeaveSignature authenticator" );

                    err = writer.StartContainer(nl::Weave::TLV::ProfileTag(nl::Weave::Profiles::kWeaveProfile_Security,  nl::Weave::Profiles::Security::kTag_WeaveSignature), nl::Weave::TLV::kTLVType_Structure, dummyType);
                    SuccessOrExit(err);

                    // dummy signature -- for now, just an algorithm specification
                    err = writer.Put(nl::Weave::TLV::ContextTag(nl::Weave::Profiles::Security::kTag_WeaveSignature_SignatureAlgorithm), nl::Weave::ASN1::kOID_SigAlgo_ECDSAWithSHA256);
                    SuccessOrExit(err);

                    err = writer.EndContainer(dummyType);
                    SuccessOrExit(err);
                    break;
                }
            case 2: // dummy signature -- Group key signature
                {
                    nl::Weave::TLV::TLVType dummyType =  nl::Weave::TLV::kTLVType_NotSpecified;

                    WeaveLogDetail(DataManagement, "Command with dummy Group Key Authenticator authenticator" );

                    err = writer.StartContainer(nl::Weave::TLV::ProfileTag(nl::Weave::Profiles::kWeaveProfile_Security,  nl::Weave::Profiles::Security::kTag_GroupKeySignature), nl::Weave::TLV::kTLVType_Structure, dummyType);
                    SuccessOrExit(err);

                    // dummy signature -- for now, just an algorithm specification
                    err = writer.Put(nl::Weave::TLV::ContextTag(nl::Weave::Profiles::Security::kTag_GroupKeySignature_SignatureAlgorithm), nl::Weave::ASN1::kOID_SigAlgo_HMACWithSHA256);
                    SuccessOrExit(err);

                    err = writer.EndContainer(dummyType);
                    SuccessOrExit(err);
                    break;
                }
        }

        request.EndOfCustomCommand();
        SuccessOrExit(err = request.GetError());

        err = writer.Finalize();
        SuccessOrExit(err);

        if (commandType == 1)
        {
            signatureType = (signatureType + 1) % 3;
        }


        err = mEcCommand->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_CustomCommandRequest,
            msgBuf, nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        msgBuf = NULL;
        SuccessOrExit(err);
    }

    err = mExchangeMgr->MessageLayer->SystemLayer->StartTimer(5000, HandleCustomCommandTimeout, this);
    SuccessOrExit(err);

    mCmdState = kCmdState_Requesting;

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        Command_End(true);
    }
}

void MockWdmSubscriptionResponderImpl::HandleDataFlipTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState,
    nl::Weave::System::Error aErr)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);

    IgnoreUnusedVariable(aErr);

    if (gEnableDataFlip == true)
    {
        WeaveLogDetail(DataManagement, "\n\n\n\n\nFlipping data...");

        switch (responder->mTestCaseId)
        {
        case kTestCase_IntegrationTrait:
        case kTestCase_RejectIncomingSubscribeRequest:
            responder->mLocaleSettingsDataSource.Mutate();
            responder->mApplicationKeysTraitDataSource.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;
        case kTestCase_TestTrait:
        case kTestCase_CompatibleVersionedRequest:
        case kTestCase_ForwardCompatibleVersionedRequest:
            responder->mTestADataSource0.Mutate();
            responder->mTestADataSource1.Mutate();
            responder->mTestBDataSource.Mutate();
            responder->mLocaleSettingsDataSource.Mutate();
            responder->Command_Send();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;
        case kTestCase_TestUpdatableTrait:
            break;
        case kTestCase_IncompatibleVersionedCommandRequest:
            responder->Command_Send();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;

        case kTestCase_TestOversizeTrait1:
            responder->mTestADataSource0.Mutate();
            responder->mTestADataSource1.Mutate();
            responder->mLocaleSettingsDataSource.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;

        case kTestCase_TestOversizeTrait2:
            responder->mTestADataSource0.Mutate();
            responder->mTestADataSource1.Mutate();
            responder->mLocaleSettingsDataSource.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;
        }
        responder->DumpPublisherTraits();
    }

    if (gNumDataChangeBeforeCancellation == -1)
    {
        WeaveLogDetail(DataManagement, "immortal, no cancel or abort, completed cycle %d", gResponderState.mDataflipCount);
        // alter data every gTimeBetweenDataChangeMsec milliseconds
        aSystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, responder);
        ++gResponderState.mDataflipCount;
    }
    else
    {
        WeaveLogDetail(DataManagement, "Completed cycle %d per %d", gResponderState.mDataflipCount, gNumDataChangeBeforeCancellation);
        if (gResponderState.mDataflipCount == gNumDataChangeBeforeCancellation)
        {
            gResponderState.mDataflipCount = 1;
            if (gFinalStatus != kIdle)
            {
                switch (gFinalStatus)
                {
                case kPublisherCancel:
                case kPublisherAbort:
                    aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, responder);
                    break;
                case kClientCancel:
                case kClientAbort:
                    aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, responder);
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            // alter data every gTimeBetweenDataChangeMsec milliseconds
            ++gResponderState.mDataflipCount;
            aSystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, responder);
        }
    }
}

void MockWdmSubscriptionResponderImpl::MonitorPublisherCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    if (NULL != gSubscriptionHandler)
    {
        if (gSubscriptionHandler->IsEstablishedIdle() && (NULL == responder->mSubscriptionClient || responder->mSubscriptionClient->IsEstablishedIdle()))
        {
            WeaveLogDetail(DataManagement, "state transitions to idle within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
            gResponderState.mPublisherStateCount = 1;
            HandlePublisherComplete();
            if (!responder->mIsMutualSubscription)
            {
                HandlePublisherRelease();
                WeaveLogDetail(DataManagement, "One_way: Good Iteration");
                responder->onCompleteTest();
            }
        }
        else
        {
            if (gResponderState.mPublisherStateCount < kMonitorCurrentStateCnt)
            {
                gResponderState.mPublisherStateCount ++;
                aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, responder);
            }
            else
            {
                gResponderState.mPublisherStateCount = 1;
                WeaveLogDetail(DataManagement, "state is not idle or aborted within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
                (void)gSubscriptionHandler->AbortSubscription();
                HandleClientRelease(responder);
                HandlePublisherRelease();
                responder->onCompleteTest();
            }
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "gSubscriptionHandler is NULL, and current session is torn down");
        HandleClientRelease(responder);
        HandlePublisherRelease();
        responder->onCompleteTest();
    }
}

void MockWdmSubscriptionResponderImpl::MonitorClientCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr)
{
    MockWdmSubscriptionResponderImpl * const responder = reinterpret_cast<MockWdmSubscriptionResponderImpl *>(aAppState);
    if (NULL != responder->mSubscriptionClient)
    {
        if (responder->mSubscriptionClient->IsEstablishedIdle() && (NULL == gSubscriptionHandler || gSubscriptionHandler->IsEstablishedIdle()))
        {
            WeaveLogDetail(DataManagement, "state transitions to idle within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
            gResponderState.mClientStateCount = 1;
            HandleClientComplete(responder);
        }
        else
        {
            if (gResponderState.mClientStateCount < kMonitorCurrentStateCnt)
            {
                gResponderState.mClientStateCount ++;
                aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, responder);
            }
            else
            {
                gResponderState.mClientStateCount = 1;
                WeaveLogDetail(DataManagement, "state is not idle or aborted within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
                (void)gSubscriptionHandler->AbortSubscription();
            }
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "mSubscriptionClient is NULL, and current session is torn down");
        (void)gSubscriptionHandler->AbortSubscription();
        HandleClientRelease(responder);
        HandlePublisherRelease();
        responder->onCompleteTest();
    }
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

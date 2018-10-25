/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements the Weave Data Management mock subscription initiator.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <new>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/security/ApplicationKeysTraitDataSink.h>
#include "MockWdmTestVerifier.h"
#include "MockWdmSubscriptionInitiator.h"
#include "MockSinkTraits.h"
#include "MockSourceTraits.h"
#include "TestGroupKeyStore.h"

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

// Any time setting lower than this would force the subscription client to send Subscribe Confirm continuously.
uint32_t gMinimumTimeBetweenLivenessCheckSec = ((WEAVE_CONFIG_WRMP_DEFAULT_MAX_RETRANS + 1) * kWRMPActiveRetransTimeoutMsec + 999) / 1000;

static int gNumDataChangeBeforeCancellation;
static int gFinalStatus;
static SubscriptionHandler *gSubscriptionHandler = NULL;
static int gTimeBetweenDataChangeMsec = 0;
static bool gIsMutualSubscription = true;
static bool gEnableDataFlip = true;
static bool gMutualSubscriptionEstablished = false;
static bool gOnewaySubscriptionEstablished = false;
static bool gEvaluateSuccessIteration = false;
static bool gCleanStatus = true;
static bool gTestCase_TestOversizeTrait2DumpFlip = true;
static nl::Weave::WRMPConfig gWRMPConfig = { kWRMPInitialRetransTimeoutMsec, kWRMPActiveRetransTimeoutMsec, kWRMPAckTimeoutMsec, kWRMPMaxRetrans };

static nlDEFINE_ALIGNED_VAR(sTestGroupKeyStore, sizeof(TestGroupKeyStore), void*);

struct VersionNode
{
    uint64_t versionInfo;
    VersionNode * next;
};


class WdmInitiatorState
{
public:
    int mDataflipCount;
    int mClientStateCount;
    int mPublisherStateCount;
    void init(void)
    {
        mDataflipCount = 0;
        mClientStateCount = 1;
        mPublisherStateCount = 1;
    }
};

static WdmInitiatorState gInitiatorState;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
class MockWdmSubscriptionInitiatorImpl: public MockWdmSubscriptionInitiator
{
public:
    MockWdmSubscriptionInitiatorImpl();

    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr,
                              uint32_t aKeyId,
                              uint32_t aTestSecurityMode,
                              const MockWdmNodeOptions &aConfig);

    virtual WEAVE_ERROR StartTesting(const uint64_t aPublisherNodeId, const uint16_t aSubnetId);
    virtual int32_t GetNumFaultInjectionEventsAvailable(void);
    void PrintVersionsLog();
    void ClearDataSinkState(void);
    void Cleanup(void);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    nl::Weave::Binding * mBinding;

    uint64_t mPublisherNodeId;
    uint16_t mPublisherSubnetId;

    static bool mClearDataSink;
    int mTestCaseId;
    int mTestSecurityMode;
    uint32_t mKeyId;

    TraitPath mTraitPaths[4];
    VersionedTraitPath mVersionedTraitPaths[4];
    uint32_t mNumPaths;

    bool mEnableRetry;
    bool mWillRetry;

    // publisher side
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[4];
    nl::Weave::Profiles::DataManagement_Current::TraitSchemaEngine::IGetDataDelegate* mSinkAddressList[9];

    // source traits
    LocaleCapabilitiesTraitDataSource mLocaleCapabilitiesDataSource;
    TestATraitDataSource mTestATraitDataSource0;
    TestATraitDataSource mTestATraitDataSource1;
    TestBTraitDataSource mTestBTraitDataSource;
    TestBLargeTraitDataSource mTestBLargeTraitDataSource;

    static void ClearDataSinkIterator(void *aTraitInstance, TraitDataHandle aHandle, void *aContext);

    static void EngineEventCallback (void * const aAppState, SubscriptionEngine::EventID aEvent,
        const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam);

    static void PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam,
        SubscriptionHandler::OutEventParam & aOutParam);

    // client side
    SingleResourceSinkTraitCatalog mSinkCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[9];

    // sink traits
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    LocaleSettingsTraitUpdatableDataSink mLocaleSettingsTraitUpdatableDataSink;
    TestATraitUpdatableDataSink mTestATraitUpdatableDataSink0;
    TestATraitUpdatableDataSink mTestATraitUpdatableDataSink1;
    TestBTraitUpdatableDataSink mTestBTraitUpdatableDataSink;

    MockWdmNodeOptions::WdmUpdateMutation mUpdateMutation;
    MockWdmNodeOptions::WdmUpdateConditionality mUpdateConditionality;
    MockWdmNodeOptions::WdmUpdateTiming mUpdateTiming;
    uint32_t mUpdateNumTraits;
    uint32_t mUpdateNumMutations;
    uint32_t mUpdateNumRepeatedMutations;
    uint32_t mUpdateSameMutationCounter;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    BoltLockSettingTraitDataSink mBoltLockSettingsTraitDataSink;
    TestATraitDataSink mTestATraitDataSink0;
    LocaleSettingsTraitDataSink mLocaleSettingsTraitDataSink;
    TestATraitDataSink mTestATraitDataSink1;
    TestBTraitDataSink mTestBTraitDataSink;

    TestApplicationKeysTraitDataSink mApplicationKeysTraitDataSink;

    enum
    {
        kTestATraitSink0Index = 0,
        kTestATraitSink1Index,
        kTestBTraitSinkIndex,
        kLocaleSettingsSinkIndex,
        kBoltLockSettingTraitSinkIndex,
        kApplicationKeysTraitSinkIndex,

        kLocaleCapabilitiesSourceIndex,
        kTestATraitSource0Index,
        kTestATraitSource1Index,
        kTestBTraitSourceIndex,
        kTestBLargeTraitSourceIndex,
        kMaxNumTraitHandles,
    };

    enum
    {
        kClientCancel = 0,
        kPublisherCancel,
        kClientAbort,
        kPublisherAbort,
        kIdle
    };

    TraitDataHandle mTraitHandleSet[kMaxNumTraitHandles];

    enum
    {
        // subscribe LocaleSettings, TestA(two instances) and TestB traits in initiator
        // publish TestA(two instances) and TestB traits in initiator
        kTestCase_TestTrait = 1,

        // subscribe Locale Setting, ApplicationKeys traits in initiator
        // publish Locale Capabilities traits in responder
        kTestCase_IntegrationTrait = 2,

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

        kTestCase_TestUpdatableTraits = 10,
    };

    enum
    {
        kMonitorCurrentStateCnt = 160,
        kMonitorCurrentStateInterval = 120 //msec
    };

    VersionNode mTraitVersionSet[kMaxNumTraitHandles];

    SubscriptionClient * mSubscriptionClient;

    void AddNewVersion (int aTraitDataSinkIndex);

    void DumpClientTraitChecksum(int TraitDataSinkIndex);
    void DumpClientTraits(void);

    void DumpPublisherTraitChecksum(int TraitDataSounceIndex);
    void DumpPublisherTraits(void);

    WEAVE_ERROR PrepareBinding();

    static void ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
        const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam);

    static void BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
        const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam);

    static void HandleClientComplete(void *aAppState);

    static void HandlePublisherComplete();

    static void HandlePublisherRelease();

    static void HandleDataFlipTimeout (nl::Weave::System::Layer *aSystemLayer, void *aAppState, nl::Weave::System::Error aErr);
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    WEAVE_ERROR ApplyWdmUpdateMutations();
#endif

    static void MonitorPublisherCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr);

    static void MonitorClientCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr);
};


bool MockWdmSubscriptionInitiatorImpl::mClearDataSink = false;

MockWdmSubscriptionInitiatorImpl::MockWdmSubscriptionInitiatorImpl() :
    mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, sizeof(mSourceCatalogStore) / sizeof(mSourceCatalogStore[0])),
    mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0]))
{
}

int32_t MockWdmSubscriptionInitiatorImpl::GetNumFaultInjectionEventsAvailable(void)
{
    int32_t retval = 0;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    if (mSubscriptionClient && mSubscriptionClient->IsUpdateInFlight())
    {
        retval = 1;
    }
#endif

    return retval;

}

WEAVE_ERROR MockWdmSubscriptionInitiatorImpl::Init(
        nl::Weave::WeaveExchangeManager *aExchangeMgr,
        uint32_t aKeyId,
        uint32_t aTestSecurityMode,
        const MockWdmNodeOptions &aConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    gIsMutualSubscription = aConfig.mEnableMutualSubscription;

    WeaveLogDetail(DataManagement, "Test Case ID: %s", (aConfig.mTestCaseId == NULL) ? "NULL" : aConfig.mTestCaseId);

    if (NULL != aConfig.mNumDataChangeBeforeCancellation)
    {
        gNumDataChangeBeforeCancellation = atoi(aConfig.mNumDataChangeBeforeCancellation);
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
        gFinalStatus = 0;
    }

    if (NULL != aConfig.mTimeBetweenDataChangeMsec)
    {
        gTimeBetweenDataChangeMsec = atoi(aConfig.mTimeBetweenDataChangeMsec);
    }
    else
    {
        gTimeBetweenDataChangeMsec = 15000;
    }

    if (NULL != aConfig.mTimeBetweenLivenessCheckSec)
    {
        gMinimumTimeBetweenLivenessCheckSec = atoi(aConfig.mTimeBetweenLivenessCheckSec);
    }
    else
    {
        gMinimumTimeBetweenLivenessCheckSec = 30;
    }

    gEnableDataFlip = aConfig.mEnableDataFlip;

    printf("aTestCaseId = %s\n", aConfig.mTestCaseId);

    if (NULL != aConfig.mTestCaseId)
    {
        mTestCaseId = atoi(aConfig.mTestCaseId);
    }
    else
    {
        mTestCaseId = kTestCase_TestTrait;
    }

    mTestSecurityMode = aTestSecurityMode;

    mKeyId = aKeyId;

    mTestATraitDataSource0.mTraitTestSet = 0;

    mTestATraitDataSource1.mTraitTestSet = 0;

    if (aConfig.mEnableDictionaryTest)
    {
        mTestATraitDataSource1.mTraitTestSet = 1;
    }

    mEnableRetry = aConfig.mEnableRetry;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    mUpdateMutation = aConfig.mWdmUpdateMutation;
    mUpdateConditionality = aConfig.mWdmUpdateConditionality;
    mUpdateTiming = aConfig.mWdmUpdateTiming;
    mUpdateNumTraits = aConfig.mWdmUpdateNumberOfTraits;
    mUpdateNumMutations = aConfig.mWdmUpdateNumberOfMutations;
    mUpdateNumRepeatedMutations = aConfig.mWdmUpdateNumberOfRepeatedMutations;
    mUpdateSameMutationCounter = 0;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    switch (mTestCaseId)
    {
    case kTestCase_TestUpdatableTraits:
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
        mSinkCatalog.Add(0, &mTestATraitUpdatableDataSink0, mTraitHandleSet[kTestATraitSink0Index]);
        mSinkCatalog.Add(1, &mTestATraitUpdatableDataSink1, mTraitHandleSet[kTestATraitSink1Index]);
        mSinkCatalog.Add(0, &mLocaleSettingsTraitUpdatableDataSink, mTraitHandleSet[kLocaleSettingsSinkIndex]);
        mSinkCatalog.Add(0, &mTestBTraitUpdatableDataSink, mTraitHandleSet[kTestBTraitSinkIndex]);
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
        break;
    default:
        mSinkCatalog.Add(0, &mTestATraitDataSink0, mTraitHandleSet[kTestATraitSink0Index]);
        mSinkCatalog.Add(1, &mTestATraitDataSink1, mTraitHandleSet[kTestATraitSink1Index]);
        mSinkCatalog.Add(0, &mTestBTraitDataSink, mTraitHandleSet[kTestBTraitSinkIndex]);
        mSinkCatalog.Add(0, &mLocaleSettingsTraitDataSink, mTraitHandleSet[kLocaleSettingsSinkIndex]);
    }

    mSinkCatalog.Add(0, &mBoltLockSettingsTraitDataSink, mTraitHandleSet[kBoltLockSettingTraitSinkIndex]);
    mApplicationKeysTraitDataSink.SetGroupKeyStore(new (&sTestGroupKeyStore) TestGroupKeyStore());
    mSinkCatalog.Add(0, &mApplicationKeysTraitDataSink, mTraitHandleSet[kApplicationKeysTraitSinkIndex]);

    mSourceCatalog.Add(0, &mLocaleCapabilitiesDataSource, mTraitHandleSet[kLocaleCapabilitiesSourceIndex]);
    mSourceCatalog.Add(1, &mTestATraitDataSource0, mTraitHandleSet[kTestATraitSource0Index]);
    mSourceCatalog.Add(2, &mTestATraitDataSource1, mTraitHandleSet[kTestATraitSource1Index]);

    switch (mTestCaseId)
    {
    case kTestCase_TestOversizeTrait1:
    case kTestCase_TestOversizeTrait2:
         mSourceCatalog.Add(1, &mTestBLargeTraitDataSource, mTraitHandleSet[kTestBLargeTraitSourceIndex]);
         break;
    default:
        mSourceCatalog.Add(1, &mTestBTraitDataSource, mTraitHandleSet[kTestBTraitSourceIndex]);
        break;
    }

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

    case kTestCase_TestUpdatableTraits:
        WeaveLogDetail(DataManagement, "kTestCase_TestUpdatableTraits");
        break;
    default:
        mTestCaseId = kTestCase_TestTrait;
        WeaveLogDetail(DataManagement, "kTestCase_TestTrait");
        break;
    }

    mExchangeMgr = aExchangeMgr;
    mBinding = NULL;

    mSubscriptionClient = NULL;

    // Note if you don't use publisher side, there is no need to initialize using this longer form
    err = SubscriptionEngine::GetInstance()->Init(mExchangeMgr, this, EngineEventCallback);
    SuccessOrExit(err);

    if (gIsMutualSubscription == true)
    {
        err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &mSourceCatalog);
        SuccessOrExit(err);
    }

    mTraitVersionSet[kTestATraitSink0Index].versionInfo = mTestATraitDataSink0.GetVersion();
    mTraitVersionSet[kTestATraitSink0Index].next = NULL;
    mTraitVersionSet[kTestATraitSink1Index].versionInfo = mTestATraitDataSink1.GetVersion();
    mTraitVersionSet[kTestATraitSink1Index].next = NULL;
    mTraitVersionSet[kTestBTraitSinkIndex].versionInfo = mTestBTraitDataSink.GetVersion();
    mTraitVersionSet[kTestBTraitSinkIndex].next = NULL;
    mTraitVersionSet[kLocaleSettingsSinkIndex].versionInfo = mLocaleSettingsTraitDataSink.GetVersion();
    mTraitVersionSet[kLocaleSettingsSinkIndex].next = NULL;
    mTraitVersionSet[kBoltLockSettingTraitSinkIndex].versionInfo = mBoltLockSettingsTraitDataSink.GetVersion();
    mTraitVersionSet[kBoltLockSettingTraitSinkIndex].next = NULL;
    mTraitVersionSet[kApplicationKeysTraitSinkIndex].versionInfo = mApplicationKeysTraitDataSink.GetVersion();
    mTraitVersionSet[kApplicationKeysTraitSinkIndex].next = NULL;

    mSinkAddressList[kTestATraitSink0Index] = &mTestATraitDataSink0;
    mSinkAddressList[kTestATraitSink1Index] = &mTestATraitDataSink1;
    mSinkAddressList[kTestBTraitSinkIndex] = &mTestBTraitDataSink;
    mSinkAddressList[kLocaleSettingsSinkIndex] = &mLocaleSettingsTraitDataSink;
    mSinkAddressList[kBoltLockSettingTraitSinkIndex] = &mBoltLockSettingsTraitDataSink;
    mSinkAddressList[kApplicationKeysTraitSinkIndex] = &mApplicationKeysTraitDataSink;

    //onCompleteTest = NULL;

exit:
    return err;
}

WEAVE_ERROR MockWdmSubscriptionInitiatorImpl::StartTesting(const uint64_t aPublisherNodeId, const uint16_t aSubnetId)
{
    gInitiatorState.init();
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mPublisherNodeId = aPublisherNodeId;
    mPublisherSubnetId = aSubnetId;

    if (mBinding == NULL)
    {
        mBinding = mExchangeMgr->NewBinding(BindingEventCallback, this);
        VerifyOrExit(NULL != mBinding, err = WEAVE_ERROR_NO_MEMORY);
    }

    if (mSubscriptionClient == NULL)
    {
        err =  SubscriptionEngine::GetInstance()->NewClient(&mSubscriptionClient,
                mBinding,
                this,
                ClientEventCallback,
                &mSinkCatalog,
                kResponseTimeoutMsec * 2); // max num of msec between subscribe request and subscribe response
        SuccessOrExit(err);
    }

    // TODO: EVENT-DEMO
    // TODO: Fix this dummy observed event list
    /*
    SubscriptionClient::LastObservedEvent DummyObservedEvents[] =
    {
        {1, 2, 3},
        {4, 5, 6},
    };
    */

    switch (mTestCaseId)
    {
    case kTestCase_IntegrationTrait:
    case kTestCase_RejectIncomingSubscribeRequest:
        mTraitPaths[0].mTraitDataHandle = mTraitHandleSet[kLocaleSettingsSinkIndex];
        mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[1].mTraitDataHandle = mTraitHandleSet[kApplicationKeysTraitSinkIndex];
        mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

        mNumPaths = 2;
        break;

    case kTestCase_TestTrait:
    case kTestCase_TestUpdatableTraits:
        mTraitPaths[0].mTraitDataHandle = mTraitHandleSet[kLocaleSettingsSinkIndex];
        mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[1].mTraitDataHandle = mTraitHandleSet[kTestATraitSink0Index];
        mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[2].mTraitDataHandle = mTraitHandleSet[kTestATraitSink1Index];
        mTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[3].mTraitDataHandle = mTraitHandleSet[kTestBTraitSinkIndex];
        mTraitPaths[3].mPropertyPathHandle = kRootPropertyPathHandle;

        mNumPaths = 4;
        break;

    case kTestCase_TestOversizeTrait1:
        mTraitPaths[0].mTraitDataHandle = mTraitHandleSet[kTestBTraitSinkIndex];
        mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[1].mTraitDataHandle = mTraitHandleSet[kTestATraitSink0Index];
        mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[2].mTraitDataHandle = mTraitHandleSet[kTestATraitSink1Index];
        mTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[3].mTraitDataHandle = mTraitHandleSet[kLocaleSettingsSinkIndex];
        mTraitPaths[3].mPropertyPathHandle = kRootPropertyPathHandle;

        mNumPaths = 4;
        break;

     case kTestCase_TestOversizeTrait2:
        mTraitPaths[0].mTraitDataHandle = mTraitHandleSet[kLocaleSettingsSinkIndex];
        mTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[1].mTraitDataHandle = mTraitHandleSet[kTestBTraitSinkIndex];
        mTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[2].mTraitDataHandle = mTraitHandleSet[kTestATraitSink0Index];
        mTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;

        mTraitPaths[3].mTraitDataHandle = mTraitHandleSet[kTestATraitSink1Index];
        mTraitPaths[3].mPropertyPathHandle = kRootPropertyPathHandle;

        mNumPaths = 4;
        break;

    case kTestCase_CompatibleVersionedRequest:
    case kTestCase_ForwardCompatibleVersionedRequest:
    case kTestCase_IncompatibleVersionedRequest:
        for (int i = 0; i < 4; i++) {
            if (mTestCaseId == kTestCase_CompatibleVersionedRequest) {
                mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 1;
                mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 1;
            }
            else if (mTestCaseId == kTestCase_ForwardCompatibleVersionedRequest) {
                mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 1;
                mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 4;
            }
            else if (mTestCaseId == kTestCase_IncompatibleVersionedRequest) {
                mVersionedTraitPaths[i].mRequestedVersionRange.mMinVersion = 2;
                mVersionedTraitPaths[i].mRequestedVersionRange.mMaxVersion = 4;
            }
        }

        mVersionedTraitPaths[0].mTraitDataHandle = mTraitHandleSet[kLocaleSettingsSinkIndex];
        mVersionedTraitPaths[0].mPropertyPathHandle = kRootPropertyPathHandle;

        mVersionedTraitPaths[1].mTraitDataHandle = mTraitHandleSet[kTestATraitSink0Index];
        mVersionedTraitPaths[1].mPropertyPathHandle = kRootPropertyPathHandle;

        mVersionedTraitPaths[2].mTraitDataHandle = mTraitHandleSet[kTestATraitSink1Index];
        mVersionedTraitPaths[2].mPropertyPathHandle = kRootPropertyPathHandle;

        mVersionedTraitPaths[3].mTraitDataHandle = mTraitHandleSet[kTestBTraitSinkIndex];
        mVersionedTraitPaths[3].mPropertyPathHandle = kRootPropertyPathHandle;

        mNumPaths = 4;
        break;

    default:
        mNumPaths = 0;
        break;
    }

    if (mEnableRetry)
    {
        mSubscriptionClient->EnableResubscribe(NULL);
    }

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
     if (mTestCaseId == kTestCase_TestUpdatableTraits &&
             mUpdateTiming == MockWdmNodeOptions::kTiming_BeforeSub)
     {
         WeaveLogDetail(DataManagement, "Mutating traits before the subscription");
         (void)ApplyWdmUpdateMutations();
         gInitiatorState.mDataflipCount++;
     }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    // TODO: EVENT-DEMO
    mSubscriptionClient->InitiateSubscription();

exit:
    WeaveLogFunctError(err);
    if (err != WEAVE_NO_ERROR && mBinding != NULL)
    {
        mBinding->Release();
        mBinding = NULL;
    }
    return err;
}

WEAVE_ERROR MockWdmSubscriptionInitiatorImpl::PrepareBinding()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Binding::Configuration bindingConfig = mBinding->BeginConfiguration()
        .Target_NodeId(mPublisherNodeId) // TODO: aPublisherNodeId
        .Transport_UDP_WRM()
        .Transport_DefaultWRMPConfig(gWRMPConfig)

        // (default) max num of msec between any outgoing message and next incoming message (could be a response to it)
        .Exchange_ResponseTimeoutMsec(kResponseTimeoutMsec);

    if (nl::Weave::kWeaveSubnetId_NotSpecified != mPublisherSubnetId)
    {
        bindingConfig.TargetAddress_WeaveFabric(mPublisherSubnetId);
    }

    switch (mTestSecurityMode)
    {
    case WeaveSecurityMode::kCASE:
        WeaveLogDetail(DataManagement, "security mode is kWdmSecurity_CASE");
        bindingConfig.Security_SharedCASESession();
        break;

    case WeaveSecurityMode::kGroupEnc:
        WeaveLogDetail(DataManagement, "security mode is kWdmSecurity_GroupKey");
        if (mKeyId == WeaveKeyId::kNone)
        {
            WeaveLogDetail(DataManagement, "Please specify a group encryption key id using the --group-enc-... options.\n");
            err = WEAVE_ERROR_INVALID_KEY_ID;
            SuccessOrExit(err);
        }
        bindingConfig.Security_Key(mKeyId);
        //.Security_Key(0x5536);
        //.Security_Key(0x4436);
        break;

    case WeaveSecurityMode::kNone:
        bindingConfig.Security_None();
        break;

    default:
        WeaveLogDetail(DataManagement, "security mode is not supported");
        err = WEAVE_ERROR_UNSUPPORTED_AUTH_MODE;
        SuccessOrExit(err);
    }

    err = bindingConfig.PrepareBinding();
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    return err;
}

void MockWdmSubscriptionInitiatorImpl::BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
    const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "%s: Event(%d)", __func__, aEvent);

    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(apAppState);

    VerifyOrDie(aInParam.Source != NULL);
    VerifyOrDie(aEvent == nl::Weave::Binding::kEvent_DefaultCheck || initiator->mBinding == aInParam.Source);

    switch (aEvent)
    {
    case nl::Weave::Binding::kEvent_PrepareRequested:
        WeaveLogDetail(DataManagement, "kEvent_PrepareRequested");
        err = initiator->PrepareBinding();
        SuccessOrExit(err);
        break;

    case nl::Weave::Binding::kEvent_PrepareFailed:
        err = aInParam.PrepareFailed.Reason;
        WeaveLogDetail(DataManagement, "kEvent_PrepareFailed: reason %d", err);
        break;

    case nl::Weave::Binding::kEvent_BindingFailed:
        err = aInParam.BindingFailed.Reason;
        WeaveLogDetail(DataManagement, "kEvent_BindingFailed: reason %d", err);
        break;

    case nl::Weave::Binding::kEvent_BindingReady:
        WeaveLogDetail(DataManagement, "kEvent_BindingReady");
        break;
    case nl::Weave::Binding::kEvent_DefaultCheck:
        WeaveLogDetail(DataManagement, "kEvent_DefaultCheck");
        // fall through
    default:
        nl::Weave::Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (NULL != initiator->onError)
        {
            initiator->onError();
        }
        initiator->mBinding->Release();
        initiator->mBinding = NULL;
        if (initiator->mSubscriptionClient)
        {
            initiator->mSubscriptionClient->Free();
            initiator->mSubscriptionClient = NULL;
        }
    }
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionInitiatorImpl::DumpPublisherTraitChecksum(int inTraitDataSourceIndex)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSource *dataSource;
    err = mSourceCatalog.Locate(mTraitHandleSet[inTraitDataSourceIndex], &dataSource);
    SuccessOrExit(err);

    ::DumpPublisherTraitChecksum(dataSource);
exit:
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionInitiatorImpl::DumpClientTraitChecksum(int inTraitDataSinkIndex)
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

void MockWdmSubscriptionInitiatorImpl::DumpClientTraits(void)
{
    switch (mTestCaseId)
    {
        case kTestCase_IntegrationTrait:
        case kTestCase_RejectIncomingSubscribeRequest:
            DumpClientTraitChecksum(kLocaleSettingsSinkIndex);
            DumpClientTraitChecksum(kApplicationKeysTraitSinkIndex);
            break;
        case kTestCase_TestTrait:
            DumpClientTraitChecksum(kTestATraitSink0Index);
            DumpClientTraitChecksum(kTestATraitSink1Index);
            DumpClientTraitChecksum(kTestBTraitSinkIndex);
            DumpClientTraitChecksum(kLocaleSettingsSinkIndex);
            break;
        case kTestCase_TestUpdatableTraits:
            break;
        case kTestCase_TestOversizeTrait1:
            DumpClientTraitChecksum(kTestATraitSink0Index);
            DumpClientTraitChecksum(kTestATraitSink1Index);
            DumpClientTraitChecksum(kLocaleSettingsSinkIndex);
            break;
        case kTestCase_TestOversizeTrait2:
            if (gTestCase_TestOversizeTrait2DumpFlip)
            {
                DumpClientTraitChecksum(kLocaleSettingsSinkIndex);
            }
            else
            {
                DumpClientTraitChecksum(kTestATraitSink0Index);
                DumpClientTraitChecksum(kTestATraitSink1Index);
            }
            break;
    }
}
void MockWdmSubscriptionInitiatorImpl::DumpPublisherTraits(void)
{
    switch (mTestCaseId)
    {
        case kTestCase_IntegrationTrait:
        case kTestCase_RejectIncomingSubscribeRequest:
            DumpPublisherTraitChecksum(kLocaleCapabilitiesSourceIndex);
            break;
        case kTestCase_TestTrait:
            DumpPublisherTraitChecksum(kTestATraitSource0Index);
            DumpPublisherTraitChecksum(kTestATraitSource1Index);
            DumpPublisherTraitChecksum(kTestBTraitSourceIndex);
            break;
        case kTestCase_TestOversizeTrait1:
        case kTestCase_TestOversizeTrait2:
            DumpPublisherTraitChecksum(kTestATraitSource0Index);
            DumpPublisherTraitChecksum(kTestATraitSource1Index);
            break;
    }
}

void MockWdmSubscriptionInitiatorImpl::EngineEventCallback (void * const aAppState,
    SubscriptionEngine::EventID aEvent,
    const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);
    switch (aEvent)
    {
    case SubscriptionEngine::kEvent_OnIncomingSubscribeRequest:
        WeaveLogDetail(DataManagement, "Engine->kEvent_OnIncomingSubscribeRequest peer = 0x%" PRIX64, aInParam.mIncomingSubscribeRequest.mEC->PeerNodeId);
        aOutParam.mIncomingSubscribeRequest.mHandlerAppState = initiator;
        aOutParam.mIncomingSubscribeRequest.mHandlerEventCallback = MockWdmSubscriptionInitiatorImpl::PublisherEventCallback;
        aOutParam.mIncomingSubscribeRequest.mRejectRequest = false;

        aInParam.mIncomingSubscribeRequest.mBinding->SetDefaultResponseTimeout(kResponseTimeoutMsec);
        aInParam.mIncomingSubscribeRequest.mBinding->SetDefaultWRMPConfig(gWRMPConfig);

        break;
    default:
        SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

void MockWdmSubscriptionInitiatorImpl::AddNewVersion(int aTraitDataSinkIndex)
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

void MockWdmSubscriptionInitiatorImpl::Cleanup()
{
    if (NULL != mSubscriptionClient)
    {
        mSubscriptionClient->Free();
        mSubscriptionClient = NULL;
    }

    if (NULL != mBinding)
    {
        mBinding->Release();
        mBinding = NULL;
    }
}

void MockWdmSubscriptionInitiatorImpl::PrintVersionsLog()
{
    for (int i = 0; i< kMaxNumTraitHandles; i++)
    {
        VersionNode * pre = &mTraitVersionSet[i];
        VersionNode * curr = mTraitVersionSet[i].next;

        printf("Initiator's trait %u versions log is : %" PRIu64, i, pre->versionInfo);
        while (curr != NULL)
        {
            pre = curr;
            curr = curr->next;
            printf(" ==> %" PRIu64, pre->versionInfo);
        }
        printf("\n");
    }
}

void MockWdmSubscriptionInitiatorImpl::ClearDataSinkIterator(void *aTraitInstance, TraitDataHandle aHandle, void *aContext)
{
    MockTraitDataSink *traitInstance = static_cast<MockTraitDataSink *>(aTraitInstance);
    traitInstance->ResetDataSink();
}

void MockWdmSubscriptionInitiatorImpl::ClearDataSinkState(void)
{
    mSinkCatalog.Iterate(MockWdmSubscriptionInitiatorImpl::ClearDataSinkIterator, NULL);
    mClearDataSink = true;
}

void MockWdmSubscriptionInitiatorImpl::ClientEventCallback (void * const aAppState, SubscriptionClient::EventID aEvent,
    const SubscriptionClient::InEventParam & aInParam, SubscriptionClient::OutEventParam & aOutParam)
{
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);

    switch (aEvent)
    {
    case SubscriptionClient::kEvent_OnExchangeStart:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnExchangeStart");
        break;
    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded");
        if (initiator->mTestCaseId >= kTestCase_CompatibleVersionedRequest && initiator->mTestCaseId <= kTestCase_IncompatibleVersionedRequest) {
            aOutParam.mSubscribeRequestPrepareNeeded.mVersionedPathList = initiator->mVersionedTraitPaths;
        }
        else {
            aOutParam.mSubscribeRequestPrepareNeeded.mPathList = initiator->mTraitPaths;
        }

        aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize = initiator->mNumPaths;
        aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = true;
        aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
        aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
        aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin = gMinimumTimeBetweenLivenessCheckSec;
        aOutParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax = 3600;
        break;

    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionEstablished");
        WeaveLogDetail(DataManagement, "Liveness Timeout: %u msec", aInParam.mSubscriptionEstablished.mClient->GetLivenessTimeoutMsec());
        if (gIsMutualSubscription == false)
        {
            gOnewaySubscriptionEstablished = true;

            if (gNumDataChangeBeforeCancellation != 0)
            {
                initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, initiator);
            }
            else
            {
                if (gFinalStatus != kIdle)
                {
                    switch (gFinalStatus)
                    {
                    case kPublisherCancel:
                    case kPublisherAbort:
                        initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, initiator);
                        break;
                    case kClientCancel:
                    case kClientAbort:
                        initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, initiator);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        break;
    case SubscriptionClient::kEvent_OnNotificationRequest:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationRequest");
        break;
    case SubscriptionClient::kEvent_OnNotificationProcessed:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnNotificationProcessed");

        switch (initiator->mTestCaseId)
        {
        case kTestCase_IntegrationTrait:
        case kTestCase_RejectIncomingSubscribeRequest:
            initiator->AddNewVersion(initiator->kLocaleSettingsSinkIndex);
            initiator->AddNewVersion(initiator->kApplicationKeysTraitSinkIndex);
            break;
        case kTestCase_TestTrait:
            initiator->AddNewVersion(initiator->kTestATraitSink0Index);
            initiator->AddNewVersion(initiator->kTestATraitSink1Index);
            initiator->AddNewVersion(initiator->kTestBTraitSinkIndex);
            initiator->AddNewVersion(initiator->kLocaleSettingsSinkIndex);
            break;
        case kTestCase_TestUpdatableTraits:
            break;
        case kTestCase_TestOversizeTrait1:
            initiator->AddNewVersion(initiator->kTestATraitSink0Index);
            initiator->AddNewVersion(initiator->kTestATraitSink1Index);
            initiator->AddNewVersion(initiator->kLocaleSettingsSinkIndex);
            break;
       case kTestCase_TestOversizeTrait2:
            if (gTestCase_TestOversizeTrait2DumpFlip)
            {
                initiator->AddNewVersion(initiator->kLocaleSettingsSinkIndex);
            }
            else
            {
                initiator->AddNewVersion(initiator->kTestATraitSink0Index);
                initiator->AddNewVersion(initiator->kTestATraitSink1Index);
            }
            break;
        }

        initiator->DumpClientTraits();

        if (initiator->mTestCaseId == kTestCase_TestOversizeTrait2)
        {
            gTestCase_TestOversizeTrait2DumpFlip = !gTestCase_TestOversizeTrait2DumpFlip;
        }

        break;
    case SubscriptionClient::kEvent_OnSubscriptionTerminated:
        WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated. Reason: %u, peer = 0x%" PRIX64 "\n",
                aInParam.mSubscriptionTerminated.mReason,
                aInParam.mSubscriptionTerminated.mClient->GetPeerNodeId());

        initiator->mWillRetry = aInParam.mSubscriptionTerminated.mWillRetry;

        switch (gFinalStatus)
        {
        case kPublisherCancel:
        case kPublisherAbort:
            initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorPublisherCurrentState, initiator);
            break;
        case kClientCancel:
        case kClientAbort:
            initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorClientCurrentState, initiator);
            break;
        case kIdle:
        default:
            break;
        }

        if (initiator->mEnableRetry == false || initiator->mWillRetry == false)
        {
            gInitiatorState.mDataflipCount = 0;

            if (gEvaluateSuccessIteration == true)
            {
                WeaveLogDetail(DataManagement, "Mutual: Good Iteration");
                gEvaluateSuccessIteration = false;
            }
            if (gNumDataChangeBeforeCancellation != 0)
            {
                initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleDataFlipTimeout, initiator);
            }
            initiator->onCompleteTest();
        }
        break;
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    case SubscriptionClient::kEvent_OnUpdateComplete:
        if ((aInParam.mUpdateComplete.mReason == WEAVE_NO_ERROR) && (nl::Weave::Profiles::Common::kStatus_Success == aInParam.mUpdateComplete.mStatusCode))
        {
            WeaveLogDetail(DataManagement, "Update: path result: success");
        }
        else
        {
            WeaveLogDetail(DataManagement, "Update: path failed: %s, %s",
                           ErrorStr(aInParam.mUpdateComplete.mReason),
                           nl::StatusReportStr(aInParam.mUpdateComplete.mStatusProfileId, aInParam.mUpdateComplete.mStatusCode));
        }
        break;
    case SubscriptionClient::kEvent_OnNoMorePendingUpdates:
        WeaveLogDetail(DataManagement, "Update: no more pending updates");
        break;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    default:
        SubscriptionClient::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

void MockWdmSubscriptionInitiatorImpl::PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam, SubscriptionHandler::OutEventParam & aOutParam)
{
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aEvent)
    {
    case SubscriptionHandler::kEvent_OnSubscribeRequestParsed:
        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscribeRequestParsed");

        // ideally this number should be set to something for cloud service, and something else for everyone else
        // we can potentially copy this from the client side, but it would take considerable amount of code to be generic enough
        // setting to some constant here seems to be easier

        aInParam.mSubscribeRequestParsed.mHandler->GetBinding()->SetDefaultResponseTimeout(kResponseTimeoutMsec);
        aInParam.mSubscribeRequestParsed.mHandler->GetBinding()->SetDefaultWRMPConfig(gWRMPConfig);

        if (NULL != initiator->mSubscriptionClient)
        {
            if (aInParam.mSubscribeRequestParsed.mIsSubscriptionIdValid)
            {
                uint64_t subscriptionId;
                err = initiator->mSubscriptionClient->GetSubscriptionId(&subscriptionId);
                SuccessOrExit(err);

                // subscription ID is largely peer-specific
                if ((aInParam.mSubscribeRequestParsed.mEC->PeerNodeId == initiator->mBinding->GetPeerNodeId()) &&
                    (aInParam.mSubscribeRequestParsed.mSubscriptionId == subscriptionId))
                {
                    WeaveLogDetail(DataManagement, "Request for mutual subscription found");
                }
            }
        }

        // AcceptSubscribeRequest and EndSubscription may be used either sync or async, to move the state machine forward
        aInParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest();

        break;

    case SubscriptionHandler::kEvent_OnExchangeStart:
        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnExchangeStart");
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionEstablished:
        if (true == mClearDataSink || true == gCleanStatus)
        {
            initiator->DumpPublisherTraits();
            gCleanStatus = false;
        }

        WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscriptionEstablished");
        gMutualSubscriptionEstablished = true;
        gSubscriptionHandler = aInParam.mSubscriptionEstablished.mHandler;
        if (gNumDataChangeBeforeCancellation != 0)
        {
            initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, initiator);
        }
        else
        {
            if (gFinalStatus != kIdle)
            {
                switch (gFinalStatus)
                {
                case kPublisherCancel:
                case kPublisherAbort:
                    initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, initiator);
                    break;
                case kClientCancel:
                case kClientAbort:
                    initiator->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, initiator);
                    break;
                default:
                    break;
                }
            }
        }
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
        WeaveLogDetail(DataManagement, "Pub: kEvent_OnSubscriptionTerminated, Reason = %d, peer = 0x%" PRIX64 "\n",
                aInParam.mSubscriptionTerminated.mReason,
                aInParam.mSubscriptionTerminated.mHandler->GetPeerNodeId());
        switch (gFinalStatus)
        {
        case kPublisherCancel:
        case kPublisherAbort:
            initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorPublisherCurrentState, initiator);
            break;
        case kClientCancel:
        case kClientAbort:
            initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(MonitorClientCurrentState, initiator);
            break;
        case kIdle:
        default:
            break;
        }

        if (gNumDataChangeBeforeCancellation != 0)
        {
            initiator->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleDataFlipTimeout, initiator);
        }

        if (initiator->mEnableRetry == false || initiator->mWillRetry == false)
        {
            HandlePublisherRelease();
            if (gEvaluateSuccessIteration == true)
            {
                WeaveLogDetail(DataManagement, "Mutual: Good Iteration");
                gEvaluateSuccessIteration = false;
            }
            gMutualSubscriptionEstablished = false;
            initiator->onCompleteTest();
        }
        break;

    default:
        SubscriptionHandler::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }

exit:
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionInitiatorImpl::HandleClientComplete(void *aAppState)
{
    WEAVE_ERROR err;
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);

    if (gIsMutualSubscription == true)
    {
        gEvaluateSuccessIteration = true;
        initiator->mWillRetry = false;
    }

    if (NULL != initiator->mSubscriptionClient)
    {
        if (gFinalStatus == kClientCancel)
        {
            err = initiator->mSubscriptionClient->EndSubscription();
            if (err != WEAVE_NO_ERROR)
            {
                initiator->mSubscriptionClient->AbortSubscription();
            }
        }
        if (gFinalStatus == kClientAbort)
        {
            (void)initiator->mSubscriptionClient->AbortSubscription();
        }
    }

    gInitiatorState.mDataflipCount = 0;
}

void MockWdmSubscriptionInitiatorImpl::HandlePublisherComplete()
{

    if (gIsMutualSubscription == true)
    {
        gEvaluateSuccessIteration = true;
    }

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

void MockWdmSubscriptionInitiatorImpl::HandlePublisherRelease()
{
    gSubscriptionHandler = NULL;
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
WEAVE_ERROR MockWdmSubscriptionInitiatorImpl::ApplyWdmUpdateMutations()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int tmp;
    MockWdmSubscriptionInitiatorImpl *initiator = this;
    MockWdmNodeOptions::WdmUpdateConditionality conditionality = initiator->mUpdateConditionality;
    static bool testATraitConditional = true;
    static bool otherTraitsConditional = true;

    switch (conditionality)
    {
        case MockWdmNodeOptions::kConditionality_Conditional:
            testATraitConditional = true;
            otherTraitsConditional = true;
            break;
        case MockWdmNodeOptions::kConditionality_Unconditional:
            testATraitConditional = false;
            otherTraitsConditional = false;
            break;
        case MockWdmNodeOptions::kConditionality_Mixed:
            testATraitConditional = true;
            otherTraitsConditional = ! testATraitConditional;
            break;
        case MockWdmNodeOptions::kConditionality_Alternate:
            testATraitConditional = ! testATraitConditional;
            otherTraitsConditional = ! testATraitConditional;
            break;
        default:
            WeaveDie();
            break;
    }

    for (uint32_t i = 0; i < initiator->mUpdateNumMutations; i++)
    {
        WeaveLogDetail(DataManagement, "Mutation %u of %u; %u trait instances",
                i+1, initiator->mUpdateNumMutations, initiator->mUpdateNumTraits);
        switch (initiator->mUpdateNumTraits)
        {
            case 4:
                err = initiator->mTestATraitUpdatableDataSink1.Mutate(initiator->mSubscriptionClient, otherTraitsConditional, initiator->mUpdateMutation);
                SuccessOrExit(err);
            case 3:
                err = initiator->mTestBTraitUpdatableDataSink.Mutate(initiator->mSubscriptionClient, otherTraitsConditional, initiator->mUpdateMutation);
                SuccessOrExit(err);
            case 2:
                err = initiator->mLocaleSettingsTraitUpdatableDataSink.Mutate(initiator->mSubscriptionClient, otherTraitsConditional, initiator->mUpdateMutation);
                SuccessOrExit(err);
            case 1:
                err = initiator->mTestATraitUpdatableDataSink0.Mutate(initiator->mSubscriptionClient, testATraitConditional, initiator->mUpdateMutation);
                SuccessOrExit(err);
                break;
            default:
                WeaveDie();
                break;
        }
        err = initiator->mSubscriptionClient->FlushUpdate();

        initiator->mUpdateSameMutationCounter++;
        if (initiator->mUpdateSameMutationCounter == initiator->mUpdateNumRepeatedMutations)
        {
            initiator->mUpdateSameMutationCounter = 0;
            tmp = initiator->mUpdateMutation;
            tmp = (tmp + 1) % MockWdmNodeOptions::kMutation_NumItems;
            initiator->mUpdateMutation = static_cast<MockWdmNodeOptions::WdmUpdateMutation>(tmp);
        }

        SuccessOrExit(err);
    }

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

void MockWdmSubscriptionInitiatorImpl::HandleDataFlipTimeout(nl::Weave::System::Layer* aSystemLayer, void *aAppState,
    nl::Weave::System::Error aErr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);

    IgnoreUnusedVariable(aErr);

    if (gIsMutualSubscription == true && gMutualSubscriptionEstablished == false)
    {
        WeaveLogDetail(DataManagement, "mutual subscription cannot be established, and do nothing until response timeout happens");
        return;
    }

    if (gIsMutualSubscription == false && gOnewaySubscriptionEstablished == false)
    {
        WeaveLogDetail(DataManagement, "one way subscription cannot be established, and do nothing until response timeout happens");
        return;
    }

    ++gInitiatorState.mDataflipCount;

    if (gNumDataChangeBeforeCancellation != -1 && gInitiatorState.mDataflipCount > gNumDataChangeBeforeCancellation)
    {
        if (gIsMutualSubscription)
        {
            switch (gFinalStatus)
            {
                case kPublisherCancel:
                case kPublisherAbort:
                    aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, initiator);
                    break;
                case kClientCancel:
                case kClientAbort:
                    aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, initiator);
                    break;
            }
        }
        else
        {
            aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, initiator);
        }
        WeaveLogDetail(DataManagement, "No more data flips; started the MonitorClientCurrentState timer", gInitiatorState.mDataflipCount, gNumDataChangeBeforeCancellation);
        ExitNow();
    }
    else
    {
        // alter data every gTimeBetweenDataChangeMsec milliseconds
        WeaveLogDetail(DataManagement, "Cycle %d of %d", gInitiatorState.mDataflipCount, gNumDataChangeBeforeCancellation);
        WeaveLogDetail(DataManagement, "Starting timer for the next cycle");
        aSystemLayer->StartTimer(gTimeBetweenDataChangeMsec, HandleDataFlipTimeout, initiator);
    }

    if (gIsMutualSubscription == true && gEnableDataFlip == true) {
        WeaveLogDetail(DataManagement, "\n\n\n\n\nFlipping data...");

        switch (initiator->mTestCaseId)
        {
        case kTestCase_IntegrationTrait:
        case kTestCase_RejectIncomingSubscribeRequest:
            initiator->mLocaleCapabilitiesDataSource.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;
        case kTestCase_TestTrait:
            initiator->mTestATraitDataSource0.Mutate();
            initiator->mTestATraitDataSource1.Mutate();
            initiator->mTestBTraitDataSource.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
        case kTestCase_TestUpdatableTraits:
        {
            err = initiator->ApplyWdmUpdateMutations();
            SuccessOrExit(err);
            break;
        }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

        case kTestCase_TestOversizeTrait1:
        case kTestCase_TestOversizeTrait2:
            initiator->mTestATraitDataSource0.Mutate();
            initiator->mTestATraitDataSource1.Mutate();
            SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
            break;
        }
        initiator->DumpPublisherTraits();
    }

exit:
    WeaveLogFunctError(err);
}

void MockWdmSubscriptionInitiatorImpl::MonitorPublisherCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr)
{
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);
    if (NULL != gSubscriptionHandler)
    {
        if (initiator->mSubscriptionClient->IsEstablishedIdle() && gSubscriptionHandler->IsEstablishedIdle())
        {
            WeaveLogDetail(DataManagement, "state transitions to idle within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
            gInitiatorState.mPublisherStateCount = 1;
            HandlePublisherComplete();
        }
        else
        {
            if (gInitiatorState.mPublisherStateCount < kMonitorCurrentStateCnt)
            {
                gInitiatorState.mPublisherStateCount ++;
                aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorPublisherCurrentState, initiator);
            }
            else
            {
                gInitiatorState.mPublisherStateCount = 1;
                WeaveLogDetail(DataManagement, "state is not idle or aborted within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
                (void)initiator->mSubscriptionClient->AbortSubscription();
                HandlePublisherRelease();
                initiator->onCompleteTest();
            }
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "gSubscriptionHandler is NULL, and current session is torn down");
        (void)initiator->mSubscriptionClient->AbortSubscription();
        HandlePublisherRelease();
        initiator->onCompleteTest();
    }

}

void MockWdmSubscriptionInitiatorImpl::MonitorClientCurrentState (nl::Weave::System::Layer* aSystemLayer, void *aAppState, INET_ERROR aErr)
{
    MockWdmSubscriptionInitiatorImpl * const initiator = reinterpret_cast<MockWdmSubscriptionInitiatorImpl *>(aAppState);
    if (NULL != initiator->mSubscriptionClient)
    {
        if (initiator->mSubscriptionClient->IsEstablishedIdle() && (gIsMutualSubscription == false || gSubscriptionHandler->IsEstablishedIdle()))
        {
            WeaveLogDetail(DataManagement, "state transitions to idle within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
            gInitiatorState.mClientStateCount = 1;
            HandleClientComplete(initiator);

            if (gIsMutualSubscription == false)
            {
                WeaveLogDetail(DataManagement, "One_way: Good Iteration");
                initiator->onCompleteTest();
            }
        }
        else
        {
            if (gInitiatorState.mClientStateCount < kMonitorCurrentStateCnt)
            {
                WeaveLogDetail(DataManagement, "state is not idle or aborted yet; count: %d", gInitiatorState.mClientStateCount);
                gInitiatorState.mClientStateCount++;
                aSystemLayer->StartTimer(kMonitorCurrentStateInterval, MonitorClientCurrentState, initiator);
            }
            else
            {
                gInitiatorState.mClientStateCount = 1;
                WeaveLogDetail(DataManagement, "state is not idle or aborted within %d msec", kMonitorCurrentStateInterval * kMonitorCurrentStateCnt);
                (void)initiator->mSubscriptionClient->AbortSubscription();
                HandlePublisherRelease();
                initiator->onCompleteTest();

            }
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "mSubscriptionClient is NULL, and current session is torn down");
        HandlePublisherRelease();
        initiator->onCompleteTest();
    }
}
} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

static nl::Weave::Profiles::DataManagement::MockWdmSubscriptionInitiatorImpl gWdmSubscriptionInitiator;
MockWdmSubscriptionInitiator * MockWdmSubscriptionInitiator::GetInstance(void)
{
    return &gWdmSubscriptionInitiator;
}

uint32_t MockWdmSubscriptionInitiator::GetNumUpdatableTraits(void)
{
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    return 4;
#else
    return 0;
#endif
}


#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements unit-tests for the protocol side of WDM.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveCircularTLVBuffer.h>
#include <Weave/Support/RandUtils.h>
#include <SystemLayer/SystemFaultInjection.h>

#include <Weave/Profiles/service-directory/ServiceDirectory.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include "MockPlatformClocks.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/init.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;


namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // for unit tests, the dummy critical section is sufficient.
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }
} // Platform
} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

static SubscriptionEngine *gSubscriptionEngine;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return gSubscriptionEngine;
}

static void TestCounterSubscription_BufferAllocFailure(nlTestSuite *inSuite, void *inContext);

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Test Counter Subscription -- Buffer Allocation Failure", TestCounterSubscription_BufferAllocFailure),

    NL_TEST_SENTINEL()
};

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class TestWdm {
public:
    TestWdm();

    int Setup();
    int Teardown();
    int Reset();
    int BuildAndProcessNotify();

    void TestCounterSubscription_BufferAllocFailure(nlTestSuite *inSuite);
    void SpoofPublisherSubscription();

    static void ClientSubscriptionEventCallback(void * const aAppState,
                                        SubscriptionClient::EventID aEvent,
                                        const SubscriptionClient::InEventParam & aInParam,
                                        SubscriptionClient::OutEventParam & aOutParam);

    static void BindingEventCallback(void * const apAppState, const nl::Weave::Binding::EventType aEventType,
                                            const nl::Weave::Binding::InEventParam & aInParam,
                                            nl::Weave::Binding::OutEventParam & aOutParam);

    static void PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam,
        SubscriptionHandler::OutEventParam & aOutParam);

private:
    SubscriptionHandler *mSubHandler;
    SubscriptionClient *mSubClient;
    NotificationEngine *mNotificationEngine;

    SubscriptionEngine mSubscriptionEngine;
    WeaveExchangeManager mExchangeMgr;

    SingleResourceSourceTraitCatalog::CatalogItem mSourceCatalogStore[4];
    SingleResourceSourceTraitCatalog mSourceCatalog;
    SingleResourceSinkTraitCatalog::CatalogItem mSinkCatalogStore[4];
    SingleResourceSinkTraitCatalog mSinkCatalog;

    Binding *mClientBinding;
    uint64_t mPeerSubscriptionId;

    uint32_t mTestCase;
    bool mPublisherSubscriptionPresent;
    bool mClientSubscriptionPresent;
};

TestWdm *gTestWdm;

TestWdm::TestWdm()
    : mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, 4),
      mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, 4),
      mClientBinding(NULL)
{
    mTestCase = 0;
}

void TestWdm::SpoofPublisherSubscription()
{
    mSubHandler->mRefCount = 1;
    mSubHandler->mLivenessTimeoutMsec = 2000;
    mSubHandler->mSubscriptionId = 1;
    mPeerSubscriptionId = 1;
    mSubHandler->mCurrentState = SubscriptionHandler::kState_SubscriptionEstablished_Idle;
}

void
TestWdm::BindingEventCallback(void * const apAppState, const nl::Weave::Binding::EventType aEventType,
                                            const nl::Weave::Binding::InEventParam & aInParam,
                                            nl::Weave::Binding::OutEventParam & aOutParam)
{
    TestWdm *_this = static_cast<TestWdm*>(apAppState);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aEventType) {
        case Binding::kEvent_PrepareRequested:
        {
            err = _this->mClientBinding->BeginConfiguration()
                .Target_ServiceEndpoint(kServiceEndpoint_Data_Management)
                .TargetAddress_WeaveService()
                .Transport_UDP_WRM()
                .Security_None()
                .PrepareBinding();
            SuccessOrExit(err);
            break;
        }

        case Binding::kEvent_BindingReady:
        {
            break;
        }

        case Binding::kEvent_PrepareFailed:
        {
            break;
        }

        case Binding::kEvent_BindingFailed:
            break;

        default:
            // Fall through.

        case Binding::kEvent_DefaultCheck:
            Binding::DefaultEventHandler(apAppState, aEventType, aInParam, aOutParam);
            break;
    }

exit:
    return;
}

void
TestWdm::ClientSubscriptionEventCallback (void * const aAppState,
                                        SubscriptionClient::EventID aEvent,
                                        const SubscriptionClient::InEventParam & aInParam,
                                        SubscriptionClient::OutEventParam & aOutParam)
{
    TestWdm *_this = static_cast<TestWdm*>(aAppState);

    switch (aEvent) {
        case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
            {
                WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscribeRequestPrepareNeeded\n");

                aOutParam.mSubscribeRequestPrepareNeeded.mPathList = NULL;
                aOutParam.mSubscribeRequestPrepareNeeded.mPathListSize = 0;
                aOutParam.mSubscribeRequestPrepareNeeded.mSubscriptionId = _this->mPeerSubscriptionId;

                aOutParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = false;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
                aOutParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
                break;
            }

        case SubscriptionClient::kEvent_OnSubscriptionTerminated:
            {
                WeaveLogDetail(DataManagement, "Client->kEvent_OnSubscriptionTerminated\n");
                _this->mClientSubscriptionPresent = false;
                break;
            }

        default:
            break;
    }
}

void
TestWdm::PublisherEventCallback (void * const aAppState,
        SubscriptionHandler::EventID aEvent, const SubscriptionHandler::InEventParam & aInParam,
        SubscriptionHandler::OutEventParam & aOutParam)
{
    TestWdm *_this = static_cast<TestWdm *>(aAppState);

    switch (aEvent)
    {
        case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
        {
            WeaveLogDetail(DataManagement, "Publisher->kEvent_OnSubscriptionTerminated\n");
            _this->mPublisherSubscriptionPresent = false;
        }

        default:
            break;
    }
}

int TestWdm::Setup()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    gSubscriptionEngine = &mSubscriptionEngine;

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);

    // Initialize SubEngine and set it up
    err = mSubscriptionEngine.Init(&ExchangeMgr, NULL, NULL);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    // Get a sub handler and prime it to the right state
    err = mSubscriptionEngine.NewSubscriptionHandler(&mSubHandler);
    SuccessOrExit(err);

    mSubHandler->mBinding = ExchangeMgr.NewBinding();
    mSubHandler->mBinding->BeginConfiguration().Transport_UDP().Target_NodeId(kServiceEndpoint_Data_Management);

    mSubHandler->mAppState = gTestWdm;
    mSubHandler->mEventCallback = PublisherEventCallback;

    mClientBinding = ExchangeMgr.NewBinding(BindingEventCallback, gTestWdm);

    err = mSubscriptionEngine.NewClient(&mSubClient, mClientBinding, gTestWdm, TestWdm::ClientSubscriptionEventCallback, &mSinkCatalog, 0);
    SuccessOrExit(err);

    mNotificationEngine = &mSubscriptionEngine.mNotificationEngine;

exit:
    if (err != WEAVE_NO_ERROR) {
        WeaveLogError(DataManagement, "Error setting up test: %d", err);
    }

    return err;
}

int TestWdm::Teardown()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mClientBinding != NULL)
    {
        mClientBinding->Release();
        mClientBinding = NULL;
    }

    return err;
}

int TestWdm::Reset()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mSubHandler->MoveToState(SubscriptionHandler::kState_SubscriptionEstablished_Idle);
    mNotificationEngine->mGraphSolver.ClearDirty();

    return err;
}

int TestWdm::BuildAndProcessNotify()
{
    bool isSubscriptionClean;
    NotificationEngine::NotifyRequestBuilder notifyRequest;
    NotificationRequest::Parser notify;
    PacketBuffer *buf = NULL;
    TLVWriter writer;
    TLVReader reader;
    TLVType dummyType1, dummyType2;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool neWriteInProgress = false;
    uint32_t maxNotificationSize = 0;
    uint32_t maxPayloadSize = 0;

    maxNotificationSize = mSubHandler->GetMaxNotificationSize();

    err = mSubHandler->mBinding->AllocateRightSizedBuffer(buf, maxNotificationSize, WDM_MIN_NOTIFICATION_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    err = notifyRequest.Init(buf, &writer, mSubHandler, maxPayloadSize);
    SuccessOrExit(err);

    err = mNotificationEngine->BuildSingleNotifyRequestDataList(mSubHandler, notifyRequest, isSubscriptionClean, neWriteInProgress);
    SuccessOrExit(err);

    if (neWriteInProgress)
    {
        err = notifyRequest.MoveToState(NotificationEngine::kNotifyRequestBuilder_Idle);
        SuccessOrExit(err);

        reader.Init(buf);

        err = reader.Next();
        SuccessOrExit(err);

        notify.Init(reader);

        err = notify.CheckSchemaValidity();
        SuccessOrExit(err);

        // Enter the struct
        err = reader.EnterContainer(dummyType1);
        SuccessOrExit(err);

        // SubscriptionId
        err = reader.Next();
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.EnterContainer(dummyType2);
        SuccessOrExit(err);

        err = mSubClient->ProcessDataList(reader);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "nothing has been written");
    }

exit:
    if (buf) {
        PacketBuffer::Free(buf);
    }

    return err;
}

void TestWdm::TestCounterSubscription_BufferAllocFailure(nlTestSuite *inSuite)
{
    Reset();

    // This spoofs a publisher-side subscription with SubscriptionId = 1
    SpoofPublisherSubscription();

    mPublisherSubscriptionPresent = true;
    mClientSubscriptionPresent = true;

    // Trigger a packet buffer fault so that the ensuing counter subscription request fails
    // because it cannot allocate a packet buffer.
    nl::Weave::System::FaultInjection::GetManager().FailAtFault(
            nl::Weave::System::FaultInjection::kFault_PacketBufferNew,
            0, 1);

    // Initiate a counter subscription request - this should trigger
    mSubClient->InitiateCounterSubscription(1000);

    // Ensure both client and publisher subscriptions are terminated.
    NL_TEST_ASSERT(inSuite, mClientSubscriptionPresent == false);
    NL_TEST_ASSERT(inSuite, mPublisherSubscriptionPresent == false);
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    static TestWdm testWdm;
    gTestWdm = &testWdm;

    return testWdm.Setup();
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return gTestWdm->Teardown();
}

static void TestCounterSubscription_BufferAllocFailure(nlTestSuite *inSuite, void *inContext)
{
    gTestWdm->TestCounterSubscription_BufferAllocFailure(inSuite);
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    nlTestSuite theSuite = {
        "weave-wdm",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

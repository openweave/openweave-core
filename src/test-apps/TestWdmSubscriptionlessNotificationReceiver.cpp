/*
 *
 *    Copyright (c) 2017-2018 Nest Labs, Inc.
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
 *      This file implements the Weave Data Management subscriptionless notification receiver.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/WeaveVersion.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>

#include <time.h>

#include "TestWdmSubscriptionlessNotification.h"

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

using nl::Weave::System::PacketBuffer;

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

#define TOOL_NAME "TestWdmSubscriptionlessNotificationReceiver"


static TestWdmSubscriptionlessNotificationReceiver gTestWdmSubscriptionlessNotificationReceiver;

TestWdmSubscriptionlessNotificationReceiver * TestWdmSubscriptionlessNotificationReceiver::GetInstance ()
{
    return &gTestWdmSubscriptionlessNotificationReceiver;
}

TestWdmSubscriptionlessNotificationReceiver::TestWdmSubscriptionlessNotificationReceiver() :
    mExchangeMgr(NULL),
    mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0]))
{
}

WEAVE_ERROR TestWdmSubscriptionlessNotificationReceiver::Init(WeaveExchangeManager *aExchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TraitDataHandle traitHandle;
    mExchangeMgr = aExchangeMgr;

    // Make some of the Data sinks accept subscriptionless notifications

    mTestATraitDataSink0 = TestATraitDataSink(true);
    mTestATraitDataSink1 = TestATraitDataSink(true);
    mTestATraitDataSink2 = TestATraitDataSink(true);

    // Add the sinks to the catalog

    mSinkCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSink0, traitHandle);
    mSinkCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSink1, traitHandle);
    mSinkCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSink2, traitHandle);

    // Initialize the SubscriptionEngine

    err = SubscriptionEngine::GetInstance()->Init(mExchangeMgr, this, EngineEventCallback);
    SuccessOrExit(err);

    // Register catalog for subscriptionless notifications

    SubscriptionEngine::GetInstance()->RegisterForSubscriptionlessNotifications(&mSinkCatalog);

exit:

    return err;
}

void TestWdmSubscriptionlessNotificationReceiver::EngineEventCallback (void * const aAppState,
    SubscriptionEngine::EventID aEvent,
    const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    TestWdmSubscriptionlessNotificationReceiver *sublessNotifyReceiver =
        static_cast<TestWdmSubscriptionlessNotificationReceiver*>(aAppState);

    switch (aEvent)
    {
        case SubscriptionEngine::kEvent_OnIncomingSubscriptionlessNotification:
            WeaveLogDetail(DataManagement, "Received Subscriptionless Notification from Node: %016" PRIX64 "\n",
                           aInParam.mIncomingSubscriptionlessNotification.mMsgInfo->SourceNodeId);
            aOutParam.mIncomingSubscriptionlessNotification.mShouldContinueProcessing = true;
        break;
        case SubscriptionEngine::kEvent_DataElementAccessControlCheck:
            aOutParam.mDataElementAccessControlForNotification.mRejectNotification = false;
            aOutParam.mDataElementAccessControlForNotification.mReason = WEAVE_NO_ERROR;
        break;
        case SubscriptionEngine::kEvent_SubscriptionlessNotificationProcessingComplete:
            WeaveLogDetail(DataManagement, "Subscriptionless Notification Processing complete\n");
            if (aInParam.mIncomingSubscriptionlessNotification.processingError ==
                WEAVE_ERROR_WDM_SUBSCRIPTIONLESS_NOTIFY_PARTIAL)
            {
                sublessNotifyReceiver->OnError();
                WeaveLogDetail(DataManagement, "Subscriptionless Notification Processing Failure\n");
            }
            else
            {
                WeaveLogDetail(DataManagement, "kEvent_SubscriptionlessNotificationProcessingComplete");

                WeaveLogDetail(DataManagement, "Subscriptionless Notification Processing Success\n");
                sublessNotifyReceiver->OnTestComplete();
            }
        break;
        default:
            SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

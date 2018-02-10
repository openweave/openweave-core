/*
 *
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
 *      This file implements the Weave Data Management subscriptionless notification sender.
 *
 */

#define WEAVE_CONFIG_ENABLE_FUNCT_ERROR_LOGGING 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/ASN1.h>
#include "MockSourceTraits.h"

#include "TestWdmSubscriptionlessNotification.h"

#define TOOL_NAME "TestWdmSubscriptionlessNotificationSender"

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;

using nl::Weave::System::PacketBuffer;

static TestWdmSubscriptionlessNotificationSender gTestWdmSubscriptionlessNotificationSender;

 TestWdmSubscriptionlessNotificationSender * TestWdmSubscriptionlessNotificationSender::GetInstance ()
{
    return &gTestWdmSubscriptionlessNotificationSender;
}

TestWdmSubscriptionlessNotificationSender::TestWdmSubscriptionlessNotificationSender() :
    mSourceCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSourceCatalogStore, sizeof(mSourceCatalogStore) / sizeof(mSourceCatalogStore[0]))
{
}

void TestWdmSubscriptionlessNotificationSender::BindingEventCallback (void * const apAppState,
                                                       const Binding::EventType aEvent,
                                                       const Binding::InEventParam & aInParam,
                                                       Binding::OutEventParam & aOutParam)
{
    switch (aEvent)
    {

    default:
        Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }
}

WEAVE_ERROR TestWdmSubscriptionlessNotificationSender::Init (nl::Weave::WeaveExchangeManager *aExchangeMgr,
                                                             const uint16_t destSubnetId,
                                                             const uint64_t destNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "TestWdmSubscriptionlessNotificationSender Init");

    VerifyOrExit(nl::Weave::kWeaveSubnetId_NotSpecified != destSubnetId, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mSourceCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSource0, mTraitPaths[kTestATraitSource0Index].mTraitDataHandle);
    mSourceCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSource1, mTraitPaths[kTestATraitSource1Index].mTraitDataHandle);
    mSourceCatalog.Add(TEST_TRAIT_INSTANCE_ID, &mTestATraitDataSource2, mTraitPaths[kTestATraitSource2Index].mTraitDataHandle);

    mNumPaths = 3;
    mExchangeMgr = aExchangeMgr;

    err = SubscriptionEngine::GetInstance()->Init(mExchangeMgr, this, EngineEventCallback);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &mSourceCatalog);
    SuccessOrExit(err);

    mBinding = mExchangeMgr->NewBinding(BindingEventCallback, this);
    VerifyOrExit(NULL != mBinding, err = WEAVE_ERROR_NO_MEMORY);

    err = mBinding->BeginConfiguration()
               .Transport_UDP()
               .Target_NodeId(destNodeId)
               .Security_None()
               .TargetAddress_WeaveFabric(destSubnetId)
               .PrepareBinding();
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR && mBinding)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    return err;
}

WEAVE_ERROR TestWdmSubscriptionlessNotificationSender::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (NULL != mBinding)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    return err;
}

void TestWdmSubscriptionlessNotificationSender::EngineEventCallback (void * const aAppState,
    SubscriptionEngine::EventID aEvent,
    const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    switch (aEvent)
    {
    default:
        SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
        break;
    }
}

WEAVE_ERROR TestWdmSubscriptionlessNotificationSender::SendSubscriptionlessNotify(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "TestWdmSubscriptionlessNotificationSender %s:", __func__);

    VerifyOrExit((NULL != mBinding), err = WEAVE_ERROR_INCORRECT_STATE);

    SubscriptionEngine::GetInstance()->GetNotificationEngine()->SendSubscriptionlessNotification(mBinding, mTraitPaths, mNumPaths);

exit:

    return err;
}
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

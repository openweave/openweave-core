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
 *      This file implements the Weave Data Management mock view client.
 *
 */

#define WEAVE_CONFIG_ENABLE_LOG_FILE_LINE_FUNC_ON_ERROR 1

#if 0 // TODO: Need to fix this but not a high priority. See WEAV-1348

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include "MockWdmViewClient.h"

#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>

#include "MockSinkTraits.h"
#include "MockTraitSchemas.h"

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::DataManagement::Traits;

void TLVPrettyPrinter(const char *aFormat, ...);
WEAVE_ERROR DebugPrettyPrint(nl::Weave::TLV::TLVReader & aReader);

class MockWdmViewClientImpl: public MockWdmViewClient
{
public:
    MockWdmViewClientImpl();

    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr, const char * const aTestCaseId);

    virtual WEAVE_ERROR StartTesting(const uint64_t aPublisherNodeId, const uint16_t aSubnetId);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    Binding * mBinding;
    ViewClient mViewClient;
    SingleResourceTraitCatalog<TraitDataSink>::CatalogItem mSinkCatalogStore[2];

    static void EventCallback (void * const aAppState, ViewClient::EventID aEvent, WEAVE_ERROR aErrorCode, ViewClient::EventParam & aEventParam);

    enum {
        kSimpleTraitSinkHandle = 0,
        kComplexTraitSinkHandle = 1
    };

    MockSimpleTraitDataSink mSimpleDataSink;
    MockComplexTraitDataSink mComplexDataSink;
    SingleResourceSinkTraitCatalog mSinkCatalog;

    static void BindingEventCallback (void * const apAppState, const Binding::EventID aEvent, const WEAVE_ERROR aErrorCode, Binding::EventParam & aParam);
};

static MockWdmViewClientImpl gWdmViewClient;

MockWdmViewClient * MockWdmViewClient::GetInstance ()
{
    return &gWdmViewClient;
}

MockWdmViewClientImpl::MockWdmViewClientImpl()
    : mSinkCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mSinkCatalogStore, sizeof(mSinkCatalogStore) / sizeof(mSinkCatalogStore[0]))
{
    mSinkCatalog.AddAt(0, &mSimpleDataSink, kSimpleTraitSinkHandle);
    mSinkCatalog.AddAt(0, &mComplexDataSink, kComplexTraitSinkHandle);
}

WEAVE_ERROR MockWdmViewClientImpl::Init (nl::Weave::WeaveExchangeManager *aExchangeMgr, const char * const aTestCaseId)
{
    WeaveLogDetail(DataManagement, "Test Case ID: %s", aTestCaseId);

    mExchangeMgr = aExchangeMgr;
    mBinding = NULL;
    onCompleteTest = NULL;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockWdmViewClientImpl::StartTesting(const uint64_t aPublisherNodeId, const uint16_t aSubnetId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Binding * binding = NULL;

    err = BindingPool::GetInstance()->Init(mExchangeMgr);
    SuccessOrExit(err);

    binding = BindingPool::GetInstance()->NewInitiatorBinding(this, BindingEventCallback, aPublisherNodeId);
    VerifyOrExit(NULL != binding, err = WEAVE_ERROR_NO_MEMORY);

    binding->Transport_WRM()
            .Security_None();

    if (nl::Weave::kWeaveSubnetId_NotSpecified != aSubnetId)
    {
        binding->Addressing_Subnet(aSubnetId);
    }

    mBinding = binding;

    err = binding->Prepare();
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

void MockWdmViewClientImpl::BindingEventCallback (void * const apAppState, const Binding::EventID aEvent,
    const WEAVE_ERROR aErrorCode, Binding::EventParam & aParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "%s: Event(%d), Error(%d)", __func__, aEvent, aErrorCode);

    MockWdmViewClientImpl * const initiator = reinterpret_cast<MockWdmViewClientImpl *>(apAppState);

    switch (aEvent)
    {
    case Binding::kEvent_Prepare_Done:
        TraitPath path[3];

        err = initiator->mViewClient.Init(initiator->mBinding, initiator, EventCallback);
        SuccessOrExit(err);


        path[0].mTraitDataHandle = kSimpleTraitSinkHandle;
        path[0].mPropertyPathHandle = kRootPropertyPathHandle;

        path[1].mTraitDataHandle = kSimpleTraitSinkHandle;
        path[1].mPropertyPathHandle = SimpleMockTrait::kHandle_active_locale;

        path[2].mTraitDataHandle = kComplexTraitSinkHandle;
        path[2].mPropertyPathHandle = kRootPropertyPathHandle;

        err = initiator->mViewClient.SendRequest(&initiator->mSinkCatalog, path, sizeof(path) / sizeof(path[0]));
        SuccessOrExit(err);
        break;

    default:
        WeaveLogDetail(DataManagement, "Unknown Binding event");
    }

exit:
    WeaveLogFunctError(err);
}

void MockWdmViewClientImpl::EventCallback (void * const apAppState, ViewClient::EventID aEvent, WEAVE_ERROR aErrorCode, ViewClient::EventParam & aEventParam)
{
    MockWdmViewClientImpl * const initiator = reinterpret_cast<MockWdmViewClientImpl *>(apAppState);

    switch (aEvent)
    {
    case ViewClient::kEvent_RequestFailed:
        WeaveLogDetail(DataManagement, "kEvent_RequestFailed");

        // Release the binding any point after here, as no one is going to reference it
        initiator->mBinding->Release();
        initiator->onCompleteTest();
        break;
    case ViewClient::kEvent_AboutToSendRequest:
        WeaveLogDetail(DataManagement, "kEvent_AboutToSendRequest: %d", aErrorCode);
        break;
    case ViewClient::kEvent_ViewResponseReceived:
        WeaveLogDetail(DataManagement, "kEvent_ViewResponseReceived: %d", aErrorCode);
        break;
    case ViewClient::kEvent_ViewResponseConsumed:
        WeaveLogDetail(DataManagement, "kEvent_ViewResponseConsumed");

        // Release the binding any point after here, as no one is going to reference it
        initiator->mBinding->Release();
        initiator->onCompleteTest();

        break;
    case ViewClient::kEvent_StatusReportReceived:
        WeaveLogDetail(DataManagement, "kEvent_StatusReportReceived");

        // Release the binding any point after here, as no one is going to reference it
        initiator->mBinding->Release();

        break;
    default:
        WeaveLogDetail(DataManagement, "Unknown ViewClient event: %d", aEvent);

        // There is really no logical things to do here other than Cancel
        // Since the API doesn't give guarantee on what the END message would be, a user of this API would have to
        // deduce many cases as END and all unknown cases as ERROR
        initiator->mViewClient.Cancel();
        initiator->mBinding->Release();
        initiator->onCompleteTest();
        break;
    }
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#endif // 0

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
 *      This file implements Update Client for Weave
 *      Data Management (WDM) profile.
 *
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/FibonacciUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <InetLayer/InetFaultInjection.h>
#include <SystemLayer/SystemFaultInjection.h>
#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * Flush the existing the exchange context
 *
 */
void UpdateClient::FlushExistingExchangeContext(const bool aAbortNow)
{
    if (NULL != mEC)
    {
        mEC->AppState          = NULL;
        mEC->OnMessageReceived = NULL;
        mEC->OnResponseTimeout = NULL;
        mEC->OnSendError       = NULL;
        mEC->OnAckRcvd         = NULL;

        if (aAbortNow)
        {
            mEC->Abort();
        }
        else
        {
            mEC->Close();
        }

        mEC = NULL;
    }
}

// Do nothing
UpdateClient::UpdateClient() { }

/**
 * AddRef to Binding
 *store pointers to binding and delegate
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(apBinding != NULL , err = WEAVE_ERROR_INCORRECT_STATE);

    // add reference to the binding
    apBinding->AddRef();

    // make a copy of the pointers
    mpBinding             = apBinding;
    mpAppState           = apAppState;
    mEventCallback       = aEventCallback;
    mEC                  = NULL;
    MoveToState(kState_Initialized);

exit:
    return err;
}


/**
 * acquire EC from binding, kick off send message
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to send update
 */
WEAVE_ERROR UpdateClient::SendUpdate(bool aIsPartialUpdate, PacketBuffer *aBuf, bool aIsFirstPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t msgType = kMsgType_UpdateRequest;

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    nl::Weave::TLV::TLVReader reader;
    UpdateRequest::Parser parser;
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    VerifyOrExit(NULL != aBuf, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(kState_Initialized == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    if (aIsFirstPayload)
    {
        FlushExistingExchangeContext();
        err = mpBinding->NewExchangeContext(mEC);
        SuccessOrExit(err);
    }

    mEC->AppState = this;
    mEC->OnMessageReceived = OnMessageReceived;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError = OnSendError;

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    reader.Init(aBuf);
    err = reader.Next();
    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(DataManagement, "Created malformed update, err: %" PRId32, err));
    parser.Init(reader);
    parser.CheckSchemaValidity();
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    if (aIsPartialUpdate)
    {
        msgType = kMsgType_PartialUpdateRequest;
    }

    // Use Inet's Send fault to fail inline
    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestSendErrorInline,
            nl::Inet::FaultInjection::GetManager().FailAtFault(
                nl::Inet::FaultInjection::kFault_Send,
                0, 1));

    // Use DropOutgoingMsg fault to fail the current outgoing message.
    // If WRM is being used, the transmission would be retried after a
    // retransmission timeout delay.
    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestDropMessage,
            nl::Inet::FaultInjection::GetManager().FailAtFault(
                nl::Weave::FaultInjection::kFault_DropOutgoingUDPMsg,
                0, 1));

    // Use WRM's SendError fault to fail asynchronously
    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestSendErrorAsync,
            nl::Weave::FaultInjection::GetManager().FailAtFault(
                nl::Weave::FaultInjection::kFault_WRMSendError,
                0, 1));

    err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, msgType, aBuf,
            nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
    aBuf = NULL;
    SuccessOrExit(err);

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestTimeout,
            SubscriptionEngine::GetInstance()->GetExchangeManager()->ExpireExchangeTimers());

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_DelayUpdateResponse,
            nl::Weave::FaultInjection::GetManager().FailAtFault(
                nl::Weave::FaultInjection::kFault_DropIncomingUDPMsg,
                0, 1));

    MoveToState(kState_AwaitingResponse);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        CancelUpdate();
    }

    if (NULL != aBuf)
    {
        PacketBuffer::Free(aBuf);
        aBuf = NULL;
    }

    WeaveLogFunctError(err);

    return err;

}

void UpdateClient::CloseUpdate(bool aAbort)
{
    if (kState_Uninitialized != mState)
    {
        FlushExistingExchangeContext(aAbort);
        if (kState_Initialized != mState)
        {
            MoveToState(kState_Initialized);
        }
    }
}

/**
 * Reset update client to initialized status. clear the buffer
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
void UpdateClient::CancelUpdate(void)
{
    bool abort = true;

    WeaveLogDetail(DataManagement, "UpdateClient::CancelUpdate");

    CloseUpdate(abort);
}

/**
 * Release binding for the update. Should only be called once.
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::Shutdown(void)
{
    if (kState_Uninitialized != mState)
    {
        CancelUpdate();

        if (NULL != mpBinding)
        {
            mpBinding->Release();
            mpBinding = NULL;
        }

        mEventCallback = NULL;
        mpAppState      = NULL;
        MoveToState(kState_Uninitialized);
    }

    return WEAVE_NO_ERROR;
}

void UpdateClient::OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;
    inParam.Clear();
    outParam.Clear();

    UpdateClient * const pUpdateClient = reinterpret_cast<UpdateClient *>(aEC->AppState);
    void * const pAppState         = pUpdateClient->mpAppState;
    EventCallback CallbackFunc     = pUpdateClient->mEventCallback;

    VerifyOrExit(kState_AwaitingResponse == pUpdateClient->mState, err = WEAVE_ERROR_INCORRECT_STATE);

    pUpdateClient->CancelUpdate();

    inParam.UpdateComplete.Reason = aErrorCode;
    inParam.Source                = pUpdateClient;
    CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogFunctError(err);
        pUpdateClient->CancelUpdate();
    }
}

void UpdateClient::OnResponseTimeout(nl::Weave::ExchangeContext * aEC)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;
    inParam.Clear();
    outParam.Clear();

    UpdateClient * pUpdateClient   = reinterpret_cast<UpdateClient *>(aEC->AppState);
    void * const pAppState     = pUpdateClient->mpAppState;
    EventCallback CallbackFunc = pUpdateClient->mEventCallback;

    VerifyOrExit(kState_AwaitingResponse == pUpdateClient->mState, err = WEAVE_ERROR_INCORRECT_STATE);

    pUpdateClient->CancelUpdate();

    inParam.UpdateComplete.Reason = WEAVE_ERROR_TIMEOUT;
    inParam.Source = pUpdateClient;
    CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);

exit:

    WeaveLogFunctError(err);

    if (err != WEAVE_NO_ERROR)
    {
        pUpdateClient->CancelUpdate();
    }

}

void UpdateClient::OnMessageReceived(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                   const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                   PacketBuffer * aPayload)
{
    WEAVE_ERROR err            = WEAVE_NO_ERROR;
    nl::Weave::Profiles::StatusReporting::StatusReport status;
    UpdateClient * pUpdateClient   = reinterpret_cast<UpdateClient *>(aEC->AppState);
    void * const pAppState     = pUpdateClient->mpAppState;
    EventCallback CallbackFunc = pUpdateClient->mEventCallback;
    InEventParam inParam;
    OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    VerifyOrExit(kState_AwaitingResponse == pUpdateClient->mState, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(aEC == pUpdateClient->mEC, err = WEAVE_NO_ERROR);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) &&
        (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        bool noAbort = false;

        err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);

        inParam.Source= pUpdateClient;
        inParam.UpdateComplete.Reason = WEAVE_NO_ERROR;
        inParam.UpdateComplete.StatusReportPtr = &status;

        pUpdateClient->CloseUpdate(noAbort);

        CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);

    }
    else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_UpdateContinue == aMsgType))
    {
        pUpdateClient->MoveToState(kState_Initialized);
        CallbackFunc(pAppState, kEvent_UpdateContinue, inParam, outParam);
    }
    else
    {
        inParam.UpdateComplete.Reason = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
        CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);
    }

exit:

    WeaveLogFunctError(err);

    if (err != WEAVE_NO_ERROR)
    {
        pUpdateClient->CancelUpdate();
    }

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    aEC = NULL;
}

void UpdateClient::DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam)
{
    IgnoreUnusedVariable(aInParam);
    IgnoreUnusedVariable(aOutParam);

    WeaveLogDetail(DataManagement, "%s event: %d", __func__, aEvent);
}

#if WEAVE_DETAIL_LOGGING
const char * UpdateClient::GetStateStr() const
{
    switch (mState)
    {
    case kState_Uninitialized:
        return "Uninitialized";

    case kState_Initialized:
        return "Initialized";

    case kState_AwaitingResponse:
        return "AwaitingResponse";
    }
    return "N/A";
}
#endif // WEAVE_DETAIL_LOGGING

void UpdateClient::MoveToState(const UpdateClientState aTargetState)
{
    mState = aTargetState;
    WeaveLogDetail(DataManagement, "UC moving to [%10.10s]", GetStateStr());
}

void UpdateClient::ClearState(void)
{
    MoveToState(kState_Uninitialized);
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

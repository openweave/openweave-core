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
#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

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
    mNumPartialUpdateRequest = 0;
    MoveToState(kState_Initialized);

exit:
    return err;
}

/**
 *  @brief Inject expiry time into the TLV stream
 *
 *  @param [in] aExpiryTimeMicroSecond  Expiry time for this request, in microseconds since UNIX epoch
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *  @retval other           Was unable to inject expiry time into the TLV stream
 */
WEAVE_ERROR UpdateClient::AddExpiryTime(utc_timestamp_t aExpiryTimeMicroSecond)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_ExpiryTime), aExpiryTimeMicroSecond);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 *  @brief Add the number of partial update requests into the TLV stream
 *
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *  @retval other           Was unable to add number of partial update requests into the TLV stream
 */
WEAVE_ERROR UpdateClient::AddNumPartialUpdateRequests(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_NumPartialUpdateRequests), mNumPartialUpdateRequest);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Initializes the update. Should only be called once.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Was unable to initialize the update.
 */
WEAVE_ERROR UpdateClient::StartUpdate(utc_timestamp_t aExpiryTimeMicroSecond, AddArgumentCallback aAddArgumentCallback, uint32_t maxUpdateSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t maxBufPayloadSize, maxPayloadSize;
    VerifyOrExit(mState == kState_Initialized, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(DataManagement, "<UC:Run> Init PacketBuf");

    mpBuf = PacketBuffer::New();
    VerifyOrExit(mpBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    maxBufPayloadSize   = mpBinding->GetMaxWeavePayloadSize(mpBuf);
    maxPayloadSize      = maxBufPayloadSize < maxUpdateSize ? maxBufPayloadSize : maxUpdateSize;

    mWriter.Init(mpBuf, maxPayloadSize);

    mAddArgumentCallback = aAddArgumentCallback;

    err = StartUpdateRequest(aExpiryTimeMicroSecond);
    SuccessOrExit(err);

    err = StartDataList();
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in StartUpdate");
        CancelUpdate();
    }

    return err;
}

/**
 * Start the construction of the update request.
 *
 * @param [in] aExpiryTimeMicroSecond  Expiry time for this request, in microseconds since UNIX epoch
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the toplevel of the buffer.
 * @retval other           Unable to construct the end of the update request.
 */
WEAVE_ERROR UpdateClient::StartUpdateRequest(utc_timestamp_t aExpiryTimeMicroSecond)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType dummyType;

    VerifyOrExit(mState == kState_Initialized, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.StartContainer(TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyType);
    SuccessOrExit(err);

    if (aExpiryTimeMicroSecond != 0)
    {
        err = AddExpiryTime(aExpiryTimeMicroSecond);
        SuccessOrExit(err);
    }

    if (mAddArgumentCallback != NULL)
    {
        err = mAddArgumentCallback(this, mpAppState, mWriter);
        SuccessOrExit(err);
    }

    err = AddNumPartialUpdateRequests();
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in StartUpdateRequest");
    }

    return err;
}

/**
 * End the construction of the update request.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at update request container.
 * @retval other           Unable to construct the end of the update request.
 */
WEAVE_ERROR UpdateClient::EndUpdateRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_BuildDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(nl::Weave::TLV::kTLVType_NotSpecified);
    SuccessOrExit(err);

    err = mWriter.Finalize();
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in EndUpdateRequest");
    }

    return err;
}

/**
 * Starts the construction of the data list array.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If it is not at the DataList container.
 * @retval other           Unable to construct the beginning of the data list.
 */
WEAVE_ERROR UpdateClient::StartDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Initialized, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.StartContainer(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_DataList), nl::Weave::TLV::kTLVType_Array, mDataListContainerType);
    SuccessOrExit(err);

    MoveToState(kState_BuildDataList);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in StartDataList");
    }

    return err;
}

/**
 * End the construction of the data list array.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If it is not at the DataList container.
 * @retval other           Unable to construct the end of the data list.
 */
WEAVE_ERROR UpdateClient::EndDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_BuildDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(mDataListContainerType);
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in EndDataList");
    }

    return err;
}

/**
 * Construct everything for data element except actual data
 *
 * @param[in] aProfileID ProfileID for data element
 * @param[in] aInstanceID InstanceID for data element. When set to 0, it will be omitted from the request and default to the first instance of the trait on the publisher.
 * @param[in] aResourceID ResourceID for data element. When set to 0, it will be omitted from the request and default to the resource ID of the publisher.
 * @param[in] aRequiredDataVersion Required version for data element.  When set to non-zero, the update will only be applied if the publisher's DataVersion for the trait matches aRequiredDataVersion.  When set to 0, the update shall be applied unconditionally.
 * @param[in] aSchemaVersionRange SchemaVersionRange for data element, optional
 * @param[in] aPathArray pathArray for data element, optional
 * @param[in] aPathLength pathLength for data element, optional
 * @param[in] aOuterWriter OuterWriter for data element
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to construct data element.
 */
WEAVE_ERROR UpdateClient::StartElement(const uint32_t &aProfileID,
                    const uint64_t &aInstanceID,
                    const uint64_t &aResourceID,
                    const DataVersion &aRequiredDataVersion,
                    const SchemaVersionRange * aSchemaVersionRange,
                    const uint64_t *aPathArray,
				    const size_t aPathLength,
				    TLV::TLVWriter &aOuterWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Path::Builder pathBuilder;
    VerifyOrExit(kState_BuildDataList == mState, err = WEAVE_ERROR_INCORRECT_STATE);
    Checkpoint(aOuterWriter);

    err = aOuterWriter.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, mDataElementContainerType);
    SuccessOrExit(err);
    err = pathBuilder.Init(&aOuterWriter, nl::Weave::TLV::ContextTag(DataElement::kCsTag_Path));
    SuccessOrExit(err);
    if (aSchemaVersionRange == NULL)
        pathBuilder.ProfileID(aProfileID);
    else
        pathBuilder.ProfileID(aProfileID, *aSchemaVersionRange);

    if (aResourceID != 0x0)
       pathBuilder.ResourceID(aResourceID);

    if (aInstanceID != 0x0)
        pathBuilder.InstanceID(aInstanceID);

    if (aPathLength != 0)
    {
        pathBuilder.TagSection();

        for (size_t pathIndex = 0; pathIndex < aPathLength;  pathIndex++)
        {
            pathBuilder.AdditionalTag(aPathArray[pathIndex]);
        }
    }

    pathBuilder.EndOfPath();

    err = pathBuilder.GetError();
    SuccessOrExit(err);

    if (aRequiredDataVersion != 0x0)
    {
        err = aOuterWriter.Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Version), aRequiredDataVersion);
        SuccessOrExit(err);
    }

    MoveToState(kState_BuildDataElement);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in StartElement");
        err = CancelElement(aOuterWriter);
    }

    return err;
}

/**
 * Construct everything for data element
 *
 * @param[in] aProfileID ProfileID for data element
 * @param[in] aInstanceID InstanceID for data element. When set to 0, it will be omitted from the request and default to the first instance of the trait on the publisher.
 * @param[in] aResourceID ResourceID for data element. When set to 0, it will be omitted from the request and default to the resource ID of the publisher.
 * @param[in] aRequiredDataVersion Data version for data element.  When set to non-zero, the update will only be applied if the publisher's DataVersion for the trait matches aRequiredDataVersion.  When set to 0, the update shall be applied unconditionally.
 * @param[in] aSchemaVersionRange SchemaVersionRange for data element, optional
 * @param[in] aPathArray pathArray for data element, optional
 * @param[in] aPathLength pathLength for data element, optional
 * @param[in] aAddElementCallback AddElementCallback is used to write actual data in data element in callback
 * @param[in] aCallstate it is used to pass call context
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to construct data element.
 *
 */
WEAVE_ERROR UpdateClient::AddElement(const uint32_t &aProfileID,
                    const uint64_t &aInstanceID,
                    const uint64_t &aResourceID,
                    const DataVersion &aRequiredDataVersion,
                    const SchemaVersionRange * aSchemaVersionRange,
                    const uint64_t *aPathArray,
				    const size_t aPathLength,
                    AddElementCallback aAddElementCallback,
                    void * aCallState
                    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLV::TLVWriter outerWriter;

    err = StartElement(aProfileID, aInstanceID, aResourceID, aRequiredDataVersion, aSchemaVersionRange, aPathArray, aPathLength, outerWriter);
    SuccessOrExit(err);

    err = aAddElementCallback(this, aCallState, outerWriter);
    SuccessOrExit(err);

    err = FinalizeElement(outerWriter);
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in AddElement %d", err);
        CancelElement(outerWriter);
    }

    return err;
}

/**
 * End the data element's container
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the DataElement container.
 * @retval other           Unable to construct the end of the data and data element.
 */
WEAVE_ERROR UpdateClient::FinalizeElement(TLV::TLVWriter &aOuterWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(kState_BuildDataElement == mState, err = WEAVE_ERROR_INCORRECT_STATE);
    mWriter = aOuterWriter;

    err = mWriter.EndContainer(mDataElementContainerType);
    SuccessOrExit(err);

    MoveToState(kState_BuildDataList);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in FinalizeElement");
        CancelElement(aOuterWriter);
    }

    return err;
}

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

/**
 * Checkpoint the request state into a TLVWriter
 *
 * @param[out] aWriter A writer to checkpoint the state of the TLV writer into.
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::Checkpoint(TLV::TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    aWriter    = mWriter;
    return err;
}

/**
 * Restore a TLVWriter into the request state
 *
 * @param aWriter[out] A writer to restore the state of the TLV writer into.
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::Rollback(TLV::TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    mWriter    = aWriter;
    return err;
}

/**
 * Rollback the update client state into the checkpointed TLVWriter
 *
 * @param[in] aOuterWriter A writer to that captured the state at some point in the past
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::CancelElement(TLV::TLVWriter &aOuterWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(kState_BuildDataElement == mState, err = WEAVE_ERROR_INCORRECT_STATE);
    err = Rollback(aOuterWriter);
    SuccessOrExit(err);

exit:

    if (err == WEAVE_NO_ERROR)
    {
        MoveToState(kState_BuildDataList);
    }
    else
    {
        Shutdown();
    }

    return err;
}

/**
 * acquire EC from binding, kick off send message
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to send update
 */
WEAVE_ERROR UpdateClient::SendUpdate(bool aIsPartialUpdate)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(NULL != mpBuf, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(kState_BuildDataList == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    err = EndDataList();
    SuccessOrExit(err);

    err = EndUpdateRequest();
    SuccessOrExit(err);

    if (mNumPartialUpdateRequest == 0)
    {
        FlushExistingExchangeContext();
    }

    err = mpBinding->NewExchangeContext(mEC);

    mEC->AppState = this;
    mEC->OnMessageReceived = OnMessageReceived;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError = OnSendError;

    if (aIsPartialUpdate)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Partial update");
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_PartialUpdateRequest, mpBuf,
                           nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        mpBuf = NULL;
        SuccessOrExit(err);

        mNumPartialUpdateRequest ++;
    }
    else
    {
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_UpdateRequest, mpBuf,
                               nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        mpBuf = NULL;
        SuccessOrExit(err);

        mNumPartialUpdateRequest = 0;
    }

    MoveToState(kState_AwaitingResponse);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in SendUpdate");
        CancelUpdate();
    }

    if (NULL != mpBuf)
    {
        PacketBuffer::Free(mpBuf);
        mpBuf = NULL;
    }

     WeaveLogFunctError(err);

    return err;

}

/**
 * Reset update client to initialized status. clear the buffer
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::CancelUpdate(void)
{
    if (kState_Uninitialized != mState && kState_Initialized != mState)
    {
        if (NULL != mpBuf)
        {
            PacketBuffer::Free(mpBuf);
            mpBuf = NULL;
        }

        mAddArgumentCallback = NULL;
        mNumPartialUpdateRequest = 0;
        FlushExistingExchangeContext();
        MoveToState(kState_Initialized);
    }

    return WEAVE_NO_ERROR;
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
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in OnSendError");
        err = pUpdateClient->CancelUpdate();
    }

    WeaveLogFunctError(err);
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

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in OnResponseTimeout");
        err = pUpdateClient->CancelUpdate();
    }

    WeaveLogFunctError(err);
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

    err = pUpdateClient->CancelUpdate();
    SuccessOrExit(err);

    err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
    SuccessOrExit(err);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) &&
        (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        inParam.Source= pUpdateClient;
        inParam.UpdateComplete.Reason = WEAVE_NO_ERROR;
        inParam.UpdateComplete.StatusReportPtr = &status;
        CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);

    }
    else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_UpdateContinue == aMsgType))
    {
        CallbackFunc(pAppState, kEvent_UpdateContinue, inParam, outParam);
    }
    else
    {
        inParam.UpdateComplete.Reason = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
        CallbackFunc(pAppState, kEvent_UpdateComplete, inParam, outParam);
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Fail in OnMessageReceived %d", err);
        err = pUpdateClient->CancelUpdate();
    }

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    WeaveLogFunctError(err);
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

    case kState_BuildDataList:
        return "BuildDataList";

    case kState_BuildDataElement:
        return "BuildDataElement";

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

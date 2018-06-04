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

/**
 *  @brief Inject expiry time into the TLV stream
 *
 *  @param [in] aExpiryTimeMicroSecond  Expiry time for this request, in microseconds since UNIX epoch
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *  @retval other           Was unable to inject expiry time into the TLV stream
 */
WEAVE_ERROR UpdateEncoder::AddExpiryTime(utc_timestamp_t aExpiryTimeMicroSecond)
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
WEAVE_ERROR UpdateEncoder::AddUpdateRequestIndex(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_UpdateRequestIndex), mUpdateRequestIndex);
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
WEAVE_ERROR UpdateEncoder::StartUpdate(utc_timestamp_t aExpiryTimeMicroSecond, AddArgumentCallback aAddArgumentCallback, uint32_t maxUpdateSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t maxPayloadSize;

    VerifyOrExit(mState == kState_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    WeaveLogDetail(DataManagement, "<UC:Run> Init PacketBuf");

    err = mpBinding->AllocateRightSizedBuffer(mpBuf, maxUpdateSize, WDM_MIN_UPDATE_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    mWriter.Init(mpBuf, maxPayloadSize);

    mAddArgumentCallback = aAddArgumentCallback;

    err = StartUpdateRequest(aExpiryTimeMicroSecond);
    SuccessOrExit(err);

    err = StartDataList();
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        CancelUpdate();
    }

    WeaveLogFunctError(err);

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
WEAVE_ERROR UpdateEncoder::StartUpdateRequest(utc_timestamp_t aExpiryTimeMicroSecond)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType dummyType;

    VerifyOrExit(mState == kState_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

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

    err = AddUpdateRequestIndex();
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * End the construction of the update request.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at update request container.
 * @retval other           Unable to construct the end of the update request.
 */
WEAVE_ERROR UpdateEncoder::EndUpdateRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_EncodingDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(nl::Weave::TLV::kTLVType_NotSpecified);
    SuccessOrExit(err);

    err = mWriter.Finalize();
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * Starts the construction of the data list array.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If it is not at the DataList container.
 * @retval other           Unable to construct the beginning of the data list.
 */
WEAVE_ERROR UpdateEncoder::StartDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.StartContainer(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_DataList), nl::Weave::TLV::kTLVType_Array, mDataListContainerType);
    SuccessOrExit(err);

    MoveToState(kState_BuildDataList);

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * End the construction of the data list array.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If it is not at the DataList container.
 * @retval other           Unable to construct the end of the data list.
 */
WEAVE_ERROR UpdateEncoder::EndDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_EncodingDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(mDataListContainerType);
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

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
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to construct data element.
 */
WEAVE_ERROR UpdateEncoder::StartElement(const uint32_t &aProfileID,
                    const uint64_t &aInstanceID,
                    const ResourceIdentifier &aResourceID,
                    const DataVersion &aRequiredDataVersion,
                    const SchemaVersionRange * aSchemaVersionRange,
                    const uint64_t *aPathArray,
				    const size_t aPathLength)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Path::Builder pathBuilder;
    VerifyOrExit(kState_EncodingDataList == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    MoveToState(kState_EncodingDataElement);

    err = mWriter.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, mDataElementContainerType);
    SuccessOrExit(err);
    err = pathBuilder.Init(&mWriter, nl::Weave::TLV::ContextTag(DataElement::kCsTag_Path));
    SuccessOrExit(err);
    if (aSchemaVersionRange == NULL)
        pathBuilder.ProfileID(aProfileID);
    else
        pathBuilder.ProfileID(aProfileID, *aSchemaVersionRange);

    if (aResourceID != ResourceIdentifier::SELF_NODE_ID)
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
        WeaveLogDetail(DataManagement, "<UC:Run> conditional update");
        err = mWriter.Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Version), aRequiredDataVersion);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "<UC:Run> unconditional update");
    }

exit:

    WeaveLogFunctError(err);

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
WEAVE_ERROR UpdateEncoder::AddElement(const uint32_t &aProfileID,
                    const uint64_t &aInstanceID,
                    const ResourceIdentifier &aResourceID,
                    const DataVersion &aRequiredDataVersion,
                    const SchemaVersionRange * aSchemaVersionRange,
                    const uint64_t *aPathArray,
				    const size_t aPathLength,
                    AddElementCallback aAddElementCallback,
                    void * aCallState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLV::TLVWriter checkpoint;

    Checkpoint(checkpoint);

    err = StartElement(aProfileID, aInstanceID, aResourceID, aRequiredDataVersion, aSchemaVersionRange, aPathArray, aPathLength);
    SuccessOrExit(err);

    err = aAddElementCallback(this, aCallState, mWriter);
    SuccessOrExit(err);

    err = FinalizeElement();
    SuccessOrExit(err);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        CancelElement(checkpoint);
    }

    WeaveLogFunctError(err);

    return err;
}

/**
 * End the data element's container
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the request is not at the DataElement container.
 * @retval other           Unable to construct the end of the data and data element.
 */
WEAVE_ERROR UpdateEncoder::FinalizeElement()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(kState_BuildDataElement == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(mDataElementContainerType);
    SuccessOrExit(err);

    MoveToState(kState_EncodingDataList);

exit:

    WeaveLogFunctError(err);

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
 */
void UpdateEncoder::Checkpoint(TLV::TLVWriter &aWriter)
{
    aWriter    = mWriter;
}

/**
 * Restore a TLVWriter into the request state
 *
 * @param aWriter[out] A writer to restore the state of the TLV writer from.
 */
void UpdateEncoder::Rollback(TLV::TLVWriter &aWriter)
{
    mWriter = aWriter;
}

/**
 * Rollback the update client state into the checkpointed TLVWriter
 *
 * @param[in] aOuterWriter A writer to that captured the state at some point in the past
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateEncoder::CancelElement(TLV::TLVWriter &aCheckpoint)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    VerifyOrExit(kState_EncodingDataElement == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    Rollback(aCheckpoint);

exit:

    if (err == WEAVE_NO_ERROR)
    {
        MoveToState(kState_EncodingDataList);
    }
    else
    {
        Shutdown();
    }

    return err;
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
    mUpdateRequestIndex  = 0;
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
WEAVE_ERROR UpdateClient::SendUpdate(bool aIsPartialUpdate, PacketBuffer *aPBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    nl::Weave::TLV::TLVReader reader;
    UpdateRequest::Parser parser;
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    VerifyOrExit(NULL != aBuf, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(kState_Initialized == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    err = EndDataList();
    SuccessOrExit(err);

    err = EndUpdateRequest();
    SuccessOrExit(err);

    if (mUpdateRequestIndex == 0)
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
    reader.Init(mpBuf);
    reader.Next();
    parser.Init(reader);
    parser.CheckSchemaValidity();
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    if (aIsPartialUpdate)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Partial update");
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_PartialUpdateRequest, mpBuf,
                           nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        aBuf = NULL;
        SuccessOrExit(err);

        mUpdateRequestIndex ++;
        WeaveLogDetail(DataManagement, "mUpdateRequestIndex: %" PRIu32 "", mUpdateRequestIndex);
    }
    else
    {
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_UpdateRequest, mpBuf,
                               nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        aBuf = NULL;
        SuccessOrExit(err);

        mUpdateRequestIndex = 0;
    }

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

void UpdateEncoder::Cancel(void)
{
    if (mBuf)
    {
        PacketBuffer::Free(mBuf);
        mBuf = NULL;
    }
    mState = kState_Ready;
    mAddArgumentCallback = NULL;
}

/**
 * Reset update client to initialized status. clear the buffer
 *
 * @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR UpdateClient::CancelUpdate(void)
{
    WeaveLogDetail(DataManagement, "UpdateClient::CancelUpdate");

    if (kState_Uninitialized != mState && kState_Initialized != mState)
    {
        mUpdateRequestIndex = 0;
        FlushExistingExchangeContext();
        MoveToState(kState_Initialized);
    }

    //TODO: this method should be void
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
        WeaveLogFunctError(err);
        err = pUpdateClient->CancelUpdate();
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
        err = pUpdateClient->CancelUpdate();
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
        err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);

        inParam.Source= pUpdateClient;
        inParam.UpdateComplete.Reason = WEAVE_NO_ERROR;
        inParam.UpdateComplete.StatusReportPtr = &status;

        err = pUpdateClient->CancelUpdate();
        SuccessOrExit(err);

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
        err = pUpdateClient->CancelUpdate();
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

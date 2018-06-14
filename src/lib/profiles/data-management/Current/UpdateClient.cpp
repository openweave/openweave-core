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

TraitUpdatableDataSink *Locate(TraitDataHandle aTraitDataHandle, const TraitCatalogBase<TraitDataSink> *aDataSinkCatalog)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSink *dataSink = NULL;
    TraitUpdatableDataSink *updatableDataSink = NULL;

    err = aDataSinkCatalog->Locate(aTraitDataHandle, &dataSink);
    SuccessOrExit(err);

    VerifyOrExit(dataSink->IsUpdatableDataSink(), );

    updatableDataSink = static_cast<TraitUpdatableDataSink *>(dataSink);

exit:
    VerifyOrDie(NULL != updatableDataSink);

    return updatableDataSink;
}


WEAVE_ERROR UpdateEncoder::EncodeRequest(Context *aContext)
{
    WEAVE_ERROR err;

    // TODO: VerifyOrExit(...

    mContext = aContext;

    mContext->mNumDataElementsAddedToPayload = 0;

    err = StartUpdate();
    SuccessOrExit(err);

    err = BuildSingleUpdateRequestDataList();
    SuccessOrExit(err);

    err = FinishUpdate();
    SuccessOrExit(err);

exit:
    mContext = NULL;
    MoveToState(kState_Uninitialized);

    return err;
}

WEAVE_ERROR UpdateEncoder::BuildSingleUpdateRequestDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const TraitSchemaEngine * schemaEngine;
    bool dictionaryOverflowed = false;
    TraitPathStore &traitPathList = *(mContext->mInProgressUpdateList);

    WeaveLogDetail(DataManagement, "Num items in progress = %u/%u; current: %u",
            traitPathList.GetNumItems(),
            traitPathList.GetPathStoreSize(),
            mContext->mItemInProgress);

    while (mContext->mItemInProgress < traitPathList.GetPathStoreSize())
    {
        size_t &i = mContext->mItemInProgress;

        if (!(traitPathList.IsItemValid(i)))
        {
            i++;
            continue;
        }

        WeaveLogDetail(DataManagement, "Encoding item %u, ForceMerge: %d, Private: %d", i, traitPathList.AreFlagsSet(i, SubscriptionClient::kFlag_ForceMerge),
                traitPathList.AreFlagsSet(i, SubscriptionClient::kFlag_Private));

        if (mContext->mNextDictionaryElementPathHandle != kNullPropertyPathHandle)
        {
            WeaveLogDetail(DataManagement, "Resume encoding a dictionary");
        }

        err = DirtyPathToDataElement();
        SuccessOrExit(err);

        dictionaryOverflowed = (mContext->mNextDictionaryElementPathHandle != kNullPropertyPathHandle);
        if (dictionaryOverflowed)
        {
            TraitPath traitPath;
            traitPathList.GetItemAt(i, traitPath);
            InsertInProgressUpdateItem(traitPath, schemaEngine);
        }

        i++;

        VerifyOrExit(!dictionaryOverflowed, /* no error */);
    }

exit:

    if (mContext->mNumDataElementsAddedToPayload > 0 &&
            (err == WEAVE_ERROR_BUFFER_TOO_SMALL))
    {
        WeaveLogDetail(DataManagement, "Suppressing error %" PRId32 "; will try again later", err);
        RemoveInProgressPrivateItemsAfter(mContext->mItemInProgress);
        err = WEAVE_NO_ERROR;
    }

    return err;

}

WEAVE_ERROR UpdateEncoder::EncodeElement(const DataElementContext &aElementContext)
{
    WEAVE_ERROR err;
    bool isDictionaryReplace = false;
    nl::Weave::TLV::TLVType dataContainerType;
    uint64_t tag = nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data);

    WeaveLogDetail(DataManagement, "<EncodeElement> with property path handle 0x%08x",
            aElementContext.mTraitPath.mPropertyPathHandle);

    if (aElementContext.mSchemaEngine->IsDictionary(aElementContext.mTraitPath.mPropertyPathHandle) &&
            ! aElementContext.mForceMerge)
    {
        isDictionaryReplace = true;
    }

    if (isDictionaryReplace)
    {
        // If the element is a whole dictionary, use the "replace" scheme.
        // The path of the DataElement points to the parent of the dictionary.
        // The data has to be a structure with one element, which is the dictionary itself.
        WeaveLogDetail(DataManagement, "<EncodeElement> replace dictionary");
        err = mWriter.StartContainer(tag, nl::Weave::TLV::kTLVType_Structure, dataContainerType);
        SuccessOrExit(err);

        tag = aElementContext.mSchemaEngine->GetTag(aElementContext.mTraitPath.mPropertyPathHandle);
    }

    err = aElementContext.mDataSink->ReadData(aElementContext.mTraitPath.mTraitDataHandle,
                                      aElementContext.mTraitPath.mPropertyPathHandle,
                                      tag,
                                      mWriter,
                                      mContext->mNextDictionaryElementPathHandle);
    SuccessOrExit(err);

    if (isDictionaryReplace)
    {
        err = mWriter.EndContainer(dataContainerType);
        SuccessOrExit(err);
    }

exit:
    return err;
}


WEAVE_ERROR UpdateEncoder::DirtyPathToDataElement()
{
    WEAVE_ERROR err;
    TLV::TLVWriter checkpoint;
    DataElementContext elementContext;

    Checkpoint(checkpoint);

    mContext->mInProgressUpdateList->GetItemAt(mContext->mItemInProgress, elementContext.mTraitPath);

    elementContext.mDataSink = Locate(elementContext.mTraitPath.mTraitDataHandle, mContext->mDataSinkCatalog);

    elementContext.mSchemaEngine = elementContext.mDataSink->GetSchemaEngine();
    VerifyOrDie(elementContext.mSchemaEngine != NULL);

    elementContext.mProfileId = elementContext.mSchemaEngine->GetProfileId();

    err = mContext->mDataSinkCatalog->GetResourceId(elementContext.mTraitPath.mTraitDataHandle,
                                          elementContext.mResourceId);
    SuccessOrExit(err);

    err = mContext->mDataSinkCatalog->GetInstanceId(elementContext.mTraitPath.mTraitDataHandle,
                                          elementContext.mInstanceId);
    SuccessOrExit(err);

    elementContext.mUpdateRequiredVersion = elementContext.mDataSink->GetUpdateRequiredVersion();

    {
        uint64_t tags[elementContext.mSchemaEngine->mSchema.mTreeDepth];

        elementContext.mTags = &(tags[0]);
        err = elementContext.mSchemaEngine->GetRelativePathTags(elementContext.mTraitPath.mPropertyPathHandle,
                elementContext.mTags,
                //ArraySize(tags),
                elementContext.mSchemaEngine->mSchema.mTreeDepth,
                elementContext.mNumTags);
        SuccessOrExit(err);

        elementContext.mForceMerge = mContext->mInProgressUpdateList->AreFlagsSet(mContext->mItemInProgress, SubscriptionClient::kFlag_ForceMerge);

        if (elementContext.mSchemaEngine->IsDictionary(elementContext.mTraitPath.mPropertyPathHandle) &&
                ! elementContext.mForceMerge)
        {
            // If the property being updated is a dictionary, we need to use the "replace"
            // scheme explicitly so that the whole property is replaced on the responder.
            // So, the path has to point to the parent of the dictionary.
            VerifyOrExit(elementContext.mNumTags > 0, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);
            elementContext.mNumTags--;
        }

        err = StartElement(elementContext);
        SuccessOrExit(err);

        err = EncodeElement(elementContext);
        SuccessOrExit(err);

        err = FinalizeElement();
        SuccessOrExit(err);

        mContext->mNumDataElementsAddedToPayload++;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        CancelElement(checkpoint);
    }

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
    err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_UpdateRequestIndex), mContext->mUpdateRequestIndex);
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
WEAVE_ERROR UpdateEncoder::StartUpdate()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Uninitialized, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(NULL != mContext->mBuf, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mWriter.Init(mContext->mBuf, mContext->mMaxPayloadSize);

    MoveToState(kState_Ready);

    err = StartUpdateRequest();
    SuccessOrExit(err);

    err = StartDataList();
    SuccessOrExit(err);

exit:
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
WEAVE_ERROR UpdateEncoder::StartUpdateRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType dummyType;

    VerifyOrExit(mState == kState_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.StartContainer(TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyType);
    SuccessOrExit(err);

    if (mContext->mExpiryTimeMicroSecond != 0)
    {
        err = AddExpiryTime(mContext->mExpiryTimeMicroSecond);
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

    MoveToState(kState_Done);

exit:

    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR UpdateEncoder::FinishUpdate()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = EndDataList();
    SuccessOrExit(err);

    err = EndUpdateRequest();
    SuccessOrExit(err);

exit:
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

    err = mWriter.StartContainer(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_DataList), nl::Weave::TLV::kTLVType_Array, mDataListOuterContainerType);
    SuccessOrExit(err);

    MoveToState(kState_EncodingDataList);

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

    err = mWriter.EndContainer(mDataListOuterContainerType);
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
WEAVE_ERROR UpdateEncoder::StartElement(const DataElementContext &aElementContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Path::Builder pathBuilder;
    VerifyOrExit(kState_EncodingDataList == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    MoveToState(kState_EncodingDataElement);

    err = mWriter.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, mDataElementOuterContainerType);
    SuccessOrExit(err);
    err = pathBuilder.Init(&mWriter, nl::Weave::TLV::ContextTag(DataElement::kCsTag_Path));
    SuccessOrExit(err);
    if (aElementContext.mSchemaVersionRange == NULL)
        pathBuilder.ProfileID(aElementContext.mProfileId);
    else
        pathBuilder.ProfileID(aElementContext.mProfileId, *aElementContext.mSchemaVersionRange);

    if (aElementContext.mResourceId != ResourceIdentifier::SELF_NODE_ID)
       pathBuilder.ResourceID(aElementContext.mResourceId);

    if (aElementContext.mInstanceId != 0x0)
        pathBuilder.InstanceID(aElementContext.mInstanceId);

    if (aElementContext.mNumTags != 0)
    {
        pathBuilder.TagSection();

        for (size_t pathIndex = 0; pathIndex < aElementContext.mNumTags;  pathIndex++)
        {
            pathBuilder.AdditionalTag(aElementContext.mTags[pathIndex]);
        }
    }

    pathBuilder.EndOfPath();

    err = pathBuilder.GetError();
    SuccessOrExit(err);

    if (aElementContext.mUpdateRequiredVersion != 0x0)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> conditional update");
        err = mWriter.Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Version), aElementContext.mUpdateRequiredVersion);
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
    VerifyOrExit(kState_EncodingDataElement == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter.EndContainer(mDataElementOuterContainerType);
    SuccessOrExit(err);

    MoveToState(kState_EncodingDataList);

exit:
    WeaveLogFunctError(err);

    return err;
}

void UpdateEncoder::RemoveInProgressPrivateItemsAfter(uint16_t aItemInProgress)
{
    int count = 0;
    TraitPathStore &list = *(mContext->mInProgressUpdateList);

    for (size_t i = list.GetNextValidItem(aItemInProgress);
            i < list.GetPathStoreSize();
            i = list.GetNextValidItem(i))
    {
        if (list.AreFlagsSet(i, SubscriptionClient::kFlag_Private))
        {
            list.RemoveItemAt(i);
            count++;
        }
    }

    if (count > 0)
    {
        list.Compact();
    }

    WeaveLogDetail(DataManagement, "Removed %d private InProgress items after %u; numItems: %u",
            count, aItemInProgress, list.GetNumItems());
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
WEAVE_ERROR UpdateClient::SendUpdate(bool aIsPartialUpdate, PacketBuffer *aBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    nl::Weave::TLV::TLVReader reader;
    UpdateRequest::Parser parser;
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    VerifyOrExit(NULL != aBuf, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(kState_Initialized == mState, err = WEAVE_ERROR_INCORRECT_STATE);

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
    reader.Init(aBuf);
    reader.Next();
    parser.Init(reader);
    parser.CheckSchemaValidity();
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    if (aIsPartialUpdate)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> Partial update");
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_PartialUpdateRequest, aBuf,
                           nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        aBuf = NULL;
        SuccessOrExit(err);

        mUpdateRequestIndex ++;
        WeaveLogDetail(DataManagement, "mUpdateRequestIndex: %" PRIu32 "", mUpdateRequestIndex);
    }
    else
    {
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_UpdateRequest, aBuf,
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

void UpdateEncoder::MoveToState(const UpdateEncoderState aTargetState)
{
    mState = aTargetState;
    WeaveLogDetail(DataManagement, "UE moving to [%10.10s]", GetStateStr());
}

/**
 * Add a private path in the list of paths in progress,
 * inserting it after the one being encoded right now.
 */
WEAVE_ERROR UpdateEncoder::InsertInProgressUpdateItem(const TraitPath &aItem,
                                                      const TraitSchemaEngine * const aSchemaEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath traitPath;
    TraitPathStore::Flags flags = (SubscriptionClient::kFlag_Private | SubscriptionClient::kFlag_ForceMerge);

    err = mContext->mInProgressUpdateList->InsertItemAfter(mContext->mItemInProgress, aItem, flags);
    SuccessOrExit(err);

exit:
    WeaveLogDetail(DataManagement, "%s %u t%u, p%u  numItems: %u, err %d", __func__,
            mContext->mItemInProgress,
            aItem.mTraitDataHandle, aItem.mPropertyPathHandle,
            mContext->mInProgressUpdateList->GetNumItems(), err);

    return err;
}


#if WEAVE_DETAIL_LOGGING
const char * UpdateEncoder::GetStateStr() const
{
    switch (mState)
    {
    case kState_Uninitialized:
        return "Uninitialized";
    case kState_Ready:
        return "Ready";
    case kState_EncodingDataList:
        return "EncDataList";
    case kState_EncodingDataElement:
        return "EncDataElement";
    case kState_Done:
        return "Done";
    }
    return "N/A";
}
#endif // WEAVE_DETAIL_LOGGING

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

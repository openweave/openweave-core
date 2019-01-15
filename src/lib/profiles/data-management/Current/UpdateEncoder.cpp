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
 *      This file implements the WDM Update encoder.
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
#include <Weave/Support/WeaveFaultInjection.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * Utility function that finds an TraitUpdatableDataSink in a TraitDataSink catalog.
 *
 * @param[in] aTraitDataHandle  Handle of the Sink to lookup.
 * @param[in] aDataSinkCatalog  Catalog to search.
 * @return      A pointer to the TraitUpdatableDataSink; NULL if the handle does not exist or
 *              it points to a non updatable TraitDataSink.
 */
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
    return updatableDataSink;
}

/**
 * Encode a WDM Update request payload. See UpdateEncoder::Context.
 * The PacketBuffer's data length is updated only in case of success, but the buffer
 * contents are not preserved.
 *
 * @retval WEAVE_NO_ERROR    At least one DataElement was encoded in the payload's DataList.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL  The first DataElement could not fit in the payload.
 * @retval WEAVE_ERROR_INVALID_ARGUMENT  aContext was initialized with invalid values.
 * @retval other             Other errors from lower level objects (TLVWriter, SchemaEngine, etc).
 */
WEAVE_ERROR UpdateEncoder::EncodeRequest(Context &aContext)
{
    WEAVE_ERROR err;

    mContext = &aContext;

    VerifyOrExit(NULL != mContext->mBuf, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mContext->mNumDataElementsAddedToPayload = 0;

    err = EncodePreamble();
    SuccessOrExit(err);

    err = EncodeDataList();
    SuccessOrExit(err);

    err = EndUpdateRequest();
    SuccessOrExit(err);

exit:
    mContext = NULL;

    return err;
}

/**
 * Add a private path in the list of paths in progress,
 * inserting it after the one being encoded at the moment.
 * This method is meant to be called by the SchemaEngine as it
 * traverses the schema tree and it needs to push dictionaries
 * back to the list.
 *
 * @param[in]   aItem   The TraitPath to insert in the list being encoded.
 *
 * @retval WEAVE_NO_ERROR   The item was inserted successfully.
 * @retval WEAVE_NO_MEMORY  There was no space in the TraitPathStore to insert the item.
 */
WEAVE_ERROR UpdateEncoder::InsertInProgressUpdateItem(const TraitPath &aItem)
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

/**
 * Starts the outermost container and encodes the fields that precede the DataList
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Was unable to initialize the update.
 */
WEAVE_ERROR UpdateEncoder::EncodePreamble()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mWriter.Init(mContext->mBuf, mContext->mMaxPayloadSize);

    err = mWriter.StartContainer(TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure,
                                 mPayloadOuterContainerType);
    SuccessOrExit(err);

    if (mContext->mExpiryTimeMicroSecond != 0)
    {
        err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_ExpiryTime),
                          mContext->mExpiryTimeMicroSecond);
        SuccessOrExit(err);
    }

    err = mWriter.Put(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_UpdateRequestIndex),
                      mContext->mUpdateRequestIndex);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Encodes the DataList
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL if the buffer can't fit any DataElements.
 * @retval #WEAVE_ERROR_WDM_SCHEMA_MISMATCH in case of schema related errors.
 * @retval other Other errors from lower level objects.
 */
WEAVE_ERROR UpdateEncoder::EncodeDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mWriter.StartContainer(nl::Weave::TLV::ContextTag(UpdateRequest::kCsTag_DataList),
                                 nl::Weave::TLV::kTLVType_Array, mDataListOuterContainerType);
    SuccessOrExit(err);

    err = EncodeDataElements();
    SuccessOrExit(err);

    err = mWriter.EndContainer(mDataListOuterContainerType);
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * Encodes the DataElements; advances mContext.mInProgressUpdateList accordingly.
 *
 * @retval #WEAVE_NO_ERROR If at least one DataElement could fit in the payload.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL if the buffer can't fit any DataElements.
 * @retval #WEAVE_ERROR_WDM_SCHEMA_MISMATCH in case of schema related errors.
 * @retval other Other errors from lower level objects.
 */
WEAVE_ERROR UpdateEncoder::EncodeDataElements()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
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

        err = EncodeDataElement();
        SuccessOrExit(err);

        dictionaryOverflowed = (mContext->mNextDictionaryElementPathHandle != kNullPropertyPathHandle);
        if (dictionaryOverflowed)
        {
            TraitPath traitPath;
            traitPathList.GetItemAt(i, traitPath);
            InsertInProgressUpdateItem(traitPath);
        }

        i++;

        VerifyOrExit(!dictionaryOverflowed, /* no error */);
    }

exit:

    if (mContext->mNumDataElementsAddedToPayload > 0 &&
            (err == WEAVE_ERROR_BUFFER_TOO_SMALL))
    {
        WeaveLogDetail(DataManagement, "DataElement didn't fit; will try again later");
        RemoveInProgressPrivateItemsAfter(*(mContext->mInProgressUpdateList), mContext->mItemInProgress);
        err = WEAVE_NO_ERROR;
    }

    return err;

}

/**
 * Encodes a DataElement.
 * If the DataElement is a dictionary, it resumes encoding from mContext->mNextDictionaryElementPathHandle.
 * If the dictionary overflows the buffer, mContext->mNextDictionaryElementPathHandle is updated accordingly.
 * This method does all the lookups required and passes everything to
 * EncodeElementPath and EncodeElementData.
 *
 * If the DataElement cannot be encoded successfully, the TLV writer is rolled back.
 *
 * @retval #WEAVE_NO_ERROR In case of success.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL if the current DataElement can't fit in the buffer.
 * @retval #WEAVE_ERROR_WDM_SCHEMA_MISMATCH in case of schema related errors.
 * @retval other Other errors from TLVWriter or SchemaEngine.
 */
WEAVE_ERROR UpdateEncoder::EncodeDataElement()
{
    WEAVE_ERROR err;
    TLV::TLVWriter checkpoint;
    DataElementDataContext dataContext;
    DataElementPathContext pathContext;

    Checkpoint(checkpoint);

    mContext->mInProgressUpdateList->GetItemAt(mContext->mItemInProgress, dataContext.mTraitPath);

    dataContext.mDataSink = Locate(dataContext.mTraitPath.mTraitDataHandle, mContext->mDataSinkCatalog);
    VerifyOrExit(NULL != dataContext.mDataSink, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);

    dataContext.mSchemaEngine = dataContext.mDataSink->GetSchemaEngine();
    VerifyOrExit(dataContext.mSchemaEngine != NULL, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);

    pathContext.mProfileId = dataContext.mSchemaEngine->GetProfileId();

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestBadProfile,
                       pathContext.mProfileId = 0xFFFFFFFF);

    err = mContext->mDataSinkCatalog->GetResourceId(dataContext.mTraitPath.mTraitDataHandle,
                                                    pathContext.mResourceId);
    SuccessOrExit(err);

    err = mContext->mDataSinkCatalog->GetInstanceId(dataContext.mTraitPath.mTraitDataHandle,
                                                    pathContext.mInstanceId);
    SuccessOrExit(err);

    dataContext.mUpdateRequiredVersion = dataContext.mDataSink->GetUpdateRequiredVersion();
    dataContext.mNextDictionaryElementPathHandle = mContext->mNextDictionaryElementPathHandle;

    {
        uint64_t tags[dataContext.mSchemaEngine->mSchema.mTreeDepth];

        pathContext.mTags = &(tags[0]);
        err = dataContext.mSchemaEngine->GetRelativePathTags(dataContext.mTraitPath.mPropertyPathHandle,
                pathContext.mTags,
                dataContext.mSchemaEngine->mSchema.mTreeDepth,
                pathContext.mNumTags);
        SuccessOrExit(err);

        dataContext.mForceMerge = mContext->mInProgressUpdateList->AreFlagsSet(mContext->mItemInProgress, SubscriptionClient::kFlag_ForceMerge);

        if (dataContext.mSchemaEngine->IsDictionary(dataContext.mTraitPath.mPropertyPathHandle) &&
                false == dataContext.mForceMerge)
        {
            // If the property being updated is a dictionary, we need to use the "replace"
            // scheme explicitly so that the whole property is replaced on the responder.
            // So, the path has to point to the parent of the dictionary.
            VerifyOrExit(pathContext.mNumTags > 0, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);
            pathContext.mNumTags--;
        }

        err = mWriter.StartContainer(nl::Weave::TLV::AnonymousTag,
                                     nl::Weave::TLV::kTLVType_Structure, mDataElementOuterContainerType);
        SuccessOrExit(err);

        err = EncodeElementPath(pathContext, mWriter);
        SuccessOrExit(err);

        err = EncodeElementData(dataContext, mWriter);
        SuccessOrExit(err);

        mContext->mNextDictionaryElementPathHandle = dataContext.mNextDictionaryElementPathHandle;

        err = mWriter.EndContainer(mDataElementOuterContainerType);
        SuccessOrExit(err);

        mContext->mNumDataElementsAddedToPayload++;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Rollback(checkpoint);
    }

    return err;
}

/**
 * Encodes the path of the DataElement.
 *
 * @param[in] aElementContext   See @DataElementPathContext
 * @param[in] aWriter           The TLVWriter to use.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL In case the buffer is too small.
 */
WEAVE_ERROR UpdateEncoder::EncodeElementPath(const DataElementPathContext &aElementContext, TLV::TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Path::Builder pathBuilder;

    err = pathBuilder.Init(&aWriter, nl::Weave::TLV::ContextTag(DataElement::kCsTag_Path));
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

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * Encodes the data of the DataElement.
 *
 * @param[in] aElementContext   See @DataElementDataContext
 * @param[in] aWriter           The writer to use
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL In case the buffer is too small.
 */
WEAVE_ERROR UpdateEncoder::EncodeElementData(DataElementDataContext &aElementContext, TLV::TLVWriter &aWriter)
{
    WEAVE_ERROR err;
    bool isDictionary = false;
    bool isDictionaryReplace = false;
    nl::Weave::TLV::TLVType dataContainerType;
    uint64_t tag = nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data);

    if (aElementContext.mUpdateRequiredVersion != 0x0)
    {
        WeaveLogDetail(DataManagement, "<UC:Run> conditional update");
        err = aWriter.Put(nl::Weave::TLV::ContextTag(DataElement::kCsTag_Version), aElementContext.mUpdateRequiredVersion);
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDetail(DataManagement, "<UC:Run> unconditional update");
    }

    WeaveLogDetail(DataManagement, "<EncodeElementData> with property path handle 0x%08x",
            aElementContext.mTraitPath.mPropertyPathHandle);

    isDictionary = aElementContext.mSchemaEngine->IsDictionary(aElementContext.mTraitPath.mPropertyPathHandle);

    if (false == isDictionary)
    {
        VerifyOrExit(aElementContext.mNextDictionaryElementPathHandle == kNullPropertyPathHandle,
                err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);
    }

    if (isDictionary && (false == aElementContext.mForceMerge))
    {
        isDictionaryReplace = true;
    }

    if (isDictionaryReplace)
    {
        // If the element is a whole dictionary, use the "replace" scheme.
        // The path of the DataElement points to the parent of the dictionary.
        // The data has to be a structure with one element, which is the dictionary itself.
        WeaveLogDetail(DataManagement, "<EncodeElementData> replace dictionary");
        err = aWriter.StartContainer(tag, nl::Weave::TLV::kTLVType_Structure, dataContainerType);
        SuccessOrExit(err);

        tag = aElementContext.mSchemaEngine->GetTag(aElementContext.mTraitPath.mPropertyPathHandle);
    }

    err = aElementContext.mDataSink->ReadData(aElementContext.mTraitPath.mTraitDataHandle,
                                      aElementContext.mTraitPath.mPropertyPathHandle,
                                      tag,
                                      aWriter,
                                      aElementContext.mNextDictionaryElementPathHandle);
    SuccessOrExit(err);

    if (isDictionaryReplace)
    {
        err = aWriter.EndContainer(dataContainerType);
        SuccessOrExit(err);
    }

exit:
    return err;
}

/**
 * End the construction of the update request.
 *
 * @retval #WEAVE_NO_ERROR On success.
 * @retval other           Unable to construct the end of the update request.
 */
WEAVE_ERROR UpdateEncoder::EndUpdateRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mWriter.EndContainer(mPayloadOuterContainerType);
    SuccessOrExit(err);

    err = mWriter.Finalize();
    SuccessOrExit(err);

exit:

    WeaveLogFunctError(err);

    return err;
}

/**
 * Removes any private TraitPath after the one specified.
 * The path list is compacted after this operation.
 * This is used to remove any private path added while encoding the current DataElement,
 * in case the DataElement does not fit and has to be processed again later.
 *
 * @param[in] aList            The list to edit.
 * @param[in] aItemInProgress  The index after which all private items are removed.
 */
void UpdateEncoder::RemoveInProgressPrivateItemsAfter(TraitPathStore &aList, size_t aItemInProgress)
{
    int count = 0;

    for (size_t i = aList.GetNextValidItem(aItemInProgress);
            i < aList.GetPathStoreSize();
            i = aList.GetNextValidItem(i))
    {
        if (aList.AreFlagsSet(i, SubscriptionClient::kFlag_Private))
        {
            aList.RemoveItemAt(i);
            count++;
        }
    }

    if (count > 0)
    {
        aList.Compact();
    }

    WeaveLogDetail(DataManagement, "Removed %d private InProgress items after %u; numItems: %u",
            count, aItemInProgress, aList.GetNumItems());
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_UPDATE

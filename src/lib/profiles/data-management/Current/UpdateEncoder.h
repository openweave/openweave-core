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
 *      This file defines the generic Update Client for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_UPDATE_ENCODER_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_UPDATE_ENCODER_CURRENT_H

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Profiles/data-management/EventLoggingTypes.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {


TraitUpdatableDataSink *Locate(TraitDataHandle aTraitDataHandle, const TraitCatalogBase<TraitDataSink> * aDataSinkCatalog);

/**
 * This object encodes WDM UpdateRequest and PartialUpdateRequest payloads.
 * Note that both requests have the same format; they are differentiated only
 * by the message type, which is outside the scope of this object.
 *
 * The encoding is done synchronously by the EncodeRequest method.
 * The only other public method is InsertInProgressUpdateItem, which is
 * called by the SchemaEngine when it needs to push a dictionary back to the queue.
 */
class UpdateEncoder
{
public:
    UpdateEncoder() { }
    ~UpdateEncoder() { }

    /**
     * This structure holds the I/O arguments to the EncodeRequest method.
     */
    struct Context
    {
        Context()
            : mBuf(NULL), mMaxPayloadSize(0), mUpdateRequestIndex(0), mExpiryTimeMicroSecond(0),
                    mItemInProgress(0), mInProgressUpdateList(NULL), mNextDictionaryElementPathHandle(kNullPropertyPathHandle),
                    mDataSinkCatalog(NULL), mNumDataElementsAddedToPayload(0)
        {  }

        // Destination buffer
        PacketBuffer *mBuf;                      /**< The output buffer. In case of failure the PacketBuffer's data length
                                                      is not updated, but the buffer contents are not preserved. */
        uint32_t mMaxPayloadSize;                /**< The maximum number of bytes to write. */

        // Other fields of the payload
        uint32_t mUpdateRequestIndex;            /**< The value of the UpdateRequestIndex field for this request. */
        utc_timestamp_t mExpiryTimeMicroSecond;  /**< The value of the ExpiryTimeMicroSecond field for this request.
                                                      It is encoded only if different than 0 */

        // What to encode
        size_t mItemInProgress;                  /**< Input: the index of the item of mInProgressUpdateList to start
                                                      encoding from.
                                                      Output: Upon returning, if the whole path list fit in the payload,
                                                      this field equals mInProgressUpdateList->GetPathStoreSize().
                                                      Otherwise, the index of the item to start the next payload from.*/

        TraitPathStore *mInProgressUpdateList;   /**< The list of TraitPaths to encode. */
        PropertyPathHandle mNextDictionaryElementPathHandle; /**< Input: if the encoding starts with a dictionary being
                                                                 resumed, this field holds the property path of the next
                                                                 dictionary item to encode. Otherwise, this field should be
                                                                 kNullPropertyPathHandle.
                                                                 Output: if the last DataElement encoded is a dictionary and
                                                                 not all items fit in the payload, this field holds the property
                                                                 path handle of the item to start from for the next payload. */


        const TraitCatalogBase<TraitDataSink> * mDataSinkCatalog; /**< Input: The catalog of TraitDataSinks which the
                                                                       TraitPaths refer to. */

        // Other
        size_t mNumDataElementsAddedToPayload;    /**< Output: The number of items encoded in the payload. */
    };

    WEAVE_ERROR EncodeRequest(Context &aContext);

    WEAVE_ERROR InsertInProgressUpdateItem(const TraitPath &aItem);

private:

    /**
     * The context used to encode the path of a DataElement.
     */
    struct DataElementPathContext
    {
        DataElementPathContext()
            : mProfileId(0), mResourceId(0), mInstanceId(0), mNumTags(0),
              mTags(NULL), mSchemaVersionRange(NULL)
        { }

        uint32_t mProfileId;                    /**< Profile ID of the data sink. */
        ResourceIdentifier mResourceId;         /**< Resource ID of the data sink; if 0 it is not encoded
                                                     and defaults to the resource ID of the publisher. */
        uint64_t mInstanceId;                   /**< Instance ID of the data sink; if 0 it is not encoded
                                                     and defaults to the first instance of the trait in the publisher. */
        uint32_t mNumTags;                      /**< Number of tags in the tags array mTags. */
        uint64_t *mTags;                        /**< Array of tags to be encoded in the path. */
        const SchemaVersionRange * mSchemaVersionRange; /**< Schema version range. */
    };

    /**
     * The context used to encode the data of a DataElement.
     */
    struct DataElementDataContext
    {
        DataElementDataContext() { memset(this, 0, sizeof(*this)); }

        TraitPath mTraitPath;                   /**< The TraitPath to encode. */
        DataVersion mUpdateRequiredVersion;     /**< If the update is conditional, the version the update is based off. */
        bool mForceMerge;                       /**< True if the property is a dictionary and should be encoded as a merge. */
        TraitUpdatableDataSink *mDataSink;      /**< DataSink the TraitPath refers to. */
        const TraitSchemaEngine *mSchemaEngine; /**< The TraitSchemaEngine of the data sink. */
        PropertyPathHandle mNextDictionaryElementPathHandle; /**< See @UpdateEncoder::Context */
    };

    WEAVE_ERROR EncodePreamble();
    WEAVE_ERROR EncodeDataList(void);
    WEAVE_ERROR EncodeDataElements();
    WEAVE_ERROR EncodeDataElement();
    static WEAVE_ERROR EncodeElementPath(const DataElementPathContext &aElementContext, TLV::TLVWriter &aWriter);
    static WEAVE_ERROR EncodeElementData(DataElementDataContext &aElementContext, TLV::TLVWriter &aWriter);
    WEAVE_ERROR EndUpdateRequest(void);

    void Checkpoint(TLV::TLVWriter &aWriter) { aWriter = mWriter; }
    void Rollback(TLV::TLVWriter &aWriter) { mWriter = aWriter; }

    static void RemoveInProgressPrivateItemsAfter(TraitPathStore &aList, size_t aItemInProgress);

    Context *mContext;

    TLV::TLVWriter mWriter;
    nl::Weave::TLV::TLVType mPayloadOuterContainerType, mDataListOuterContainerType, mDataElementOuterContainerType;

};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

#endif // _WEAVE_DATA_MANAGEMENT_UPDATE_ENCODER_CURRENT_H

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

class UpdateEncoder
{
public:
    UpdateEncoder() : mState(kState_Uninitialized) { }
    ~UpdateEncoder() { }

    struct Context
    {
        Context() { memset(this, 0, sizeof(*this)); }

        // Destination buffer
        PacketBuffer *mBuf;
        uint32_t mMaxPayloadSize;

        // Other fields of the payload
        uint32_t mUpdateRequestIndex;
        utc_timestamp_t mExpiryTimeMicroSecond;

        // What to encode
        size_t mItemInProgress;
        TraitPathStore *mInProgressUpdateList;
        PropertyPathHandle mNextDictionaryElementPathHandle;
        TraitCatalogBase<TraitDataSink> * mDataSinkCatalog;

        // Other
        size_t mNumDataElementsAddedToPayload;
    };

    struct DataElementContext
    {
        DataElementContext() { memset(this, 0, sizeof(*this)); }

        TraitPath mTraitPath;
        bool mForceMerge;
        TraitUpdatableDataSink *mDataSink;
        const TraitSchemaEngine *mSchemaEngine;
        uint32_t mProfileId;
        ResourceIdentifier mResourceId;
        uint64_t mInstanceId;
        uint32_t mNumTags;
        uint64_t *mTags;
        const SchemaVersionRange * mSchemaVersionRange;
        DataVersion mUpdateRequiredVersion;
    };

    WEAVE_ERROR InsertInProgressUpdateItem(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine);
    void RemoveInProgressPrivateItemsAfter(uint16_t aItemInProgress);

    WEAVE_ERROR EncodeRequest(Context *aContext);

    typedef WEAVE_ERROR (*AddElementCallback)(UpdateEncoder * aEncoder, void *apCallState, TLV::TLVWriter & aOuterWriter);

    WEAVE_ERROR StartUpdate();

    WEAVE_ERROR BuildSingleUpdateRequestDataList();

    WEAVE_ERROR DirtyPathToDataElement();

    WEAVE_ERROR EncodeElement(const DataElementContext &aElementContext);

    WEAVE_ERROR StartElement(const DataElementContext &aElementContext);

    WEAVE_ERROR FinalizeElement();

    WEAVE_ERROR CancelElement(TLV::TLVWriter & aOuterWriter);

    void Checkpoint(TLV::TLVWriter &aWriter);

    void Rollback(TLV::TLVWriter &aWriter);

    WEAVE_ERROR StartDataList(void);

    WEAVE_ERROR EndDataList(void);

    WEAVE_ERROR StartUpdateRequest();

    WEAVE_ERROR FinishUpdate();

    WEAVE_ERROR EndUpdateRequest(void);

    WEAVE_ERROR AddExpiryTime(utc_timestamp_t aExpiryTimeMicroSecond);

    WEAVE_ERROR AddUpdateRequestIndex(void);

private:
    enum UpdateEncoderState
    {
        kState_Uninitialized,        //< The encoder has not been initialized
        kState_Ready,                //< The encoder has been initialized and is ready
        kState_EncodingDataList,     //< The encoder has opened the DataList; DataElements can be added
        kState_EncodingDataElement,  //< The encoder is encoding a DataElement
        kState_Done,                 //< The DataList has been closed
    };

    void MoveToState(const UpdateEncoderState aTargetState);
    const char * GetStateStr(void) const;

    Context *mContext;

    TLV::TLVWriter mWriter;
    UpdateEncoderState mState;
    nl::Weave::TLV::TLVType mDataListOuterContainerType, mDataElementOuterContainerType;

};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_UPDATE_ENCODER_CURRENT_H

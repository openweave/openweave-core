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
 *      This file forms the crux of the TDM layer (trait data management), providing
 *      various classes that manage and process data as it applies to traits and their
 *      associated schemas.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <Weave/Support/RandUtils.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

UpdateDirtyPathFilter::UpdateDirtyPathFilter(SubscriptionClient *apSubClient,
                                             TraitDataHandle traitDataHandle,
                                             const TraitSchemaEngine * aEngine)
{
    mpSubClient = apSubClient;
    mTraitDataHandle = traitDataHandle;
    mSchemaEngine = aEngine;
}

bool UpdateDirtyPathFilter::FilterPath (PropertyPathHandle pathhandle)
{
    bool retval = false;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    if (mpSubClient)
    {
        // TODO: clean this up:
        // mpSubClient is set to something only by UpdatableDataSink instances
        retval = mpSubClient->FilterNotifiedPath(mTraitDataHandle, pathhandle, mSchemaEngine);
    }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    return retval;
}

UpdateDictionaryDirtyPathCut::UpdateDictionaryDirtyPathCut(TraitDataHandle aTraitDataHandle, UpdateEncoder * apEncoder)
{
    mpUpdateEncoder = apEncoder;
    mTraitDataHandle = aTraitDataHandle;
}

WEAVE_ERROR UpdateDictionaryDirtyPathCut::CutPath (PropertyPathHandle aPathhandle, const TraitSchemaEngine * apEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // TODO: rename this struct, and pass apEngine to the constructor.
    // Probably just replace the struct with a function; I don't see the point of it really.
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    err = mpUpdateEncoder->InsertInProgressUpdateItem(TraitPath(mTraitDataHandle, aPathhandle));
    WeaveLogDetail(DataManagement, "Cut dictionary %u, %u; err %d", mTraitDataHandle, aPathhandle, err);
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    return err;
}

WEAVE_ERROR TraitSchemaEngine::MapPathToHandle(TLVReader & aPathReader, PropertyPathHandle & aHandle) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PropertyPathHandle childProperty, curProperty;
    nl::Weave::TLV::TLVType dummyContainerType = kTLVType_Path;

    // initialize the out argument to NULL
    aHandle = kNullPropertyPathHandle;

    // Set our starting point for traversal to the root node.
    curProperty = kRootPropertyPathHandle;

    // Descend into our schema tree using the tags encountered to help navigate through the various branches.
    while ((err = aPathReader.Next()) == WEAVE_NO_ERROR)
    {
        const uint64_t tag = aPathReader.GetTag();

        // If it's a profile tag, we know we're dealing with a dictionary item - get the appropriate dictionary item. Otherwise,
        // treat it like a regular child node.
        if (IsProfileTag(tag))
        {
            VerifyOrExit(ProfileIdFromTag(tag) == kWeaveProfile_DictionaryKey, err = WEAVE_ERROR_INVALID_TLV_TAG);
            childProperty = GetDictionaryItemHandle(curProperty, TagNumFromTag(tag));
        }
        else
        {
            childProperty = GetChildHandle(curProperty, TagNumFromTag(tag));
        }

        if (IsNullPropertyPathHandle(childProperty))
        {
            err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
            break;
        }

        // Set the current node.
        curProperty = childProperty;
    }

    // End of TLV is the only expected error here and if so, correctly update the handle passed in by the caller.
    if (err == WEAVE_END_OF_TLV)
    {
        err = aPathReader.ExitContainer(dummyContainerType);
        SuccessOrExit(err);

        aHandle = curProperty;
        err     = WEAVE_NO_ERROR;
    }

exit:
    return err;
}

WEAVE_ERROR TraitSchemaEngine::MapHandleToPath(PropertyPathHandle aHandle, TLVWriter & aPathWriter) const
{
    // Use the tree depth specified by the schema to correctly size our path walk store
    PropertyPathHandle pathWalkStore[mSchema.mTreeDepth];
    uint32_t pathWalkDepth         = 0;
    WEAVE_ERROR err                = WEAVE_NO_ERROR;
    PropertyPathHandle curProperty = aHandle;

    // Walk up the path till root and keep track of the handles encountered along the way.
    while (curProperty != kRootPropertyPathHandle)
    {
        pathWalkStore[pathWalkDepth++] = curProperty;
        curProperty                    = GetParent(curProperty);
    }

    // Write it into TLV by reverse walking over the encountered handles starting from root.
    while (pathWalkDepth)
    {
        PropertyPathHandle curHandle = pathWalkStore[pathWalkDepth - 1];

        err = aPathWriter.PutNull(GetTag(curHandle));
        SuccessOrExit(err);

        pathWalkDepth--;
    }

exit:
    return WEAVE_NO_ERROR;
}

uint64_t TraitSchemaEngine::GetTag(PropertyPathHandle aHandle) const
{
    if (IsDictionary(GetParent(aHandle)))
    {
        return ProfileTag(kWeaveProfile_DictionaryKey, GetPropertyDictionaryKey(aHandle));
    }
    else
    {
        return ContextTag(GetMap(aHandle)->mContextTag);
    }
}

WEAVE_ERROR TraitSchemaEngine::RetrieveData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                            IGetDataDelegate * aDelegate, IDirtyPathCut * updateDirtyPathCut) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (IsLeaf(aHandle) || IsNullable(aHandle) || IsOptional(aHandle))
    {
        bool isPresent = true, isNull = false;

        err = aDelegate->GetData(aHandle, aTagToWrite, aWriter, isNull, isPresent);
        SuccessOrExit(err);

        if (!isPresent && !(IsOptional(aHandle) || IsEphemeral(aHandle)))
        {
            err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH;
        }
        VerifyOrExit(isPresent, /* no-op, either no error or set above */);

        if (isNull)
        {
            if (!IsNullable(aHandle))
            {
                err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH;
            }
            else
            {
                err = aWriter.PutNull(aTagToWrite);
                SuccessOrExit(err);
            }
        }
        VerifyOrExit(!isNull, /* no-op, either no error or set above */);
    }

    if (!IsLeaf(aHandle))
    {
        TLVType type;

        err = aWriter.StartContainer(aTagToWrite, kTLVType_Structure, type);
        SuccessOrExit(err);

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        if (IsDictionary(aHandle))
        {
            PropertyDictionaryKey dictionaryItemKey;
            uintptr_t context = 0;
            if (NULL == updateDirtyPathCut)
            {

                // TODO: this looks like RetrieveUpdatableDictionaryData; must avoid the duplication

                // if it's a dictionary, we need to iterate through the items in the container by asking our delegate.
                while ((err = aDelegate->GetNextDictionaryItemKey(aHandle, context, dictionaryItemKey)) == WEAVE_NO_ERROR)
                {
                    uint64_t tag                    = ProfileTag(kWeaveProfile_DictionaryKey, dictionaryItemKey);
                    PropertySchemaHandle itemHandle = GetFirstChild(aHandle);

                    VerifyOrExit(itemHandle != kNullPropertyPathHandle, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);

                    err = RetrieveData(CreatePropertyPathHandle(itemHandle, dictionaryItemKey), tag, aWriter, aDelegate);
                    SuccessOrExit(err);
                }

                VerifyOrExit(err == WEAVE_END_OF_INPUT, );
                err = WEAVE_NO_ERROR;
            }
            else
            {
                // Looks like we come down to a dictionary during a recursion started by TraitUpdatableDataSink::ReadData,
                // because updateDirtyPathCut is not NULL.
                // The dictionary is supposed to be replaced completely.
                // The way this is implemented is:
                // - an empty dictionary is encoded here;
                // - the handle of the dictionary is put back in the queue.
                // The empty dictionary is encoded here because if the dictionary is not a child of the
                // current DataElement's path, it cannot be omitted.
                // The dictionary will be encoded in a new DataElement as a "merge".
                // The reason for that is that if the dictionary is too large to fit in the payload,
                // it's easier to split it in more than one data element outside of a recursion.
                if (aDelegate->GetNextDictionaryItemKey(aHandle, context, dictionaryItemKey) == WEAVE_NO_ERROR)
                {
                    err = updateDirtyPathCut->CutPath(aHandle, this);
                    SuccessOrExit(err);
                }
            }
        }
        else
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        {
            PropertyPathHandle childProperty;

            // Recursively iterate over all child nodes and call RetrieveData on them.
            for (childProperty = GetFirstChild(aHandle); !IsNullPropertyPathHandle(childProperty);
                 childProperty = GetNextChild(aHandle, childProperty))
            {
                const PropertyInfo * childInfo = GetMap(childProperty);

                err = RetrieveData(childProperty, ContextTag(childInfo->mContextTag), aWriter, aDelegate, updateDirtyPathCut);
                SuccessOrExit(err);
            }
        }

        err = aWriter.EndContainer(type);
        SuccessOrExit(err);
    } // if (!IsLeaf(aHandle))

exit:
    return err;
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
WEAVE_ERROR TraitSchemaEngine::RetrieveUpdatableDictionaryData(PropertyPathHandle aHandle,
                                                               uint64_t aTagToWrite,
                                                               TLVWriter & aWriter,
                                                               IGetDataDelegate * aDelegate,
                                                               PropertyPathHandle & aPropertyPathHandleOfDictItemToStartFrom) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    nl::Weave::TLV::TLVType dataContainerType;
    PropertyDictionaryKey dictionaryItemKey;
    uintptr_t context = 0;
    uint32_t numKeysEncoded = 0;
    PropertySchemaHandle dictionaryItemSchemaHandle = GetPropertySchemaHandle(GetFirstChild(aHandle));
    PropertyPathHandle dictionaryItemPathHandle;
    PropertyPathHandle itemToSkipTo = aPropertyPathHandleOfDictItemToStartFrom;

    VerifyOrExit(IsDictionary(aHandle), err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);

    // Clear the key to start from; it will be set again below if we can't encode all remaing
    // items.
    aPropertyPathHandleOfDictItemToStartFrom = kNullPropertyPathHandle;

    err = aWriter.StartContainer(aTagToWrite,
            nl::Weave::TLV::kTLVType_Structure, dataContainerType);
    SuccessOrExit(err);

    while ((err = aDelegate->GetNextDictionaryItemKey(aHandle, context, dictionaryItemKey)) == WEAVE_NO_ERROR)
    {
        uint64_t tag = ProfileTag(kWeaveProfile_DictionaryKey, dictionaryItemKey);

        dictionaryItemPathHandle = CreatePropertyPathHandle(dictionaryItemSchemaHandle, dictionaryItemKey);

        if (dictionaryItemPathHandle < itemToSkipTo)
        {
            continue;
        }

        TLVWriter backupWriter = aWriter;

        aPropertyPathHandleOfDictItemToStartFrom = kNullPropertyPathHandle;

        err = RetrieveData(dictionaryItemPathHandle, tag, aWriter, aDelegate);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogDetail(DataManagement, "Dictionary item whith path 0x%" PRIx32 ", tag 0x% " PRIx64 " failed with error % " PRIu32 "",
                    dictionaryItemPathHandle, tag, err);
        }
        if (numKeysEncoded > 0 &&
                ((err == WEAVE_ERROR_BUFFER_TOO_SMALL) || (err == WEAVE_ERROR_NO_MEMORY)))
        {
            // BUFFER_TOO_SMALL means there is no more space in the current buffer.
            // NO_MEMORY means the application is trying to build a chain of pBufs, but
            // there are no more buffers.
            aWriter = backupWriter;
            aPropertyPathHandleOfDictItemToStartFrom = dictionaryItemPathHandle;
            err = WEAVE_NO_ERROR;
            break;
        }
        SuccessOrExit(err);

        numKeysEncoded++;
    }

    if (err == WEAVE_END_OF_INPUT)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = aWriter.EndContainer(dataContainerType);
    SuccessOrExit(err);

exit:
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    return err;
}

WEAVE_ERROR TraitSchemaEngine::GetRelativePathTags(const PropertyPathHandle aCandidateHandle,
                                                   uint64_t *aTags,
                                                   const uint32_t aTagsSize,
                                                   uint32_t &aNumTags) const
{
    PropertyPathHandle pathWalkStore[mSchema.mTreeDepth];
    uint32_t pathWalkDepth         = 0;
    PropertyPathHandle curProperty;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aNumTags = 0;

    if (aCandidateHandle != kRootPropertyPathHandle)
    {
        curProperty = aCandidateHandle;

        while (curProperty != kRootPropertyPathHandle)
        {
            pathWalkStore[pathWalkDepth++] = curProperty;
            curProperty                    = GetParent(curProperty);
        }

        VerifyOrExit(aTagsSize >= pathWalkDepth, err = WEAVE_ERROR_NO_MEMORY);

        // Write it into TLV by reverse walking over the encountered handles starting from root.
        while (pathWalkDepth)
        {
            PropertyPathHandle curHandle = pathWalkStore[pathWalkDepth - 1];
            aTags[aNumTags] = GetTag(curHandle);
            pathWalkDepth--;
            aNumTags++;
        }
    }

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

WEAVE_ERROR TraitSchemaEngine::StoreData(PropertyPathHandle aHandle, TLVReader & aReader, ISetDataDelegate * aDelegate, IPathFilter * apPathFilter) const
{
    WEAVE_ERROR err                 = WEAVE_NO_ERROR;
    TLVType type                    = kTLVType_NotSpecified;
    PropertyPathHandle curHandle    = aHandle;
    PropertyPathHandle parentHandle = kNullPropertyPathHandle;
    bool dictionaryEventSignalled   = false;
    PropertyPathHandle dictionaryItemHandle;
    bool descending = true;

    VerifyOrExit(!(apPathFilter != NULL && apPathFilter->FilterPath(curHandle)), );

    // While the logic to actually parse out dictionaries is relatively easy, the logic to appropriately emit the
    // OnReplace and OnItemModified events is not similarly so.
    //
    // This logic here deals with the case where this function was called with a path made to a dictionary *element* or deeper.
    // The logic further below deals with the cases where this function was called on a path handle at the dictionary or higher.
    if (IsInDictionary(curHandle, dictionaryItemHandle))
    {
        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryItemModifyBegin, dictionaryItemHandle);
        dictionaryEventSignalled = true;
    }

    if (IsLeaf(curHandle))
    {
        err = aDelegate->SetData(curHandle, aReader, aReader.GetType() == kTLVType_Null);
        SuccessOrExit(err);
    }
    else
    {
        // The crux of this loop is to iteratively parse out TLV and descend into the schema as necessary. The loop is bounded
        // by the return of the iterator handle (curHandle) back to the original start point (aHandle).
        //
        // The loop also has a notion of ascension and descension. Descension occurs when you go deeper into the schema tree while
        // ascension is returning back to a higher point in the tree.
        do
        {
            if (!(apPathFilter != NULL && apPathFilter->FilterPath(curHandle))) {
#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
                if (!IsNullPropertyPathHandle(curHandle))
#endif
                {
                    if (!IsLeaf(curHandle)) {
                        if (descending) {
                            bool enterContainer = (aReader.GetType() != kTLVType_Null);
                            if (enterContainer) {
                                err = aReader.EnterContainer(type);
                                SuccessOrExit(err);

                                parentHandle = curHandle;
                            } else {
                                if (IsNullable(curHandle)) {
                                    err = aDelegate->SetData(curHandle, aReader, !enterContainer);
                                } else {
                                    err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH;
                                }
                                SuccessOrExit(err);
                            }
                        }
                    } else {
                        err = aDelegate->SetData(curHandle, aReader, aReader.GetType() == kTLVType_Null);
                        SuccessOrExit(err);

                        // Setting a leaf data can be interpreted as ascension since you are evaluating another node
                        // at the same level there-after by going back up to your parent and checking for more children.
                        descending = false;
                    }
                }

                if (!descending) {
                    if (IsDictionary(curHandle)) {
                        // We can surmise this is a replace if we're ascending to a node that is a dictionary, and that node
                        // is lower than the target node this function was directed at (we can't get to this point in code if the
                        // two handles (target and current) are equivalent to each other).
                        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryReplaceEnd, curHandle);
                    } else if (IsDictionary(parentHandle)) {
                        // We can surmise this is a modify/add if we're ascending to a node whose parent is a dictionary, and that node
                        // is lower than the target node this function was directed at (we can't get to this point in code if the
                        // two handles (target and current) are equivalent to each other). Those cases are handled by the two 'if'
                        // statements at the top and bottom of this function.
                        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryItemModifyEnd, curHandle);
                    }
                }
            }

            // Get the next element in this container.
            err = aReader.Next();
            VerifyOrExit((err == WEAVE_NO_ERROR) || (err == WEAVE_END_OF_TLV), );

            if (err == WEAVE_END_OF_TLV) {
                // We've hit the end of the container - exit out and point our current handle to its parent.
                // In the process, restore the parentHandle as well.
                err = aReader.ExitContainer(type);
                SuccessOrExit(err);

                curHandle = parentHandle;
                parentHandle = GetParent(curHandle);

                descending = false;
            } else {
                const uint64_t tag = aReader.GetTag();

                descending = true;

                if (IsProfileTag(tag)) {
                    VerifyOrExit(ProfileIdFromTag(tag) == kWeaveProfile_DictionaryKey,
                                 err = WEAVE_ERROR_INVALID_TLV_TAG);
                    curHandle = GetDictionaryItemHandle(parentHandle, TagNumFromTag(tag));
                } else {
                    curHandle = GetChildHandle(parentHandle, TagNumFromTag(tag));
                }

                if (!(apPathFilter != NULL && apPathFilter->FilterPath(curHandle))) {
                    if (IsDictionary(curHandle)) {
                        // If we're descending onto a node that is a dictionary, we know for certain that it is a replace operation
                        // since the target path handle for this function was higher in the tree than the node representing the
                        // dictionary itself.
                        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryReplaceBegin, curHandle);
                    } else if (IsDictionary(parentHandle)) {
                        // Alternatively, if we're descending onto a node whose parent is a dictionary, we know that this node
                        // represents an element in the dictionary and as such, is an appropriate point in the traversal to notify the
                        // application of an upcoming dictionary item modification/insertion.
                        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryItemModifyBegin,
                                                   curHandle);
                    }
                }
#if !TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
                if (IsNullPropertyPathHandle(curHandle)) {
                    err = WEAVE_ERROR_TLV_TAG_NOT_FOUND;
                    break;
                }
#endif
            }
        } while (curHandle != aHandle);
    }

    if (dictionaryEventSignalled)
    {
        aDelegate->OnDataSinkEvent(ISetDataDelegate::kDataSinkEvent_DictionaryItemModifyEnd, dictionaryItemHandle);
    }

exit:
    return err;
}

PropertyPathHandle TraitSchemaEngine::GetFirstChild(PropertyPathHandle aParentHandle) const
{
    return GetNextChild(aParentHandle, kRootPropertyPathHandle);
}

bool TraitSchemaEngine::IsParent(PropertyPathHandle aChildHandle, PropertyPathHandle aParentHandle) const
{
    bool retval = false;

    VerifyOrExit(aChildHandle != kNullPropertyPathHandle &&
                 aParentHandle != kNullPropertyPathHandle, );

    do
    {
        aChildHandle = GetParent(aChildHandle);

        if (aChildHandle == aParentHandle)
        {
            ExitNow(retval = true);
        }
    } while (aChildHandle != kNullPropertyPathHandle);

exit:
    return retval;
}

PropertyPathHandle TraitSchemaEngine::GetParent(PropertyPathHandle aHandle) const
{
    PropertySchemaHandle schemaHandle   = GetPropertySchemaHandle(aHandle);
    PropertyDictionaryKey dictionaryKey = GetPropertyDictionaryKey(aHandle);
    const PropertyInfo * handleMap      = GetMap(schemaHandle);

    if (!handleMap)
    {
        return kNullPropertyPathHandle;
    }

    // update the schema handle to point to the parent handle.
    schemaHandle = handleMap->mParentHandle;

    // if the parent is a dictionary, just return the schema handle with the key cleared out since the key doesn't make sense
    // anymore at this level or higher.
    if (IsDictionary(schemaHandle))
    {
        return schemaHandle;
    }
    else
    {
        // Otherwise, preserve the dictionaroy key in the new path handle being created.
        return CreatePropertyPathHandle(schemaHandle, dictionaryKey);
    }

    return WEAVE_NO_ERROR;
}

PropertyPathHandle TraitSchemaEngine::GetNextChild(PropertyPathHandle aParentHandle, PropertyPathHandle aChildHandle) const
{
    unsigned int i;
    PropertySchemaHandle parentSchemaHandle   = GetPropertySchemaHandle(aParentHandle);
    PropertySchemaHandle childSchemaHandle    = GetPropertySchemaHandle(aChildHandle);
    PropertyDictionaryKey parentDictionaryKey = GetPropertyDictionaryKey(aParentHandle);

    // Starting from 1 node after the child node that's been passed in, iterate till we find the next child belonging to aParentId.
    for (i = (childSchemaHandle - 1); i < mSchema.mNumSchemaHandleEntries; i++)
    {
        if (mSchema.mSchemaHandleTbl[i].mParentHandle == parentSchemaHandle)
        {
            break;
        }
    }

    if (i == mSchema.mNumSchemaHandleEntries)
    {
        return kNullPropertyPathHandle;
    }
    else
    {
        return CreatePropertyPathHandle(i + kHandleTableOffset, parentDictionaryKey);
    }
}

PropertyPathHandle TraitSchemaEngine::GetChildHandle(PropertyPathHandle aParentHandle, uint8_t aContextTag) const
{
    if (IsDictionary(aParentHandle))
    {
        return kNullPropertyPathHandle;
    }

    return _GetChildHandle(aParentHandle, aContextTag);
}

PropertyPathHandle TraitSchemaEngine::_GetChildHandle(PropertyPathHandle aParentHandle, uint8_t aContextTag) const
{
    for (PropertyPathHandle childProperty = GetFirstChild(aParentHandle); !IsNullPropertyPathHandle(childProperty);
         childProperty                    = GetNextChild(aParentHandle, childProperty))
    {
        if (mSchema.mSchemaHandleTbl[GetPropertySchemaHandle(childProperty) - kHandleTableOffset].mContextTag == aContextTag)
        {
            return childProperty;
        }
    }

    return kNullPropertyPathHandle;
}

PropertyPathHandle TraitSchemaEngine::GetDictionaryItemHandle(PropertyPathHandle aParentHandle, uint16_t aDictionaryKey) const
{
    if (!IsDictionary(aParentHandle))
    {
        return kNullPropertyPathHandle;
    }

    return CreatePropertyPathHandle(_GetChildHandle(aParentHandle, 0), aDictionaryKey);
}

bool TraitSchemaEngine::IsLeaf(PropertyPathHandle aHandle) const
{
    PropertySchemaHandle schemaHandle = GetPropertySchemaHandle(aHandle);

    // Root is by definition not a leaf. This also conveniently handles the cases where we have traits that
    // don't have any properties in them.
    if (aHandle == kRootPropertyPathHandle)
    {
        return false;
    }
    else
    {
        for (unsigned int i = 0; i < mSchema.mNumSchemaHandleEntries; i++)
        {
            if (mSchema.mSchemaHandleTbl[i].mParentHandle == schemaHandle)
            {
                return false;
            }
        }

        return true;
    }
}

int32_t TraitSchemaEngine::GetDepth(PropertyPathHandle aHandle) const
{
    int depth                         = 0;
    PropertySchemaHandle schemaHandle = GetPropertySchemaHandle(aHandle);

    if (schemaHandle > (mSchema.mNumSchemaHandleEntries + 1))
    {
        return -1;
    }

    while (schemaHandle != kRootPropertyPathHandle)
    {
        depth++;
        schemaHandle = mSchema.mSchemaHandleTbl[schemaHandle - kHandleTableOffset].mParentHandle;
    }

    return depth;
}

PropertyPathHandle TraitSchemaEngine::FindLowestCommonAncestor(PropertyPathHandle aHandle1, PropertyPathHandle aHandle2,
                                                               PropertyPathHandle * aHandle1BranchChild,
                                                               PropertyPathHandle * aHandle2BranchChild) const
{
    int32_t depth1 = GetDepth(aHandle1);
    int32_t depth2 = GetDepth(aHandle2);
    PropertyPathHandle laggingHandle1, laggingHandle2;

    if (depth1 < 0 || depth2 < 0)
    {
        return kNullPropertyPathHandle;
    }

    laggingHandle1 = kNullPropertyPathHandle;
    laggingHandle2 = kNullPropertyPathHandle;

    while (depth1 != depth2)
    {
        if (depth1 > depth2)
        {
            laggingHandle1 = aHandle1;
            aHandle1       = GetParent(aHandle1);
            depth1--;
        }
        else
        {
            laggingHandle2 = aHandle2;
            aHandle2       = GetParent(aHandle2);
            depth2--;
        }
    }

    while (aHandle1 != aHandle2)
    {
        laggingHandle1 = aHandle1;
        laggingHandle2 = aHandle2;

        aHandle1 = GetParent(aHandle1);
        aHandle2 = GetParent(aHandle2);
    }

    if (aHandle1BranchChild)
    {
        *aHandle1BranchChild = laggingHandle1;
    }

    if (aHandle2BranchChild)
    {
        *aHandle2BranchChild = laggingHandle2;
    }

    return aHandle1;
}

const TraitSchemaEngine::PropertyInfo * TraitSchemaEngine::GetMap(PropertyPathHandle aHandle) const
{
    PropertySchemaHandle schemaHandle = GetPropertySchemaHandle(aHandle);

    if (schemaHandle < 2 || (schemaHandle >= (mSchema.mNumSchemaHandleEntries + kHandleTableOffset)))
    {
        return NULL;
    }

    return &mSchema.mSchemaHandleTbl[schemaHandle - kHandleTableOffset];
}

bool TraitSchemaEngine::IsDictionary(PropertyPathHandle aHandle) const
{
    // The 'mIsDictionaryBitfield' is only populated by code-gen on traits that do have dictionaries. Otherwise, it defaults
    // to NULL.
    return GetBitFromPathHandleBitfield(mSchema.mIsDictionaryBitfield, aHandle);
}

bool TraitSchemaEngine::IsInDictionary(PropertyPathHandle aHandle, PropertyPathHandle & aDictionaryItemHandle) const
{
    while (aHandle != kRootPropertyPathHandle)
    {
        PropertyPathHandle parentHandle = GetParent(aHandle);

        if (IsDictionary(parentHandle))
        {
            aDictionaryItemHandle = aHandle;
            return true;
        }

        aHandle = parentHandle;
    }

    return false;
}

bool TraitSchemaEngine::IsOptional(PropertyPathHandle aHandle) const
{
    return GetBitFromPathHandleBitfield(mSchema.mIsOptionalBitfield, aHandle);
}

bool TraitSchemaEngine::IsNullable(PropertyPathHandle aHandle) const
{
    return GetBitFromPathHandleBitfield(mSchema.mIsNullableBitfield, aHandle);
}

bool TraitSchemaEngine::IsEphemeral(PropertyPathHandle aHandle) const
{
    return GetBitFromPathHandleBitfield(mSchema.mIsEphemeralBitfield, aHandle);
}

bool TraitSchemaEngine::GetBitFromPathHandleBitfield(uint8_t * aBitfield, PropertyPathHandle aPathHandle) const
{
    bool retval = false;
    if (aBitfield != NULL && !IsRootPropertyPathHandle(aPathHandle) && !IsNullPropertyPathHandle(aPathHandle))
    {
        PropertySchemaHandle adjustedSchemaHandle = GetPropertySchemaHandle(aPathHandle) - kHandleTableOffset;
        retval                                    = aBitfield[(adjustedSchemaHandle) / 8] & (1 << (adjustedSchemaHandle % 8));
    }
    return retval;
}

bool TraitSchemaEngine::MatchesProfileId(uint32_t aProfileId) const
{
    return (aProfileId == mSchema.mProfileId);
}

uint32_t TraitSchemaEngine::GetProfileId(void) const
{
    return mSchema.mProfileId;
}

bool TraitSchemaEngine::GetVersionIntersection(SchemaVersionRange & aVersion, SchemaVersionRange & aIntersection) const
{
    SchemaVersion minCurrentVersion = 1;
    SchemaVersion maxCurrentVersion = 1;
#if (TDM_VERSIONING_SUPPORT)
    if (mSchema.mVersionRange != NULL)
    {
        minCurrentVersion = mSchema.mVersionRange->mMinVersion;
        maxCurrentVersion = mSchema.mVersionRange->mMaxVersion;
    }
#endif // TDM_VERSIONING_SUPPORT

    aIntersection.mMinVersion = max(aVersion.mMinVersion, minCurrentVersion);
    aIntersection.mMaxVersion = min(aVersion.mMaxVersion, maxCurrentVersion);

    if (aIntersection.mMinVersion <= aIntersection.mMaxVersion)
    {
        return true;
    }
    else
    {
        return false;
    }
}

SchemaVersion TraitSchemaEngine::GetHighestForwardVersion(SchemaVersion aVersion) const
{
    SchemaVersion currentVersion = 1;
#if (TDM_VERSIONING_SUPPORT)
    if (mSchema.mVersionRange != NULL)
    {
        currentVersion = mSchema.mVersionRange->mMaxVersion;
    }
#endif // TDM_VERSIONING_SUPPORT
    if (aVersion > currentVersion)
    {
        return 0;
    }
    else
    {
        return currentVersion;
    }
}

SchemaVersion TraitSchemaEngine::GetLowestCompatibleVersion(SchemaVersion aVersion) const
{
    SchemaVersion currentVersion = 1;
#if (TDM_VERSIONING_SUPPORT)
    if (mSchema.mVersionRange != NULL)
    {
        currentVersion = mSchema.mVersionRange->mMinVersion;
    }
#endif // TDM_VERSIONING_SUPPORT
    // TODO: current assumption is that the trait is fully backwars compatible
    return currentVersion;
}

SchemaVersion TraitSchemaEngine::GetMinVersion() const
{
    SchemaVersion currentVersion = 1;
#if (TDM_VERSIONING_SUPPORT)
    if (mSchema.mVersionRange != NULL)
    {
        currentVersion = mSchema.mVersionRange->mMinVersion;
    }
#endif // TDM_VERSIONING_SUPPORT
    return currentVersion;
}

SchemaVersion TraitSchemaEngine::GetMaxVersion() const
{
    SchemaVersion currentVersion = 1;
#if (TDM_VERSIONING_SUPPORT)
    if (mSchema.mVersionRange != NULL)
    {
        currentVersion = mSchema.mVersionRange->mMaxVersion;
    }
#endif // TDM_VERSIONING_SUPPORT
    return currentVersion;
}

void TraitDataSink::SetVersion(uint64_t aVersion)
{
    if (mHasValidVersion)
    {
        if (aVersion != mVersion)
        {
            WeaveLogDetail(DataManagement, "Trait %08x version: 0x%" PRIx64 " -> 0x%" PRIx64 "", mSchemaEngine->GetProfileId(), mVersion,
                    aVersion);
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "Trait %08x version: n/a -> 0x%" PRIx64 "", mSchemaEngine->GetProfileId(), aVersion);
    }
    mVersion = aVersion;
    mHasValidVersion = true;
}

void TraitDataSink::SetLastNotifyVersion(uint64_t aVersion)
{
    WeaveLogDetail(DataManagement, "Trait %08x last notify version: 0x%" PRIx64 " -> 0x%" PRIx64 "", mSchemaEngine->GetProfileId(), mLastNotifyVersion,
            aVersion);

    mLastNotifyVersion = aVersion;
}

TraitDataSink::OnChangeRejection TraitDataSink::sChangeRejectionCb = NULL;
void * TraitDataSink::sChangeRejectionContext                      = NULL;

TraitDataSink::TraitDataSink(const TraitSchemaEngine * aEngine)
{
    mSchemaEngine    = aEngine;
    mVersion         = 0;
    mLastNotifyVersion = 0;
    mHasValidVersion = 0;
}

WEAVE_ERROR TraitDataSink::StoreDataElement(PropertyPathHandle aHandle, TLVReader & aReader, uint8_t aFlags,
                                            OnChangeRejection aFunc, void * aContext, TraitDataHandle aDatahandle)
{
    DataElement::Parser parser;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DataVersion versionInDE;
    bool dataPresent = false, deletePresent = false;

    err = parser.Init(aReader);
    SuccessOrExit(err);

    err = parser.GetVersion(&versionInDE);
    SuccessOrExit(err);

    if (IsVersionNewer(versionInDE))
    {
        if (mHasValidVersion)
        {
            WeaveLogDetail(DataManagement, "<StoreDataElement> [Trait %08x] version: 0x%" PRIx64 " -> 0x%" PRIx64 "",
                           mSchemaEngine->GetProfileId(), mVersion, versionInDE);
        }
        else
        {
            WeaveLogDetail(DataManagement, "<StoreDataElement> [Trait %08x] version: n/a -> 0x%" PRIx64 "",
                           mSchemaEngine->GetProfileId(), versionInDE);
        }

        err = parser.CheckPresence(&dataPresent, &deletePresent);
        SuccessOrExit(err);

        if (aFlags & kFirstElementInChange)
        {
            OnEvent(kEventChangeBegin, NULL);
        }

        // Signal to the app we're about to process a data element.
        OnEvent(kEventDataElementBegin, NULL);

        // TODO: we need to check if there are pending updates for paths
        // being deleted
        if (deletePresent)
        {
            err = parser.GetDeletedDictionaryKeys(&aReader);
            SuccessOrExit(err);

            while ((err = aReader.Next()) == WEAVE_NO_ERROR)
            {
                PropertyDictionaryKey key;
                PropertyPathHandle handle;

                err = aReader.Get(key);
                SuccessOrExit(err);

                // In the case of a delete, the path is usually directed to the dictionary itself. We
                // need to get the handle to the child dictionary element handle first before we can
                // pass it up to the application.
                handle = mSchemaEngine->GetFirstChild(aHandle);
                VerifyOrExit(handle != kNullPropertyPathHandle, err = WEAVE_ERROR_INVALID_ARGUMENT);

                handle = CreatePropertyPathHandle(GetPropertySchemaHandle(handle), key);
                OnEvent(kEventDictionaryItemDelete, &handle);
            }

            VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );
            err = WEAVE_NO_ERROR;
        }

        if (aHandle != kNullPropertyPathHandle && dataPresent)
        {
            err = parser.GetData(&aReader);
            SuccessOrExit(err);

            UpdateDirtyPathFilter pathFilter(GetSubscriptionClient(), aDatahandle, mSchemaEngine);
            err = mSchemaEngine->StoreData(aHandle, aReader, this, &pathFilter);
        }

        OnEvent(kEventDataElementEnd, NULL);

        // Only update the version number if the StoreData succeeded
        if (err == WEAVE_NO_ERROR)
        {
            // Only update our internal version tracker if this is indeed the last element in the change.
            if (aFlags & kLastElementInChange)
            {
                SetVersion(versionInDE);

                OnEvent(kEventChangeEnd, NULL);
            }
        }
        else
        {
            // We need to clear this since we don't have a good version of data anymore.
            mHasValidVersion = false;
        }
    }
    else
    {
        WeaveLogDetail(DataManagement, "<StoreData> [Trait %08x] version: 0x%" PRIx64 " (no-change)",
                       mSchemaEngine->GetProfileId(), mVersion);
    }

    if (aFlags & kLastElementInChange)
    {
        SetLastNotifyVersion(versionInDE);
    }

exit:
    return err;
}

void TraitDataSink::OnDataSinkEvent(DataSinkEventType aEventType, PropertyPathHandle aHandle)
{
    EventType event;

    switch (aEventType)
    {
    case kDataSinkEvent_DictionaryReplaceBegin:
        event = kEventDictionaryReplaceBegin;
        break;

    case kDataSinkEvent_DictionaryReplaceEnd:
        event = kEventDictionaryReplaceEnd;
        break;

    case kDataSinkEvent_DictionaryItemModifyBegin:
        event = kEventDictionaryItemModifyBegin;
        break;

    case kDataSinkEvent_DictionaryItemModifyEnd:
        event = kEventDictionaryItemModifyEnd;
        break;

    default:
        return;
    };

    OnEvent(event, &aHandle);
}

void TraitDataSink::RejectChange(uint16_t aRejectionStatusCode)
{
    if (sChangeRejectionCb)
    {
        sChangeRejectionCb(aRejectionStatusCode, mVersion, sChangeRejectionContext);
    }
}

WEAVE_ERROR TraitDataSink::SetData(PropertyPathHandle aHandle, TLVReader & aReader, bool aIsNull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // if a trait has no nullable handles, aIsNull will always be false
    // and serves no purpose. this is true for the default implementation.
    IgnoreUnusedVariable(aIsNull);

    if (mSchemaEngine->IsLeaf(aHandle))
    {
        err = SetLeafData(aHandle, aReader);
        if (err != WEAVE_NO_ERROR)
        {
            WeaveLogDetail(DataManagement, "ahandle %u err: %d", aHandle, err);
        }
    }
    return err;
}

TraitDataSource::TraitDataSource(const TraitSchemaEngine * aEngine)
{
    // Set the version to 0, indicating the lack of a valid version.
    SetVersion(0);

    mManagedVersion = true;
    mSetDirtyCalled = false;
    mSchemaEngine   = aEngine;

#if (WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER == IntermediateGraphSolver)
    ClearRootDirty();
#endif
}

uint64_t TraitDataSource::GetVersion(void)
{
    // At the time of version retrieval, check to see if the version is still at the sentinel value of 0 (indicating 'no version') set at construction. If it is,
    // it means that the data source has not over-ridden the version to something other than 0, indicating that it desires to use randomized data versions.
    while (mVersion == 0)
    {
        mVersion = GetRandU64();
    }

    return mVersion;
}

void TraitDataSource::IncrementVersion()
{
    // By invoking GetVersion within here, we get the benefit of checking if the version is currently 0 and if so, randomize it.
    SetVersion(GetVersion() + 1);
}

/**
 *  @brief Handler for custom command
 *
 *  This is a virtual method. If not overridden, the default behavior is to return a status report
 *  with status code Common::kStatus_UnsupportedMessage
 *
 */
void TraitDataSource::OnCustomCommand(Command * aCommand, const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                      nl::Weave::PacketBuffer * aPayload, const uint64_t & aCommandType,
                                      const bool aIsExpiryTimeValid, const int64_t & aExpiryTimeMicroSecond,
                                      const bool aIsMustBeVersionValid, const uint64_t & aMustBeVersion,
                                      nl::Weave::TLV::TLVReader & aArgumentReader)
{
    IgnoreUnusedVariable(aCommandType);
    IgnoreUnusedVariable(aIsExpiryTimeValid);
    IgnoreUnusedVariable(aExpiryTimeMicroSecond);
    IgnoreUnusedVariable(aIsMustBeVersionValid);
    IgnoreUnusedVariable(aMustBeVersion);

    return OnCustomCommand(aCommand, aMsgInfo, aPayload, aArgumentReader);
}

/**
 *  @brief Handler for custom command
 *
 *  This is a virtual method. If not overridden, the default behavior is to return a status report
 *  with status code Common::kStatus_UnsupportedMessage
 *
 *  @note All meta information for the Command are included in the Command
 *  object.
 */
void TraitDataSource::OnCustomCommand(Command * aCommand, const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                      nl::Weave::PacketBuffer * aPayload,
                                      nl::Weave::TLV::TLVReader & aArgumentReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    IgnoreUnusedVariable(aArgumentReader);

    PacketBuffer::Free(aPayload);
    aPayload = NULL;

    err = aCommand->SendError(nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kStatus_UnsupportedMessage,
                              WEAVE_NO_ERROR);
    aCommand = NULL;

    WeaveLogFunctError(err);
}

WEAVE_ERROR TraitDataSource::GetData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                     bool & aIsNull, bool & aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aIsNull    = false;
    aIsPresent = true;
    if (mSchemaEngine->IsLeaf(aHandle))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}

WEAVE_ERROR TraitDataSource::ReadData(PropertyPathHandle aHandle, uint64_t aTagToWrite, TLVWriter & aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Lock();
    err = mSchemaEngine->RetrieveData(aHandle, aTagToWrite, aWriter, this);
    Unlock();

    return err;
}

void TraitDataSource::SetDirty(PropertyPathHandle aPropertyHandle)
{
    if (aPropertyHandle != kNullPropertyPathHandle)
    {
        mSetDirtyCalled = true;
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->SetDirty(this, aPropertyHandle);
    }
}

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
void TraitDataSource::DeleteKey(PropertyPathHandle aPropertyHandle)
{
    // Should only delete the dictionary key, which is a child of the dictionary handle. Only proceed if this is true.
    if (mSchemaEngine->IsDictionary(mSchemaEngine->GetParent(aPropertyHandle)))
    {
        mSetDirtyCalled = true;
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->DeleteKey(this, aPropertyHandle);
    }
}
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

WEAVE_ERROR TraitDataSource::Lock()
{
    mSetDirtyCalled = false;
    VerifyOrDie(SubscriptionEngine::GetInstance());
    return SubscriptionEngine::GetInstance()->Lock();
}

WEAVE_ERROR TraitDataSource::Unlock()
{
    if (mManagedVersion && mSetDirtyCalled)
    {
        IncrementVersion();
    }

    VerifyOrDie(SubscriptionEngine::GetInstance());
    return SubscriptionEngine::GetInstance()->Unlock();
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
TraitUpdatableDataSink::TraitUpdatableDataSink(const TraitSchemaEngine * aEngine) :
    TraitDataSink(aEngine),
    mUpdateRequiredVersion(0),
    mUpdateStartVersion(0),
    mConditionalUpdate(false),
    mPotentialDataLoss(false),
    mpSubClient(NULL),
    mpUpdateEncoder(NULL)
{
};

void TraitUpdatableDataSink::Lock(SubscriptionClient * apSubClient)
{
    VerifyOrDie(apSubClient!=NULL);
    apSubClient->LockUpdateMutex();
}

void TraitUpdatableDataSink::Unlock(SubscriptionClient * apSubClient)
{
    VerifyOrDie(apSubClient!=NULL);
    apSubClient->UnlockUpdateMutex();
}

WEAVE_ERROR TraitUpdatableDataSink::GetData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                            bool & aIsNull, bool & aIsPresent)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    aIsNull    = false;
    aIsPresent = true;
    if (mSchemaEngine->IsLeaf(aHandle))
    {
        err = GetLeafData(aHandle, aTagToWrite, aWriter);
    }

    return err;
}

void TraitUpdatableDataSink::SetUpdateRequiredVersion(const uint64_t &aUpdateRequiredVersion)
{
    if (aUpdateRequiredVersion != mUpdateRequiredVersion)
    {
        WeaveLogDetail(DataManagement, "[Trait %08x] UpdateRequiredVersion: 0x%" PRIx64 " -> 0x%" PRIx64 "",
                mSchemaEngine->GetProfileId(), mUpdateRequiredVersion, aUpdateRequiredVersion);
        mUpdateRequiredVersion = aUpdateRequiredVersion;

        // TODO: Ideally, this fault would be injected when the payload is encoded; but, all DataElements for the
        // same trait instance need to have the same RequiredVersion, and it's easier to achieve that by
        // changing the state here. The loop that does the encoding is being reworked, so this will
        // probably change soon.
        WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_SendUpdateBadVersion, mUpdateRequiredVersion -= 1);
    }

    return;
}

void TraitUpdatableDataSink::SetUpdateStartVersion(void)
{
    if (GetVersion() != mUpdateStartVersion)
    {
        WeaveLogDetail(DataManagement, "[Trait %08x] UpdateStartVersion: 0x%" PRIx64 " -> 0x%" PRIx64 "",
                mSchemaEngine->GetProfileId(), mUpdateStartVersion, GetVersion());
        mUpdateStartVersion = GetVersion();
    }
}

WEAVE_ERROR TraitUpdatableDataSink::ReadData(TraitDataHandle aTraitDataHandle,
                                             PropertyPathHandle aHandle,
                                             uint64_t aTagToWrite,
                                             TLVWriter & aWriter,
                                             PropertyPathHandle & aPropertyPathHandleOfDictItemToStartFrom)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mSchemaEngine->IsDictionary(aHandle))
    {
        WeaveLogDetail(DataManagement, "process dictionary in update");
        err = mSchemaEngine->RetrieveUpdatableDictionaryData(aHandle, aTagToWrite, aWriter,
                                                             static_cast<TraitSchemaEngine::IGetDataDelegate*>(this),
                                                             aPropertyPathHandleOfDictItemToStartFrom);
    }
    else
    {
        UpdateDictionaryDirtyPathCut updateDirtyPathCut(aTraitDataHandle, GetUpdateEncoder());
        err = mSchemaEngine->RetrieveData(aHandle, aTagToWrite, aWriter,
                                          static_cast<TraitSchemaEngine::IGetDataDelegate *>(this),
                                          &updateDirtyPathCut);
    }
    SuccessOrExit(err);

exit:

    return err;
}

/**
 * Declares that the given PropertyPathHandle has local modifications.
 * The NotificationEngine will not override the handle and its descendants
 * until the update request has been processed.
 * The application will receive kEvent_OnUpdateComplete callbacks for this
 * handle or for one of its ancestors with the result of the update operation.
 * The modification can be conditional or not. Conditional data updates will
 * be lost if the local copy of the trait instance is not in sync anymore with
 * publisher's because it was mutated by the publisher itself or by another client.
 * Conditional and unconditional mutations are not supported at the same time in
 * the same trait instance.
 *
 * @param[in]   apSubClient         A pointer to the SubscriptionClient managing this sink.
 * @param[in]   aPropertyHandle     Any valid PropertyPathHandle for this Trait instance.
 * @param[in]   aIsConditional      True for a conditional update; false otherwise.
 *
 * @retval      WEAVE_NO_ERROR if the property handle was successfully added to the set of
 *                  properties to be sent to the owner of the trait.
 * @retval      WEAVE_ERROR_INVALID_ARGUMENT if the handle or the SubscriptionClient
 *                  pointer are invalid.
 * @retval      WEAVE_ERROR_WDM_INCONSISTENT_CONDITIONALITY if the trait instance is already being
 *                  updated with the opposite conditionality.
 * @retval      WEAVE_ERROR_WDM_LOCAL_DATA_INCONSISTENT if aIsConditional is true but the
 *                  trait instance does not have a valid version.
 * @retval      WEAVE_ERROR_WDM_PATH_STORE_FULL if there is no memory to store the path.
 * @retval      Other WEAVE_ERROR codes depending on the failure.
 */
WEAVE_ERROR TraitUpdatableDataSink::SetUpdated(SubscriptionClient * apSubClient, PropertyPathHandle aPropertyHandle, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aPropertyHandle != kNullPropertyPathHandle, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = apSubClient->SetUpdated(this, aPropertyHandle, aIsConditional);
    SuccessOrExit(err);

exit:
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

/*
 *
 *    Copyright (c) 2020 Google LLC.
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

#ifndef GENERIC_TRAIT_CATALOG_IMPL_IPP
#define GENERIC_TRAIT_CATALOG_IMPL_IPP

#include <map>
#include <queue>
#include <limits>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

template <typename T>
GenericTraitCatalogImpl<T>::GenericTraitCatalogImpl(void) : mNodeId(ResourceIdentifier::SELF_NODE_ID)
{
    // Nothing to do.
}

template <typename T>
GenericTraitCatalogImpl<T>::~GenericTraitCatalogImpl(void)
{
    Clear();
}

template <typename T>
void GenericTraitCatalogImpl<T>::SetNodeId(uint64_t aNodeId)
{
    mNodeId = aNodeId;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Add(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId,
                                            PropertyPathHandle basePathHandle, T * traitInstance, TraitDataHandle & aHandle)
{
    WEAVE_ERROR err    = WEAVE_NO_ERROR;
    CatalogItem * item = NULL;
    TraitDataHandle handle;

    // Make sure there is space
    VerifyOrExit(mItemStore.size() < std::numeric_limits<TraitDataHandle>::max(), err = WEAVE_ERROR_NO_MEMORY);

    // Create the CatalogItem
    item = new CatalogItem();
    VerifyOrExit(item != NULL, err = WEAVE_ERROR_NO_MEMORY);

    item->mProfileId      = traitInstance->GetSchemaEngine()->GetProfileId();
    item->mInstanceId     = aInstanceId;
    item->mResourceId     = aResourceId;
    item->mItem           = traitInstance;
    item->mBasePathHandle = basePathHandle;

    // Stop if this path already exists
    err = Locate(item->mProfileId, item->mInstanceId, item->mResourceId, handle);
    VerifyOrExit(err != WEAVE_NO_ERROR, err = WEAVE_ERROR_DUPLICATE_KEY_ID);
    err                 = WEAVE_NO_ERROR;
    // Store the item
    aHandle             = GetNextHandle();
    mItemStore[aHandle] = item;

exit:
    if (err != WEAVE_NO_ERROR && item != NULL)
    {
        delete item;
    }

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Remove(T * traitInstance)
{
    WEAVE_ERROR err;
    TraitDataHandle handle;

    err = Locate(traitInstance, handle);
    SuccessOrExit(err);

    err = Remove(handle);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Remove(TraitDataHandle aHandle)
{
    WEAVE_ERROR err    = WEAVE_NO_ERROR;
    CatalogItem * item = NULL;

    // Make sure the handle exists
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Remove the item and delete it
    item = itemIterator->second;
    mItemStore.erase(itemIterator);
    delete item;
    mRecycledHandles.push(aHandle);
exit:
    return err;
}

template <typename T>
TraitDataHandle GenericTraitCatalogImpl<T>::GetNextHandle(void)
{
    TraitDataHandle rv;
    if (mRecycledHandles.empty())
    {
        rv = static_cast<TraitDataHandle>(mItemStore.size());
    }
    else
    {
        rv = mRecycledHandles.front();
        mRecycledHandles.pop();
    }
    // assert correctness: returned handle must not be an existing key in the map
    VerifyOrDie(mItemStore.find(rv) == mItemStore.end());

    return rv;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Clear(void)
{
    WEAVE_ERROR err    = WEAVE_NO_ERROR;
    CatalogItem * item = NULL;
    std::queue<TraitDataHandle> empty;

    // Loop through the items and remove them all
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        item = itemIterator->second;
        delete item;
    }
    mItemStore.clear();

    std::swap(mRecycledHandles, empty);

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                                        SchemaVersionRange & aSchemaVersionRange) const
{
    WEAVE_ERROR err     = WEAVE_NO_ERROR;
    uint32_t profileId  = 0;
    uint64_t instanceId = 0;
    ResourceIdentifier resourceId(mNodeId);
    Path::Parser path;
    TLV::TLVReader reader;

    err = path.Init(aReader);
    SuccessOrExit(err);

    err = path.GetProfileID(&profileId, &aSchemaVersionRange);
    SuccessOrExit(err);

    err = path.GetInstanceID(&instanceId);
    if ((WEAVE_NO_ERROR != err) && (WEAVE_END_OF_TLV != err))
    {
        ExitNow();
    }

    err = path.GetResourceID(&reader);
    if (WEAVE_NO_ERROR == err)
    {
        err = resourceId.FromTLV(reader);
        SuccessOrExit(err);
    }
    else if (WEAVE_END_OF_TLV == err)
    {
        // no-op -- resource already initialized to mNodeId in the constructor
    }
    else if (WEAVE_NO_ERROR != err)
    {
        ExitNow();
    }

    path.GetTags(&aReader);

    VerifyOrExit(profileId != 0, err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);

    err = Locate(profileId, instanceId, resourceId, aHandle);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter,
                                                        SchemaVersionRange & aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLV::TLVType type;
    CatalogItem * item = NULL;
    // Make sure the handle exists
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(aSchemaVersionRange.IsValid(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    item = itemIterator->second;

    err = aWriter.StartContainer(TLV::ContextTag(Path::kCsTag_InstanceLocator), TLV::kTLVType_Structure, type);
    SuccessOrExit(err);

    if (aSchemaVersionRange.mMinVersion != 1 || aSchemaVersionRange.mMaxVersion != 1)
    {
        TLV::TLVType type2;

        err = aWriter.StartContainer(TLV::ContextTag(Path::kCsTag_TraitProfileID), TLV::kTLVType_Array, type2);
        SuccessOrExit(err);

        err = aWriter.Put(TLV::AnonymousTag, item->mItem->GetSchemaEngine()->GetProfileId());
        SuccessOrExit(err);

        // Only encode the max version if it isn't 1.
        if (aSchemaVersionRange.mMaxVersion != 1)
        {
            err = aWriter.Put(TLV::AnonymousTag, aSchemaVersionRange.mMaxVersion);
            SuccessOrExit(err);
        }

        // Only encode the min version if it isn't 1.
        if (aSchemaVersionRange.mMinVersion != 1)
        {
            err = aWriter.Put(TLV::AnonymousTag, aSchemaVersionRange.mMinVersion);
            SuccessOrExit(err);
        }

        err = aWriter.EndContainer(type2);
        SuccessOrExit(err);
    }
    else
    {
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_TraitProfileID), item->mItem->GetSchemaEngine()->GetProfileId());
        SuccessOrExit(err);
    }

    if (item->mInstanceId)
    {
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_TraitInstanceID), item->mInstanceId);
        SuccessOrExit(err);
    }

    err = item->mResourceId.ToTLV(aWriter);
    SuccessOrExit(err);

    err = aWriter.EndContainer(type);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Locate(TraitDataHandle aHandle, T ** aTraitInstance) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // Make sure the handle exists
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait instance
    *aTraitInstance = itemIterator->second->mItem;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Locate(T * aTraitInstance, TraitDataHandle & aHandle) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_ARGUMENT;
    // Iterate and find this trait instance
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        CatalogItem * item = itemIterator->second;
        if (aTraitInstance == item->mItem)
        {
            aHandle = itemIterator->first;
            err     = WEAVE_NO_ERROR;
            break;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Locate(uint32_t aProfileId, uint64_t aInstanceId, ResourceIdentifier aResourceId,
                                               TraitDataHandle & aHandle) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_PROFILE_ID;
    // Loop through the items and check if the path matches
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        CatalogItem * item = itemIterator->second;
        if (item->mProfileId == aProfileId && item->mResourceId == aResourceId && item->mInstanceId == aInstanceId)
        {
            aHandle = itemIterator->first;
            err     = WEAVE_NO_ERROR;
            break;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Locate(uint32_t aProfileId, uint64_t aInstanceId, ResourceIdentifier aResourceId,
                                               T ** aTraitInstance) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_PROFILE_ID;
    // Loop through the items and check if the path matches
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        CatalogItem * item = itemIterator->second;
        if (item->mProfileId == aProfileId && item->mResourceId == aResourceId && item->mInstanceId == aInstanceId)
        {
            *aTraitInstance = itemIterator->second->mItem;
            err             = WEAVE_NO_ERROR;
            break;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::DispatchEvent(uint16_t aEvent, void * aContext) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // Send the event to all the items
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        CatalogItem * item = itemIterator->second;
        item->mItem->OnEvent(aEvent, aContext);
    }

    return err;
}

template <typename T>
void GenericTraitCatalogImpl<T>::Iterate(IteratorCallback aCallback, void * aContext)
{
    // Send the event to all the items
    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        aCallback(itemIterator->second->mItem, itemIterator->first, aContext);
    }
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::GetInstanceId(TraitDataHandle aHandle, uint64_t & aInstanceId) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // Make sure the handle exists
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait mInstanceId
    aInstanceId = itemIterator->second->mInstanceId;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::GetResourceId(TraitDataHandle aHandle, ResourceIdentifier & aResourceId) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // Make sure the handle exists
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait mResourceId
    aResourceId = itemIterator->second->mResourceId;

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

template <typename T>
uint32_t GenericTraitCatalogImpl<T>::Size(void) const
{
    return mItemStore.size();
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::PrepareSubscriptionSpecificPathList(TraitPath * pathList, uint16_t pathListSize,
                                                                            TraitDataHandle aHandle)
{
    WEAVE_ERROR err   = WEAVE_NO_ERROR;
    auto itemIterator = mItemStore.find(aHandle);
    VerifyOrExit(itemIterator != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(pathListSize == 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    *pathList = TraitPath(aHandle, itemIterator->second->mBasePathHandle);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize,
                                                                    uint16_t & pathListLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    pathListLen     = 0;

    VerifyOrExit(mItemStore.size() <= pathListSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    for (auto itemIterator = mItemStore.begin(); itemIterator != mItemStore.end(); itemIterator++)
    {
        CatalogItem * item = itemIterator->second;
        *pathList++        = TraitPath(itemIterator->first, item->mBasePathHandle);
        pathListLen++;
    }

exit:
    return err;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // GENERIC_TRAIT_CATALOG_IMPL_IPP

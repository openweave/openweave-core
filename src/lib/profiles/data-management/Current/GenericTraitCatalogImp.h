/*
 *
 *    Copyright (c) 2019 Google, LLC.
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
 *    Defines generic trait catalogs that are used to house trait data sources/sinks and can be used with the
 *    various WDM engines to correctly map to/from resources specified in a WDM path to actual trait
 *    data instances.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMP_H
#define _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMP_H

#include <map>
#include <limits>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
/*
 *  @class GenericTraitCatalogImpl
 *
 *  @brief A Weave provided implementation of the TraitCatalogBase interface for a collection of trait data instances
 *         that all refer to the same resource. It provides a map-backed storage for these instances.
 */
template <typename T>
class GenericTraitCatalogImpl : public TraitCatalogBase<T>
{
public:
    GenericTraitCatalogImpl(void);
    virtual ~GenericTraitCatalogImpl(void);

    WEAVE_ERROR Add(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId, PropertyPathHandle basePathHandle, T * traitInstance, TraitDataHandle & aHandle);
    WEAVE_ERROR Remove(T * traitInstance);
    WEAVE_ERROR Remove(TraitDataHandle aHandle);
    WEAVE_ERROR PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize, uint16_t & pathListLen, TraitDataHandle aHandle, bool aNeedSubscribeAll);
    WEAVE_ERROR Clear(void);

    virtual WEAVE_ERROR AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR Locate(TraitDataHandle aHandle, T ** aTraitInstance) const;
    virtual WEAVE_ERROR Locate(T * aTraitInstance, TraitDataHandle & aHandle) const;

    WEAVE_ERROR Locate (uint32_t aProfileId,
                        uint64_t aInstanceId,
                        ResourceIdentifier aResourceId,
                        TraitDataHandle &aHandle) const;

    WEAVE_ERROR Locate (uint32_t aProfileId,
                        uint64_t aInstanceId,
                        ResourceIdentifier aResourceId,
                        T ** aTraitInstance) const;

    virtual WEAVE_ERROR DispatchEvent(uint16_t aEvent, void * aContext) const;
    virtual void Iterate(IteratorCallback aCallback, void * aContext);
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    virtual WEAVE_ERROR GetInstanceId(TraitDataHandle aHandle, uint64_t &aInstanceId) const;
    virtual WEAVE_ERROR GetResourceId(TraitDataHandle aHandle, ResourceIdentifier &aResourceId) const;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    /**
     * Return the number of trait instances in the catalog.
     */
    uint32_t Count(void) const;

private:
    struct CatalogItem
    {
        uint32_t mProfileId;
        uint64_t mInstanceId;
        ResourceIdentifier mResourceId;
        T *mItem;
        PropertyPathHandle mBasePathHandle;
    };
    TraitDataHandle mNextHandle;

    typedef std::map<TraitDataHandle, CatalogItem*> Handle2ItemMap_t;
    Handle2ItemMap_t mItemStore;
};

typedef GenericTraitCatalogImpl<TraitDataSink> GenericTraitSinkCatalog;
typedef GenericTraitCatalogImpl<TraitDataSource> GenericTraitSourceCatalog;

template <typename T>
GenericTraitCatalogImpl<T>::GenericTraitCatalogImpl(void)
        : mNextHandle(0)
{
    // Nothing to do.
}

template <typename T>
GenericTraitCatalogImpl<T>::~GenericTraitCatalogImpl(void)
{
    Clear();
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Add(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId,
                                     PropertyPathHandle basePathHandle, T * traitInstance, TraitDataHandle & aHandle)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CatalogItem *item = NULL;
    TraitDataHandle handle;
    typename Handle2ItemMap_t::iterator it;

    // Make sure there is space
    VerifyOrExit(mItemStore.size() < std::numeric_limits<TraitDataHandle>::max(),
                 err = WEAVE_ERROR_NO_MEMORY);

    // Create the CatalogItem
    item = new CatalogItem();
    VerifyOrExit(item != NULL, err = WEAVE_ERROR_NO_MEMORY);

    item->mProfileId = traitInstance->GetSchemaEngine()->GetProfileId();
    item->mInstanceId = aInstanceId;
    item->mResourceId = aResourceId;
    item->mItem = traitInstance;
    item->mBasePathHandle = basePathHandle;

    // Stop if this path already exists
    err = Locate(item->mProfileId, item->mInstanceId, item->mResourceId, handle);
    VerifyOrExit(err != WEAVE_NO_ERROR, err = WEAVE_ERROR_DUPLICATE_KEY_ID);

    do {
        // Assign a new handle
        aHandle = mNextHandle++;

        // Make sure it's available as mNextHandle may have wrapped
        it = mItemStore.find(aHandle);
    } while (it != mItemStore.end());

    // Store the item
    mItemStore[aHandle] = item;
    err = WEAVE_NO_ERROR;

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
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_ARGUMENT;
    typename Handle2ItemMap_t::const_iterator it;
    // Loop through the items and delete the items
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        if (item && item->mItem == traitInstance)
        {
            delete item;
            err = WEAVE_NO_ERROR;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::Remove(TraitDataHandle aHandle)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::iterator it;
    CatalogItem *item = NULL;

    // Make sure the handle exists
    it = mItemStore.find(aHandle);
    VerifyOrExit(it != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Remove the item and delete it
    item = it->second;
    mItemStore.erase(it);
    delete item;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::Clear(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::iterator it;
    CatalogItem *item = NULL;
    // Loop through the items and remove them all
    it = mItemStore.begin();
    while (it != mItemStore.end())
    {
        item = it->second;
        delete item;
        it ++;
    }
    mItemStore.clear();

    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::AddressToHandle (TLV::TLVReader &aReader,
                                         TraitDataHandle &aHandle,
                                         SchemaVersionRange &aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t profileId = 0;
    uint64_t instanceId = 0;
    ResourceIdentifier resourceId;
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
    if (err == WEAVE_NO_ERROR)
    {
        err = resourceId.FromTLV(reader);
        SuccessOrExit(err);
    }
    else if (err == WEAVE_END_OF_TLV)
    {
        resourceId = ResourceIdentifier(ResourceIdentifier::RESOURCE_TYPE_RESERVED,
                                        ResourceIdentifier::SELF_NODE_ID);
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
WEAVE_ERROR
GenericTraitCatalogImpl<T>::HandleToAddress (TraitDataHandle aHandle,
                                             TLV::TLVWriter &aWriter,
                                         SchemaVersionRange &aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLV::TLVType type;
    typename Handle2ItemMap_t::const_iterator it;
    CatalogItem *item = NULL;

    // Make sure the handle exists
    it = mItemStore.find(aHandle);
    VerifyOrExit(it != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(aSchemaVersionRange.IsValid(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    item = it->second;

    err = aWriter.StartContainer(TLV::ContextTag(Path::kCsTag_InstanceLocator),
                                 TLV::kTLVType_Structure, type);
    SuccessOrExit(err);

    if (aSchemaVersionRange.mMinVersion != 1 || aSchemaVersionRange.mMaxVersion != 1) {
        TLV::TLVType type2;

        err = aWriter.StartContainer(TLV::ContextTag(Path::kCsTag_TraitProfileID), TLV::kTLVType_Array, type2);
        SuccessOrExit(err);

        err = aWriter.Put(TLV::AnonymousTag, item->mItem->GetSchemaEngine()->GetProfileId());
        SuccessOrExit(err);

        // Only encode the max version if it isn't 1.
        if (aSchemaVersionRange.mMaxVersion != 1) {
            err = aWriter.Put(TLV::AnonymousTag, aSchemaVersionRange.mMaxVersion);
            SuccessOrExit(err);
        }

        // Only encode the min version if it isn't 1.
        if (aSchemaVersionRange.mMinVersion != 1) {
            err = aWriter.Put(TLV::AnonymousTag, aSchemaVersionRange.mMinVersion);
            SuccessOrExit(err);
        }

        err = aWriter.EndContainer(type2);
        SuccessOrExit(err);
    }
    else {
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_TraitProfileID), item->mItem->GetSchemaEngine()->GetProfileId());
        SuccessOrExit(err);
    }

    if (item->mInstanceId) {
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_TraitInstanceID),
                          item->mInstanceId);
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
WEAVE_ERROR
GenericTraitCatalogImpl<T>::Locate (TraitDataHandle aHandle, T **aTraitInstance) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::const_iterator it;

    // Make sure the handle exists
    it = mItemStore.find(aHandle);
    VerifyOrExit(it != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait instance
    *aTraitInstance = it->second->mItem;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::Locate (T *aTraitInstance,
                                TraitDataHandle &aHandle) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_ARGUMENT;
    typename Handle2ItemMap_t::const_iterator it;

    // Iterate and find this trait instance
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        if (aTraitInstance == it->second->mItem)
        {
            aHandle = it->first;
            err = WEAVE_NO_ERROR;
            break;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::Locate (uint32_t aProfileId,
                                uint64_t aInstanceId,
                                ResourceIdentifier aResourceId,
                                TraitDataHandle &aHandle) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_PROFILE_ID;
    typename Handle2ItemMap_t::const_iterator it;

    // Loop through the items and check if the path matches
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        if (item->mProfileId == aProfileId &&
            item->mResourceId == aResourceId &&
            item->mInstanceId == aInstanceId)
        {
            err = WEAVE_NO_ERROR;
            aHandle = it->first;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::Locate (uint32_t aProfileId,
                                    uint64_t aInstanceId,
                                    ResourceIdentifier aResourceId,
                                    T **aTraitInstance) const
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_PROFILE_ID;
    typename Handle2ItemMap_t::const_iterator it;

    // Loop through the items and check if the path matches
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        if (item->mProfileId == aProfileId &&
            item->mResourceId == aResourceId &&
            item->mInstanceId == aInstanceId)
        {
            err = WEAVE_NO_ERROR;
            *aTraitInstance = it->second->mItem;
        }
    }

    return err;
}

template <typename T>
WEAVE_ERROR
GenericTraitCatalogImpl<T>::DispatchEvent (uint16_t aEvent, void *aContext) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::const_iterator it;

    // Send the event to all the items
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        item->mItem->OnEvent(aEvent, aContext);
    }

    return err;
}

template <typename T>
void
GenericTraitCatalogImpl<T>::Iterate (IteratorCallback aCallback, void *aContext)
{
    typename Handle2ItemMap_t::const_iterator it;

    // Send the event to all the items
    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        aCallback(it->second->mItem, it->first, aContext);
    }
}

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::GetInstanceId (TraitDataHandle aHandle, uint64_t &aInstanceId) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::const_iterator it;

    // Make sure the handle exists
    it = mItemStore.find(aHandle);
    VerifyOrExit(it != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait mInstanceId
    aInstanceId = it->second->mInstanceId;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::GetResourceId (TraitDataHandle aHandle, ResourceIdentifier &aResourceId) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::const_iterator it;

    // Make sure the handle exists
    it = mItemStore.find(aHandle);
    VerifyOrExit(it != mItemStore.end(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the trait mResourceId
    aResourceId = it->second->mResourceId;

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

template <typename T>
uint32_t GenericTraitCatalogImpl<T>::Count(void) const
{
    uint32_t count = 0;
    typename Handle2ItemMap_t::const_iterator it;

    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        if (item->mItem != NULL)
        {
            count++;
        }
    }

    return count;
}

template <typename T>
WEAVE_ERROR GenericTraitCatalogImpl<T>::PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize, uint16_t & pathListLen, TraitDataHandle aHandle, bool aNeedSubscribeAll)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    typename Handle2ItemMap_t::const_iterator it;
    pathListLen = 0;

    for (it = mItemStore.begin(); it != mItemStore.end(); it++)
    {
        CatalogItem *item = it->second;
        if (item->mItem != NULL)
        {
            VerifyOrExit(pathListLen < pathListSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            if (aNeedSubscribeAll)
            {
                *pathList++ = TraitPath(it->first, item->mBasePathHandle);
                pathListLen++;
            }
            else
            {
                if (it->first == aHandle)
                {
                    pathList[0] = TraitPath(it->first, item->mBasePathHandle);
                    pathListLen = 1;
                    err = WEAVE_NO_ERROR;
                    break;
                }
                else
                {
                    err = WEAVE_ERROR_INVALID_ARGUMENT;
                }
            }
        }
    }


exit:
    return err;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMP_H

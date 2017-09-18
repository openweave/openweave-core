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
 *    Defines catalogs that are used to house trait data sources/sinks and can be used with the
 *    various WDM engines to correctly map to/from resources specified in a WDM path to actual trait
 *    data instances.
 *
 */


#ifndef _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CATALOG_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CATALOG_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/data-management/TraitData.h>
#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/* Unique handle to a particular trait data instance within a catalog */
typedef uint16_t TraitDataHandle;

/* A structure representing a path to a property or set of properties within a trait instance belonging to a particular resource */
struct TraitPath
{
    TraitPath() { }
    TraitPath(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyPathHandle) : mTraitDataHandle(aDataHandle), mPropertyPathHandle(aPropertyPathHandle) { }
    bool operator == (const TraitPath &rhs) const { return ((rhs.mTraitDataHandle == mTraitDataHandle) && (rhs.mPropertyPathHandle == mPropertyPathHandle)); }
    bool IsValid() { return !(mPropertyPathHandle == kNullPropertyPathHandle); }
    TraitDataHandle mTraitDataHandle;
    PropertyPathHandle mPropertyPathHandle;
};

struct VersionedTraitPath : public TraitPath
{
    VersionedTraitPath() { }
    VersionedTraitPath(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyPathHandle, SchemaVersionRange aRequestedVersionRange)
        : TraitPath(aDataHandle, aPropertyPathHandle) { mRequestedVersionRange = aRequestedVersionRange; }

    SchemaVersionRange mRequestedVersionRange;
};

/**
 * Trait handle iterator
 */
typedef void (*IteratorCallback)(void *aTraitInstance, TraitDataHandle aHandle, void *aContext);

/*
 *  @class TraitCatalogBase
 *
 *  @brief A catalog interface that all concrete catalogs need to adhere to.
 *
 */
template <typename T>
class TraitCatalogBase
{
public:
    /**
     * Given a reader positioned at the Path::kCsTag_RootSection structure on a WDM path, parse that structure
     * and return the matching handle to the trait.
     */
    virtual WEAVE_ERROR AddressToHandle(TLV::TLVReader &aReader, TraitDataHandle &aHandle, SchemaVersionRange &aSchemaVersionRange) const = 0;

    /**
     * Given a trait handle, write out the TLV for the Path::kCsTag_RootSection structure.
     */
    virtual WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter &aWriter, SchemaVersionRange &aSchemaVersionRange) const = 0;

    /**
     * Given a handle, return a reference to the matching trait data instance.
     */
    virtual WEAVE_ERROR Locate(TraitDataHandle aHandle, T **aTraitInstance) const = 0;

    /**
     * Reverse
     */
    virtual WEAVE_ERROR Locate(T *aTraitInstance, TraitDataHandle &aHandle) const = 0;

    /**
     * Dispatch an event to all trait data instance housed in this catalog.
     */
    virtual WEAVE_ERROR DispatchEvent(uint16_t aEvent, void *aContext) const = 0;

    virtual void Iterate(IteratorCallback aCallback, void *aContext) = 0;
};

/**
 * This structure provides a way to represent the identity of a resource
 * either by way of the NodeId of the device, or a UUID.
 */
struct ResourceIdentifier {
    ResourceIdentifier(uint64_t aNodeId) {
        mSelector = kNodeId;
        mNodeId = aNodeId;
    }

    ResourceIdentifier(uint8_t *aUUID) {
        mSelector = kUUID;
        memcpy(mUUID, aUUID, 6);
    }

    /* Defines a special value for NodeId that refers to 'self'. In certain WDM interactions, having a value of
     * self for resource allows for compressing out that information as it is redundant to the source node id of
     * the device expressed in the Weave message itself
     */
    static const uint64_t SELF_NODE_ID = 0;

    enum { kNodeId, kUUID } mSelector;
    union {
        uint64_t mNodeId;
        uint8_t mUUID[16];
    };
};

/*
 *  @class SingleResourceTraitCatalog
 *
 *  @brief A Weave provided implementation of the TraitCatalogBase interface for a collection of trait data instances
 *         that all refer to the same resource. It provides an array-backed, bounded storage for these instances.
 */
template <typename T>
class SingleResourceTraitCatalog : public TraitCatalogBase<T>
{
public:
    struct CatalogItem {
        uint64_t mInstanceId;
        T *mItem;
    };

    /*
     * Instances a trait catalog given a pointer to the underlying array store.
     */
    SingleResourceTraitCatalog(ResourceIdentifier aResourceIdentifier, CatalogItem *aCatalogStore, uint32_t aNumMaxCatalogItems);

    /*
     * Add a new trait data instance into the catalog and return a handle to it.
     */
    WEAVE_ERROR Add(uint64_t aInstanceId, T *aItem, TraitDataHandle &aHandle);

    /*
     * Add a new trait data instance bound to a user-selected trait handle (which in this particular implementation, denotes the offset in the array).
     * The handle is to be between 0 and the size of the array. Also, the caller should ensure no gaps form after every call made to this method.
     */
    WEAVE_ERROR AddAt(uint64_t aInstanceId, T *aItem, TraitDataHandle aHandle);

    /**
     * Removes a trait instance from the catalog.
     */
    WEAVE_ERROR Remove(TraitDataHandle aHandle);

    WEAVE_ERROR Locate(uint64_t aProfileId, uint64_t aInstanceId, TraitDataHandle &aHandle) const;

public: // TraitCatalogBase
    WEAVE_ERROR AddressToHandle(TLV::TLVReader &aReader, TraitDataHandle &aHandle, SchemaVersionRange &aSchemaVersionRange) const;
    WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter &aWriter, SchemaVersionRange &aSchemaVersionRange) const;
    WEAVE_ERROR Locate(TraitDataHandle aHandle, T **aTraitInstance) const;
    WEAVE_ERROR Locate(T *aTraitInstance, TraitDataHandle &aHandle) const;
    WEAVE_ERROR DispatchEvent(uint16_t aEvent, void *aContext) const;
    void Iterate(IteratorCallback aCallback, void *aContext);

private:
    CatalogItem *mCatalogStore;
    ResourceIdentifier mResourceId;
    uint32_t mNumMaxCatalogItems;
    uint32_t mNumCurCatalogItems;
};

typedef SingleResourceTraitCatalog<TraitDataSink> SingleResourceSinkTraitCatalog;
typedef SingleResourceTraitCatalog<TraitDataSource> SingleResourceSourceTraitCatalog;

template <typename T>
SingleResourceTraitCatalog<T>::SingleResourceTraitCatalog(ResourceIdentifier aResourceIdentifier, CatalogItem *aCatalogStore, uint32_t aNumMaxCatalogItems)
    : mResourceId(aResourceIdentifier)
{
    mCatalogStore = aCatalogStore;
    mNumMaxCatalogItems = aNumMaxCatalogItems;
    mNumCurCatalogItems = 0;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::Add(uint64_t aInstanceId, T *aItem, TraitDataHandle &aHandle)
{
    if (mNumCurCatalogItems >= mNumMaxCatalogItems) {
        return WEAVE_ERROR_NO_MEMORY;
    }

    mCatalogStore[mNumCurCatalogItems].mInstanceId = aInstanceId;
    mCatalogStore[mNumCurCatalogItems].mItem = aItem;
    aHandle = mNumCurCatalogItems++;

    return WEAVE_NO_ERROR;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::AddAt(uint64_t aInstanceId, T *aItem, TraitDataHandle aHandle)
{
    if (aHandle >= mNumMaxCatalogItems) {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }

    mCatalogStore[aHandle].mInstanceId = aInstanceId;
    mCatalogStore[aHandle].mItem = aItem;
    mNumCurCatalogItems++;

    return WEAVE_NO_ERROR;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::Remove(TraitDataHandle aHandle)
{
    if (aHandle >= mNumCurCatalogItems) {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }

    memcpy(mCatalogStore + aHandle, mCatalogStore + aHandle + 1, (mNumCurCatalogItems - 1 - aHandle) * sizeof(CatalogItem));
    mNumCurCatalogItems--;

    return WEAVE_NO_ERROR;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::AddressToHandle(TLV::TLVReader &aReader, TraitDataHandle &aHandle, SchemaVersionRange &aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t profileId = 0;
    uint64_t instanceId = 0;
    Path::Parser path;

    err = path.Init(aReader);
    SuccessOrExit(err);

    err = path.GetProfileID(&profileId, &aSchemaVersionRange);
    SuccessOrExit(err);

    err = path.GetInstanceID(&instanceId);
    if ((WEAVE_NO_ERROR != err) && (WEAVE_END_OF_TLV != err))
    {
        ExitNow();
    }

    if (ResourceIdentifier::kNodeId == mResourceId.mSelector)
    {
        uint64_t resourceId;
        err = path.GetResourceID(&resourceId);
        if ((WEAVE_NO_ERROR != err) && (WEAVE_END_OF_TLV != err))
        {
            ExitNow();
        }
    }

    path.GetTags(&aReader);

    VerifyOrExit(profileId != 0, err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);

    err = Locate(profileId, instanceId, aHandle);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::Locate(uint64_t aProfileId, uint64_t aInstanceId, TraitDataHandle &aHandle) const
{
    for (uint32_t i = 0; i < mNumCurCatalogItems; i++) {
        if ((mCatalogStore[i].mItem->GetSchemaEngine()->GetProfileId() == aProfileId) && (mCatalogStore[i].mInstanceId == aInstanceId)) {
            aHandle = i;
            return WEAVE_NO_ERROR;
        }
    }

    return WEAVE_ERROR_INVALID_PROFILE_ID;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter &aWriter, SchemaVersionRange &aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CatalogItem *item;
    TLV::TLVType type;

    VerifyOrExit(aHandle < mNumCurCatalogItems, err = WEAVE_ERROR_INVALID_ARGUMENT);
    item = &mCatalogStore[aHandle];

    VerifyOrExit(aSchemaVersionRange.IsValid(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = aWriter.StartContainer(TLV::ContextTag(Path::kCsTag_InstanceLocator), TLV::kTLVType_Structure, type);
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
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_TraitInstanceID), item->mInstanceId);
        SuccessOrExit(err);
    }

    if (mResourceId.mSelector == ResourceIdentifier::kNodeId && mResourceId.mNodeId != ResourceIdentifier::SELF_NODE_ID) {
        err = aWriter.Put(TLV::ContextTag(Path::kCsTag_ResourceID), mResourceId.mNodeId);
        SuccessOrExit(err);
    }
    else if (mResourceId.mSelector == ResourceIdentifier::kUUID) {
        err = aWriter.PutBytes(TLV::ContextTag(Path::kCsTag_ResourceID), mResourceId.mUUID, sizeof(mResourceId.mUUID));
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(type);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::Locate(TraitDataHandle aHandle, T **aTraitInstance) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aHandle < mNumCurCatalogItems, err = WEAVE_ERROR_INVALID_ARGUMENT);
    *aTraitInstance = mCatalogStore[aHandle].mItem;

exit:
    return err;
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::Locate(T *aTraitInstance, TraitDataHandle &aHandle) const
{
    for (uint32_t i = 0; i < mNumCurCatalogItems; i++) {
        if (mCatalogStore[i].mItem == aTraitInstance) {
            aHandle = i;
            return WEAVE_NO_ERROR;
        }
    }

    return WEAVE_ERROR_KEY_NOT_FOUND;
}

template <typename T>
void
SingleResourceTraitCatalog<T>::Iterate(IteratorCallback aCallback, void *aContext)
{
    for (uint32_t i = 0; i < mNumCurCatalogItems; i++) {
        aCallback(mCatalogStore[i].mItem, i, aContext);
    }
}

template <typename T>
WEAVE_ERROR
SingleResourceTraitCatalog<T>::DispatchEvent(uint16_t aEvent, void *aContext) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (uint32_t i = 0; i < mNumCurCatalogItems; i++) {
        mCatalogStore[i].mItem->OnEvent(aEvent, aContext);
    }

    return err;
}

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CATALOG_CURRENT_H

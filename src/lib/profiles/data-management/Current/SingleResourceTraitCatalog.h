/*
 *
 *    Copyright (c) 2020 Google, LLC.
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

#ifndef _WEAVE_DATA_MANAGEMENT_SINGLE_RESOURCE_TRAIT_CATALOG_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_SINGLE_RESOURCE_TRAIT_CATALOG_CURRENT_H

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
    struct CatalogItem
    {
        uint64_t mInstanceId;
        T * mItem;
    };

    /*
     * Instances a trait catalog given a pointer to the underlying array store.
     */
    SingleResourceTraitCatalog(ResourceIdentifier aResourceIdentifier, CatalogItem * aCatalogStore, uint32_t aNumMaxCatalogItems);

    /*
     * Add a new trait data instance into the catalog and return a handle to it.
     */
    WEAVE_ERROR Add(uint64_t aInstanceId, T * aItem, TraitDataHandle & aHandle);

    /*
     * Add a new trait data instance bound to a user-selected trait handle (which in this particular implementation, denotes the
     * offset in the array). The handle is to be between 0 and the size of the array. Also, the caller should ensure no gaps form
     * after every call made to this method.
     */
    WEAVE_ERROR AddAt(uint64_t aInstanceId, T * aItem, TraitDataHandle aHandle);

    /**
     * Removes a trait instance from the catalog.
     */
    WEAVE_ERROR Remove(TraitDataHandle aHandle);

    WEAVE_ERROR Locate(uint64_t aProfileId, uint64_t aInstanceId, TraitDataHandle & aHandle) const;

    /**
     * Return the number of trait instances in the catalog.
     */
    uint32_t Count() const;

public: // TraitCatalogBase
    WEAVE_ERROR AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                SchemaVersionRange & aSchemaVersionRange) const;
    WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter, SchemaVersionRange & aSchemaVersionRange) const;
    WEAVE_ERROR Locate(TraitDataHandle aHandle, T ** aTraitInstance) const;
    WEAVE_ERROR Locate(T * aTraitInstance, TraitDataHandle & aHandle) const;
    WEAVE_ERROR DispatchEvent(uint16_t aEvent, void * aContext) const;
    void Iterate(IteratorCallback aCallback, void * aContext);

#if    WEAVE_CONFIG_ENABLE_WDM_UPDATE
    WEAVE_ERROR GetInstanceId(TraitDataHandle aHandle, uint64_t &aInstanceId) const;
    WEAVE_ERROR GetResourceId(TraitDataHandle aHandle, ResourceIdentifier &aResourceId) const;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

private:
    CatalogItem * mCatalogStore;
    ResourceIdentifier mResourceId;
    uint32_t mNumMaxCatalogItems;
    uint32_t mNumOfUsedCatalogItems;
};

typedef SingleResourceTraitCatalog<TraitDataSink> SingleResourceSinkTraitCatalog;
typedef SingleResourceTraitCatalog<TraitDataSource> SingleResourceSourceTraitCatalog;

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl
#endif // _WEAVE_DATA_MANAGEMENT_SINGLE_RESOURCE_TRAIT_CATALOG_CURRENT_H

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

/**
 *    @file
 *    Defines generic trait catalogs that are used to house trait data sources/sinks and can be used with the
 *    various WDM engines to correctly map to/from resources specified in a WDM path to actual trait
 *    data instances.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMPL_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMPL_CURRENT_H

#include <map>
#include <queue>
#include <limits>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
/**
 *  @class GenericTraitCatalogImpl
 *
 *  @brief A Weave provided implementation of the TraitCatalogBase interface for a collection of trait data instances
 *         that all refer to the same resource. It provides a c++ map-backed storage for these instances.
 */
template <typename T>
class GenericTraitCatalogImpl : public TraitCatalogBase<T>
{
public:
    GenericTraitCatalogImpl(void);
    virtual ~GenericTraitCatalogImpl(void);

    void SetNodeId(uint64_t aNodeId);

    WEAVE_ERROR Add(const ResourceIdentifier & aResourceId, const uint64_t & aInstanceId, PropertyPathHandle basePathHandle,
                    T * traitInstance, TraitDataHandle & aHandle);
    WEAVE_ERROR Remove(T * traitInstance);
    WEAVE_ERROR Remove(TraitDataHandle aHandle);
    WEAVE_ERROR PrepareSubscriptionSpecificPathList(TraitPath * pathList, uint16_t pathListSize, TraitDataHandle aHandle);
    WEAVE_ERROR PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize, uint16_t & pathListLen);
    WEAVE_ERROR Clear(void);

    virtual WEAVE_ERROR AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR Locate(TraitDataHandle aHandle, T ** aTraitInstance) const;
    virtual WEAVE_ERROR Locate(T * aTraitInstance, TraitDataHandle & aHandle) const;

    WEAVE_ERROR Locate(uint32_t aProfileId, uint64_t aInstanceId, ResourceIdentifier aResourceId, TraitDataHandle & aHandle) const;

    WEAVE_ERROR Locate(uint32_t aProfileId, uint64_t aInstanceId, ResourceIdentifier aResourceId, T ** aTraitInstance) const;

    virtual WEAVE_ERROR DispatchEvent(uint16_t aEvent, void * aContext) const;
    virtual void Iterate(IteratorCallback aCallback, void * aContext);
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    virtual WEAVE_ERROR GetInstanceId(TraitDataHandle aHandle, uint64_t & aInstanceId) const;
    virtual WEAVE_ERROR GetResourceId(TraitDataHandle aHandle, ResourceIdentifier & aResourceId) const;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    /**
     * Return the number of trait instances in the catalog.
     */
    uint32_t Size(void) const;

private:
    struct CatalogItem
    {
        uint32_t mProfileId;
        uint64_t mInstanceId;
        ResourceIdentifier mResourceId;
        T * mItem;
        PropertyPathHandle mBasePathHandle;
    };

    TraitDataHandle GetNextHandle();

    uint64_t mNodeId;
    std::map<TraitDataHandle, CatalogItem *> mItemStore;
    std::queue<TraitDataHandle> mRecycledHandles;
};

typedef GenericTraitCatalogImpl<TraitDataSink> GenericTraitSinkCatalog;
typedef GenericTraitCatalogImpl<TraitDataSource> GenericTraitSourceCatalog;

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_GENERIC_TRAIT_CATALOG_IMPL_CURRENT_H

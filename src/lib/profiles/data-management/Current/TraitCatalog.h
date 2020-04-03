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
    TraitPath(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyPathHandle) :
        mTraitDataHandle(aDataHandle), mPropertyPathHandle(aPropertyPathHandle)
    { }
    bool operator ==(const TraitPath & rhs) const
    {
        return ((rhs.mTraitDataHandle == mTraitDataHandle) && (rhs.mPropertyPathHandle == mPropertyPathHandle));
    }
    bool IsValid() { return !(mPropertyPathHandle == kNullPropertyPathHandle); }
    TraitDataHandle mTraitDataHandle;
    PropertyPathHandle mPropertyPathHandle;
};

struct VersionedTraitPath : public TraitPath
{
    VersionedTraitPath() { }
    VersionedTraitPath(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyPathHandle,
                       SchemaVersionRange aRequestedVersionRange) :
        TraitPath(aDataHandle, aPropertyPathHandle)
    {
        mRequestedVersionRange = aRequestedVersionRange;
    }

    SchemaVersionRange mRequestedVersionRange;
};

/**
 * Trait handle iterator
 */
typedef void (*IteratorCallback)(void * aTraitInstance, TraitDataHandle aHandle, void * aContext);

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
    virtual WEAVE_ERROR AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                        SchemaVersionRange & aSchemaVersionRange) const = 0;

    /**
     * Given a trait handle, write out the TLV for the Path::kCsTag_RootSection structure.
     */
    virtual WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter,
                                        SchemaVersionRange & aSchemaVersionRange) const = 0;

    /**
     * Given a handle, return a reference to the matching trait data instance.
     */
    virtual WEAVE_ERROR Locate(TraitDataHandle aHandle, T ** aTraitInstance) const = 0;

    /**
     * Reverse
     */
    virtual WEAVE_ERROR Locate(T * aTraitInstance, TraitDataHandle & aHandle) const = 0;

    /**
     * Dispatch an event to all trait data instance housed in this catalog.
     */
    virtual WEAVE_ERROR DispatchEvent(uint16_t aEvent, void * aContext) const = 0;

    virtual void Iterate(IteratorCallback aCallback, void * aContext) = 0;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE || WEAVE_CONFIG_ENABLE_WDM_CUSTOM_COMMAND_SENDER
    virtual WEAVE_ERROR GetInstanceId(TraitDataHandle aHandle, uint64_t &aInstanceId) const = 0;

    virtual WEAVE_ERROR GetResourceId(TraitDataHandle aHandle, ResourceIdentifier &aResourceId) const = 0;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE || WEAVE_CONFIG_ENABLE_WDM_CUSTOM_COMMAND_SENDER
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#include <Weave/Profiles/data-management/Current/SingleResourceTraitCatalog.h>

#endif // _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CATALOG_CURRENT_H

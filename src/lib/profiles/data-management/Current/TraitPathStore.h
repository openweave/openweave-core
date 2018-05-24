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
 *    Defines TraitPathStore: a data structure to store lists or sets of TraitPaths.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_TRAIT_PATH_STORE_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_TRAIT_PATH_STORE_CURRENT_H

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

struct TraitPathStore
{
    public:
        enum Flag {
            kFlag_None       = 0x0,
            kFlag_InUse      = 0x1,
            kFlag_Failed     = 0x2, /**< The item is in use, but is not valid anymore.
                                      */
            kFlag_ForceMerge = 0x4, /**< In UpdateRequest, DataElements are encoded with the "replace" format by
                                      default; this flag is used to force the encoding of
                                      dictionaries so that the items are merged.
                                      */
            kFlag_Private    = 0x8, /**< The path was created internally by the engine
                                      to encode a dictionary in its own separate
                                      DataElement; the application is notified about it
                                      only if the update fails.
                                      */

            kFlags_ReservedFlags = kFlag_InUse,
        };
        typedef uint8_t Flags;

        struct Record {
            Flags mFlags;
            TraitPath mTraitPath;
        };

        TraitPathStore();

        void Init(Record *aRecordArray, size_t aNumItems);

        bool IsEmpty() { return mNumItems == 0; }
        bool IsFull() { return mNumItems >= mStoreSize; }
        size_t GetNumItems() { return mNumItems; }
        size_t GetPathStoreSize() { return mStoreSize; }

        WEAVE_ERROR AddItem(const TraitPath &aItem);
        WEAVE_ERROR AddItem(const TraitPath &aItem, Flags aFlags);
        WEAVE_ERROR AddItemDedup(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine);
        WEAVE_ERROR InsertItemAt(size_t aIndex, const TraitPath &aItem, Flags aFlags);
        WEAVE_ERROR InsertItemAfter(size_t aIndex, const TraitPath &aItem, Flags aFlags) { return InsertItemAt(aIndex+1, aItem, aFlags); }

        void SetFailed(size_t aIndex) { SetFlag(aIndex, kFlag_Failed, true); }
        void SetFailedTrait(TraitDataHandle aDataHandle);

        void GetItemAt(size_t aIndex, TraitPath &aTraitPath) { aTraitPath = mStore[aIndex].mTraitPath; }

        size_t GetFirstValidItem() const;
        size_t GetNextValidItem(size_t i) const;
        size_t GetFirstValidItem(TraitDataHandle aTraitDataHandle) const;
        size_t GetNextValidItem(size_t i, TraitDataHandle aTraitDataHandle) const;

        void RemoveTrait(TraitDataHandle aDataHandle);
        void RemoveItemAt(size_t aIndex);

        void Compact();

        void Clear();

        bool IsPresent(const TraitPath &aItem) const;
        bool Includes(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine) const;
        bool Intersects(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine) const;
        bool IsTraitPresent(TraitDataHandle aDataHandle) const;
        bool IsItemInUse(size_t aIndex) const { return IsFlagSet(aIndex, kFlag_InUse); }
        bool IsItemValid(size_t aIndex) const { return (IsItemInUse(aIndex) && (!IsFlagSet(aIndex, kFlag_Failed))); }
        bool IsItemFailed(size_t aIndex) const { return IsFlagSet(aIndex, kFlag_Failed); }
        bool IsItemForceMerge(size_t aIndex) const { return IsFlagSet(aIndex, kFlag_ForceMerge); }
        bool IsItemPrivate(size_t aIndex) const { return IsFlagSet(aIndex, kFlag_Private); }

        bool IsFlagSet(size_t aIndex, Flag aFlag) const { return ((mStore[aIndex].mFlags & static_cast<Flags>(aFlag)) == aFlag); }
        WEAVE_ERROR SetFlags(size_t aIndex, Flag aFlag, bool aValue);
        Flags GetFlags(size_t aIndex) const { return mStore[aIndex].mFlags; }

        Record *mStore;

   private:
        size_t FindFirstAvailableItem() const;
        void SetItem(size_t aIndex, const TraitPath &aItem, Flags aFlags);
        void ClearItem(size_t aIndex);
        void SetFlag(size_t aIndex, Flag aFlag, bool aValue);

        size_t mStoreSize;
        size_t mNumItems;
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_TRAIT_PATH_STORE_CURRENT_H

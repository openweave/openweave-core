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


struct TraitPathStore
{
    public:
        enum Flag {
            kFlag_None       = 0x0,
            kFlag_InUse      = 0x1,
            kFlag_Failed     = 0x2, /**< The item is in use, but is not valid anymore.
                                      */
            kFlag_ForceMerge = 0x4, /**< Paths are encoded with the "replace" format by
                                      default; this flag is used to force the encoding of
                                      dictionaries so that the items are merged.
                                      */
            kFlag_Private    = 0x8, /**< The path was created internally by the engine
                                      to encode a dictionary in its own separate
                                      DataElement.
                                      */
        };
        typedef uint8_t Flags;

        struct Record {
            Flags mFlags;
            TraitPath mTraitPath;
        };

        TraitPathStore();

        void Init(Record *aRecordArray, uint32_t aNumItems);

        bool IsEmpty();
        bool IsFull();
        uint32_t GetNumItems();
        uint32_t GetPathStoreSize();

        WEAVE_ERROR AddItem(TraitPath aItem, bool aForceMerge = false, bool aPrivate = false);
        WEAVE_ERROR AddItem(TraitPath aItem, Flags aFlags);
        WEAVE_ERROR InsertItemAfter(uint32_t aIndex, TraitPath aItem, bool aForceMerge = false, bool aPrivate = false);
        WEAVE_ERROR InsertItemAfter(uint32_t aIndex, TraitPath aItem, Flags aFlags);

        void SetFailed(uint32_t aIndex) { SetFlag(aIndex, kFlag_Failed, true); }
        void SetFailedTrait(TraitDataHandle aDataHandle);

        void GetItemAt(uint32_t aIndex, TraitPath &aTraitPath) { aTraitPath = mStore[aIndex].mTraitPath; }

        void RemoveItem(TraitDataHandle aDataHandle);
        void RemoveItemAt(uint32_t aIndex);

        void Compact();

        void Clear();

        bool Includes(TraitPath aItem, const TraitSchemaEngine * const apSchemaEngine);
        bool Intersects(TraitPath aItem, const TraitSchemaEngine * const apSchemaEngine);
        bool IsPresent(TraitPath aItem);
        bool IsTraitPresent(TraitDataHandle aDataHandle);
        bool IsItemInUse(uint32_t aIndex) { return IsFlagSet(aIndex, kFlag_InUse); }
        bool IsItemValid(uint32_t aIndex) { return (IsItemInUse(aIndex) && (!IsFlagSet(aIndex, kFlag_Failed))); }
        bool IsItemFailed(uint32_t aIndex) { return IsFlagSet(aIndex, kFlag_Failed); }
        bool IsItemForceMerge(uint32_t aIndex) { return IsFlagSet(aIndex, kFlag_ForceMerge); }
        bool IsItemPrivate(uint32_t aIndex) { return IsFlagSet(aIndex, kFlag_Private); }

        bool IsFlagSet(uint32_t aIndex, Flag aFlag) { return ((mStore[aIndex].mFlags & static_cast<Flags>(aFlag)) == aFlag); }
        void SetFlag(uint32_t aIndex, Flag aFlag, bool aValue);
        Flags GetFlags(uint32_t aIndex) { return mStore[aIndex].mFlags; }

        Record *mStore;
        uint32_t mStoreSize;
        uint32_t mNumItems;
};


#endif // _WEAVE_DATA_MANAGEMENT_TRAIT_PATH_STORE_CURRENT_H

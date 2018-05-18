/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *    Implements TraitPathStore: a data structure to store lists or sets of TraitPaths.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Profiles/data-management/TraitPathStore.h>
#include <Weave/Support/WeaveFaultInjection.h>


using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::DataManagement_Current;


TraitPathStore::TraitPathStore()
    : mStore(NULL), mStoreSize(0), mNumItems(0)
{
}

void TraitPathStore::Init(TraitPathStore::Record *aRecordArray, uint32_t aArrayLength)
{
    mStore = aRecordArray;
    mStoreSize = aArrayLength;

    Clear();
}

bool TraitPathStore::IsEmpty()
{
    return mNumItems == 0;
}

bool TraitPathStore::IsFull()
{
    return mNumItems >= mStoreSize;
}

uint32_t TraitPathStore::GetNumItems()
{
    return mNumItems;
}

uint32_t TraitPathStore::GetPathStoreSize()
{
    return mStoreSize;
}

WEAVE_ERROR TraitPathStore::AddItem(TraitPath aItem, Flags aFlags)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;

    for (size_t i = 0; i < mStoreSize; i++)
    {
        if (!IsItemInUse(i))
        {
            mStore[i].mTraitPath = aItem;
            mStore[i].mFlags = aFlags;
            SetFlag(i, kFlag_InUse, true);
            mNumItems++;
            err = WEAVE_NO_ERROR;
            break;
        }
    }

    return err;
}

// Assumes the array is compacted!
WEAVE_ERROR TraitPathStore::InsertItemAfter(uint32_t aIndex, TraitPath aItem, Flags aFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t destIndex = aIndex + 1;
    size_t numItemsToMove = mNumItems - destIndex;
    size_t numBytesToMove = numItemsToMove * sizeof(mStore[0]);

    VerifyOrExit(false == IsFull(), err = WEAVE_ERROR_NO_MEMORY);

    if (numItemsToMove > 0)
    {
        memmove(&mStore[destIndex+1], &mStore[destIndex], numBytesToMove);
    }

    mStore[destIndex].mTraitPath = aItem;
    mStore[destIndex].mFlags = aFlags;
    SetFlag(destIndex, kFlag_InUse, true);

    mNumItems++;

exit:
    return err;
}

WEAVE_ERROR TraitPathStore::AddItem(TraitPath aItem, bool aForceMerge, bool aPrivate)
{
    Flags flags = static_cast<Flags>(kFlag_None);

    if (aForceMerge)
    {
        flags |= static_cast<Flags>(kFlag_ForceMerge);
    }
    if (aPrivate)
    {
        flags |= static_cast<Flags>(kFlag_Private);
    }

    return AddItem(aItem, flags);
}

WEAVE_ERROR TraitPathStore::InsertItemAfter(uint32_t aIndex, TraitPath aItem, bool aForceMerge, bool aPrivate)
{
    Flags flags = static_cast<Flags>(kFlag_None);

    if (aForceMerge)
    {
        flags |= static_cast<Flags>(kFlag_ForceMerge);
    }
    if (aPrivate)
    {
        flags |= static_cast<Flags>(kFlag_Private);
    }

    return InsertItemAfter(aIndex, aItem, flags);
}

void TraitPathStore::RemoveItem(TraitDataHandle aDataHandle)
{
    for (size_t i = 0; i < mStoreSize; i++)
    {
        if (IsItemInUse(i) && (mStore[i].mTraitPath.mTraitDataHandle == aDataHandle))
        {
            RemoveItemAt(i);
        }
    }
}

bool TraitPathStore::IsTraitPresent(TraitDataHandle aDataHandle)
{
    bool found = false;

    if (mNumItems == 0)
        ExitNow();

    for (size_t i = 0; i < mStoreSize; i++)
    {
        if (IsItemInUse(i) && (mStore[i].mTraitPath.mTraitDataHandle == aDataHandle))
        {
            found = true;
            break;
        }
    }

exit:
    return found;
}

void TraitPathStore::SetFailedTrait(TraitDataHandle aDataHandle)
{
    if (mNumItems == 0)
        ExitNow();

    for (size_t i = 0; i < mStoreSize; i++)
    {
        if (IsItemInUse(i) && (mStore[i].mTraitPath.mTraitDataHandle == aDataHandle))
        {
            SetFailed(i);
        }
    }

exit:
    return;
}

void TraitPathStore::SetFlag(uint32_t aIndex, Flag aFlag, bool aValue)
{
    mStore[aIndex].mFlags &= ~aFlag;
    if (aValue)
    {
        mStore[aIndex].mFlags |= static_cast<Flags>(aFlag);
    }
}

void TraitPathStore::RemoveItemAt(uint32_t aIndex)
{
    VerifyOrDie(mNumItems >0);

    if (IsItemInUse(aIndex))
    {
        SetFlag(aIndex, kFlag_InUse, false);
        mNumItems--;
    }
}

void TraitPathStore::Compact()
{
    uint32_t numItemsRemaining = mNumItems;
    size_t numBytesToMove, numItemsToMove;
    const size_t lastIndex = mStoreSize -1;
    size_t i = 0;

    while (i < mStoreSize && numItemsRemaining > 0)
    {
        if (IsItemInUse(i))
        {
            numItemsRemaining--;
            i++;
            continue;
        }

        numItemsToMove = lastIndex -i;
        numBytesToMove = numItemsToMove * sizeof(mStore[0]);
        memmove(&mStore[i], &mStore[i+1], numBytesToMove);
        SetFlag(lastIndex, kFlag_InUse, false);
    }
}

bool TraitPathStore::IsPresent(TraitPath aItem)
{
    for (size_t i = 0; i < mStoreSize; i++)
    {
        if (IsItemValid(i) && (mStore[i].mTraitPath == aItem))
        {
            return true;
        }
    }

    return false;
}

bool TraitPathStore::Intersects(TraitPath aItem, const TraitSchemaEngine * const apSchemaEngine)
{
    bool found = false;
    TraitDataHandle curDataHandle = aItem.mTraitDataHandle;

    for (size_t i = 0; i < mStoreSize; i++)
    {
        PropertyPathHandle pathHandle = aItem.mPropertyPathHandle;
        if (! (IsItemValid(i) && (mStore[i].mTraitPath.mTraitDataHandle == curDataHandle)))
        {
            continue;
        }
        if (pathHandle == mStore[i].mTraitPath.mPropertyPathHandle ||
                mStore[i].mTraitPath.mPropertyPathHandle == kRootPropertyPathHandle ||
                apSchemaEngine->IsParent(pathHandle, mStore[i].mTraitPath.mPropertyPathHandle) ||
                pathHandle == kRootPropertyPathHandle ||
                apSchemaEngine->IsParent(mStore[i].mTraitPath.mPropertyPathHandle, pathHandle))
        {
            found = true;
            break;
        }
    }

    return found;
}

bool TraitPathStore::Includes(TraitPath aItem, const TraitSchemaEngine * const apSchemaEngine)
{
    bool found = false;
    TraitDataHandle dataHandle = aItem.mTraitDataHandle;

    // Check if the store contains aItem or one of its uncestors.
    for (size_t i = 0; i < mStoreSize; i++)
    {
        PropertyPathHandle pathHandle = aItem.mPropertyPathHandle;
        if (! (IsItemValid(i) && (mStore[i].mTraitPath.mTraitDataHandle == dataHandle)))
        {
            continue;
        }
        if (pathHandle == mStore[i].mTraitPath.mPropertyPathHandle ||
                mStore[i].mTraitPath.mPropertyPathHandle == kRootPropertyPathHandle ||
                apSchemaEngine->IsParent(pathHandle, mStore[i].mTraitPath.mPropertyPathHandle))
        {
            found = true;
            break;
        }
    }

    return found;
}

void TraitPathStore::Clear()
{
    mNumItems = 0;

    for (size_t i = 0; i < mStoreSize; i++)
    {
        mStore[i].mFlags                         = 0x0;
        mStore[i].mTraitPath.mPropertyPathHandle = kNullPropertyPathHandle;
        mStore[i].mTraitPath.mTraitDataHandle    = UINT16_MAX;
    }
}


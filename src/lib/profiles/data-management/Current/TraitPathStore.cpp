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

/**
 * Empty constructor
 */
TraitPathStore::TraitPathStore()
    : mStore(NULL), mStoreSize(0), mNumItems(0)
{
}

/**
 * Inits the TraitPathStore
 *
 * @param[in]   aRecordArray    Pointer to an array of Records that will be used
 *                              to store paths and flags.
 * @param[in]   aArrayLength    Length of the storage array in number of items.
 */
void TraitPathStore::Init(TraitPathStore::Record *aRecordArray, size_t aArrayLength)
{
    mStore = aRecordArray;
    mStoreSize = aArrayLength;

    Clear();
}

/**
 * @fn bool TraitPathStore::IsEmpty()
 * @return  Returns true if the store is empty; false otherwise.
 */

/**
 * @fn bool TraitPathStore::IsFull()
 * @return  Returns true if the store is full; false otherwise.
 */

/**
 * @fn size_t TraitPathStore::GetNumItems()
 * @return  Returns the number of TraitPaths in the store.
 */

/**
 * @fn size_t TraitPathStore::GetPathStoreSize()
 * @return  Returns the capacity of the store.
 */

/**
 * Adds a TraitPath to the store with a given set of flags
 *
 * @param[in]   aItem   The TraitPath to be stored
 * @param[in]   aFlags  The flags to be set to true for the item being added
 *
 * @retval WEAVE_NO_ERROR                   in case of success.
 * @retval WEAVE_ERROR_WDM_PATH_STORE_FULL  if the store is full.
 * @retval WEAVE_ERROR_INVALID_ARGUMENT     if aFlags contains reserved flags
 */
WEAVE_ERROR TraitPathStore::AddItem(const TraitPath &aItem, Flags aFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t i = 0;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_PathStoreFull, return WEAVE_ERROR_WDM_PATH_STORE_FULL);

    VerifyOrExit((aFlags & static_cast<Flags>(kFlag_ReservedFlags)) == 0x0,
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    i = FindFirstAvailableItem();
    VerifyOrExit(i < mStoreSize, err = WEAVE_ERROR_WDM_PATH_STORE_FULL);

    SetItem(i, aItem, aFlags);
    mNumItems++;

exit:
    return err;
}

/**
 * Adds a TraitPath to the store.
 *
 * @param[in]   aItem   The TraitPath to be stored
 *
 * @retval WEAVE_NO_ERROR                   in case of success.
 * @retval WEAVE_ERROR_WDM_PATH_STORE_FULL  if the store is full.
 */
WEAVE_ERROR TraitPathStore::AddItem(const TraitPath &aItem)
{
    return AddItem(aItem, kFlag_None);
}

WEAVE_ERROR TraitPathStore::AddItemDedup(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (Includes(aItem, aSchemaEngine))
    {
        WeaveLogDetail(DataManagement, "Path already present");
        ExitNow();
    }

    // Remove any paths of which aItem is an ancestor
    for (size_t i = GetFirstValidItem(aItem.mTraitDataHandle);
            i < GetPathStoreSize();
            i = GetNextValidItem(i, aItem.mTraitDataHandle))
    {
        if (aSchemaEngine->IsParent(mStore[i].mTraitPath.mPropertyPathHandle, aItem.mPropertyPathHandle))
        {
            WeaveLogDetail(DataManagement, "Removing item %u t%u p%u while adding p%u", i,
                    mStore[i].mTraitPath.mTraitDataHandle,
                    mStore[i].mTraitPath.mPropertyPathHandle,
                    aItem.mPropertyPathHandle);
            RemoveItemAt(i);
        }
    }

    err = AddItem(aItem, kFlag_None);

exit:
    return err;
}

/**
 * Adds an TraitPath to the store, inserting it at a given index.
 * Assumes the store has no gaps.
 *
 * @param[in]   aIndex  The index at which to insert the TraitPath; the insertion
 *                      has to keep the store compacted.
 * @param[in]   aFlags  The flags to be set to true for the item being added.
 *
 * @retval WEAVE_ERROR_INCORRECT_STATE      if the store has gaps.
 * @retval WEAVE_ERROR_INVALID_ARGUMENT     if adding the TraitPath at aIndex would make
 *                                          the store not compact.
 * @retval WEAVE_ERROR_WDM_PATH_STORE_FULL  if the store is full.
 * @retval WEAVE_NO_ERROR                   in case of success.
 */
WEAVE_ERROR TraitPathStore::InsertItemAt(size_t aIndex, const TraitPath &aItem, Flags aFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t numItemsToMove;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_PathStoreFull, return WEAVE_ERROR_WDM_PATH_STORE_FULL);

    VerifyOrExit(false == IsFull(), err = WEAVE_ERROR_WDM_PATH_STORE_FULL);
    VerifyOrExit(FindFirstAvailableItem() == mNumItems, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(aIndex <= mNumItems, err = WEAVE_ERROR_INVALID_ARGUMENT);

    numItemsToMove = mNumItems - aIndex;

    if (numItemsToMove > 0)
    {
        memmove(&mStore[aIndex+1], &mStore[aIndex],
                numItemsToMove * sizeof(mStore[0]));
        SetFlags(aIndex, kFlag_InUse, false);
    }

    SetItem(aIndex, aItem, aFlags);
    mNumItems++;

exit:
    return err;
}

/**
 * Remove all TraitPaths that refer to a given TraitDataHandle
 *
 * @param[in]   aDataHandle     The TraitDataHandle
 */
void TraitPathStore::RemoveTrait(TraitDataHandle aDataHandle)
{
    for (size_t i = GetFirstValidItem(aDataHandle);
            i < mStoreSize;
            i = GetNextValidItem(i, aDataHandle))
    {
        RemoveItemAt(i);
    }
}

/**
 * @param[in] aDataHandle   The TraitDataHandle to look for.
 * @return  Returns true if the store contains one or more paths referring
 *          to the given TraitDataHandle
 */
bool TraitPathStore::IsTraitPresent(TraitDataHandle aDataHandle) const
{
    size_t i = GetFirstValidItem(aDataHandle);

    return i < mStoreSize;
}

/**
 * Mark all TraitPaths as failed.
 */
void TraitPathStore::SetFailed()
{
    for (size_t i = GetFirstValidItem();
            i < mStoreSize;
            i = GetNextValidItem(i))
    {
        SetFailed(i);
    }
}

/**
 * Mark all TraitPaths referring to the given TraitDataHandle as failed.
 *
 * @param aDataHandle   The TraitDataHandle to look for.
 */
void TraitPathStore::SetFailedTrait(TraitDataHandle aDataHandle)
{
    for (size_t i = GetFirstValidItem(aDataHandle);
            i < mStoreSize;
            i = GetNextValidItem(i, aDataHandle))
    {
        SetFailed(i);
    }
}

void TraitPathStore::RemoveItem(const TraitPath &aItem)
{
    for (size_t i = GetFirstValidItem(aItem.mTraitDataHandle);
         i < GetPathStoreSize();
         i = GetNextValidItem(i, aItem.mTraitDataHandle))
    {
        if (mStore[i].mTraitPath.mPropertyPathHandle == aItem.mPropertyPathHandle)
        {
            RemoveItemAt(i);
            WeaveLogDetail(DataManagement, "Removing item %u t%u p%u", i,
                           mStore[i].mTraitPath.mTraitDataHandle,
                           mStore[i].mTraitPath.mPropertyPathHandle);
        }
    }
}

void TraitPathStore::RemoveItemAt(size_t aIndex)
{
    VerifyOrDie(mNumItems > 0);
    VerifyOrDie(IsItemInUse(aIndex));

    if (IsItemInUse(aIndex))
    {
        ClearItem(aIndex);
        mNumItems--;
    }
}

/**
 * Compacts the store moving all items in use down towards
 * the start of the array.
 * This is useful to use TraitPathStore to implement a list
 * that can be edited (like the list of in-progress paths maintained
 * by SubscriptionClient).
 */
void TraitPathStore::Compact()
{
    size_t numItemsRemaining = mNumItems;
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
        SetFlags(lastIndex, kFlag_InUse, false);
    }
}

/**
 * Checks if a given TraitPath is already in the store.
 *
 * @param[in] aItem The TraitPath to look for.
 *
 * @return Returns true if the store contains aItem.
 */
bool TraitPathStore::IsPresent(const TraitPath &aItem) const
{
    for (size_t i = GetFirstValidItem(); i < mStoreSize; i = GetNextValidItem(i))
    {
        if (mStore[i].mTraitPath == aItem)
        {
            return true;
        }
    }

    return false;
}

/**
 * Check if any of the TraitPaths in the store intersects a given TraitPath.
 * Two TraitPaths intersect each other if any of the following is true:
 * - the two TraitPaths are the same;
 * - one of the two TraitPaths is an ancestor of the other TraitPath.
 *
 * @param[in]   aTraitPath  The TraitPath to be checked against the store.
 * @param[in]   aSchemaEngine   A pointer to the TraitSchemaEngine for the trait instance
 *                              aTraitPath refers to.
 *
 * @return  true if the store intersects the given TraitPath; false otherwise.
 */
bool TraitPathStore::Intersects(const TraitPath &aTraitPath, const TraitSchemaEngine * const aSchemaEngine) const
{
    bool intersects = false;
    TraitDataHandle dataHandle = aTraitPath.mTraitDataHandle;
    PropertyPathHandle pathHandle = aTraitPath.mPropertyPathHandle;

    for (size_t i = GetFirstValidItem(dataHandle); i < mStoreSize; i = GetNextValidItem(i, dataHandle))
    {
        if (pathHandle == mStore[i].mTraitPath.mPropertyPathHandle ||
                aSchemaEngine->IsParent(pathHandle, mStore[i].mTraitPath.mPropertyPathHandle) ||
                aSchemaEngine->IsParent(mStore[i].mTraitPath.mPropertyPathHandle, pathHandle))
        {
            intersects = true;
            break;
        }
    }

    return intersects;
}

/**
 * Check if any of the TraitPaths in the store includes a given TraitPath.
 * TraitPath A includes TraitPath B if either:
 * - the two TraitPaths are the same;
 * - A is an ancestor of B.
 *
 * @param[in]   aTraitPath      The TraitPath to be checked against the store.
 * @param[in]   aSchemaEngine   A pointer to the TraitSchemaEngine for the trait instance
 *                              aTraitPath refers to.
 *
 * @return  true if the TraitPath is already included by the paths in the store.
 */
bool TraitPathStore::Includes(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine) const
{
    bool found = false;
    TraitDataHandle dataHandle = aItem.mTraitDataHandle;
    PropertyPathHandle pathHandle = aItem.mPropertyPathHandle;

    for (size_t i = GetFirstValidItem(dataHandle); i < mStoreSize; i = GetNextValidItem(i, dataHandle))
    {
        if (pathHandle == mStore[i].mTraitPath.mPropertyPathHandle ||
                aSchemaEngine->IsParent(pathHandle, mStore[i].mTraitPath.mPropertyPathHandle))
        {
            found = true;
            break;
        }
    }

    return found;
}

/**
 * Empties the store.
 */
void TraitPathStore::Clear()
{
    mNumItems = 0;

    for (size_t i = 0; i < mStoreSize; i++)
    {
        ClearItem(i);
    }
}

/**
 * @return The index of the first item of the store for which IsValidItem()
 *          returns true.
 *          If no item is valid, the function returns mStoreSize.
 */
size_t TraitPathStore::GetFirstValidItem() const
{
    size_t i = 0;

    if (mNumItems == 0)
    {
        i = mStoreSize;
    }

    while (i < mStoreSize && false == IsItemValid(i))
    {
        i++;
    }

    return i;
}

/**
 * @param[in]   aIndex   An index into the store.
 * @return The index of the first item of the store following i for which IsValidItem()
 *          returns true.
 *          If i is the last valid item, the function returns mStoreSize.
 */
size_t TraitPathStore::GetNextValidItem(size_t aIndex) const
{
    do
    {
        aIndex++;
    }
    while (aIndex < mStoreSize && false == IsItemValid(aIndex));

    return aIndex;
}

/**
 * @param[in] aTDH The TraitDataHandle of the trait instance to iterate on.
 *
 * @return The index of the first item of the store for which IsValidItem()
 *          returns true and which refers to the TraitDataHandle passed in.
 *          If no item is valid, the function returns mStoreSize.
 */
size_t TraitPathStore::GetFirstValidItem(TraitDataHandle aTDH) const
{
    size_t i = GetFirstValidItem();

    while (i < mStoreSize && mStore[i].mTraitPath.mTraitDataHandle != aTDH)
    {
        i = GetNextValidItem(i);
    }

    return i;
}

/**
 * @param[in]   aIndex   An index into the store.
 * @param[in] aTDH The TraitDataHandle of the trait instance to iterate on.
 *
 * @return The index of the first item of the store following i for which IsValidItem()
 *          returns true and which refers to the TraitDataHandle passed in.
 *          If i is the last valid item for the trait instance, the function returns mStoreSize.
 */
size_t TraitPathStore::GetNextValidItem(size_t aIndex, TraitDataHandle aTDH) const
{
    do
    {
        aIndex = GetNextValidItem(aIndex);
    }
    while (aIndex < mStoreSize && mStore[aIndex].mTraitPath.mTraitDataHandle != aTDH);

    return aIndex;
}

bool TraitPathStore::AreFlagsSet(size_t aIndex, Flags aFlags) const
{
    if ((aFlags & kFlag_ReservedFlags) != 0x0)
    {
        return false;
    }

    return AreFlagsSet_private(aIndex, aFlags);
}

// Private members

size_t TraitPathStore::FindFirstAvailableItem() const
{
    size_t i = 0;

    while (i < mStoreSize && IsItemInUse(i))
    {
        i++;
    }

    return i;
}

void TraitPathStore::SetItem(size_t aIndex, const TraitPath &aItem, Flags aFlags)
{
    mStore[aIndex].mTraitPath = aItem;
    mStore[aIndex].mFlags = aFlags;
    SetFlags(aIndex, kFlag_InUse, true);
}

void TraitPathStore::ClearItem(size_t aIndex)
{
    mStore[aIndex].mFlags                         = 0x0;
    mStore[aIndex].mTraitPath.mPropertyPathHandle = kNullPropertyPathHandle;
    mStore[aIndex].mTraitPath.mTraitDataHandle    = UINT16_MAX;
}

void TraitPathStore::SetFlags(size_t aIndex, Flags aFlags, bool aValue)
{
    mStore[aIndex].mFlags &= ~aFlags;
    if (aValue)
    {
        mStore[aIndex].mFlags |= aFlags;
    }
}

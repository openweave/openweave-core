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
 *      This file implements notification engine for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif //__STDC_LIMIT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Profiles/time/WeaveTime.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BasicGraphSolver
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NotificationEngine::BasicGraphSolver::IsPropertyPathSupported(PropertyPathHandle aHandle)
{
    // Only support subscriptions to root with the basic solver.
    if (aHandle != kRootPropertyPathHandle)
    {
        return false;
    }
    else
    {
        return true;
    }
}

WEAVE_ERROR NotificationEngine::BasicGraphSolver::RetrieveTraitInstanceData(NotifyRequestBuilder * aBuilder,
                                                                            TraitDataHandle aTraitDataHandle,
                                                                            SchemaVersion aSchemaVersion, bool aRetrieveAll)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = aBuilder->WriteDataElement(aTraitDataHandle, kRootPropertyPathHandle, aSchemaVersion, NULL, 0, NULL, 0);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::BasicGraphSolver::SetDirty(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyHandle)
{
    SubscriptionEngine * subEngine = SubscriptionEngine::GetInstance();

    // Iterate over all subscriptions and their trait instance info lists and mark them dirty as appropriate
    for (int i = 0; i < SubscriptionEngine::kMaxNumSubscriptionHandlers; ++i)
    {
        SubscriptionHandler * subHandler = &subEngine->mHandlers[i];

        if (subHandler->IsActive())
        {
            SubscriptionHandler::TraitInstanceInfo * traitInstance = subHandler->GetTraitInstanceInfoList();

            for (size_t j = 0; j < subHandler->GetNumTraitInstances(); j++)
            {
                if (traitInstance[j].mTraitDataHandle == aDataHandle)
                {
                    WeaveLogDetail(DataManagement, "<BSolver:SetD> Set S%u:T%u dirty", i, j);
                    traitInstance[j].SetDirty();
                }
            }
        }
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NotificationEngine::BasicGraphSolver::ClearDirty()
{
    return WEAVE_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IntermediateGraphSolver::Store
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NotificationEngine::IntermediateGraphSolver::Store::Store()
{
    mNumItems = 0;

    for (size_t i = 0; i < WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; i++)
    {
        mStore[i].mPropertyPathHandle = kNullPropertyPathHandle;
        mStore[i].mTraitDataHandle    = UINT16_MAX;
        mValidFlags[i]                = false;
    }
}

bool NotificationEngine::IntermediateGraphSolver::Store::AddItem(TraitPath aItem)
{
    if (mNumItems >= WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE)
    {
        return false;
    }

    for (size_t i = 0; i < WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; i++)
    {
        if (!mValidFlags[i])
        {
            mStore[i]      = aItem;
            mValidFlags[i] = true;
            mNumItems++;
            return true;
        }
    }

    return false;

    // Shouldn't get here since that would imply that mNumItems and mValidFlags are out of sync
    // which should never happen unless someone mucked with the flags themselves. Continuing past
    // this point runs the risk of unpredictable behavior and so, it's better to just assert
    // at this point.
    VerifyOrDie(0);
}

void NotificationEngine::IntermediateGraphSolver::Store::RemoveItem(TraitDataHandle aDataHandle)
{
    if (mNumItems)
    {
        for (size_t i = 0; i < WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; i++)
        {
            if (mValidFlags[i] && (mStore[i].mTraitDataHandle == aDataHandle))
            {
                mValidFlags[i] = false;
                mNumItems--;
            }
        }
    }
}

void NotificationEngine::IntermediateGraphSolver::Store::RemoveItemAt(uint32_t aIndex)
{
    if (mNumItems)
    {
        mValidFlags[aIndex] = false;
        mNumItems--;
    }
}

bool NotificationEngine::IntermediateGraphSolver::Store::IsPresent(TraitPath aItem)
{
    for (size_t i = 0; i < WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE; i++)
    {
        if (mValidFlags[i] && (mStore[i] == aItem))
        {
            return true;
        }
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IntermediateGraphSolver
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool NotificationEngine::IntermediateGraphSolver::IsPropertyPathSupported(PropertyPathHandle aHandle)
{
    // The intermediate solver also only supports subscribing to root.
    return BasicGraphSolver::IsPropertyPathSupported(aHandle);
}

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
WEAVE_ERROR NotificationEngine::IntermediateGraphSolver::DeleteKey(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyHandle)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t i;
    SubscriptionEngine * subEngine = SubscriptionEngine::GetInstance();
    TraitDataSource * dataSource;

    WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> T%u::(%u:%u), CurDeleteItems = %u/%u", aDataHandle,
                   GetPropertyDictionaryKey(aPropertyHandle), GetPropertySchemaHandle(aPropertyHandle), mDeleteStore.GetNumItems(),
                   WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE);

    // Check if the data source is already marked dirty at the root.
    err = subEngine->mPublisherCatalog->Locate(aDataHandle, &dataSource);
    SuccessOrExit(err);

    // Set the subscribers dirty.
    err = BasicGraphSolver::SetDirty(aDataHandle, aPropertyHandle);
    SuccessOrExit(err);

    // if it's marked root dirty already, nothing more to be done!
    VerifyOrExit(!dataSource->IsRootDirty(), WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> Already root dirty!");
                 err = WEAVE_NO_ERROR);

    // if previously present in the delete store, nothing more to be done!
    if (mDeleteStore.IsPresent(TraitPath(aDataHandle, aPropertyHandle)))
    {
        WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> Previously dirty");
        return WEAVE_NO_ERROR;
    }

    // If we have exceeded the num items in the store, we need to mark the whole trait instance as dirty and remove all
    // existing references to this trait instance in the delete store.
    if (mDeleteStore.IsFull())
    {
        WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> No more space in granular store!");

        mDeleteStore.RemoveItem(aDataHandle);

        // Mark the data source is being entirely dirty.
        dataSource->SetRootDirty();
    }
    else
    {
        mDeleteStore.AddItem(TraitPath(aDataHandle, aPropertyHandle));

        // If we are deleting something, we need to remove any prior additions to this dictionary for this item.
        for (i = 0; i < mDirtyStore.GetStoreSize(); i++)
        {
            if (mDirtyStore.mValidFlags[i] && (mDirtyStore.mStore[i].mTraitDataHandle == aDataHandle))
            {
                if (mDirtyStore.mStore[i].mPropertyPathHandle == aPropertyHandle ||
                        dataSource->GetSchemaEngine()->IsParent(mDirtyStore.mStore[i].mPropertyPathHandle, aPropertyHandle))
                {
                    WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> Removing previously added dirty handle (%u:%u)",
                                   GetPropertyDictionaryKey(mDirtyStore.mStore[i].mPropertyPathHandle),
                                   GetPropertySchemaHandle(mDirtyStore.mStore[i].mPropertyPathHandle));
                    mDirtyStore.RemoveItemAt(i);
                }
            }
        }
    }

exit:
    return err;
}
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

WEAVE_ERROR NotificationEngine::IntermediateGraphSolver::SetDirty(TraitDataHandle aDataHandle, PropertyPathHandle aPropertyHandle)
{
    WEAVE_ERROR err                = WEAVE_NO_ERROR;
    SubscriptionEngine * subEngine = SubscriptionEngine::GetInstance();
    TraitDataSource * dataSource;

    WeaveLogDetail(DataManagement, "<ISolver:SetDirty> T%u::(%u:%u), CurDirtyItems = %u/%u", aDataHandle,
                   GetPropertyDictionaryKey(aPropertyHandle), GetPropertySchemaHandle(aPropertyHandle), mDirtyStore.GetNumItems(),
                   WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE);

    // Check if the data source is already marked dirty at the root.
    err = subEngine->mPublisherCatalog->Locate(aDataHandle, &dataSource);
    SuccessOrExit(err);

    // Set the subscribers dirty.
    err = BasicGraphSolver::SetDirty(aDataHandle, aPropertyHandle);
    SuccessOrExit(err);

    // if it's marked root dirty already, nothing more to be done!
    VerifyOrExit(!dataSource->IsRootDirty(), WeaveLogDetail(DataManagement, "<ISolver:SetDirty> Already root dirty!");
                 err = WEAVE_NO_ERROR);

    // if previously present in the delete store, nothing more to be done!
    if (mDirtyStore.IsPresent(TraitPath(aDataHandle, aPropertyHandle)))
    {
        WeaveLogDetail(DataManagement, "<ISolver:SetDirty> Previously dirty");
        return WEAVE_NO_ERROR;
    }

    // If we have exceeded the num items in the store, we need to mark the whole trait instance as dirty and remove all
    // existing references to this trait instance in the dirty store.
    if (mDirtyStore.IsFull())
    {
        WeaveLogDetail(DataManagement, "<ISolver:SetDirty> No more space in granular store!");

        mDirtyStore.RemoveItem(aDataHandle);

        // Mark the data source is being entirely dirty.
        dataSource->SetRootDirty();
    }
    else
    {
        PropertyPathHandle handleToAdd = aPropertyHandle;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        // If we're adding/modifying a dictionary element, remove any previous deletions of this element to maintain correctness.
        for (size_t i = 0; i < mDeleteStore.GetStoreSize(); i++)
        {
            if (mDeleteStore.mValidFlags[i] && (mDeleteStore.mStore[i].mTraitDataHandle == aDataHandle))
            {
                if (aPropertyHandle == mDeleteStore.mStore[i].mPropertyPathHandle ||
                        dataSource->GetSchemaEngine()->IsParent(aPropertyHandle, mDeleteStore.mStore[i].mPropertyPathHandle))
                {
                    WeaveLogDetail(DataManagement, "<ISolver:DeleteKey> Removing previously deleted element (%u:%u)",
                                   GetPropertyDictionaryKey(mDeleteStore.mStore[i].mPropertyPathHandle),
                                   GetPropertySchemaHandle(mDeleteStore.mStore[i].mPropertyPathHandle));

                    // Given that the handle to add could be a deep leaf path within the dictionary element, we need to actually
                    // mark the root dictionary element as being dirty in the case where we previously were tracking a deletion to
                    // this item. Otherwise, we'll just send a modification to the leaf part of the element which will be incorrect.
                    dataSource->GetSchemaEngine()->IsInDictionary(aPropertyHandle, handleToAdd);
                    VerifyOrExit(handleToAdd != kNullPropertyPathHandle, err = WEAVE_ERROR_INCORRECT_STATE);

                    mDeleteStore.RemoveItemAt(i);
                }
            }
        }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

        mDirtyStore.AddItem(TraitPath(aDataHandle, handleToAdd));
    }

exit:
    return err;
}

PropertyPathHandle NotificationEngine::IntermediateGraphSolver::GetNextCandidateHandle(uint32_t & aChangeStoreCursor,
                                                                                       TraitDataHandle aTargetDataHandle,
                                                                                       bool & aCandidateHandleIsDelete)
{
    PropertyPathHandle candidateHandle = kNullPropertyPathHandle;

    while (aChangeStoreCursor < mDirtyStore.GetStoreSize())
    {
        TraitPath dirtyPath = mDirtyStore.mStore[aChangeStoreCursor];

        if (mDirtyStore.mValidFlags[aChangeStoreCursor] && (dirtyPath.mTraitDataHandle == aTargetDataHandle))
        {
            candidateHandle          = dirtyPath.mPropertyPathHandle;
            aCandidateHandleIsDelete = false;
            aChangeStoreCursor++;
            break;
        }

        aChangeStoreCursor++;
    }

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    while (aChangeStoreCursor >= mDirtyStore.GetStoreSize() &&
           aChangeStoreCursor < (mDeleteStore.GetStoreSize() + mDirtyStore.GetStoreSize()))
    {
        TraitPath deletePath = mDeleteStore.mStore[aChangeStoreCursor - mDirtyStore.GetStoreSize()];

        if (mDeleteStore.mValidFlags[aChangeStoreCursor - mDirtyStore.GetStoreSize()] &&
            (deletePath.mTraitDataHandle == aTargetDataHandle))
        {
            candidateHandle          = deletePath.mPropertyPathHandle;
            aCandidateHandleIsDelete = true;
            aChangeStoreCursor++;
            break;
        }

        aChangeStoreCursor++;
    }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

    return candidateHandle;
}

WEAVE_ERROR NotificationEngine::IntermediateGraphSolver::RetrieveTraitInstanceData(NotifyRequestBuilder * aBuilder,
                                                                                   TraitDataHandle aTraitDataHandle,
                                                                                   SchemaVersion aSchemaVersion, bool aRetrieveAll)
{
    WEAVE_ERROR err;
    PropertyPathHandle mergeHandleSet[WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET]  = { kNullPropertyPathHandle };
    PropertyPathHandle deleteHandleSet[WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET] = { kNullPropertyPathHandle };
    int32_t numMergeHandles                                                                    = 0;
    int32_t numDeleteHandles                                                                   = 0;
    PropertyPathHandle currentCommonHandle                                                     = kNullPropertyPathHandle;
    TraitDataSource * dataSource;
    const TraitSchemaEngine * schemaEngine;

    err = SubscriptionEngine::GetInstance()->mPublisherCatalog->Locate(aTraitDataHandle, &dataSource);
    SuccessOrExit(err);

    schemaEngine = dataSource->GetSchemaEngine();
    WeaveLogDetail(DataManagement, "<ISolver::Retr> CurDirtyItems = %u/%u", mDirtyStore.GetNumItems(),
                   WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE);

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    WeaveLogDetail(DataManagement, "<ISolver::Retr> CurDeleteItems = %u/%u", mDeleteStore.GetNumItems(),
                   WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE);
#endif

    // If we are told to retrieve all (i.e root), our job here is done
    if (aRetrieveAll)
    {
        WeaveLogDetail(DataManagement, "<ISolver::Retr> Retrieving all!");
        currentCommonHandle = kRootPropertyPathHandle;
    }
    // If the data source as a whole has been marked dirty, our job here is done
    else if (dataSource->IsRootDirty())
    {
        WeaveLogDetail(DataManagement, "<ISolver::Retr> Root is dirty!");
        currentCommonHandle = kRootPropertyPathHandle;
    }
    else
    {
        PropertyPathHandle nextCommonHandle, candidateHandle;
        PropertyPathHandle laggingHandles[2] = { kNullPropertyPathHandle, kNullPropertyPathHandle };
        bool oldCandidateHandleIsDelete = false, candidateHandleIsDelete = false;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        bool modifyDeleteToModify = false;
#endif

        uint32_t changeStoreCursor = 0;

        //
        // This loop forms the crux of the TDM part of the NotificationEngine. It is responsible for gathering up the dirty bits
        // within a data source instance and generating a *single* data element that maximally encompasses all that dirtiness. To do
        // so, it iteratively computes a 'nextCommonHandle' that is the parent to all dirty path handles accumulated up to each
        // iteration. This parent handle is termed as the Lowest Common Ancestor, or LCA.
        //
        // The WDM protocol rules state that all handles in the data at the first level(i.e immediate children of the handle
        // referenced by the path) are to be merged into the eventual data, while data at the 2nd level and beyond are to be
        // replaced. The algorithm below tries to exploit the merge semantics to just send the handles that are dirty relative to
        // the common handle. Given the handle set is finitely sized, an overflow of that set results in all child handles being
        // merged in.
        //
        // It also deals with deletions as well. Deletions are treated somewhat similarly to modifications/additions from the algo
        // perspective with some minor adjustments:
        //
        //      1. Deletions are only applicable so long as all deletions apply to the same dictionary. Once we have deletions that
        //         span multiple dictionaries, we cannot express a deletion anymore and the deletion is treated like a modify/add
        //         from the algorithm perspective for the purposes of computing the LCA and adding entries to the merge handle set.
        //
        //      2. Deletions can co-exist with modifications/additions to the same dictionary. If there are mods/adds present in
        //      other parts of the tree/other dictionaries, the deletion reverts to the same treatment as mentioned in 1)
        //
        // Key Variables:
        //
        //      currentCommonHandle = The current LCA of all handles evaluated thus far.
        //
        //      candidateHandle = The next handle picked out from either the dirty or delete stores that will be evaluated against
        //                   the current common handle to compute the next common handle
        //
        //      nextCommonHandle = The next computed LCA of the current handle and the candidate handle
        //
        //      laggingHandles = immediate children of the newly computed LCA that encompass the two
        //                   candidates passed into the LCA computation function respectively. If either of the two input handles
        //                   passed in match the newly computed LCA, the lagging handle will be set to kNullPropertyPathHandle
        //
        //      mergeHandleSet = set of handles that will be merged in relative to the currentCommonHandle. If empty, all children
        //                   under the commonHandle will be included.
        //
        while ((candidateHandle = GetNextCandidateHandle(changeStoreCursor, aTraitDataHandle, candidateHandleIsDelete)) !=
               kNullPropertyPathHandle)
        {
            oldCandidateHandleIsDelete = candidateHandleIsDelete;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
            // This flag tracks whether we have stopped trying to express deletions (setup in previous iterations) and now have
            // reverted to converting them over to look like adds/modifies. This variable will remain set in this value for
            // remaining iterations.
            if (modifyDeleteToModify)
            {
                candidateHandleIsDelete = false;
            }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

            WeaveLogDetail(DataManagement, "Candidate Handle = %u:%u (%c -> %c)", GetPropertyDictionaryKey(candidateHandle),
                           GetPropertySchemaHandle(candidateHandle), oldCandidateHandleIsDelete ? 'D' : 'M',
                           candidateHandleIsDelete ? 'D' : 'M');

            //
            // Evaluate the next LCA
            //
            // Given our current common ancestor handle and our candidate handle, we compute the next LCA.  The next common handle
            // will be stored in 'nextCommonHandle' while the two lagging branches will be represented through laggingHandles[0] and
            // laggingHandles[1]. [0] will correspond to the lagging branch for the current common handle while [1] will correspond
            // to that for the candidate handle.
            //
            if (currentCommonHandle == kNullPropertyPathHandle)
            {
                // If we're first starting out, we need to pick a sensible common handle. Unlike modifications where the LCA is the
                // first modified/added handle we encounter, deletions need to be expressed relative to the parent dictionary
                // handle. Hence, we setup it up to look like a 'merge' by having the common handle point to the dictionary and the
                // lagging handle point to the deleted element.
                if (candidateHandleIsDelete)
                {
                    nextCommonHandle  = schemaEngine->GetParent(candidateHandle);
                    laggingHandles[0] = kNullPropertyPathHandle;
                    laggingHandles[1] = candidateHandle;
                }
                else
                {
                    nextCommonHandle = candidateHandle;
                }

                WeaveLogDetail(DataManagement, "<ISolver::Retr> (%c) nextCommonHandle = %u:%u", candidateHandleIsDelete ? 'D' : 'M',
                               GetPropertyDictionaryKey(nextCommonHandle), GetPropertySchemaHandle(nextCommonHandle));
            }
            else
            {
                // Find the lowest common parent of the currently tracked common handle and the next item in the dirty set. Also,
                // return the two child handles that lag the ancestor that are parents of the two input handles to the LCA.
                nextCommonHandle = schemaEngine->FindLowestCommonAncestor(currentCommonHandle, candidateHandle, &laggingHandles[0],
                                                                          &laggingHandles[1]);
                VerifyOrExit(nextCommonHandle != kNullPropertyPathHandle, err = WEAVE_ERROR_INVALID_ARGUMENT);

                WeaveLogDetail(DataManagement,
                               "<ISolver::Retr> (%c) nextCommonHandle += (%u:%u) = (%u:%u) (Lag-set = (%u:%u), (%u:%u))",
                               candidateHandleIsDelete ? 'D' : 'M', GetPropertyDictionaryKey(candidateHandle),
                               GetPropertySchemaHandle(candidateHandle), GetPropertyDictionaryKey(nextCommonHandle),
                               GetPropertySchemaHandle(nextCommonHandle), GetPropertyDictionaryKey(laggingHandles[0]),
                               GetPropertySchemaHandle(laggingHandles[0]), GetPropertyDictionaryKey(laggingHandles[1]),
                               GetPropertySchemaHandle(laggingHandles[1]));
            }

            // If we compute a new next handle, we'll need to wipe our merge handle set since the old set of merge/delete handles
            // were referenced against a now-stale handle
            if (currentCommonHandle != nextCommonHandle)
            {
                WeaveLogDetail(DataManagement, "<ISolver::Retr> (%c) nextHandle != currentHandle, wiping merge/delete sets",
                               candidateHandleIsDelete ? 'D' : 'M');
                numMergeHandles  = 0;
                numDeleteHandles = 0;
            }

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
            if (candidateHandleIsDelete)
            {
                // The deleteHandleSet only makes sense as long as the next common handle is the parent of the delete set.
                // If not, we start treating it as a add/modify.
                if (nextCommonHandle == schemaEngine->GetParent(candidateHandle))
                {
                    int32_t i;

                    for (i = 0; i < numDeleteHandles; i++)
                    {
                        if (deleteHandleSet[i] == laggingHandles[1])
                        {
                            WeaveLogDetail(DataManagement, "<ISolver::Retr> (D) Handle (%u:%u) already present",
                                           GetPropertyDictionaryKey(laggingHandles[1]), GetPropertySchemaHandle(laggingHandles[1]));
                            break;
                        }
                    }

                    if (i == numDeleteHandles)
                    {
                        // If our delete handle set overflows, we degenerate to expressing the deletes as a replacement of the
                        // dictionary itself.
                        if (numDeleteHandles >= WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET)
                        {
                            WeaveLogDetail(DataManagement, "<ISolver::Retr> (D) delete set overflowed, converting to replace");

                            laggingHandles[0] = kNullPropertyPathHandle;
                            laggingHandles[1] = nextCommonHandle;
                            nextCommonHandle  = schemaEngine->GetParent(nextCommonHandle);

                            numMergeHandles  = 0;
                            numDeleteHandles = 0;

                            candidateHandleIsDelete = false;
                            modifyDeleteToModify    = true;
                        }
                        else
                        {
                            WeaveLogDetail(DataManagement,
                                           "<ISolver::Retr> (D) Adding delete handle = (%u:%u) (numCurHandles = %u)",
                                           GetPropertyDictionaryKey(laggingHandles[1]), GetPropertySchemaHandle(laggingHandles[1]),
                                           numDeleteHandles + 1);
                            deleteHandleSet[numDeleteHandles++] = laggingHandles[1];

                            // There's always a possibility that the other lagging handle was pointing to a modified/added handle.
                            // We set the laggingHandle[1] as null to prevent it from getting added but set candidateHandleIsDelete
                            // to false to force it get evaluated in the section below for addition to the mergeHandleSet.
                            laggingHandles[1]       = kNullPropertyPathHandle;
                            candidateHandleIsDelete = false;
                        }
                    }
                }
                else
                {
                    WeaveLogDetail(DataManagement, "<ISolver::Retr> (D) Making delete a merge instead");
                    candidateHandleIsDelete = false;
                }
            }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

            if (!candidateHandleIsDelete)
            {
                // If our next handle matches the current dirty handle, we know we cannot do a merge so wipe the merge set.
                if (nextCommonHandle == candidateHandle)
                {
                    numMergeHandles = 0;

                    WeaveLogDetail(DataManagement, "<ISolver::Retr> (M) next is dirty handle - wiping merge set");

                    // We make a small exception if the dirty handle is a dictionary - it doesn't make a lot of sense to mark a
                    // dictionary as dirty if you were just intending to convey modifications/additions only. Instead, let's do a
                    // replace given that makes more sense for a dynamic data type like this.
                    if (schemaEngine->IsDictionary(candidateHandle))
                    {
                        WeaveLogDetail(DataManagement, "<ISolver::Retr> (M) next is dictionary - setting up replace");
                        mergeHandleSet[0] = candidateHandle;
                        nextCommonHandle  = schemaEngine->GetParent(candidateHandle);
                        numMergeHandles   = 1;
                    }
                }
                else
                {
                    for (size_t i = 0; i < 2; i++)
                    {
                        if (laggingHandles[i] != kNullPropertyPathHandle)
                        {
                            int j;

                            for (j = 0; j < numMergeHandles; j++)
                            {
                                if (mergeHandleSet[j] == laggingHandles[i])
                                {
                                    WeaveLogDetail(DataManagement, "<ISolver::Retr> (M) Handle (%u:%u) already present",
                                                   GetPropertyDictionaryKey(laggingHandles[i]),
                                                   GetPropertySchemaHandle(laggingHandles[i]));
                                    break;
                                }
                            }

                            if (numMergeHandles >= 0 && j == numMergeHandles)
                            {
                                if (numMergeHandles >= WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET)
                                {
                                    WeaveLogDetail(DataManagement, "<ISolver::Retr> (M) merge set overflowed");
                                    numMergeHandles = -1;
                                }
                                else
                                {
                                    WeaveLogDetail(DataManagement, "<ISolver::Retr> (M) Merge handle = (%u:%u) (numhandles = %u)",
                                                   GetPropertyDictionaryKey(laggingHandles[i]),
                                                   GetPropertySchemaHandle(laggingHandles[i]), numMergeHandles + 1);
                                    mergeHandleSet[numMergeHandles++] = laggingHandles[i];
                                }
                            }
                        }
                    }
                }
            }

            currentCommonHandle = nextCommonHandle;
        }
    }

    // If our algo is working correctly, currentCommonHandle should always be pointing to a valid handle. This is always the case
    // since a) this function only gets called if we know there is dirtiness in this trait and b) the current common handle is
    // always a function of the dirty handle set, which by definition, cannot be null.
    VerifyOrDie(currentCommonHandle != kNullPropertyPathHandle);

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    // If we're expressing a deletion (i.e numDeleteHandles > 0), then it has to be done against a path that points to a dictionary.
    // If that isn't the case, something really wrong has happened in the algorithm above.
    if (numDeleteHandles > 0)
    {
        VerifyOrDie(schemaEngine->IsDictionary(currentCommonHandle));
    }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

    WeaveLogDetail(DataManagement, "<ISolver::Retr> Final handle = (%u:%u), numMergeHandles = %d, numDeleteHandles = %d",
                   GetPropertyDictionaryKey(currentCommonHandle), GetPropertySchemaHandle(currentCommonHandle), numMergeHandles,
                   numDeleteHandles);

    // if we overflow, let's clear them back to 0.
    if (numMergeHandles < 0)
    {
        numMergeHandles = 0;
    }

    // Generate data elements
    err = aBuilder->WriteDataElement(aTraitDataHandle, currentCommonHandle, aSchemaVersion, mergeHandleSet, numMergeHandles,
                                     deleteHandleSet, numDeleteHandles);
    SuccessOrExit(err);

exit:
    return err;
}

void NotificationEngine::IntermediateGraphSolver::ClearTraitInstanceDirty(void * aDataSource, TraitDataHandle aDataHandle,
                                                                          void * aContext)
{
    TraitDataSource * dataSource = static_cast<TraitDataSource *>(aDataSource);
    dataSource->ClearRootDirty();
}

WEAVE_ERROR NotificationEngine::IntermediateGraphSolver::ClearDirty()
{
    // Iterate over every publisher trait instance and clear their dirty field.
    SubscriptionEngine::GetInstance()->mPublisherCatalog->Iterate(ClearTraitInstanceDirty, this);

    // Clear out our granular dirty store.
    mDirtyStore.Clear();

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    mDeleteStore.Clear();
#endif

    return WEAVE_NO_ERROR;
}

void NotificationEngine::IntermediateGraphSolver::Store::Clear()
{
    mNumItems = 0;
    memset(mValidFlags, 0, sizeof(mValidFlags));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NotifyRequestBuilder
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::Init(PacketBuffer * aBuf, TLV::TLVWriter * aWriter,
                                                           SubscriptionHandler * aSubHandler,
                                                           uint32_t aMaxPayloadSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aBuf != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mWriter         = aWriter;
    mState          = kNotifyRequestBuilder_Idle;
    mBuf            = aBuf;
    mSub            = aSubHandler;
    mMaxPayloadSize = aMaxPayloadSize;

exit:

    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::StartNotifyRequest()
{
    TLVType dummyType;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit((mState == kNotifyRequestBuilder_Idle) && (mBuf != NULL), err = WEAVE_ERROR_INCORRECT_STATE);

    mWriter->Init(mBuf, mMaxPayloadSize);

    err = mWriter->StartContainer(AnonymousTag, kTLVType_Structure, dummyType);
    SuccessOrExit(err);

    if (mSub)
    {
        err = mWriter->Put(ContextTag(BaseMessageWithSubscribeId::kCsTag_SubscriptionId), mSub->mSubscriptionId);
        SuccessOrExit(err);
    }

    mState = kNotifyRequestBuilder_Ready;

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::EndNotifyRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kNotifyRequestBuilder_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->EndContainer(kTLVType_NotSpecified);
    SuccessOrExit(err);

    err = mWriter->Finalize();
    SuccessOrExit(err);

    mState = kNotifyRequestBuilder_Idle;

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::StartDataList()
{
    TLVType dummyType; // Per spec requirement, will be set to TLVStructure
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kNotifyRequestBuilder_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->StartContainer(ContextTag(NotificationRequest::kCsTag_DataList), kTLVType_Array, dummyType);
    SuccessOrExit(err);

    mState = kNotifyRequestBuilder_BuildDataList;

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::EndDataList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kNotifyRequestBuilder_BuildDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->EndContainer(kTLVType_Structure); // corresponds to dummyType in Start*List
    SuccessOrExit(err);

    mState = kNotifyRequestBuilder_Ready;
exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::StartEventList()
{
    TLVType dummyType; // Per spec requirement, will be set to TLVStructure
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kNotifyRequestBuilder_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->StartContainer(ContextTag(NotificationRequest::kCsTag_EventList), kTLVType_Array, dummyType);
    SuccessOrExit(err);

    mState = kNotifyRequestBuilder_BuildEventList;

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::EndEventList()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kNotifyRequestBuilder_BuildEventList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->EndContainer(kTLVType_Structure); // corresponds to dummyType in Start*List
    SuccessOrExit(err);

    mState = kNotifyRequestBuilder_Ready;
exit:
    return err;
}

WEAVE_ERROR
NotificationEngine::NotifyRequestBuilder::WriteDataElement(TraitDataHandle aTraitDataHandle, PropertyPathHandle aPropertyPathHandle,
                                                           SchemaVersion aSchemaVersion, PropertyPathHandle * aMergeDataHandleSet,
                                                           uint32_t aNumMergeDataHandles, PropertyPathHandle * aDeleteHandleSet,
                                                           uint32_t aNumDeleteHandles)
{
    WEAVE_ERROR err;
    TLVType dummyContainerType;
    TraitDataSource * dataSource;
    bool retrievingData = false;
    SchemaVersionRange versionRange;

    VerifyOrExit(mState == kNotifyRequestBuilder_BuildDataList, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mWriter->StartContainer(AnonymousTag, kTLVType_Structure, dummyContainerType);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->mPublisherCatalog->Locate(aTraitDataHandle, &dataSource);
    SuccessOrExit(err);

    versionRange.mMaxVersion = aSchemaVersion;
    versionRange.mMinVersion = dataSource->GetSchemaEngine()->GetLowestCompatibleVersion(versionRange.mMaxVersion);

    err = mWriter->StartContainer(ContextTag(DataElement::kCsTag_Path), kTLVType_Path, dummyContainerType);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->mPublisherCatalog->HandleToAddress(aTraitDataHandle, *mWriter, versionRange);
    SuccessOrExit(err);

    err = dataSource->GetSchemaEngine()->MapHandleToPath(aPropertyPathHandle, *mWriter);
    SuccessOrExit(err);

    err = mWriter->EndContainer(dummyContainerType);
    SuccessOrExit(err);

    err = mWriter->Put(ContextTag(DataElement::kCsTag_Version), dataSource->GetVersion());
    SuccessOrExit(err);

    if (aNumMergeDataHandles > 0 || aNumDeleteHandles > 0)
    {
        const TraitSchemaEngine * schemaEngine = dataSource->GetSchemaEngine();

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        if (aNumDeleteHandles > 0)
        {
            err =
                mWriter->StartContainer(ContextTag(DataElement::kCsTag_DeletedDictionaryKeys), kTLVType_Array, dummyContainerType);
            SuccessOrExit(err);

            for (size_t i = 0; i < aNumDeleteHandles; i++)
            {
                err = mWriter->Put(AnonymousTag, GetPropertyDictionaryKey(aDeleteHandleSet[i]));
                SuccessOrExit(err);
            }

            err = mWriter->EndContainer(dummyContainerType);
            SuccessOrExit(err);
        }
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

        if (aNumMergeDataHandles > 0)
        {
            err = mWriter->StartContainer(ContextTag(DataElement::kCsTag_Data), kTLVType_Structure, dummyContainerType);
            SuccessOrExit(err);

            retrievingData = true;

            for (size_t i = 0; i < aNumMergeDataHandles; i++)
            {
                WeaveLogDetail(DataManagement, "<NE::WriteDE> Merging in 0x%08x", aMergeDataHandleSet[i]);

                err = dataSource->ReadData(aMergeDataHandleSet[i], schemaEngine->GetTag(aMergeDataHandleSet[i]), *mWriter);
                SuccessOrExit(err);
            }

            retrievingData = false;

            err = mWriter->EndContainer(dummyContainerType);
            SuccessOrExit(err);
        }
    }
    else
    {
        retrievingData = true;

        err = dataSource->ReadData(aPropertyPathHandle, ContextTag(DataElement::kCsTag_Data), *mWriter);
        SuccessOrExit(err);

        retrievingData = false;
    }

    err = mWriter->EndContainer(kTLVType_Array);
    SuccessOrExit(err);

exit:
    if (retrievingData && err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DataManagement, "Error retrieving data from trait (instanceHandle: %u, profileId: %08x), err = %d",
                      aTraitDataHandle, dataSource->GetSchemaEngine()->GetProfileId(), err);
    }

    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::MoveToState(NotifyRequestBuilderState aDesiredState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If we're already in the correct builder state, exit without doing anything else
    if (aDesiredState == mState)
    {
        ExitNow();
    }

    // Get to the toplevel of the request
    switch (mState)
    {
    case kNotifyRequestBuilder_Idle:
        err = StartNotifyRequest();
        break;
    case kNotifyRequestBuilder_Ready:
        break;
    case kNotifyRequestBuilder_BuildDataList:
        err = EndDataList();
        break;
    case kNotifyRequestBuilder_BuildEventList:
        err = EndEventList();
        break;
    }

    // verify that we're at the toplevel
    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(DataManagement, "<NE:Builder> Failed to reach Ready: %d", err));
    // Extra paranoia: verify that we're in toplevel state
    VerifyOrExit(mState == kNotifyRequestBuilder_Ready, err = WEAVE_ERROR_INCORRECT_STATE);

    // Now, go to the desired state

    switch (aDesiredState)
    {
    case kNotifyRequestBuilder_Idle:
        err = EndNotifyRequest();
        break;
    case kNotifyRequestBuilder_Ready:
        break;
    case kNotifyRequestBuilder_BuildDataList:
        err = StartDataList();
        break;
    case kNotifyRequestBuilder_BuildEventList:
        err = StartEventList();
        break;
    }
    VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogDetail(DataManagement, "<NE:Builder> Failed to reach desired state: %d", err));
    // Extra paranoia: verify that we're in desired state
    VerifyOrExit(mState == aDesiredState, err = WEAVE_ERROR_INCORRECT_STATE);

exit:
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::Checkpoint(TLVWriter & aPoint)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    aPoint          = *mWriter;
    return err;
}

WEAVE_ERROR NotificationEngine::NotifyRequestBuilder::Rollback(TLVWriter & aPoint)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    *mWriter        = aPoint;
    return err;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NotificationEngine
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WEAVE_ERROR NotificationEngine::Init()
{
    mCurSubscriptionHandlerIdx = 0;
    mCurTraitInstanceIdx       = 0;
    mNumNotifiesInFlight       = 0;

    return WEAVE_NO_ERROR;
}

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
WEAVE_ERROR NotificationEngine::DeleteKey(TraitDataSource * aDataSource, PropertyPathHandle aPropertyHandle)
{
    WEAVE_ERROR err;
    TraitDataHandle dataHandle;
    SubscriptionEngine * subEngine = SubscriptionEngine::GetInstance();
    bool isLocked                  = false;

    err = subEngine->mPublisherCatalog->Locate(aDataSource, dataHandle);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->Lock();
    SuccessOrExit(err);

    isLocked = true;

    err = mGraphSolver.DeleteKey(dataHandle, aPropertyHandle);
    SuccessOrExit(err);

exit:
    if (isLocked)
    {
        SubscriptionEngine::GetInstance()->Unlock();
    }

    return err;
}
#endif // TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT

WEAVE_ERROR NotificationEngine::SetDirty(TraitDataSource * aDataSource, PropertyPathHandle aPropertyHandle)
{
    WEAVE_ERROR err;
    TraitDataHandle dataHandle;
    SubscriptionEngine * subEngine = SubscriptionEngine::GetInstance();
    bool isLocked                  = false;

    err = subEngine->mPublisherCatalog->Locate(aDataSource, dataHandle);
    SuccessOrExit(err);

    err = SubscriptionEngine::GetInstance()->Lock();
    SuccessOrExit(err);

    isLocked = true;

    err = mGraphSolver.SetDirty(dataHandle, aPropertyHandle);
    SuccessOrExit(err);

exit:
    if (isLocked)
    {
        SubscriptionEngine::GetInstance()->Unlock();
    }

    return err;
}

WEAVE_ERROR NotificationEngine::RetrieveTraitInstanceData(SubscriptionHandler * aSubHandler,
                                                          SubscriptionHandler::TraitInstanceInfo * aTraitInfo,
                                                          NotifyRequestBuilder * aBuilder, bool * aPacketFull)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    *aPacketFull = false;

    err = mGraphSolver.RetrieveTraitInstanceData(aBuilder, aTraitInfo->mTraitDataHandle, aTraitInfo->mRequestedVersion,
                                                 aSubHandler->IsSubscribing());
    SuccessOrExit(err);

    // Clear out the dirty bit since we're done processing this trait instance.
    aTraitInfo->ClearDirty();

exit:
    if ((err == WEAVE_ERROR_BUFFER_TOO_SMALL) || (err == WEAVE_ERROR_NO_MEMORY))
    {
        *aPacketFull = true;
        err          = WEAVE_NO_ERROR;
    }

    return err;
}

WEAVE_ERROR NotificationEngine::SendNotify(PacketBuffer * aBuffer, SubscriptionHandler * aSubHandler)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = aSubHandler->SendNotificationRequest(aBuffer);
    SuccessOrExit(err);

    // We can only have 1 notify in flight for any given subscription - increment and break out.
    mNumNotifiesInFlight++;

exit:
    return err;
}

void NotificationEngine::OnNotifyConfirm(SubscriptionHandler * aSubHandler, bool aNotifyDelivered)
{
    VerifyOrDie(mNumNotifiesInFlight > 0);

    WeaveLogDetail(DataManagement, "<NE> OnNotifyConfirm: NumNotifies-- = %d", mNumNotifiesInFlight - 1);
    mNumNotifiesInFlight--;

    if (aNotifyDelivered && aSubHandler->mSubscribeToAllEvents)
    {
        LoggingManagement & logger = LoggingManagement::GetInstance();

        for (int iterator = kImportanceType_First; iterator <= kImportanceType_Last; iterator++)
        {
            size_t i                  = static_cast<size_t>(iterator - kImportanceType_First);
            ImportanceType importance = (ImportanceType) iterator;
            logger.NotifyEventsDelivered(importance, aSubHandler->mSelfVendedEvents[i] - 1, aSubHandler->GetPeerNodeId());
        }
    }

    // Run NE again now that a notify has come back/error'ed out and that we might be able to do more work.
    Run();
}

/**
 *  @brief
 *    Given the `SubscriptionHandler`, fill in the `EventList` element
 *    within the `NotifyRequest`,
 *
 *  The function will fill in a `NotifyRequest`'s `EventList`. If the
 *  event logs occupy more space than available in the current
 *  `NotifyRequest`, the function will only pack enough events to fit
 *  within the buffer and adjust the state of the
 *  `SubscriptionHandler` to resume processing at unprocessed event.
 *  The events are sent in the order of priority. To avoid endless
 *  cycling through events, the function sets the end goal within the
 *  event log that it will reach before it considers the subscription
 *  clean.
 *
 *  @param[in] aSubHandler A pointer to the SubscriptionHandler for
 *                                   which we are attempting to create
 *                                   a `NotifyRequest`
 *
 *  @param[in] aNotificationRequest A builder object encapsulating the
 *                                   request we are trying to build
 *
 *  @param[out] aIsSubscriptionClean A boolean that is set to true
 *                                   when no further traits have data
 *                                   that needs to be processed by the
 *                                   SubscriptionHandler and to false
 *                                   otherwise.
 *
 *  @param[out] aNeWriteInProgress A boolean that is set to true
 *                                   when there is data written in notify request
 *                                   to false otherwise.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval other          The processing failed within the subroutines. The
 *                         errors may have resulted from state
 *                         corruption, TDM errors, insufficient
 *                         buffering, etc.
 */
WEAVE_ERROR NotificationEngine::BuildSingleNotifyRequestEventList(SubscriptionHandler * aSubHandler,
                                                                  NotifyRequestBuilder & aNotifyRequest,
                                                                  bool & aIsSubscriptionClean, bool & aNeWriteInProgress)
{
    WEAVE_ERROR err      = WEAVE_NO_ERROR;
    aIsSubscriptionClean = true;

    event_id_t initialEvents[kImportanceType_Last - kImportanceType_First + 1];
    memcpy(initialEvents, aSubHandler->mSelfVendedEvents, sizeof(initialEvents));

    int event_count = 0;

    // events only enter the picture if the subscription handler is
    // subscribed to events.
    if (aSubHandler->mSubscribeToAllEvents)
    {
        // Verify that we have events to transmit
        LoggingManagement & logger = LoggingManagement::GetInstance();

        // If the logger is not valid or has not been initialized,
        // skip the rest of processing
        VerifyOrExit(logger.IsValid(), /* no-op */);

        for (int i = 0; i < kImportanceType_Last - kImportanceType_First + 1; i++)
        {
            event_id_t tmp_id = logger.GetFirstEventID(static_cast<ImportanceType>(i + 1));
            if (tmp_id > initialEvents[i])
            {
                initialEvents[i] = tmp_id;
            }
        }

        // Check whether we are in a middle of an upload

        if (aSubHandler->mCurrentImportance == kImportanceType_Invalid)
        {
            // Upload is not underway.  Check for new events, and set a checkpoint
            aIsSubscriptionClean = aSubHandler->CheckEventUpToDate(logger);
            if (!aIsSubscriptionClean)
            {
                // We have more events. snapshot last event IDs
                aSubHandler->SetEventLogEndpoint(logger);
            }

            // initialize the next importance level to transfer
            aSubHandler->mCurrentImportance = aSubHandler->FindNextImportanceForTransfer();
        }
        else
        {
            aSubHandler->mCurrentImportance = aSubHandler->FindNextImportanceForTransfer();
            aIsSubscriptionClean            = (aSubHandler->mCurrentImportance == kImportanceType_Invalid);
        }

        // proceed only if there are new events.
        if (aIsSubscriptionClean)
        {
            ExitNow(); // subscription clean, move along
        }

        // Ensure we have a buffer and we've started EventList
        err = aNotifyRequest.MoveToState(kNotifyRequestBuilder_BuildEventList);
        // if we did not have enough space for event list at all,
        // squash the error and exit immediately
        if ((err == WEAVE_ERROR_NO_MEMORY) || (err == WEAVE_ERROR_BUFFER_TOO_SMALL))
        {
            err = WEAVE_NO_ERROR;
            ExitNow();
        }
        SuccessOrExit(err);

        while (aSubHandler->mCurrentImportance != kImportanceType_Invalid)
        {
            size_t i = static_cast<size_t>(aSubHandler->mCurrentImportance - kImportanceType_First);
            err      = logger.FetchEventsSince(*aNotifyRequest.GetWriter(), aSubHandler->mCurrentImportance,
                                          aSubHandler->mSelfVendedEvents[i]);

            if ((err == WEAVE_END_OF_TLV) || (err == WEAVE_ERROR_TLV_UNDERRUN) || (err == WEAVE_NO_ERROR))
            {
                // We have successfully reached the end of the log for
                // the current importance. Advance to the next
                // importance level.
                err                             = WEAVE_NO_ERROR;
                aSubHandler->mCurrentImportance = aSubHandler->FindNextImportanceForTransfer();
            }
            else if ((err == WEAVE_ERROR_BUFFER_TOO_SMALL) || (err == WEAVE_ERROR_NO_MEMORY))
            {
                for (int t = 0; t <= kImportanceType_Last - kImportanceType_First; t++)
                {
                    if (aSubHandler->mSelfVendedEvents[t] > initialEvents[t])
                    {
                        event_count += aSubHandler->mSelfVendedEvents[t] - initialEvents[t];
                    }
                }

                if (event_count > 0)
                {
                    aNeWriteInProgress = true;
                }

                // when first trait event is too big to fit in the packet, ignore that trait event.
                if (!aNeWriteInProgress)
                {
                    aSubHandler->mSelfVendedEvents[i]++;
                    WeaveLogDetail(DataManagement, "<NE:Run> trait event is too big so that it fails to fit in the packet!");
                    err = WEAVE_NO_ERROR;
                }
                else
                {
                    // `FetchEventsSince` has filled the available space
                    // within the allowed buffer before it fit all the
                    // available events.  This is an expected condition,
                    // so we do not propagate the error to higher levels;
                    // instead, we terminate the event processing for now
                    // (we will get another chance immediately afterwards,
                    // with a ew buffer) and do not advance the processing
                    // to the next importance level.
                    err = WEAVE_NO_ERROR;
                    ExitNow();
                }
            }
            else
            {
                // All other errors are propagated to higher level.
                // Exiting here and returning an error will lead to
                // abandoning subscription.
                ExitNow();
            }
        }
    }

exit:

    event_count = 0;

    for (int i = 0; i <= kImportanceType_Last - kImportanceType_First; i++)
    {
        // There are many acceptable situations where the initial event id for
        // an importance buffer is greater than the vended event id for the
        // subscription. We know that we have not loaded any events from that
        // importance into the current NotifyRequest, so we can skip over it
        // for the purposes of the "Fetched events" log line.
        if (aSubHandler->mSelfVendedEvents[i] > initialEvents[i])
        {
            event_count += aSubHandler->mSelfVendedEvents[i] - initialEvents[i];
        }
    }

    WeaveLogDetail(DataManagement, "Fetched %d events", event_count);

    if (event_count > 0)
    {
        aNeWriteInProgress = true;
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DataManagement, "Error retrieving events, err = %d", err);
    }

    return err;
}

/**
 *  @brief
 *    Given the `SubscriptionHandler`, fill in the `DataList` element
 *    within the `NotifyRequest`
 *
 *  The function will fill in a `NotifyRequest`'s `DataList`.  If
 *  the property changes occupy more space than available in the
 *  underlying buffer, the function will only pack enough elements to
 *  fit within the buffer and adjust the state of the
 *  `SubscriptionHandler` to resume processing at the first
 *  unprocessed trait.
 *
 *  @param[in] aSubHandler           A pointer to the SubscriptionHandler for
 *                                   which we are attempting to create
 *                                   a `NotifyRequest`
 *
 *  @param[in] aNotificationRequest  A builder object encapsulating the
 *                                   request we are trying to build
 *
 *  @param[out] aIsSubscriptionClean A boolean that is set to true
 *                                   when no further traits have data
 *                                   that needs to be processed by the
 *                                   SubscriptionHandler and to false
 *                                   otherwise.
 *
 *  @param[out] aNeWriteInProgress   A boolean that is set to true
 *                                   when there is data written in notify request
 *                                   to false otherwise.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval other          The processing failed within the subroutines. The
 *                         errors may have resulted from state
 *                         corruption, TDM errors, insufficient
 *                         buffering, etc.
 */

WEAVE_ERROR NotificationEngine::BuildSingleNotifyRequestDataList(SubscriptionHandler * aSubHandler,
                                                                 NotifyRequestBuilder & aNotifyRequest, bool & aIsSubscriptionClean,
                                                                 bool & aNeWriteInProgress)
{
    WEAVE_ERROR err   = WEAVE_NO_ERROR;
    bool packetIsFull = false;
    SubscriptionHandler::TraitInstanceInfo * traitInfo =
        aSubHandler->GetTraitInstanceInfoList() + aSubHandler->mCurProcessingTraitInstanceIdx;

    while (aSubHandler->mCurProcessingTraitInstanceIdx < aSubHandler->GetNumTraitInstances())
    {
        if (traitInfo->IsDirty())
        {
            aIsSubscriptionClean = false;
            TLVWriter writerCpy;

            WeaveLogDetail(DataManagement, "<NE:Run> T%u is dirty", aSubHandler->mCurProcessingTraitInstanceIdx);

            // Ensure we're in the DataList element.  May allocate memory.
            err = aNotifyRequest.MoveToState(kNotifyRequestBuilder_BuildDataList);
            SuccessOrExit(err);

            // Make a back-up of the writer so that we can rewind back if the next retrieval fails due to the packet getting full.
            aNotifyRequest.Checkpoint(writerCpy);

            // Retrieve data for this trait instance and clear its dirty flag.
            err = RetrieveTraitInstanceData(aSubHandler, traitInfo, &aNotifyRequest, &packetIsFull);
            VerifyOrExit(err == WEAVE_NO_ERROR,
                         WeaveLogError(DataManagement, "<NE:Run> Error retrieving data from trait, aborting"));

            if (packetIsFull)
            {
                WeaveLogDetail(DataManagement, "<NE:Run> Packet got full!");
                // Restore the writer
                aNotifyRequest.Rollback(writerCpy);
                // when first trait property is too big to fit in the packet, ignore that trait property.
                if (!aNeWriteInProgress)
                {
                    WeaveLogDetail(DataManagement, "<NE:Run> trait property is too big so that it fails to fit in the packet");
                    traitInfo->ClearDirty();
                }
                else
                {
                    break;
                }
            }
            else
            {
                aNeWriteInProgress = true;
            }
        }

        aSubHandler->mCurProcessingTraitInstanceIdx++;
        traitInfo++;
    }

    // Only do this if our sub handler is still valid at this point (which it may not be)
    if (aSubHandler->GetNumTraitInstances())
    {
        aSubHandler->mCurProcessingTraitInstanceIdx %= aSubHandler->GetNumTraitInstances();
    }

exit:
    return err;
}

/**
 *  @brief
 *    Build and send a single notify request for a given subscription handler
 *
 *  The function creates and sends a single `NotifyRequest` for a
 *  given `SubscriptionHandler`.  If there are changes in the TDM or
 *  in the event log state, the function will allocate a buffer, fill
 *  it with trait and event data (as appropriate) and send it the
 *  buffer to the subscriber.  If the data to be sent to the
 *  subscriber spans more than a single notify request, the function
 *  must be called multiple times to ensure that all the trait and
 *  event data is synchronized between publisher and subscriber; in
 *  that case, the function will adjust the internal state of the
 *  `SubscriptionHandler` such that the subsequent `NotifyRequest`s
 *  resume at a point where this request left off.
 *
 *  The function prioritizes trait properties over events: the trait
 *  properties are serialized first and events are serialized into
 *  space leftover after the properties have been serialized.
 *
 *  The function allocates at most one `PacketBuffer`.  At the end of
 *  the function, either the ownership of this buffer is passed to the
 *  `WeaveMessageLayer` or the buffer is de-allocated.
 *
 *  If the function encounters any error that's not a WEAVE_ERR_MEM,
 *  the function will abort the subscription.
 *
 *  @param[in] aSubHandler A pointer to the SubscriptionHandler for
 *                                   which we are attempting to create
 *                                   a `NotifyRequest`
 *
 *  @param[out] aSubscriptionHandled An output parameter used in
 *                                   tracking the iteration through
 *                                   subscriptions.
 *
 *  @param[out] aIsSubscriptionClean An output parameter that is set
 *                                   to true if there is no processing
 *                                   left for this
 *                                   `SubscriptionHandler`, and to
 *                                   false otherwise.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERR_MEM The function could not allocate memory. The
 *                         caller may need to abort its current
 *                         iteration and restart processing under less
 *                         memory pressure.
 */

WEAVE_ERROR NotificationEngine::BuildSingleNotifyRequest(SubscriptionHandler * aSubHandler, bool & aSubscriptionHandled,
                                                         bool & aIsSubscriptionClean)
{
    WEAVE_ERROR err    = WEAVE_NO_ERROR;
    PacketBuffer * buf = NULL;
    TLVWriter writer;
    NotifyRequestBuilder notifyRequest;
    bool subClean;
    bool neWriteInProgress = false;
    uint32_t maxNotificationSize = 0;
    uint32_t maxPayloadSize = 0;

    aIsSubscriptionClean = true; // assume no work it to be done

    // If we're picking up from where we left off last, don't assume the subscription will be clean nor handled completely in this
    // evaluation round.
    if (aSubHandler->mCurProcessingTraitInstanceIdx != 0)
    {
        aIsSubscriptionClean = false;
        aSubscriptionHandled = false;
    }

    maxNotificationSize = aSubHandler->GetMaxNotificationSize();

    err = aSubHandler->mBinding->AllocateRightSizedBuffer(buf, maxNotificationSize, WDM_MIN_NOTIFICATION_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    // Create a notify request.
    err = notifyRequest.Init(buf, &writer, aSubHandler, maxPayloadSize);
    SuccessOrExit(err);

    // Fill in the DataList.  Allocation may take place
    subClean = true;

    err = BuildSingleNotifyRequestDataList(aSubHandler, notifyRequest, subClean, neWriteInProgress);
    SuccessOrExit(err);

    aIsSubscriptionClean &= subClean;
    subClean = true;

#if WEAVE_CONFIG_EVENT_LOGGING_WDM_OFFLOAD
    // Fill in the EventList.  Allocation may take place
    err = BuildSingleNotifyRequestEventList(aSubHandler, notifyRequest, subClean, neWriteInProgress);
    SuccessOrExit(err);
#endif

    aIsSubscriptionClean &= subClean;

    // transition request builder to the Init state.  If buffer was
    // not allocated, then the function is a noop.  Otherwise, the TLV
    // elements get closed (through the NotificationRequest), and buf
    // is non-null
    err = notifyRequest.MoveToState(kNotifyRequestBuilder_Idle);
    SuccessOrExit(err);

    // TODO: JIRA-1419. change the NotifyRequest to provide a facility of buffer
    // ownership transfer.  At this point, perhaps it is of minor
    // utility, but it we ever get tooling to track buffer ownership,
    // it would be handy.  As it is, at this point in the code, the
    // request builder should be dead.
    if (neWriteInProgress && buf)
    {
        WeaveLogDetail(DataManagement, "<NE:Run> Sending notify...");

        err = SendNotify(buf, aSubHandler);
        // NULL out the buf since we've handed it over to the message layer
        buf = NULL;
        VerifyOrExit(err == WEAVE_NO_ERROR, WeaveLogError(DataManagement, "<NE:Run> Error sending out notify!"));
    }

exit:
    // On any error, abort the subscription, and consider it handled.
    if (err != WEAVE_NO_ERROR)
    {
        // abort subscription, squash error, signal to upper
        // layers that the subscription is done
        aSubHandler->HandleSubscriptionTerminated(err, NULL);

        aSubscriptionHandled = true;
        err                  = WEAVE_NO_ERROR;
    }

    if (buf != NULL)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
WEAVE_ERROR NotificationEngine::SendSubscriptionlessNotification(Binding * const apBinding,
                                                                 TraitPath *aPathList,
                                                                 uint16_t aPathListSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    ExchangeContext *ec = NULL;
    uint32_t maxPayloadSize = 0;

    VerifyOrExit(apBinding != NULL && aPathList != NULL,
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = apBinding->AllocateRightSizedBuffer(msgBuf, WDM_MAX_NOTIFICATION_SIZE, WDM_MIN_NOTIFICATION_SIZE, maxPayloadSize);
    SuccessOrExit(err);

    // Build Notify Request for subscriptionless notification

    err = BuildSubscriptionlessNotification(msgBuf, maxPayloadSize, aPathList, aPathListSize);
    SuccessOrExit(err);

    err = apBinding->NewExchangeContext(ec);
    SuccessOrExit(err);

    ec->AppState          = this;

    err = ec->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscriptionlessNotification, msgBuf);
    msgBuf = NULL;
    SuccessOrExit(err);

    ec->Close();
    ec = NULL;
exit:

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (NULL != ec)
    {
        ec->Abort();
        ec = NULL;
    }

    return err;
}

WEAVE_ERROR NotificationEngine::BuildSubscriptionlessNotification(PacketBuffer *aMsgBuf, uint32_t maxPayloadSize,
                                                                  TraitPath *aPathList,
                                                                  uint16_t aPathListSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    NotifyRequestBuilder notifyRequest;
    TraitDataSource *dataSource;
    TraitPath *currPath;
    TraitCatalogBase<TraitDataSource> * pubCatalog;
    SchemaVersion schemaVersion;

    VerifyOrExit(aPathList != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    currPath = aPathList;

    // Get a handle of the publisher catalog
    pubCatalog = SubscriptionEngine::GetInstance()->mPublisherCatalog;

    // Create a notify request.
    err = notifyRequest.Init(aMsgBuf, &writer, NULL, maxPayloadSize);
    SuccessOrExit(err);

    // Ensure we're in the DataList element.
    err = notifyRequest.MoveToState(kNotifyRequestBuilder_BuildDataList);
    SuccessOrExit(err);

    // Iterate through the trait path list and populate the notifyrequest with
    // trait instance data.
    for (uint16_t i = 0; i < aPathListSize; i++, currPath++)
    {
        TraitDataHandle traitHandle = currPath->mTraitDataHandle;

        // Get the max version from the datasource
        // Locate() can return an error if the sink has been removed from the catalog. In that case,
        // skip this path
        if (pubCatalog->Locate(traitHandle, &dataSource) == WEAVE_NO_ERROR)
        {
            schemaVersion = dataSource->GetSchemaEngine()->GetMaxVersion();

            err = notifyRequest.WriteDataElement(traitHandle, kRootPropertyPathHandle, schemaVersion, NULL, 0, NULL, 0);
            SuccessOrExit(err);
        }
    }

    err = notifyRequest.MoveToState(kNotifyRequestBuilder_Idle);
    SuccessOrExit(err);

exit:

    return err;
}

#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

void NotificationEngine::Run()
{
    WEAVE_ERROR err                  = WEAVE_NO_ERROR;
    uint32_t numSubscriptionsHandled = 0;
    SubscriptionEngine * subEngine   = SubscriptionEngine::GetInstance();
    SubscriptionHandler * subHandler = subEngine->mHandlers + mCurSubscriptionHandlerIdx;
    bool subscriptionHandled, isSubscriptionClean;
    bool isClean  = true;
    bool isLocked = false;

    // Lock before attempting to modify any of the shared data structures.
    err = subEngine->Lock();
    SuccessOrExit(err);

    isLocked = true;

    WeaveLogDetail(DataManagement, "<NE:Run> NotifiesInFlight = %u", mNumNotifiesInFlight);

    while ((mNumNotifiesInFlight < WDM_PUBLISHER_MAX_NOTIFIES_IN_FLIGHT) &&
           (numSubscriptionsHandled < SubscriptionEngine::kMaxNumSubscriptionHandlers))
    {
        subscriptionHandled = true;

        // limit the prints to handlers that are in meaingful subscribing/notifying states.
        if (subHandler->IsNotifying() || subHandler->IsSubscribing())
        {
            WeaveLogDetail(DataManagement, "<NE:Run> Eval Subscription: %u (state = %s, num-traits = %u)!",
                           mCurSubscriptionHandlerIdx, subHandler->GetStateStr(), subHandler->GetNumTraitInstances());
        }

        if (subHandler->IsNotifiable())
        {
            // This is needed because some error could trigger abort on subscription, which leads to destroy of the handler
            subHandler->_AddRef();
            err = BuildSingleNotifyRequest(subHandler, subscriptionHandled, isSubscriptionClean);
            SuccessOrExit(err);

            if (isSubscriptionClean)
            {
                // TODO: notification based on the event list state.
                subHandler->OnNotifyProcessingComplete(false, NULL, 0);
            }
            subHandler->_Release();
        }

        if (subscriptionHandled)
        {
            numSubscriptionsHandled++;
        }
        else
        {
            WeaveLogDetail(DataManagement, "<NE:Run> Subscription %u not handled", mCurSubscriptionHandlerIdx);
            numSubscriptionsHandled = 0;
        }

        mCurSubscriptionHandlerIdx = (mCurSubscriptionHandlerIdx + 1) % SubscriptionEngine::kMaxNumSubscriptionHandlers;
        subHandler                 = subEngine->mHandlers + mCurSubscriptionHandlerIdx;
    }

    subHandler = subEngine->mHandlers;
    isClean    = true;

    // We only wipe our granular dirty stores if all the subscriptions are clean. To do so, we iterate over
    // all of them and check each of their dirty flags.
    for (int i = 0; i < SubscriptionEngine::kMaxNumSubscriptionHandlers; i++)
    {
        if (subHandler->IsActive())
        {
            SubscriptionHandler::TraitInstanceInfo * traitInfo = subHandler->GetTraitInstanceInfoList();
            for (size_t j = 0; j < subHandler->GetNumTraitInstances(); j++)
            {
                if (traitInfo->IsDirty())
                {
                    WeaveLogDetail(DataManagement, "<NE:Run> S%u:T%u still dirty", i, j);
                    isClean = false;
                    break;
                }

                traitInfo++;
            }
        }

        subHandler++;
    }

    if (isClean)
    {
        WeaveLogDetail(DataManagement, "<NE> Done processing!");
        mGraphSolver.ClearDirty();
    }

exit:
    if (isLocked)
    {
        subEngine->Unlock();
    }

    return;
}

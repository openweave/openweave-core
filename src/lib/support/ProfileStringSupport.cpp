/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements functions to register and deregister
 *      support callbacks, particularly those for returning
 *      human-readable strings, for Weave profiles.
 *
 */

#include <stddef.h>
#include <stdlib.h>

#include <Weave/Support/ProfileStringSupport.hpp>

namespace nl {

namespace Weave {

namespace Support {

/**
 *  Registry singly-linked list head pointer.
 */
static ProfileStringContext *sProfileStringContextHead;

/**
 *  @brief
 *    Find a profile string support context matching the specified
 *    context, based on the profile identifier associated with the
 *    context.
 *
 *  This function finds a profile string support context matching the
 *  specified context, based on the profile identifier associated with
 *  the context.
 *
 *  @param[in]  inContext    A read-only reference to the profile string
 *                           support context to find.
 *
 *  @return a read-only pointer to the profile string support context
 *  if found; otherwise, NULL.
 *
 */
static ProfileStringContext *FindProfileStringContext(const ProfileStringContext &inContext)
{
    ProfileStringContext *current;

    current  = sProfileStringContextHead;

    while (current != NULL)
    {
        if (current->mStringInfo.mProfileId == inContext.mStringInfo.mProfileId)
        {
            break;
        }

        current = current->mNext;
    }

    return (current);
}

/**
 *  @brief
 *    Insert the specified profile string support context into the
 *    registry.
 *
 *  This function inserts the specified profile string support context
 *  into the registry, if not already present, in sorted order, based
 *  on ascending profile identifier.
 *
 *  @param[inout]  inOutContext  A reference to the profile string support
 *                               context that will be inserted into the
 *                               registry. While the context is present in
 *                               the registry, the mNext field may be modified
 *                               as other contexts are inserted or removed.
 *
 *  @return true if the context was inserted; otherwise, false.
 *
 */
static bool InsertProfileStringContext(ProfileStringContext &inOutContext)
{
    ProfileStringContext *current;
    ProfileStringContext *previous;
    bool insert = true;

    previous = NULL;
    current  = sProfileStringContextHead;

    while (current != NULL)
    {
        if (current->mStringInfo.mProfileId == inOutContext.mStringInfo.mProfileId)
        {
            insert = false;

            break;
        }
        else if (current->mStringInfo.mProfileId > inOutContext.mStringInfo.mProfileId)
        {
            insert = true;

            break;
        }

        previous = current;
        current = current->mNext;
    }

    if (insert)
    {
        if (previous != NULL)
        {
            // Insert a body or add a tail element

            inOutContext.mNext = previous->mNext;

            previous->mNext = &inOutContext;
        }
        else
        {
            // Add or replace the head

            if (current != NULL)
            {
                // Replace the head element

                inOutContext.mNext = sProfileStringContextHead;
            }

            sProfileStringContextHead = &inOutContext;
        }
    }

    return (insert);
}

/**
 *  @brief
 *    Remove the specified profile string support context from the
 *    registry.
 *
 *  This function removes the specified profile string support context
 *  from the registry, if present, based on the profile identifier.
 *
 *  @param[inout]  inOutContext  A reference to the profile string support
 *                               context that will be removed, if present.
 *                               When the context is removed, the mNext
 *                               field may be modified.
 *
 *  @return true if the context was removed; otherwise, false.
 *
 */
static bool RemoveProfileStringContext(ProfileStringContext &inOutContext)
{
    ProfileStringContext *current;
    ProfileStringContext *previous;
    bool remove = false;

    previous = NULL;
    current  = sProfileStringContextHead;

    while (current != NULL)
    {
        if (current->mStringInfo.mProfileId == inOutContext.mStringInfo.mProfileId)
        {
            remove = true;

            break;
        }

        previous = current;
        current = current->mNext;
    }

    if (remove)
    {
        if (previous != NULL)
        {
            // Remove a body or tail element

            previous->mNext = current->mNext;
        }
        else
        {
            // Remove the head element

            sProfileStringContextHead = current->mNext;
        }

        current->mNext = NULL;
    }

    return (remove);
}

/**
 *  @brief
 *    Register the provided profile string support callbacks.
 *
 *  This function registers and makes available the provided profile
 *  string support callbacks.
 *
 *  @param[inout]  inOutContext  A reference to the profile string support
 *                               context that will be registered and added
 *                               to the registry. While the context is
 *                               registered, the mNext field may be modified
 *                               as other contexts are registered or
 *                               unregistered.
 *
 *  @retval  #WEAVE_NO_ERROR On success.
 *
 *  @retval  #WEAVE_ERROR_PROFILE_STRING_CONTEXT_ALREADY_REGISTERED If the context is
 *           already registered.
 *
 *  @sa FindStringProfileInfo
 *  @sa UnregisterProfielStringInfo
 *
 */
NL_DLL_EXPORT WEAVE_ERROR RegisterProfileStringInfo(ProfileStringContext &inOutContext)
{
    const bool status = InsertProfileStringContext(inOutContext);

    return ((status)? WEAVE_NO_ERROR : WEAVE_ERROR_PROFILE_STRING_CONTEXT_ALREADY_REGISTERED);
}

/**
 *  @brief
 *    Unregister the provided profile string support callbacks.
 *
 *  This function unregisters and makes unavailable the provided profile
 *  string support callbacks.
 *
 *  @param[inout]  inOutContext  A reference to the profile string support
 *                               context that will be unregistered, if
 *                               registered, and removed from the registry.
 *                               When the context is unregistered, the
 *                               mNext field may be modified.
 *
 *  @retval  #WEAVE_NO_ERROR On success.
 *
 *  @retval  #WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED If the context is not
 *           registered.
 *
 *  @sa FindStringProfileInfo
 *  @sa RegisterProfielStringInfo
 *
 */
NL_DLL_EXPORT WEAVE_ERROR UnregisterProfileStringInfo(ProfileStringContext &inOutContext)
{
    const bool status = RemoveProfileStringContext(inOutContext);

    return ((status) ? WEAVE_NO_ERROR : WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED);
}

/**
 *  @brief
 *    Find, if registered, the profile string support callbacks associated
 *    with the specified profile identifier.
 *
 *  @param[in]  inProfileId  The profile identifier to find string support
 *                           callbacks for.
 *
 *  @return a read-only pointer to the profile string support
 *  callbacks if found; otherwise, NULL.
 *
 *  @sa RegisterProfielStringInfo
 *  @sa UnregisterProfielStringInfo
 *
 */
NL_DLL_EXPORT const ProfileStringInfo *FindProfileStringInfo(uint32_t inProfileId)
{
    const ProfileStringInfo    info      = { inProfileId, NULL, NULL, NULL };
    const ProfileStringContext target    = { NULL, info };
    ProfileStringContext *     context;

    context = FindProfileStringContext(target);

    return ((context != NULL) ? &context->mStringInfo : NULL);
}

}; // namespace Support

}; // namespace Weave

}; // namespace nl

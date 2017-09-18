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
 *      This file defines objects and functions to register and
 *      deregister support callbacks, particularly those for returning
 *      human-readable strings, for Weave profiles.
 *
 */

#ifndef PROFILESUPPORT_HPP
#define PROFILESUPPORT_HPP

#include <stdint.h>

#include <Weave/Core/WeaveError.h>
#include <Weave/Support/NLDLLUtil.h>

namespace nl {

namespace Weave {

namespace Support {

/**
 *  @brief
 *    Typedef for a callback function that returns a human-readable
 *    NULL-terminated C string describing the message type associated
 *    with the specified profile identifier.
 *
 *  This callback, when registered, is invoked when a human-readable
 *  NULL-terminated C string is needed to describe the message type
 *  associated with the specified profile identifier.
 *
 *  @param[in]  inProfileId  The profile identifier associated with the
 *                           specified message type.
 *
 *  @param[in]  inMsgType    The message type for which a human-readable
 *                           descriptive string is sought.
 *
 *  @return a pointer to the NULL-terminated C string if a match is
 *  found; otherwise, NULL.
 *
 */
typedef const char * (*MessageNameFunct)(uint32_t inProfileId, uint8_t inMsgType);

/**
 *  @brief
 *    Typedef for a callback function that returns a human-readable
 *    NULL-terminated C string describing the profile with the
 *    specified profile identifer.
 *
 *  This callback, when registered, is invoked when a human-readable
 *  NULL-terminated C string is needed to describe the profile with
 *  the specified profile identifer.
 *
 *  @param[in]  inProfileId  The profile identifier for which a human-readable
 *                           descriptive string is sought.
 *
 *  @return a pointer to the NULL-terminated C string if a match is
 *  found; otherwise, NULL.
 *
 */
typedef const char * (*ProfileNameFunct)(uint32_t inProfileId);


/**
 *  @brief
 *    Typedef for a callback function that returns a human-readable
 *    NULL-terminated C string describing the status code associated
 *    with the specified profile identifier.
 *
 *  This callback, when registered, is invoked when a human-readable
 *  NULL-terminated C string is needed to describe the status code
 *  associated with the specified profile identifier.
 *
 *  @param[in]  inProfileId   The profile identifier associated with the
 *                            specified status code.
 *
 *  @param[in]  inStatusCode  The status code for which a human-readable
 *                            descriptive string is sought.
 *
 *  @return a pointer to the NULL-terminated C string if a match is
 *  found; otherwise, NULL.
 *
 */
typedef const char * (*StatusReportFormatStringFunct)(uint32_t inProfileId, uint16_t inStatusCode);

/**
 *  @brief
 *    Callbacks associated with the specified profile identifier for
 *    returning human-readable support strings associated with the
 *    profile.
 *
 *  This structure provides storage for callbacks associated with the
 *  specified profile identifier for returning human-readable support
 *  strings associated with the profile.
 *
 *  The structure may be registered (along with a companion context
 *  structure), looked up once registered, and deregistered (along
 *  with a companion context structure).
 *
 *  To optimize space in constrained applications, this structure
 *  should typically be allocated with constant, static storage
 *  qualifiers (i.e. static const).
 *
 */
struct ProfileStringInfo
{
    uint32_t                       mProfileId;                      //!< The profile identifier under which to register string callbacks.

    MessageNameFunct               mMessageNameFunct;               //!< An optional pointer to a callback to return descriptive names associated with profile message types.
    ProfileNameFunct               mProfileNameFunct;               //!< An optional pointer to a callback to return a descriptive name associated with the profile.
    StatusReportFormatStringFunct  mStatusReportFormatStringFunct;  //!< An optional pointer to a callback to return a descriptive string for profile status codes.
};

/**
 *  @brief
 *    Context for registering and deregistering callbacks associated
 *    with the specified profile identifier for returning
 *    human-readable support strings associated with the profile.
 *
 *  This context will be modified, via mNext, within a registry and,
 *  consequently, must be allocated with read-write storage
 *  qualifiers.
 *
 */
struct ProfileStringContext
{
    ProfileStringContext *         mNext;        //!< A pointer to the next
                                                 //!< context in the registry.

    const ProfileStringInfo &      mStringInfo;  //!< A read-only reference to
                                                 //!< the profile string
                                                 //!< support callbacks
                                                 //!< associated with this
                                                 //!< context.
};

extern WEAVE_ERROR RegisterProfileStringInfo(ProfileStringContext &inOutContext);
extern WEAVE_ERROR UnregisterProfileStringInfo(ProfileStringContext &inOutContext);
extern const ProfileStringInfo *FindProfileStringInfo(uint32_t inProfileId);

}; // namespace Support

}; // namespace Weave

}; // namespace nl

#endif // PROFILESUPPORT_HPP

/*
 *
 *    Copyright (c) 2020 Google, Inc.
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
 *      This file defines platform-specific profile utility functions.
 *
 */

#ifndef PROFILEUTILS_H_
#define PROFILEUTILS_H_

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/NLDLLUtil.h>

namespace nl {
namespace Weave {
namespace Platform {

/**
 * @brief
 *   Reads in a profile type and stores it to be compared for silencing.
 */
void SilenceProfilePrints(nl::Weave::Profiles::WeaveProfileId aProfileId);

/**
 * @brief
 *   Returns if the profile has been silenced.
 */
bool IsProfileSilenced(nl::Weave::Profiles::WeaveProfileId aProfileId);

/**
 * @brief
 *   Clears a profile type ( unsilences the profile type)
 */
NL_DLL_EXPORT void UnSilenceProfile(nl::Weave::Profiles::WeaveProfileId);

} // namespace Platform
} // namespace Weave
} // namespace nl

#endif /* PROFILEUTILS_H_ */

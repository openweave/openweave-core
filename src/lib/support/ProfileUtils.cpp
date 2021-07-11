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
 *      This file implements default platform-specific profile utility functions.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <Weave/Core/WeaveConfig.h>

#include "ProfileUtils.h"

namespace nl {
namespace Weave {
namespace Platform {

uint64_t local_common_profile;
uint64_t local_nest_profile;

void SilenceProfilePrints(nl::Weave::Profiles::WeaveProfileId aProfileId)
{
    if((0x235A0000 & aProfileId) != 0)
    {
        uint64_t interm_val = (0x0000FFFF & aProfileId);
        local_nest_profile |= (1 << interm_val);
    }
    else
    {
        local_common_profile |= (1 << aProfileId);
    }
}
bool IsProfileSilenced(nl::Weave::Profiles::WeaveProfileId aProfileId)
{
    bool retVal = false;
    if((0x235A0000 & aProfileId) != 0)
    {
        uint64_t interm_val = (0x0000FFFF & aProfileId);
        if((local_nest_profile &  (1 << interm_val)) != 0)
        {
            retVal = true;
        }
    }
    else
    {
        if((local_common_profile & (1 << aProfileId)) != 0)
        {
            retVal = true;
        }
    }
    return retVal;
}

void UnSilenceProfile(nl::Weave::Profiles::WeaveProfileId aProfileId)
{
    if((0x235A0000 & aProfileId) != 0)
    {
        uint64_t interm_val = (0x0000FFFF & aProfileId);
        local_nest_profile &= (~(1 << interm_val));
    }
    else
    {
        local_common_profile &= (~(1 << aProfileId));
    }

}

} // namespace Platform
} // namespace Weave
} // namespace nl

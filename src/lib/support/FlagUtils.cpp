/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements functions for manipulating Boolean flags in
 *      a bitfield.
 *
 */

#include <errno.h>
#include <stdint.h>

#include "FlagUtils.hpp"

namespace nl {

static inline bool _GetFlag(const uint8_t inFlags, const uint8_t inFlag)
{
    return ((inFlags & inFlag) == inFlag);
}

bool GetFlag(const uint8_t inFlags, const uint8_t inFlag)
{
    return _GetFlag(inFlags, inFlag);
}

void ClearFlag(uint8_t &outFlags, const uint8_t inFlag)
{
    SetFlag(outFlags, inFlag, false);
}

void SetFlag(uint8_t &outFlags, const uint8_t inFlag, const bool inValue)
{
    const bool setValue = _GetFlag(outFlags, inFlag);

    if (setValue != inValue)
    {
        if (!setValue)
            outFlags |= inFlag;
        else
            outFlags &= ~inFlag;
    }
}

}; // namespace nl

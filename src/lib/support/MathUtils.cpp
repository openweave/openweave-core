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
 *      This file implements default platform-specific math utility functions.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <Weave/Core/WeaveConfig.h>

#include "MathUtils.h"

namespace nl {
namespace Weave {
namespace Platform {

#if !(WEAVE_CONFIG_WILL_OVERRIDE_PLATFORM_MATH_FUNCS)
int64_t Divide(int64_t inDividend, int64_t inDivisor)
{
    // let the compiler and standard/intrinsic library handle this
    return inDividend / inDivisor;
}

uint32_t DivideBy1000(uint64_t inDividend)
{
    return inDividend / 1000;
}
#endif // ! WEAVE_CONFIG_WILL_OVERRIDE_PLATFORM_MATH_FUN

} // namespace Platform
} // namespace Weave
} // namespace nl

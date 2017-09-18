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
 *      This file defines platform-specific math utility functions.
 *
 */

#ifndef MATHUTILS_H_
#define MATHUTILS_H_

#include <Weave/Support/NLDLLUtil.h>

namespace nl {
namespace Weave {
namespace Platform {

/**
 * @brief
 *   Performs signed 64-bit divided by 64-bit operation and returns the quotient.
 *
 * @param[in] inDividend    The dividend in this function.
 *
 * @param[in] inDivisor     The divisor in this function.
 *
 * @note
 *   This is the same as the / operator in C++, for int64_t, which is needed since some platforms
 *   might lack intrinsic support for 64-bit division.
 *
 * @return The quotient of inDividend divided by inDivisor
 */
NL_DLL_EXPORT int64_t Divide(int64_t inDividend, int64_t inDivisor);

/**
 * @brief
 *   Divide a 64-bit unsigned dividend by 1000 producing a 32-bit value.
 *
 * @param[in] inDividend 64-bit value to be divided
 *
 * @result the 32-bit quotient.
 */
NL_DLL_EXPORT uint32_t DivideBy1000(uint64_t inDividend);

} // namespace Platform
} // namespace Weave
} // namespace nl

#endif /* MATHUTILS_H_ */

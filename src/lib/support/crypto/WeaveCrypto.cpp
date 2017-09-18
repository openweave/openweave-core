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
 *      This file implements general purpose cryptographic functions for the Weave layer.
 *
 */

#include <string.h>

#include "WeaveCrypto.h"

namespace nl {
namespace Weave {
namespace Crypto {

/**
 * Compares the first #len bytes of memory area #buf1 and memory area #buf2.
 *
 * The time taken by this function is independent of the data in memory areas #buf1 and #buf2.
 *
 * @param[in]  buf1   Pointer to a memory block.
 *
 * @param[in]  buf2   Pointer to a memory block.
 *
 * @param[in]  len    Size of memory area to compare in bytes.
 *
 * @retval  true      if first #len bytes of memory area #buf1 and #buf2 are equal.
 * @retval  false     otherwise.
 *
 */
bool ConstantTimeCompare(const uint8_t *buf1, const uint8_t *buf2, uint16_t len)
{
    uint8_t c = 0;
    for (uint16_t i = 0; i < len; i++)
        c |= buf1[i] ^ buf2[i];
    return c == 0;
}

/**
 * Clears the first #len bytes of memory area #buf.
 *
 * In this implementation first #len bytes of memory area #buf are filled with zeros.
 *
 * @param[in]  buf     Pointer to a memory buffer holding secret data that should be cleared.
 *
 * @param[in]  len     Specifies secret data size in bytes.
 *
 */
NL_DLL_EXPORT void ClearSecretData(uint8_t *buf, uint32_t len)
{
    memset(buf, 0, len);
}

} // namespace Crypto
} // namespace Weave
} // namespace nl

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
 *      This file implements SHA1 and SHA256 hash functions for the Weave layer.
 *      The implementation is based on the MinCrypt Android library functions.
 *      This implementation is used when #WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT
 *      is enabled (1).
 *
 */

#include <string.h>

#include "WeaveCrypto.h"
#include "HashAlgos.h"

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT

SHA1::SHA1()
{
}

SHA1::~SHA1()
{
}

void SHA1::Begin()
{
    SHA_init(&mSHACtx);
}

void SHA1::AddData(const uint8_t *data, uint16_t dataLen)
{
    SHA_update(&mSHACtx, data, dataLen);
}

void SHA1::Finish(uint8_t *hashBuf)
{
    const uint8_t *hashResult = SHA_final(&mSHACtx);
    memcpy(hashBuf, hashResult, kHashLength);
}

void SHA1::Reset()
{
    memset(this, 0, sizeof(*this));
}

SHA256::SHA256()
{
}

SHA256::~SHA256()
{
}

void SHA256::Begin()
{
    SHA256_init(&mSHACtx);
}

void SHA256::AddData(const uint8_t *data, uint16_t dataLen)
{
    SHA256_update(&mSHACtx, data, dataLen);
}

void SHA256::Finish(uint8_t *hashBuf)
{
    const uint8_t *hashResult = SHA256_final(&mSHACtx);
    memcpy(hashBuf, hashResult, kHashLength);
}

void SHA256::Reset()
{
    memset(this, 0, sizeof(*this));
}

#endif // WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT

} /* namespace Security */
} /* namespace Platform */
} /* namespace Weave */
} /* namespace nl */

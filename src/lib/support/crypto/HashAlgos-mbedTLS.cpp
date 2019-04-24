/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements SHA1 and SHA256 hash functions for OpenWeave
 *      based on the mbedTLS library functions.
 *
 *      This implementation is used when #WEAVE_CONFIG_HASH_IMPLEMENTATION_MBEDTLS
 *      is enabled (1).
 *
 */

#include <string.h>

#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_MBEDTLS

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

SHA1::SHA1()
{
    mbedtls_sha1_init(&mSHACtx);
}

SHA1::~SHA1()
{
    mbedtls_sha1_free(&mSHACtx);
}

void SHA1::Begin()
{
    int res = mbedtls_sha1_starts_ret(&mSHACtx);
    VerifyOrDie(res == 0);
}

void SHA1::AddData(const uint8_t * data, uint16_t dataLen)
{
    int res = mbedtls_sha1_update_ret(&mSHACtx, data, (size_t)dataLen);
    VerifyOrDie(res == 0);
}

void SHA1::Finish(uint8_t *hashBuf)
{
    int res = mbedtls_sha1_finish_ret(&mSHACtx, hashBuf);
    VerifyOrDie(res == 0);
}

void SHA1::Reset()
{
    mbedtls_sha1_init(&mSHACtx);
}

SHA256::SHA256()
{
    mbedtls_sha256_init(&mSHACtx);
}

SHA256::~SHA256()
{
    mbedtls_sha256_free(&mSHACtx);
}

void SHA256::Begin()
{
    int res = mbedtls_sha256_starts_ret(&mSHACtx, 0);
    VerifyOrDie(res == 0);
}

void SHA256::AddData(const uint8_t * data, uint16_t dataLen)
{
    int res = mbedtls_sha256_update_ret(&mSHACtx, data, (size_t)dataLen);
    VerifyOrDie(res == 0);
}

void SHA256::Finish(uint8_t * hashBuf)
{
    int res = mbedtls_sha256_finish_ret(&mSHACtx, hashBuf);
    VerifyOrDie(res == 0);
}

void SHA256::Reset()
{
    mbedtls_sha256_init(&mSHACtx);
}

} /* namespace Security */
} /* namespace Platform */
} /* namespace Weave */
} /* namespace nl */

#endif // WEAVE_CONFIG_HASH_IMPLEMENTATION_MBEDTLS

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
 *      This file provides SHA1 and SHA256 hash functions for the Weave layer.
 *      Functions in this file are platform specific and their various custom
 *      implementations can be enabled.
 *
 *      Platforms that wish to provide their own implementation of hash
 *      functions should assert #WEAVE_CONFIG_HASH_IMPLEMENTATION_PLATFORM
 *      and create platform specific WeaveProjectHashAlgos.h header. Platforms
 *      that support hardware acceleration of hash algorithms are likely to
 *      use this option. The package configuration tool should specify
 *      --with-weave-project-includes=DIR where DIR is the directory that
 *      contains the header.
 *
 *    @note WeaveProjectHashAlgos.h should include declarations of SHA_CTX_PLATFORM
 *          and SHA256_CTX_PLATFORM context structures.
 *
 */

#ifndef HashAlgos_H_
#define HashAlgos_H_

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include "WeaveCrypto.h"

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL && !WEAVE_WITH_OPENSSL
#error "INVALID WEAVE CONFIG: OpenSSL hash implementation enabled but OpenSSL not available (WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL == 1 && WEAVE_WITH_OPENSSL == 0)."
#endif

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL
#include <openssl/sha.h>
#endif

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT
// Map mincrypt's SHA_CTX/SHA256_CTX types to unique names so that they can co-exist with OpenSSL's similarly named types.
// This allows the use of mincrypt's SHA implementations while still using OpenSSL for other crypto functions.
#define SHA_CTX MINCRYPT_SHA_CTX
#define SHA256_CTX MINCRYPT_SHA256_CTX
#include "mincrypt/sha.h"
#include "mincrypt/sha256.h"
#undef SHA_CTX
#undef SHA256_CTX
#endif

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_PLATFORM
#include "WeaveProjectHashAlgos.h"
#endif

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

class NL_DLL_EXPORT SHA1
{
public:
    enum
    {
        kHashLength             = 20,
        kBlockLength            = 64
    };

    SHA1(void);
    ~SHA1(void);

    void Begin(void);
    void AddData(const uint8_t *data, uint16_t dataLen);
#if WEAVE_WITH_OPENSSL
    void AddData(const BIGNUM& num);
#endif
    void Finish(uint8_t *hashBuf);
    void Reset(void);

private:
#if WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL
    SHA_CTX mSHACtx;
#elif WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT
    MINCRYPT_SHA_CTX mSHACtx;
#elif WEAVE_CONFIG_HASH_IMPLEMENTATION_PLATFORM
    SHA_CTX_PLATFORM mSHACtx;
#endif
};

class SHA256
{
public:
    enum
    {
        kHashLength             = 32,
        kBlockLength            = 64
    };

    SHA256(void);
    ~SHA256(void);

    void Begin(void);
    void AddData(const uint8_t *data, uint16_t dataLen);
#if WEAVE_WITH_OPENSSL
    void AddData(const BIGNUM& num);
#endif
    void Finish(uint8_t *hashBuf);
    void Reset(void);

private:
#if WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL
    SHA256_CTX mSHACtx;
#elif WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT
    MINCRYPT_SHA256_CTX mSHACtx;
#elif WEAVE_CONFIG_HASH_IMPLEMENTATION_PLATFORM
    SHA256_CTX_PLATFORM mSHACtx;
#endif
};

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif /* HashAlgos_H_ */

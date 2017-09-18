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
 *      This file provides AES block cipher functions for the Weave layer.
 *      Functions in this file are platform specific and their various custom
 *      implementations can be enabled.
 *
 */

#ifndef AES_H_
#define AES_H_

#include <limits.h>

#include "WeaveCrypto.h"

#if WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL && !WEAVE_WITH_OPENSSL
#error "INVALID WEAVE CONFIG: OpenSSL AES implementation enabled but OpenSSL not available (WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL == 1 && WEAVE_WITH_OPENSSL == 0)."
#endif

#if WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL
#include <openssl/aes.h>
#endif

#if WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI
#include <wmmintrin.h>
#endif

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

class AES128BlockCipher
{
public:
    enum
    {
        kKeyLength      = 16,
        kKeyLengthBits  = kKeyLength * CHAR_BIT,
        kBlockLength    = 16,
        kRoundCount     = 10
    };

    void Reset(void);

protected:
    AES128BlockCipher(void);
    ~AES128BlockCipher(void);

#if WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL
    AES_KEY mKey;
#elif WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI
    __m128i mKey[kRoundCount + 1];
#elif WEAVE_CONFIG_AES_USE_EXPANDED_KEY
    uint8_t mKey[kBlockLength * (kRoundCount + 1)];
#else
    uint8_t mKey[kKeyLength];
#endif
};

class NL_DLL_EXPORT AES128BlockCipherEnc : public AES128BlockCipher
{
public:
    void SetKey(const uint8_t *key);
    void EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock);
};

class NL_DLL_EXPORT AES128BlockCipherDec : public AES128BlockCipher
{
public:
    void SetKey(const uint8_t *key);
    void DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock);
};

class AES256BlockCipher
{
public:
    enum
    {
        kKeyLength      = 32,
        kKeyLengthBits  = kKeyLength * CHAR_BIT,
        kBlockLength    = 16,
        kRoundCount     = 14
    };

    void Reset(void);

protected:
    AES256BlockCipher(void);
    ~AES256BlockCipher(void);

#if WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL
    AES_KEY mKey;
#elif WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI
    __m128i mKey[kRoundCount + 1];
#elif WEAVE_CONFIG_AES_USE_EXPANDED_KEY
    uint8_t mKey[kBlockLength * (kRoundCount + 1)];
#else
    uint8_t mKey[kKeyLength];
#endif
};

class NL_DLL_EXPORT AES256BlockCipherEnc : public AES256BlockCipher
{
public:
    void SetKey(const uint8_t *key);
    void EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock);
};

class NL_DLL_EXPORT AES256BlockCipherDec : public AES256BlockCipher
{
public:
    void SetKey(const uint8_t *key);
    void DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock);
};

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif /* AES_H_ */

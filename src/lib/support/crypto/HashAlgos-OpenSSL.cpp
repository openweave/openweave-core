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
 *      The implementation is based on the OpenSSL library functions.
 *      This implementation is used when #WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL
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

#if WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL

SHA1::SHA1()
{
}

SHA1::~SHA1()
{
}

void SHA1::Begin()
{
    SHA1_Init(&mSHACtx);
}

void SHA1::AddData(const uint8_t *data, uint16_t dataLen)
{
    SHA1_Update(&mSHACtx, data, dataLen);
}

void SHA1::Finish(uint8_t *hashBuf)
{
    SHA1_Final(hashBuf, &mSHACtx);
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
    SHA256_Init(&mSHACtx);
}

void SHA256::AddData(const uint8_t *data, uint16_t dataLen)
{
    SHA256_Update(&mSHACtx, data, dataLen);
}

void SHA256::Finish(uint8_t *hashBuf)
{
    SHA256_Final(hashBuf, &mSHACtx);
}

void SHA256::Reset()
{
    memset(this, 0, sizeof(*this));
}

#endif // WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL

#if WEAVE_WITH_OPENSSL

/**
 * Add an OpenSSL BIGNUM value to the hash.
 *
 * @note
 *   The input value to the hash for BIGNUMs consists of a single sign byte
 *   (00 for positive, FF for negative) followed by the absolute value of the
 *   number, encoded big endian, in the minimum number of bytes.
 *
 */
void SHA1::AddData(const BIGNUM& num)
{
    if (BN_is_zero(&num))
    {
        uint8_t z[2];
        z[0] = 0;
        z[1] = 0;
        AddData(z, 2);
    }
    else
    {
        int bnSize = BN_num_bytes(&num);
        uint8_t *encodedNum = (uint8_t *)OPENSSL_malloc(bnSize + 1);

        encodedNum[0] = (BN_is_negative(&num)) ? 0xFF : 0x00;

        BN_bn2bin(&num, encodedNum + 1);

        AddData(encodedNum, bnSize + 1);

        OPENSSL_free(encodedNum);
    }
}

/**
 * Add an OpenSSL BIGNUM value to the hash.
 *
 * @note
 *   The input value to the hash for BIGNUMs consists of a single sign byte
 *   (00 for positive, FF for negative) followed by the absolute value of the
 *   number, encoded big endian, in the minimum number of bytes.
 *
 */
void SHA256::AddData(const BIGNUM& num)
{
    if (BN_is_zero(&num))
    {
        uint8_t z[2];
        z[0] = 0;
        z[1] = 0;
        AddData(z, 2);
    }
    else
    {
        int bnSize = BN_num_bytes(&num);
        uint8_t *encodedNum = (uint8_t *)OPENSSL_malloc(bnSize + 1);

        encodedNum[0] = (BN_is_negative(&num)) ? 0xFF : 0x00;

        BN_bn2bin(&num, encodedNum + 1);

        AddData(encodedNum, bnSize + 1);

        OPENSSL_free(encodedNum);
    }
}

#endif // WEAVE_WITH_OPENSSL

} /* namespace Security */
} /* namespace Platform */
} /* namespace Weave */
} /* namespace nl */

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
 *      This file implements a template object for doing keyed-hash
 *      message authentication code (HMAC) and specialized objects
 *      for HMAC with SHA-1 and SHA-256.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <string.h>

#include "WeaveCrypto.h"
#include "HMAC.h"

namespace nl {
namespace Weave {
namespace Crypto {

template <class H>
HMAC<H>::HMAC()
{
    Reset();
}

template <class H>
HMAC<H>::~HMAC()
{
    Reset();
}

template <class H>
void HMAC<H>::Begin(const uint8_t *key, uint16_t keyLen)
{
    uint8_t pad[kBlockLength];

    Reset();

    // Copy the key. If the key is larger than a block, hash it an use the result as the key.
    if (keyLen > kBlockLength)
    {
        mHash.Begin();
        mHash.AddData(key, keyLen);
        mHash.Finish(mKey);
        mKeyLen = keyLen = kDigestLength;
    }
    else
    {
        memcpy(mKey, key, keyLen);
        mKeyLen = keyLen;
    }

    // Form the pad for the inner hash.
    memcpy(pad, mKey, mKeyLen);
    if (keyLen < kBlockLength)
        memset(pad + mKeyLen, 0, kBlockLength - mKeyLen);
    for (size_t i = 0; i < kBlockLength; i++)
        pad[i] = pad[i] ^ 0x36;

    // Begin generating the inner hash starting with the pad.
    mHash.Begin();
    mHash.AddData(pad, kBlockLength);

    ClearSecretData(pad, sizeof(kBlockLength));
}

template <class H>
void HMAC<H>::AddData(const uint8_t *msgData, uint16_t dataLen)
{
    // Add a chunk of data to the inner hash.
    mHash.AddData(msgData, dataLen);
}

#if WEAVE_WITH_OPENSSL
template <class H>
void HMAC<H>::AddData(const BIGNUM& num)
{
    // Add a chunk of data to the inner hash.
    mHash.AddData(num);
}
#endif

template <class H>
void HMAC<H>::Finish(uint8_t *hashBuf)
{
    uint8_t pad[kBlockLength];
    uint8_t innerHash[kDigestLength];

    // Finalize the inner hash.
    mHash.Finish(innerHash);

    // Form the pad for the outer hash.
    memcpy(pad, mKey, mKeyLen);
    if (mKeyLen < kBlockLength)
        memset(pad + mKeyLen, 0, kBlockLength - mKeyLen);
    for (size_t i = 0; i < kBlockLength; i++)
        pad[i] = pad[i] ^ 0x5c;

    // Generate the outer hash from the pad and the inner hash.
    mHash.Begin();
    mHash.AddData(pad, kBlockLength);
    mHash.AddData(innerHash, kDigestLength);
    mHash.Finish(hashBuf);

    // Clear state.
    Reset();
    ClearSecretData(pad, sizeof(kBlockLength));
    ClearSecretData(innerHash, sizeof(kDigestLength));
}

template <class H>
void HMAC<H>::Reset()
{
    mHash.Reset();
    ClearSecretData(mKey, sizeof(mKey));
    mKeyLen = 0;
}

template class HMAC<Platform::Security::SHA1>;
template class HMAC<Platform::Security::SHA256>;

} // namespace Crypto
} // namespace Weave
} // namespace nl

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
 *      Implementation of HMAC-based Extract-and-Expand Key Derivation Function (RFC-5869)
 *
 */

#include <string.h>

#include "WeaveCrypto.h"
#include "HKDF.h"
#include <Weave/Support/CodeUtils.h>

#if HAVE_NEW
#include <new>
#else
inline void * operator new     (size_t, void * p) throw() { return p; }
inline void * operator new[]   (size_t, void * p) throw() { return p; }
#endif

namespace nl {
namespace Weave {
namespace Crypto {

template <class H>
HKDF<H>::HKDF()
{
    memset(PseudoRandomKey, 0, kPseudoRandomKeyLength);
}

template <class H>
HKDF<H>::~HKDF()
{
    Reset();
}

template <class H>
void HKDF<H>::BeginExtractKey(const uint8_t *salt, uint16_t saltLen)
{
    mHMAC.Begin(salt, saltLen);
}

template <class H>
void HKDF<H>::AddKeyMaterial(const uint8_t *keyData, uint16_t keyDataLen)
{
    mHMAC.AddData(keyData, keyDataLen);
}

#if WEAVE_WITH_OPENSSL
template <class H>
void HKDF<H>::AddKeyMaterial(const BIGNUM& num)
{
    mHMAC.AddData(num);
}
#endif

template <class H>
WEAVE_ERROR HKDF<H>::FinishExtractKey()
{
    mHMAC.Finish(PseudoRandomKey);
    return WEAVE_NO_ERROR;
}

template <class H>
WEAVE_ERROR HKDF<H>::ExpandKey(const uint8_t *info, uint16_t infoLen, uint16_t keyLen, uint8_t *outKey)
{
    uint8_t hashNum = 1;

    if (keyLen < 1 || keyLen > 255 * H::kHashLength)
        return WEAVE_ERROR_INVALID_ARGUMENT;

    while (true)
    {
        mHMAC.Reset();

        mHMAC.Begin(PseudoRandomKey, kPseudoRandomKeyLength);

        if (hashNum > 1)
            mHMAC.AddData(outKey - H::kHashLength, H::kHashLength);

        if (info != NULL && infoLen > 0)
            mHMAC.AddData(info, infoLen);

        mHMAC.AddData(&hashNum, 1);

        if (keyLen < H::kHashLength)
        {
            uint8_t finalHash[H::kHashLength];
            mHMAC.Finish(finalHash);
            memcpy(outKey, finalHash, keyLen);
            memset(finalHash, 0, sizeof(H::kHashLength));
            break;
        }

        mHMAC.Finish(outKey);
        outKey += H::kHashLength;
        keyLen -= H::kHashLength;
        hashNum++;
    }

    return WEAVE_NO_ERROR;
}

template <class H>
WEAVE_ERROR HKDF<H>::DeriveKey(const uint8_t *salt, uint16_t saltLen,
                               const uint8_t *keyMaterial1, uint16_t keyMaterial1Len,
                               const uint8_t *keyMaterial2, uint16_t keyMaterial2Len,
                               const uint8_t *info, uint16_t infoLen,
                               uint8_t *outKey, uint16_t outKeyBufSize, uint16_t outKeyLen)
{
    WEAVE_ERROR err;
    HKDF<H> hkdf;

    VerifyOrExit(outKeyLen <= outKeyBufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    hkdf.BeginExtractKey(salt, saltLen);
    hkdf.AddKeyMaterial(keyMaterial1, keyMaterial1Len);
    hkdf.AddKeyMaterial(keyMaterial2, keyMaterial2Len);

    err = hkdf.FinishExtractKey();
    SuccessOrExit(err);

    err = hkdf.ExpandKey(info, infoLen, outKeyLen, outKey);
    SuccessOrExit(err);

exit:
    hkdf.Reset();

    return err;
}

template <class H>
void HKDF<H>::Reset()
{
    mHMAC.Reset();
    ClearSecretData(PseudoRandomKey, kPseudoRandomKeyLength);
}

template class HKDF<Platform::Security::SHA1>;

template class HKDF<Platform::Security::SHA256>;


HKDFSHA1Or256::HKDFSHA1Or256(bool useSHA1)
{
    mUseSHA1 = useSHA1;
    if (useSHA1)
        new(ObjBuf()) HKDFSHA1;
    else
        new(ObjBuf()) HKDFSHA256;
}

void HKDFSHA1Or256::BeginExtractKey(const uint8_t* salt, uint16_t saltLen)
{
    if (mUseSHA1)
        HKDFSHA1Obj()->BeginExtractKey(salt, saltLen);
    else
        HKDFSHA256Obj()->BeginExtractKey(salt, saltLen);
}

void HKDFSHA1Or256::AddKeyMaterial(const uint8_t* keyData, uint16_t keyDataLen)
{
    if (mUseSHA1)
        HKDFSHA1Obj()->AddKeyMaterial(keyData, keyDataLen);
    else
        HKDFSHA256Obj()->AddKeyMaterial(keyData, keyDataLen);
}

WEAVE_ERROR HKDFSHA1Or256::FinishExtractKey(void)
{
    if (mUseSHA1)
        return HKDFSHA1Obj()->FinishExtractKey();
    else
        return HKDFSHA256Obj()->FinishExtractKey();
}

WEAVE_ERROR HKDFSHA1Or256::ExpandKey(const uint8_t* info, uint16_t infoLen, uint16_t keyLen, uint8_t* outKey)
{
    if (mUseSHA1)
        return HKDFSHA1Obj()->ExpandKey(info, infoLen, keyLen, outKey);
    else
        return HKDFSHA256Obj()->ExpandKey(info, infoLen, keyLen, outKey);
}

void HKDFSHA1Or256::Reset(void)
{
    if (mUseSHA1)
        HKDFSHA1Obj()->Reset();
    else
        HKDFSHA256Obj()->Reset();
}


} /* namespace Crypto */
} /* namespace Weave */
} /* namespace nl */

/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements AES block cipher functions for the Weave layer
 *      using Intel AES-NI intrinsics.
 *
 */

#include <string.h>

#include "WeaveCrypto.h"
#include "AESBlockCipher.h"

#if WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

#define SWAP(A, B, TMP) do { TMP = A; A = B; B = TMP; } while (0)
#define SWAP_WITH_OP(A, B, OP, TMP) do { TMP = A; A = OP(B); B = OP(TMP); } while (0)

using namespace nl::Weave::Crypto;

AES128BlockCipher::AES128BlockCipher()
{
    memset(&mKey, 0, sizeof(mKey));
}

AES128BlockCipher::~AES128BlockCipher()
{
    Reset();
}

void AES128BlockCipher::Reset()
{
    ClearSecretData((uint8_t *)&mKey, sizeof(mKey));
}

#define ExpandRoundKey128(KEYS, N, RCON, TMP)                           \
do {                                                                    \
    TMP = _mm_aeskeygenassist_si128(KEYS[N-1], RCON);                   \
    TMP = _mm_shuffle_epi32(TMP, 0xff);                                 \
    KEYS[N] = _mm_xor_si128(KEYS[N-1], _mm_slli_si128(KEYS[N-1], 4));   \
    KEYS[N] = _mm_xor_si128(KEYS[N], _mm_slli_si128(KEYS[N], 4));       \
    KEYS[N] = _mm_xor_si128(KEYS[N], _mm_slli_si128(KEYS[N], 4));       \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);                              \
} while (0)

static void ExpandKey128(const uint8_t *key, __m128i *expandedKey)
{
    __m128i tmp;

    expandedKey[0] = _mm_loadu_si128((__m128i *)key);
    ExpandRoundKey128(expandedKey, 1, 0x01, tmp);
    ExpandRoundKey128(expandedKey, 2, 0x02, tmp);
    ExpandRoundKey128(expandedKey, 3, 0x04, tmp);
    ExpandRoundKey128(expandedKey, 4, 0x08, tmp);
    ExpandRoundKey128(expandedKey, 5, 0x10, tmp);
    ExpandRoundKey128(expandedKey, 6, 0x20, tmp);
    ExpandRoundKey128(expandedKey, 7, 0x40, tmp);
    ExpandRoundKey128(expandedKey, 8, 0x80, tmp);
    ExpandRoundKey128(expandedKey, 9, 0x1b, tmp);
    ExpandRoundKey128(expandedKey, 10, 0x36, tmp);
    ClearSecretData((uint8_t *)&tmp, sizeof(tmp));
}

void AES128BlockCipherEnc::SetKey(const uint8_t *key)
{
    ExpandKey128(key, mKey);
}

void AES128BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    __m128i block;

    block = _mm_loadu_si128((const __m128i *)inBlock);
    block = _mm_xor_si128(block, mKey[0]);
    block = _mm_aesenc_si128(block, mKey[1]);
    block = _mm_aesenc_si128(block, mKey[2]);
    block = _mm_aesenc_si128(block, mKey[3]);
    block = _mm_aesenc_si128(block, mKey[4]);
    block = _mm_aesenc_si128(block, mKey[5]);
    block = _mm_aesenc_si128(block, mKey[6]);
    block = _mm_aesenc_si128(block, mKey[7]);
    block = _mm_aesenc_si128(block, mKey[8]);
    block = _mm_aesenc_si128(block, mKey[9]);
    block = _mm_aesenclast_si128(block, mKey[10]);
    _mm_storeu_si128((__m128i*)outBlock, block);
    ClearSecretData((uint8_t *)&block, sizeof(block));
}

void AES128BlockCipherDec::SetKey(const uint8_t *key)
{
    __m128i tmp;

    ExpandKey128(key, mKey);
    SWAP(mKey[10], mKey[0], tmp);
    SWAP_WITH_OP(mKey[9], mKey[1], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[8], mKey[2], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[7], mKey[3], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[6], mKey[4], _mm_aesimc_si128, tmp);
    mKey[5] = _mm_aesimc_si128(mKey[5]);
    ClearSecretData((uint8_t *)&tmp, sizeof(tmp));
}

void AES128BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    __m128i block;

    block = _mm_loadu_si128((const __m128i *)inBlock);
    block = _mm_xor_si128(block, mKey[0]);
    block = _mm_aesdec_si128(block, mKey[1]);
    block = _mm_aesdec_si128(block, mKey[2]);
    block = _mm_aesdec_si128(block, mKey[3]);
    block = _mm_aesdec_si128(block, mKey[4]);
    block = _mm_aesdec_si128(block, mKey[5]);
    block = _mm_aesdec_si128(block, mKey[6]);
    block = _mm_aesdec_si128(block, mKey[7]);
    block = _mm_aesdec_si128(block, mKey[8]);
    block = _mm_aesdec_si128(block, mKey[9]);
    block = _mm_aesdeclast_si128(block, mKey[10]);
    _mm_storeu_si128((__m128i*)outBlock, block);
    ClearSecretData((uint8_t *)&block, sizeof(block));
}

AES256BlockCipher::AES256BlockCipher()
{
    memset(&mKey, 0, sizeof(mKey));
}

AES256BlockCipher::~AES256BlockCipher()
{
    Reset();
}

void AES256BlockCipher::Reset()
{
    ClearSecretData((uint8_t *)&mKey, sizeof(mKey));
}

#define ExpandEvenRoundKey256(KEYS, N, RCON, TMP)     \
do {                                                  \
    TMP = _mm_slli_si128(KEYS[N-2], 0x4);             \
    KEYS[N] = _mm_xor_si128(KEYS[N-2], TMP);          \
    TMP = _mm_slli_si128(TMP, 0x4);                   \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);            \
    TMP = _mm_slli_si128(TMP, 0x4);                   \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);            \
    TMP = _mm_aeskeygenassist_si128(KEYS[N-1], RCON); \
    TMP = _mm_shuffle_epi32(TMP, 0xff);               \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);            \
} while (0)
                                                      \
#define ExpandOddRoundKey256(KEYS, N, TMP)            \
do {                                                  \
    TMP = _mm_slli_si128(KEYS[N-2], 0x4);             \
    KEYS[N] = _mm_xor_si128(KEYS[N-2], TMP);          \
    TMP = _mm_slli_si128(TMP, 0x4);                   \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);            \
    TMP = _mm_slli_si128(TMP, 0x4);                   \
    KEYS[N] = _mm_xor_si128(KEYS[N], TMP);            \
    TMP = _mm_aeskeygenassist_si128(KEYS[N-1], 0x0);  \
    TMP = _mm_shuffle_epi32(TMP, 0xaa);               \
    KEYS[N] = _mm_xor_si128 (KEYS[N], TMP);           \
} while (0)

static void ExpandKey256(const uint8_t *key, __m128i *expandedKey)
{
    __m128i tmp;

    expandedKey[0] = _mm_loadu_si128((const __m128i *)key);
    expandedKey[1] = _mm_loadu_si128((const __m128i *)(key + 16));
    ExpandEvenRoundKey256(expandedKey, 2, 0x01, tmp);
    ExpandOddRoundKey256(expandedKey, 3, tmp);
    ExpandEvenRoundKey256(expandedKey, 4, 0x02, tmp);
    ExpandOddRoundKey256(expandedKey, 5, tmp);
    ExpandEvenRoundKey256(expandedKey, 6, 0x04, tmp);
    ExpandOddRoundKey256(expandedKey, 7, tmp);
    ExpandEvenRoundKey256(expandedKey, 8, 0x08, tmp);
    ExpandOddRoundKey256(expandedKey, 9, tmp);
    ExpandEvenRoundKey256(expandedKey, 10, 0x10, tmp);
    ExpandOddRoundKey256(expandedKey, 11, tmp);
    ExpandEvenRoundKey256(expandedKey, 12, 0x20, tmp);
    ExpandOddRoundKey256(expandedKey, 13, tmp);
    ExpandEvenRoundKey256(expandedKey, 14, 0x40, tmp);
    ClearSecretData((uint8_t *)&tmp, sizeof(tmp));
}

void AES256BlockCipherEnc::SetKey(const uint8_t *key)
{
    ExpandKey256(key, mKey);
}

void AES256BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    __m128i block;

    block = _mm_loadu_si128((const __m128i *)inBlock);
    block = _mm_xor_si128(block, mKey[0]);
    block = _mm_aesenc_si128(block, mKey[1]);
    block = _mm_aesenc_si128(block, mKey[2]);
    block = _mm_aesenc_si128(block, mKey[3]);
    block = _mm_aesenc_si128(block, mKey[4]);
    block = _mm_aesenc_si128(block, mKey[5]);
    block = _mm_aesenc_si128(block, mKey[6]);
    block = _mm_aesenc_si128(block, mKey[7]);
    block = _mm_aesenc_si128(block, mKey[8]);
    block = _mm_aesenc_si128(block, mKey[9]);
    block = _mm_aesenc_si128(block, mKey[10]);
    block = _mm_aesenc_si128(block, mKey[11]);
    block = _mm_aesenc_si128(block, mKey[12]);
    block = _mm_aesenc_si128(block, mKey[13]);
    block = _mm_aesenclast_si128(block, mKey[14]);
    _mm_storeu_si128((__m128i*)outBlock, block);
    ClearSecretData((uint8_t *)&block, sizeof(block));
}

void AES256BlockCipherDec::SetKey(const uint8_t *key)
{
    __m128i tmp;

    ExpandKey256(key, mKey);
    SWAP(mKey[14], mKey[0], tmp);
    SWAP_WITH_OP(mKey[13], mKey[1], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[12], mKey[2], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[11], mKey[3], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[10], mKey[4], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[9], mKey[5], _mm_aesimc_si128, tmp);
    SWAP_WITH_OP(mKey[8], mKey[6], _mm_aesimc_si128, tmp);
    mKey[7] = _mm_aesimc_si128(mKey[7]);
    ClearSecretData((uint8_t *)&tmp, sizeof(tmp));
}

void AES256BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    __m128i block;

    block = _mm_loadu_si128((const __m128i *)inBlock);
    block = _mm_xor_si128(block, mKey[0]);
    block = _mm_aesdec_si128(block, mKey[1]);
    block = _mm_aesdec_si128(block, mKey[2]);
    block = _mm_aesdec_si128(block, mKey[3]);
    block = _mm_aesdec_si128(block, mKey[4]);
    block = _mm_aesdec_si128(block, mKey[5]);
    block = _mm_aesdec_si128(block, mKey[6]);
    block = _mm_aesdec_si128(block, mKey[7]);
    block = _mm_aesdec_si128(block, mKey[8]);
    block = _mm_aesdec_si128(block, mKey[9]);
    block = _mm_aesdec_si128(block, mKey[10]);
    block = _mm_aesdec_si128(block, mKey[11]);
    block = _mm_aesdec_si128(block, mKey[12]);
    block = _mm_aesdec_si128(block, mKey[13]);
    block = _mm_aesdeclast_si128(block, mKey[14]);
    _mm_storeu_si128((__m128i*)outBlock, block);
    ClearSecretData((uint8_t *)&block, sizeof(block));
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI

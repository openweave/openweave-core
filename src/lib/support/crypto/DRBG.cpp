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
 *      This file implements a template object for a Counter Mode
 *      Deterministic Random Bit Generator (CTR-DRBG) and a
 *      specialized object for CTR-DRBG with AES-128 CTR Mode.
 *
 */

#include <string.h>

#include "WeaveCrypto.h"
#include "DRBG.h"
#include "AESBlockCipher.h"
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Crypto {

template <class BlockCipher>
CTR_DRBG<BlockCipher>::CTR_DRBG()
{
    memset(this, 0, sizeof(*this));
}

template <class BlockCipher>
CTR_DRBG<BlockCipher>::~CTR_DRBG()
{
    Uninstantiate();
}

template <class BlockCipher>
WEAVE_ERROR CTR_DRBG<BlockCipher>::Instantiate(EntropyFunct entropyFunct, uint16_t entropyLen,
                                               const uint8_t *personalizationData, uint16_t perDataLen)
{
    WEAVE_ERROR err;
    uint8_t key[kKeyLength] = { 0 };

    // Verify valid entropy function pointer
    VerifyOrExit(entropyFunct != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Initialize entropy function
    mEntropyFunct = entropyFunct;

    // Verify valid entropy length parameter
    VerifyOrExit(entropyLen <= WEAVE_CONFIG_DRBG_MAX_ENTROPY_LENGTH &&
                 entropyLen >= kSecurityStrength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Initialize entropy length
    mEntropyLen = entropyLen;

    // Set reseed counter to zero
    mReseedCounter = 0;

    // Initialize Counter to zero
    memset(mCounter, 0, kBlockLength);

    // Initialize with zero key
    mBlockCipher.SetKey(key);

    // Verify valid entropy length parameter
    VerifyOrExit((entropyLen + perDataLen) >= (kSecurityStrength * 3 / 2),
                                                  err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Reseed
    err = Reseed(personalizationData, perDataLen);

exit:
    if (err != WEAVE_NO_ERROR)
        Uninstantiate();
    return err;
}

template <class BlockCipher>
WEAVE_ERROR CTR_DRBG<BlockCipher>::Reseed(const uint8_t *addData, uint16_t addDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int ret;
    union
    {
        uint8_t entropy[WEAVE_CONFIG_DRBG_MAX_ENTROPY_LENGTH];
        uint8_t seed[kSeedLength];
    } u;

    // Verify that DRBG was instantiated
    VerifyOrExit(mEntropyFunct != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify correct addData input
    if (addDataLen > 0)
        VerifyOrExit(addData != NULL &&
                     addDataLen + mEntropyLen < 0xFFFF, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Gather entropy
    ret = mEntropyFunct(u.entropy, mEntropyLen);
    VerifyOrExit(ret == 0, err = WEAVE_ERROR_DRBG_ENTROPY_SOURCE_FAILED);

    // Use derivation function to generate seed
    DerivationFunction(u.seed, addData, addDataLen, u.entropy, mEntropyLen);

    // DRBG update function
    Update(u.seed);

    // Restart reseed counter
    mReseedCounter = 1;

exit:
    return err;
}

template <class BlockCipher>
WEAVE_ERROR CTR_DRBG<BlockCipher>::GenerateInternal(uint8_t *outData, uint16_t outDataLen,
                                                    const uint8_t *addData, uint16_t addDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t seed[kSeedLength] = { 0 };
    uint8_t encryptedCounter[kBlockLength];
    uint8_t bytesToCopy;

    if (addDataLen > 0)
    {
        // Verify that addData is not NULL
        VerifyOrExit(addData != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Use derivation function to generate seed
        DerivationFunction(seed, addData, addDataLen);

        // DRBG update function
        Update(seed);
    }

    for (uint16_t j = 0; j < outDataLen; j += kBlockLength)
    {
        // Increment counter
        IncrementCounter();

        // Encrypt counter value
        mBlockCipher.EncryptBlock(mCounter, encryptedCounter);

        // Last block can be partial if outDataLen is not multiple of block size
        bytesToCopy = (outDataLen - j >= kBlockLength) ? kBlockLength : outDataLen - j;

        // Copy result
        memcpy(outData + j, encryptedCounter, bytesToCopy);
    }

    // DRBG update function
    Update(seed);

    // Increment reseed counter
    mReseedCounter++;

exit:
    return err;
}

template <class BlockCipher>
WEAVE_ERROR CTR_DRBG<BlockCipher>::Generate(uint8_t *outData, uint16_t outDataLen,
                                            const uint8_t *addData, uint16_t addDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that DRBG was instantiated
    VerifyOrExit(mEntropyFunct != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reseed if needed
    if (mReseedCounter > WEAVE_CONFIG_DRBG_RESEED_INTERVAL)
    {
        err = Reseed(addData, addDataLen);
        SuccessOrExit(err);

        // Clear addDataLen to avoid the same addData use in GenerateInternal()
        addDataLen = 0;
    }

    // Using separate function to decrease total stack utilization
    err = GenerateInternal(outData, outDataLen, addData, addDataLen);
    SuccessOrExit(err);

exit:
    return err;
}

template <class BlockCipher>
void CTR_DRBG<BlockCipher>::Uninstantiate()
{
    mBlockCipher.Reset();
    memset(mCounter, 0, kBlockLength);
    mEntropyFunct = NULL;
}

template <class BlockCipher>
void CTR_DRBG<BlockCipher>::Update(const uint8_t *data)
{
    uint8_t tmp[kRoundedSeedLength];

    for (uint8_t i = 0; i < kSeedLength; i += kBlockLength)
    {
        // Increment counter by one
        IncrementCounter();

        // Encrypt block
        mBlockCipher.EncryptBlock(mCounter, tmp + i);
    }

    // XOR input data with encrypted counter blocks
    for (uint8_t i = 0; i < kSeedLength; i++)
        tmp[i] ^= data[i];

    // Update DRBG State (key and counter)
    mBlockCipher.SetKey(tmp);
    memcpy(mCounter, tmp + kKeyLength, kBlockLength);
}

template <class BlockCipher>
void CTR_DRBG<BlockCipher>::IncrementCounter()
{
    for (uint8_t i = kBlockLength; i > 0; i--)
        if (++mCounter[i-1] != 0)
            return;
}

template <class BlockCipher>
void CTR_DRBG<BlockCipher>::DerivationFunction(uint8_t *seed, const uint8_t *data2, uint16_t data2Len,
                                               const uint8_t *data1, uint16_t data1Len)
{
    BlockCipher blockCipher;
    union
    {
        uint8_t key[kKeyLength];
        uint8_t block[kBlockLength];
    } u;
    uint8_t temp[kRoundedSeedLength];
    uint8_t *x;
    uint16_t dataLen = data2Len + data1Len;
    uint16_t data1Idx;
    uint16_t data2Idx;
    uint8_t blockIdx;
    uint8_t bytesToCopy;

    // Key = leftmost kKeyLength bytes of 0x00010203...1D1E1F
    for (uint8_t j = 0; j < kKeyLength; j++)
        u.key[j] = j;

    // Initialize Key
    blockCipher.SetKey(u.key);

    // Reduce data to kSeedLength bytes of data
    for (uint8_t j = 0; j < kSeedLength; j += kBlockLength)
    {
        // Initialize input data indexes
        data1Idx = 0;
        data2Idx = 0;

        // BCC Function: processes input data to generate one block of data
        for (uint16_t i = 0; i < (kBlockLength + 8 + dataLen + 1); i += kBlockLength)
        {
            // initialize block index to zero
            blockIdx = 0;

            switch (i / kBlockLength)
            {
            case 0:
                // IV = 4 byte Counter (j) zero padded to 16 bytes
                //      Note that j < 256 and only one byte is used
                memset(u.block, 0, kBlockLength);
                u.block[3] = (uint8_t) (j / kBlockLength);
                break;

            case 1:
                //     (4 bytes) |   (4 bytes)   | (dataLen bytes)  | (1 byte)
                // S = <dataLen> | <kSeedLength> | <data1 || data2> |  <0x80>

                // assume that dataLen < 2^16
                u.block[2] ^= (uint8_t) (dataLen >> 8);
                u.block[3] ^= (uint8_t) (dataLen);
                // kSeedLength < 256
                u.block[7] ^= (uint8_t) (kSeedLength);

                // fall through with blockIdx initialized to 8
                blockIdx = 8;

                // fall through
            default:
                for (; blockIdx < kBlockLength; blockIdx++)
                {
                    if (data1Idx < data1Len)
                    {
                        u.block[blockIdx] ^= data1[data1Idx++];
                    }
                    else if (data2Idx < data2Len)
                    {
                        u.block[blockIdx] ^= data2[data2Idx++];
                    }
                    else
                    {
                        u.block[blockIdx] ^= 0x80;
                        break;
                    }
                }
            }

            // Encrypt the next block
            blockCipher.EncryptBlock(u.block, u.block);
        }

        // copy BCC result to temp buffer
        memcpy(temp + j, u.block, kBlockLength);
    }

    // Initialize Key to the kKeyLength leftmost bytes of temp
    blockCipher.SetKey(temp);

    // Initialize X to the next kBlockLength bytes
    x = temp + kKeyLength;

    for (uint8_t j = 0; j < kSeedLength; j += kBlockLength)
    {
        // Encrypt block X
        blockCipher.EncryptBlock(x, x);

        // Last block can be partial only when AES-192 is used
        bytesToCopy = (kSeedLength - j >= kBlockLength) ? kBlockLength : kSeedLength - j;

        // Copy result to destination
        memcpy(seed + j, x, bytesToCopy);
    }
}

template class CTR_DRBG<Platform::Security::AES128BlockCipherEnc>;

} /* namespace Crypto */
} /* namespace Weave */
} /* namespace nl */

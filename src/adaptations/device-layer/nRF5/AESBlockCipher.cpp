/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides implementations for the OpenWeave AES BlockCipher classes
 *          on the Nordic nRF52 platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

#include <string.h>

#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/AESBlockCipher.h>

#include "nrf_crypto_aes.h"

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

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

void AES128BlockCipherEnc::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES128BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    nrf_crypto_aes_context_t ctx;
    size_t outSize;

    nrf_crypto_aes_crypt(&ctx,
                         &g_nrf_crypto_aes_ecb_128_info,
                         NRF_CRYPTO_ENCRYPT,
                         mKey,
                         NULL,
                         (uint8_t *)inBlock,
                         kBlockLength,
                         (uint8_t *)outBlock,
                         &outSize);
}

void AES128BlockCipherDec::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES128BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    nrf_crypto_aes_context_t ctx;
    size_t outSize;

    nrf_crypto_aes_crypt(&ctx,
                         &g_nrf_crypto_aes_ecb_128_info,
                         NRF_CRYPTO_DECRYPT,
                         mKey,
                         NULL,
                         (uint8_t *)inBlock,
                         kBlockLength,
                         (uint8_t *)outBlock,
                         &outSize);
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

void AES256BlockCipherEnc::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES256BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    nrf_crypto_aes_context_t ctx;
    size_t outSize;

    nrf_crypto_aes_crypt(&ctx,
                         &g_nrf_crypto_aes_ecb_256_info,
                         NRF_CRYPTO_ENCRYPT,
                         mKey,
                         NULL,
                         (uint8_t *)inBlock,
                         kBlockLength,
                         (uint8_t *)outBlock,
                         &outSize);
}

void AES256BlockCipherDec::SetKey(const uint8_t *key)
{
    memcpy(mKey, key, kKeyLength);
}

void AES256BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    nrf_crypto_aes_context_t ctx;
    size_t outSize;

    nrf_crypto_aes_crypt(&ctx,
                         &g_nrf_crypto_aes_ecb_256_info,
                         NRF_CRYPTO_DECRYPT,
                         mKey,
                         NULL,
                         (uint8_t *)inBlock,
                         kBlockLength,
                         (uint8_t *)outBlock,
                         &outSize);
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl


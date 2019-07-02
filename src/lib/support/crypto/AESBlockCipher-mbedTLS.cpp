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
 *      This file implements AES block cipher functions for OpenWeave
 *      using mbed TLS APIs.
 *
 */

#include <string.h>

#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/crypto/AESBlockCipher.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_AES_IMPLEMENTATION_MBEDTLS

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

AES128BlockCipher::AES128BlockCipher()
{
    mbedtls_aes_init(&mCtx);
}

AES128BlockCipher::~AES128BlockCipher()
{
    mbedtls_aes_free(&mCtx);
}

void AES128BlockCipher::Reset()
{
    mbedtls_aes_init(&mCtx);
}

void AES128BlockCipherEnc::SetKey(const uint8_t *key)
{
    mbedtls_aes_setkey_enc(&mCtx, key, kKeyLengthBits);
}

void AES128BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    int res = mbedtls_aes_crypt_ecb(&mCtx, MBEDTLS_AES_ENCRYPT, inBlock, outBlock);
    VerifyOrDie(res == 0);
}

void AES128BlockCipherDec::SetKey(const uint8_t *key)
{
    mbedtls_aes_setkey_dec(&mCtx, key, kKeyLengthBits);
}

void AES128BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    int res = mbedtls_aes_crypt_ecb(&mCtx, MBEDTLS_AES_DECRYPT, inBlock, outBlock);
    VerifyOrDie(res == 0);
}

AES256BlockCipher::AES256BlockCipher()
{
    mbedtls_aes_init(&mCtx);
}

AES256BlockCipher::~AES256BlockCipher()
{
    mbedtls_aes_free(&mCtx);
}

void AES256BlockCipher::Reset()
{
    mbedtls_aes_init(&mCtx);
}

void AES256BlockCipherEnc::SetKey(const uint8_t *key)
{
    mbedtls_aes_setkey_enc(&mCtx, key, kKeyLengthBits);
}

void AES256BlockCipherEnc::EncryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    int res = mbedtls_aes_crypt_ecb(&mCtx, MBEDTLS_AES_ENCRYPT, inBlock, outBlock);
    VerifyOrDie(res == 0);
}

void AES256BlockCipherDec::SetKey(const uint8_t *key)
{
    mbedtls_aes_setkey_dec(&mCtx, key, kKeyLengthBits);
}

void AES256BlockCipherDec::DecryptBlock(const uint8_t *inBlock, uint8_t *outBlock)
{
    int res = mbedtls_aes_crypt_ecb(&mCtx, MBEDTLS_AES_DECRYPT, inBlock, outBlock);
    VerifyOrDie(res == 0);
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_AES_IMPLEMENTATION_MBEDTLS

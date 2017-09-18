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
 *      This file provides general purpose cryptographic functions for the Weave layer.
 *
 */

#ifndef WEAVECRYPTO_H_
#define WEAVECRYPTO_H_

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <stddef.h>

#include <Weave/Core/WeaveConfig.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Support/NLDLLUtil.h>

// If one or more of the enabled Weave features depends on OpenSSL but WEAVE_WITH_OPENSSL has
// NOT been defined, then assume we will be using OpenSSL and set WEAVE_WITH_OPENSSL == 1.
#define WEAVE_CONFIG_REQUIRES_OPENSSL (WEAVE_CONFIG_USE_OPENSSL_ECC || WEAVE_CONFIG_RNG_IMPLEMENTATION_OPENSSL || \
                                       WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL || WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL || \
                                       WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 || WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT)
#if WEAVE_CONFIG_REQUIRES_OPENSSL && !defined(WEAVE_WITH_OPENSSL)
#define WEAVE_WITH_OPENSSL 1
#endif

#if WEAVE_WITH_OPENSSL
#include <openssl/crypto.h>
#include <openssl/bn.h>
#ifdef OPENSSL_IS_BORINGSSL
#include <openssl/mem.h>
#endif // OPENSSL_IS_BORINGSSL
#endif // WEAVE_WITH_OPENSSL

/**
 *   @namespace nl::Weave::Crypto
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for shared
 *     cryptographic support.
 */

namespace nl {
namespace Weave {
namespace Crypto {

/**
 * Signature of a function used to gather random data from an entropy source.
 */
typedef int (*EntropyFunct)(uint8_t *buf, size_t bufSize);

extern bool ConstantTimeCompare(const uint8_t *buf1, const uint8_t *buf2, uint16_t len);
extern void ClearSecretData(uint8_t *buf, uint32_t len);

#if WEAVE_WITH_OPENSSL

extern WEAVE_ERROR EncodeBIGNUMValueLE(const BIGNUM& val, uint16_t size, uint8_t *& p);
extern WEAVE_ERROR DecodeBIGNUMValueLE(BIGNUM& val, uint16_t size, const uint8_t *& p);

#endif // WEAVE_WITH_OPENSSL

} // namespace Crypto
} // namespace Weave
} // namespace nl

using namespace nl::Weave::Crypto;

#include "WeaveRNG.h"
#include "AESBlockCipher.h"

#endif /* WEAVECRYPTO_H_ */

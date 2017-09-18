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
 *      This file defines interfaces for encoding and decoding Weave
 *      elliptic curve private keys.
 *
 */

#ifndef WEAVEPRIVATEKEY_H_
#define WEAVEPRIVATEKEY_H_

#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/EllipticCurve.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using nl::Weave::ASN1::OID;
using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;

// Utility functions for encoding/decoding private keys in Weave TLV format.

extern WEAVE_ERROR EncodeWeaveECPrivateKey(uint32_t weaveCurveId,
                                           const EncodedECPublicKey *pubKey,
                                           const EncodedECPrivateKey& privKey,
                                           uint8_t *outBuf, uint32_t outBufSize, uint32_t& outLen);

extern WEAVE_ERROR DecodeWeaveECPrivateKey(const uint8_t *buf, uint32_t len, uint32_t& weaveCurveId,
                                           EncodedECPublicKey& pubKey, EncodedECPrivateKey& privKey);

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVEPRIVATEKEY_H_ */

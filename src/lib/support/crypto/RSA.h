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
 *      This file defines types, objects, and methods for working with
 *      RSA public and private keys and RSA signatures.
 *
 */

#ifndef RSA_H_
#define RSA_H_

#include "WeaveCrypto.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/MathUtils.h>

namespace nl {
namespace Weave {
namespace Crypto {

using nl::Weave::TLV::TLVReader;
using nl::Weave::TLV::TLVWriter;
using nl::Weave::ASN1::OID;

class EncodedRSAKey
{
public:
    enum
    {
        kMaxValueLength = Platform::BitsToByteLength(WEAVE_CONFIG_MAX_RSA_BITS + 1)
    };

    uint8_t *Key;                       // ASN.1 DER Integer value format.
    uint16_t Len;

    bool IsEqual(const EncodedRSAKey& other) const;
};

class EncodedRSASignature
{
public:
    enum
    {
        kMaxValueLength = Platform::BitsToByteLength(WEAVE_CONFIG_MAX_RSA_BITS)
    };

    uint8_t *Sig;                       // ASN.1 DER Integer value format
    uint16_t Len;

    bool IsEqual(const EncodedRSASignature& other) const;

    WEAVE_ERROR ReadSignature(TLVReader& reader, uint64_t tag);
    WEAVE_ERROR WriteSignature(TLVWriter& writer, uint64_t tag) const;
};

inline WEAVE_ERROR EncodedRSASignature::WriteSignature(TLVWriter& writer, uint64_t tag) const
{
    return writer.PutBytes(tag, Sig, Len);
}

// =============================================================
// Primary RSA utility functions used by Weave security code.
// =============================================================

#if WEAVE_WITH_OPENSSL

extern WEAVE_ERROR GenerateAndEncodeWeaveRSASignature(OID sigAlgoOID,
                                                      TLVWriter& writer, uint64_t tag,
                                                      const uint8_t * hash, uint8_t hashLen,
                                                      const uint8_t * keyDER, uint16_t keyDERLen);

extern WEAVE_ERROR VerifyRSASignature(OID sigAlgoOID,
                                      const uint8_t * hash, uint8_t hashLen,
                                      const EncodedRSASignature& sig,
                                      const uint8_t * certDER, uint16_t certDERLen);

#endif // WEAVE_WITH_OPENSSL

} // namespace Crypto
} // namespace Weave
} // namespace nl

#endif /* RSA_H_ */

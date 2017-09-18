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
 *      General elliptic curve utility functions.
 *
 */

#include "WeaveCrypto.h"
#include "EllipticCurve.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Crypto {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::Encoding;

bool EncodedECPublicKey::IsEqual(const EncodedECPublicKey& other) const
{
    return (ECPoint != NULL &&
            other.ECPoint != NULL &&
            ECPointLen == other.ECPointLen &&
            memcmp(ECPoint, other.ECPoint, ECPointLen) == 0);
}

bool EncodedECDSASignature::IsEqual(const EncodedECDSASignature& other) const
{
    return (R != NULL &&
            other.R != NULL &&
            S != NULL &&
            other.S != NULL &&
            RLen == other.RLen &&
            SLen == other.SLen &&
            memcmp(R, other.R, RLen) == 0 &&
            memcmp(S, other.S, SLen) == 0);
}

bool EncodedECPrivateKey::IsEqual(const EncodedECPrivateKey& other) const
{
    return (PrivKey != NULL &&
            other.PrivKey != NULL &&
            PrivKeyLen == other.PrivKeyLen &&
            memcmp(PrivKey, other.PrivKey, PrivKeyLen) == 0);
}

} // namespace Crypto
} // namespace Weave
} // namespace nl

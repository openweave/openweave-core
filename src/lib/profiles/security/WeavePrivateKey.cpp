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
 *      This file implements interfaces for encoding and decoding Weave
 *      elliptic curve private keys.
 *
 */

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/ASN1OID.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Crypto;

// Encode an elliptic curve public/private key pair in Weave TLV format.
NL_DLL_EXPORT WEAVE_ERROR EncodeWeaveECPrivateKey(uint32_t weaveCurveId, const EncodedECPublicKey *pubKey, const EncodedECPrivateKey& privKey,
                                    uint8_t *outBuf, uint32_t outBufSize, uint32_t& outLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType containerType;

    writer.Init(outBuf, outBufSize);

    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_EllipticCurvePrivateKey), kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = writer.Put(ContextTag(kTag_EllipticCurvePrivateKey_CurveIdentifier), weaveCurveId);
    SuccessOrExit(err);

    err = writer.PutBytes(ContextTag(kTag_EllipticCurvePrivateKey_PrivateKey), privKey.PrivKey, privKey.PrivKeyLen);
    SuccessOrExit(err);

    if (pubKey != NULL)
    {
        err = writer.PutBytes(ContextTag(kTag_EllipticCurvePrivateKey_PublicKey), pubKey->ECPoint, pubKey->ECPointLen);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    outLen = writer.GetLengthWritten();

exit:
    return err;
}

// Decode an elliptic curve public/private key pair in Weave TLV format.
NL_DLL_EXPORT WEAVE_ERROR DecodeWeaveECPrivateKey(const uint8_t *buf, uint32_t len, uint32_t& weaveCurveId,
                                    EncodedECPublicKey& pubKey, EncodedECPrivateKey& privKey)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType containerType;

    weaveCurveId = kWeaveCurveId_NotSpecified;
    pubKey.ECPoint = NULL;
    privKey.PrivKey = NULL;

    reader.Init(buf, len);

    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_EllipticCurvePrivateKey));
    SuccessOrExit(err);

    err = reader.EnterContainer(containerType);
    SuccessOrExit(err);

    // TODO: simplify this

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag = reader.GetTag();
        VerifyOrExit(IsContextTag(tag), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        switch (TagNumFromTag(tag))
        {
        case kTag_EllipticCurvePrivateKey_CurveIdentifier:
            VerifyOrExit(weaveCurveId == kOID_NotSpecified, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
            err = reader.Get(weaveCurveId);
            SuccessOrExit(err);
            // Support old form of Nest curve ids that did not include vendor.
            if (weaveCurveId < 65536)
                weaveCurveId |= (kWeaveVendor_NestLabs << 16);
            break;

        case kTag_EllipticCurvePrivateKey_PrivateKey:
            VerifyOrExit(privKey.PrivKey == NULL, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
            err = reader.GetDataPtr((const uint8_t *&)privKey.PrivKey);
            SuccessOrExit(err);
            privKey.PrivKeyLen = reader.GetLength();
            break;

        case kTag_EllipticCurvePrivateKey_PublicKey:
            VerifyOrExit(pubKey.ECPoint == NULL, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
            err = reader.GetDataPtr((const uint8_t *&)pubKey.ECPoint);
            SuccessOrExit(err);
            pubKey.ECPointLen = reader.GetLength();
            break;

        default:
            ExitNow(err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        }
    }

    if (err != WEAVE_END_OF_TLV)
        ExitNow();

    err = reader.ExitContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}


} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

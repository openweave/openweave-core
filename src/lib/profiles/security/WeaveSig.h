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
 *      This file defines interfaces for generating, verifying, and
 *      working with Weave security signatures.
 *
 */

#ifndef WEAVESIG_H_
#define WEAVESIG_H_

#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Profiles/security/WeaveCert.h>


namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

enum {
    kGenerateWeaveSignatureFlag_None                                      = 0,
    kGenerateWeaveSignatureFlag_IncludeSigningCertSubjectDN               = 0x0001,
    kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId                   = 0x0002,
    kGenerateWeaveSignatureFlag_IncludeRelatedCertificates                = 0x0004,
};


/**
 * Provides generic functionality for generating WeaveSignatures.
 *
 * This is an abstract base class that can be used encode WeaveSignature TLV structures.
 * This class provides the common functionality for encoding such signatures but delegates
 * to the subclass to compute and encode the signature data field.
 */
class WeaveSignatureGeneratorBase
{
public:
    enum
    {
        kFlag_None                                          = 0,
        kFlag_IncludeSigningCertSubjectDN                   = 0x0001,
        kFlag_IncludeSigningCertKeyId                       = 0x0002,
        kFlag_IncludeRelatedCertificates                    = 0x0004,
    };

    WeaveCertificateSet & CertSet;
    WeaveCertificateData * SigningCert;
    OID SigAlgoOID;
    uint16_t Flags;

    WEAVE_ERROR GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer);
    WEAVE_ERROR GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, uint8_t * sigBuf, uint16_t sigBufSize, uint16_t & sigLen);
    virtual WEAVE_ERROR GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag);

protected:
    WeaveSignatureGeneratorBase(WeaveCertificateSet & certSet);

    virtual WEAVE_ERROR GenerateSignatureData(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer) = 0;
};

/**
 * Generates a WeaveSignature using an in-memory private key.
 *
 * This is class can be used encode a WeaveSignature TLV structure where the signature data field is computed
 * using a supplied private key.
 */
class WeaveSignatureGenerator : public WeaveSignatureGeneratorBase
{
public:
    const uint8_t * PrivKey;
    uint16_t PrivKeyLen;

    WeaveSignatureGenerator(WeaveCertificateSet & certSet, const uint8_t * privKey, uint16_t privKeyLen);

    WEAVE_ERROR GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag) __OVERRIDE;

    using WeaveSignatureGeneratorBase::GenerateSignature;

private:
    virtual WEAVE_ERROR GenerateSignatureData(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer) __OVERRIDE;
};


inline WeaveSignatureGeneratorBase::WeaveSignatureGeneratorBase(WeaveCertificateSet & certSet)
: CertSet(certSet)
{
    SigningCert = certSet.LastCert();
    SigAlgoOID = nl::Weave::ASN1::kOID_SigAlgo_ECDSAWithSHA256;
    Flags = kFlag_IncludeRelatedCertificates;
}

inline WeaveSignatureGenerator::WeaveSignatureGenerator(WeaveCertificateSet & certSet, const uint8_t * privKey, uint16_t privKeyLen)
: WeaveSignatureGeneratorBase(certSet), PrivKey(privKey), PrivKeyLen(privKeyLen)
{
}


extern WEAVE_ERROR VerifyWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                        const uint8_t *sig, uint16_t sigLen,
                                        WeaveCertificateSet& certSet,
                                        ValidationContext& certValidContext);

extern WEAVE_ERROR VerifyWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                        const uint8_t *sig, uint16_t sigLen, OID expectedSigAlgoOID,
                                        WeaveCertificateSet& certSet,
                                        ValidationContext& certValidContext);

extern WEAVE_ERROR GetWeaveSignatureAlgo(const uint8_t *sig, uint16_t sigLen, OID& sigAlgoOID);

extern WEAVE_ERROR GenerateAndEncodeWeaveECDSASignature(TLVWriter& writer, uint64_t tag,
        const uint8_t * msgHash, uint8_t msgHashLen,
        const uint8_t * signingKey, uint16_t signingKeyLen);

extern WEAVE_ERROR EncodeWeaveECDSASignature(TLVWriter& writer, EncodedECDSASignature& sig, uint64_t tag);
extern WEAVE_ERROR DecodeWeaveECDSASignature(TLVReader& reader, EncodedECDSASignature& sig);
extern WEAVE_ERROR ConvertECDSASignature_DERToWeave(const uint8_t * sigBuf, uint8_t sigLen, TLVWriter& writer, uint64_t tag);
extern WEAVE_ERROR InsertRelatedCertificatesIntoWeaveSignature(
                                        uint8_t *sigBuf, uint16_t sigLen, uint16_t sigBufLen,
                                        const uint8_t *relatedCerts, uint16_t relatedCertsLen,
                                        uint16_t& outSigLen);

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVESIG_H_ */

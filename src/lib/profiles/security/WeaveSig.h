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

extern WEAVE_ERROR GenerateWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                          WeaveCertificateData& signingCert, WeaveCertificateSet& certSet,
                                          const uint8_t *weavePrivKey, uint16_t weavePrivKeyLen,
                                          OID sigAlgoOID, uint16_t flags,
                                          uint8_t *sigBuf, uint16_t sigBufLen, uint16_t& sigLen);

extern WEAVE_ERROR GenerateWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                          WeaveCertificateData& signingCert, WeaveCertificateSet& certSet,
                                          const uint8_t *weavePrivKey, uint16_t weavePrivKeyLen,
                                          OID sigAlgoOID, uint16_t flags,
                                          TLVWriter& writer);

extern WEAVE_ERROR GenerateWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                          WeaveCertificateData& signingCert, WeaveCertificateSet& certSet,
                                          const uint8_t *weavePrivKey, uint16_t weavePrivKeyLen,
                                          uint8_t *sigBuf, uint16_t sigBufLen, uint16_t& sigLen);

extern WEAVE_ERROR VerifyWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                        const uint8_t *sig, uint16_t sigLen,
                                        WeaveCertificateSet& certSet,
                                        ValidationContext& certValidContext);

extern WEAVE_ERROR VerifyWeaveSignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                        const uint8_t *sig, uint16_t sigLen, OID expectedSigAlgoOID,
                                        WeaveCertificateSet& certSet,
                                        ValidationContext& certValidContext);

extern WEAVE_ERROR GetWeaveSignatureAlgo(const uint8_t *sig, uint16_t sigLen, OID& sigAlgoOID);

extern WEAVE_ERROR EncodeWeaveECDSASignature(TLVWriter& writer, EncodedECDSASignature& sig, uint64_t tag);
extern WEAVE_ERROR DecodeWeaveECDSASignature(TLVReader& reader, EncodedECDSASignature& sig);

extern WEAVE_ERROR InsertRelatedCertificatesIntoWeaveSignature(
                                        uint8_t *sigBuf, uint16_t sigLen, uint16_t sigBufLen,
                                        const uint8_t *relatedCerts, uint16_t relatedCertsLen,
                                        uint16_t& outSigLen);

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVESIG_H_ */

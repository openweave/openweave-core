/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Utility functions for interacting with Weave Access Tokens.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

extern WEAVE_ERROR LoadAccessTokenCerts(const uint8_t *accessToken, uint32_t accessTokenLen, WeaveCertificateSet& certSet, uint16_t decodeFlags, WeaveCertificateData *& accessTokenCert);
extern WEAVE_ERROR LoadAccessTokenCerts(TLVReader& reader, WeaveCertificateSet& certSet, uint16_t decodeFlags, WeaveCertificateData *& accessTokenCert);

extern WEAVE_ERROR CASECertInfoFromAccessToken(const uint8_t *accessToken, uint32_t accessTokenLen, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen);
extern WEAVE_ERROR CASECertInfoFromAccessToken(TLVReader& reader, TLVWriter& writer);

extern WEAVE_ERROR ExtractPrivateKeyFromAccessToken(const uint8_t *accessToken, uint32_t accessTokenLen, uint8_t *privKeyBuf, uint16_t privKeyBufSize, uint16_t& privKeyLen);
extern WEAVE_ERROR ExtractPrivateKeyFromAccessToken(TLVReader& reader, TLVWriter& writer);


} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

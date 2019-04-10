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
 *      This file defines data types and objects for modeling and
 *      working with Weave security certificates.
 *
 */

#ifndef WEAVECERT_H_
#define WEAVECERT_H_

#include <string.h>

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/crypto/HashAlgos.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using nl::Weave::TLV::TLVReader;
using nl::Weave::TLV::TLVWriter;
using nl::Weave::ASN1::OID;
using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;
using nl::Weave::Crypto::EncodedECDSASignature;

/** X.509 Certificate Key Purpose Flags
 */
enum
{
    kKeyPurposeFlag_ServerAuth                  = 0x01,
    kKeyPurposeFlag_ClientAuth                  = 0x02,
    kKeyPurposeFlag_CodeSigning                 = 0x04,
    kKeyPurposeFlag_EmailProtection             = 0x08,
    kKeyPurposeFlag_TimeStamping                = 0x10,
    kKeyPurposeFlag_OCSPSigning                 = 0x20,

    kKeyPurposeFlag_Max                         = 0xFF,
};

/** X.509 Certificate Key Usage Flags
 */
enum
{
    kKeyUsageFlag_DigitalSignature              = 0x0001,
    kKeyUsageFlag_NonRepudiation                = 0x0002,
    kKeyUsageFlag_KeyEncipherment               = 0x0004,
    kKeyUsageFlag_DataEncipherment              = 0x0008,
    kKeyUsageFlag_KeyAgreement                  = 0x0010,
    kKeyUsageFlag_KeyCertSign                   = 0x0020,
    kKeyUsageFlag_CRLSign                       = 0x0040,
    kKeyUsageFlag_EncipherOnly                  = 0x0080,
    kKeyUsageFlag_DecipherOnly                  = 0x0100,

    kKeyUsageFlag_Max                           = 0xFFFF,
};

/** Weave Certificate Flags
 *
 * Contains information about a certificate that has been loaded into a WeaveCertSet object.
 */
enum
{
    kCertFlag_ExtPresent_AuthKeyId              = 0x0001,
    kCertFlag_ExtPresent_SubjectKeyId           = 0x0002,
    kCertFlag_ExtPresent_KeyUsage               = 0x0004,
    kCertFlag_ExtPresent_BasicConstraints       = 0x0008,
    kCertFlag_ExtPresent_ExtendedKeyUsage       = 0x0010,
    kCertFlag_AuthKeyIdPresent                  = 0x0020,
    kCertFlag_PathLenConstPresent               = 0x0040,
    kCertFlag_IsCA                              = 0x0080,
    kCertFlag_IsTrusted                         = 0x0100,
    kCertFlag_TBSHashPresent                    = 0x0200,
    kCertFlag_UnsupportedSubjectDN              = 0x0400,
    kCertFlag_UnsupportedIssuerDN               = 0x0800,

    kCertFlag_Max                               = 0xFFFF,
};

/** Weave Certificate Decode Flags
 *
 * Contains information specifying how a certificate should be decoded.
 */
enum
{
    kDecodeFlag_GenerateTBSHash                 = 0x0001,
    kDecodeFlag_IsTrusted                       = 0x0002
};

/** Weave Certificate Validate Flags
 *
 * Contains information specifying how a certificate should be validated.
 */
enum
{
    kValidateFlag_IgnoreNotBefore               = 0x0001,
    kValidateFlag_IgnoreNotAfter                = 0x0002,
    kValidateFlag_RequireSHA256                 = 0x0004,
};

enum
{
    kNullCertTime = 0
};

// WeaveDN -- Represents a Distinguished Name in a Weave certificate.
class WeaveDN
{
public:
    union
    {
        uint64_t WeaveId;
        struct
        {
            const uint8_t *Value;
            uint32_t Len;
        } String;
    } AttrValue;
    OID AttrOID;

    bool IsEqual(const WeaveDN& other) const;
    bool IsEmpty(void) const { return AttrOID == nl::Weave::ASN1::kOID_NotSpecified; }
    void Clear(void) { AttrOID = nl::Weave::ASN1::kOID_NotSpecified; }
};


// CertificateKeyId -- Represents a certificate key identifier.
class CertificateKeyId
{
public:
    const uint8_t *Id;
    uint8_t Len;

    bool IsEqual(const CertificateKeyId& other) const;
    bool IsEmpty(void) const { return Id == NULL; }
    void Clear(void) { Id = NULL; }
};


// WeaveCertificateData -- In-memory representation of data extracted
//   from a Weave certificate.
class WeaveCertificateData
{
public:
    enum
    {
        kMaxTBSHashLen = nl::Weave::Platform::Security::SHA256::kHashLength
    };

    WeaveDN SubjectDN;
    WeaveDN IssuerDN;
    CertificateKeyId SubjectKeyId;
    CertificateKeyId AuthKeyId;
    union
    {
        EncodedECPublicKey EC;
    } PublicKey;
    union
    {
        EncodedECDSASignature EC;
    } Signature;
    uint32_t PubKeyCurveId;
    const uint8_t *EncodedCert;
    uint16_t EncodedCertLen;
    uint16_t CertFlags;
    uint16_t KeyUsageFlags;
    uint16_t PubKeyAlgoOID;
    uint16_t SigAlgoOID;
    uint8_t CertType;
    uint8_t KeyPurposeFlags;
    uint16_t NotBeforeDate;
    uint16_t NotAfterDate;
    uint8_t PathLenConstraint;
    uint8_t TBSHash[kMaxTBSHashLen];
};


// ValidationContext -- Context information used during certification validation.
class ValidationContext
{
public:
    uint32_t EffectiveTime;
    WeaveCertificateData *TrustAnchor;
    WeaveCertificateData *SigningCert;
    uint16_t RequiredKeyUsages;
    uint16_t ValidateFlags;
#if WEAVE_CONFIG_DEBUG_CERT_VALIDATION
    WEAVE_ERROR *CertValidationResults;
    uint8_t CertValidationResultsLen;
#endif
    uint8_t RequiredKeyPurposes;
    uint8_t RequiredCertType;

    void Reset();
};


// WeaveCertificateSet -- Collection of Weave certificate data providing methods for
//   certificate validation and signature verification.
class NL_DLL_EXPORT WeaveCertificateSet
{
public:
    typedef void *(*AllocFunct)(size_t size);
    typedef void (*FreeFunct)(void *p);

    WeaveCertificateSet(void);

    WeaveCertificateData *Certs;                // [READ-ONLY] Pointer to array of certificate data
    uint8_t CertCount;                          // [READ-ONLY] Number of certificates in Certs array
    uint8_t MaxCerts;                           // [READ-ONLY] Length of Certs array

    WEAVE_ERROR Init(uint8_t maxCerts, uint16_t decodeBufSize);
    WEAVE_ERROR Init(uint8_t maxCerts, uint16_t decodeBufSize, AllocFunct allocFunct, FreeFunct freeFunct);
    WEAVE_ERROR Init(WeaveCertificateData *certBuf, uint8_t certBufSize, uint8_t *decodeBuf, uint16_t decodeBufSize);
    void Release(void);
    void Clear(void);

    WEAVE_ERROR LoadCert(const uint8_t *weaveCert, uint32_t weaveCertLen, uint16_t decodeFlags, WeaveCertificateData *& cert);
    WEAVE_ERROR LoadCert(TLVReader& reader, uint16_t decodeFlags, WeaveCertificateData *& cert);
    WEAVE_ERROR LoadCerts(const uint8_t *encodedCerts, uint32_t encodedCertsLen, uint16_t decodeFlags);
    WEAVE_ERROR LoadCerts(TLVReader& reader, uint16_t decodeFlags);
    WEAVE_ERROR AddTrustedKey(uint64_t caId, uint32_t curveId, const EncodedECPublicKey& pubKey,
                              const uint8_t *pubKeyId, uint16_t pubKeyIdLen);
    WEAVE_ERROR SaveCerts(TLVWriter& writer, WeaveCertificateData *firstCert, bool includeTrusted);
    WeaveCertificateData *FindCert(const CertificateKeyId& subjectKeyId) const;
    WeaveCertificateData *LastCert(void) const { return (CertCount > 0) ? &Certs[CertCount-1] : NULL; }

    WEAVE_ERROR ValidateCert(WeaveCertificateData& cert, ValidationContext& context);
    WEAVE_ERROR FindValidCert(const WeaveDN& subjectDN, const CertificateKeyId& subjectKeyId,
            ValidationContext& context, WeaveCertificateData *& cert);

    WEAVE_ERROR GenerateECDSASignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                       WeaveCertificateData& cert,
                                       const EncodedECPrivateKey& privKey,
                                       EncodedECDSASignature& encodedSig);
    WEAVE_ERROR VerifyECDSASignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                     const EncodedECDSASignature& encodedSig,
                                     WeaveCertificateData& cert);

protected:
    AllocFunct mAllocFunct;
    FreeFunct mFreeFunct;
    uint8_t *mDecodeBuf;
    uint16_t mDecodeBufSize;

    WEAVE_ERROR DecodeWeaveCert(TLVReader& reader, uint16_t decodeFlags, WeaveCertificateData& certData);
    WEAVE_ERROR FindValidCert(const WeaveDN& subjectDN, const CertificateKeyId& subjectKeyId,
            ValidationContext& context, uint16_t validateFlags, uint8_t depth, WeaveCertificateData *& cert);
    WEAVE_ERROR ValidateCert(WeaveCertificateData& cert, ValidationContext& context, uint16_t validateFlags, uint8_t depth);
};

extern WEAVE_ERROR DecodeWeaveCert(const uint8_t *weaveCert, uint32_t weaveCertLen, WeaveCertificateData& certData);
extern WEAVE_ERROR DecodeWeaveCert(TLVReader& reader, WeaveCertificateData& certData);

extern WEAVE_ERROR DecodeWeaveDN(TLVReader& reader, WeaveDN& dn);

extern WEAVE_ERROR ConvertX509CertToWeaveCert(const uint8_t *x509Cert, uint32_t x509CertLen, uint8_t *weaveCertBuf, uint32_t weaveCertBufSize, uint32_t& weaveCertLen);
extern WEAVE_ERROR ConvertWeaveCertToX509Cert(const uint8_t *weaveCert, uint32_t weaveCertLen, uint8_t *x509CertBuf, uint32_t x509CertBufSize, uint32_t& x509CertLen);

extern WEAVE_ERROR DetermineCertType(WeaveCertificateData& cert);

extern WEAVE_ERROR PackCertTime(const nl::Weave::ASN1::ASN1UniversalTime& time, uint32_t& packedTime);
extern WEAVE_ERROR UnpackCertTime(uint32_t packedTime, nl::Weave::ASN1::ASN1UniversalTime& time);
extern uint16_t PackedCertTimeToDate(uint32_t packedTime);
extern uint32_t PackedCertDateToTime(uint16_t packedDate);
extern uint32_t SecondsSinceEpochToPackedCertTime(uint32_t secondsSinceEpoch);

inline void ValidationContext::Reset()
{
    memset((void *)this, 0, sizeof(*this));
}

// True if the OID represents a Weave-defined X.509 distinguished named attribute.
inline bool IsWeaveX509Attr(OID oid)
{
    return (oid == nl::Weave::ASN1::kOID_AttributeType_WeaveDeviceId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveServiceEndpointId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveCAId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveSoftwarePublisherId);
}

// True if the OID represents a Weave-defined X.509 distinguished named attribute
// that contains a 64-bit Weave id.
inline bool IsWeaveIdX509Attr(OID oid)
{
    return (oid == nl::Weave::ASN1::kOID_AttributeType_WeaveDeviceId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveServiceEndpointId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveCAId ||
            oid == nl::Weave::ASN1::kOID_AttributeType_WeaveSoftwarePublisherId);
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVECERT_H_ */

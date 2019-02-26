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
 *      This file defines base and common data types and interfaces
 *      for the Weave Security profile.
 *
 */

#ifndef WEAVESECURITY_H_
#define WEAVESECURITY_H_

#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Support/ASN1.h>

/**
 *   @namespace nl::Weave::Profiles::Security
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {


// Message Types for Weave Security Profile
//
enum
{
    // ---- PASE Protocol Messages ----
    kMsgType_PASEInitiatorStep1                 = 1,
    kMsgType_PASEResponderStep1                 = 2,
    kMsgType_PASEResponderStep2                 = 3,
    kMsgType_PASEInitiatorStep2                 = 4,
    kMsgType_PASEResponderKeyConfirm            = 5,
    kMsgType_PASEResponderReconfigure           = 6,

    // ---- CASE Protocol Messages ----
    kMsgType_CASEBeginSessionRequest            = 10,
    kMsgType_CASEBeginSessionResponse           = 11,
    kMsgType_CASEInitiatorKeyConfirm            = 12,
    kMsgType_CASEReconfigure                    = 13,

    // ---- TAKE Protocol Messages ----
    kMsgType_TAKEIdentifyToken                  = 20,
    kMsgType_TAKEIdentifyTokenResponse          = 21,
    kMsgType_TAKETokenReconfigure               = 22,
    kMsgType_TAKEAuthenticateToken              = 23,
    kMsgType_TAKEAuthenticateTokenResponse      = 24,
    kMsgType_TAKEReAuthenticateToken            = 25,
    kMsgType_TAKEReAuthenticateTokenResponse    = 26,

    // ---- Key Extraction Protocol ----
    kMsgType_KeyExportRequest                   = 30,
    kMsgType_KeyExportResponse                  = 31,
    kMsgType_KeyExportReconfigure               = 32,

    // ---- General Messages ----
    kMsgType_EndSession                         = 100,
    kMsgType_KeyError                           = 101,
    kMsgType_MsgCounterSyncResp                 = 102,
};


// Weave Security Status Codes
//
enum
{
    kStatusCode_SessionAborted                  = 1,  // The sender has aborted the session establishment process.
    kStatusCode_PASESupportsOnlyConfig1         = 2,  // PASE supports only Config1.
    kStatusCode_UnsupportedEncryptionType       = 3,  // The requested encryption type is not supported.
    kStatusCode_InvalidKeyId                    = 4,  // An invalid key id was requested.
    kStatusCode_DuplicateKeyId                  = 5,  // The specified key id is already in use.
    kStatusCode_KeyConfirmationFailed           = 6,  // The derived session keys do not agree.
    kStatusCode_InternalError                   = 7,  // The sender encountered an internal error (e.g. no memory, etc...).
    kStatusCode_AuthenticationFailed            = 8,  // The sender rejected the authentication attempt.
    kStatusCode_UnsupportedCASEConfiguration    = 9,  // No common CASE configuration supported.
    kStatusCode_UnsupportedCertificate          = 10, // An unsupported certificate was offered.
    kStatusCode_NoCommonPASEConfigurations      = 11, // No common PASE configuration supported.
    kStatusCode_KeyNotFound                     = 12, // The specified key is not found.
    kStatusCode_WrongEncryptionType             = 13, // The specified encryption type is invalid.
    kStatusCode_UnknownKeyType                  = 14, // The specified key has unknown key type.
    kStatusCode_InvalidUseOfSessionKey          = 15, // The specified key is used incorrectly.
    kStatusCode_InternalKeyError                = 16, // The receiver of the Weave message encountered key error.
    kStatusCode_NoCommonKeyExportConfiguration  = 17, // No common key export protocol configuration supported.
    kStatusCode_UnathorizedKeyExportRequest     = 18, // An unauthorized key export request.
};

// Weave Key Error Message Size
//
enum
{
    kWeaveKeyErrorMessageSize                   = 9,  // The size of the key error message.
};

// Weave Message Counter Synchronization Response Message Size.
//
enum
{
    kWeaveMsgCounterSyncRespMsgSize             = 4,  // The size of the message counter synchronization response message.
};

// Data Element Tags for the Weave Security Profile
//
enum
{
    // ---- Top-level Profile-Specific Tags ----
    kTag_WeaveCertificate                       = 1,    // [ structure ] A Weave certificate.
    kTag_EllipticCurvePrivateKey                = 2,    // [ structure ] An elliptic curve private key.
    kTag_RSAPrivateKey                          = 3,    // [ structure ] An RSA private key.
    kTag_WeaveCertificateList                   = 4,    // [ array ] An array of Weave certificates.
    kTag_WeaveSignature                         = 5,    // [ structure ] A Weave signature object.
    kTag_WeaveCertificateReference              = 6,    // [ structure ] A Weave certificate reference object.
    kTag_WeaveCASECertificateInformation        = 7,    // [ structure ] A Weave CASE certificate information object.
    kTag_WeaveCASESignature                     = 8,    // [ structure ] An Weave CASE signature object.
                                                        //    Presently this has the same internal structure as an ECDSASignature.
    kTag_WeaveAccessToken                       = 9,    // [ structure ] A Weave Access Token object
    kTag_GroupKeySignature                      = 10,   // [ structure ] A Weave group Key signature object

    // ---- Context-specific Tags for WeaveCertificate Structure ----
    kTag_SerialNumber                           = 1,    // [ byte string ] Certificate serial number, in BER integer encoding.
    kTag_SignatureAlgorithm                     = 2,    // [ unsigned int ] Enumerated value identifying the certificate signature algorithm.
    kTag_Issuer                                 = 3,    // [ path ] The issuer distinguished name of the certificate.
    kTag_NotBefore                              = 4,    // [ unsigned int ] Certificate validity period start (certificate date format).
    kTag_NotAfter                               = 5,    // [ unsigned int ] Certificate validity period end (certificate date format).
    kTag_Subject                                = 6,    // [ path ] The subject distinguished name of the certificate.
    kTag_PublicKeyAlgorithm                     = 7,    // [ unsigned int ] Identifies the algorithm with which the public key can be used.
    kTag_EllipticCurveIdentifier                = 8,    // [ unsigned int ] For EC certs, identifies the elliptic curve used.
    kTag_RSAPublicKey                           = 9,    // [ structure ] The RSA public key.
    kTag_EllipticCurvePublicKey                 = 10,   // [ byte string ] The elliptic curve public key, in X9.62 encoded format.
    kTag_RSASignature                           = 11,   // [ byte string ] The RSA signature for the certificate.
    kTag_ECDSASignature                         = 12,   // [ structure ] The ECDSA signature for the certificate.
    // Tags identifying certificate extensions (tag numbers 128 - 255)
    kCertificateExtensionTagsStart              = 128,
    kTag_AuthorityKeyIdentifier                 = 128,  // [ structure ] Information about the public key used to sign the certificate.
    kTag_SubjectKeyIdentifier                   = 129,  // [ structure ] Information about the certificate's public key.
    kTag_KeyUsage                               = 130,  // [ structure ] TODO: document me
    kTag_BasicConstraints                       = 131,  // [ structure ] TODO: document me
    kTag_ExtendedKeyUsage                       = 132,  // [ structure ] TODO: document me
    kCertificateExtensionTagsEnd                = 255,

    // ---- Context-specific Tags for RSAPublicKey Structure ----
    kTag_RSAPublicKey_Modulus                   = 1,    // [ byte string ] RSA public key modulus, in ASN.1 integer encoding.
    kTag_RSAPublicKey_PublicExponent            = 2,    // [ unsigned int ] RSA public key exponent.

    // ---- Context-specific Tags for ECDSASignature Structure ----
    kTag_ECDSASignature_r                       = 1,    // [ byte string ] ECDSA r value, in ASN.1 integer encoding.
    kTag_ECDSASignature_s                       = 2,    // [ byte string ] ECDSA s value, in ASN.1 integer encoding.

    // ---- Context-specific Tags for AuthorityKeyIdentifier Structure ----
    kTag_AuthorityKeyIdentifier_Critical        = 1,    // [ boolean ] True if the AuthorityKeyIdentifier extension is critical. Otherwise absent.
    kTag_AuthorityKeyIdentifier_KeyIdentifier   = 2,    // [ byte string ] TODO: document me
    kTag_AuthorityKeyIdentifier_Issuer          = 3,    // [ path ] TODO: document me
    kTag_AuthorityKeyIdentifier_SerialNumber    = 4,    // [ byte string ] TODO: document me

    // ---- Context-specific Tags for SubjectKeyIdentifier Structure ----
    kTag_SubjectKeyIdentifier_Critical          = 1,    // [ boolean ] True if the SubjectKeyIdentifier extension is critical. Otherwise absent.
    kTag_SubjectKeyIdentifier_KeyIdentifier     = 2,    // [ byte string ] Unique identifier for certificate's public key, per RFC5280.

    // ---- Context-specific Tags for KeyUsage Structure ----
    kTag_KeyUsage_Critical                      = 1,    // [ boolean ] True if the KeyUsage extension is critical. Otherwise absent.
    kTag_KeyUsage_KeyUsage                      = 2,    // [ unsigned int ] Integer containing key usage bits, per to RFC5280.

    // ---- Context-specific Tags for BasicConstraints Structure ----
    kTag_BasicConstraints_Critical              = 1,    // [ boolean ] True if the BasicConstraints extension is critical. Otherwise absent.
    kTag_BasicConstraints_IsCA                  = 2,    // [ boolean ] True if the certificate can be used to verify certificate signatures.
    kTag_BasicConstraints_PathLenConstraint     = 3,    // [ unsigned int ] Maximum number of subordinate intermediate certificates.

    // ---- Context-specific Tags for ExtendedKeyUsage Structure ----
    kTag_ExtendedKeyUsage_Critical              = 1,    // [ boolean ] True if the ExtendedKeyUsage extension is critical. Otherwise absent.
    kTag_ExtendedKeyUsage_KeyPurposes           = 2,    // [ array ] Array of enumerated values giving the purposes for which the public key can be used.

    // ---- Context-specific Tags for EllipticCurvePrivateKey Structure ----
    kTag_EllipticCurvePrivateKey_CurveIdentifier = 1,   // [ unsigned int ] WeaveCurveId identifying the elliptic curve.
    kTag_EllipticCurvePrivateKey_PrivateKey     = 2,    // [ byte string ] Private key encoded using the I2OSP algorithm defined in RFC3447.
    kTag_EllipticCurvePrivateKey_PublicKey      = 3,    // [ byte string ] The elliptic curve public key, in X9.62 encoded format.

    // ---- Context-specific Tags for RSAPrivateKey Structure ----
    // ... TBD ...

    // ---- Context-specific Tags for WeaveSignature Structure ----
    kTag_WeaveSignature_ECDSASignatureData      = 1,    // [ structure ] ECDSA signature data for the signed message.
    kTag_WeaveSignature_RSASignatureData        = 2,    // [ byte string ] RSA signature for the signed message.
                                                        //   Per the schema, exactly one of ECDSASignature or RSASignature must be present.
    kTag_WeaveSignature_SigningCertificateRef   = 3,    // [ structure ] A Weave certificate reference structure identifying the certificate
                                                        //   used to generate the signature. If absent, the signature was generated by the
                                                        //   first certificate in the RelatedCertificates list.
    kTag_WeaveSignature_RelatedCertificates     = 4,    // [ array ] Array of certificates needed to validate the signature.  May be omitted if
                                                        //   validators are expected to have the necessary certificates for validation.
                                                        //   At least one of SigningCertificateRef or RelatedCertificates must be present.
    kTag_WeaveSignature_SignatureAlgorithm      = 5,    // [ unsigned int ] Enumerated value identifying the signature algorithm.
                                                        //   Legal values per the schema are: kOID_SigAlgo_ECDSAWithSHA1, kOID_SigAlgo_ECDSAWithSHA256
                                                        //     and kOID_SigAlgo_SHA1WithRSAEncryption.
                                                        //   For backwards compatibility, this field should be omitted when the signature
                                                        //     algorithm is ECDSAWithSHA1.
                                                        //   When this field is included it must appear first within the WeaveSignature structure.
                                                        //   kOID_SigAlgo_SHA1WithRSAEncryption is not presently supported in the code.

    // ---- Context-specific Tags for Weave Certificate Reference Structure ----
    kTag_WeaveCertificateRef_Subject            = 1,    // [ path ] The subject DN of the referenced certificate.
    kTag_WeaveCertificateRef_PublicKeyId        = 2,    // [ byte string ] Unique identifier for referenced certificate's public key, per RFC5280.

    // ---- Context-specific Tags for Weave CASE Certificate Information Structure ----
    kTag_CASECertificateInfo_EntityCertificate    = 1,  // [ structure ] A Weave certificate object representing the authenticating entity.
    kTag_CASECertificateInfo_EntityCertificateRef = 2,  // [ structure ] A Weave certificate reference object identifying the authenticating entity.
    kTag_CASECertificateInfo_RelatedCertificates  = 3,  // [ path ] A collection of certificates related to the authenticating entity.
    kTag_CASECertificateInfo_TrustAnchors         = 4,  // [ path ] A collection of Weave certificate reference identifying certificates trusted
                                                        //   by the authenticating entity.

    // ---- Context-specific Tags for Weave Access Token Structure ----
    kTag_AccessToken_Certificate                = 1,    // [ structure ] A Weave certificate object representing the entity that is trusted to
                                                        //   access a device or fabric.
    kTag_AccessToken_PrivateKey                 = 2,    // [ structure ] An EllipticCurvePrivateKey object containing the private key associated
                                                        //   with the access token certificate.
    kTag_AccessToken_RelatedCertificates        = 3,    // [ array, optional ] An optional array of certificates related to the access token
                                                        //   certificate that may be needed to validate it.

    kTag_GroupKeySignature_SignatureAlgorithm   = 1,    //  [ unsigned int ] Enumerated value identifying the certificate signature
                                                        //  algorithm.  Legal values are taken from the kOID_SigAlgo_* constant
                                                        //  namespace.  The only value currently supported is
                                                        //  kOID_SigAlgo_HMACWithSHA256.  When the tag is ommitted the signature
                                                        //  algorithm defaults to HMACWithSHA256
    kTag_GroupKeySignature_KeyId                = 2,    //  [ unsigned int ] Weave KeyId to be used to generate and verify the signature
    kTag_GroupKeySignature_Signature            = 3,    //  [ byte string ] Signature bytes themselves.


    // ---- Context-specific Tags for Weave representation of X.509 Distinguished Name Attributes ----
    //
    // The value used here must match *exactly* the OID enum values assigned to the corresponding object ids in the gen-oid-table.py script.
    //
    // WARNING! Assign no values higher than 127.
    //
    kTag_DNAttrType_CommonName                  = 1,    // [ UTF8 string ]
    kTag_DNAttrType_Surname                     = 2,    // [ UTF8 string ]
    kTag_DNAttrType_SerialNumber                = 3,    // [ UTF8 string ]
    kTag_DNAttrType_CountryName                 = 4,    // [ UTF8 string ]
    kTag_DNAttrType_LocalityName                = 5,    // [ UTF8 string ]
    kTag_DNAttrType_StateOrProvinceName         = 6,    // [ UTF8 string ]
    kTag_DNAttrType_OrganizationName            = 7,    // [ UTF8 string ]
    kTag_DNAttrType_OrganizationalUnitName      = 8,    // [ UTF8 string ]
    kTag_DNAttrType_Title                       = 9,    // [ UTF8 string ]
    kTag_DNAttrType_Name                        = 10,   // [ UTF8 string ]
    kTag_DNAttrType_GivenName                   = 11,   // [ UTF8 string ]
    kTag_DNAttrType_Initials                    = 12,   // [ UTF8 string ]
    kTag_DNAttrType_GenerationQualifier         = 13,   // [ UTF8 string ]
    kTag_DNAttrType_DNQualifier                 = 14,   // [ UTF8 string ]
    kTag_DNAttrType_Pseudonym                   = 15,   // [ UTF8 string ]
    kTag_DNAttrType_DomainComponent             = 16,   // [ UTF8 string ]
    kTag_DNAttrType_WeaveDeviceId               = 17,   // [ unsigned int ]
    kTag_DNAttrType_WeaveServiceEndpointId      = 18,   // [ unsigned int ]
    kTag_DNAttrType_WeaveCAId                   = 19,   // [ unsigned int ]
    kTag_DNAttrType_WeaveSoftwarePublisherId    = 20    // [ unsigned int ]
};

// Weave-defined elliptic curve ids
//
// NOTE: The bottom bits of each curve id must match the enum value used in the curve's
// ASN1 OID (see ASN1OID.h).
enum
{
    kWeaveCurveId_NotSpecified                  = 0,

    kWeaveCurveId_secp160r1                     = (kWeaveVendor_NestLabs << 16) | 0x0021,
    kWeaveCurveId_prime192v1                    = (kWeaveVendor_NestLabs << 16) | 0x0015,
    kWeaveCurveId_secp224r1                     = (kWeaveVendor_NestLabs << 16) | 0x0025,
    kWeaveCurveId_prime256v1                    = (kWeaveVendor_NestLabs << 16) | 0x001B,

    kWeaveCurveId_VendorMask                    = 0xFFFF0000,
    kWeaveCurveId_VendorShift                   = 16,
    kWeaveCurveId_CurveNumMask                  = ASN1::kOID_Mask,
};

// Bit-field value represented set of defined elliptic curves.
enum
{
    kWeaveCurveSet_Mask                         = 0xFF,

    kWeaveCurveSet_secp160r1                    = 0x01,
    kWeaveCurveSet_prime192v1                   = 0x02,
    kWeaveCurveSet_secp224r1                    = 0x04,
    kWeaveCurveSet_prime256v1                   = 0x08,

    kWeaveCurveSet_All                          = (kWeaveCurveSet_secp160r1|kWeaveCurveSet_prime192v1|kWeaveCurveSet_secp224r1|kWeaveCurveSet_prime256v1)
};

extern bool IsSupportedCurve(uint32_t curveId);

extern bool IsCurveInSet(uint32_t curveId, uint8_t curveSet);

extern ASN1::OID WeaveCurveIdToOID(uint32_t weaveCurveId);

inline uint32_t OIDToWeaveCurveId(ASN1::OID curveOID)
{
    return (((uint32_t)kWeaveVendor_NestLabs) << kWeaveCurveId_VendorShift) | (kWeaveCurveId_CurveNumMask & curveOID);
}



} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVESECURITY_H_ */

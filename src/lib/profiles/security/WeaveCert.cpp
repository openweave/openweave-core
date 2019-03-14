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
 *      This file implements objects for modeling and working with
 *      Weave security certificates.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Crypto;

extern WEAVE_ERROR DecodeConvertTBSCert(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData);

#if HAVE_MALLOC && HAVE_FREE
static void *DefaultAlloc(size_t size)
{
    return malloc(size);
}

static void DefaultFree(void *p)
{
    free(p);
}
#endif // HAVE_MALLOC && HAVE_FREE

WEAVE_ERROR WeaveCertificateSet::Init(uint8_t maxCerts, uint16_t decodeBufSize)
{
#if HAVE_MALLOC && HAVE_FREE
    return Init(maxCerts, decodeBufSize, DefaultAlloc, DefaultFree);
#else
    return WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif // HAVE_MALLOC && HAVE_FREE
}

WeaveCertificateSet::WeaveCertificateSet()
{
    memset(this, 0, sizeof(*this));
}

WEAVE_ERROR WeaveCertificateSet::Init(uint8_t maxCerts, uint16_t decodeBufSize, AllocFunct allocFunct, FreeFunct freeFunct)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Certs = (WeaveCertificateData *)allocFunct(sizeof(WeaveCertificateData) * maxCerts);
    VerifyOrExit(Certs != NULL, err = WEAVE_ERROR_NO_MEMORY);

    CertCount = 0;
    MaxCerts = maxCerts;

    mDecodeBuf = NULL;
    mDecodeBufSize = decodeBufSize;

    mAllocFunct = allocFunct;
    mFreeFunct = freeFunct;

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::Init(WeaveCertificateData *certsArray, uint8_t certArraySize, uint8_t *decodeBuf, uint16_t decodeBufSize)
{
    Certs = certsArray;
    CertCount = 0;
    MaxCerts = certArraySize;
    mDecodeBuf = decodeBuf;
    mDecodeBufSize = decodeBufSize;
    mAllocFunct = NULL;
    mFreeFunct = NULL;
    return WEAVE_NO_ERROR;
}

void WeaveCertificateSet::Release()
{
    if (mFreeFunct != NULL)
    {
        if (Certs != NULL)
        {
            mFreeFunct(Certs);
            Certs = NULL;
        }
        if (mDecodeBuf != NULL)
        {
            mFreeFunct(mDecodeBuf);
            mDecodeBuf = NULL;
        }
    }
}

void WeaveCertificateSet::Clear()
{
    memset(Certs, 0, sizeof(WeaveCertificateData) * MaxCerts);
    CertCount = 0;
}

WeaveCertificateData *WeaveCertificateSet::FindCert(const CertificateKeyId& subjectKeyId) const
{
    for (uint8_t i = 0; i < CertCount; i++)
    {
        WeaveCertificateData& cert = Certs[i];
        if (cert.SubjectKeyId.IsEqual(subjectKeyId))
            return &cert;
    }
    return NULL;
}

WEAVE_ERROR WeaveCertificateSet::LoadCert(const uint8_t *weaveCert, uint32_t weaveCertLen, uint16_t decodeFlags, WeaveCertificateData *& cert)
{
    WEAVE_ERROR err;
    TLVReader reader;

    reader.Init(weaveCert, weaveCertLen);
    reader.ImplicitProfileId = kWeaveProfile_Security;

    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate));
    SuccessOrExit(err);

    err = LoadCert(reader, decodeFlags, cert);

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::LoadCert(TLVReader& reader, uint16_t decodeFlags, WeaveCertificateData *& cert)
{
    WEAVE_ERROR err;
    ASN1Writer writer;
    uint8_t *decodeBuf = mDecodeBuf;

    cert = NULL;

    // Must be positioned on the structure element representing the certificate.
    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify we have room for the new certificate.
    VerifyOrExit(CertCount < MaxCerts, err = WEAVE_ERROR_NO_MEMORY);

    if (decodeBuf == NULL && mAllocFunct != NULL)
        decodeBuf = (uint8_t *)(mAllocFunct(mDecodeBufSize));
    VerifyOrExit(decodeBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    cert = &Certs[CertCount];
    memset(cert, 0, sizeof(*cert));

    // Record the starting point of the certificate's elements.
    cert->EncodedCert = reader.GetReadPoint();

    {
        TLVType containerType;

        // Enter the certificate structure...
        err = reader.EnterContainer(containerType);
        SuccessOrExit(err);

        // Initialize an ASN1Writer and convert the TBS (to-be-signed) portion of the certificate to ASN.1 DER
        // encoding.  At the same time, parse various components within the certificate and set the corresponding
        // fields in the CertificateData object.
        writer.Init(decodeBuf, mDecodeBufSize);
        err = DecodeConvertTBSCert(reader, writer, *cert);
        SuccessOrExit(err);

        // Verify the cert has both the Subject Key Id and Authority Key Id extensions present.
        // Only certs with both these extensions are supported for the purposes of certificate validation.
        {
            const uint16_t expectedFlags = kCertFlag_ExtPresent_SubjectKeyId | kCertFlag_ExtPresent_AuthKeyId;
            VerifyOrExit((cert->CertFlags & expectedFlags) == expectedFlags, err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);
        }

        // Verify the cert was signed with ECDSA-SHA1 or ECDSA-SHA256. These are the only signature algorithms
        // currently supported.
        VerifyOrExit((cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1 || cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256),
                     err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

        // If requested, generate the hash of the TBS portion of the certificate...
        if ((decodeFlags & kDecodeFlag_GenerateTBSHash) != 0)
        {
            // Finish writing the ASN.1 DER encoding of the TBS certificate.
            err = writer.Finalize();
            SuccessOrExit(err);

            // Generate a SHA hash of the encoded TBS certificate.
            if (cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1)
            {
                Platform::Security::SHA1 sha1;
                sha1.Begin();
                sha1.AddData(decodeBuf, writer.GetLengthWritten());
                sha1.Finish(cert->TBSHash);
            }
            else
            {
                Platform::Security::SHA256 sha256;
                sha256.Begin();
                sha256.AddData(decodeBuf, writer.GetLengthWritten());
                sha256.Finish(cert->TBSHash);
            }

            cert->CertFlags |= kCertFlag_TBSHashPresent;
        }

        // Extract the certificate's signature...
        {
            TLVType containerType2;

            // Verify the tag and type
            VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);
            VerifyOrExit(reader.GetTag() == ContextTag(kTag_ECDSASignature), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            err = reader.EnterContainer(containerType2);
            SuccessOrExit(err);

            // Extract the signature r value
            err = reader.Next(kTLVType_ByteString, ContextTag(kTag_ECDSASignature_r));
            SuccessOrExit(err);
            err = reader.GetDataPtr((const uint8_t *&)cert->Signature.EC.R);
            SuccessOrExit(err);
            cert->Signature.EC.RLen = reader.GetLength();

            // Extract the signature s value
            err = reader.Next(kTLVType_ByteString, ContextTag(kTag_ECDSASignature_s));
            SuccessOrExit(err);
            err = reader.GetDataPtr((const uint8_t *&)cert->Signature.EC.S);
            SuccessOrExit(err);
            cert->Signature.EC.SLen = reader.GetLength();

            err = reader.ExitContainer(containerType2);
            SuccessOrExit(err);
        }

        err = reader.ExitContainer(containerType);
        SuccessOrExit(err);
    }

    // Record the overall size of the certificate.
    cert->EncodedCertLen = reader.GetReadPoint() - cert->EncodedCert;

    CertCount++;

    // If requested by the caller, mark the certificate as trusted.
    if (decodeFlags & kDecodeFlag_IsTrusted)
    {
    	cert->CertFlags |= kCertFlag_IsTrusted;
    }

    // Assign a default type for the certificate based on its subject and attributes.
    err = DetermineCertType(*cert);
    SuccessOrExit(err);

exit:
    if (decodeBuf != NULL && decodeBuf != mDecodeBuf && mFreeFunct != NULL)
        mFreeFunct(decodeBuf);
    return err;
}

WEAVE_ERROR WeaveCertificateSet::LoadCerts(const uint8_t *encodedCerts, uint32_t encodedCertsLen, uint16_t decodeFlags)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType type;
    uint64_t tag;

    reader.Init(encodedCerts, encodedCertsLen);
    reader.ImplicitProfileId = kWeaveProfile_Security;

    err = reader.Next();
    SuccessOrExit(err);

    type = reader.GetType();
    tag = reader.GetTag();

    VerifyOrExit((type == kTLVType_Structure && tag == ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate)) ||
                 (type == kTLVType_Array && tag == ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificateList)),
                 err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = LoadCerts(reader, decodeFlags);

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::LoadCerts(TLVReader& reader, uint16_t decodeFlags)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    // If positioned on a structure, we assume that structure is a single certificate.
    if (reader.GetType() == kTLVType_Structure)
    {
        err = LoadCert(reader, decodeFlags, cert);
        SuccessOrExit(err);
    }

    // Other we expect to be position on an Array or Path that contains a sequence of
    // zero or more certificates...
    else
    {
        TLVType containerType;

        err = reader.EnterContainer(containerType);
        SuccessOrExit(err);

        while ((err = reader.Next()) == WEAVE_NO_ERROR)
        {
            err = LoadCert(reader, decodeFlags, cert);
            SuccessOrExit(err);
        }
        if (err != WEAVE_END_OF_TLV)
            ExitNow();

        err = reader.ExitContainer(containerType);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::AddTrustedKey(uint64_t caId, uint32_t curveId, const EncodedECPublicKey& pubKey,
                                               const uint8_t *pubKeyId, uint16_t pubKeyIdLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateData *cert;

    // Verify we have room for the new certificate.
    VerifyOrExit(CertCount < MaxCerts, err = WEAVE_ERROR_NO_MEMORY);

    cert = &Certs[CertCount];
    memset(cert, 0, sizeof(*cert));
    cert->SubjectDN.AttrOID = kOID_AttributeType_WeaveCAId;
    cert->SubjectDN.AttrValue.WeaveId = caId;
    cert->IssuerDN = cert->SubjectDN;
    cert->PubKeyCurveId = curveId;
    cert->PublicKey.EC = pubKey;
    cert->SubjectKeyId.Id = cert->AuthKeyId.Id = pubKeyId;
    cert->SubjectKeyId.Len = cert->AuthKeyId.Len = pubKeyIdLen;
    cert->KeyUsageFlags = kKeyUsageFlag_KeyCertSign;
    cert->CertFlags = kCertFlag_AuthKeyIdPresent | kCertFlag_ExtPresent_AuthKeyId |
            kCertFlag_ExtPresent_BasicConstraints | kCertFlag_ExtPresent_SubjectKeyId |
            kCertFlag_ExtPresent_KeyUsage | kCertFlag_IsCA | kCertFlag_IsTrusted;
    cert->CertType = kCertType_CA;

    CertCount++;

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::SaveCerts(TLVWriter& writer, WeaveCertificateData *firstCert, bool includeTrusted)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (firstCert != NULL)
    {
        err = writer.PutPreEncodedContainer(AnonymousTag, kTLVType_Structure, firstCert->EncodedCert, firstCert->EncodedCertLen);
        SuccessOrExit(err);
    }

    for (uint8_t i = 0; i < CertCount; i++)
    {
        WeaveCertificateData& cert = Certs[i];
        if (cert.EncodedCert != NULL && &cert != firstCert && (includeTrusted || (cert.CertFlags & kCertFlag_IsTrusted) == 0))
        {
            err = writer.PutPreEncodedContainer(AnonymousTag, kTLVType_Structure, cert.EncodedCert, cert.EncodedCertLen);
            SuccessOrExit(err);
        }
    }

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::ValidateCert(WeaveCertificateData& cert, ValidationContext& context)
{
    WEAVE_ERROR err;

    VerifyOrExit(&cert >= Certs && &cert < &Certs[CertCount], err = WEAVE_ERROR_INVALID_ARGUMENT);

#if WEAVE_CONFIG_DEBUG_CERT_VALIDATION

    if (context.CertValidationResults != NULL)
    {
        VerifyOrExit(context.CertValidationResultsLen >= CertCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

        for (uint8_t i = 0; i < context.CertValidationResultsLen; i++)
            context.CertValidationResults[i] = WEAVE_CERT_NOT_USED;
    }

#endif

    context.TrustAnchor = NULL;

    err = ValidateCert(cert, context, context.ValidateFlags, 0);

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::FindValidCert(const WeaveDN& subjectDN, const CertificateKeyId& subjectKeyId,
        ValidationContext& context, WeaveCertificateData *& cert)
{
    WEAVE_ERROR err;

#if WEAVE_CONFIG_DEBUG_CERT_VALIDATION

    if (context.CertValidationResults != NULL)
    {
        VerifyOrExit(context.CertValidationResultsLen >= CertCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

        for (uint8_t i = 0; i < context.CertValidationResultsLen; i++)
            context.CertValidationResults[i] = WEAVE_CERT_NOT_USED;
    }

#endif

    context.TrustAnchor = NULL;

    err = FindValidCert(subjectDN, subjectKeyId, context, context.ValidateFlags, 0, cert);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveCertificateSet::GenerateECDSASignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                                        WeaveCertificateData& cert,
                                                        const nl::Weave::Crypto::EncodedECPrivateKey& privKey,
                                                        nl::Weave::Crypto::EncodedECDSASignature& encodedSig)
{
    return nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(cert.PubKeyCurveId),
            msgHash, msgHashLen, privKey, encodedSig);
}

WEAVE_ERROR WeaveCertificateSet::VerifyECDSASignature(const uint8_t *msgHash, uint8_t msgHashLen,
                                                      const nl::Weave::Crypto::EncodedECDSASignature& encodedSig,
                                                      WeaveCertificateData& cert)
{
    return nl::Weave::Crypto::VerifyECDSASignature(WeaveCurveIdToOID(cert.PubKeyCurveId),
            msgHash, msgHashLen, encodedSig, cert.PublicKey.EC);
}

WEAVE_ERROR WeaveCertificateSet::ValidateCert(WeaveCertificateData& cert, ValidationContext& context, uint16_t validateFlags, uint8_t depth)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateData *caCert = NULL;
    uint8_t hashLen;
    enum { kLastSecondOfDay = kSecondsPerDay - 1 };

    // If the depth is greater than 0 then the certificate is required to be a CA certificate...
    if (depth > 0)
    {
        // Verify the isCA flag is present.
        VerifyOrExit((cert.CertFlags & kCertFlag_IsCA) != 0, err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

        // Verify the key usage extension is present and contains the 'keyCertSign' flag.
        VerifyOrExit((cert.CertFlags & kCertFlag_ExtPresent_KeyUsage) != 0 &&
                     (cert.KeyUsageFlags & kKeyUsageFlag_KeyCertSign) != 0,
                     err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

        // Verify that the certificate type is set to "CA".
        VerifyOrExit(cert.CertType == kCertType_CA, err = WEAVE_ERROR_WRONG_CERT_TYPE);

        // If a path length constraint was included, verify the cert depth vs. the specified constraint.
        //
        // From the RFC, the path length constraint "gives the maximum number of non-self-issued
        // intermediate certificates that may follow this certificate in a valid certification path.
        // (Note: The last certificate in the certification path is not an intermediate certificate,
        // and is not included in this limit...)"
        //
        if ((cert.CertFlags & kCertFlag_PathLenConstPresent) != 0)
            VerifyOrExit((depth - 1) <= cert.PathLenConstraint, err = WEAVE_ERROR_CERT_PATH_LEN_CONSTRAINT_EXCEEDED);
    }

    // Otherwise verify the desired certificate usages/purposes/type given in the validation context...
    else
    {
        // If a set of desired key usages has been specified, verify that the key usage extension exists
        // in the certificate and that the corresponding usages are supported.
        if (context.RequiredKeyUsages != 0)
            VerifyOrExit((cert.CertFlags & kCertFlag_ExtPresent_KeyUsage) != 0 &&
                         (cert.KeyUsageFlags & context.RequiredKeyUsages) == context.RequiredKeyUsages,
                         err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

        // If a set of desired key purposes has been specified, verify that the extended key usage extension
        // exists in the certificate and that the corresponding purposes are supported.
        if (context.RequiredKeyPurposes != 0)
            VerifyOrExit((cert.CertFlags & kCertFlag_ExtPresent_ExtendedKeyUsage) != 0 &&
                         (cert.KeyPurposeFlags & context.RequiredKeyPurposes) == context.RequiredKeyPurposes,
                         err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

        // If a required certificate type has been specified, verify it against the current certificate's type.
        if (context.RequiredCertType != kCertType_NotSpecified)
            VerifyOrExit(cert.CertType == context.RequiredCertType, err = WEAVE_ERROR_WRONG_CERT_TYPE);
    }

    // Verify the validity time of the certificate, if requested.
    if (cert.NotBeforeDate != 0 && (validateFlags & kValidateFlag_IgnoreNotBefore) == 0)
        VerifyOrExit(context.EffectiveTime >= PackedCertDateToTime(cert.NotBeforeDate), err = WEAVE_ERROR_CERT_NOT_VALID_YET);
    if (cert.NotAfterDate != 0 && (validateFlags & kValidateFlag_IgnoreNotAfter) == 0)
        VerifyOrExit(context.EffectiveTime <= PackedCertDateToTime(cert.NotAfterDate) + kLastSecondOfDay, err = WEAVE_ERROR_CERT_EXPIRED);

    // If the certificate itself is trusted, then it is implicitly valid.  Record this certificate as the trust
    // anchor and return success.
    if ((cert.CertFlags & kCertFlag_IsTrusted) != 0)
    {
        context.TrustAnchor = &cert;
        ExitNow(err = WEAVE_NO_ERROR);
    }

    // Otherwise we must validate the certificate by looking for a chain of valid certificates up to a trusted
    // certificate known as the 'trust anchor'.

    // Fail validation if the certificate is self-signed. Since we don't trust this certificate (see the check above) and
    // it has no path we can follow to a trust anchor, it can't be considered valid.
    if (cert.IssuerDN.IsEqual(cert.SubjectDN) && cert.AuthKeyId.IsEqual(cert.SubjectKeyId))
        ExitNow(err = WEAVE_ERROR_CERT_NOT_TRUSTED);

    // Verify that the certificate depth is less than the total number of certificates. It is technically possible to create
    // a circular chain of certificates.  Limiting the maximum depth of the certificate path prevents infinite
    // recursion in such a case.
    VerifyOrExit(depth < CertCount, err = WEAVE_ERROR_CERT_PATH_TOO_LONG);

    // Verify that a hash of the 'to-be-signed' portion of the certificate has been computed. We will need this to
    // verify the cert's signature below.
    VerifyOrExit((cert.CertFlags & kCertFlag_TBSHashPresent) != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // If a certificate signed with SHA-256 is required, verify the signature algorithm.
    if ((validateFlags & kValidateFlag_RequireSHA256) != 0)
        VerifyOrExit(cert.SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256, err = WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM);

	// If the current certificate was signed with SHA-256, require that the CA certificate is also signed with SHA-256.
	if (cert.SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256)
		validateFlags |= kValidateFlag_RequireSHA256;

    // Search for a valid CA certificate that matches the Issuer DN and Authority Key Id of the current certificate.
	// Fail if no acceptable certificate is found.
	err = FindValidCert(cert.IssuerDN, cert.AuthKeyId, context, validateFlags, depth + 1, caCert);
	if (err != WEAVE_NO_ERROR)
		ExitNow(err = WEAVE_ERROR_CA_CERT_NOT_FOUND);

    // Verify signature of the current certificate against public key of the CA certificate. If signature verification
	// succeeds, the current certificate is valid.
    hashLen = (cert.SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256)
              ? (uint8_t)Platform::Security::SHA256::kHashLength
              : (uint8_t)Platform::Security::SHA1::kHashLength;
    err = VerifyECDSASignature(cert.TBSHash, hashLen, cert.Signature.EC, *caCert);
    SuccessOrExit(err);

exit:

#if WEAVE_CONFIG_DEBUG_CERT_VALIDATION
    if (context.CertValidationResults != NULL)
        context.CertValidationResults[&cert - Certs] = err;
#endif

    return err;
}

WEAVE_ERROR WeaveCertificateSet::FindValidCert(const WeaveDN& subjectDN, const CertificateKeyId& subjectKeyId,
        ValidationContext& context, uint16_t validateFlags, uint8_t depth, WeaveCertificateData *& cert)
{
    WEAVE_ERROR err;

    // Default error if we don't find any matching cert.
    err = (depth > 0) ? WEAVE_ERROR_CA_CERT_NOT_FOUND : WEAVE_ERROR_CERT_NOT_FOUND;

    // Fail immediately if neither of the input criteria are specified.
    if (subjectDN.IsEmpty() && subjectKeyId.IsEmpty())
        ExitNow();

    // For each cert in the set...
    for (uint8_t i = 0; i < CertCount; i++)
    {
        WeaveCertificateData& candidateCert = Certs[i];

        // Skip the certificate if its subject DN and key id do not match the input criteria.
        if (!subjectDN.IsEmpty() && !candidateCert.SubjectDN.IsEqual(subjectDN))
            continue;
        if (!subjectKeyId.IsEmpty() && !candidateCert.SubjectKeyId.IsEqual(subjectKeyId))
            continue;

        // Attempt to validate the cert.  If the cert is valid, return it to the caller. Otherwise,
        // save the returned error and continue searching.  If there are no other matching certs this
        // will be the error returned to the caller.
        err = ValidateCert(candidateCert, context, validateFlags, depth);
        if (err == WEAVE_NO_ERROR)
        {
            cert = &candidateCert;
            ExitNow();
        }
    }

    cert = NULL;

exit:
    return err;
}

/**
 * Determine general type of a Weave certificate.
 *
 * This function performs a general assessment of a certificate's type based
 * on the structure of its subject DN and the extensions present.  Applications
 * are free to override this assessment by setting cert.CertType to another value,
 * including an application-defined one.
 *
 * In general, applications will only trust a peer's certificate if it chains to a trusted
 * root certificate.  However, the type assigned to a certificate can influence the *nature*
 * of this trust, e.g. to allow or disallow access to certain features.  Because of this,
 * changes to this algorithm can have VERY SIGNIFICANT and POTENTIALLY CATASTROPHIC effects
 * on overall system security, and should not be made without a thorough understanding of
 * the implications.
 *
 * NOTE: Access token certificates cannot be distinguished solely by their structure.
 * Thus this function never sets cert.CertType = kCertType_AccessToken.
 */
WEAVE_ERROR DetermineCertType(WeaveCertificateData& cert)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the BasicConstraints isCA flag is true...
    if ((cert.CertFlags & kCertFlag_IsCA) != 0)
    {
        // Verify the key usage extension is present and contains the 'keyCertSign' flag.
        VerifyOrExit((cert.CertFlags & kCertFlag_ExtPresent_KeyUsage) != 0 &&
                     (cert.KeyUsageFlags & kKeyUsageFlag_KeyCertSign) != 0,
                     err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

        // Set the certificate type to CA.
        cert.CertType = kCertType_CA;
    }

    // If the certificate subject contains a WeaveDeviceId attribute set the certificate type to Device.
    else if (cert.SubjectDN.AttrOID == kOID_AttributeType_WeaveDeviceId)
    {
        cert.CertType = kCertType_Device;
    }

    // If the certificate subject contains a WeaveServiceEndpointId attribute set the certificate type to ServiceEndpoint.
    else if (cert.SubjectDN.AttrOID == kOID_AttributeType_WeaveServiceEndpointId)
    {
        cert.CertType = kCertType_ServiceEndpoint;
    }

    // If the certificate subject contains a WeaveSoftwarePublisherId attribute set the certificate type to FirmwareSigning.
    else if (cert.SubjectDN.AttrOID == kOID_AttributeType_WeaveSoftwarePublisherId)
    {
        cert.CertType = kCertType_FirmwareSigning;
    }

    // Otherwise set the certificate type to General.
    else
    {
        cert.CertType = kCertType_General;
    }

exit:
    return err;
}

bool WeaveDN::IsEqual(const WeaveDN& other) const
{
    if (AttrOID == kOID_Unknown || AttrOID == kOID_NotSpecified || AttrOID != other.AttrOID)
        return false;

    if (IsWeaveIdX509Attr(AttrOID))
        return AttrValue.WeaveId == other.AttrValue.WeaveId;
    else
        return (AttrValue.String.Len == other.AttrValue.String.Len &&
                memcmp(AttrValue.String.Value, other.AttrValue.String.Value, AttrValue.String.Len) == 0);
}

bool CertificateKeyId::IsEqual(const CertificateKeyId& other) const
{
    return Id != NULL && other.Id != NULL && Len == other.Len && memcmp(Id, other.Id, Len) == 0;
}

/**
 * @brief
 *   Convert a certificate date/time (in the form of an ASN.1 universal time structure) into a packed
 *   certificate date/time.
 *
 * @details
 *   Packed certificate date/times provide a compact representation for the time values within a certificate
 *   (notBefore and notAfter) that does not require full calendar math to interpret.
 *
 *   A packed certificate date/time contains the fields of a calendar date/time--i.e. year, month, day, hour,
 *   minute, second--packed into an unsigned integer. The bit representation is organized such that
 *   ordinal comparisons of packed date/time values correspond to the natural ordering of the corresponding
 *   times.  To reduce their size, packed certificate date/times are limited to representing times that are on
 *   or after 2000/01/01 00:00:00.  When housed within a 32-bit unsigned integer, packed certificate
 *   date/times can represent times up to the year 2133.
 *
 * @note
 *   This function makes no attempt to verify the correct range of the input time other than year.
 *   Therefore callers must make sure the supplied values are valid prior to invocation.
 *
 * @param time
 *   The calendar date/time to be converted.
 *
 * @param packedTime
 *   A reference to an integer that will receive packed date/time.
 *
 * @retval  #WEAVE_NO_ERROR                     If the input time was successfully converted.
 * @retval  #ASN1_ERROR_UNSUPPORTED_ENCODING    If the input time contained a year value that could not
 *                                              be represented in a packed certificate time value.
 */
NL_DLL_EXPORT WEAVE_ERROR PackCertTime(const ASN1UniversalTime& time, uint32_t& packedTime)
{
    enum {
        kCertTimeBaseYear = 2000,
        kCertTimeMaxYear = kCertTimeBaseYear + UINT32_MAX / (kMonthsPerYear * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute),
        kX509NoWellDefinedExpirationDateYear = 9999
    };

    // The packed time in a Weave certificate cannot represent dates prior to 2000/01/01.
    if (time.Year < kCertTimeBaseYear)
        return ASN1_ERROR_UNSUPPORTED_ENCODING;

    // X.509/RFC5280 defines the special time 99991231235959Z to mean 'no well-defined expiration date'.
    // We represent that as a packed time value of 0, which for simplicity's sake is assigned to any
    // date in the associated year.
    if (time.Year == kX509NoWellDefinedExpirationDateYear)
    {
        packedTime = kNullCertTime;
        return WEAVE_NO_ERROR;
    }

    // Technically packed certificate time values could grow beyond 32bits. However we restrict it here
    // to dates that fit within 32bits to reduce code size and eliminate the need for 64bit math.
    if (time.Year > kCertTimeMaxYear)
        return ASN1_ERROR_UNSUPPORTED_ENCODING;

    packedTime = time.Year - kCertTimeBaseYear;
    packedTime = packedTime * kMonthsPerYear + time.Month - 1;
    packedTime = packedTime * kMaxDaysPerMonth + time.Day - 1;
    packedTime = packedTime * kHoursPerDay + time.Hour;
    packedTime = packedTime * kMinutesPerHour + time.Minute;
    packedTime = packedTime * kSecondsPerMinute + time.Second;

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *   Unpack a packed certificate date/time into an ASN.1 universal time structure.
 *
 * @param packedTime
 *   A packed certificate time to be unpacked.
 *
 * @param time
 *   A reference to an ASN1UniversalTime structure to receive the unpacked date/time.
 *
 * @retval  #WEAVE_NO_ERROR                     If the input time was successfully unpacked.
 */
NL_DLL_EXPORT WEAVE_ERROR UnpackCertTime(uint32_t packedTime, ASN1UniversalTime& time)
{
    enum {
        kCertTimeBaseYear = 2000,
        kX509NoWellDefinedExpirationDateYear = 9999,
    };

    // X.509/RFC5280 defines the special time 99991231235959Z to mean 'no well-defined expiration date'.
    // We represent that as a packed time value of 0.
    if (packedTime == kNullCertTime)
    {
        time.Year = kX509NoWellDefinedExpirationDateYear;
        time.Month = kMonthsPerYear;
        time.Day = kMaxDaysPerMonth;
        time.Hour = kHoursPerDay - 1;
        time.Minute = kMinutesPerHour - 1;
        time.Second = kSecondsPerMinute - 1;
    }

    else
    {
        time.Second = packedTime % kSecondsPerMinute;
        packedTime /= kSecondsPerMinute;

        time.Minute = packedTime % kMinutesPerHour;
        packedTime /= kMinutesPerHour;

        time.Hour = packedTime % kHoursPerDay;
        packedTime /= kHoursPerDay;

        time.Day = (packedTime % kMaxDaysPerMonth) + 1;
        packedTime /= kMaxDaysPerMonth;

        time.Month = (packedTime % kMonthsPerYear) + 1;
        packedTime /= kMonthsPerYear;

        time.Year = packedTime + kCertTimeBaseYear;
    }

    return WEAVE_NO_ERROR;
}

/**
 * @brief
 *   Convert a packed certificate date/time to a packed certificate date.
 *
 * @details
 *   A packed certificate date contains the fields of a calendar date--year, month, day--packed into an
 *   unsigned integer.  The bits are organized such that ordinal comparisons of packed date values
 *   correspond to the natural ordering of the corresponding dates.  To reduce their size, packed
 *   certificate dates are limited to representing dates on or after 2000/01/01.  When housed within
 *   a 16-bit unsigned integer, packed certificate dates can represent dates up to the year 2176.
 *
 * @param packedTime
 *   The packed certificate date/time to be converted.
 *
 * @return
 *   A corresponding packet certificate date.
 */
NL_DLL_EXPORT uint16_t PackedCertTimeToDate(uint32_t packedTime)
{
    return (uint16_t)(packedTime / kSecondsPerDay);
}

/**
 * @brief
 *   Convert a packed certificate date to a corresponding packed certificate date/time, where
 *   the time portion of the value is set to 00:00:00.
 *
 * @param packedDate
 *   The packed certificate date to be converted.
 *
 * @return
 *   A corresponding packet certificate date/time.
 */
NL_DLL_EXPORT uint32_t PackedCertDateToTime(uint16_t packedDate)
{
    return (uint32_t)packedDate * kSecondsPerDay;
}

/**
 * @brief
 *   Convert the number of seconds since 1970-01-01 00:00:00 UTC to a packed certificate date/time.
 *
 * @param secondsSinceEpoch
 *   Number of seconds since 1970-01-01 00:00:00 UTC.  Note: this value is compatible with
 *   *positive* values of the POSIX time_t value, up to the year 2105.
 *
 * @return
 *   A corresponding packet certificate date/time.
 */
NL_DLL_EXPORT uint32_t SecondsSinceEpochToPackedCertTime(uint32_t secondsSinceEpoch)
{
    nl::Weave::ASN1::ASN1UniversalTime asn1Time;
    uint32_t packedTime;

    // Convert seconds-since-epoch to calendar date and time and store in an ASN1UniversalTime structure.
    SecondsSinceEpochToCalendarTime(secondsSinceEpoch, asn1Time.Year, asn1Time.Month, asn1Time.Day, asn1Time.Hour, asn1Time.Minute, asn1Time.Second);

    // Convert the calendar date/time to a packed certificate date/time.
    PackCertTime(asn1Time, packedTime);

    return packedTime;
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

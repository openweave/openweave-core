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
 *      This file implements interfaces for generating, verifying, and
 *      working with Weave security signatures.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/ASN1Macros.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Crypto;
using nl::Weave::Platform::Security::SHA1;
using nl::Weave::Platform::Security::SHA256;

#define kWeaveSigTag ProfileTag(kWeaveProfile_Security, kTag_WeaveSignature)

WEAVE_ERROR WeaveSignatureGeneratorBase::GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer)
{
    return GenerateSignature(msgHash, msgHashLen, writer, kWeaveSigTag);
}

WEAVE_ERROR WeaveSignatureGeneratorBase::GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen,
        uint8_t * sigBuf, uint16_t sigBufSize, uint16_t & sigLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;

    writer.Init(sigBuf, sigBufSize);

    err = GenerateSignature(msgHash, msgHashLen, writer, kWeaveSigTag);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    sigLen = writer.GetLengthWritten();

exit:
    return err;
}

WEAVE_ERROR WeaveSignatureGeneratorBase::GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    TLVType containerType;

    VerifyOrExit(SigningCert != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Fail with UNSUPPORTED error if caller requested inclusion of certificate DN.
    // This feature is not currently supported.
    VerifyOrExit((Flags & kGenerateWeaveSignatureFlag_IncludeSigningCertSubjectDN) == 0,
                 err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

    // Start encoding the WeaveSignature structure.
    err = writer.StartContainer(tag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // If the signature algorithm is NOT ECDSAWithSHA1, encode the SignatureAlgorithm field.
    if (SigAlgoOID != kOID_SigAlgo_ECDSAWithSHA1)
    {
        err = writer.Put(ContextTag(kTag_WeaveSignature_SignatureAlgorithm), SigAlgoOID);
        SuccessOrExit(err);
    }

    // Call the subclass to compute the actual signature data and encode it into the WeaveSignature structure.
    err = GenerateSignatureData(msgHash, msgHashLen, writer);
    SuccessOrExit(err);

    // If requested to include the signing certificate subject key id...
    if ((Flags & kGenerateWeaveSignatureFlag_IncludeSigningCertKeyId) != 0)
    {
        TLVType containerType2;

        // Verify that the signing certificate data includes a subject key id.
        VerifyOrExit(!SigningCert->SubjectKeyId.IsEmpty(), err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Start the SigningCertificateRef structure.
        err = writer.StartContainer(ContextTag(kTag_WeaveSignature_SigningCertificateRef),
                                    kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // Write the Public Key Id field containing signing certificate's subject key id.
        err = writer.PutBytes(ContextTag(kTag_WeaveCertificateRef_PublicKeyId),
                              SigningCert->SubjectKeyId.Id, SigningCert->SubjectKeyId.Len);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // If requested to include related certificates...
    if ((Flags & kGenerateWeaveSignatureFlag_IncludeRelatedCertificates) != 0)
    {
        TLVType containerType2;

        // Start the RelatedCertificates array.  This contains the list of certificates the signature verifier
        // will need to verify the signature.
        err = writer.StartContainer(ContextTag(kTag_WeaveSignature_RelatedCertificates), kTLVType_Array, containerType2);
        SuccessOrExit(err);

        // Write all the non-trusted certificates currently in the certificate set, placing the signing certificate
        // first in the list.
        err = CertSet.SaveCerts(writer, SigningCert, false);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveSignatureGenerator::GenerateSignature(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;

    VerifyOrExit(PrivKey != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = WeaveSignatureGeneratorBase::GenerateSignature(msgHash, msgHashLen, writer, tag);
    SuccessOrExit(err);

exit:
    return err;
}
WEAVE_ERROR WeaveSignatureGenerator::GenerateSignatureData(const uint8_t * msgHash, uint8_t msgHashLen, TLVWriter & writer)
{
    WEAVE_ERROR err;
    EncodedECPrivateKey privKey;
    EncodedECPublicKey pubKeyForPrivKey;
    EncodedECDSASignature ecdsaSig;
    uint32_t privKeyCurveId;
    uint8_t ecdsaRBuf[EncodedECDSASignature::kMaxValueLength];
    uint8_t ecdsaSBuf[EncodedECDSASignature::kMaxValueLength];

    // Verify the specified signature algorithm is supported.
    VerifyOrExit(SigAlgoOID != kOID_SigAlgo_SHA1WithRSAEncryption, err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);
    VerifyOrExit(SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1 || SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify the length of the supplied message hash.
    VerifyOrExit((SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1 && msgHashLen == SHA1::kHashLength) ||
                 (SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256 && msgHashLen == SHA256::kHashLength), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Decode the supplied private key.
    err = DecodeWeaveECPrivateKey(PrivKey, PrivKeyLen, privKeyCurveId, pubKeyForPrivKey, privKey);
    SuccessOrExit(err);

    // Verify the signing cert's public key and the supplied private key are from the same curve.
    VerifyOrExit(privKeyCurveId == SigningCert->PubKeyCurveId, err = WEAVE_ERROR_WRONG_KEY_TYPE);

    // If the private key included a copy of the public key, verify it matches the public key of
    // the certificate.
    if (pubKeyForPrivKey.ECPoint != NULL)
        VerifyOrExit(pubKeyForPrivKey.IsEqual(SigningCert->PublicKey.EC), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Use temporary buffers to hold the generated signature value until we write it.
    ecdsaSig.R = ecdsaRBuf;
    ecdsaSig.RLen = sizeof(ecdsaRBuf);
    ecdsaSig.S = ecdsaSBuf;
    ecdsaSig.SLen = sizeof(ecdsaSBuf);

    // Generate an ECDSA signature for the given message hash.
    err = nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(privKeyCurveId),
                                                    msgHash, msgHashLen,
                                                    privKey,
                                                    ecdsaSig);
    SuccessOrExit(err);

    // Encode the the signature as a Weave ECDSASignature TLV structure, using the
    // appropriate WeaveSignature context tag to identify the type of signature.
    err = EncodeWeaveECDSASignature(writer, ecdsaSig, ContextTag(kTag_WeaveSignature_ECDSASignatureData));
    SuccessOrExit(err);

exit:
    return err;
}

NL_DLL_EXPORT WEAVE_ERROR VerifyWeaveSignature(
    const uint8_t *msgHash, uint8_t msgHashLen,
    const uint8_t *sig, uint16_t sigLen,
    WeaveCertificateSet& certSet,
    ValidationContext& certValidContext)
{
    return VerifyWeaveSignature(msgHash, msgHashLen, sig, sigLen, kOID_SigAlgo_ECDSAWithSHA1, certSet, certValidContext);
}

NL_DLL_EXPORT WEAVE_ERROR VerifyWeaveSignature(
    const uint8_t *msgHash, uint8_t msgHashLen,
    const uint8_t *sig, uint16_t sigLen, OID expectedSigAlgoOID,
    WeaveCertificateSet& certSet,
    ValidationContext& certValidContext)
{
    WEAVE_ERROR err;
    TLVReader reader;
    EncodedECDSASignature ecdsaSig;
    WeaveCertificateData *signingCert = NULL;
    WeaveDN signingCertDN;
    CertificateKeyId signingCertSubjectKeyId;
    OID sigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1; // Default if not specified in signature.

    VerifyOrExit(expectedSigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1 || expectedSigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256,
                 err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

    signingCertDN.Clear();
    signingCertSubjectKeyId.Clear();

    reader.Init(sig, sigLen);

    // Parse the beginning of the WeaveSignature structure.
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveSignature));
    SuccessOrExit(err);

    {
        TLVType containerType;

        err = reader.EnterContainer(containerType);
        SuccessOrExit(err);

        err = reader.Next();

        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_WeaveSignature_SignatureAlgorithm))
        {
            err = reader.Get(sigAlgoOID);
            SuccessOrExit(err);

            err = reader.Next();
        }

        VerifyOrExit(sigAlgoOID == expectedSigAlgoOID, err = WEAVE_ERROR_WRONG_WEAVE_SIGNATURE_ALGORITHM);

        if (err == WEAVE_NO_ERROR)
        {
            VerifyOrExit(reader.GetTag() == ContextTag(kTag_WeaveSignature_ECDSASignatureData), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
            VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            // Decode the contained ECDSA signature.
            err = DecodeWeaveECDSASignature(reader, ecdsaSig);
            SuccessOrExit(err);

            err = reader.Next();
        }

        // Look for the SigningCertificateRef structure.  If found...
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_WeaveSignature_SigningCertificateRef))
        {
            TLVType containerType2;

            VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            // Enter the SigningCertificateRef structure.
            err = reader.EnterContainer(containerType2);
            SuccessOrExit(err);

            // Advance to the first element.
            err = reader.Next();

            // Fail with an UNSUPPORTED error if the SigningCertificateRef contains a Subject path.  This form
            // of certificate reference is not currently supported.
            if (err == WEAVE_NO_ERROR)
            {
                VerifyOrExit(reader.GetTag() != ContextTag(kTag_WeaveCertificateRef_Subject), err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);
            }

            // Look for the PublicKeyId field.  If found...
            if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_WeaveCertificateRef_PublicKeyId))
            {
                VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                // Extract the subject key id of the signing certificate.
                uint32_t len = reader.GetLength();
                VerifyOrExit(len < UINT8_MAX, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
                signingCertSubjectKeyId.Len = (uint8_t)len;
                err = reader.GetDataPtr(signingCertSubjectKeyId.Id);
                SuccessOrExit(err);

                err = reader.Next();
            }

            if (err != WEAVE_END_OF_TLV)
                SuccessOrExit(err);

            err = reader.VerifyEndOfContainer();
            SuccessOrExit(err);

            err = reader.ExitContainer(containerType2);
            SuccessOrExit(err);

            err = reader.Next();
        }

        // If the RelatedCertificates array is present, load the specified certificates into the cert set...
        if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_WeaveSignature_RelatedCertificates))
        {
            uint8_t initialCertCount = certSet.CertCount;

            VerifyOrExit(reader.GetType() == kTLVType_Array, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            err = certSet.LoadCerts(reader, kDecodeFlag_GenerateTBSHash);
            SuccessOrExit(err);

            // If one or more certificates were loaded...
            if (certSet.CertCount > initialCertCount)
            {
                // Unless otherwise specified, the signing certificate will be the first certificate
                // in the RelatedCertificates array.  So extract its subject key id and DN.
                if (signingCertDN.IsEmpty() && signingCertSubjectKeyId.IsEmpty())
                {
                    signingCertSubjectKeyId = certSet.Certs[initialCertCount].SubjectKeyId;
                    signingCertDN = certSet.Certs[initialCertCount].SubjectDN;
                }
            }

            err = reader.Next();
        }

        if (err != WEAVE_END_OF_TLV)
            SuccessOrExit(err);

        err = reader.VerifyEndOfContainer();
        SuccessOrExit(err);

        err = reader.ExitContainer(containerType);
        SuccessOrExit(err);
    }

    // Verify the length of the supplied message hash.
    VerifyOrExit((sigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1 && msgHashLen == SHA1::kHashLength) ||
                 (sigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256 && msgHashLen == SHA256::kHashLength), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Search the certificate set for the signing certificate and validate that it is trusted and
    // suitable for signing.
    certValidContext.RequiredKeyUsages |= kKeyUsageFlag_DigitalSignature;
    err = certSet.FindValidCert(signingCertDN, signingCertSubjectKeyId, certValidContext, signingCert);
    SuccessOrExit(err);

    // Verify the signature against the given message hash and the signing cert's public key.
    err = nl::Weave::Crypto::VerifyECDSASignature(WeaveCurveIdToOID(signingCert->PubKeyCurveId),
                                                  msgHash, msgHashLen,
                                                  ecdsaSig,
                                                  signingCert->PublicKey.EC);
    SuccessOrExit(err);

    // Record signing certificate.
    certValidContext.SigningCert = signingCert;

exit:
    return err;
}

WEAVE_ERROR GetWeaveSignatureAlgo(const uint8_t *sig, uint16_t sigLen, OID& sigAlgoOID)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType containerType;

    // Defaults to ECDSA-SHA1 if not specified in signature object.
    sigAlgoOID = kOID_SigAlgo_ECDSAWithSHA1;

    reader.Init(sig, sigLen);

    // Parse the beginning of the WeaveSignature structure.
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveSignature));
    SuccessOrExit(err);

    err = reader.EnterContainer(containerType);
    SuccessOrExit(err);

    err = reader.Next();

    if (err == WEAVE_NO_ERROR && reader.GetTag() == ContextTag(kTag_WeaveSignature_SignatureAlgorithm))
    {
        err = reader.Get(sigAlgoOID);
        SuccessOrExit(err);
    }

exit:
    return err;
}

/**
 * Generate and encode a Weave ECDSA signature
 *
 * Computes an ECDSA signature using a given private key and message hash and write the signature
 * as a Weave ECDSASignature structure to the specified TLV writer with the given tag.
 *
 * @param[in] writer            The TLVWriter object to which the encoded signature should
 *                              be written.
 * @param[in] tag               TLV tag to be associated with the encoded signature structure.
 * @param[in] msgHash           A buffer containing the hash of the message to be signed.
 * @param[in] msgHashLen        The length in bytes of the message hash.
 * @param[in] signingKey        A buffer containing the private key to be used to generate
 *                              the signature.  The private key is expected to be encoded as
 *                              a Weave EllipticCurvePrivateKey TLV structure.
 * @param[in] signingKeyLen     The length in bytes of the encoded private key.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to decoding the private key,
 *                              generating the signature or encoding the signature.
 *
 */
WEAVE_ERROR GenerateAndEncodeWeaveECDSASignature(TLVWriter& writer, uint64_t tag,
        const uint8_t * msgHash, uint8_t msgHashLen,
        const uint8_t * signingKey, uint16_t signingKeyLen)
{
    WEAVE_ERROR err;
    uint32_t privKeyCurveId;
    EncodedECPublicKey pubKey;
    EncodedECPrivateKey privKey;
    EncodedECDSASignature ecdsaSig;
    uint8_t ecdsaRBuf[EncodedECDSASignature::kMaxValueLength];
    uint8_t ecdsaSBuf[EncodedECDSASignature::kMaxValueLength];

    // Decode the supplied private key.
    err = DecodeWeaveECPrivateKey(signingKey, signingKeyLen, privKeyCurveId, pubKey, privKey);
    SuccessOrExit(err);

    // Use temporary buffers to hold the generated signature value until we write it.
    ecdsaSig.R = ecdsaRBuf;
    ecdsaSig.RLen = sizeof(ecdsaRBuf);
    ecdsaSig.S = ecdsaSBuf;
    ecdsaSig.SLen = sizeof(ecdsaSBuf);

    // Generate the signature for the message based on its hash.
    err = GenerateECDSASignature(WeaveCurveIdToOID(privKeyCurveId), msgHash, msgHashLen, privKey, ecdsaSig);
    SuccessOrExit(err);

    // Encode an ECDSASignature structure into the supplied writer.
    err = EncodeWeaveECDSASignature(writer, ecdsaSig, tag);
    SuccessOrExit(err);

exit:
    return err;
}

// Encode a Weave ECDSASignature structure.
WEAVE_ERROR EncodeWeaveECDSASignature(TLVWriter& writer, EncodedECDSASignature& sig, uint64_t tag)
{
    WEAVE_ERROR err;
    TLVType containerType;

    // Start the ECDSASignature structure
    err = writer.StartContainer(tag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Write the R value
    err = writer.PutBytes(ContextTag(kTag_ECDSASignature_r), sig.R, sig.RLen);
    SuccessOrExit(err);

    // Write the S value
    err = writer.PutBytes(ContextTag(kTag_ECDSASignature_s), sig.S, sig.SLen);
    SuccessOrExit(err);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

// Takes an ECDSA signature in DER form and converts it to Weave form.
// ECDSA-Sig-Value ::= SEQUENCE { r INTEGER, s INTEGER }
WEAVE_ERROR ConvertECDSASignature_DERToWeave(const uint8_t * sigBuf, uint8_t sigLen, TLVWriter& writer, uint64_t tag)
{
    WEAVE_ERROR err;
    EncodedECDSASignature sig;
    ASN1Reader reader;

    reader.Init(sigBuf, sigLen);

    // ECDSA-Sig-Value ::= SEQUENCE
    ASN1_PARSE_ENTER_SEQUENCE {
        // r INTEGER
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
        sig.R = const_cast<uint8_t *>(reader.Value);
        sig.RLen = reader.ValueLen;
        // s INTEGER
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
        sig.S = const_cast<uint8_t *>(reader.Value);
        sig.SLen = reader.ValueLen;
    } ASN1_EXIT_SEQUENCE;

    err = EncodeWeaveECDSASignature(writer, sig, tag);
    SuccessOrExit(err);

exit:
    return err;
}

// Decode a Weave ECDSASignature structure.
WEAVE_ERROR DecodeWeaveECDSASignature(TLVReader& reader, EncodedECDSASignature& sig)
{
    WEAVE_ERROR err;
    TLVType containerType;

    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.EnterContainer(containerType);
    SuccessOrExit(err);

    // r INTEGER
    err = reader.Next(kTLVType_ByteString, ContextTag(kTag_ECDSASignature_r));
    SuccessOrExit(err);
    err = reader.GetDataPtr((const uint8_t *&)sig.R);
    SuccessOrExit(err);
    sig.RLen = reader.GetLength();

    // s INTEGER
    err = reader.Next(kTLVType_ByteString, ContextTag(kTag_ECDSASignature_s));
    SuccessOrExit(err);
    err = reader.GetDataPtr((const uint8_t *&)sig.S);
    SuccessOrExit(err);
    sig.SLen = reader.GetLength();

    err = reader.ExitContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR InsertRelatedCertificatesIntoWeaveSignature(
    uint8_t *sigBuf, uint16_t sigLen, uint16_t sigBufLen,
    const uint8_t *relatedCerts, uint16_t relatedCertsLen,
    uint16_t& outSigLen)
{
    WEAVE_ERROR err;
    TLVUpdater sigUpdater;
    TLVReader certsReader;
    TLVType sigContainerType, relatedCertsContainerType;

    // Initialize a TLVUpdater to rewrite the given signature.
    err = sigUpdater.Init(sigBuf, sigLen, sigBufLen);
    SuccessOrExit(err);

    // Parse the beginning of the existing WeaveSignature structure.
    err = sigUpdater.Next();
    SuccessOrExit(err);
    VerifyOrExit(sigUpdater.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);
    VerifyOrExit(sigUpdater.GetTag() == ProfileTag(kWeaveProfile_Security, kTag_WeaveSignature), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    // Enter the WeaveSignature structure.
    err = sigUpdater.EnterContainer(sigContainerType);
    SuccessOrExit(err);

    // Loop through all fields within the current WeaveSignature, moving them to the output signature.
    // HOWEVER, if an existing RelatedCertificates field is encountered, fail with an error.
    while ((err = sigUpdater.Next()) == WEAVE_NO_ERROR)
    {
        VerifyOrExit(sigUpdater.GetTag() != ContextTag(kTag_WeaveSignature_RelatedCertificates), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        err = sigUpdater.Move();
        SuccessOrExit(err);
    }

    // Verify we successfully found the end of the WeaveSignature structure.
    VerifyOrExit(err == WEAVE_END_OF_TLV, /* no-op */);

    // Start writing a RelatedCertificates array.
    err = sigUpdater.StartContainer(ContextTag(kTag_WeaveSignature_RelatedCertificates), kTLVType_Array, relatedCertsContainerType);
    SuccessOrExit(err);

    // Initialize a reader to read the given related certs TLV.
    certsReader.Init(relatedCerts, relatedCertsLen);
    certsReader.ImplicitProfileId = kWeaveProfile_Security;

    // Move to the first element of the related certs TLV.
    err = certsReader.Next();
    SuccessOrExit(err);

    // If the related certs TLV is an array, enter the array and advance to the first element.
    if (certsReader.GetType() == kTLVType_Array)
    {
        TLVType outerContainerType;
        err = certsReader.EnterContainer(outerContainerType);
        SuccessOrExit(err);
        err = certsReader.Next();
    }

    // Loop for each element in the related certs TLV...
    while (err == WEAVE_NO_ERROR)
    {
        // Verify that the current element from the related certs TLV is a structure (presumed to contain a certificate).
        VerifyOrExit(certsReader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        // Write a copy of the certificate element into the RelatedCertificates array.
        err = sigUpdater.CopyElement(AnonymousTag, certsReader);
        SuccessOrExit(err);

        // Advance to the next element.
        err = certsReader.Next();
    }

    // Verify we successfully processed the entire contents of the related certs TLV.
    VerifyOrExit(err == WEAVE_END_OF_TLV, /* no-op */);

    // Write the end of the RelatedCertificates array.
    err = sigUpdater.EndContainer(relatedCertsContainerType);

    // Move the remainder of the input WeaveSignature structure to the output.
    sigUpdater.MoveUntilEnd();

    // Finalize writing the output signature.
    err = sigUpdater.Finalize();
    SuccessOrExit(err);

    // Return the length of the updated signature to the caller.
    outSigLen = (uint16_t)sigUpdater.GetLengthWritten();

exit:
    return err;
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

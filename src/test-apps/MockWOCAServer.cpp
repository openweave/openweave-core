/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Certificate Provisioned protocol of the
 *      Weave Security profile used for the Weave mock
 *      device command line functional testing tool.
 *      This server is also known as Weave Operational Certificate Authority (WOCA).
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include "MockWOCAServer.h"
#include "TestWeaveCertData.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/RSA.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

#if WEAVE_WITH_OPENSSL
#include <openssl/x509.h>
#endif

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;

static WEAVE_ERROR ValidateWeaveDeviceCert(WeaveCertificateSet & certSet)
{
    WEAVE_ERROR err;
    WeaveCertificateData * cert = certSet.Certs;
    bool isSelfSigned = cert->IssuerDN.IsEqual(cert->SubjectDN);
    enum { kLastSecondOfDay = nl::kSecondsPerDay - 1 };

    // Verify that the certificate of device type.
    VerifyOrExit(cert->CertType == kCertType_Device, err = WEAVE_ERROR_WRONG_CERT_TYPE);

    // Verify correct subject attribute.
    VerifyOrExit(cert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_WeaveDeviceId, err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

    // Verify that the key usage extension exists in the certificate and that the corresponding usages are supported.
    VerifyOrExit((cert->CertFlags & kCertFlag_ExtPresent_KeyUsage) != 0 &&
                 (cert->KeyUsageFlags == (kKeyUsageFlag_DigitalSignature | kKeyUsageFlag_KeyEncipherment)),
                 err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

    // Verify the validity time of the certificate.
    {
        uint32_t effectiveTime;
        ASN1UniversalTime effectiveTimeASN1 = {
            .Year   = 2020,
            .Month  = 01,
            .Day    = 01,
            .Hour   = 00,
            .Minute = 00,
            .Second = 00
        };
        err = PackCertTime(effectiveTimeASN1, effectiveTime);
        SuccessOrExit(err);

        VerifyOrExit(effectiveTime >= PackedCertDateToTime(cert->NotBeforeDate), err = WEAVE_ERROR_CERT_NOT_VALID_YET);

        VerifyOrExit(effectiveTime <= PackedCertDateToTime(cert->NotAfterDate) + kLastSecondOfDay, err = WEAVE_ERROR_CERT_EXPIRED);
    }

    // Verify that a hash of the 'to-be-signed' portion of the certificate has been computed. We will need this to
    // verify the cert's signature below.
    VerifyOrExit((cert->CertFlags & kCertFlag_TBSHashPresent) != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct public key algorithm.
    VerifyOrExit(cert->PubKeyAlgoOID == kOID_PubKeyAlgo_ECPublicKey, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct key purpose.
    VerifyOrExit(cert->KeyPurposeFlags == (kKeyPurposeFlag_ServerAuth | kKeyPurposeFlag_ClientAuth), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct EC curve.
    VerifyOrExit((cert->PubKeyCurveId == kWeaveCurveId_prime256v1) ||
                 (cert->PubKeyCurveId == kWeaveCurveId_secp224r1), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    if (isSelfSigned)
    {
        // Verify that the certificate is self-signed.
        VerifyOrExit(cert->AuthKeyId.IsEqual(cert->SubjectKeyId), err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

        // Verify the signature algorithm.
        VerifyOrExit(cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256, err = WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM);

        // Verify certificate signature.
        err = VerifyECDSASignature(WeaveCurveIdToOID(cert->PubKeyCurveId),
                                   cert->TBSHash, SHA256::kHashLength,
                                   cert->Signature.EC, cert->PublicKey.EC);
        SuccessOrExit(err);
    }
    else
    {
        CertificateKeyId caKeyId;
        EncodedECPublicKey caPublicKey;
        OID caCurveOID;
        uint8_t tbsHashLen;

        if (cert->IssuerDN.AttrValue.WeaveId == nl::NestCerts::Development::DeviceCA::CAId)
        {
            caKeyId.Id = nl::NestCerts::Development::DeviceCA::SubjectKeyId;
            caKeyId.Len = static_cast<uint8_t>(nl::NestCerts::Development::DeviceCA::SubjectKeyIdLength);

            caPublicKey.ECPoint = const_cast<uint8_t *>(nl::NestCerts::Development::DeviceCA::PublicKey);
            caPublicKey.ECPointLen = static_cast<uint16_t>(nl::NestCerts::Development::DeviceCA::PublicKeyLength);

            caCurveOID = WeaveCurveIdToOID(nl::NestCerts::Development::DeviceCA::CurveOID);
        }
        else if (cert->IssuerDN.AttrValue.WeaveId == nl::TestCerts::sTestCert_CA_Id)
        {
            caKeyId.Id = nl::TestCerts::sTestCert_CA_SubjectKeyId;
            caKeyId.Len = nl::TestCerts::sTestCertLength_CA_SubjectKeyId;

            caPublicKey.ECPoint = const_cast<uint8_t *>(nl::TestCerts::sTestCert_CA_PublicKey);
            caPublicKey.ECPointLen = static_cast<uint16_t>(nl::TestCerts::sTestCertLength_CA_PublicKey);

            caCurveOID = WeaveCurveIdToOID(nl::TestCerts::sTestCert_CA_CurveId);
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_WRONG_CERT_SUBJECT);
        }

        // Verify that the certificate is signed by the device CA.
        VerifyOrExit(cert->AuthKeyId.IsEqual(caKeyId), err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

        // Verify the signature algorithm.
        VerifyOrExit((cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256) ||
                     (cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1), err = WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM);

        tbsHashLen = ((cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256) ? static_cast<uint8_t>(SHA256::kHashLength) : static_cast<uint8_t>(SHA1::kHashLength));

        // Verify certificate signature.
        err = VerifyECDSASignature(caCurveOID, cert->TBSHash, tbsHashLen, cert->Signature.EC, caPublicKey);
        SuccessOrExit(err);
    }

exit:
    return err;
}

#if WEAVE_WITH_OPENSSL

static WEAVE_ERROR ValidateX509DeviceCert(X509Cert *certSet, uint8_t certCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIO *certBuf[kMaxCertCount] = { NULL };
    X509 *cert[kMaxCertCount] = { NULL };
    X509_STORE *store = NULL;
    X509_STORE_CTX *ctx = NULL;
    X509_VERIFY_PARAM *param = NULL;
    int res;

    VerifyOrExit(certSet != NULL && certCount > 0 && certCount <= kMaxCertCount, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Add Trusted X509 Root Certificate.
    certSet[certCount].Cert = TestDevice_X509_RSA_RootCert;
    certSet[certCount++].Len = TestDevice_X509_RSA_RootCertLength;

    store = X509_STORE_new();
    VerifyOrExit(store != NULL, err = WEAVE_ERROR_NO_MEMORY);

    for (int i = 0; i < certCount; i++)
    {
        VerifyOrExit(certSet[i].Cert != NULL && certSet[i].Len > 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

        certBuf[i] = BIO_new_mem_buf(certSet[i].Cert, certSet[i].Len);
        VerifyOrExit(certBuf[i] != NULL, err = WEAVE_ERROR_NO_MEMORY);

        cert[i] = d2i_X509_bio(certBuf[i], NULL);
        VerifyOrExit(cert[i] != NULL, err = WEAVE_ERROR_NO_MEMORY);

        if (i > 0)
        {
            res = X509_STORE_add_cert(store, cert[i]);
            VerifyOrExit(res == 1, err = WEAVE_ERROR_NO_MEMORY);
        }
    }

    ctx = X509_STORE_CTX_new();
    VerifyOrExit(ctx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    param = X509_VERIFY_PARAM_new();
    VerifyOrExit(param != NULL, err = WEAVE_ERROR_NO_MEMORY);

    X509_VERIFY_PARAM_clear_flags(param, X509_V_FLAG_USE_CHECK_TIME);
    X509_STORE_CTX_set0_param(ctx, param);

    res = X509_STORE_CTX_init(ctx, store, cert[0], NULL);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    res = X509_verify_cert(ctx);
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    if (NULL != param) X509_VERIFY_PARAM_free(param);
    if (NULL != ctx) X509_STORE_CTX_free(ctx);
    if (NULL != store) X509_STORE_free(store);
    for (int i = 0; i < certCount; i++)
    {
        if (NULL != cert[i]) X509_free(cert[i]);
        if (NULL != certBuf[i]) BIO_free(certBuf[i]);
    }

    return err;
}

#endif // WEAVE_WITH_OPENSSL

static WEAVE_ERROR GenerateTestDeviceCert(uint64_t deviceId, EncodedECPublicKey& devicePubKey,
                                          const uint8_t *caCert, uint16_t caCertLen,
                                          const uint8_t *caKey, uint16_t caKeyLen,
                                          uint8_t *cert, uint16_t certBufSize, uint16_t& certLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType containerType;
    TLVType containerType2;
    TLVType containerType3;
    uint8_t *certDecodeBuf = NULL;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;
    WeaveCertificateData *caCertData = NULL;
    WeaveCertificateData *certData = NULL;

    VerifyOrExit(devicePubKey.ECPoint != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(caCert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(caKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Get CA certificate data.
    {
        err = certSet.Init(1, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        certSetInitialized = true;

        // Load Weave operational device certificate.
        err = certSet.LoadCert(caCert, caCertLen, kDecodeFlag_GenerateTBSHash, caCertData);
        SuccessOrExit(err);
    }

    writer.Init(cert, certBufSize);

    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Certificate serial number.
    {
        enum
        {
            kCertSerialNumber_Length          = 8,      // Length of the certificate serial number.
            kCertSerialNumber_FirstByteMask   = 0x7F,   // Mask applied on the first byte of the key Id value.
            kCertSerialNumber_FirstBytePrefix = 0x40,   // 4-bit Type value (0100) added at the beginning of the key Id value.
        };
        uint8_t certSerialNumber[kCertSerialNumber_Length];

        // Generate a random value to be used as the serial number.
        err = nl::Weave::Platform::Security::GetSecureRandomData(certSerialNumber, kCertSerialNumber_Length);
        SuccessOrExit(err);

        // Apply mask to avoid negative numbers.
        certSerialNumber[0] &= kCertSerialNumber_FirstByteMask;

        // Apply mask to guarantee the first byte is not zero.
        certSerialNumber[0] |= kCertSerialNumber_FirstBytePrefix;

        err = writer.PutBytes(ContextTag(kTag_SerialNumber), certSerialNumber, sizeof(certSerialNumber));
        SuccessOrExit(err);
    }

    // Weave signature algorithm.
    err = writer.Put(ContextTag(kTag_SignatureAlgorithm), static_cast<uint8_t>(kOID_SigAlgo_ECDSAWithSHA256 & ~kOIDCategory_Mask));
    SuccessOrExit(err);

    // Certificate issuer Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Issuer), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), caCertData->SubjectDN.AttrValue.WeaveId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate validity times.
    err = writer.Put(ContextTag(kTag_NotBefore), PackedCertDateToTime(WEAVE_CONFIG_OP_DEVICE_CERT_VALID_DATE_NOT_BEFORE));
    SuccessOrExit(err);

    // Certificate validity period is 10 year.
    err = writer.Put(ContextTag(kTag_NotAfter), PackedCertDateToTime(WEAVE_CONFIG_OP_DEVICE_CERT_VALID_DATE_NOT_BEFORE + (10 * 12 * 31)));
    SuccessOrExit(err);

    // Certificate subject Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Subject), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), deviceId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // EC public key algorithm.
    err = writer.Put(ContextTag(kTag_PublicKeyAlgorithm), static_cast<uint8_t>(kOID_PubKeyAlgo_ECPublicKey & kOID_Mask));
    SuccessOrExit(err);

    // EC public key curve Id.
    err = writer.Put(ContextTag(kTag_EllipticCurveIdentifier), static_cast<uint32_t>(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID));
    SuccessOrExit(err);

    // EC public key.
    err = writer.PutBytes(ContextTag(kTag_EllipticCurvePublicKey), devicePubKey.ECPoint, devicePubKey.ECPointLen);
    SuccessOrExit(err);

    // Certificate extension: basic constraints.
    {
        err = writer.StartContainer(ContextTag(kTag_BasicConstraints), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_BasicConstraints_Critical), true);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: key usage.
    {
        err = writer.StartContainer(ContextTag(kTag_KeyUsage), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_KeyUsage_Critical), true);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_KeyUsage_KeyUsage), static_cast<uint16_t>(kKeyUsageFlag_DigitalSignature | kKeyUsageFlag_KeyEncipherment));
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: extended key usage.
    {
        err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_ExtendedKeyUsage_Critical), true);
        SuccessOrExit(err);

        err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage_KeyPurposes), kTLVType_Array, containerType3);
        SuccessOrExit(err);

        // Key purpose is client authentication.
        err = writer.Put(AnonymousTag, static_cast<uint8_t>(kOID_KeyPurpose_ClientAuth & kOID_Mask));
        SuccessOrExit(err);

        // Key purpose is server authentication.
        err = writer.Put(AnonymousTag, static_cast<uint8_t>(kOID_KeyPurpose_ServerAuth & kOID_Mask));
        SuccessOrExit(err);

        err = writer.EndContainer(containerType3);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: subject key identifier.
    {
        /* Use "truncated" SHA-1 hash. Per RFC5280:
         *
         *     "(2) The keyIdentifier is composed of a four-bit type field with the value 0100 followed
         *     by the least significant 60 bits of the SHA-1 hash of the value of the BIT STRING
         *     subjectPublicKey (excluding the tag, length, and number of unused bits)."
         */
        enum
        {
            kCertKeyId_Length          = 8,      // Length of the certificate key identifier.
            kCertKeyId_FirstByte       = nl::Weave::Platform::Security::SHA1::kHashLength - kCertKeyId_Length,
                                                 // First byte of the SHA1 hash that used to generate certificate keyId.
            kCertKeyId_FirstByteMask   = 0x0F,   // Mask applied on the first byte of the key Id value.
            kCertKeyId_FirstBytePrefix = 0x40,   // 4-bit Type value (0100) added at the beginning of the key Id value.
        };
        nl::Weave::Platform::Security::SHA1 sha1;
        uint8_t hash[nl::Weave::Platform::Security::SHA1::kHashLength];
        uint8_t *certKeyId = &hash[kCertKeyId_FirstByte];

        sha1.Begin();
        sha1.AddData(devicePubKey.ECPoint, devicePubKey.ECPointLen);
        sha1.Finish(hash);

        certKeyId[0] &= kCertKeyId_FirstByteMask;
        certKeyId[0] |= kCertKeyId_FirstBytePrefix;

        err = writer.StartContainer(ContextTag(kTag_SubjectKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_SubjectKeyIdentifier_KeyIdentifier), certKeyId, kCertKeyId_Length);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: authority key identifier.
    {
        err = writer.StartContainer(ContextTag(kTag_AuthorityKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_AuthorityKeyIdentifier_KeyIdentifier), caCertData->SubjectKeyId.Id, caCertData->SubjectKeyId.Len);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Start the ECDSASignature structure.
    // Note that the ECDSASignature tag is added here but the actual certificate data (S and R values)
    // will be written later. This is needed to prevent DecodeConvertTBSCert() function from failing.
    // This function expects to read new non-hashable element after all TBS data is converted.
    err = writer.StartContainer(ContextTag(kTag_ECDSASignature), kTLVType_Structure, containerType2);
    SuccessOrExit(err);

    {
        enum
        {
            kCertDecodeBufferSize      = 1024,   // Maximum ASN1 encoded size of the operational device certificate.
        };
        TLVReader reader;
        ASN1Writer tbsWriter;
        TLVType readContainerType;

        reader.Init(cert, certBufSize);

        // Parse the beginning of the WeaveSignature structure.
        err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate));
        SuccessOrExit(err);

        // Enter the certificate structure.
        err = reader.EnterContainer(readContainerType);
        SuccessOrExit(err);

        // Allocate decode memory buffer.
        certDecodeBuf = static_cast<uint8_t *>(nl::Weave::Platform::Security::MemoryAlloc(kCertDecodeBufferSize));
        VerifyOrExit(certDecodeBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Allocate certificate data  structure.
        certData = static_cast<WeaveCertificateData *>(nl::Weave::Platform::Security::MemoryAlloc(sizeof(WeaveCertificateData)));
        VerifyOrExit(certData != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Initialize an ASN1Writer and convert the TBS (to-be-signed) portion of
        // the certificate to ASN.1 DER encoding.
        tbsWriter.Init(certDecodeBuf, kCertDecodeBufferSize);
        err = DecodeConvertTBSCert(reader, tbsWriter, *certData);
        SuccessOrExit(err);

        // Finish writing the ASN.1 DER encoding of the TBS certificate.
        err = tbsWriter.Finalize();
        SuccessOrExit(err);

        // Generate a SHA hash of the encoded TBS certificate.
        nl::Weave::Platform::Security::SHA256 sha256;
        sha256.Begin();
        sha256.AddData(certDecodeBuf, tbsWriter.GetLengthWritten());
        sha256.Finish(certData->TBSHash);

        uint32_t caCurveId;
        EncodedECPublicKey caPubKey;
        EncodedECPrivateKey caPrivKey;

        // Decode the CA private key.
        err = DecodeWeaveECPrivateKey(caKey, caKeyLen, caCurveId, caPubKey, caPrivKey);
        SuccessOrExit(err);

        // Reuse already allocated decode buffer to hold the generated signature value.
        EncodedECDSASignature ecdsaSig;
        ecdsaSig.R = certDecodeBuf;
        ecdsaSig.RLen = EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.S = certDecodeBuf + EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.SLen = EncodedECDSASignature::kMaxValueLength;

        // Generate an ECDSA signature for the given message hash.
        err = nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(caCurveId), certData->TBSHash, nl::Weave::Platform::Security::SHA256::kHashLength, caPrivKey, ecdsaSig);
        SuccessOrExit(err);

        // Write the R value.
        err = writer.PutBytes(ContextTag(kTag_ECDSASignature_r), ecdsaSig.R, ecdsaSig.RLen);
        SuccessOrExit(err);

        // Write the S value.
        err = writer.PutBytes(ContextTag(kTag_ECDSASignature_s), ecdsaSig.S, ecdsaSig.SLen);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType2);
    SuccessOrExit(err);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    certLen = static_cast<uint16_t>(writer.GetLengthWritten());

exit:
    if (certDecodeBuf != NULL)
        nl::Weave::Platform::Security::MemoryFree(certDecodeBuf);

    if (certSetInitialized)
        certSet.Release();

    return err;
}

GetCertificateRequestMessage::GetCertificateRequestMessage()
{
    mReqType = WeaveCertProvEngine::kReqType_NotSpecified;
    mMfrAttestType = kMfrAttestType_Undefined;

    AuthorizeInfoPairingToken = NULL;
    AuthorizeInfoPairingTokenLen = 0;
    AuthorizeInfoPairingInitData = NULL;
    AuthorizeInfoPairingInitDataLen = 0;

    memset(MfrAttestX509CertSet, 0, sizeof(MfrAttestX509CertSet));
    MfrAttestX509CertCount = 0;

    OperationalSigAlgo = kOID_NotSpecified;
    memset(&OperationalSig, 0, sizeof(OperationalSig));
    MfrAttestSigAlgo = kOID_NotSpecified;
    memset(&MfrAttestSig, 0, sizeof(MfrAttestSig));

    mOperationalCertSetInitialized = false;
    mMfrAttestCertSetInitialized = false;

    mTBSDataStart = NULL;
    mTBSDataLen = 0;
}

WEAVE_ERROR GetCertificateRequestMessage::Decode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType outerContainer;
    WeaveCertificateData * certData;

    reader.Init(msgBuf);

    // Advance the reader to the start of the GetCertificateRequest message structure.
    err = reader.Next(kTLVType_Structure, AnonymousTag);
    SuccessOrExit(err);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    // Request Type.
    {
        mTBSDataStart = reader.GetReadPoint();

        err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_GetCertReqMsg_ReqType));
        SuccessOrExit(err);

        err = reader.Get(mReqType);
        SuccessOrExit(err);

        VerifyOrExit(RequestType() == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert ||
                     RequestType() == WeaveCertProvEngine::kReqType_RotateOpDeviceCert, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Request authorization information - pairing token (optional).
    if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_Authorize_PairingToken))
    {
        err = reader.GetDataPtr(AuthorizeInfoPairingToken);
        SuccessOrExit(err);

        AuthorizeInfoPairingTokenLen = reader.GetLength();

        err = reader.Next();
        SuccessOrExit(err);

        // Request authorization information - pairing init data (optional).
        if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_Authorize_PairingInitData))
        {
            err = reader.GetDataPtr(AuthorizeInfoPairingInitData);
            SuccessOrExit(err);

            AuthorizeInfoPairingInitDataLen = reader.GetLength();

            err = reader.Next();
            SuccessOrExit(err);
        }
    }

    // Operational Device Certificate.
    {
        VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);
        VerifyOrExit(reader.GetTag() == ContextTag(kTag_GetCertReqMsg_OpDeviceCert), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        err = OperationalCertSet.Init(kMaxCertCount, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        mOperationalCertSetInitialized = true;

        // Load Weave operational device certificate.
        err = OperationalCertSet.LoadCert(reader, kDecodeFlag_GenerateTBSHash, certData);
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Load intermediate certificates (optional).
    if (reader.GetType() == kTLVType_Array && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_OpRelatedCerts))
    {
        // Intermediate certificates are not expected when self-signed certificate is used
        // in the Get Initial Operational Device Certificate Request.
        VerifyOrExit(RequestType() != WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        err = OperationalCertSet.LoadCerts(reader, kDecodeFlag_GenerateTBSHash);
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Manufacturer Attestation Information (optional).
    if (reader.GetType() == kTLVType_Structure && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveCert))
    {
        err = MfrAttestWeaveCertSet.Init(kMaxCertCount, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        mMfrAttestCertSetInitialized = true;

        // Load manufacturer attestation Weave certificate.
        err = MfrAttestWeaveCertSet.LoadCert(reader, kDecodeFlag_GenerateTBSHash, certData);
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next(kTLVType_Array, ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveRelCerts));

        if (err == WEAVE_NO_ERROR)
        {
            // Load intermediate certificate.
            err = MfrAttestWeaveCertSet.LoadCerts(reader, kDecodeFlag_GenerateTBSHash);
            SuccessOrExit(err);

            mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

            err = reader.Next();
            SuccessOrExit(err);
        }

        MfrAttestType(kMfrAttestType_WeaveCert);
    }
    else if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttest_X509Cert))
    {
        err = reader.GetDataPtr(const_cast<const uint8_t *&>(MfrAttestX509CertSet[MfrAttestX509CertCount].Cert));
        SuccessOrExit(err);

        MfrAttestX509CertSet[MfrAttestX509CertCount++].Len = reader.GetLength();

        mTBSDataLen = MfrAttestX509CertSet[0].Cert + reader.GetLength() - mTBSDataStart;

        err = reader.Next(kTLVType_Array, ContextTag(kTag_GetCertReqMsg_MfrAttest_X509RelCerts));

        // Intermediate certificates (optional).
        if (err == WEAVE_NO_ERROR)
        {
            TLVType outerContainer2;

            err = reader.EnterContainer(outerContainer2);
            SuccessOrExit(err);

            err = reader.Next();

            while (err != WEAVE_END_OF_TLV)
            {
                SuccessOrExit(err);

                VerifyOrExit(MfrAttestX509CertCount < kMaxCertCount, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

                err = reader.GetDataPtr(const_cast<const uint8_t *&>(MfrAttestX509CertSet[MfrAttestX509CertCount].Cert));
                SuccessOrExit(err);

                MfrAttestX509CertSet[MfrAttestX509CertCount++].Len = reader.GetLength();

                err = reader.Next();
            }

            err = reader.ExitContainer(outerContainer2);
            SuccessOrExit(err);

            mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

            err = reader.Next();
            SuccessOrExit(err);
        }

        MfrAttestType(kMfrAttestType_X509Cert);
    }
    else if (reader.GetType() == kTLVType_UnsignedInteger && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttest_HMACKeyId))
    {
        err = reader.Get(MfrAttestHMACKeyId);
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next();
        SuccessOrExit(err);

        // Request authorization information - pairing init data (optional).
        if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttest_HMACMetaData))
        {
            err = reader.GetDataPtr(MfrAttestHMACMetaData);
            SuccessOrExit(err);

            MfrAttestHMACMetaDataLen = reader.GetLength();

            mTBSDataLen = MfrAttestHMACMetaData + MfrAttestHMACMetaDataLen - mTBSDataStart;

            err = reader.Next();
            SuccessOrExit(err);
        }

        MfrAttestType(kMfrAttestType_HMAC);
    }
    else
    {
        VerifyOrExit(!MfrAttestRequired(), err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Operational Device Signature.
    {
        VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
        VerifyOrExit(reader.GetTag() == ContextTag(kTag_GetCertReqMsg_OpDeviceSigAlgo), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        err = reader.Get(OperationalSigAlgo);
        SuccessOrExit(err);

        err = reader.Next(kTLVType_Structure, ContextTag(kTag_GetCertReqMsg_OpDeviceSig_ECDSA));
        SuccessOrExit(err);

        err = DecodeWeaveECDSASignature(reader, OperationalSig);
        SuccessOrExit(err);

        err = reader.Next();
    }

    // Manufacturer Attestation Signature (optional).
    if (MfrAttestPresent())
    {
        VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
        VerifyOrExit(reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttestSigAlgo), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        err = reader.Get(MfrAttestSigAlgo);
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        if (reader.GetType() == kTLVType_Structure && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttestSig_ECDSA))
        {
            VerifyOrExit(MfrAttestType() == kMfrAttestType_WeaveCert, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            err = DecodeWeaveECDSASignature(reader, MfrAttestSig.EC);
            SuccessOrExit(err);
        }
        else if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttestSig_RSA))
        {
            VerifyOrExit(MfrAttestType() == kMfrAttestType_X509Cert, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            err = MfrAttestSig.RSA.ReadSignature(reader);
            SuccessOrExit(err);
        }
        else if (reader.GetType() == kTLVType_ByteString && reader.GetTag() == ContextTag(kTag_GetCertReqMsg_MfrAttestSig_HMAC))
        {
            VerifyOrExit(MfrAttestType() == kMfrAttestType_HMAC, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

            err = MfrAttestSig.HMAC.ReadSignature(reader);
            SuccessOrExit(err);

            err = reader.Next();
        }
        else
        {
            // Any other manufacturer attestation types are not currently supported.
            ExitNow(err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        }
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GetCertificateRequestMessage::GenerateTBSHash(uint8_t *tbsHash)
{
    nl::Weave::Platform::Security::SHA256 sha256;

    sha256.Begin();
    sha256.AddData(mTBSDataStart, mTBSDataLen);
    sha256.Finish(tbsHash);

    return WEAVE_NO_ERROR;
}

MockWeaveOperationalCAServer::MockWeaveOperationalCAServer()
{
    mExchangeMgr = NULL;
    mLogMessageData = false;
    mIncludeRelatedCerts = false;
    mDoNotRotateCert = false;

    mCACert = nl::TestCerts::sTestCert_CA_Weave;
    mCACertLen = nl::TestCerts::sTestCertLength_CA_Weave;

    mCAPrivateKey = nl::TestCerts::sTestCert_CA_PrivateKey_Weave;
    mCAPrivateKeyLen = nl::TestCerts::sTestCertLength_CA_PrivateKey_Weave;
}

WEAVE_ERROR MockWeaveOperationalCAServer::Init(nl::Weave::WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    mExchangeMgr = exchangeMgr;

    // Register to receive unsolicited GetCertificateRequest messages from the exchange manager.
    err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Security, kMsgType_GetCertificateRequest, HandleClientRequest, this);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockWeaveOperationalCAServer::Shutdown()
{
    if (mExchangeMgr != NULL)
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Security, kMsgType_GetCertificateRequest);
    return WEAVE_NO_ERROR;
}

void MockWeaveOperationalCAServer::HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *pktInfo,
                                                       const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                       uint8_t msgType, PacketBuffer *reqMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockWeaveOperationalCAServer *server = static_cast<MockWeaveOperationalCAServer *>(ec->AppState);
    GetCertificateRequestMessage getCertMsg;
    PacketBuffer *respMsg = NULL;
    char ipAddrStr[64];
    ec->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    VerifyOrExit(profileId == kWeaveProfile_Security &&
                 msgType == kMsgType_GetCertificateRequest, err = WEAVE_ERROR_NO_MEMORY);

    printf("GetCertificate request received from node %" PRIX64 " (%s)\n", ec->PeerNodeId, ipAddrStr);

    err = server->ProcessGetCertificateRequest(reqMsg, getCertMsg);
    SuccessOrExit(err);

    if ((getCertMsg.RequestType() == WeaveCertProvEngine::kReqType_RotateOpDeviceCert) && server->mDoNotRotateCert)
    {
        err = server->SendStatusReport(ec, Security::kStatusCode_NoNewOperationalCertRequired);
        SuccessOrExit(err);
    }
    else
    {
        respMsg = PacketBuffer::New();
        VerifyOrExit(respMsg != NULL, err = WEAVE_ERROR_NO_MEMORY);

        err = server->GenerateGetCertificateResponse(respMsg, *getCertMsg.OperationalCertSet.Certs);
        SuccessOrExit(err);

        err = ec->SendMessage(kWeaveProfile_Security, kMsgType_GetCertificateResponse, respMsg, 0);
        respMsg = NULL;
        SuccessOrExit(err);
    }

exit:
    if (err != WEAVE_NO_ERROR)
        server->SendStatusReport(ec, Security::kStatusCode_AuthenticationFailed);

    if (reqMsg != NULL)
        PacketBuffer::Free(reqMsg);

    if (respMsg != NULL)
        PacketBuffer::Free(respMsg);
}

WEAVE_ERROR MockWeaveOperationalCAServer::SendStatusReport(nl::Weave::ExchangeContext *ec, uint16_t statusCode)
{
    WEAVE_ERROR err;
    PacketBuffer *statusMsg;
    StatusReporting::StatusReport statusReport;

    statusMsg = PacketBuffer::New();
    VerifyOrExit(statusMsg != NULL, err = WEAVE_ERROR_NO_MEMORY);

    statusReport.mProfileId = kWeaveProfile_Security;
    statusReport.mStatusCode = statusCode;

    err = statusReport.pack(statusMsg);
    SuccessOrExit(err);

    err = ec->SendMessage(kWeaveProfile_Common, Common::kMsgType_StatusReport, statusMsg, 0);
    statusMsg = NULL;
    SuccessOrExit(err);

exit:
    if (statusMsg != NULL)
        PacketBuffer::Free(statusMsg);

    return err;
}

WEAVE_ERROR MockWeaveOperationalCAServer::ProcessGetCertificateRequest(PacketBuffer *msgBuf, GetCertificateRequestMessage & msg)
{
    WEAVE_ERROR err;
    uint8_t tbsHash[SHA256::kHashLength];

    err = msg.Decode(msgBuf);
    SuccessOrExit(err);

    // Validate Manufacturer Attestation Information if present.
    if (msg.AuthorizeInfoPresent())
    {
        // TODO: Not Implemented.
        // For testing purposes can use DUMMY_PAIRING_TOKEN and DUMMY_INIT_DATA
        // defined in src/test-apps/happy/lib/WeaveDeviceManager.py
    }

    err = ValidateWeaveDeviceCert(msg.OperationalCertSet);
    SuccessOrExit(err);

    if (msg.MfrAttestRequired())
    {
        VerifyOrExit(msg.MfrAttestPresent(), err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Validate Manufacturer Attestation Information if present.
    if (msg.MfrAttestPresent())
    {
        if (msg.MfrAttestType() == kMfrAttestType_WeaveCert)
        {
            err = ValidateWeaveDeviceCert(msg.MfrAttestWeaveCertSet);
            SuccessOrExit(err);
        }
        else if (msg.MfrAttestType() == kMfrAttestType_X509Cert)
        {
            err = ValidateX509DeviceCert(msg.MfrAttestX509CertSet, msg.MfrAttestX509CertCount);
            SuccessOrExit(err);
        }
        else if (msg.MfrAttestType() == kMfrAttestType_HMAC)
        {
            // Currently HMAC Manufacturer Attestation Method is not supported by mock WOCA server.
            ExitNow(WEAVE_ERROR_NOT_IMPLEMENTED);
        }
        else
        {
            ExitNow(WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }

    err = msg.GenerateTBSHash(tbsHash);
    SuccessOrExit(err);

    // Only ECDSAWithSHA256 algorithm is allowed for operational signature.
    VerifyOrExit(msg.OperationalSigAlgo == kOID_SigAlgo_ECDSAWithSHA256, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify operational signature.
    err = VerifyECDSASignature(WeaveCurveIdToOID(msg.OperationalCertSet.Certs->PubKeyCurveId),
                               tbsHash, SHA256::kHashLength, msg.OperationalSig, msg.OperationalCertSet.Certs->PublicKey.EC);
    SuccessOrExit(err);

    // Verify Manufacturer Attestation Signature if present.
    if (msg.MfrAttestPresent())
    {
        if (msg.MfrAttestSigAlgo == kOID_SigAlgo_ECDSAWithSHA256)
        {
            err = VerifyECDSASignature(WeaveCurveIdToOID(msg.MfrAttestWeaveCertSet.Certs->PubKeyCurveId),
                                       tbsHash, SHA256::kHashLength, msg.MfrAttestSig.EC, msg.MfrAttestWeaveCertSet.Certs->PublicKey.EC);
            SuccessOrExit(err);
        }
        else if (msg.MfrAttestSigAlgo == kOID_SigAlgo_SHA256WithRSAEncryption)
        {
#if WEAVE_WITH_OPENSSL
            err = VerifyRSASignature(kOID_SigAlgo_SHA256WithRSAEncryption, tbsHash, SHA256::kHashLength, msg.MfrAttestSig.RSA,
                                     msg.MfrAttestX509CertSet[0].Cert, msg.MfrAttestX509CertSet[0].Len);
            SuccessOrExit(err);
#else
            ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
        }
        else if (msg.MfrAttestSigAlgo == kOID_SigAlgo_HMACWithSHA256)
        {
            // Currently HMAC Manufacturer Attestation Method is not supported by mock WOCA server.
            ExitNow(WEAVE_ERROR_NOT_IMPLEMENTED);
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);
        }
    }

exit:
    return err;
}

WEAVE_ERROR MockWeaveOperationalCAServer::GenerateGetCertificateResponse(PacketBuffer * msgBuf, WeaveCertificateData & receivedDeviceCertData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    TLVType containerType;
    uint8_t cert[nl::TestCerts::kTestCertBufSize];
    uint16_t certLen;

    err = GenerateTestDeviceCert(receivedDeviceCertData.SubjectDN.AttrValue.WeaveId, receivedDeviceCertData.PublicKey.EC,
                                 mCACert, mCACertLen,
                                 mCAPrivateKey, mCAPrivateKeyLen,
                                 cert, sizeof(cert), certLen);
    SuccessOrExit(err);

    writer.Init(msgBuf);

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = writer.CopyContainer(ContextTag(kTag_GetCertRespMsg_OpDeviceCert), cert, certLen);
    SuccessOrExit(err);

    if (mIncludeRelatedCerts)
    {
        TLVType containerType2;

        // Start the RelatedCertificates array. This contains the list of certificates the signature verifier
        // will need to verify the signature.
        err = writer.StartContainer(ContextTag(kTag_GetCertRespMsg_OpRelatedCerts), kTLVType_Array, containerType2);
        SuccessOrExit(err);

        // Copy the intermediate test device CA certificate.
        err = writer.CopyContainer(AnonymousTag, mCACert, mCACertLen);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

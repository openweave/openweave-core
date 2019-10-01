/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include "MockCAService.h"
#include "TestWeaveCertData.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security::CertProvisioning;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;

GetCertificateRequestMessage::GetCertificateRequestMessage()
{
    ReqType = kReqType_NotSpecified;

    OperationalSig.R = NULL;
    OperationalSig.S = NULL;
    OperationalSig.RLen = 0;
    OperationalSig.SLen = 0;

    ManufAttestSig.R = NULL;
    ManufAttestSig.S = NULL;
    ManufAttestSig.RLen = 0;
    ManufAttestSig.SLen = 0;

    mAuthInfoPresent = false;
    mOperationalCertSetInitialized = false;
    mManufAttestCertSetInitialized = false;

    mTBSDataStart = NULL;
    mTBSDataLen = 0;
}

WEAVE_ERROR GetCertificateRequestMessage::Decode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType outerContainer;

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

        err = reader.Get(ReqType);
        SuccessOrExit(err);

        VerifyOrExit(ReqType == kReqType_GetInitialOpDeviceCert ||
                     ReqType == kReqType_RotateCert, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Authorization Information (optional).
    if (reader.GetTag() == ProfileTag(kWeaveProfile_Security, kTag_GetCertAuthorizeInfo))
    {
        mAuthInfoPresent = true;

        // Ignore authorization information data in the current implementation of the Mock CA Service.
        err = reader.Skip();
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Operational Device Certificate.
    {
        WeaveCertificateData *certData;

        VerifyOrExit(reader.GetTag() == ContextTag(kTag_GetCertReqMsg_OpDeviceCert), err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = mOperationalCertSet.Init(kMaxCerts, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        mOperationalCertSetInitialized = true;

        err = mOperationalCertSet.LoadCert(reader, kDecodeFlag_GenerateTBSHash, certData);
        SuccessOrExit(err);

        err = reader.Skip();
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next();
        SuccessOrExit(err);
    }

    // Manufacturer Attestation Information (optional).
    if (reader.GetTag() == ProfileTag(kWeaveProfile_Security, kTag_ManufAttestInfo_Weave))
    {
        WeaveCertificateData *certData;
        TLVType outerContainer2;

        err = reader.EnterContainer(outerContainer2);
        SuccessOrExit(err);

        err = reader.Next(kTLVType_Structure, ContextTag(kTag_ManufAttestInfo_Weave_DeviceCert));
        SuccessOrExit(err);

        err = mManufAttestCertSet.Init(kMaxCerts, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        mManufAttestCertSetInitialized = true;

        // Load manufacturer attestation Weave certificate.
        err = mManufAttestCertSet.LoadCert(reader, kDecodeFlag_GenerateTBSHash, certData);
        SuccessOrExit(err);

        err = reader.Next(kTLVType_Array, ContextTag(kTag_ManufAttestInfo_Weave_RelatedCerts));

        if (err == WEAVE_NO_ERROR)
        {
            // Load intermediate certificate.
            err = mManufAttestCertSet.LoadCerts(reader, kDecodeFlag_GenerateTBSHash);
            SuccessOrExit(err);
        }

        err = reader.VerifyEndOfContainer();
        SuccessOrExit(err);

        err = reader.ExitContainer(outerContainer2);
        SuccessOrExit(err);

        mTBSDataLen = reader.GetReadPoint() - mTBSDataStart;

        err = reader.Next();
        SuccessOrExit(err);
    }
    else if (reader.GetTag() == ProfileTag(kWeaveProfile_Security, kTag_ManufAttestInfo_X509))
    {
        // TODO: Implement
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
    }
    else
    {
        VerifyOrExit(!ManufAttestRequired(), err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Operational Device Signature.
    {
        VerifyOrExit(reader.GetTag() == ContextTag(kTag_GetCertReqMsg_OpDeviceSig_ECDSA), err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = DecodeWeaveECDSASignature(reader, OperationalSig);
        SuccessOrExit(err);

        err = reader.Next();
    }

    // Manufacturer Attestation Signature (optional).
    if (reader.GetTag() == ContextTag(kTag_GetCertReqMsg_ManufAttestSig_ECDSA))
    {
        err = DecodeWeaveECDSASignature(reader, ManufAttestSig);
        SuccessOrExit(err);
    }
    else if (reader.GetTag() == ContextTag(kTag_GetCertReqMsg_ManufAttestSig_RSA))
    {
        // TODO: Implement
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
    }
    else
    {
        VerifyOrExit(!ManufAttestRequired(), err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

//WEAVE_ERROR ValidateWeaveDeviceCert(WeaveCertificateData & cert)
WEAVE_ERROR ValidateWeaveDeviceCert(WeaveCertificateSet & certSet)
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

WEAVE_ERROR GetCertificateRequestMessage::GenerateTBSHash(uint8_t *tbsHash)
{
    nl::Weave::Platform::Security::SHA256 sha256;

    sha256.Begin();
    sha256.AddData(mTBSDataStart, mTBSDataLen);
    sha256.Finish(tbsHash);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockCAService::GenerateServiceAssignedDeviceCert(WeaveCertificateData& certData, uint8_t *cert, uint16_t certBufSize, uint16_t& certLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType containerType;
    TLVType containerType2;
    TLVType containerType3;

    // Update certificate fields for the service assigned certificate.
    certData.IssuerDN.AttrValue.WeaveId = nl::TestCerts::sTestCert_CA_Id;
    certData.AuthKeyId.Id               = nl::TestCerts::sTestCert_CA_SubjectKeyId;
    certData.AuthKeyId.Len              = nl::TestCerts::sTestCertLength_CA_SubjectKeyId;

    // Set Test Device Certification Authority (CA) private key.
    EncodedECPrivateKey caPrivKey = {
        .PrivKey                        = const_cast<uint8_t *>(nl::TestCerts::sTestCert_CA_PrivateKey),
        .PrivKeyLen                     = static_cast<uint16_t>(nl::TestCerts::sTestCertLength_CA_PrivateKey)
    };
    OID caCurveOID                      = WeaveCurveIdToOID(nl::TestCerts::sTestCert_CA_CurveId);

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
    err = writer.Put(ContextTag(kTag_SignatureAlgorithm), static_cast<uint8_t>(certData.SigAlgoOID & ~kOIDCategory_Mask));
    SuccessOrExit(err);

    // Certificate issuer Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Issuer), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), certData.IssuerDN.AttrValue.WeaveId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate validity times.
    {
        uint32_t packedNotBeforeTime;
        uint32_t packedNotAfterTime;
        ASN1UniversalTime validTime = {
            .Year   = 2019,
            .Month  = 8,
            .Day    = 1,
            .Hour   = 14,
            .Minute = 11,
            .Second = 54
        };

        err = PackCertTime(validTime, packedNotBeforeTime);
        SuccessOrExit(err);

        validTime.Year += 10;

        err = PackCertTime(validTime, packedNotAfterTime);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_NotBefore), packedNotBeforeTime);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_NotAfter), packedNotAfterTime);
        SuccessOrExit(err);
    }

    // Certificate subject Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Subject), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), certData.SubjectDN.AttrValue.WeaveId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // EC public key algorithm.
    err = writer.Put(ContextTag(kTag_PublicKeyAlgorithm), static_cast<uint8_t>(certData.PubKeyAlgoOID & kOID_Mask));
    SuccessOrExit(err);

    // EC public key curve Id.
    err = writer.Put(ContextTag(kTag_EllipticCurveIdentifier), static_cast<uint32_t>(certData.PubKeyCurveId));
    SuccessOrExit(err);

    // EC public key.
    err = writer.PutBytes(ContextTag(kTag_EllipticCurvePublicKey), certData.PublicKey.EC.ECPoint, certData.PublicKey.EC.ECPointLen);
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
        err = writer.StartContainer(ContextTag(kTag_SubjectKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_SubjectKeyIdentifier_KeyIdentifier), certData.SubjectKeyId.Id, certData.SubjectKeyId.Len);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: authority key identifier.
    {
        err = writer.StartContainer(ContextTag(kTag_AuthorityKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_AuthorityKeyIdentifier_KeyIdentifier), certData.AuthKeyId.Id, certData.AuthKeyId.Len);
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
        uint8_t certDecodeBuf[kCertDecodeBufferSize];
        TLVType readContainerType;
        WeaveCertificateData certData2;

        reader.Init(cert, certBufSize);

        // Parse the beginning of the WeaveSignature structure.
        err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate));
        SuccessOrExit(err);

        // Enter the certificate structure.
        err = reader.EnterContainer(readContainerType);
        SuccessOrExit(err);

        // Initialize an ASN1Writer and convert the TBS (to-be-signed) portion of
        // the certificate to ASN.1 DER encoding.
        tbsWriter.Init(certDecodeBuf, kCertDecodeBufferSize);
        err = DecodeConvertTBSCert(reader, tbsWriter, certData2);
        SuccessOrExit(err);

        // Finish writing the ASN.1 DER encoding of the TBS certificate.
        err = tbsWriter.Finalize();
        SuccessOrExit(err);

        // Generate a SHA hash of the encoded TBS certificate.
        nl::Weave::Platform::Security::SHA256 sha256;
        sha256.Begin();
        sha256.AddData(certDecodeBuf, tbsWriter.GetLengthWritten());
        sha256.Finish(certData.TBSHash);

        // Reuse already allocated decode buffer to hold the generated signature value.
        EncodedECDSASignature ecdsaSig;
        ecdsaSig.R = certDecodeBuf;
        ecdsaSig.RLen = EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.S = certDecodeBuf + EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.SLen = EncodedECDSASignature::kMaxValueLength;

        // Generate an ECDSA signature for the given message hash.
        err = nl::Weave::Crypto::GenerateECDSASignature(caCurveOID, certData.TBSHash, nl::Weave::Platform::Security::SHA256::kHashLength, caPrivKey, ecdsaSig);
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
    return err;
}


MockCAService::MockCAService()
{
    mExchangeMgr = NULL;
    mLogMessageData = false;
    mIncludeIntermediateCert = false;
}

WEAVE_ERROR MockCAService::Init(nl::Weave::WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    mExchangeMgr = exchangeMgr;

    // Register to receive unsolicited GetCertificateRequest messages from the exchange manager.
    err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Security, kMsgType_GetCertificateRequest, HandleClientRequest, this);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCAService::Shutdown()
{
    if (mExchangeMgr != NULL)
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Security, kMsgType_GetCertificateRequest);
    return WEAVE_NO_ERROR;
}

void MockCAService::HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *pktInfo,
                                        const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                        uint8_t msgType, PacketBuffer *reqMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockCAService *caService = static_cast<MockCAService *>(ec->AppState);
    GetCertificateRequestMessage getCertMsg;
    PacketBuffer *respMsg = NULL;
    char ipAddrStr[64];
    ec->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    VerifyOrExit(profileId == kWeaveProfile_Security &&
                 msgType == kMsgType_GetCertificateRequest, err = WEAVE_ERROR_NO_MEMORY);

    printf("GetCertificate request received from node %" PRIX64 " (%s)\n", ec->PeerNodeId, ipAddrStr);

    err = caService->ProcessGetCertificateRequest(reqMsg, getCertMsg);
    SuccessOrExit(err);

    {
        respMsg = PacketBuffer::New();
        VerifyOrExit(respMsg != NULL, err = WEAVE_ERROR_NO_MEMORY);

        err = caService->GenerateGetCertificateResponse(respMsg, *getCertMsg.mOperationalCertSet.Certs);
        SuccessOrExit(err);

        err = ec->SendMessage(kWeaveProfile_Security, kMsgType_GetCertificateResponse, respMsg, 0);
        respMsg = NULL;
        SuccessOrExit(err);
    }

exit:
    if (err != WEAVE_NO_ERROR)
        caService->SendStatusReport(ec);

    if (reqMsg != NULL)
        PacketBuffer::Free(reqMsg);

    if (respMsg != NULL)
        PacketBuffer::Free(respMsg);
}

WEAVE_ERROR MockCAService::SendStatusReport(nl::Weave::ExchangeContext *ec)
{
    WEAVE_ERROR err;
    PacketBuffer *statusMsg;
    StatusReporting::StatusReport statusReport;

    statusMsg = PacketBuffer::New();
    VerifyOrExit(statusMsg != NULL, err = WEAVE_ERROR_NO_MEMORY);

    statusReport.mProfileId = kWeaveProfile_Common;
    statusReport.mStatusCode = Common::kStatus_BadRequest;

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

WEAVE_ERROR MockCAService::ProcessGetCertificateRequest(PacketBuffer *msgBuf, GetCertificateRequestMessage & msg)
{
    WEAVE_ERROR err;
    uint8_t tbsHash[SHA256::kHashLength];

    err = msg.Decode(msgBuf);
    SuccessOrExit(err);

    // err = ValidateWeaveDeviceCert(*msg.mOperationalCertSet.Certs);
    err = ValidateWeaveDeviceCert(msg.mOperationalCertSet);
    SuccessOrExit(err);

    if (msg.ManufAttestRequired())
    {
        // err = ValidateWeaveDeviceCert(*msg.mManufAttestCertSet.Certs);
        err = ValidateWeaveDeviceCert(msg.mManufAttestCertSet);
        SuccessOrExit(err);
    }

    err = msg.GenerateTBSHash(tbsHash);
    SuccessOrExit(err);

    err = VerifyECDSASignature(WeaveCurveIdToOID(msg.mOperationalCertSet.Certs->PubKeyCurveId),
                               tbsHash, SHA256::kHashLength, msg.OperationalSig, msg.mOperationalCertSet.Certs->PublicKey.EC);
    SuccessOrExit(err);

    if (msg.ManufAttestRequired())
    {
        err = VerifyECDSASignature(WeaveCurveIdToOID(msg.mManufAttestCertSet.Certs->PubKeyCurveId),
                                   tbsHash, SHA256::kHashLength, msg.ManufAttestSig, msg.mManufAttestCertSet.Certs->PublicKey.EC);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR MockCAService::GenerateGetCertificateResponse(PacketBuffer *msgBuf, WeaveCertificateData& currentOpDeviceCert)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    TLVType containerType;
    uint8_t certBuf[nl::TestCerts::kTestCertBufSize];
    uint16_t certLen;

    err = GenerateServiceAssignedDeviceCert(currentOpDeviceCert, certBuf, sizeof(certBuf), certLen);
    SuccessOrExit(err);

    writer.Init(msgBuf);

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = writer.CopyContainer(ContextTag(kTag_GetCertRespMsg_OpDeviceCert), certBuf, certLen);
    SuccessOrExit(err);

    if (mIncludeIntermediateCert)
    {
        TLVType containerType2;

        // Start the RelatedCertificates array. This contains the list of certificates the signature verifier
        // will need to verify the signature.
        err = writer.StartContainer(ContextTag(kTag_GetCertRespMsg_RelatedCerts), kTLVType_Array, containerType2);
        SuccessOrExit(err);

        // Copy the intermediate test device CA certificate.
        err = writer.CopyContainer(AnonymousTag, nl::TestCerts::sTestCert_CA_Weave, nl::TestCerts::sTestCertLength_CA_Weave);
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

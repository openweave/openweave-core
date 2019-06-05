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
 *      Implementation of CertProvOptions object, which provides an implementation of the
 *      WeaveNodeOpAuthDelegate and WeaveNodeManufAttestDelegate interfaces for use
 *      in test applications.
 *
 */

#include "stdio.h"

#include "ToolCommon.h"
#include "TestWeaveCertData.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include "CertProvOptions.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;

const uint8_t TestDevice1_PairingToken[] =
{
    /*
    -----BEGIN PAIRING TOKEN-----
    1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C
    NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ
    TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB
    GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq
    dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/
    fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz
    NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY
    -----END PAIRING TOKEN-----
    */
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00, 0x35, 0x01, 0x30, 0x01, 0x08, 0x4e, 0x2f, 0x32, 0x4b,
    0x41, 0xd7, 0x3a, 0xdb, 0x24, 0x02, 0x04, 0x37, 0x03, 0x2c, 0x81, 0x10, 0x44, 0x55, 0x4d, 0x4d,
    0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18, 0x26, 0x04, 0xcb,
    0xa8, 0xfa, 0x1b, 0x26, 0x05, 0x4b, 0x35, 0x4f, 0x42, 0x37, 0x06, 0x2c, 0x81, 0x10, 0x44, 0x55,
    0x4d, 0x4d, 0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18, 0x24,
    0x07, 0x02, 0x26, 0x08, 0x25, 0x00, 0x5a, 0x23, 0x30, 0x0a, 0x39, 0x04, 0x2b, 0xd9, 0xdb, 0x5a,
    0x62, 0xef, 0xba, 0xb1, 0x53, 0x2a, 0x0f, 0x99, 0x63, 0xb7, 0x8a, 0x30, 0xc5, 0x8a, 0x41, 0x29,
    0xa5, 0x19, 0x4e, 0x4b, 0x0b, 0xf3, 0x7e, 0xda, 0xc5, 0xe9, 0xb3, 0x35, 0xf0, 0x75, 0x18, 0x6d,
    0x49, 0x5d, 0x86, 0xc4, 0x44, 0x25, 0x07, 0x41, 0xb4, 0xd3, 0xa9, 0xef, 0xee, 0xb4, 0x2a, 0xd6,
    0x0a, 0x5d, 0x9d, 0xe0, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x05,
    0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x18, 0x18, 0x35, 0x81, 0x30,
    0x02, 0x08, 0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08,
    0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1d, 0x00, 0x8a,
    0x61, 0x86, 0x62, 0x3d, 0x17, 0xb2, 0xd2, 0xcf, 0xd2, 0x6d, 0x39, 0x3d, 0xe4, 0x25, 0x69, 0xe0,
    0x91, 0xea, 0x05, 0x6a, 0x75, 0xce, 0xdd, 0x45, 0xeb, 0x83, 0xcf, 0x30, 0x02, 0x1c, 0x74, 0xb4,
    0x2b, 0xa4, 0x6d, 0x14, 0x65, 0xb7, 0xb7, 0x71, 0x9a, 0x5a, 0xaf, 0x64, 0xd2, 0x88, 0x60, 0x6e,
    0xb3, 0xb1, 0xa0, 0x31, 0xca, 0x92, 0x6f, 0xca, 0xf2, 0x43, 0x18, 0x18, 0x35, 0x02, 0x26, 0x01,
    0x25, 0x00, 0x5a, 0x23, 0x30, 0x02, 0x1c, 0xba, 0xfc, 0xe3, 0x4f, 0x86, 0xbf, 0x7c, 0x38, 0x12,
    0x70, 0x2b, 0xf9, 0xa3, 0x10, 0xbf, 0x4f, 0x9b, 0xf3, 0xef, 0x30, 0x0f, 0xa5, 0x44, 0x4d, 0x42,
    0x3a, 0x67, 0x6a, 0x30, 0x03, 0x39, 0x04, 0x2b, 0xd9, 0xdb, 0x5a, 0x62, 0xef, 0xba, 0xb1, 0x53,
    0x2a, 0x0f, 0x99, 0x63, 0xb7, 0x8a, 0x30, 0xc5, 0x8a, 0x41, 0x29, 0xa5, 0x19, 0x4e, 0x4b, 0x0b,
    0xf3, 0x7e, 0xda, 0xc5, 0xe9, 0xb3, 0x35, 0xf0, 0x75, 0x18, 0x6d, 0x49, 0x5d, 0x86, 0xc4, 0x44,
    0x25, 0x07, 0x41, 0xb4, 0xd3, 0xa9, 0xef, 0xee, 0xb4, 0x2a, 0xd6, 0x0a, 0x5d, 0x9d, 0xe0, 0x18,
    0x18,
};

const uint16_t TestDevice1_PairingTokenLength = sizeof(TestDevice1_PairingToken);

const uint8_t TestDevice1_PairingInitData[] =
{
    0x6E, 0x3C, 0x71, 0x5B, 0xE0, 0x19, 0xD4, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01,
    0x24, 0x02, 0x05, 0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01,
};

const uint16_t TestDevice1_PairingInitDataLength = sizeof(TestDevice1_PairingInitData);

const uint32_t TestDevice1_ManufAttest_HMACKeyId = 0xCAFECAFE;

const uint8_t TestDevice1_ManufAttest_HMACMetaData[] =
{
    0x2a, 0xd6, 0x0a, 0x29, 0x01, 0x6E, 0x71, 0x29, 0x01, 0x18, 0x35
};

const uint16_t TestDevice1_ManufAttest_HMACMetaDataLength = sizeof(TestDevice1_ManufAttest_HMACMetaData);

const uint8_t TestDevice1_ManufAttest_HMACKey[] =
{
    0xd9, 0xdb, 0x5a, 0x62, 0xE0, 0x19, 0xD4, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01,
    0x24, 0x02, 0x05, 0x18, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x29, 0x01, 0x0b, 0xf3, 0xa0, 0x31
};

const uint16_t TestDevice1_ManufAttest_HMACKeyLength = sizeof(TestDevice1_ManufAttest_HMACKey);

CertProvOptions gCertProvOptions;

enum
{
    kMaxCerts                         = 4,      // Maximum number of certificates in the certificate verification chain.
    kCertDecodeBufSize                = 1024,   // Size of buffer needed to hold any of the test certificates
                                                // (in either Weave or DER form), or to decode the certificates.
};

static uint8_t sDeviceOperationalCert[kCertDecodeBufSize];
static uint16_t sDeviceOperationalCertLength = 0;

static uint8_t sDeviceOperationalRelatedCerts[kCertDecodeBufSize];
static uint16_t sDeviceOperationalRelatedCertsLength = 0;

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

/**
 *  Handler for Certificate Provisioning Client API events.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the client object.
 *  @param[in]  eventType   Event ID passed by the event callback.
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback.
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback.
 *
 */
void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;
    WeaveCertProvEngine *client = inParam.Source;
    Binding *binding = client->GetBinding();
    uint64_t peerNodeId;
    IPAddress peerAddr;
    uint16_t peerPort;
    InterfaceId peerInterfaceId;

    if (binding != NULL)
    {
        peerNodeId = binding->GetPeerNodeId();
        binding->GetPeerIPAddress(peerAddr, peerPort, peerInterfaceId);
    }

    switch (eventType)
    {
    case WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo:
    {
        if (binding != NULL)
            WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo; to node %" PRIX64 " (%s)", peerNodeId, peerAddr);
        else
            WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo");

        TLVWriter * writer = inParam.PrepareAuthorizeInfo.Writer;

        if (gCertProvOptions.IncludeAuthorizeInfo)
        {
            // Pairing Token.
            err = writer->PutBytes(ContextTag(kTag_GetCertReqMsg_Authorize_PairingToken), gCertProvOptions.PairingToken, gCertProvOptions.PairingTokenLength);
            SuccessOrExit(err);

            // Pairing Initialization Data.
            err = writer->PutBytes(ContextTag(kTag_GetCertReqMsg_Authorize_PairingInitData), gCertProvOptions.PairingInitData, gCertProvOptions.PairingInitDataLength);
            SuccessOrExit(err);
        }
        break;
    }

    case WeaveCertProvEngine::kEvent_ResponseReceived:
    {
        if (inParam.ResponseReceived.ReplaceCert)
        {
            if (binding != NULL)
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; from node %" PRIX64 " (%s)", peerNodeId, peerAddr);
            else
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived");

            WeaveCertificateData *certData;
            const uint8_t * cert = inParam.ResponseReceived.Cert;
            uint16_t certLen = inParam.ResponseReceived.CertLen;
            const uint8_t * relatedCerts = inParam.ResponseReceived.RelatedCerts;
            uint16_t relatedCertsLen = inParam.ResponseReceived.RelatedCertsLen;

            err = certSet.Init(kMaxCerts, kCertDecodeBufSize);
            SuccessOrExit(err);

            certSetInitialized = true;

            // Load service assigned operational certificate.
            // Even when callback function doesn't do certificate validation this step is recommended
            // to make sure that message wasn't corrupted in transmission.
            err = certSet.LoadCert(cert, certLen, kDecodeFlag_GenerateTBSHash, certData);
            SuccessOrExit(err);

            if (relatedCerts != NULL)
            {
                // Load intermediate certificate.
                // Even when callback function doesn't do certificate validation this step is recommended
                // to make sure that message wasn't corrupted in transmission.
                err = certSet.LoadCerts(relatedCerts, relatedCertsLen, kDecodeFlag_GenerateTBSHash);
                SuccessOrExit(err);
            }

            // This certificate validation step is added for testing purposes only.
            // In reality, device doesn't have to validate certificate issued by the CA service.
            err = ValidateWeaveDeviceCert(certSet);
            SuccessOrExit(err);

            // Store service issued operational device certificate.
            VerifyOrExit(certLen <= sizeof(sDeviceOperationalCert), err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            memcpy(sDeviceOperationalCert, cert, certLen);
            sDeviceOperationalCertLength = certLen;

            // Store device intermediate certificates related to the device certificate.
            VerifyOrExit(relatedCertsLen <= sizeof(sDeviceOperationalRelatedCerts), err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            memcpy(sDeviceOperationalRelatedCerts, relatedCerts, relatedCertsLen);
            sDeviceOperationalRelatedCertsLength = relatedCertsLen;
        }
        else
        {
            if (binding != NULL)
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; received status report from node %" PRIX64 " (%s): "
                               "No Need to Replace Operational Device Certificate", peerNodeId, peerAddr);
            else
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; received status report: No Need to Replace Operational Device Certificate");
        }

        client->AbortCertificateProvisioning();

        // WaitingForGetCertResponse = false;
        // GetCertResponseCount++;
        // LastGetCertTime = Now();

        break;
    }

    case WeaveCertProvEngine::kEvent_CommunicationError:
    {
        if (inParam.CommunicationError.Reason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            if (binding != NULL)
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError; received status report from node %" PRIX64 " (%s): %s", peerNodeId, peerAddr,
                              nl::StatusReportStr(inParam.CommunicationError.RcvdStatusReport->mProfileId, inParam.CommunicationError.RcvdStatusReport->mStatusCode));
            else
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError; received status report: %s",
                              nl::StatusReportStr(inParam.CommunicationError.RcvdStatusReport->mProfileId, inParam.CommunicationError.RcvdStatusReport->mStatusCode));
        }
        else
        {
            if (binding != NULL)
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError with node %" PRIX64 " (%s): %s",
                              peerNodeId, peerAddr, ErrorStr(inParam.CommunicationError.Reason));
            else
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError: %s",
                              ErrorStr(inParam.CommunicationError.Reason));
        }

        client->AbortCertificateProvisioning();
        // WaitingForGetCertResponse = false;

        break;
    }

    default:
        if (binding != NULL)
            WeaveLogError(SecurityManager, "WeaveCertProvEngine: unrecognized API event with node %" PRIX64 " (%s)", peerNodeId, peerAddr);
        else
            WeaveLogError(SecurityManager, "WeaveCertProvEngine: unrecognized API event");
        break;
    }

exit:
    if (eventType == WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo)
        outParam.PrepareAuthorizeInfo.Error = err;
    else if (eventType == WeaveCertProvEngine::kEvent_ResponseReceived)
        outParam.ResponseReceived.Error = err;

    if (certSetInitialized)
        certSet.Release();
}

bool ParseGetCertReqType(const char *str, uint8_t& output)
{
    int reqType;

    if (!ParseInt(str, reqType))
        return false;

    switch (reqType)
    {
    case 1:
        output = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert;
        return true;
    case 2:
        output = WeaveCertProvEngine::kReqType_RotateOpDeviceCert;
        return true;
    default:
        return false;
    }
}

CertProvOptions::CertProvOptions()
{
    static OptionDef optionDefs[] =
    {
        { "get-cert-req-type",      kArgumentRequired,      kToolCommonOpt_GetCertReqType        },
        { "pairing-token",          kArgumentRequired,      kToolCommonOpt_PairingToken          },
        { "send-auth-info",         kNoArgument,            kToolCommonOpt_SendAuthorizeInfo     },
        { "op-cert",                kArgumentRequired,      kToolCommonOpt_OpCert                },
        { "op-key",                 kArgumentRequired,      kToolCommonOpt_OpKey                 },
        { "op-ca-cert",             kArgumentRequired,      kToolCommonOpt_OpCACert              },
        { "send-op-ca-cert",        kNoArgument,            kToolCommonOpt_SendOpCACert          },
        { "ma-type",                kArgumentRequired,      kToolCommonOpt_ManufAttestType       },
        { "ma-cert",                kArgumentRequired,      kToolCommonOpt_ManufAttestCert       },
        { "ma-key",                 kArgumentRequired,      kToolCommonOpt_ManufAttestKey        },
        { "ma-ca-cert",             kArgumentRequired,      kToolCommonOpt_ManufAttestCACert     },
        { "ma-ca-cert2",            kArgumentRequired,      kToolCommonOpt_ManufAttestCACert2    },
        { "send-ma-ca-cert",        kNoArgument,            kToolCommonOpt_SendManufAttestCACert },
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "CERTIFICATE PROVISIONING OPTIONS";

    OptionHelp =
        "  --get-cert-req-type <int>\n"
        "       Get Certificate Request type. If not specified the default value is used.\n"
        "       Valid values are:\n"
        "           1 - get initial operational certificate (default).\n"
        "           2 - rotate operational certificate.\n"
        "\n"
        "  --pairing-token <pairing-token-file>\n"
        "       File containing a Weave Pairing Token to be used to authorize the certificate\n"
        "       provisioning request. If not specified the default test pairing token is used.\n"
        "\n"
        "  --send-auth-info\n"
        "       Do not send an intermediate certificate when establishing a CASE session.\n"
        "\n"
        "  --op-cert <cert-file>\n"
        "       File containing a Weave Operational certificate to be used to authenticate the node\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64. If not specified the default test certificate is used.\n"
        "\n"
        "  --op-key <key-file>\n"
        "       File containing an Operational private key to be used to authenticate the node's\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64. If not specified the default test key is used.\n"
        "\n"
        "  --op-ca-cert <cert-file>\n"
        "       File containing a Weave Operational CA certificate to be included along with the\n"
        "       node's Operational certificate in the Get Certificat Request message. The file can contain\n"
        "       either raw TLV or base-64. If not specified the default test CA certificate is used.\n"
        "\n"
        "  --send-op-ca-cert\n"
        "       Include a Weave Operational CA certificate in the Get Certificat Request message.\n"
        "       This option is set automatically when op-ca-cert is specified.\n"
        "\n"
        "  --ma-type <int>\n"
        "       Accept the specified set of key export configurations when either initiating or\n"
        "       responding to a key export request.\n"
        "\n"
        "  --ma-cert <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation certificate to be used to authenticate\n"
        "       the node's manufacturer. The file can contain either raw TLV or base-64. If not\n"
        "       specified the default test certificate is used.\n"
        "\n"
        "  --ma-key <key-file>\n"
        "       File containing a Manufacturer Attestation private key to be used to authenticate\n"
        "       the node's manufacturer. The file can contain either raw TLV orbase-64. If not\n"
        "       specified the default test key is used.\n"
        "\n"
        "  --ma-ca-cert <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation CA certificate to be included along\n"
        "       with the node's Manufacturer Attestation certificate in the Get Certificat Request\n"
        "       message. The file can contain either raw TLV or base-64. If not specified the default\n"
        "       test CA certificate is used.\n"
        "\n"
        "  --ma-ca-cert2 <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation second CA certificate to be included along\n"
        "       with the node's Manufacturer Attestation certificate in the Get Certificat Request\n"
        "       message. The file can contain either raw TLV or base-64. If not specified the default\n"
        "       test CA certificate is used.\n"
        "\n"
        "  --send-ma-ca-cert\n"
        "       Include a Weave Manufacturer Attestation CA certificate in the Get Certificat Request message.\n"
        "       This option is set automatically when ma-ca-cert is specified.\n"
      "";

    // Defaults
    RequestType = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert;
    IncludeAuthorizeInfo = false;
    PairingToken = TestDevice1_PairingToken;
    PairingTokenLength = TestDevice1_PairingTokenLength;
    PairingInitData = TestDevice1_PairingInitData;
    PairingInitDataLength = TestDevice1_PairingInitDataLength;
    OperationalCert = NULL;
    OperationalCertLength = 0;
    OperationalPrivateKey = NULL;
    OperationalPrivateKeyLength = 0;
    IncludeOperationalCACerts = false;
    OperationalCACert = NULL;
    OperationalCACertLength = 0;
    ManufAttestType = kManufAttestType_WeaveCert;
    ManufAttestCert = NULL;
    ManufAttestCertLength = 0;
    ManufAttestPrivateKey = NULL;
    ManufAttestPrivateKeyLength = 0;
    IncludeManufAttestCACerts = false;
    ManufAttestCACert = NULL;
    ManufAttestCACertLength = 0;
    ManufAttestCACert2 = NULL;
    ManufAttestCACert2Length = 0;
}

bool CertProvOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    uint32_t len;

    switch (id)
    {
    case kToolCommonOpt_GetCertReqType:
        if (!ParseGetCertReqType(arg, RequestType))
        {
            PrintArgError("%s: Invalid value specified for GetCert request type: %s\n", progName, arg);
            return false;
        }
        break;

    case kToolCommonOpt_PairingToken:
        PairingToken = ReadFileArg(arg, len);
        if (PairingToken == NULL)
            return false;
        PairingTokenLength = len;
        break;

    case kToolCommonOpt_PairingInitData:
        PairingInitData = ReadFileArg(arg, len);
        if (PairingInitData == NULL)
            return false;
        PairingInitDataLength = len;
        break;

    case kToolCommonOpt_SendAuthorizeInfo:
        IncludeAuthorizeInfo = true;
        break;

    case kToolCommonOpt_OpCert:
        if (!ReadCertFile(arg, (uint8_t *&)OperationalCert, OperationalCertLength))
            return false;
        break;

    case kToolCommonOpt_OpKey:
        if (!ReadPrivateKeyFile(arg, (uint8_t *&)OperationalPrivateKey, OperationalPrivateKeyLength))
            return false;
        break;

    case kToolCommonOpt_OpCACert:
        if (!ReadCertFile(arg, (uint8_t *&)OperationalCACert, OperationalCACertLength))
            return false;
        break;

   case kToolCommonOpt_SendOpCACert:
        IncludeOperationalCACerts = true;
        break;

    case kToolCommonOpt_ManufAttestCert:
        if (!ReadCertFile(arg, (uint8_t *&)ManufAttestCert, ManufAttestCertLength))
            return false;
        break;

    case kToolCommonOpt_ManufAttestKey:
        if (!ReadPrivateKeyFile(arg, (uint8_t *&)ManufAttestPrivateKey, ManufAttestPrivateKeyLength))
            return false;
        break;

    case kToolCommonOpt_ManufAttestCACert:
        if (!ReadCertFile(arg, (uint8_t *&)ManufAttestCACert, ManufAttestCACertLength))
            return false;
        break;

    case kToolCommonOpt_ManufAttestCACert2:
        if (!ReadCertFile(arg, (uint8_t *&)ManufAttestCACert2, ManufAttestCACert2Length))
            return false;
        break;

    case kToolCommonOpt_SendManufAttestCACert:
        IncludeManufAttestCACerts = true;
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

// ===== Methods that implement the WeaveNodeOpAuthDelegate interface

WEAVE_ERROR CertProvOptions::EncodeOpCert(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    const uint8_t * cert = OperationalCert;
    uint16_t certLen = OperationalCertLength;

    if (cert == NULL || certLen == 0)
    {
        bool res;

        if (RequestType == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert)
            res = GetTestOperationalSelfSignedNodeCert(EphemeralNodeId, cert, certLen);
        else
            res = GetTestOperationalServiceAssignedNodeCert(EphemeralNodeId, cert, certLen);

        if (!res)
        {
            printf("ERROR: Node Operational certificate not configured\n");
            ExitNow(err = WEAVE_ERROR_CERT_NOT_FOUND);
        }
    }

    err = writer.CopyContainer(tag, cert, certLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType containerType;
    const uint8_t * cert = OperationalCACert;
    uint16_t certLen = OperationalCACertLength;

    if (IncludeOperationalCACerts)
    {
        if (cert == NULL || certLen == 0)
        {
            cert = nl::NestCerts::Development::DeviceCA::Cert;
            certLen = nl::NestCerts::Development::DeviceCA::CertLength;
        }

        err = writer.StartContainer(tag, kTLVType_Array, containerType);
        SuccessOrExit(err);

        err = writer.CopyContainer(AnonymousTag, cert, certLen);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    const uint8_t * key = OperationalPrivateKey;
    uint16_t keyLen = OperationalPrivateKeyLength;

    if (key == NULL || keyLen == 0)
    {
        bool res;

        res = GetTestOperationalNodePrivateKey(EphemeralNodeId, key, keyLen);

        if (!res)
        {
            printf("ERROR: Node Operational private key not configured\n");
            ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);
        }
    }

    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, hash, hashLen, key, keyLen);
    SuccessOrExit(err);

exit:
    return err;
}

// ===== Methods that implement the WeaveNodeManufAttestDelegate interface

WEAVE_ERROR CertProvOptions::EncodeMAInfo(TLVWriter & writer)
{
    WEAVE_ERROR err;
    TLVType containerType;
    const uint8_t * cert = ManufAttestCert;
    uint16_t certLen = ManufAttestCertLength;
    const uint8_t * caCert = ManufAttestCACert;
    uint16_t caCertLen = ManufAttestCACertLength;
    const uint8_t * caCert2 = ManufAttestCACert2;
    uint16_t caCert2Len = ManufAttestCACert2Length;

    if (ManufAttestType == kManufAttestType_WeaveCert)
    {
        if (cert == NULL || certLen == 0)
        {
            if (!GetTestNodeManufAttestCert(EphemeralNodeId, cert, certLen))
            {
                printf("ERROR: Node Manufacturer Attestation certificate not configured\n");
                ExitNow(err = WEAVE_ERROR_CERT_NOT_FOUND);
            }
        }

        err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_ManufAttest_WeaveCert), cert, certLen);
        SuccessOrExit(err);

        if (IncludeManufAttestCACerts)
        {
            if (caCert == NULL || caCertLen == 0)
            {
                caCert = nl::NestCerts::Development::DeviceCA::Cert;
                caCertLen = nl::NestCerts::Development::DeviceCA::CertLength;
            }

            err = writer.StartContainer(ContextTag(kTag_GetCertReqMsg_ManufAttest_WeaveRelCerts), kTLVType_Array, containerType);
            SuccessOrExit(err);

            err = writer.CopyContainer(AnonymousTag, caCert, caCertLen);
            SuccessOrExit(err);

            err = writer.EndContainer(containerType);
            SuccessOrExit(err);
        }
    }
    else if (ManufAttestType == kManufAttestType_X509Cert)
    {
        if (cert == NULL || certLen == 0)
        {
            cert  = TestDevice1_X509_RSA_Cert;
            certLen  = TestDevice1_X509_RSA_CertLength;
        }

        // Copy the test device manufacturer attestation X509 RSA certificate into supplied TLV writer.
        err = writer.PutBytes(ContextTag(kTag_GetCertReqMsg_ManufAttest_X509Cert), cert, certLen);
        SuccessOrExit(err);

        if (IncludeManufAttestCACerts)
        {
            if (caCert == NULL || caCertLen == 0)
            {
                caCert = TestDevice1_X509_RSA_ICACert1;
                caCertLen = TestDevice1_X509_RSA_ICACert1Length;

                caCert2 = TestDevice1_X509_RSA_ICACert2;
                caCert2Len = TestDevice1_X509_RSA_ICACert2Length;
            }

            // Start the RelatedCertificates array. This contains the list of certificates the signature verifier
            // will need to verify the signature.
            err = writer.StartContainer(ContextTag(kTag_GetCertReqMsg_ManufAttest_X509RelCerts), kTLVType_Array, containerType);
            SuccessOrExit(err);

            // Copy first Intermidiate CA (ICA) certificate.
            err = writer.PutBytes(AnonymousTag, caCert, caCertLen);
            SuccessOrExit(err);

            // Copy second Intermidiate CA (ICA) certificate.
            if (caCert2 != NULL && caCert2Len > 0)
            {
                err = writer.PutBytes(AnonymousTag, caCert2, caCert2Len);
                SuccessOrExit(err);
            }

            err = writer.EndContainer(containerType);
            SuccessOrExit(err);
        }
    }
    else if (ManufAttestType == kManufAttestType_HMAC)
    {
        err = writer.Put(ContextTag(kTag_GetCertReqMsg_ManufAttest_HMACKeyId), TestDevice1_ManufAttest_HMACKeyId);
        SuccessOrExit(err);

        if (IncludeManufAttestCACerts)
        {
            err = writer.PutBytes(ContextTag(kTag_GetCertReqMsg_ManufAttest_HMACMetaData), TestDevice1_ManufAttest_HMACMetaData, TestDevice1_ManufAttest_HMACMetaDataLength);
            SuccessOrExit(err);
        }
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer)
{
    WEAVE_ERROR err;
    const uint8_t * key = ManufAttestPrivateKey;
    uint16_t keyLen = ManufAttestPrivateKeyLength;
    nl::Weave::Platform::Security::SHA256 sha256;
    uint8_t hash[SHA256::kHashLength];

    // Calculate data hash.
    if (ManufAttestType == kManufAttestType_WeaveCert || ManufAttestType == kManufAttestType_X509Cert)
    {
        sha256.Begin();
        sha256.AddData(data, dataLen);
        sha256.Finish(hash);
    }

    if (ManufAttestType == kManufAttestType_WeaveCert)
    {
        if (key == NULL || keyLen == 0)
        {
            if (!GetTestNodeManufAttestPrivateKey(EphemeralNodeId, key, keyLen))
            {
                printf("ERROR: Node Manufacturer Attestation private key not configured\n");
                ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);
            }
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_ManufAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_ECDSAWithSHA256));
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_ManufAttestSig_ECDSA), hash, SHA256::kHashLength, key, keyLen);
        SuccessOrExit(err);
    }
    else if (ManufAttestType == kManufAttestType_X509Cert)
    {
#if WEAVE_WITH_OPENSSL
        if (key == NULL || keyLen == 0)
        {
            key = TestDevice1_X509_RSA_PrivateKey;
            keyLen = TestDevice1_X509_RSA_PrivateKeyLength;
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_ManufAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_SHA256WithRSAEncryption));
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveRSASignature(ASN1::kOID_SigAlgo_SHA256WithRSAEncryption, writer, ContextTag(kTag_GetCertReqMsg_ManufAttestSig_RSA),
                                                 hash, SHA256::kHashLength, key, keyLen);
        SuccessOrExit(err);
#else
        printf("ERROR: Manufacturer Attestation X509 encoded certificates not supported.\n");
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }
    else if (ManufAttestType == kManufAttestType_HMAC)
    {
        if (key == NULL || keyLen == 0)
        {
            key = TestDevice1_ManufAttest_HMACKey;
            keyLen = TestDevice1_ManufAttest_HMACKeyLength;
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_ManufAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_HMACWithSHA256));
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveHMACSignature(ASN1::kOID_SigAlgo_HMACWithSHA256, writer, ContextTag(kTag_GetCertReqMsg_ManufAttestSig_HMAC),
                                                  data, dataLen, key, keyLen);
        SuccessOrExit(err);
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

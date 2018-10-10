/*
 *
 *    Copyright (c) 2017-2018 Nest Labs, Inc.
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
 *      This file implements unit tests for the Weave key export protocol.
 *
 */

#include <stdio.h>

#include <nltest.h>

#include "ToolCommon.h"
#include "TestGroupKeyStore.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveKeyExport.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ASN1.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Inet;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::KeyExport;
using namespace nl::Weave::Profiles::Security::AppKeys;
using namespace nl::Weave::ASN1;

using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;
using nl::Weave::System::PacketBuffer;

#define DEBUG_PRINT_ENABLE 0

static WEAVE_ERROR InitValidationContext(ValidationContext& validContext)
{
    WEAVE_ERROR err;
    ASN1UniversalTime validTime;

    // Arrange to validate the signature for code signing purposes.
    memset(&validContext, 0, sizeof(validContext));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;

    // Set the effective validation time.
    validTime.Year = 2017;
    validTime.Month = 01;
    validTime.Day = 31;
    validTime.Hour = validTime.Minute = validTime.Second = 0;
    err = PackCertTime(validTime, validContext.EffectiveTime);
    SuccessOrExit(err);

exit:
    return err;
}

class TestKeyExportDelegate : public WeaveKeyExportDelegate
{
private:
    enum
    {
        // Max Device Private Key Size -- Size of the temporary buffer used to hold
        // a device's TLV encoded private key.
        kMaxDevicePrivateKeySize = 300,

        // Max Validation Certs -- This controls the maximum number of certificates
        // that can be involved in the validation of an image signature. It must
        // include room for the signing cert, the trust anchors and any intermediate
        // certs included in the signature object.
        kMaxCerts = 4,

        // Certificate Decode Buffer Size -- Size of the temporary buffer used to decode
        // certs. The buffer must be big enough to hold the ASN1 DER encoding of the
        // TBSCertificate portion of the largest cert involved in signature verification.
        // Note that all certificates included in the signature are decoded using this
        // buffer, even if they are ultimately not involved in verifying the image
        // signature.
        kCertDecodeBufferSize = 644
    };

    bool mIsInitiator;

public:
    TestKeyExportDelegate(bool isInitiator)
    : mIsInitiator(isInitiator)
    {
    }

#if !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

    WEAVE_ERROR GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) __OVERRIDE
    {
        return GetNodeCertSet(keyExport->IsInitiator(), certSet);
    }

    WEAVE_ERROR ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) __OVERRIDE
    {
        return ReleaseNodeCertSet(keyExport->IsInitiator(), certSet);
    }

    WEAVE_ERROR GenerateNodeSignature(WeaveKeyExport * keyExport, const uint8_t * msgHash, uint8_t msgHashLen,
        TLVWriter & writer) __OVERRIDE
    {
        WEAVE_ERROR err;
        const uint8_t * privKey = NULL;
        uint16_t privKeyLen;

        err = GetNodePrivateKey(keyExport->IsInitiator(), privKey, privKeyLen);
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveECDSASignature(writer, TLV::ContextTag(kTag_WeaveSignature_ECDSASignatureData), msgHash, msgHashLen, privKey, privKeyLen);
        SuccessOrExit(err);

    exit:
        if (privKey != NULL)
        {
            WEAVE_ERROR relErr = ReleaseNodePrivateKey(keyExport->IsInitiator(), privKey);
            err = (err == WEAVE_NO_ERROR) ? relErr : err;
        }
        return err;
    }

    WEAVE_ERROR BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE
    {
        return BeginCertValidation(keyExport->IsInitiator(), certSet, validCtx);
    }

    WEAVE_ERROR HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, uint32_t requestedKeyId) __OVERRIDE
    {
        return HandleCertValidationResult(keyExport->IsInitiator(), certSet, validCtx, NULL, keyExport->MessageInfo(), requestedKeyId);
    }

    WEAVE_ERROR EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE
    {
        return EndCertValidation(keyExport->IsInitiator(), certSet, validCtx);
    }

    WEAVE_ERROR ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId) __OVERRIDE
    {
        return ValidateUnsignedKeyExportMessage(keyExport->IsInitiator(), NULL, keyExport->MessageInfo(), requestedKeyId);
    }

#endif // !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

    // Get the key export certificate set for the local node.
    WEAVE_ERROR GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
    {
        WEAVE_ERROR err;
        WeaveCertificateData *cert;
        bool certSetInitialized = false;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Initialize certificate set.
        err = certSet.Init(kMaxCerts, kCertDecodeBufferSize, nl::Weave::Platform::Security::MemoryAlloc, nl::Weave::Platform::Security::MemoryFree);
        SuccessOrExit(err);
        certSetInitialized = true;

        // Load Nest development root cert and mark it trusted.
        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);

        cert->CertFlags |= kCertFlag_IsTrusted;

        // Load the intermediate (DeviceCA) cert.
        err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength, 0, cert);
        SuccessOrExit(err);

        // Load the signing cert.
        if (isInitiator)
        {
            err = certSet.LoadCert(TestDevice1_Cert, TestDevice1_CertLength, 0, cert);
            SuccessOrExit(err);
        }
        else
        {
            err = certSet.LoadCert(TestDevice2_Cert, TestDevice2_CertLength, 0, cert);
            SuccessOrExit(err);
        }

    exit:
        if (err != WEAVE_NO_ERROR && certSetInitialized)
            certSet.Release();

        return err;
    }

    // Called when the key export engine is done with the certificate set returned by GetNodeCertSet().
    WEAVE_ERROR ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        certSet.Release();

    exit:
        return err;
    }

    // Get the local node's private key.
    WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        weavePrivKey = (uint8_t *)nl::Weave::Platform::Security::MemoryAlloc(kMaxDevicePrivateKeySize);
        VerifyOrExit(weavePrivKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

        if (isInitiator)
        {
            memcpy((uint8_t *)weavePrivKey, TestDevice1_PrivateKey, TestDevice1_PrivateKeyLength);
            weavePrivKeyLen = TestDevice1_PrivateKeyLength;
        }
        else
        {
            memcpy((uint8_t *)weavePrivKey, TestDevice2_PrivateKey, TestDevice2_PrivateKeyLength);
            weavePrivKeyLen = TestDevice1_PrivateKeyLength;
        }

    exit:
        return err;
    }

    // Called when the key export engine is done with the buffer returned by GetNodePrivateKey().
    WEAVE_ERROR ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        if (weavePrivKey != NULL)
        {
            nl::Weave::Platform::Security::MemoryFree((void *)weavePrivKey);
            weavePrivKey = NULL;
        }

    exit:
        return err;
    }

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err;
        WeaveCertificateData *cert;
        bool certSetInitialized = false;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Initialize certificate set.
        err = certSet.Init(kMaxCerts, kCertDecodeBufferSize, nl::Weave::Platform::Security::MemoryAlloc, nl::Weave::Platform::Security::MemoryFree);
        SuccessOrExit(err);
        certSetInitialized = true;

        // Load Nest development root cert and mark it trusted.
        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);

        cert->CertFlags |= kCertFlag_IsTrusted;

        // Initialize the validation context.
        InitValidationContext(validContext);

    exit:
        if (err != WEAVE_NO_ERROR && certSetInitialized)
            certSet.Release();

        return err;
    }

    // Called with the results of validating the peer's certificate.
    // Requestor verifies that response came from expected node.
    WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
                                                   const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
    {
        WEAVE_ERROR err;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        if (isInitiator)
        {
            // Client root key export response message should be signed by the Weave device certificate.
            if (requestedKeyId == WeaveKeyId::kClientRootKey &&
                validContext.SigningCert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_WeaveDeviceId)
                err = WEAVE_NO_ERROR;
            else
                err = WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE;
        }
        else
        {
            // IN THIS TEST ONLY:
            //   - Client root key can be exported by any Weave node if the request message was signed
            //     and the trust anchor is Nest root certificate.
            // IN THE REAL IMPLEMENTATION:
            //   - Client root key can be exported only by mobiles, i.e. the trust anchor should be an access
            //     token certificate.
            if (requestedKeyId == WeaveKeyId::kClientRootKey && validContext.TrustAnchor == &certSet.Certs[0])
                err = WEAVE_NO_ERROR;
            else
                err = WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST;
        }

    exit:
        return err;
    }

    // Called when peer certificate validation is complete.
    WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        certSet.Release();

    exit:
        return err;
    }

    // Called by requestor and responder to verify that received message was appropriately secured when the message isn't signed.
    WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
    {
        WEAVE_ERROR err;

        VerifyOrExit(isInitiator == mIsInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // IN THIS TEST ONLY:
        //   - Fabric secret can be exported by any Weave node if the request/response messages are encrypted
        //     with session key, which was created during PASE handshake.
        //   - Intermediate application key can be exported by the service end point.
        // IN THE REAL IMPLEMENTATION:
        //   - Currently there is no use case where fabric secret or any other key can be exported if the
        //     request/response messages are unsigned. This function should always return
        //     WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE error.
        if (requestedKeyId == WeaveKeyId::kFabricSecret &&
            WeaveKeyId::IsSessionKey(msgInfo->KeyId) &&
            msgInfo->PeerAuthMode == kWeaveAuthMode_PASE_PairingCode)
            err = WEAVE_NO_ERROR;
        else if (WeaveKeyId::GetType(requestedKeyId) == WeaveKeyId::kType_AppIntermediateKey &&
                 WeaveKeyId::IsSessionKey(msgInfo->KeyId) &&
                 msgInfo->PeerAuthMode == kWeaveAuthMode_CASE_ServiceEndPoint)
            err = WEAVE_NO_ERROR;
        else
            err = WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE;

    exit:
        return err;
    }
};


enum
{
    kKeyExportRequestError_None             = 0,
    kKeyExportRequestError_InvalidConfig,
};

enum
{
    kKeyExportResponseError_None             = 0,
    kKeyExportResponseError_Reconfigure,
    kKeyExportResponseError_NoCommonConfig,
};

// Test input vector format.
struct TestContext {
    uint8_t config;
    bool signMessages;
    const uint32_t* requestedKeyId;
    const uint32_t* expectedKeyId;
    const uint8_t* expectedKey;
    const uint8_t* expectedKeyLen;
    const uint16_t* msgKeyId;
    WeaveAuthMode msgKeyAuthMode;
    uint8_t requestErrorType;
    uint8_t responseErrorType;
};

static const uint32_t sFabricSecretId = WeaveKeyId::kFabricSecret;
static const uint32_t sClientRootKeyId = WeaveKeyId::kClientRootKey;
static const uint16_t sNoneKeyId = WeaveKeyId::kNone;

// Test input data.
static struct TestContext sContext[] = {
    // Proposed Config1 tests.
    { kKeyExportConfig_Config1, false, &sFabricSecretId, &sFabricSecretId, sFabricSecret,
      &sFabricSecretLen, &sTestDefaultSessionKeyId, kWeaveAuthMode_PASE_PairingCode,
      kKeyExportRequestError_InvalidConfig, kKeyExportResponseError_None },
    { kKeyExportConfig_Config1, false, &sIntermediateKeyId_FRK_EC, &sIntermediateKeyId_FRK_E2, sIntermediateKey_FRK_E2,
      &sIntermediateKeyLen_FRK_E2, &sTestDefaultSessionKeyId, kWeaveAuthMode_CASE_ServiceEndPoint,
      kKeyExportRequestError_None, kKeyExportResponseError_None },
    { kKeyExportConfig_Config1, true, &sClientRootKeyId, &sClientRootKeyId, sClientRootKey,
      &sClientRootKeyLen, &sNoneKeyId, kWeaveAuthMode_Unauthenticated,
      kKeyExportRequestError_None, kKeyExportResponseError_None },
    // Proposed Config2 tests.
    { kKeyExportConfig_Config2, false, &sFabricSecretId, &sFabricSecretId, sFabricSecret,
      &sFabricSecretLen, &sTestDefaultSessionKeyId, kWeaveAuthMode_PASE_PairingCode,
      kKeyExportRequestError_None, kKeyExportResponseError_None },
    { kKeyExportConfig_Config2, false, &sIntermediateKeyId_FRK_EC, &sIntermediateKeyId_FRK_E2, sIntermediateKey_FRK_E2,
      &sIntermediateKeyLen_FRK_E2, &sTestDefaultSessionKeyId, kWeaveAuthMode_CASE_ServiceEndPoint,
      kKeyExportRequestError_InvalidConfig, kKeyExportResponseError_None },
    { kKeyExportConfig_Config2, true, &sClientRootKeyId, &sClientRootKeyId, sClientRootKey,
      &sClientRootKeyLen, &sNoneKeyId, kWeaveAuthMode_Unauthenticated,
      kKeyExportRequestError_None, kKeyExportResponseError_None },
    // Proposed Config1 reconfigured to Config2 tests.
    { kKeyExportConfig_Config1, false, &sFabricSecretId, &sFabricSecretId, sFabricSecret,
      &sFabricSecretLen, &sTestDefaultSessionKeyId, kWeaveAuthMode_PASE_PairingCode,
      kKeyExportRequestError_None, kKeyExportResponseError_Reconfigure },
    { kKeyExportConfig_Config1, false, &sIntermediateKeyId_FRK_EC, &sIntermediateKeyId_FRK_E2, sIntermediateKey_FRK_E2,
      &sIntermediateKeyLen_FRK_E2, &sTestDefaultSessionKeyId, kWeaveAuthMode_CASE_ServiceEndPoint,
      kKeyExportRequestError_None, kKeyExportResponseError_Reconfigure },
    { kKeyExportConfig_Config1, true, &sClientRootKeyId, &sClientRootKeyId, sClientRootKey,
      &sClientRootKeyLen, &sNoneKeyId, kWeaveAuthMode_Unauthenticated,
      kKeyExportRequestError_InvalidConfig, kKeyExportResponseError_Reconfigure },
    // Proposed Config2 reconfigured to Config1 tests.
    { kKeyExportConfig_Config2, false, &sFabricSecretId, &sFabricSecretId, sFabricSecret,
      &sFabricSecretLen, &sTestDefaultSessionKeyId, kWeaveAuthMode_PASE_PairingCode,
      kKeyExportRequestError_None, kKeyExportResponseError_Reconfigure },
    { kKeyExportConfig_Config2, false, &sIntermediateKeyId_FRK_EC, &sIntermediateKeyId_FRK_E2, sIntermediateKey_FRK_E2,
      &sIntermediateKeyLen_FRK_E2, &sTestDefaultSessionKeyId, kWeaveAuthMode_CASE_ServiceEndPoint,
      kKeyExportRequestError_None, kKeyExportResponseError_Reconfigure },
    { kKeyExportConfig_Config2, true, &sClientRootKeyId, &sClientRootKeyId, sClientRootKey,
      &sClientRootKeyLen, &sNoneKeyId, kWeaveAuthMode_Unauthenticated,
      kKeyExportRequestError_None, kKeyExportResponseError_Reconfigure },
    // No common Configs for requester and responder tests.
    { kKeyExportConfig_Config1, false, &sFabricSecretId, &sFabricSecretId, sFabricSecret,
      &sFabricSecretLen, &sTestDefaultSessionKeyId, kWeaveAuthMode_PASE_PairingCode,
      kKeyExportRequestError_InvalidConfig, kKeyExportResponseError_NoCommonConfig },
    { kKeyExportConfig_Config2, false, &sIntermediateKeyId_FRK_EC, &sIntermediateKeyId_FRK_E2, sIntermediateKey_FRK_E2,
      &sIntermediateKeyLen_FRK_E2, &sTestDefaultSessionKeyId, kWeaveAuthMode_CASE_ServiceEndPoint,
      kKeyExportRequestError_None, kKeyExportResponseError_NoCommonConfig },
};

// Number of test context examples.
static const size_t kTestElements = sizeof(sContext) / sizeof(struct TestContext);

static void KeyExportProtocol_Test(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    WeaveKeyExport initiatorEng;
    WeaveKeyExport responderEng;
    PacketBuffer *msgBuf = NULL;
    TestKeyExportDelegate initiatorDelegate(true);
    TestKeyExportDelegate responderDelegate(false);
    TestGroupKeyStore keyStore;
    uint32_t exportedKeyId;
    uint16_t exportedKeyLen;
    uint8_t exportedKey[AppKeys::kWeaveFabricSecretSize];
    uint16_t dataLen;
    IPPacketInfo pktInfo;
    WeaveMessageInfo msgInfo;

    struct TestContext *theContext = (struct TestContext *)(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++, theContext++)
    {
        uint8_t proposedConfig = theContext->config;
        bool signMessages = theContext->signMessages;
        uint32_t keyId = *theContext->requestedKeyId;
        uint32_t expectedKeyId = *theContext->expectedKeyId;
        const uint8_t* expectedKey = theContext->expectedKey;
        uint8_t expectedKeyLen = *theContext->expectedKeyLen;

        msgInfo.Clear();
        pktInfo.Clear();
        msgInfo.KeyId = *theContext->msgKeyId;
        msgInfo.PeerAuthMode = theContext->msgKeyAuthMode;
        msgInfo.InPacketInfo = &pktInfo;

        sCurrentUTCTime = sEpochKey2_StartTime + 1;

        if (theContext->responseErrorType == kKeyExportResponseError_Reconfigure)
            printf("Running Key Export Protocol Test with proposed Config%d (Reconfigured to Config%d) with %s messages to export KeyId = %08X.\n",
                   proposedConfig, (KeyExport::kKeyExportSupportedConfig_All & ~proposedConfig), (signMessages ? "Signed" : "Unsigned"), keyId);
        else if (theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
            printf("Running Key Export Protocol Test with proposed Config%d while responder only supports Config%d, which results in NoCommonConfig error.\n",
                   proposedConfig, (KeyExport::kKeyExportSupportedConfig_All & ~proposedConfig));
        else
            printf("Running Key Export Protocol Test with proposed Config%d with %s messages to export KeyId = %08X.\n",
                   proposedConfig, (signMessages ? "Signed" : "Unsigned"), keyId);

        {
            initiatorEng.Reset();
            initiatorEng.Init(&initiatorDelegate);

            if (theContext->requestErrorType == kKeyExportRequestError_InvalidConfig)
                initiatorEng.SetAllowedConfigs(0);
            else if (theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
                initiatorEng.SetAllowedConfigs(proposedConfig);
            else
                initiatorEng.SetAllowedConfigs(KeyExport::kKeyExportSupportedConfig_All);

            msgBuf = PacketBuffer::New();
            NL_TEST_ASSERT(inSuite, msgBuf != NULL);

            err = initiatorEng.GenerateKeyExportRequest(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, proposedConfig, keyId, signMessages);

            if (theContext->requestErrorType == kKeyExportRequestError_InvalidConfig)
            {
                NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_KEY_EXPORT_CONFIGURATION);

                initiatorEng.Reset();
                initiatorEng.Init(&initiatorDelegate);

                if (theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
                    initiatorEng.SetAllowedConfigs(proposedConfig);
                else
                    initiatorEng.SetAllowedConfigs(KeyExport::kKeyExportSupportedConfig_All);

                PacketBuffer::Free(msgBuf);
                msgBuf = PacketBuffer::New();
                NL_TEST_ASSERT(inSuite, msgBuf != NULL);

                err = initiatorEng.GenerateKeyExportRequest(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, proposedConfig, keyId, signMessages);
            }

            NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

            msgBuf->SetDataLength(dataLen);
        }

#if DEBUG_PRINT_ENABLE
        printf("KeyExportRequest Message (%d bytes):\n", msgBuf->DataLength());
        DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
#endif

        {
            responderEng.Reset();
            responderEng.Init(&responderDelegate, &keyStore);

            if (theContext->responseErrorType == kKeyExportResponseError_Reconfigure ||
                theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
                responderEng.SetAllowedConfigs(KeyExport::kKeyExportSupportedConfig_All & ~proposedConfig);
            else
                responderEng.SetAllowedConfigs(KeyExport::kKeyExportSupportedConfig_All);

            err = responderEng.ProcessKeyExportRequest(msgBuf->Start(), msgBuf->DataLength(), &msgInfo);
            if (theContext->responseErrorType == kKeyExportResponseError_Reconfigure)
                NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED);
            else if (theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
                NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_NO_COMMON_KEY_EXPORT_CONFIGURATIONS);
            else
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;

            if (theContext->responseErrorType == kKeyExportResponseError_Reconfigure)
            {
                msgBuf = PacketBuffer::New();
                NL_TEST_ASSERT(inSuite, msgBuf != NULL);

                err = responderEng.GenerateKeyExportReconfigure(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen);
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

                msgBuf->SetDataLength(dataLen);

#if DEBUG_PRINT_ENABLE
                printf("KeyExportReconfigure Message (%d bytes):\n", msgBuf->DataLength());
                DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
#endif

                uint8_t newConfig;

                err = initiatorEng.ProcessKeyExportReconfigure(msgBuf->Start(), msgBuf->DataLength(), newConfig);
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

                PacketBuffer::Free(msgBuf);
                msgBuf = NULL;

                msgBuf = PacketBuffer::New();
                NL_TEST_ASSERT(inSuite, msgBuf != NULL);

                err = initiatorEng.GenerateKeyExportRequest(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, newConfig, keyId, signMessages);
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

                msgBuf->SetDataLength(dataLen);

#if DEBUG_PRINT_ENABLE
                printf("Reconfigured KeyExportRequest Message (%d bytes):\n", msgBuf->DataLength());
                DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
#endif

                // For responder this request is unrelated to the previous key export request.
                responderEng.Reset();
                responderEng.Init(&responderDelegate, &keyStore);

                responderEng.SetAllowedConfigs(KeyExport::kKeyExportSupportedConfig_All & ~proposedConfig);

                err = responderEng.ProcessKeyExportRequest(msgBuf->Start(), msgBuf->DataLength(), &msgInfo);
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

                PacketBuffer::Free(msgBuf);
                msgBuf = NULL;
            }
            else if (theContext->responseErrorType == kKeyExportResponseError_NoCommonConfig)
            {
                continue;
            }

            msgBuf = PacketBuffer::New();
            NL_TEST_ASSERT(inSuite, msgBuf != NULL);

            err = responderEng.GenerateKeyExportResponse(msgBuf->Start(), msgBuf->AvailableDataLength(), dataLen, NULL);
            NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

            msgBuf->SetDataLength(dataLen);
        }

#if DEBUG_PRINT_ENABLE
        printf("KeyExportResponse Message (%d bytes):\n", msgBuf->DataLength());
        DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
#endif

        {
            err = initiatorEng.ProcessKeyExportResponse(msgBuf->Start(), msgBuf->DataLength(), &msgInfo,
                                                        exportedKey, sizeof(exportedKey), exportedKeyLen, exportedKeyId);
            NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

#if DEBUG_PRINT_ENABLE
        printf("Exported Client Root Key:\n");
        DumpMemoryCStyle(exportedKey, exportedKeyLen, "  ", 16);
#endif

        // Compare the exported key.
        NL_TEST_ASSERT(inSuite, exportedKeyId == expectedKeyId);
        NL_TEST_ASSERT(inSuite, exportedKeyLen == expectedKeyLen);
        NL_TEST_ASSERT(inSuite, memcmp(exportedKey, expectedKey, exportedKeyLen) == 0);
    }
}


/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("KeyExportProtocol",                KeyExportProtocol_Test),
    NL_TEST_SENTINEL()
};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    nlTestSuite testSuite = {
        "weave-key-export-protocol",
        &sTests[0]
    };

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, &sContext);

    return nlTestRunnerStats(&testSuite);
}

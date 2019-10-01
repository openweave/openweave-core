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
 *      Unit tests for the WeaveCertProvClient class.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ToolCommon.h"
#include "MockCAService.h"
#include "TestWeaveCertData.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;
using namespace nl::Weave::ASN1;

using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;
using nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvClient;

#define TOOL_NAME "TestCASE"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

const char *gCurTest = NULL;

#define VerifyOrQuit(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stdout, "%s FAILED: ", (gCurTest != NULL) ? gCurTest : __FUNCTION__); \
        fputs(MSG, stdout); \
        fputs("\n", stdout); \
        exit(-1); \
    } \
} while (0)

#define SuccessOrQuit(ERR, MSG) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        fprintf(stdout, "%s FAILED: ", (gCurTest != NULL) ? gCurTest : __FUNCTION__); \
        fputs(MSG, stdout); \
        fputs(": ", stdout); \
        fputs(ErrorStr(ERR), stdout); \
        fputs("\n", stdout); \
        exit(-1); \
    } \
} while (0)

extern WEAVE_ERROR MakeCertInfo(uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen,
                                const uint8_t *entityCert, uint16_t entityCertLen,
                                const uint8_t *intermediateCert, uint16_t intermediateCertLen);

static bool sIncludeAuthorizedInfo = false;
static uint8_t * sPairingToken = NULL;
static uint16_t sPairingTokenLength;
static uint8_t * sPairingInitData = NULL;
static uint16_t sPairingInitDataLength;

static uint8_t sDeviceOperationalCert[nl::TestCerts::kTestCertBufSize];
static uint16_t sDeviceOperationalCertLength;

static uint8_t sDeviceRelatedCerts[nl::TestCerts::kTestCertBufSize];
static uint16_t sDeviceRelatedCertsLength;

enum DeviceType
{
    kDevType_WeaveCertProvisioned         = 1,
    kDevType_X509RSACertProvisioned       = 2,
};

class OpAuthCertProvDelegate : public WeaveCertProvAuthDelegate
{
public:
    OpAuthCertProvDelegate(void)
    {
    }

    // ===== Methods that implement the OpAuthCertProvDelegate interface.

    WEAVE_ERROR EncodeNodeCert(TLVWriter & writer) __OVERRIDE
    {
        // Copy the test device operational certificate into supplied TLV writer.
        return writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_OpDeviceCert), TestDevice1_OperationalCert, TestDevice1_OperationalCertLength);
    }

    WEAVE_ERROR GenerateNodeSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer) __OVERRIDE
    {
        return GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_OpDeviceSig_ECDSA), hash, hashLen,
                                                    TestDevice1_OperationalPrivateKey, TestDevice1_OperationalPrivateKeyLength);
    }
};

class ManufAttestCertProvDelegate : public WeaveCertProvAuthDelegate
{
public:
    ManufAttestCertProvDelegate(uint8_t devType, bool includeIntermediateCert)
    : mDeviceType(devType),
      mIncludeManufAttestRelatedCerts(includeIntermediateCert)
    {
    }

    // ===== Methods that implement the ManufAttestCertProvDelegate interface.

    WEAVE_ERROR EncodeNodeCert(TLVWriter & writer) __OVERRIDE
    {
        WEAVE_ERROR err;
        TLVType containerType;

        if (IsWeaveProvisionedDevice())
        {
            err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_ManufAttestInfo_Weave), kTLVType_Structure, containerType);
            SuccessOrExit(err);

            // Copy the test device manufacturer attestation (factory provisioned) certificate into supplied TLV writer.
            err = writer.CopyContainer(ContextTag(kTag_ManufAttestInfo_Weave_DeviceCert), TestDevice1_Cert, TestDevice1_CertLength);
            SuccessOrExit(err);

            if (mIncludeManufAttestRelatedCerts)
            {
                TLVType containerType2;

                // Start the RelatedCertificates array. This contains the list of certificates the signature verifier
                // will need to verify the signature.
                err = writer.StartContainer(ContextTag(kTag_ManufAttestInfo_Weave_RelatedCerts), kTLVType_Array, containerType2);
                SuccessOrExit(err);

                // Copy the intermediate test device CA certificate.
                err = writer.CopyContainer(AnonymousTag, nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength);
                SuccessOrExit(err);

                err = writer.EndContainer(containerType2);
                SuccessOrExit(err);
            }

            err = writer.EndContainer(containerType);
            SuccessOrExit(err);
        }
        else
        {
            // TODO: Support X509 ASN1 encoded certificate (kTag_ManufAttestInfo_X509)
            ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
        }

    exit:
        return err;
    }

    WEAVE_ERROR GenerateNodeSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer) __OVERRIDE
    {
        if (IsWeaveProvisionedDevice())
        {
            return GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_ManufAttestSig_ECDSA), hash, hashLen,
                                                       TestDevice1_PrivateKey, TestDevice1_PrivateKeyLength);
        }
        else
        {
            // TODO: Support RSA Signature (kTag_GetCertReqMsg_ManufAttestSig_RSA)
            return WEAVE_ERROR_NOT_IMPLEMENTED;
        }
    }

private:
    uint8_t mDeviceType;
    bool mIncludeManufAttestRelatedCerts;

    inline bool IsWeaveProvisionedDevice(void) { return (mDeviceType == kDevType_WeaveCertProvisioned); }
};

/**
 *  Handler for Certificate Provisioning Client API events.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the client object.
 *  @param[in]  eventType   Event ID passed by the event callback.
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback.
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback.
 *
 */
static void CertProvEventCallback(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;

    switch (eventType)
    {
    case kEvent_PrepareAuthorizeInfo:
    {
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_PrepareAuthorizeInfo");

        TLVWriter * writer = inParam.PrepareAuthorizeInfo.Writer;

        if (sIncludeAuthorizedInfo)
        {
            TLVType containerType;

            VerifyOrExit((sPairingToken != NULL) || (sPairingInitData != NULL), err = WEAVE_ERROR_INVALID_ARGUMENT);

            err = writer->StartContainer(ProfileTag(kWeaveProfile_Security, kTag_GetCertAuthorizeInfo), kTLVType_Structure, containerType);
            SuccessOrExit(err);

            // Pairing Token.
            if (sPairingToken != NULL)
            {
                err = writer->PutBytes(ContextTag(kTag_GetCertAuthorizeInfo_PairingToken), sPairingToken, sPairingTokenLength);
                SuccessOrExit(err);
            }

            // Pairing Initialization Data.
            if (sPairingInitData != NULL)
            {
                err = writer->PutBytes(ContextTag(kTag_GetCertAuthorizeInfo_PairingInitData), sPairingInitData, sPairingInitDataLength);
                SuccessOrExit(err);
            }

            err = writer->EndContainer(containerType);
            SuccessOrExit(err);
        }
        break;
    }

    case kEvent_RequestSent:
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_RequestSent");
        break;

    case kEvent_ResponseReceived:
    {
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_ResponseReceived");

        WeaveCertificateData *certData;
        const uint8_t * cert = inParam.ResponseReceived.Cert;
        uint16_t certLen = inParam.ResponseReceived.CertLen;
        const uint8_t * relatedCerts = inParam.ResponseReceived.RelatedCerts;
        uint16_t relatedCertsLen = inParam.ResponseReceived.RelatedCertsLen;

        // This certificate verification step is added for testing purposes only.
        // In reality, device doesn't have to validate certificate issued by the CA service.
        {
            err = certSet.Init(4, nl::TestCerts::kTestCertBufSize);
            SuccessOrExit(err);

            certSetInitialized = true;

            err = certSet.LoadCert(cert, certLen, kDecodeFlag_GenerateTBSHash, certData);
            SuccessOrExit(err);

            if (relatedCerts != NULL)
            {
                // Load intermediate certificate.
                err = certSet.LoadCerts(relatedCerts, relatedCertsLen, kDecodeFlag_GenerateTBSHash);
                SuccessOrExit(err);
            }

            err = ValidateWeaveDeviceCert(certSet);
            SuccessOrExit(err);
        }

        // Store service issued operational device certificate.
        VerifyOrExit(certLen <= sizeof(sDeviceOperationalCert), err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        memcpy(sDeviceOperationalCert, cert, certLen);
        sDeviceOperationalCertLength = certLen;

        // Store device intermediate certificates related to the device certificate.
        VerifyOrExit(relatedCertsLen <= sizeof(sDeviceRelatedCerts), err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        memcpy(sDeviceRelatedCerts, relatedCerts, relatedCertsLen);
        sDeviceRelatedCertsLength = relatedCertsLen;

        break;
    }

    case kEvent_ResponseProcessError:
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_ResponseProcessError");
        break;

    case kEvent_CommuncationError:
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_CommuncationError");
        break;

    case kEvent_ResponseTimeout:
        WeaveLogDetail(SecurityManager, "CertProvisioning::kEvent_ResponseTimeout");
        break;

    default:
        WeaveLogError(SecurityManager, "CertProvisioning unrecognized API event");
        break;
    }

exit:
    if (eventType == kEvent_PrepareAuthorizeInfo)
        outParam.PrepareAuthorizeInfo.Error = err;
    if (certSetInitialized)
        certSet.Release();
}

class MessageMutator
{
public:
    virtual ~MessageMutator() { }
    virtual void Reset() = 0;
    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvClient& clientEng, MockCAService& serviceEng) = 0;
    virtual bool IsComplete() = 0;
};

class NullMutator : public MessageMutator
{
public:
    virtual void Reset() { }
    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvClient& clientEng, MockCAService& serviceEng) { };
    virtual bool IsComplete() { return true; }
};

static NullMutator gNullMutator;

class MessageFuzzer : public MessageMutator
{
public:
    MessageFuzzer(const char *msgType)
    {
        mMsgType = msgType;
        mIndex = 0;
        mSkipStart = 0;
        mSkipLen = 0;
        mComplete = false;
        mTimeLimit = 0;
    }

    virtual void Reset() { mIndex = 0; mComplete = false; }

    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvClient& clientEng, MockCAService& serviceEng)
    {
        if (strcmp(msgType, mMsgType) == 0)
        {
            uint8_t *msgStart = msgBuf->Start();
            uint16_t msgLen = msgBuf->DataLength();
            uint8_t fuzzMask;

            VerifyOrQuit(msgLen > 0, "Unexpected packet length");

            if (mIndex == mSkipStart)
                mIndex += mSkipLen;

            if (mIndex >= msgLen)
                mIndex = msgLen - 1;

            do
                fuzzMask = GetRandU8();
            while (fuzzMask == 0);

            printf("MessageFuzzer: %s message mutated (offset %u, fuzz mask 0x%02X, orig value 0x%02X)\n", msgType, mIndex, fuzzMask, msgStart[mIndex]);

            msgStart[mIndex] ^= fuzzMask;

            mIndex++;

            mComplete = (mIndex >= msgLen);
        }
    }

    virtual bool IsComplete()
    {
        if (mComplete)
            return true;

        if (mTimeLimit != 0)
        {
            time_t now;
            time(&now);
            if (now >= mTimeLimit)
                return true;
        }

        return false;
    }

    MessageFuzzer& Skip(uint16_t start, uint16_t len) { mSkipStart = start; mSkipLen = len; return *this; }

    MessageFuzzer& TimeLimit(time_t timeLimit) { mTimeLimit = timeLimit; return *this; }

private:
    const char *mMsgType;
    uint16_t mIndex;
    uint16_t mSkipStart;
    uint16_t mSkipLen;
    bool mComplete;
    time_t mTimeLimit;
};

class CertProvEngineTest
{
public:
    CertProvEngineTest(const char *testName)
    {
        mTestName = testName;
        memset(mExpectedErrors, 0, sizeof(mExpectedErrors));
        mMutator = &gNullMutator;
        mReqType = kReqType_GetInitialOpDeviceCert;
        mDevType = kDevType_WeaveCertProvisioned;
        mLogMessageData = false;
        mClientIncludeManufAttestRelatedCerts = false;
        mServerIncludeDeviceCACert = false;
    }

    const char *TestName() const { return mTestName; }

    uint8_t RequestType() const { return mReqType; }
    CertProvEngineTest& RequestType (uint8_t val) { mReqType = val; return *this; }

    uint8_t DeviceType() const { return mDevType; }
    CertProvEngineTest& DeviceType (uint8_t val) { mDevType = val; return *this; }

    bool LogMessageData() const { return mLogMessageData; }
    CertProvEngineTest& LogMessageData(bool val) { mLogMessageData = val; return *this; }

    bool ClientIncludeIntermediateCert() const { return mClientIncludeManufAttestRelatedCerts; }
    CertProvEngineTest& ClientIncludeIntermediateCert(bool val) { mClientIncludeManufAttestRelatedCerts = val; return *this; }

    bool ServerIncludeIntermediateCert() const { return mServerIncludeDeviceCACert; }
    CertProvEngineTest& ServerIncludeIntermediateCert(bool val) { mServerIncludeDeviceCACert = val; return *this; }

    CertProvEngineTest& ExpectError(WEAVE_ERROR err)
    {
        return ExpectError(NULL, err);
    }

    CertProvEngineTest& ExpectError(const char *opName, WEAVE_ERROR err)
    {
        for (size_t i = 0; i < kMaxExpectedErrors; i++)
        {
            if (mExpectedErrors[i].Error == WEAVE_NO_ERROR)
            {
                mExpectedErrors[i].Error = err;
                mExpectedErrors[i].OpName = opName;
                break;
            }
        }

        return *this;
    }

    bool IsExpectedError(const char *opName, WEAVE_ERROR err) const
    {
        for (size_t i = 0; i < kMaxExpectedErrors && mExpectedErrors[i].Error != WEAVE_NO_ERROR; i++)
        {
            if (mExpectedErrors[i].Error == err &&
                (mExpectedErrors[i].OpName == NULL || strcmp(mExpectedErrors[i].OpName, opName) == 0))
                return true;
        }
        return false;
    }

    bool IsSuccessExpected() const { return mExpectedErrors[0].Error == WEAVE_NO_ERROR; }

    CertProvEngineTest& Mutator(MessageMutator *mutator) { mMutator = mutator; return *this; }

    void Run() const;

private:
    enum
    {
        kMaxExpectedErrors = 32
    };

    struct ExpectedError
    {
        const char *OpName;
        WEAVE_ERROR Error;
    };

    const char *mTestName;
    uint8_t mReqType;
    uint8_t mDevType;
    bool mLogMessageData;
    bool mClientIncludeManufAttestRelatedCerts;
    bool mServerIncludeDeviceCACert;
    ExpectedError mExpectedErrors[kMaxExpectedErrors];
    MessageMutator *mMutator;
};

void CertProvEngineTest::Run() const
{
    WEAVE_ERROR err;
    MockCAService serviceEng;
    PacketBuffer *msgBuf = NULL;
    PacketBuffer *msgBuf2 = NULL;
    OpAuthCertProvDelegate opAuthDelegate;
    ManufAttestCertProvDelegate manufAttestDelegate(DeviceType(), ClientIncludeIntermediateCert());
    WeaveExchangeManager exchangeMgr;

    printf("========== Starting Test: %s\n", TestName());

    gCurTest = TestName();

    mMutator->Reset();

    do
    {
        serviceEng.Init(&exchangeMgr);
        serviceEng.LogMessageData(LogMessageData());
        serviceEng.IncludeIntermediateCert(ServerIncludeIntermediateCert());

        // ========== Client Forms GetCertificateRequest ==========

        {
            msgBuf = PacketBuffer::New();
            VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");

            printf("Calling CertProvisioning::GenerateGetCertificateRequest\n");

            err = GenerateGetCertificateRequest(msgBuf, mReqType, &opAuthDelegate, &manufAttestDelegate, CertProvEventCallback);

            if (IsExpectedError("CertProvisioning::GenerateGetCertificateRequest", err))
                goto onExpectedError;

            SuccessOrQuit(err, "CertProvisioning::GenerateGetCertificateRequest() failed");
        }

        // ========== Client Sends GetCertificateRequest to the CA Service ==========

        // mMutator->MutateMessage("GetCertificateRequest", msgBuf, clientEng, serviceEng);

        printf("Client->Service: GetCertificateRequest Message (%d bytes)\n", msgBuf->DataLength());
        if (LogMessageData())
            DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "    ", 16);

        // ========== CA Service Processes GetCertificateRequest ==========

        {
            printf("Service: Calling ProcessGetCertificateRequest\n");

            GetCertificateRequestMessage msg;

            err = serviceEng.ProcessGetCertificateRequest(msgBuf, msg);

            if (IsExpectedError("Service:ProcessGetCertificateRequest", err))
                goto onExpectedError;

            SuccessOrQuit(err, "MockCAService::ProcessGetCertificateRequest() failed");

            // ========== CA Service Forms GetCertificateResponse ==========

            msgBuf2 = PacketBuffer::New();
            VerifyOrQuit(msgBuf2 != NULL, "PacketBuffer::New() failed");

            printf("Service: Calling GenerateGetCertificateResponse\n");

            err = serviceEng.GenerateGetCertificateResponse(msgBuf2, *msg.mOperationalCertSet.Certs);

            if (IsExpectedError("Service:GenerateGetCertificateResponse", err))
                goto onExpectedError;

            SuccessOrQuit(err, "MockCAService::GenerateGetCertificateResponse() failed");

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

        // ========== CA Service Sends GetCertificateResponse to Client ==========

        // mMutator->MutateMessage("GetCertificateResponse", msgBuf2, clientEng, serviceEng);

        printf("Service->Client: GetCertificateResponse Message (%d bytes)\n", msgBuf2->DataLength());
        if (LogMessageData())
            DumpMemory(msgBuf2->Start(), msgBuf2->DataLength(), "    ", 16);

        // ========== Client Processes GetCertificateResponse ==========

        {
            printf("Client: Calling ProcessGetCertificateResponse\n");

            err = ProcessGetCertificateResponse(msgBuf2, CertProvEventCallback);

            if (IsExpectedError("CertProvisioning::ProcessGetCertificateResponse()", err))
                goto onExpectedError;

            SuccessOrQuit(err, "CertProvisioning::ProcessGetCertificateResponse() failed");

            PacketBuffer::Free(msgBuf2);
            msgBuf2 = NULL;
        }

        // TODO: Check the result here.

        VerifyOrQuit(IsSuccessExpected(), "Test succeeded unexpectedly");

    onExpectedError:

        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        PacketBuffer::Free(msgBuf2);
        msgBuf2 = NULL;

        serviceEng.Shutdown();

    } while (!mMutator->IsComplete());

    printf("Test Complete: %s\n", TestName());

    gCurTest = NULL;
}

void CertProvEngineTests_BasicTests()
{
    // Basic sanity test with standard parameters
    CertProvEngineTest("Sanity test")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .LogMessageData(true)
        .Run();

    // Basic sanity test with standard parameters
    CertProvEngineTest("Sanity test")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .LogMessageData(true)
        .ClientIncludeIntermediateCert(true)
        .Run();

    // Basic sanity test with standard parameters
    CertProvEngineTest("Sanity test")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .LogMessageData(true)
        .ServerIncludeIntermediateCert(true)
        .Run();

    // Basic sanity test with standard parameters
    CertProvEngineTest("Sanity test")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .LogMessageData(true)
        .ClientIncludeIntermediateCert(true)
        .ServerIncludeIntermediateCert(true)
        .Run();
}

void CertProvEngineTests_ConfigNegotiationTests()
{
    // Initiator only supports Config1, responder supports Config1 and Config2, expect use of Config1
    CertProvEngineTest("Config1-only Initiator")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .Run();

    // Initiator proposes Config1 but supports Config2, responder only supports Config1, expect use of Config1
    CertProvEngineTest("Config1-only Responder")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .Run();

    // Initiator only supports Config2, Responder supports Config1 and Config2, expect use of Config2
    CertProvEngineTest("Config2-only initiator")
        .RequestType(kReqType_RotateCert)
        .Run();

    // Initiator proposes Config1 but supports Config2, Responder only supports Config2, expect reconfig to Config2
    CertProvEngineTest("Config2-only responder")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .Run();

    // Initiator proposes Config1 but supports Config2, Responder supports Config1 and Config2, expect reconfig to Config2
    CertProvEngineTest("Reconfig to Config2")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .Run();

    // Initiator proposes Config2 but supports Config1, Responder only supports Config1, expect reconfig to Config1
    CertProvEngineTest("Reconfig to Config1")
        .RequestType(kReqType_RotateCert)
        .Run();

    // Initiator only supports Config1, responder only supports Config2, expect error
    CertProvEngineTest("No Common Configs 1")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION)
        .Run();

    // Initiator only supports Config2, responder only supports Config1, expect error
    CertProvEngineTest("No Common Configs 2")
        .RequestType(kReqType_RotateCert)
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION)
        .Run();

    // Responder repeatedly sends Reconfigure, expect initiator error
    CertProvEngineTest("Repeated reconfigs")
        .RequestType(kReqType_GetInitialOpDeviceCert)
        .ExpectError("Initiator:ProcessReconfigure", WEAVE_ERROR_TOO_MANY_CASE_RECONFIGURATIONS)
        .Run();
}

void CertProvEngineTests_CurveNegotiationTests()
{
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1 && WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1 && WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1

    // Initiator proposes prime192v1 and supports secp224r1, responder supports secp224r1 and prime256v1, expect reconfig to secp224r1
    CertProvEngineTest("Reconfig to common curve")
        .Run();

    // Initiator only supports secp224r1, responder only supports prime256v1, expect error
    CertProvEngineTest("No common curves")
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
        .Run();

#endif
}

uint32_t gFuzzTestDurationSecs = 5;

void CertProvEngineTests_FuzzTests()
{
    time_t now, endTime;

    time(&now);
    endTime = now + gFuzzTestDurationSecs;

    while (true)
    {
        time(&now);
        if (now >= endTime)
            break;

        // Fuzz contents of BeginSessionRequest message, verify protocol error.
        {
            MessageFuzzer fuzzer = MessageFuzzer("BeginSessionRequest")
                .Skip(8, 8)  // Avoid mutating the proposed protocol config or ECDH curve fields
                             // in the BeginSessionRequest, as doing so will elicit a reconfigure
                             // rather than an error.
                .TimeLimit(endTime);
            CertProvEngineTest("Mutate BeginSessionRequest")
                .Mutator(&fuzzer)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_WRONG_TLV_TYPE)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_INVALID_TLV_TAG)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_INVALID_TLV_ELEMENT)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_END_OF_TLV)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_TLV_UNDERRUN)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_INVALID_SIGNATURE)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_INVALID_ARGUMENT)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_MESSAGE_INCOMPLETE)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_CA_CERT_NOT_FOUND)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_INCORRECT_STATE)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_CERT_NOT_VALID_YET)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_CERT_EXPIRED)
                .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED)
                .ExpectError("Responder:ProcessBeginSessionRequest", ASN1_ERROR_UNKNOWN_OBJECT_ID)
                .ExpectError("Responder:ProcessBeginSessionRequest", ASN1_ERROR_OVERFLOW)
                .Run();
        }

        // Fuzz contents of BeginSessionResponse message, verify protocol error.
        {
            MessageFuzzer fuzzer = MessageFuzzer("BeginSessionResponse")
                .TimeLimit(endTime);
            CertProvEngineTest("Mutate BeginSessionResponse")
                .Mutator(&fuzzer)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_WRONG_TLV_TYPE)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_INVALID_TLV_TAG)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_INVALID_TLV_ELEMENT)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_END_OF_TLV)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_TLV_UNDERRUN)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_INVALID_SIGNATURE)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_INVALID_ARGUMENT)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_MESSAGE_INCOMPLETE)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_CA_CERT_NOT_FOUND)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_INCORRECT_STATE)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_KEY_CONFIRMATION_FAILED)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_CERT_NOT_VALID_YET)
                .ExpectError("Initiator:ProcessBeginSessionResponse", WEAVE_ERROR_CERT_EXPIRED)
                .ExpectError("Initiator:ProcessBeginSessionResponse", ASN1_ERROR_UNKNOWN_OBJECT_ID)
                .ExpectError("Initiator:ProcessBeginSessionResponse", ASN1_ERROR_OVERFLOW)
                .Run();
        }

        // Fuzz contents of InitiatorKeyConfirm message, verify protocol error.
        {
            MessageFuzzer fuzzer = MessageFuzzer("InitiatorKeyConfirm")
                .TimeLimit(endTime);
            CertProvEngineTest("Mutate InitiatorKeyConfirm")
                .Mutator(&fuzzer)
                .ExpectError("Responder:ProcessInitiatorKeyConfirm", WEAVE_ERROR_KEY_CONFIRMATION_FAILED)
                .Run();
        }
    }

}

static OptionDef gToolOptionDefs[] =
{
    { "fuzz-duration", kArgumentRequired, 'f' },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -f, --fuzz-duration <seconds>\n"
    "       Fuzzing duration in seconds.\n"
    "\n";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Unit tests for Weave CASE engine.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    if (!ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    CertProvEngineTests_BasicTests();
    CertProvEngineTests_ConfigNegotiationTests();
    CertProvEngineTests_CurveNegotiationTests();
    CertProvEngineTests_FuzzTests();

    printf("All tests succeeded\n");

    exit(EXIT_SUCCESS);
}

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'f':
        if (!ParseInt(arg, gFuzzTestDurationSecs))
        {
            PrintArgError("%s: Invalid value specified for fuzz duration: %s\n", progName, arg);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

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
#include "CertProvOptions.h"
#include "MockCAService.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_WITH_OPENSSL
#include <openssl/rsa.h>
#include <openssl/pem.h>
#endif

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;
using namespace nl::Weave::ASN1;

using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;
using nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvEngine;

#define DEBUG_PRINT_ENABLE 0

uint32_t debugPrintCount = 0;

#define TOOL_NAME "TestCertProv"

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

class MessageMutator
{
public:
    virtual ~MessageMutator() { }
    virtual void Reset() = 0;
    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvEngine& clientEng, MockCAService& serviceEng) = 0;
    virtual bool IsComplete() = 0;
};

class NullMutator : public MessageMutator
{
public:
    virtual void Reset() { }
    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvEngine& clientEng, MockCAService& serviceEng) { };
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

    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCertProvEngine& clientEng, MockCAService& serviceEng)
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
            while (fuzzMask == 0 ||
                   // Make sure the EndOfContainer element modifies its Type field - otherwize it might still be interpreted as EndOfContainer element.
                   ((msgStart[mIndex] == kTLVElementType_EndOfContainer) && ((fuzzMask & kTLVTypeMask) == 0)));

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
        mManufAttestType = kManufAttestType_WeaveCert;
        mReqType = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert;
        mLogMessageData = false;
        mClientIncludeAuthorizeInfo = false;
        mClientIncludeOperationalRelatedCerts = false;
        mClientIncludeManufAttestInfo = true;
        mClientIncludeManufAttestRelatedCerts = false;
        mServerIncludeDeviceCACert = false;
    }

    const char *TestName() const { return mTestName; }

    uint8_t RequestType() const { return mReqType; }
    CertProvEngineTest& RequestType (uint8_t val) { mReqType = val; return *this; }

    uint8_t ManufAttestType() const { return mManufAttestType; }
    CertProvEngineTest& ManufAttestType (uint8_t val) { mManufAttestType = val; return *this; }

    bool LogMessageData() const { return mLogMessageData; }
    CertProvEngineTest& LogMessageData(bool val) { mLogMessageData = val; return *this; }

    bool ClientIncludeAuthorizeInfo() const { return mClientIncludeAuthorizeInfo; }
    CertProvEngineTest& ClientIncludeAuthorizeInfo(bool val) { mClientIncludeAuthorizeInfo = val; return *this; }

    bool ClientIncludeOperationalRelatedCerts() const { return mClientIncludeOperationalRelatedCerts; }
    CertProvEngineTest& ClientIncludeOperationalRelatedCerts(bool val) { mClientIncludeOperationalRelatedCerts = val; return *this; }

    bool ClientIncludeManufAttestInfo() const { return mClientIncludeManufAttestInfo; }
    CertProvEngineTest& ClientIncludeManufAttestInfo(bool val) { mClientIncludeManufAttestInfo = val; return *this; }

    bool ClientIncludeManufAttestRelatedCerts() const { return mClientIncludeManufAttestRelatedCerts; }
    CertProvEngineTest& ClientIncludeManufAttestRelatedCerts(bool val) { mClientIncludeManufAttestRelatedCerts = val; return *this; }

    bool ServerIncludeRelatedCerts() const { return mServerIncludeDeviceCACert; }
    CertProvEngineTest& ServerIncludeRelatedCerts(bool val) { mServerIncludeDeviceCACert = val; return *this; }

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
    uint8_t mManufAttestType;
    bool mLogMessageData;
    bool mClientIncludeAuthorizeInfo;
    bool mClientIncludeOperationalRelatedCerts;
    bool mClientIncludeManufAttestInfo;
    bool mClientIncludeManufAttestRelatedCerts;
    bool mServerIncludeDeviceCACert;
    ExpectedError mExpectedErrors[kMaxExpectedErrors];
    MessageMutator *mMutator;

    void PrintGetCertificateRequestMessage(PacketBuffer *msgBuf) const;
};

void CertProvEngineTest::PrintGetCertificateRequestMessage(PacketBuffer *msgBuf) const
{
    debugPrintCount++;
    printf("// ------------------- GET CERTIFICATE REQUEST MESSAGE EXAMPLE %02d --------------------------\n", debugPrintCount);
    printf("// GetCertReqMsg_ReqType                   : %s\n", (RequestType() == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ?
           "Get Initial Operational Device Certificate" : "Rotate Operational Device Certificate");
    printf("// GetCertAuthorizeInfo                    : %s\n", ClientIncludeAuthorizeInfo() ? "Yes" : "-----");
    printf("// GetCertReqMsg_OpDeviceCert              : %s\n", (RequestType() == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ?
           "TestDevice1_OperationalSelfSignedCert" : "TestDevice1_OperationalServiceAssignedCert");
    printf("// GetCertReqMsg_OpRelatedCerts            : %s\n", ClientIncludeOperationalRelatedCerts() ? "nl::NestCerts::Development::DeviceCA::Cert" : "-----");
    if (ClientIncludeManufAttestInfo())
    {
        if ((ManufAttestType() == kManufAttestType_WeaveCert))
        {
            printf("// GetCertReqMsg_ManufAttest_WeaveCert     : %s\n", "TestDevice1_Cert");
            printf("// GetCertReqMsg_ManufAttest_WeaveRelCerts : %s\n", ClientIncludeManufAttestRelatedCerts() ? "nl::NestCerts::Development::DeviceCA::Cert" : "-----");
        }
        else if ((ManufAttestType() == kManufAttestType_X509Cert))
        {
            printf("// GetCertReqMsg_ManufAttest_X509Cert      : %s\n", "TestDevice1_X509_RSA_Cert");
            printf("// GetCertReqMsg_ManufAttest_X509RelCerts  : %s\n", ClientIncludeManufAttestRelatedCerts() ? "TestDevice1_X509_RSA_ICACert1 (_ICACert2)" : "-----");
        }
        else if ((ManufAttestType() == kManufAttestType_HMAC))
        {
            printf("// GetCertReqMsg_ManufAttest_HMACKeyId     : 0x%" PRIX32 "\n", TestDevice1_ManufAttest_HMACKeyId);
        }
    }
    else
    {
        printf("// GetCertReqMsg_ManufAttestInfo           : -----\n");
    }
    printf("// GetCertReqMsg_OpDeviceSigAlgo           : ECDSAWithSHA256\n");
    printf("// GetCertReqMsg_OpDeviceSig_ECDSA         : ECDSASignature\n");
    if (ClientIncludeManufAttestInfo())
    {
        if ((ManufAttestType() == kManufAttestType_WeaveCert))
        {
            printf("// GetCertReqMsg_ManufAttestSigAlgo        : ECDSAWithSHA256\n");
            printf("// GetCertReqMsg_ManufAttestSig_ECDSA      : ECDSASignature\n");
        }
        else if ((ManufAttestType() == kManufAttestType_X509Cert))
        {
            printf("// GetCertReqMsg_ManufAttestSigAlgo        : SHA256WithRSAEncryption\n");
            printf("// GetCertReqMsg_ManufAttestSig_RSA        : RSASignature\n");
        }
        else if ((ManufAttestType() == kManufAttestType_HMAC))
        {
            printf("// GetCertReqMsg_ManufAttestSigAlgo        : HMACWithSHA256\n");
            printf("// GetCertReqMsg_ManufAttestSig_HMAC       : HMACSignature\n");
        }
    }
    else
    {
        printf("// GetCertReqMsg_ManufAttestSig            : -----\n");
    }
    printf("// EXPECTED RESULT                         : %s\n", IsSuccessExpected() ? "SUCCESS" : "ERROR");
    printf("// -----------------------------------------------------------------------------------------\n");

    uint8_t * data = msgBuf->Start();
    uint16_t dataLen = msgBuf->DataLength();

    printf("\nextern const uint8_t sGetCertRequestMsg_Example%02d[] =\n{", debugPrintCount);

    for (uint32_t i = 0; i < dataLen; i++)
    {
        if (i % 16 == 0)
            printf("\n    ");
        printf("0x%02X, ", data[i]);
    }

    printf("\n};\n\n");
}

void CertProvEngineTest::Run() const
{
    WEAVE_ERROR err;
    WeaveCertProvEngine clientEng;
    MockCAService serviceEng;
    PacketBuffer *msgBuf = NULL;
    PacketBuffer *msgBuf2 = NULL;
    WeaveExchangeManager exchangeMgr;

    printf("========== Starting Test: %s\n", TestName());
    printf("    Manufacturer Attestation Type             : %s\n", (ManufAttestType() == kManufAttestType_WeaveCert) ? "Weave Certificate" : "X509 Certificate");
    printf("    Request Type                              : %s\n", (RequestType() == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ? "GetInitialOpDeviceCert" : "RotateCert");
    printf("    Client Include Authorization Info         : %s\n", ClientIncludeAuthorizeInfo() ? "Yes" : "No");
    printf("    Client Include Op Related Certs           : %s\n", ClientIncludeOperationalRelatedCerts() ? "Yes" : "No");
    printf("    Client Include Manufacturer Attest Info   : %s\n", ClientIncludeManufAttestInfo() ? "Yes" : "No");
    printf("    Client Include Manuf Attest Related Certs : %s\n", ClientIncludeManufAttestRelatedCerts() ? "Yes" : "No");
    printf("    Server Include Op Related Certs           : %s\n", ServerIncludeRelatedCerts() ? "Yes" : "No");
    printf("    Expected Error                            : %s\n", IsSuccessExpected() ? "No" : "Yes");
    printf("==========\n");

    gCurTest = TestName();

    mMutator->Reset();

    gCertProvOptions.EphemeralNodeId = TestDevice1_OperationalNodeId;
    gCertProvOptions.RequestType = RequestType();
    gCertProvOptions.IncludeAuthorizeInfo = ClientIncludeAuthorizeInfo();
    gCertProvOptions.IncludeOperationalCACerts = ClientIncludeOperationalRelatedCerts();
    gCertProvOptions.ManufAttestType = ManufAttestType();
    gCertProvOptions.IncludeManufAttestCACerts = ClientIncludeManufAttestRelatedCerts();

    do
    {
        // clientEng.Init(NULL, &opAuthDelegate, &manufAttestDelegate, CertProvClientEventHandler, NULL);
        clientEng.Init(NULL, &gCertProvOptions, &gCertProvOptions, CertProvClientEventHandler, NULL);
        serviceEng.Init(&exchangeMgr);
        serviceEng.LogMessageData(LogMessageData());
        serviceEng.IncludeRelatedCerts(ServerIncludeRelatedCerts());

        // ========== Client Forms GetCertificateRequest ==========

        {
            msgBuf = PacketBuffer::New();
            VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");
            printf("Calling WeaveCertProvEngine::GenerateGetCertificateRequest\n");

            err = clientEng.GenerateGetCertificateRequest(msgBuf, RequestType(), ClientIncludeManufAttestInfo());

#if DEBUG_PRINT_ENABLE
            PrintGetCertificateRequestMessage(msgBuf);
#endif

            if (IsExpectedError("WeaveCertProvEngine::GenerateGetCertificateRequest", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCertProvEngine::GenerateGetCertificateRequest() failed");
        }

        // ========== Client Sends GetCertificateRequest to the CA Service ==========

        mMutator->MutateMessage("GetCertificateRequest", msgBuf, clientEng, serviceEng);

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

            err = serviceEng.GenerateGetCertificateResponse(msgBuf2, *msg.OperationalCertSet.Certs);

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;

            if (IsExpectedError("Service:GenerateGetCertificateResponse", err))
                goto onExpectedError;

            SuccessOrQuit(err, "MockCAService::GenerateGetCertificateResponse() failed");
        }

        // ========== CA Service Sends GetCertificateResponse to Client ==========

        mMutator->MutateMessage("GetCertificateResponse", msgBuf2, clientEng, serviceEng);

        printf("Service->Client: GetCertificateResponse Message (%d bytes)\n", msgBuf2->DataLength());
        if (LogMessageData())
            DumpMemory(msgBuf2->Start(), msgBuf2->DataLength(), "    ", 16);

        // ========== Client Processes GetCertificateResponse ==========

        {
            printf("Client: Calling ProcessGetCertificateResponse\n");

            err = clientEng.ProcessGetCertificateResponse(msgBuf2);

            PacketBuffer::Free(msgBuf2);
            msgBuf2 = NULL;

            if (IsExpectedError("Client:ProcessGetCertificateResponse", err))
                goto onExpectedError;

            SuccessOrQuit(err, "CertProvisioningClient::ProcessGetCertificateResponse() failed");
        }

        VerifyOrQuit(clientEng.GetState() == WeaveCertProvEngine::kState_Idle, "Client not in Idle state");

        VerifyOrQuit(IsSuccessExpected(), "Test succeeded unexpectedly");

    onExpectedError:

        if (msgBuf != NULL)
        {
            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

        if (msgBuf2 != NULL)
        {
            PacketBuffer::Free(msgBuf2);
            msgBuf2 = NULL;
        }

        clientEng.Shutdown();
        serviceEng.Shutdown();

    } while (!mMutator->IsComplete());

    printf("Test Complete: %s\n", TestName());

    gCurTest = NULL;
}

void CertProvEngineTests_GetInitialCertTests()
{
    bool logData = false;

    struct TestCase
    {
        uint8_t maType;             // Manufacturer Attestation Type.
        uint8_t reqType;            // Request Type.
        bool cIncludeAI;            // Client Includes Request Authorization Information.
        bool cIncludeOpRC;          // Client Includes Operational Device Related Certificates.
        bool cIncludeMA;            // Client Includes Manufacturer Attestation Information.
        bool cIncludeMARC;          // Client Includes Manufacturer Attestation Related Certificates.
        bool sIncludeSOpRC;         // Server Includes Operational Device Related Certificates.
        struct
        {
            WEAVE_ERROR err;        // Expected error.
            const char * opName;    // Function name.
        } ExpectedResult;
    };

    enum
    {
        // Short-hand names to make the test cases table more concise.
        WeaveCert             = kManufAttestType_WeaveCert,
        X509Cert              = kManufAttestType_X509Cert,
        HMAC                  = kManufAttestType_HMAC,
        InitReq               = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert,
        RotateReq             = WeaveCertProvEngine::kReqType_RotateOpDeviceCert,
    };

    static const TestCase sTestCases[] =
    {
        // Manuf                 Client     Client     Client     Client     Server
        // Attest     Req        Includes   Includes   Includes   Includes   Includes
        // Type       Type       AuthInfo   OpRCerts   ManufAtt   MARCerts   OpRCerts    Expected Result
        // ==============================================================================================================

        // Basic testing of certificate provisioning protocol with different load orders.
        {  WeaveCert, InitReq,   false,     false,     false,     false,     false,      { WEAVE_ERROR_INVALID_ARGUMENT, "Service:ProcessGetCertificateRequest" } },
        {  WeaveCert, InitReq,   false,     false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, InitReq,   false,     false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, InitReq,   false,     true,      true,      true,      false,      { WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT, "Service:ProcessGetCertificateRequest" } },

        {  WeaveCert, InitReq,   true,      false,     false,     false,     false,      { WEAVE_ERROR_INVALID_ARGUMENT, "Service:ProcessGetCertificateRequest" } },
        {  WeaveCert, InitReq,   true,      false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, InitReq,   true,      false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, InitReq,   true,      true,      true,      true,      false,      { WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT, "Service:ProcessGetCertificateRequest" } },

        {  WeaveCert, RotateReq, false,     false,     false,     false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, false,     false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, false,     false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, false,     true,      true,      true,      false,      { WEAVE_NO_ERROR, NULL } },

        {  WeaveCert, RotateReq, true,      false,     false,     false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, true,      false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, true,      false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  WeaveCert, RotateReq, true,      true,      true,      true,      false,      { WEAVE_NO_ERROR, NULL } },

#if WEAVE_SYSTEM_CONFIG_PACKETBUFFER_CAPACITY_MAX > 4400
        {  X509Cert,  InitReq,   false,     false,     true,      false,     false,      { WEAVE_ERROR_INVALID_SIGNATURE, "Service:ProcessGetCertificateRequest" } },
        {  X509Cert,  InitReq,   false,     false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  X509Cert,  InitReq,   false,     true,      true,      true,      false,      { WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT, "Service:ProcessGetCertificateRequest" } },

        {  X509Cert,  InitReq,   true,      false,     true,      false,     false,      { WEAVE_ERROR_INVALID_SIGNATURE, "Service:ProcessGetCertificateRequest" } },
        {  X509Cert,  InitReq,   true,      false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  X509Cert,  InitReq,   true,      true,      true,      true,      false,      { WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT, "Service:ProcessGetCertificateRequest" } },

        {  X509Cert,  RotateReq, false,     false,     true,      false,     false,      { WEAVE_ERROR_INVALID_SIGNATURE, "Service:ProcessGetCertificateRequest" } },
        {  X509Cert,  RotateReq, false,     false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  X509Cert,  RotateReq, false,     true,      true,      true,      false,      { WEAVE_NO_ERROR, NULL } },

        {  X509Cert,  RotateReq, true,      false,     true,      false,     false,      { WEAVE_ERROR_INVALID_SIGNATURE, "Service:ProcessGetCertificateRequest" } },
        {  X509Cert,  RotateReq, true,      false,     true,      true,      true,       { WEAVE_NO_ERROR, NULL } },
        {  X509Cert,  RotateReq, true,      true,      true,      true,      false,      { WEAVE_NO_ERROR, NULL } },
#endif // WEAVE_SYSTEM_CONFIG_PACKETBUFFER_CAPACITY_MAX > 4000

        {  HMAC,      InitReq,   false,     false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  HMAC,      InitReq,   false,     false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  HMAC,      InitReq,   true,      false,     true,      false,     true,       { WEAVE_NO_ERROR, NULL } },
        {  HMAC,      InitReq,   true,      true,      true,      false,     true,       { WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT, "Service:ProcessGetCertificateRequest" } },

        {  HMAC,      RotateReq, true,      false,     true,      false,     false,      { WEAVE_NO_ERROR, NULL } },
        {  HMAC,      RotateReq, false,     true,      true,      false,     true,       { WEAVE_NO_ERROR, NULL } },
    };

    static const size_t sNumTestCases = sizeof(sTestCases) / sizeof(sTestCases[0]);

    for (unsigned i = 0; i < sNumTestCases; i++)
    {
        const TestCase& testCase = sTestCases[i];

        // Basic sanity test
        CertProvEngineTest("Basic")
            .ManufAttestType(testCase.maType)
            .RequestType(testCase.reqType)
            .ClientIncludeAuthorizeInfo(testCase.cIncludeAI)
            .ClientIncludeOperationalRelatedCerts(testCase.cIncludeOpRC)
            .ClientIncludeManufAttestInfo(testCase.cIncludeMA)
            .ClientIncludeManufAttestRelatedCerts(testCase.cIncludeMARC)
            .ServerIncludeRelatedCerts(testCase.sIncludeSOpRC)
            .ExpectError(testCase.ExpectedResult.opName, testCase.ExpectedResult.err)
            .LogMessageData(logData)
            .Run();
    }
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

        // Fuzz contents of GetCertificateRequest message, verify protocol error.
        {
            MessageFuzzer fuzzer = MessageFuzzer("GetCertificateRequest")
                // .Skip(8, 8)
                .TimeLimit(endTime);
            CertProvEngineTest("Mutate GetCertificateRequest")
                .Mutator(&fuzzer)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_WRONG_TLV_TYPE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_INVALID_TLV_TAG)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_INVALID_TLV_ELEMENT)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_END_OF_TLV)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_TLV_UNDERRUN)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_INVALID_SIGNATURE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_INVALID_ARGUMENT)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_CA_CERT_NOT_FOUND)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_WRONG_CERT_SUBJECT)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_WRONG_CERT_TYPE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_INCORRECT_STATE)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_CERT_NOT_VALID_YET)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_CERT_EXPIRED)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM)
                .ExpectError("Service:ProcessGetCertificateRequest", ASN1_ERROR_UNKNOWN_OBJECT_ID)
                .ExpectError("Service:ProcessGetCertificateRequest", ASN1_ERROR_OVERFLOW)
                .ExpectError("Service:ProcessGetCertificateRequest", ASN1_ERROR_UNSUPPORTED_ENCODING)
                .ExpectError("Service:ProcessGetCertificateRequest", WEAVE_ERROR_NOT_IMPLEMENTED)  // TODO: Remove once X509 RSA Certificates are Supported
                .Run();
        }

        // Fuzz contents of GetCertificateResponse message, verify protocol error.
        {
            MessageFuzzer fuzzer = MessageFuzzer("GetCertificateResponse")
                .TimeLimit(endTime);
            CertProvEngineTest("Mutate GetCertificateResponse")
                .Mutator(&fuzzer)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_WRONG_TLV_TYPE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_INVALID_TLV_TAG)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_INVALID_TLV_ELEMENT)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_END_OF_TLV)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_TLV_UNDERRUN)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_INVALID_SIGNATURE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_INVALID_ARGUMENT)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_CA_CERT_NOT_FOUND)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_WRONG_CERT_SUBJECT)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_WRONG_CERT_TYPE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_INCORRECT_STATE)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_CERT_NOT_VALID_YET)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_CERT_EXPIRED)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED)
                .ExpectError("Client:ProcessGetCertificateResponse", ASN1_ERROR_UNKNOWN_OBJECT_ID)
                .ExpectError("Client:ProcessGetCertificateResponse", ASN1_ERROR_OVERFLOW)
                .ExpectError("Client:ProcessGetCertificateResponse", ASN1_ERROR_UNSUPPORTED_ENCODING)
                .ExpectError("Client:ProcessGetCertificateResponse", WEAVE_ERROR_NOT_IMPLEMENTED)  // TODO: Remove once X509 RSA Certificates are Supported
                .LogMessageData(false)
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

    CertProvEngineTests_GetInitialCertTests();
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

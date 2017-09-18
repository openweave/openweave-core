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
 *      Unit tests for the WeaveCASEEngine class.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ToolCommon.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCASE.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CASE;
using namespace nl::Weave::ASN1;

using nl::Weave::Crypto::EncodedECPublicKey;
using nl::Weave::Crypto::EncodedECPrivateKey;

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

class InitiatorAuthDelegate : public WeaveCASEAuthDelegate
{
public:
    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen)
    {
        return MakeCertInfo(buf, bufSize, certInfoLen, TestDevice1_Cert, TestDevice1_CertLength, NULL, 0);
    }

    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
    {
        weavePrivKey = TestDevice1_PrivateKey;
        weavePrivKeyLen = TestDevice1_PrivateKeyLength;
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey)
    {
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen)
    {
        payloadLen = 0;
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err;
        ASN1UniversalTime validTime;
        WeaveCertificateData *cert;

        certSet.Init(10, 1024);

        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        memset(&validContext, 0, sizeof(validContext));
        validTime.Year = 2013;
        validTime.Month = 11;
        validTime.Day = 20;
        validTime.Hour = validTime.Minute = validTime.Second = 0;
        err = PackCertTime(validTime, validContext.EffectiveTime);
        SuccessOrExit(err);

        validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
        validContext.RequiredKeyPurposes = kKeyPurposeFlag_ServerAuth;

    exit:
        return err;
    }

    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        return WEAVE_NO_ERROR;
    }
};

class ResponderAuthDelegate : public WeaveCASEAuthDelegate
{
public:
    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen)
    {
        return MakeCertInfo(buf, bufSize, certInfoLen,
                TestDevice2_Cert, TestDevice2_CertLength,
                nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength);
    }

    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
    {
        weavePrivKey = TestDevice2_PrivateKey;
        weavePrivKeyLen = TestDevice2_PrivateKeyLength;
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey)
    {
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen)
    {
        payloadLen = 0;
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err;
        ASN1UniversalTime validTime;
        WeaveCertificateData *cert;

        certSet.Init(10, 1024);

        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);

        memset(&validContext, 0, sizeof(validContext));
        validTime.Year = 2013;
        validTime.Month = 11;
        validTime.Day = 20;
        validTime.Hour = validTime.Minute = validTime.Second = 0;
        err = PackCertTime(validTime, validContext.EffectiveTime);
        SuccessOrExit(err);

        validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
        validContext.RequiredKeyPurposes = kKeyPurposeFlag_ClientAuth;

    exit:
        return err;
    }

    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        return WEAVE_NO_ERROR;
    }
};

class MessageMutator
{
public:
    virtual ~MessageMutator() { }
    virtual void Reset() = 0;
    virtual void MutateMessage(const char *msgName, PacketBuffer *msgBuf, WeaveCASEEngine& initiatorEng, WeaveCASEEngine& responderEng) = 0;
    virtual bool IsComplete() = 0;
};

class NullMutator : public MessageMutator
{
public:
    virtual void Reset() { }
    virtual void MutateMessage(const char *msgName, PacketBuffer *msgBuf, WeaveCASEEngine& initiatorEng, WeaveCASEEngine& responderEng) { }
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

    virtual void MutateMessage(const char *msgType, PacketBuffer *msgBuf, WeaveCASEEngine& initiatorEng, WeaveCASEEngine& responderEng)
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

class CASEEngineTest
{
public:
    CASEEngineTest(const char *testName)
    {
        mTestName = testName;
        mProposedConfig = mExpectedConfig = kCASEConfig_NotSpecified;
        mProposedCurve = mExpectedCurve = kWeaveCurveId_NotSpecified;
        mInitiatorAllowedConfigs = mResponderAllowedConfigs = kCASEAllowedConfig_Config1|kCASEAllowedConfig_Config2;
        mInitiatorAllowedCurves = mResponderAllowedCurves = kWeaveCurveSet_prime192v1|kWeaveCurveSet_secp160r1|kWeaveCurveSet_secp224r1|kWeaveCurveSet_prime256v1;
        mInitiatorRequestKeyConfirm = true;
        mResponderRequiresKeyConfirm = false;
        mExpectReconfig = false;
        mExpectedConfig = kCASEConfig_NotSpecified;
        mExpectedCurve = kWeaveCurveId_NotSpecified;
        mForceRepeatedReconfig = false;
        memset(mExpectedErrors, 0, sizeof(mExpectedErrors));
        mMutator = &gNullMutator;
        mLogMessageData = false;
    }

    const char *TestName() const { return mTestName; }

    uint32_t ProposedConfig() const { return mProposedConfig; }
    CASEEngineTest& ProposedConfig(uint32_t val) { mProposedConfig = val; return *this; }

    uint32_t ProposedCurve() const { return mProposedCurve; }
    CASEEngineTest& ProposedCurve(uint32_t val) { mProposedCurve = val; return *this; }

    uint32_t InitiatorAllowedConfigs() const { return mInitiatorAllowedConfigs; }
    CASEEngineTest& InitiatorAllowedConfigs(uint8_t val) { mInitiatorAllowedConfigs = val; return *this; }

    uint32_t ResponderAllowedConfigs() const { return mResponderAllowedConfigs; }
    CASEEngineTest& ResponderAllowedConfigs(uint8_t val) { mResponderAllowedConfigs = val; return *this; }

    uint8_t InitiatorAllowedCurves() const { return mInitiatorAllowedCurves; }
    CASEEngineTest& InitiatorAllowedCurves(uint8_t val) { mInitiatorAllowedCurves = val; return *this; }

    uint8_t ResponderAllowedCurves() const { return mResponderAllowedCurves; }
    CASEEngineTest& ResponderAllowedCurves(uint8_t val) { mResponderAllowedCurves = val; return *this; }

    bool InitiatorRequestKeyConfirm() const { return mInitiatorRequestKeyConfirm; }
    CASEEngineTest& InitiatorRequestKeyConfirm(bool val) { mInitiatorRequestKeyConfirm = val; return *this; }

    bool ResponderRequiresKeyConfirm() const { return mResponderRequiresKeyConfirm; }
    CASEEngineTest& ResponderRequiresKeyConfirm(bool val) { mResponderRequiresKeyConfirm = val; return *this; }

    uint32_t ExpectReconfig() const { return mExpectReconfig; }
    CASEEngineTest& ExpectReconfig(uint32_t expectedConfig)
    {
        mExpectReconfig = true;
        mExpectedConfig = expectedConfig;
        return *this;
    }
    CASEEngineTest& ExpectReconfigCurve(uint32_t expectedCurve)
    {
        mExpectReconfig = true;
        mExpectedCurve = expectedCurve;
        return *this;
    }
    uint32_t ExpectedConfig() const { return mExpectedConfig != kCASEConfig_NotSpecified ? mExpectedConfig : mProposedConfig; }
    uint32_t ExpectedCurve() const { return mExpectedCurve != kWeaveCurveId_NotSpecified ? mExpectedCurve : mProposedCurve; }

    bool ForceRepeatedReconfig() const { return mForceRepeatedReconfig; }
    CASEEngineTest& ForceRepeatedReconfig(bool val) { mForceRepeatedReconfig = val; return *this; }

    CASEEngineTest& ExpectError(WEAVE_ERROR err)
    {
        return ExpectError(NULL, err);
    }

    CASEEngineTest& ExpectError(const char *opName, WEAVE_ERROR err)
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

    CASEEngineTest& Mutator(MessageMutator *mutator) { mMutator = mutator; return *this; }

    bool LogMessageData() const { return mLogMessageData; }
    CASEEngineTest& LogMessageData(bool val) { mLogMessageData = val; return *this; }

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
    uint32_t mProposedConfig;
    uint32_t mProposedCurve;
    uint8_t mInitiatorAllowedConfigs;
    uint8_t mInitiatorAllowedCurves;
    uint8_t mResponderAllowedConfigs;
    uint8_t mResponderAllowedCurves;
    bool mInitiatorRequestKeyConfirm;
    bool mResponderRequiresKeyConfirm;
    bool mExpectReconfig;
    uint32_t mExpectedConfig;
    uint32_t mExpectedCurve;
    bool mForceRepeatedReconfig;
    ExpectedError mExpectedErrors[kMaxExpectedErrors];
    MessageMutator *mMutator;
    bool mLogMessageData;
};

void CASEEngineTest::Run() const
{
    WEAVE_ERROR err;
    WeaveCASEEngine initiatorEng;
    WeaveCASEEngine responderEng;
    PacketBuffer *msgBuf = NULL;
    PacketBuffer *msgBuf2 = NULL;
    InitiatorAuthDelegate initiatorDelegate;
    ResponderAuthDelegate responderDelegate;
    const WeaveEncryptionKey *initiatorKey;
    const WeaveEncryptionKey *responderKey;

    printf("========== Starting Test: %s\n", TestName());

    gCurTest = TestName();

    mMutator->Reset();

    do
    {
        bool reconfigPerformed = false;
        uint32_t config = ProposedConfig();
        uint32_t curveId = ProposedCurve();

        initiatorEng.Init();
        initiatorEng.AuthDelegate = &initiatorDelegate;
        initiatorEng.SetAllowedConfigs(InitiatorAllowedConfigs());
        initiatorEng.SetAllowedCurves(InitiatorAllowedCurves());

    onReconfig:

        responderEng.Init();
        responderEng.AuthDelegate = &responderDelegate;
        responderEng.SetAllowedConfigs(ResponderAllowedConfigs());
        responderEng.SetAllowedCurves(ResponderAllowedCurves());
        responderEng.SetResponderRequiresKeyConfirm(ResponderRequiresKeyConfirm());

        // ========== Initiator Forms BeginSessionRequest ==========

        {
            BeginSessionRequestMessage req;
            req.Reset();
            req.ProtocolConfig = config;
            initiatorEng.SetAlternateConfigs(req);
            req.CurveId = curveId;
            initiatorEng.SetAlternateCurves(req);
            req.PerformKeyConfirm = InitiatorRequestKeyConfirm();
            req.SessionKeyId = sTestDefaultSessionKeyId;
            req.EncryptionType = kWeaveEncryptionType_AES128CTRSHA1;

            msgBuf = PacketBuffer::New();
            VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");

            printf("Initiator: Calling GenerateBeginSessionRequest\n");

            err = initiatorEng.GenerateBeginSessionRequest(req, msgBuf);

            if (IsExpectedError("Initiator:GenerateBeginSessionRequest", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCASEEngine::GenerateBeginSessionRequest() failed");
        }

        // ========== Initiator Sends BeginSessionRequest to Responder ==========

        mMutator->MutateMessage("BeginSessionRequest", msgBuf, initiatorEng, responderEng);

        printf("Initiator->Responder: BeginSessionRequest Message (%d bytes)\n", msgBuf->DataLength());
        if (LogMessageData())
            DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);

        // ========== Responder Processes BeginSessionRequest ==========

        {
            BeginSessionRequestMessage req;
            ReconfigureMessage reconf;
            req.Reset();
            reconf.Reset();

            printf("Responder: Calling ProcessBeginSessionRequest\n");

            err = responderEng.ProcessBeginSessionRequest(msgBuf, req, reconf);

            if (IsExpectedError("Responder:ProcessBeginSessionRequest", err))
                goto onExpectedError;

            if (ExpectReconfig() && !reconfigPerformed)
            {
                VerifyOrQuit(err == WEAVE_ERROR_CASE_RECONFIG_REQUIRED, "WEAVE_ERROR_CASE_RECONFIG_REQUIRED error expected");

                VerifyOrQuit(ExpectedConfig() == kCASEConfig_NotSpecified || reconf.ProtocolConfig == ExpectedConfig(), "Unexpected config proposed in ReconfigureMessage");
                VerifyOrQuit(ExpectedCurve() == kWeaveCurveId_NotSpecified || reconf.CurveId == ExpectedCurve(), "Unexpected curve proposed in ReconfigureMessage");

                PacketBuffer::Free(msgBuf);
                msgBuf = NULL;

                // ========== Responder Forms Reconfigure ==========

                printf("Responder: Generating Reconfigure Message\n");

                msgBuf = PacketBuffer::New();
                VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");
                err = reconf.Encode(msgBuf);
                SuccessOrQuit(err, "ReconfigureMessage::Encode() failed");

                // ========== Responder Sends Reconfigure to Initiator ==========

                mMutator->MutateMessage("Reconfigure", msgBuf, initiatorEng, responderEng);

                printf("Responder->Initiator: Reconfigure Message (%d bytes)\n", msgBuf->DataLength());
                if (LogMessageData())
                    DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);

                // ========== Initiator Processes Reconfigure ==========

                printf("Initiator: Calling ProcessReconfigure\n");

                err = initiatorEng.ProcessReconfigure(msgBuf, reconf);

                if (IsExpectedError("Initiator:ProcessReconfigure", err))
                    goto onExpectedError;

                SuccessOrQuit(err, "WeaveCASEEngine::ProcessReconfigure() failed");

                PacketBuffer::Free(msgBuf);
                msgBuf = NULL;

                if (!ForceRepeatedReconfig())
                {
                    reconfigPerformed = true;

                    config = reconf.ProtocolConfig;
                    curveId = reconf.CurveId;
                }

                responderEng.Shutdown();

                goto onReconfig;
            }

            else
            {
                VerifyOrQuit(err != WEAVE_ERROR_CASE_RECONFIG_REQUIRED, "Unexpected reconfig");
            }

            SuccessOrQuit(err, "WeaveCASEEngine::ProcessBeginSessionRequest() failed");

            // ========== Responder Forms BeginSessionResponse ==========

            BeginSessionResponseMessage resp;
            resp.Reset();
            resp.ProtocolConfig = req.ProtocolConfig;
            resp.CurveId = req.CurveId;

            msgBuf2 = PacketBuffer::New();
            VerifyOrQuit(msgBuf2 != NULL, "PacketBuffer::New() failed");

            printf("Responder: Calling GenerateBeginSessionResponse\n");

            err = responderEng.GenerateBeginSessionResponse(resp, msgBuf2, req);

            if (IsExpectedError("Responder:GenerateBeginSessionResponse", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCASEEngine::GenerateBeginSessionResponse() failed");

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

        // ========== Responder Sends BeginSessionResponse to Initiator ==========

        mMutator->MutateMessage("BeginSessionResponse", msgBuf2, initiatorEng, responderEng);

        printf("Responder->Initiator: BeginSessionResponse Message (%d bytes)\n", msgBuf2->DataLength());
        if (LogMessageData())
            DumpMemory(msgBuf2->Start(), msgBuf2->DataLength(), "  ", 16);

        // ========== Initiator Processes BeginSessionResponse ==========

        {
            BeginSessionResponseMessage resp;
            resp.Reset();

            printf("Initiator: Calling ProcessBeginSessionResponse\n");

            err = initiatorEng.ProcessBeginSessionResponse(msgBuf2, resp);

            if (IsExpectedError("Initiator:ProcessBeginSessionResponse", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCASEEngine::ProcessBeginSessionResponse() failed");

            PacketBuffer::Free(msgBuf2);
            msgBuf2 = NULL;
        }

        if (InitiatorRequestKeyConfirm() || ResponderRequiresKeyConfirm())
        {
            VerifyOrQuit(initiatorEng.PerformingKeyConfirm(), "Initiator not performing key confirmation");
            VerifyOrQuit(responderEng.PerformingKeyConfirm(), "Responder not performing key confirmation");

            // ========== Initiator Forms InitiatorKeyConfirm ==========

            msgBuf = PacketBuffer::New();
            VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");

            printf("Initiator: Calling GenerateInitiatorKeyConfirm\n");

            err = initiatorEng.GenerateInitiatorKeyConfirm(msgBuf);

            if (IsExpectedError("Initiator:GenerateInitiatorKeyConfirm", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCASEEngine::GenerateInitiatorKeyConfirm() failed");

            // ========== Initiator Sends InitiatorKeyConfirm to Responder ==========

            mMutator->MutateMessage("InitiatorKeyConfirm", msgBuf, initiatorEng, responderEng);

            printf("Initiator->Responder: InitiatorKeyConfirm Message (%d bytes)\n", msgBuf->DataLength());
            if (LogMessageData())
                DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);

            // ========== Responder Processes InitiatorKeyConfirm ==========

            printf("Responder: Calling ProcessInitiatorKeyConfirm\n");

            err = responderEng.ProcessInitiatorKeyConfirm(msgBuf);

            if (IsExpectedError("Responder:ProcessInitiatorKeyConfirm", err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeaveCASEEngine::ProcessInitiatorKeyConfirm() failed");

            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

        VerifyOrQuit(initiatorEng.State == WeaveCASEEngine::kState_Complete, "Initiator not in Complete state");
        VerifyOrQuit(responderEng.State == WeaveCASEEngine::kState_Complete, "Responder not in Complete state");

        if (ExpectedConfig() != kCASEConfig_NotSpecified)
        {
            VerifyOrQuit(initiatorEng.SelectedConfig() == ExpectedConfig(), "Initiator did not select expected config");
            VerifyOrQuit(responderEng.SelectedConfig() == ExpectedConfig(), "Responder did not select expected config");
        }

        if (ExpectedCurve() != kWeaveCurveId_NotSpecified)
        {
            VerifyOrQuit(initiatorEng.SelectedCurve() == ExpectedCurve(), "Initiator did not select expected curve");
            VerifyOrQuit(responderEng.SelectedCurve() == ExpectedCurve(), "Responder did not select expected curve");
        }

        printf("Initiator: Calling GetSessionKey\n");

        err = initiatorEng.GetSessionKey(initiatorKey);
        SuccessOrQuit(err, "WeaveCASEEngine::GetSessionKey() failed");

        printf("Responder: Calling GetSessionKey\n");

        err = responderEng.GetSessionKey(responderKey);
        SuccessOrQuit(err, "WeaveCASEEngine::GetSessionKey() failed");

        VerifyOrQuit(memcmp(initiatorKey->AES128CTRSHA1.DataKey, responderKey->AES128CTRSHA1.DataKey, WeaveEncryptionKey_AES128CTRSHA1::DataKeySize) == 0,
                     "Data key mismatch");

        VerifyOrQuit(memcmp(initiatorKey->AES128CTRSHA1.IntegrityKey, responderKey->AES128CTRSHA1.IntegrityKey, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize) == 0,
                     "Integrity key mismatch");

        VerifyOrQuit(IsSuccessExpected(), "Test succeeded unexpectedly");

    onExpectedError:

        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        PacketBuffer::Free(msgBuf2);
        msgBuf2 = NULL;

        initiatorEng.Shutdown();
        responderEng.Shutdown();

    } while (!mMutator->IsComplete());

    printf("Test Complete: %s\n", TestName());

    gCurTest = NULL;
}

void CASEEngineTests_BasicTests()
{
    // Basic sanity test with standard parameters
    CASEEngineTest("Sanity test")
        .Run();
}

void CASEEngineTests_EllipticCurveTests()
{
    // Test secp160r1 curve
    CASEEngineTest("Test secp160r1")
        .ProposedCurve(kWeaveCurveId_secp160r1)
#if !WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
        .ExpectError("Initiator:GenerateBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
#endif
        .Run();

    // Test prime192v1 curve
    CASEEngineTest("Test prime192v1")
        .ProposedCurve(kWeaveCurveId_prime192v1)
#if !WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
        .ExpectError("Initiator:GenerateBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
#endif
        .Run();

    // Test secp224r1 curve
    CASEEngineTest("Test secp224r1")
        .ProposedCurve(kWeaveCurveId_secp224r1)
#if !WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
        .ExpectError("Initiator:GenerateBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
#endif
        .Run();

    // Test prime256v1 curve
    CASEEngineTest("Test prime256v1")
        .ProposedCurve(kWeaveCurveId_prime256v1)
#if !WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
        .ExpectError("Initiator:GenerateBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
#endif
        .Run();
}

void CASEEngineTests_ConfigNegotiationTests()
{
#if WEAVE_CONFIG_SUPPORT_CASE_CONFIG1

    // Initiator only supports Config1, responder supports Config1 and Config2, expect use of Config1
    CASEEngineTest("Config1-only Initiator")
        .ProposedConfig(kCASEConfig_Config1)
        .InitiatorAllowedConfigs(kCASEAllowedConfig_Config1)
        .Run();

    // Initiator proposes Config1 but supports Config2, responder only supports Config1, expect use of Config1
    CASEEngineTest("Config1-only Responder")
        .ProposedConfig(kCASEConfig_Config1)
        .ResponderAllowedConfigs(kCASEAllowedConfig_Config1)
        .Run();

    // Initiator only supports Config2, Responder supports Config1 and Config2, expect use of Config2
    CASEEngineTest("Config2-only initiator")
        .ProposedConfig(kCASEConfig_Config2)
        .InitiatorAllowedConfigs(kCASEAllowedConfig_Config2)
        .Run();

    // Initiator proposes Config1 but supports Config2, Responder only supports Config2, expect reconfig to Config2
    CASEEngineTest("Config2-only responder")
        .ProposedConfig(kCASEConfig_Config1)
        .ResponderAllowedConfigs(kCASEAllowedConfig_Config2)
        .ExpectReconfig(kCASEConfig_Config2)
        .Run();

    // Initiator proposes Config1 but supports Config2, Responder supports Config1 and Config2, expect reconfig to Config2
    CASEEngineTest("Reconfig to Config2")
        .ProposedConfig(kCASEConfig_Config1)
        .ExpectReconfig(kCASEConfig_Config2)
        .Run();

    // Initiator proposes Config2 but supports Config1, Responder only supports Config1, expect reconfig to Config1
    CASEEngineTest("Reconfig to Config1")
        .ProposedConfig(kCASEConfig_Config2)
        .ResponderAllowedConfigs(kCASEAllowedConfig_Config1)
        .ExpectReconfig(kCASEConfig_Config1)
        .Run();

    // Initiator only supports Config1, responder only supports Config2, expect error
    CASEEngineTest("No Common Configs 1")
        .ProposedConfig(kCASEConfig_Config1)
        .InitiatorAllowedConfigs(kCASEAllowedConfig_Config1)
        .ResponderAllowedConfigs(kCASEAllowedConfig_Config2)
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION)
        .Run();

    // Initiator only supports Config2, responder only supports Config1, expect error
    CASEEngineTest("No Common Configs 2")
        .ProposedConfig(kCASEConfig_Config2)
        .InitiatorAllowedConfigs(kCASEAllowedConfig_Config2)
        .ResponderAllowedConfigs(kCASEAllowedConfig_Config1)
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION)
        .Run();

    // Responder repeatedly sends Reconfigure, expect initiator error
    CASEEngineTest("Repeated reconfigs")
        .ProposedConfig(kCASEConfig_Config1)
        .ExpectReconfig(kCASEConfig_Config2)
        .ForceRepeatedReconfig(true)
        .ExpectError("Initiator:ProcessReconfigure", WEAVE_ERROR_TOO_MANY_CASE_RECONFIGURATIONS)
        .Run();

#endif // WEAVE_CONFIG_SUPPORT_CASE_CONFIG1
}

void CASEEngineTests_CurveNegotiationTests()
{
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1 && WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1 && WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1

    // Initiator proposes prime192v1 and supports secp224r1, responder supports secp224r1 and prime256v1, expect reconfig to secp224r1
    CASEEngineTest("Reconfig to common curve")
        .ProposedCurve(kWeaveCurveId_prime192v1)
        .InitiatorAllowedCurves(kWeaveCurveSet_prime192v1|kWeaveCurveSet_secp224r1)
        .ResponderAllowedCurves(kWeaveCurveSet_secp224r1|kWeaveCurveSet_prime256v1)
        .ExpectReconfigCurve(kWeaveCurveId_secp224r1)
        .Run();

    // Initiator only supports secp224r1, responder only supports prime256v1, expect error
    CASEEngineTest("No common curves")
        .ProposedCurve(kWeaveCurveId_secp224r1)
        .InitiatorAllowedCurves(kWeaveCurveSet_secp224r1)
        .ResponderAllowedCurves(kWeaveCurveSet_prime256v1)
        .ExpectError("Responder:ProcessBeginSessionRequest", WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE)
        .Run();

#endif
}

void CASEEngineTests_KeyConfirmationTests()
{
    // Initiator does not request key confirmation.
    CASEEngineTest("No initiator key confirm")
        .InitiatorRequestKeyConfirm(false)
        .Run();

    // Initiator does not request key confirmation, but responder requires it.
    CASEEngineTest("Responder requires key confirm")
        .InitiatorRequestKeyConfirm(false)
        .ResponderRequiresKeyConfirm(true)
        .Run();
}

uint32_t gFuzzTestDurationSecs = 5;

void CASEEngineTests_FuzzTests()
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
            CASEEngineTest("Mutate BeginSessionRequest")
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
            CASEEngineTest("Mutate BeginSessionResponse")
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
            CASEEngineTest("Mutate InitiatorKeyConfirm")
                .Mutator(&fuzzer)
                .InitiatorRequestKeyConfirm(true)
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

    CASEEngineTests_BasicTests();
    CASEEngineTests_EllipticCurveTests();
    CASEEngineTests_ConfigNegotiationTests();
    CASEEngineTests_CurveNegotiationTests();
    CASEEngineTests_KeyConfirmationTests();
    CASEEngineTests_FuzzTests();

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

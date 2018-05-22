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
 *      A library for manipulating PASE state for testing and fuzzing
 *
 */


#include <stdio.h>
#include <vector>

#include "ToolCommon.h"
#include "PASEEngineTest.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePASE.h>
#include <Weave/Support/RandUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;
using System::PacketBuffer;

#define TOOL_NAME "TestPASE"
const char * INITIATOR_STEP_1 = "InitiatorStep1";
const char * RESPONDER_RECONFIGURE = "ResponderReconfigure";
const char * RESPONDER_STEP_1 = "ResponderStep1";
const char * RESPONDER_STEP_2 = "ResponderStep2";
const char * INITIATOR_STEP_2 = "InitiatorStep2";
const char * RESPONDER_KEY_CONFIRM = "ResponderKeyConfirm";

#define VerifyOrQuit(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        exit(-1); \
    } \
} while (0)

#define SuccessOrQuit(ERR, MSG) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        fputs(ErrorStr(ERR), stderr); \
        fputs("\n", stderr); \
        exit(-1); \
    } \
} while (0)

void MessageMutator::MutateMessage(const char *msgName, PacketBuffer *msgBuf) { }

static MessageMutator gNullMutator;

MessageExternalFuzzer::MessageExternalFuzzer(const char *msgType)
{
    mMsgType = msgType;
    mSaveCorpus = false;
}
void MessageExternalFuzzer::MutateMessage(const char *msgType, PacketBuffer *msgBuf)
{
    if (strcmp(msgType, mMsgType) == 0)
    {
        uint8_t *msgStart = msgBuf->Start();
        if (mSaveCorpus)
        {
            MessageExternalFuzzer::SaveCorpus(msgStart, msgBuf->DataLength(), mMsgType);
        }
        msgBuf->SetDataLength(mFuzzInputSize);
        memcpy(msgStart, mFuzzInput, msgBuf->DataLength());
    }
}

void MessageExternalFuzzer::SaveCorpus(const uint8_t *inBuf, size_t size, const char *fileName)
{
    FILE* file = fopen(fileName, "wb+" );
    VerifyOrQuit(file != NULL, "Could not open file");
    VerifyOrQuit((fwrite(inBuf, 1, size, file) == size), "Could not write corpus file.");
    fclose(file);
}

MessageExternalFuzzer& MessageExternalFuzzer::SaveCorpusFile(bool val) { mSaveCorpus = val; return *this; }

MessageExternalFuzzer& MessageExternalFuzzer::FuzzInput(const uint8_t *val, size_t size) { mFuzzInput = val; mFuzzInputSize = size; return *this; }

//Start PASEEngineTest Initialization
PASEEngineTest::PASEEngineTest(const char *testName)
{
    mTestName = testName;
    mProposedConfig = mExpectedConfig = kPASEConfig_Unspecified;
    mInitPW = mRespPW = "TestPassword";
    mInitiatorAllowedConfigs = mResponderAllowedConfigs = kPASEConfig_Config1|kPASEConfig_Config4;
    mExpectReconfig = false;
    mForceRepeatedReconfig = false;
    memset(mExpectedErrors, 0, sizeof(mExpectedErrors));
    mMutator = &gNullMutator;
    mLogMessageData = false;
}

const char *PASEEngineTest::TestName() const { return mTestName; }

uint32_t PASEEngineTest::ProposedConfig() const { return mProposedConfig; }
PASEEngineTest& PASEEngineTest::ProposedConfig(uint32_t val) { mProposedConfig = val; return *this; }

uint32_t PASEEngineTest::InitiatorAllowedConfigs() const { return mInitiatorAllowedConfigs; }
PASEEngineTest& PASEEngineTest::InitiatorAllowedConfigs(uint32_t val) { mInitiatorAllowedConfigs = val; return *this; }

uint32_t PASEEngineTest::ResponderAllowedConfigs() const { return mResponderAllowedConfigs; }
PASEEngineTest& PASEEngineTest::ResponderAllowedConfigs(uint32_t val) { mResponderAllowedConfigs = val; return *this; }

const char* PASEEngineTest::InitiatorPassword() const { return mInitPW; }
PASEEngineTest& PASEEngineTest::InitiatorPassword(const char* val) { mInitPW = val; return *this; }

const char* PASEEngineTest::ResponderPassword() const { return mRespPW; }
PASEEngineTest& PASEEngineTest::ResponderPassword(const char* val) { mRespPW = val; return *this; }

uint32_t PASEEngineTest::ExpectReconfig() const { return mExpectReconfig; }
PASEEngineTest& PASEEngineTest::ExpectReconfig(uint32_t expectedConfig)
{
    mExpectReconfig = true;
    mExpectedConfig = expectedConfig;
    return *this;
}
uint32_t PASEEngineTest::ExpectedConfig() const { return mExpectedConfig != kPASEConfig_Unspecified ? mExpectedConfig : mProposedConfig; }

bool PASEEngineTest::PerformReconfig() const { return mForceRepeatedReconfig; }
PASEEngineTest& PASEEngineTest::PerformReconfig(bool val) { mForceRepeatedReconfig = val; return *this; }

bool PASEEngineTest::ConfirmKey() const { return mConfirmKey; }
PASEEngineTest& PASEEngineTest::ConfirmKey(bool val) { mConfirmKey = val; return *this; }

PASEEngineTest& PASEEngineTest::ExpectError(WEAVE_ERROR err)
{
    return ExpectError(NULL, err);
}

PASEEngineTest& PASEEngineTest::ExpectError(const char *opName, WEAVE_ERROR err)
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

bool PASEEngineTest::IsExpectedError(const char *opName, WEAVE_ERROR err) const
{
    for (size_t i = 0; i < kMaxExpectedErrors && mExpectedErrors[i].Error != WEAVE_NO_ERROR; i++)
    {
        if (mExpectedErrors[i].Error == err &&
            (mExpectedErrors[i].OpName == NULL || strcmp(mExpectedErrors[i].OpName, opName) == 0))
        {
            return true;
        }
    }
    return false;
}

bool PASEEngineTest::IsSuccessExpected() const { return mExpectedErrors[0].Error == WEAVE_NO_ERROR; }

PASEEngineTest& PASEEngineTest::Mutator(MessageMutator *mutator) { mMutator = mutator; return *this; }

bool PASEEngineTest::LogMessageData() const { return mLogMessageData; }
PASEEngineTest& PASEEngineTest::LogMessageData(bool val) { mLogMessageData = val; return *this; }

//private
void PASEEngineTest::setAllowedResponderConfigs(WeavePASEEngine &responderEng)
{
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
        if (mResponderAllowedConfigs == kPASEConfig_Config0_TEST_ONLY)
            responderEng.AllowedPASEConfigs = kPASEConfig_SupportConfig0Bit_TEST_ONLY;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        if (mResponderAllowedConfigs == kPASEConfig_Config1)
            responderEng.AllowedPASEConfigs = kPASEConfig_SupportConfig1Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
        if (mResponderAllowedConfigs == kPASEConfig_Config2)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig2Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
        if (mResponderAllowedConfigs == kPASEConfig_Config3)
            responderEng.AllowedPASEConfigs = kPASEConfig_SupportConfig3Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
        if (mResponderAllowedConfigs == kPASEConfig_Config4)
            responderEng.AllowedPASEConfigs = kPASEConfig_SupportConfig4Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
        if (mResponderAllowedConfigs == kPASEConfig_Config5)
            responderEng.AllowedPASEConfigs = kPASEConfig_SupportConfig5Bit;
        else
#endif
            responderEng.AllowedPASEConfigs = 0x0;
}

enum
{
    kMaxExpectedErrors = 32
};

//end PASEEngineTest Initialization
void PASEEngineTest::Run()
{
    WEAVE_ERROR err;
    WeavePASEEngine initiatorEng;
    WeavePASEEngine responderEng;
    PacketBuffer *msgBuf = NULL;
    PacketBuffer *msgBuf2 = NULL;
    WeaveFabricState initFabricState;
    WeaveFabricState respFabricState;
    const WeaveEncryptionKey *initiatorKey;
    const WeaveEncryptionKey *responderKey;

    uint64_t initNodeId = 1;
    uint64_t respNodeId = 2;
    uint16_t sessionKeyId = sTestDefaultSessionKeyId;
    uint16_t encType = kWeaveEncryptionType_AES128CTRSHA1;
    uint16_t pwSrc = kPasswordSource_PairingCode;
    bool expectSuccess = strcmp(mInitPW, mRespPW) == 0;

    if (LogMessageData())
    {
        printf("========== Starting Test: %s\n", TestName());
        printf("Pr: %d\nex: %d\n", ProposedConfig(), ExpectedConfig());
    }
    initiatorEng.Init();
    err = initFabricState.Init();
    SuccessOrQuit(err, "initFabricState.Init failed\n");
    initiatorEng.Pw = (const uint8_t *)mInitPW;
    initiatorEng.PwLen = (uint16_t)strlen(mInitPW);

onReconfig:
    responderEng.Init();
    err = respFabricState.Init();
    setAllowedResponderConfigs(responderEng);

    SuccessOrQuit(err, "respFabricState.Init failed\n");
    respFabricState.PairingCode = mRespPW;

    // =========== Start PASE InitiatorStep1 ==============================
    msgBuf = PacketBuffer::New();
    VerifyOrQuit(msgBuf != NULL, "PacketBuffer::New() failed");

    // Initiator generates and sends PASE Initiator Step 1 message.
    err = initiatorEng.GenerateInitiatorStep1(msgBuf, ProposedConfig(), initNodeId, respNodeId, sessionKeyId, encType, pwSrc, &initFabricState, mConfirmKey);

    if (IsExpectedError("Initiator:GenerateInitiatorStep1", err))
        goto onExpectedError;
    SuccessOrQuit(err, "WeavePASEEngine::GenerateInitiatorStep1 failed\n");

    // =========== Initiator Sends InitiatorStep1 to Responder ============

    mMutator->MutateMessage(INITIATOR_STEP_1, msgBuf);
    if (LogMessageData())
    {
        printf("Initiator->Responder: InitiatorStep1 Message (%d bytes)\n", msgBuf->DataLength());
        DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
    }

    // =========== Responder Processes PASE InitiatorStep1 ================
    err = responderEng.ProcessInitiatorStep1(msgBuf, respNodeId, initNodeId, &respFabricState);
    if (IsExpectedError(INITIATOR_STEP_1, err))
        goto onExpectedError;

    if (ExpectReconfig())
    {
        VerifyOrQuit(err == WEAVE_ERROR_PASE_RECONFIGURE_REQUIRED, "WEAVE_ERROR_PASE_RECONFIG_REQUIRED error expected");
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        // =========== Responder generates PASE ResponderReconfigMessage ==
        {
            msgBuf = PacketBuffer::New();
            err = responderEng.GenerateResponderReconfigure(msgBuf);
            SuccessOrQuit(err, "WeavePASEEngine::GenerateResponderReconfigure failed\n");
            // Reset PASE Engines
            responderEng.Reset();
        }
            // ========== Responder sends ResponderReconfig Message ============
        mMutator->MutateMessage(RESPONDER_RECONFIGURE, msgBuf);

        if (LogMessageData())
        {
            printf("Responder->Initiator: ResponderReconfig Message (%d bytes)\n", msgBuf->DataLength());
            DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
        }

        // =========== Initiator processes PASE ResponderReconfig =========
        {
            uint32_t tempProposedConfig = mProposedConfig;
            err = initiatorEng.ProcessResponderReconfigure(msgBuf, mProposedConfig);
            if (IsExpectedError("Initiator:ProcessResponderReconfigure", err))
            {
                mProposedConfig = tempProposedConfig;
                goto onExpectedError;
            }
            SuccessOrQuit(err, "WeavePASEEngine::ProcessResponderReconfigure failed\n");
            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }

        respFabricState.Shutdown();
        mExpectReconfig = false;

        goto onReconfig;

    } else {
        VerifyOrQuit(err != WEAVE_ERROR_PASE_RECONFIGURE_REQUIRED, "Unexpected reconfig!");
    }

    SuccessOrQuit(err, "WeavePASEEngine::ProcessInitiatorStep1 failed\n");
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    // =========== Responder Generates ResponderStep1 and ResponderStep2 ==
    {
        msgBuf = PacketBuffer::New();
        err = responderEng.GenerateResponderStep1(msgBuf);
        SuccessOrQuit(err, "WeavePASEEngine::GenerateResponderStep1 failed\n");

        // Responder generates and sends PASE Responder Step 2 message.
        msgBuf2 = PacketBuffer::New();
        err = responderEng.GenerateResponderStep2(msgBuf2);
        SuccessOrQuit(err, "WeavePASEEngine::GenerateResponderStep2 failed\n");
    }
    // =========== Responder sends ResponderStep1 ==========================
    mMutator->MutateMessage(RESPONDER_STEP_1, msgBuf);

    if (LogMessageData())
    {
        printf("Responder->Initiator: ResponderStep1 Message (%d bytes)\n", msgBuf->DataLength());
        DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
    }

    // =========== Responder sends ResponderStep2 ==========================
    mMutator->MutateMessage(RESPONDER_STEP_2, msgBuf2);

    if (LogMessageData())
    {
        printf("Responder->Initiator: ResponderStep2 Message (%d bytes)\n", msgBuf2->DataLength());
        DumpMemory(msgBuf2->Start(), msgBuf2->DataLength(), "  ", 16);
    }

    // =========== Initator Parses ResponderStep1 and ResponderStep2 ======
    {
        // Initiator receives and processes PASE Responder Step 1 message.
        err = initiatorEng.ProcessResponderStep1(msgBuf);
        if (IsExpectedError(RESPONDER_STEP_1, err))
            goto onExpectedError;
        SuccessOrQuit(err, "WeavePASEEngine::ProcessResponderStep1 failed\n");
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        // Initiator receives and processes PASE Responder Step 2 message.
        err = initiatorEng.ProcessResponderStep2(msgBuf2);
        if (IsExpectedError(RESPONDER_STEP_2, err))
            goto onExpectedError;
        SuccessOrQuit(err, "WeavePASEEngine::ProcessResponderStep2 failed\n");
        PacketBuffer::Free(msgBuf2);
        msgBuf2 = NULL;
    }

    // =========== Initator Generates InitatorStep2 ===========================
    {
        msgBuf = PacketBuffer::New();
        err = initiatorEng.GenerateInitiatorStep2(msgBuf);
        SuccessOrQuit(err, "WeavePASEEngine::GenerateInitiatorStep2 failed\n");
    }
    //=========== Initator Sends InitatorStep2 ============================

    mMutator->MutateMessage(INITIATOR_STEP_2, msgBuf);

    if (LogMessageData())
    {
        printf("Initiator->Responder: InitatorStep2 Message (%d bytes)\n", msgBuf->DataLength());
        DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
    }

    // =========== Responder Parses InitatorStep2 =========================
    {
        err = responderEng.ProcessInitiatorStep2(msgBuf);
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;

        if (IsExpectedError(INITIATOR_STEP_2, err))
            goto onExpectedError;

        if (expectSuccess)
            SuccessOrQuit(err, "WeavePASEEngine::ProcessInitiatorStep2 failed\n");
        else if (mConfirmKey)
        {
            VerifyOrQuit(err == WEAVE_ERROR_KEY_CONFIRMATION_FAILED, "Expected error from WeavePASEEngine::ProcessInitiatorStep2\n");
            return;
        }
    }

    if (mConfirmKey)
    {
        // ========== Responder Forms ResponderKeyConfirm =================
        {
            msgBuf = PacketBuffer::New();
            err = responderEng.GenerateResponderKeyConfirm(msgBuf);
            SuccessOrQuit(err, "WeavePASEEngine::GenerateResponderKeyConfirm failed\n");
        }

        // ========== Responder Sends ResponderKeyConfirm to Responder ====

        mMutator->MutateMessage(RESPONDER_KEY_CONFIRM, msgBuf);

        if (LogMessageData())
        {
            printf("Responder->Initiator: ResponderKeyConfirm Message (%d bytes)\n", msgBuf->DataLength());
            DumpMemory(msgBuf->Start(), msgBuf->DataLength(), "  ", 16);
        }

        // ========== Initiator Processes ResponderKeyConfirm =============
        {
            err = initiatorEng.ProcessResponderKeyConfirm(msgBuf);

            if (IsExpectedError(RESPONDER_KEY_CONFIRM, err))
                goto onExpectedError;

            SuccessOrQuit(err, "WeavePASEEngine::ProcessResponderKeyConfirm failed\n");
            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;
        }
    }

    VerifyOrQuit(initiatorEng.State == WeavePASEEngine::kState_InitiatorDone, "Initiator state != Done\n");
    VerifyOrQuit(responderEng.State == WeavePASEEngine::kState_ResponderDone, "Responder state != Done\n");

    VerifyOrQuit(initiatorEng.SessionKeyId == responderEng.SessionKeyId, "Initiator SessionKeyId != Responder SessionKeyId\n");
    VerifyOrQuit(initiatorEng.EncryptionType == responderEng.EncryptionType, "Initiator EncryptionType != Responder EncryptionType\n");
    VerifyOrQuit(initiatorEng.PerformKeyConfirmation == responderEng.PerformKeyConfirmation, "Initiator SessionKeyId != Responder SessionKeyId\n");

    err = initiatorEng.GetSessionKey(initiatorKey);
    SuccessOrQuit(err, "WeavePASEEngine::GetSessionKey() failed\n");

    err = responderEng.GetSessionKey(responderKey);
    SuccessOrQuit(err, "WeavePASEEngine::GetSessionKey() failed\n");

    VerifyOrQuit(memcmp(initiatorKey->AES128CTRSHA1.DataKey, responderKey->AES128CTRSHA1.DataKey, WeaveEncryptionKey_AES128CTRSHA1::DataKeySize) == 0,
                 "Data key mismatch\n");
    VerifyOrQuit(memcmp(initiatorKey->AES128CTRSHA1.IntegrityKey, responderKey->AES128CTRSHA1.IntegrityKey, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize) == 0,
                 "Integrity key mismatch\n");

    // Shutdown the Initiator/Responder FabricState objects
    err = initFabricState.Shutdown();
    SuccessOrQuit(err, "initFabricState.Shutdown failed\n");
    err = respFabricState.Shutdown();
    SuccessOrQuit(err, "respFabricState.Shutdown failed\n");

onExpectedError:
    PacketBuffer::Free(msgBuf);
    msgBuf = NULL;

    PacketBuffer::Free(msgBuf2);
    msgBuf2 = NULL;

    initiatorEng.Shutdown();
    responderEng.Shutdown();
    initFabricState.Shutdown();
    respFabricState.Shutdown();
    if (LogMessageData())
        printf("Test Complete: %s\n", TestName());
}

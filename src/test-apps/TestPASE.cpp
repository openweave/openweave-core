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
 *      This file implements a process to effect a functional test for
 *      the Weave Password Authenticated Session Establishment (PASE)
 *      protocol engine.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ToolCommon.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePASE.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;

#define DEBUG_PRINT_ENABLE 0
#define DEBUG_PRINT_MESSAGE_LENGTH 0

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        exit(-1); \
    } \
} while (0)

#define SuccessOrFail(ERR, MSG) \
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


void TestPASEEngine(const char *initPW, const char *respPW, uint32_t kPASEConfig, bool confirmKey, bool reconfigureTest)
{
    static WeavePASEEngine initEng;
    static WeavePASEEngine respEng;
    static WeaveFabricState initFabricState;
    static WeaveFabricState respFabricState;

    WEAVE_ERROR err;
    uint64_t initNodeId = 1;
    uint64_t respNodeId = 2;
    uint16_t sessionKeyId = sTestDefaultSessionKeyId;
    uint16_t encType = kWeaveEncryptionType_AES128CTRSHA1;
    uint16_t pwSrc = kPasswordSource_PairingCode;
    PacketBuffer *encodedMsg, *encodedMsg2;
    uint8_t reconfigureRequired = 0;
    bool expectSuccess = strcmp(initPW, respPW) == 0;
    const WeaveEncryptionKey *initSessionKey;
    const WeaveEncryptionKey *respSessionKey;

    // Zero static memory
    memset(&initEng, 0, sizeof initEng);
    memset(&respEng, 0, sizeof respEng);
    memset(&initFabricState, 0, sizeof initFabricState);
    memset(&respFabricState, 0, sizeof respFabricState);

    // Init PASE Engines
    initEng.Init();
    respEng.Init();

    // Initialize the Initiator/Responder FabricState objects
    err = initFabricState.Init();
    SuccessOrFail(err, "initFabricState.Init failed\n");
    err = respFabricState.Init();
    SuccessOrFail(err, "respFabricState.Init failed\n");

    // Set Initiator Pairing Password
    initEng.Pw = (const uint8_t *)initPW;
    initEng.PwLen = (uint16_t)strlen(initPW);

    // Set Responder Pairing Password
    respFabricState.PairingCode = respPW;

    // Allow only one congif option for Responder if we don't want to test Reconfigure Request
    if (!reconfigureTest)
    {
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
        if (kPASEConfig == kPASEConfig_Config0_TEST_ONLY)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig0Bit_TEST_ONLY;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
        if (kPASEConfig == kPASEConfig_Config1)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig1Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
        if (kPASEConfig == kPASEConfig_Config2)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig2Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
        if (kPASEConfig == kPASEConfig_Config3)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig3Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
        if (kPASEConfig == kPASEConfig_Config4)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig4Bit;
        else
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
        if (kPASEConfig == kPASEConfig_Config5)
            respEng.AllowedPASEConfigs = kPASEConfig_SupportConfig5Bit;
        else
#endif
            respEng.AllowedPASEConfigs = 0x0;
    }

    // Initiator generates and sends PASE Initiator Step 1 message.
    {
        encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateInitiatorStep1 (Started):\n");
#endif
        err = initEng.GenerateInitiatorStep1(encodedMsg, kPASEConfig, initNodeId, respNodeId, sessionKeyId, encType, pwSrc, &initFabricState, confirmKey);
        SuccessOrFail(err, "WeavePASEEngine::GenerateInitiatorStep1 failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
	fprintf(stdout, "GenerateInitiatorStep1: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateInitiatorStep1 (Finished):\n");
#endif
    }

    // Responder receives and processes PASE Initiator Step 1 message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessInitiatorStep1 (Started):\n");
#endif

        err = respEng.ProcessInitiatorStep1(encodedMsg, respNodeId, initNodeId, &respFabricState);
	if (err == WEAVE_ERROR_PASE_RECONFIGURE_REQUIRED)
            reconfigureRequired = 1;
	else
            SuccessOrFail(err, "WeavePASEEngine::ProcessInitiatorStep1 failed\n");

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessInitiatorStep1 (Finished):\n");
#endif
        PacketBuffer::Free(encodedMsg);
        encodedMsg = NULL;
    }

    if (reconfigureRequired)
    {
        // Responder generates and sends PASE Responder Reconfig Message
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateResponderReconfigure (Started):\n");
#endif

            err = respEng.GenerateResponderReconfigure(encodedMsg);
            SuccessOrFail(err, "WeavePASEEngine::GenerateResponderReconfigure failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateResponderReconfigure: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateResponderReconfigure (Finished):\n");
#endif
            // Reset PASE Engines
            respEng.Reset();
        }

        // Initiator receives and processes PASE Responder Reconfig Message
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessResponderReconfigure (Started):\n");
#endif

            err = initEng.ProcessResponderReconfigure(encodedMsg, kPASEConfig);
            SuccessOrFail(err, "WeavePASEEngine::ProcessResponderReconfigure failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessResponderReconfigure (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }

        // Initiator generates and sends PASE Initiator Step 1 message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateInitiatorStep1 (Started):\n");
#endif
            err = initEng.GenerateInitiatorStep1(encodedMsg, kPASEConfig, initNodeId, respNodeId, sessionKeyId, encType, pwSrc, &initFabricState, confirmKey);
            SuccessOrFail(err, "WeavePASEEngine::GenerateInitiatorStep1 failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
	    fprintf(stdout, "GenerateInitiatorStep1: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateInitiatorStep1 (Finished):\n");
#endif
        }

        // Responder receives and processes PASE Initiator Step 1 message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessInitiatorStep1 (Started):\n");
#endif

	    err = respEng.ProcessInitiatorStep1(encodedMsg, respNodeId, initNodeId, &respFabricState);
            SuccessOrFail(err, "WeavePASEEngine::ProcessInitiatorStep1 failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessInitiatorStep1 (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }

    }

    // Responder generates and sends PASE Responder Step 1 message.
    {
        encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateResponderStep1 (Started):\n");
#endif
        err = respEng.GenerateResponderStep1(encodedMsg);
        SuccessOrFail(err, "WeavePASEEngine::GenerateResponderStep1 failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
	fprintf(stdout, "GenerateResponderStep1: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateResponderStep1 (Finished):\n");
#endif
    }

    // Responder generates and sends PASE Responder Step 2 message.
    {
        encodedMsg2 = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateResponderStep2 (Started):\n");
#endif

        err = respEng.GenerateResponderStep2(encodedMsg2);
        SuccessOrFail(err, "WeavePASEEngine::GenerateResponderStep2 failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
	fprintf(stdout, "GenerateResponderStep2: Message Size = %d \n", encodedMsg2->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateResponderStep2 (Finished):\n");
#endif
    }

    // Initiator receives and processes PASE Responder Step 1 message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessResponderStep1 (Started):\n");
#endif

        err = initEng.ProcessResponderStep1(encodedMsg);
        SuccessOrFail(err, "WeavePASEEngine::ProcessResponderStep1 failed\n");

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessResponderStep1 (Finished):\n");
#endif
        PacketBuffer::Free(encodedMsg);
        encodedMsg = NULL;
    }

    // Initiator receives and processes PASE Responder Step 2 message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessResponderStep2 (Started):\n");
#endif

        err = initEng.ProcessResponderStep2(encodedMsg2);
        SuccessOrFail(err, "WeavePASEEngine::ProcessResponderStep2 failed\n");

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessResponderStep2 (Finished):\n");
#endif
        PacketBuffer::Free(encodedMsg2);
        encodedMsg2 = NULL;
    }

    // Initiator generates and sends PASE Initiator Step 2 message.
    {
        encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateInitiatorStep2 (Started):\n");
#endif

        err = initEng.GenerateInitiatorStep2(encodedMsg);
        SuccessOrFail(err, "WeavePASEEngine::GenerateInitiatorStep2 failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
	fprintf(stdout, "GenerateInitiatorStep2: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateInitiatorStep2 (Finished):\n");
#endif
    }

    // Responder receives and processes PASE Initiator Step 2 message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessInitiatorStep2 (Started):\n");
#endif

        err = respEng.ProcessInitiatorStep2(encodedMsg);
        PacketBuffer::Free(encodedMsg);
        encodedMsg = NULL;

        if (expectSuccess)
            SuccessOrFail(err, "WeavePASEEngine::ProcessInitiatorStep2 failed\n");
        else if (confirmKey)
        {
            VerifyOrFail(err == WEAVE_ERROR_KEY_CONFIRMATION_FAILED, "Expected error from WeavePASEEngine::ProcessInitiatorStep2\n");
            return;
        }

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessInitiatorStep2 (Finished):\n");
#endif
    }

    if (confirmKey)
    {
        // Responder generates and sends PASE Responder Key Confirmation message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateResponderKeyConfirm (Started):\n");
#endif
            err = respEng.GenerateResponderKeyConfirm(encodedMsg);
            SuccessOrFail(err, "WeavePASEEngine::GenerateResponderKeyConfirm failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateResponderKeyConfirm:   Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateResponderKeyConfirm (Finished):\n");
#endif
        }

        // Initiator receives and processes PASE Responder Key Confirmation message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessResponderKeyConfirm (Started):\n");
#endif

            err = initEng.ProcessResponderKeyConfirm(encodedMsg);
            SuccessOrFail(err, "WeavePASEEngine::ProcessResponderKeyConfirm failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessResponderKeyConfirm (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }
    }

    VerifyOrFail(initEng.State == WeavePASEEngine::kState_InitiatorDone, "Initiator state != Done\n");
    VerifyOrFail(respEng.State == WeavePASEEngine::kState_ResponderDone, "Responder state != Done\n");

    VerifyOrFail(initEng.SessionKeyId == respEng.SessionKeyId, "Initiator SessionKeyId != Responder SessionKeyId\n");
    VerifyOrFail(initEng.EncryptionType == respEng.EncryptionType, "Initiator EncryptionType != Responder EncryptionType\n");
    VerifyOrFail(initEng.PerformKeyConfirmation == respEng.PerformKeyConfirmation, "Initiator SessionKeyId != Responder SessionKeyId\n");

    err = initEng.GetSessionKey(initSessionKey);
    SuccessOrFail(err, "WeaveCASEEngine::GetSessionKey() failed\n");

    err = respEng.GetSessionKey(respSessionKey);
    SuccessOrFail(err, "WeaveCASEEngine::GetSessionKey() failed\n");

    VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.DataKey, respSessionKey->AES128CTRSHA1.DataKey, WeaveEncryptionKey_AES128CTRSHA1::DataKeySize) == 0,
                 "Data key mismatch\n");

    VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.IntegrityKey, respSessionKey->AES128CTRSHA1.IntegrityKey, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize) == 0,
                 "Integrity key mismatch\n");

    // Shutdown the Initiator/Responder FabricState objects
    err = initFabricState.Shutdown();
    SuccessOrFail(err, "initFabricState.Shutdown failed\n");
    err = respFabricState.Shutdown();
    SuccessOrFail(err, "respFabricState.Shutdown failed\n");

    // Shutdown PASE Engines
    respEng.Shutdown();
    initEng.Shutdown();
}

void PASEEngine_Test1(uint32_t kPASEConfig)
{
    TestPASEEngine("TestPassword", "TestPassword", kPASEConfig, true, false);
}

void PASEEngine_Test2(uint32_t kPASEConfig)
{
    TestPASEEngine("TestPassword", "TestPassword", kPASEConfig, false, false);
}

void PASEEngine_Test3(uint32_t kPASEConfig)
{
    TestPASEEngine("TestPassword", "TestwordPass", kPASEConfig, true, false);
}

void PASEEngine_Test4(uint32_t kPASEConfig)
{
    TestPASEEngine("TestPassword", "TestPassword", kPASEConfig, true, true);
}

// #if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1

// void PASEEngine_Test4()
// {
//     WEAVE_ERROR err;
//     WeavePASEEngine eng;
//     uint64_t initNodeId = 2;
//     uint64_t respNodeId = 0x18B43000001E8775ULL;
//     uint16_t sessionKeyId = WeaveKeyId::kType_Session | 0;
//     uint16_t encType = kWeaveEncryptionType_AES128CTRSHA1;
//     uint16_t pwSrc = kPasswordSource_PairingCode;
//     const char *pw = "TEST";
//     bool confirmKey = true;

//     {
//         static BN_ULONG sStep1_GXa_Data[] =
//         {
//             0x489074D4, 0x1193F992, 0x91F2366A, 0xDE0D0674, 0x1F0B4EAA, 0x1CACC906, 0x8431D6B8, 0x1BA5CF84,
//             0xAFB7CFFC, 0x1C0CD0B2, 0x30107FCD, 0x6C1F82BD, 0x60E4D718, 0x147B3479, 0x1FE9BC5D, 0x72A16982,
//             0xC4A7BD74, 0x694E53B4, 0x6BDD1919, 0xC04FF3CA, 0xF3F699CA, 0x0AAC83EE, 0xCD62B502, 0xD17926E4,
//             0x6464227B, 0x656C08C7, 0xA84E3742, 0x065EA552, 0x27D150CA, 0x5B842DF0, 0x92CF93D3, 0x19A0D1D6
//         };
//         static BIGNUM sStep1_GXa =
//         {
//             sStep1_GXa_Data,
//             sizeof(sStep1_GXa_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_GXa_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

//         static BN_ULONG sStep1_GXb_Data[] =
//         {
//             0x81E18A01, 0x713C878C, 0x962F52CA, 0xF24B104A, 0x9881D2FD, 0x9DEEDB68, 0x7EB7C7BF, 0xA41EB45D,
//             0x75D130C6, 0xEED03DCA, 0xA579049F, 0xACF81B96, 0x7EE5C37F, 0xFA0B9F7B, 0x00F249E4, 0x6077423F,
//             0x41E2884A, 0x12901333, 0xE70D84B5, 0x89E2156D, 0x20BC2FA3, 0x6CD51881, 0xC5D0B8FC, 0x600FEC4A,
//             0x1FFA2B28, 0xA7ADE0D3, 0x9EFAF283, 0x58F90D59, 0xA5C38799, 0x7DDC9026, 0x5CEAC89E, 0xA6F89527
//         };
//         static BIGNUM sStep1_GXb =
//         {
//             sStep1_GXb_Data,
//             sizeof(sStep1_GXb_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_GXb_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

//         static BN_ULONG sStep1_ZKPXaGR_Data[] =
//         {
//             0x75269981, 0xB55D5588, 0xA0B51CD6, 0x24E8CBF0, 0x159C04A9, 0xB6E106E7, 0xDEDA34FE, 0xAE7E14BB,
//             0x654C34F6, 0x395420B9, 0x79AA6244, 0x3CBBD127, 0x707BFBF9, 0x5D0C8931, 0xD798146B, 0x41EAC306,
//             0xED825191, 0x63222B03, 0xAC9C8870, 0x40F7A4B7, 0x237D25CF, 0x7828CBB8, 0x3C6D9E88, 0xC2D1C807,
//             0x7730F83F, 0x782E3935, 0x99FD5A5E, 0x47F8D507, 0x5B36F070, 0xEA29F69E, 0x2F64AD00, 0x15DC1EBC
//         };
//         static BIGNUM sStep1_ZKPXaGR =
//         {
//             sStep1_ZKPXaGR_Data,
//             sizeof(sStep1_ZKPXaGR_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_ZKPXaGR_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

//         static BN_ULONG sStep1_ZKPXaB_Data[] =
//         {
//             0x108B6CF1, 0x92CD53F9, 0x8A01E0D0, 0x1FAC5D41, 0x6315ED77
//         };
//         static BIGNUM sStep1_ZKPXaB =
//         {
//             sStep1_ZKPXaB_Data,
//             sizeof(sStep1_ZKPXaB_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_ZKPXaB_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

//         static BN_ULONG sStep1_ZKPXbGR_Data[] =
//         {
//             0xD21EF93B, 0x84F3FD15, 0x764815C5, 0x7FE5C7F9, 0x669FE0F2,
//             0x21760B13, 0x26550314, 0x0B50ED64, 0xF6100E47, 0x96C99FB7,
//             0x61DD062C, 0x08724518, 0xBE3AC240, 0x50DE63FF, 0x6819D65C,
//             0xE67ED248, 0x0D196466, 0x1A5F64EC, 0x954855F9, 0x6CB8DCD0,
//             0x8E08B4A2, 0x103F04B3, 0xC59FA937, 0x48D363A8, 0x98F686CA,
//             0x9F29FA89, 0xBC87A0C2, 0xD185017B, 0xAA4A469D, 0x4329CBC0,
//             0x6863E791, 0xBAE0CE9C
//         };
//         static BIGNUM sStep1_ZKPXbGR =
//         {
//             sStep1_ZKPXbGR_Data,
//             sizeof(sStep1_ZKPXbGR_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_ZKPXbGR_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

//         static BN_ULONG sStep1_ZKPXbB_Data[] =
//         {
//                 0x4F113953, 0xBD0FAB7D, 0x405A9C56, 0x451A745F, 0xB061B450
//         };
//         static BIGNUM sStep1_ZKPXbB =
//         {
//             sStep1_ZKPXbB_Data,
//             sizeof(sStep1_ZKPXbB_Data)/sizeof(BN_ULONG),
//             sizeof(sStep1_ZKPXbB_Data)/sizeof(BN_ULONG),
//             0,
//             BN_FLG_STATIC_DATA
//         };

// 	//        InitiatorStep1Message initStep1Msg;
//         initStep1Msg.GXa = sStep1_GXa;
//         initStep1Msg.GXb = sStep1_GXb;
//         initStep1Msg.ZKPXaB = sStep1_ZKPXaB;
//         initStep1Msg.ZKPXaGR = sStep1_ZKPXaGR;
//         initStep1Msg.ZKPXbB = sStep1_ZKPXbB;
//         initStep1Msg.ZKPXbGR = sStep1_ZKPXbGR;
//         initStep1Msg.ProtocolConfig = kPASEConfig_Config1;
//         initStep1Msg.AlternateConfigCount = 0;
//         initStep1Msg.SessionKeyId = sessionKeyId;
//         initStep1Msg.EncryptionType = encType;
//         initStep1Msg.PasswordSource = pwSrc;
//         initStep1Msg.PerformKeyConfirmation = confirmKey;
//         PacketBuffer *BufStep1Msg = PacketBuffer::New();

//         err = eng.ProcessInitiatorStep1(BufStep1Msg, respNodeId, initNodeId, (const uint8_t *)pw, (uint16_t)strlen(pw));
//         PacketBuffer::Free(BufStep1Msg);
//         SuccessOrFail(err, "WeavePASEEngine::ProcessInitiatorStep1 failed\n");
//     }
// }
// #endif  /* WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 */

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

#define NUMBER_OF_ITERATIONS 1

    int i;
    uint32_t PASE_Configs[] = {
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG0_TEST_ONLY
                                 kPASEConfig_Config0_TEST_ONLY,
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
                                 kPASEConfig_Config1,
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG2
                                 kPASEConfig_Config2,
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG3
                                 kPASEConfig_Config3,
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG4
                                 kPASEConfig_Config4,
#endif
#if WEAVE_CONFIG_SUPPORT_PASE_CONFIG5
                                 kPASEConfig_Config5,
#endif
                               };
    time_t sec_now;
    time_t sec_last;

    for (uint8_t j = 0; j < sizeof(PASE_Configs)/sizeof(uint32_t); j++)
    {
        printf("\nTEST1 for PASE Config %08X (%d iterations)\n", PASE_Configs[j], NUMBER_OF_ITERATIONS);
	sec_last = time(NULL);
        for (i=0; i < NUMBER_OF_ITERATIONS; i++)
            PASEEngine_Test1(PASE_Configs[j]);
	sec_now = time(NULL);
        printf("TIME DELTA (sec) = %ld sec\n", (sec_now - sec_last));

        printf("\nTEST2 for PASE Config %08X (%d iterations)\n", PASE_Configs[j], NUMBER_OF_ITERATIONS);
	sec_last = time(NULL);
        for (i=0; i < NUMBER_OF_ITERATIONS; i++)
            PASEEngine_Test2(PASE_Configs[j]);
        sec_now = time(NULL);
        printf("TIME DELTA (sec) = %ld sec\n", (sec_now - sec_last));

        printf("\nTEST3 for PASE Config %08X (%d iterations)\n", PASE_Configs[j], NUMBER_OF_ITERATIONS);
	sec_last = time(NULL);
        for (i=0; i < NUMBER_OF_ITERATIONS; i++)
            PASEEngine_Test3(PASE_Configs[j]);
        sec_now = time(NULL);
        printf("TIME DELTA (sec) = %ld sec\n", (sec_now - sec_last));

        printf("\nTEST4 for PASE Config %08X (%d iterations)\n", PASE_Configs[j], NUMBER_OF_ITERATIONS);
	sec_last = time(NULL);
        for (i=0; i < NUMBER_OF_ITERATIONS; i++)
            PASEEngine_Test4(PASE_Configs[j]);
        sec_now = time(NULL);
        printf("TIME DELTA (sec) = %ld sec\n", (sec_now - sec_last));
    }
// #if WEAVE_CONFIG_SUPPORT_PASE_CONFIG1
//     PASEEngine_Test4();
// #endif
    printf("All tests succeeded\n");
}

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
 *      the Weave Token Authenticated Key Exchange (TAKE) protocol
 *      engine.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ToolCommon.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveTAKE.h>
#include <Weave/Support/crypto/AESBlockCipher.h>
#include "TAKEOptions.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Inet;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::TAKE;

#define DEBUG_PRINT_ENABLE 0
#define DEBUG_PRINT_MESSAGE_LENGTH 0

#define Clean() \
do { \
    if (encodedMsg) \
        PacketBuffer::Free(encodedMsg); \
} while (0)

#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        Clean(); \
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
        Clean(); \
        exit(-1); \
    } \
} while (0)

class TAKEConfigNoAuthorized : public MockTAKEChallengerDelegate
{
public:

    WEAVE_ERROR GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen)
    {
        tokenId = kNodeIdNotSpecified;
        return WEAVE_NO_ERROR;
    }
};

uint8_t junk0[] = { 0x6a, 0x75, 0x6e, 0x6b, 0x6a, 0x75, 0x6e, 0x6b, 0x6a, 0x75, 0x6e, 0x6b, 0x6a, 0x75, 0x6e, 0x6b };
uint8_t junk1[] = { 0x74, 0x72, 0x61, 0x73, 0x68, 0x74, 0x72, 0x61, 0x73, 0x68, 0x74, 0x72, 0x61, 0x73, 0x68, 0x74 };
uint8_t junk2[] = { 0x74, 0x68, 0x69, 0x73, 0x69, 0x73, 0x61, 0x70, 0x69, 0x6c, 0x65, 0x6f, 0x73, 0x68, 0x69, 0x74 };

class TAKEConfigJunkAuthorized : public MockTAKEChallengerDelegate
{
    int position;
public:

    TAKEConfigJunkAuthorized() : position(0) { }

    WEAVE_ERROR RewindIdentificationKeyIterator()
    {
        position = 0;
        return WEAVE_NO_ERROR;
    }

    // Get next {tokenId, IK} pair.
    // returns tokenId = kNodeIdNotSpecified if no more IKs are available.
    WEAVE_ERROR GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen)
    {
        if (identificationKeyLen < kIdentificationKeySize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        identificationKeyLen = kIdentificationKeySize;

        switch (position)
        {
            case 0:
                memcpy(identificationKey, junk0, identificationKeyLen);
                break;
            case 1:
                 memcpy(identificationKey, junk1, identificationKeyLen);
                break;
            case 2:
                memcpy(identificationKey, junk2, identificationKeyLen);
                break;
            default:
                tokenId = kNodeIdNotSpecified;
                break;
        }
        position++;
        return WEAVE_NO_ERROR;
    }
};


// This IK correspond to an IK generated with takeTime = 17167, (number of days til 01/01/2017) which is the value used by the test Token auth delegate
uint8_t IK_timeLimited[] = { 0x0F, 0x8E, 0x23, 0x34, 0xA4, 0xA1, 0xF7, 0x60, 0x29, 0x42, 0xB3, 0x4C, 0xA5, 0x28, 0xC5, 0xA9 };

class TAKEConfigTimeLimitedIK : public MockTAKEChallengerDelegate
{
public:

    // Get next {tokenId, IK} pair.
    // returns tokenId = kNodeIdNotSpecified if no more IKs are available.
    WEAVE_ERROR GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen)
    {
        if (Rewinded)
        {
            if (identificationKeyLen < kIdentificationKeySize)
                return WEAVE_ERROR_BUFFER_TOO_SMALL;
            tokenId = 1;
            identificationKeyLen = kIdentificationKeySize;
            memcpy(identificationKey, IK_timeLimited, identificationKeyLen);
            Rewinded = false;
        }
        else
        {
            tokenId = nl::Weave::kNodeIdNotSpecified;
        }
        return WEAVE_NO_ERROR;
    }
};

uint8_t IK_ChallengerIdIsNodeId[] = { 0xAE, 0x2D, 0xD8, 0x16, 0x4B, 0xAE, 0x1A, 0x77, 0xB8, 0xCF, 0x52, 0x0D, 0x20, 0x21, 0xE2, 0x45 };
class TAKEConfigChallengerIdIsNodeId : public MockTAKEChallengerDelegate
{
public:

    // Get next {tokenId, IK} pair.
    // returns tokenId = kNodeIdNotSpecified if no more IKs are available.
    WEAVE_ERROR GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen)
    {
        if (Rewinded)
        {
            if (identificationKeyLen < kIdentificationKeySize)
                return WEAVE_ERROR_BUFFER_TOO_SMALL;
            tokenId = 1;
            identificationKeyLen = kIdentificationKeySize;
            memcpy(identificationKey, IK_ChallengerIdIsNodeId, identificationKeyLen);
            Rewinded = false;
        }
        else
        {
            tokenId = nl::Weave::kNodeIdNotSpecified;
        }
        return WEAVE_NO_ERROR;
    }
};


uint8_t MasterKey[] = { 0x11, 0xFF, 0xF1, 0x1F, 0xD1, 0x3F, 0xB1, 0x5F, 0x91, 0x7F, 0x71, 0x9F, 0x51, 0xBF, 0x31, 0xDF, 0x11, 0xFF, 0xF1, 0x1F, 0xD1, 0x3F, 0xB1, 0x5F, 0x91, 0x7F, 0x71, 0x9F, 0x51, 0xBF, 0x31, 0xDF };

void TestTAKEEngine(WeaveTAKEChallengerAuthDelegate& ChallengerAuthDelegate, bool authorized = true, uint8_t config = kTAKEConfig_Config1, bool encryptAuthPhase = false, bool encryptCommPhase = true, bool timeLimitedIK = false, bool canDoReauth = false, bool sendChallengerId = true)
{
    WEAVE_ERROR err;
    WeaveTAKEEngine initEng;
    WeaveTAKEEngine respEng;
    uint16_t sessionKeyIdInitiator = sTestDefaultSessionKeyId;
    MockTAKETokenDelegate takeTokenAuthDelegate;
    uint64_t challengerNodeId = 1337;

    PacketBuffer *encodedMsg = NULL;
    const WeaveEncryptionKey *initSessionKey;
    const WeaveEncryptionKey *respSessionKey;

    // Init TAKE Engines
    initEng.Init();
    respEng.Init();

    initEng.ChallengerAuthDelegate = &ChallengerAuthDelegate;
    respEng.TokenAuthDelegate = &takeTokenAuthDelegate;

    // Initiator generates and sends Identify Token message.
    {
        encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateIdentifyTokenMessage (Started):\n");
#endif
        err = initEng.GenerateIdentifyTokenMessage(sessionKeyIdInitiator, config, encryptAuthPhase, encryptCommPhase, timeLimitedIK, sendChallengerId, kWeaveEncryptionType_AES128CTRSHA1, challengerNodeId, encodedMsg);
        if (config != kTAKEConfig_Config1)
        {
            VerifyOrFail(err == WEAVE_ERROR_UNSUPPORTED_TAKE_CONFIGURATION, "GenerateIdentifyTokenMessage: should have returned an error");
            Clean();
            return;
        }
        SuccessOrFail(err, "WeaveTAKEEngine::GenerateIdentifyTokenMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
        fprintf(stdout, "GenerateIdentifyTokenMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateIdentifyTokenMessage (Finished):\n");
#endif
    }

    // Responder receives and processes Identify Token message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessIdentifyTokenMessage (Started):\n");
#endif

        err = respEng.ProcessIdentifyTokenMessage(challengerNodeId, encodedMsg); // peerNodeId parameter is not used
        if (err == WEAVE_ERROR_TAKE_RECONFIGURE_REQUIRED)
        {
            VerifyOrFail(config != kTAKEConfig_Config1, "WeaveTAKEEngine::ProcessIdentifyTokenMessage asks for unescessary reconfigure");
            Clean();
            return;
        }
        else
        {
            VerifyOrFail(config == kTAKEConfig_Config1, "WeaveTAKEEngine::ProcessIdentifyTokenMessage do not asks for reconfigure");
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessIdentifyTokenMessage failed\n");
        }

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessIdentifyTokenMessage (Finished):\n");
#endif
        PacketBuffer::Free(encodedMsg);
        encodedMsg = NULL;
    }

    // Responder generates and sends Identify Token Response message.
    {
        encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateIdentifyTokenResponseMessage (Started):\n");
#endif
        err = respEng.GenerateIdentifyTokenResponseMessage(encodedMsg);
        SuccessOrFail(err, "WeaveTAKEEngine::GenerateIdentifyTokenResponseMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
        fprintf(stdout, "GenerateIdentifyTokenResponseMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "GenerateIdentifyTokenResponseMessage (Finished):\n");
#endif
    }

    // Initiator receives and processes Identify token Response message.
    {
#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessIdentifyTokenResponseMessage (Started):\n");
#endif

        err = initEng.ProcessIdentifyTokenResponseMessage(encodedMsg);
        if (!authorized)
        {
            VerifyOrFail(err == WEAVE_ERROR_TAKE_TOKEN_IDENTIFICATION_FAILED, "Initiator accepted key, but shouldn't have done so\n");
            Clean();
            return;
        }
        if (canDoReauth)
        {
            VerifyOrFail(err == WEAVE_ERROR_TAKE_REAUTH_POSSIBLE, "Initiator should have initiated a Reauth\n");
        }
        else
        {
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessIdentifyTokenResponseMessage failed\n");
        }

#if DEBUG_PRINT_ENABLE
        fprintf(stdout, "ProcessIdentifyTokenResponseMessage (Finished):\n");
#endif
        PacketBuffer::Free(encodedMsg);
        encodedMsg = NULL;
    }

    VerifyOrFail(initEng.SessionKeyId == respEng.SessionKeyId, "Initiator SessionKeyId != Responder SessionKeyId\n");
    VerifyOrFail(initEng.ControlHeader == respEng.ControlHeader, "Initiator controlHeader != Responder controlHeader\n");
    VerifyOrFail(initEng.EncryptionType == respEng.EncryptionType, "Initiator encryptionType != Responder encryptionType\n");
    VerifyOrFail(initEng.ProtocolConfig == respEng.ProtocolConfig, "Initiator protocolConfig != Responder protocolConfig\n");
    VerifyOrFail(initEng.GetNumOptionalConfigurations() == respEng.GetNumOptionalConfigurations(), "Initiator numOptionalConfigurations != Responder numOptionalConfigurations\n");
    VerifyOrFail(initEng.ProtocolConfig == respEng.ProtocolConfig, "Initiator protocolConfig != Responder protocolConfig\n");
    VerifyOrFail(memcmp(initEng.OptionalConfigurations, respEng.OptionalConfigurations, respEng.GetNumOptionalConfigurations()*sizeof(uint8_t)) == 0, "Initiator optionalConfigurations != Responder optionalConfigurations\n");

    VerifyOrFail(initEng.IsEncryptAuthPhase() == respEng.IsEncryptAuthPhase(), "Initiator EAP != Responder EAP\n");
    VerifyOrFail(initEng.IsEncryptCommPhase() == respEng.IsEncryptCommPhase(), "Initiator ECP != Responder ECP\n");
    VerifyOrFail(initEng.IsTimeLimitedIK() == respEng.IsTimeLimitedIK(), "Initiator TL != Responder TL\n");

    VerifyOrFail(initEng.ChallengerIdLen == respEng.ChallengerIdLen, "Initiator ChallengerIdLen != Responder ChallengerIdLen\n");
    VerifyOrFail(memcmp(initEng.ChallengerId, respEng.ChallengerId, initEng.ChallengerIdLen) == 0, "Initiator ChallengerId != Responder ChallengerId\n");
    VerifyOrFail(memcmp(initEng.ChallengerNonce, respEng.ChallengerNonce, kNonceSize) == 0, "Initiator ChallengerNonce != Responder ChallengerNonce\n");
    VerifyOrFail(memcmp(initEng.TokenNonce, respEng.TokenNonce, kNonceSize) == 0, "Initiator TokenNonce != Responder TokenNonce\n");

    if (encryptAuthPhase)
    {
        err = initEng.GetSessionKey(initSessionKey);
        SuccessOrFail(err, "WeaveTAKEEngine::GetSessionKey() failed\n");

        err = respEng.GetSessionKey(respSessionKey);
        SuccessOrFail(err, "WeaveTAKEEngine::GetSessionKey() failed\n");

        VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.DataKey, respSessionKey->AES128CTRSHA1.DataKey, WeaveEncryptionKey_AES128CTRSHA1::DataKeySize) == 0,
                    "Data key mismatch\n");

        VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.IntegrityKey, respSessionKey->AES128CTRSHA1.IntegrityKey, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize) == 0,
                    "Integrity key mismatch\n");
    }

    if (canDoReauth)
    {
        // Initiator generates and sends ReAuthenticate Token message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateReAuthenticateTokenMessage (Started):\n");
#endif
            err = initEng.GenerateReAuthenticateTokenMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::GenerateReAuthenticateTokenMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateReAuthenticateTokenMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateReAuthenticateTokenMessage (Finished):\n");
#endif
        }

        // Responder receives and processes REAuthenticate Token message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessReAuthenticateTokenMessage (Started):\n");
#endif

            err = respEng.ProcessReAuthenticateTokenMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessReAuthenticateTokenMessage failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessReAuthenticateTokenMessage (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }

        // Responder generates and sends ReAuthenticate Token Response message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateReAuthenticateTokenResponseMessage (Started):\n");
#endif
            err = respEng.GenerateReAuthenticateTokenResponseMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::GenerateReAuthenticateTokenResponseMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateReAuthenticateTokenResponseMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateReAuthenticateTokenResponseMessage (Finished):\n");
#endif
        }

        // Initiator receives and processes ReAuthenticate Token Response message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessReAuthenticateTokenResponseMessage (Started):\n");
#endif

            err = initEng.ProcessReAuthenticateTokenResponseMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessReAuthenticateTokenResponseMessage failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessReAuthenticateTokenResponseMessage (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }

    }
    else
    {
        // Initiator generates and sends Authenticate Token message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateAuthenticateTokenMessage (Started):\n");
#endif
            err = initEng.GenerateAuthenticateTokenMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::GenerateAuthenticateTokenMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateAuthenticateTokenMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateAuthenticateTokenMessage (Finished):\n");
#endif
        }

        // Responder receives and processes Authenticate Token message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessAuthenticateTokenMessage (Started):\n");
#endif

            err = respEng.ProcessAuthenticateTokenMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessAuthenticateTokenMessage failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessAuthenticateTokenMessage (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }

        // Responder generates and sends Authenticate Token Response message.
        {
            encodedMsg = PacketBuffer::New();

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateAuthenticateTokenResponseMessage (Started):\n");
#endif
            err = respEng.GenerateAuthenticateTokenResponseMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::GenerateAuthenticateTokenResponseMessage failed\n");

#if DEBUG_PRINT_MESSAGE_LENGTH
            fprintf(stdout, "GenerateAuthenticateTokenResponseMessage: Message Size = %d \n", encodedMsg->DataLength());
#endif

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "GenerateAuthenticateTokenResponseMessage (Finished):\n");
#endif
        }

        // Initiator receives and processes Authenticate Token Response message.
        {
#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessAuthenticateTokenResponseMessage (Started):\n");
#endif

            err = initEng.ProcessAuthenticateTokenResponseMessage(encodedMsg);
            SuccessOrFail(err, "WeaveTAKEEngine::ProcessAuthenticateTokenResponseMessage failed\n");

#if DEBUG_PRINT_ENABLE
            fprintf(stdout, "ProcessAuthenticateTokenResponseMessage (Finished):\n");
#endif
            PacketBuffer::Free(encodedMsg);
            encodedMsg = NULL;
        }
    }

    if (encryptCommPhase)
    {
        err = initEng.GetSessionKey(initSessionKey);
        SuccessOrFail(err, "WeaveTAKEEngine::GetSessionKey() failed\n");

        err = respEng.GetSessionKey(respSessionKey);
        SuccessOrFail(err, "WeaveTAKEEngine::GetSessionKey() failed\n");

        VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.DataKey, respSessionKey->AES128CTRSHA1.DataKey, WeaveEncryptionKey_AES128CTRSHA1::DataKeySize) == 0,
                    "Data key mismatch\n");

        VerifyOrFail(memcmp(initSessionKey->AES128CTRSHA1.IntegrityKey, respSessionKey->AES128CTRSHA1.IntegrityKey, WeaveEncryptionKey_AES128CTRSHA1::IntegrityKeySize) == 0,
                    "Integrity key mismatch\n");
    }
}

void test1(bool EAP, bool ECP)
{
    // Standard test, Authenticate
    MockTAKEChallengerDelegate config;
    TestTAKEEngine(config, true, kTAKEConfig_Config1, EAP, ECP);
}

void test2(bool EAP, bool ECP)
{
    // No authorised Tokens
    TAKEConfigNoAuthorized config;
    TestTAKEEngine(config, false, kTAKEConfig_Config1, EAP, ECP);
}
void test3(bool EAP, bool ECP)
{
    // 3 authorised tokens, none of them are the correct one
    TAKEConfigJunkAuthorized config;
    TestTAKEEngine(config, false, kTAKEConfig_Config1, EAP, ECP);
}
void test4(bool EAP, bool ECP)
{
    // Tries to use an invalid config
    MockTAKEChallengerDelegate config;
    TestTAKEEngine(config, true, 27, EAP, ECP);
}
void test5(bool EAP, bool ECP)
{
    // TL is true, invalid argument
    TAKEConfigTimeLimitedIK config;
    TestTAKEEngine(config, true, kTAKEConfig_Config1, EAP, ECP, true); // TL is true => error
}

void test6(bool EAP, bool ECP)
{
    uint8_t AK[] = { 0x9F, 0x0F, 0x92, 0xE3, 0xB9, 0x04, 0x96, 0xA1, 0xCB, 0x7C, 0x94, 0x99, 0xAB, 0x34, 0xDD, 0x04 };
    uint8_t ENC_AK[] = { 0xE6, 0xC4, 0x03, 0xE8, 0xEE, 0xA3, 0x80, 0x56, 0xE0, 0xB1, 0x9C, 0xE9, 0xE3, 0xA6, 0xD8, 0x3A };

    MockTAKEChallengerDelegate config;
    uint16_t authKeyLen = TAKE::kAuthenticationKeySize;
    uint16_t encryptedAuthKeyLen = kTokenEncryptedStateSize;
    WEAVE_ERROR err = config.StoreTokenAuthData(1, kTAKEConfig_Config1, AK, authKeyLen, ENC_AK, encryptedAuthKeyLen);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "%s FAILED: ", __FUNCTION__);
        fputs("TAKEConfig::UpdateAuthenticationKeys", stderr);
        fputs(ErrorStr(err), stderr);
        fputs("\n", stderr);
        exit(-1);
    }
    TestTAKEEngine(config, true, kTAKEConfig_Config1, EAP, ECP, false, true);
}

void test7(bool EAP, bool ECP)
{
    // Standard test, Authenticate
    TAKEConfigChallengerIdIsNodeId config;
    TestTAKEEngine(config, true, kTAKEConfig_Config1, EAP, ECP, false, false, false);
}

// Parameters are EAP and ECP
typedef void(*testFunction)(bool, bool);

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    int i;
    time_t sec_now;
    time_t sec_last;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

#define NUMBER_OF_ITERATIONS 1
#define NUMBER_OF_TESTS 7
#define NUMBER_OF_CONFIGS 4

    testFunction tests[NUMBER_OF_TESTS] = { test1, test2, test3, test4, test5, test6, test7 };
    bool configs[NUMBER_OF_CONFIGS][2] = { { false, false }, { true, false }, { false, true }, { true, true } };

    for (int test = 0; test < NUMBER_OF_TESTS; test++)
    {
        for (int conf = 0; conf < NUMBER_OF_CONFIGS; conf++)
        {
            bool EAP = configs[conf][0];
            bool ECP = configs[conf][1];
            printf("\nTEST%d, EAP = %s, ECP = %s (%d iterations)\n", test+1, EAP ? "true" : "false", ECP ? "true" : "false", NUMBER_OF_ITERATIONS);
            sec_last = time(NULL);
            for (i=0; i < NUMBER_OF_ITERATIONS; i++)
            {
                tests[test](EAP, ECP);
            }
            sec_now = time(NULL);
            printf("TIME DELTA (sec) = %ld sec\n", (sec_now - sec_last));
        }
    }

    printf("All tests succeeded\n");
}

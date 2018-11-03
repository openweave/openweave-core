/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements a unit test for the Weave message encoding
 *      and decoding functions of the WeaveMessageLayer class.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <string.h>

#include <nlunit-test.h>

#include "ToolCommon.h"

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

namespace nl {
namespace Weave {

class TestMessageEncodingHelper
{
public:
    WeaveFabricState FabricState;
    WeaveMessageLayer MessageLayer;

    WEAVE_ERROR Init()
    {
        WEAVE_ERROR err;

        // Initialize the FabricState object.
        err = FabricState.Init();
        SuccessOrExit(err);
        FabricState.LocalNodeId = 1;
        FabricState.FabricId = 1;
        FabricState.DefaultSubnet = kWeaveSubnetId_PrimaryWiFi;

        // Partially initialize the MessageLayer object, enough to support message encoding/decoding.
        MessageLayer.FabricState = &FabricState;

    exit:
        return err;
    }

    WEAVE_ERROR EncodeMessage(WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf, WeaveConnection *con, uint16_t maxLen,
            uint16_t reserve)
    {
        return MessageLayer.EncodeMessage(msgInfo, msgBuf, con, maxLen, reserve);
    }

    WEAVE_ERROR DecodeMessage(PacketBuffer *msgBuf, uint64_t sourceNodeId, WeaveConnection *con,
            WeaveMessageInfo *msgInfo, uint8_t **rPayload, uint16_t *rPayloadLen)
    {
        return MessageLayer.DecodeMessage(msgBuf, sourceNodeId, con, msgInfo, rPayload, rPayloadLen);
    }

    uint32_t GetNextUnencryptedUDPMessageId()
    {
        return FabricState.NextUnencUDPMsgId.GetValue();
    }

    void SetNextUnencryptedUDPMessageId(uint32_t msgId)
    {
        FabricState.NextUnencUDPMsgId.Init(msgId);
    }
};

} // namespace nl
} // namespace Weave

static ::nl::Weave::TestMessageEncodingHelper sTestHelper;

#include "MsgEncTestVectors.h"

static void MessageEncodeDecodeTest(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;
    WeaveSessionKey * encodeSessionKey = NULL;
    WeaveSessionKey * decodeSessionKey = NULL;
    PacketBuffer * msgBuf = NULL;
    uint8_t *payload;
    uint16_t payloadLen;

    static WeaveMessageInfo msgInfo;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    NL_TEST_ASSERT(inSuite, msgBuf != NULL);
    VerifyOrExit(msgBuf != NULL, /**/);

    for (size_t i = 0; i < sNumMessageEncodingTestVectors; i++)
    {
        const MessageEncodingTestVector & testVec = *sMessageEncodingTestVectors[i];

#if !WEAVE_CONFIG_AES128EAX128
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128EAX128)
            continue;
#endif
#if !WEAVE_CONFIG_AES128EAX64
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128EAX64)
            continue;
#endif

        // Configure the fabric state to operate as the source node.
        sTestHelper.FabricState.LocalNodeId = testVec.MsgInfo.SourceNodeId;

        // If the message will not be encrypted, set the global next unencrypted UDP message id counter to
        // the message id for the test message.
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_None)
        {
            sTestHelper.SetNextUnencryptedUDPMessageId(testVec.MsgInfo.MessageId);
        }

        // Otherwise if the message will be encrypted...
        else
        {
            // Create an entry in the session key table associated with the source node id. This will be used
            // for message decoding, and for encoding messages to the 'any' node id.
            err = sTestHelper.FabricState.AllocSessionKey(testVec.MsgInfo.SourceNodeId, testVec.MsgInfo.KeyId, NULL, decodeSessionKey);
            NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
            if (err != WEAVE_NO_ERROR)
                goto next;
            sTestHelper.FabricState.SetSessionKey(decodeSessionKey, testVec.MsgInfo.EncryptionType, kWeaveAuthMode_NotSpecified, testVec.EncKey);
            decodeSessionKey->NextMsgId.Init(testVec.MsgInfo.MessageId);

            // If the destination node id is not 'any', initialize an entry in the session key table associated
            // with destination node id.  This will be used for message encoding when *not* sending to the 'any'
            // node.
            if (testVec.MsgInfo.DestNodeId != kAnyNodeId)
            {
                err = sTestHelper.FabricState.AllocSessionKey(testVec.MsgInfo.DestNodeId, testVec.MsgInfo.KeyId, NULL, encodeSessionKey);
                NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
                if (err != WEAVE_NO_ERROR)
                    goto next;
                sTestHelper.FabricState.SetSessionKey(encodeSessionKey, testVec.MsgInfo.EncryptionType, kWeaveAuthMode_NotSpecified, testVec.EncKey);
                encodeSessionKey->NextMsgId.Init(testVec.MsgInfo.MessageId);
            }
            else
            {
                encodeSessionKey = decodeSessionKey;
            }
        }

        // Copy payload data into the buffer.
        memcpy(msgBuf->Start(), testVec.MsgPayload, testVec.MsgPayloadLen);
        msgBuf->SetDataLength(testVec.MsgPayloadLen);

        // Invoke the Weave Message Layer encode function.
        msgInfo = testVec.MsgInfo;
        err = sTestHelper.EncodeMessage(&msgInfo, msgBuf, NULL, UINT16_MAX, 0);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        if (err != WEAVE_NO_ERROR)
            goto next;

        // Verify the encoded message against the expected value.
        {
            bool matchesExpectedEncoding = (msgBuf->DataLength() == testVec.ExpectedEncodedMsgLen &&
                                            memcmp(msgBuf->Start(), testVec.ExpectedEncodedMsg, testVec.ExpectedEncodedMsgLen) == 0);
            NL_TEST_ASSERT(inSuite, matchesExpectedEncoding);
            if (!matchesExpectedEncoding)
            {
                printf("Test %" PRIu32 ":\n", (uint32_t)i);
                printf("  Expected:\n");
                DumpMemoryCStyle(testVec.ExpectedEncodedMsg, testVec.ExpectedEncodedMsgLen, "    ", 16);
                printf("  Actual:\n");
                DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "    ", 16);
            }
        }

        // Verify that the appropriate next message id counter was incremented during message encoding.
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_None)
        {
            NL_TEST_ASSERT(inSuite, sTestHelper.GetNextUnencryptedUDPMessageId() == testVec.MsgInfo.MessageId + 1);
        }
        else
        {
            NL_TEST_ASSERT(inSuite, encodeSessionKey->NextMsgId.GetValue() == testVec.MsgInfo.MessageId + 1);
        }

        // Switch the fabric state to operate as the destination node.
        sTestHelper.FabricState.LocalNodeId = testVec.MsgInfo.DestNodeId;

        // Decode the encoded message.
        memset(&msgInfo, 0, sizeof(msgInfo));
        err = sTestHelper.DecodeMessage(msgBuf, testVec.MsgInfo.SourceNodeId, NULL, &msgInfo, &payload, &payloadLen);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

        // Verify the decode message payload against the expected value.
        NL_TEST_ASSERT(inSuite, payloadLen == testVec.MsgPayloadLen);
        NL_TEST_ASSERT(inSuite, memcmp(payload, testVec.MsgPayload, payloadLen) == 0);

        // Verify the returned message metadata.
        NL_TEST_ASSERT(inSuite, msgInfo.SourceNodeId == testVec.MsgInfo.SourceNodeId);
        NL_TEST_ASSERT(inSuite, msgInfo.DestNodeId == testVec.MsgInfo.DestNodeId);
        NL_TEST_ASSERT(inSuite, msgInfo.MessageId == testVec.MsgInfo.MessageId);
        NL_TEST_ASSERT(inSuite, msgInfo.MessageVersion == testVec.MsgInfo.MessageVersion);
        NL_TEST_ASSERT(inSuite, msgInfo.EncryptionType == testVec.MsgInfo.EncryptionType);
        if (testVec.MsgInfo.EncryptionType != kWeaveEncryptionType_None)
        {
            NL_TEST_ASSERT(inSuite, msgInfo.KeyId == testVec.MsgInfo.KeyId);
        }

    next:

        // Remove the session keys as necessary.
        if (encodeSessionKey != NULL)
        {
            sTestHelper.FabricState.RemoveSessionKey(encodeSessionKey);
        }
        if (decodeSessionKey != NULL && decodeSessionKey != encodeSessionKey)
        {
            sTestHelper.FabricState.RemoveSessionKey(decodeSessionKey);
        }
        encodeSessionKey = NULL;
        decodeSessionKey = NULL;
    }

exit:
    if (encodeSessionKey != NULL)
    {
        sTestHelper.FabricState.RemoveSessionKey(encodeSessionKey);
    }
    if (decodeSessionKey != NULL && decodeSessionKey != encodeSessionKey)
    {
        sTestHelper.FabricState.RemoveSessionKey(decodeSessionKey);
    }

    // Release buffer.
    PacketBuffer::Free(msgBuf);
}

static void MessageDecodeFuzzTest(nlTestSuite * inSuite, void * inContext)
{
    WEAVE_ERROR err;
    PacketBuffer * msgBuf = NULL;
    WeaveSessionKey * decodeSessionKey = NULL;
    uint8_t *payload;
    uint16_t payloadLen;
    uint16_t fuzzIndex;
    uint8_t fuzzBit;

    static WeaveMessageInfo msgInfo;

    // Allocate buffer.
    msgBuf = PacketBuffer::New();
    NL_TEST_ASSERT(inSuite, msgBuf != NULL);
    VerifyOrExit(msgBuf != NULL, /**/);

    for (size_t i = 0; i < sNumMessageEncodingTestVectors; i++)
    {
        const MessageEncodingTestVector & testVec = *sMessageEncodingTestVectors[i];

#if !WEAVE_CONFIG_AES128EAX128
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128EAX128)
            continue;
#endif
#if !WEAVE_CONFIG_AES128EAX64
        if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128EAX64)
            continue;
#endif

        // Configure the fabric state to operate as the destination node.
        sTestHelper.FabricState.LocalNodeId = testVec.MsgInfo.DestNodeId;

        // If the message is encrypted...
        if (testVec.MsgInfo.EncryptionType != kWeaveEncryptionType_None)
        {
            // Create an entry in the session key table associated with the source node id.
            err = sTestHelper.FabricState.AllocSessionKey(testVec.MsgInfo.SourceNodeId, testVec.MsgInfo.KeyId, NULL, decodeSessionKey);
            NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
            SuccessOrExit(err);
            sTestHelper.FabricState.SetSessionKey(decodeSessionKey, testVec.MsgInfo.EncryptionType, kWeaveAuthMode_NotSpecified, testVec.EncKey);
            decodeSessionKey->NextMsgId.Init(testVec.MsgInfo.MessageId);
        }

        fuzzIndex = 0;
        fuzzBit = 1;
        while (fuzzIndex < testVec.ExpectedEncodedMsgLen)
        {
            bool failureExpected = true;

            if (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_None)
            {
                if ((fuzzIndex == 1 && (fuzzBit & 0x0F) != 0) || (fuzzIndex > 1))
                {
                    failureExpected = false;
                }
            }
            else
            {
                if ((testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128CTRSHA1 && fuzzIndex == 0 && fuzzBit == 0x10) ||
                    (testVec.MsgInfo.EncryptionType == kWeaveEncryptionType_AES128EAX64 && fuzzIndex == 0 && fuzzBit == 0x20) ||
                     (fuzzIndex == 1 && fuzzBit == 0x08))
                {
                    failureExpected = false;
                }
            }

            // Copy the encoded message into the buffer.
            memcpy(msgBuf->Start(), testVec.ExpectedEncodedMsg, testVec.ExpectedEncodedMsgLen);
            msgBuf->SetDataLength(testVec.ExpectedEncodedMsgLen);

            // Fuzz the message.
            msgBuf->Start()[fuzzIndex] ^= fuzzBit;

            // Attempt to decode the fuzzed message.
            memset(&msgInfo, 0, sizeof(msgInfo));
            err = sTestHelper.DecodeMessage(msgBuf, testVec.MsgInfo.SourceNodeId, NULL, &msgInfo, &payload, &payloadLen);

            // Verify that message decoding fails if expected.
            if (failureExpected)
            {
                NL_TEST_ASSERT(inSuite, err != WEAVE_NO_ERROR);
                if (err == WEAVE_NO_ERROR)
                {
                    printf("Test %" PRIu32 ": index=%" PRIu16 ", bit=0x%" PRIX8 ", error=%" PRIu32,
                            (uint32_t)i, fuzzIndex, fuzzBit, (uint32_t)err);
                }
            }

            fuzzBit <<= 1;
            if (fuzzBit == 0)
            {
                fuzzIndex++;
                fuzzBit = 1;
            }
        }

        // Remove the session key.
        if (decodeSessionKey != NULL)
        {
            sTestHelper.FabricState.RemoveSessionKey(decodeSessionKey);
            decodeSessionKey = NULL;
        }
    }


exit:

    if (decodeSessionKey != NULL)
    {
        sTestHelper.FabricState.RemoveSessionKey(decodeSessionKey);
    }

    // Release buffer.
    PacketBuffer::Free(msgBuf);
}

int main(int argc, char *argv[])
{
    static const nlTest tests[] = {
        NL_TEST_DEF("MessageEncodeDecodeTest", MessageEncodeDecodeTest),
        NL_TEST_DEF("MessageDecodeFuzzTest", MessageDecodeFuzzTest),
        NL_TEST_SENTINEL()
    };

    static nlTestSuite testSuite = {
        "message-encoding",
        &tests[0]
    };

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Dummy references to PersistedStorage Read()/Write() methods to un-confuse linker.
    {
        uint32_t v;
        ::nl::Weave::Platform::PersistedStorage::Read("", v);
        ::nl::Weave::Platform::PersistedStorage::Write("", v);
    }

    sTestHelper.Init();

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, NULL);

    return nlTestRunnerStats(&testSuite);
}

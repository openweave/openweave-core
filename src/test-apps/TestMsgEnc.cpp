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
#include <nltest.h>
#include <string.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveConfig.h>
#include <Weave/Support/crypto/CTRMode.h>
#include <Weave/Support/crypto/WeaveCrypto.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace nl::Inet;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Profiles::Security;

#define DEBUG_PRINT_ENABLE 0

namespace nl {
namespace Weave {

class NL_DLL_EXPORT WeaveMessageLayerTestObject
{
public:
    WeaveMessageLayer *msgLayer;

    WEAVE_ERROR DecodeMessage(PacketBuffer *msgBuf, uint64_t sourceNodeId, WeaveConnection *con,
            WeaveMessageInfo *msgInfo, uint8_t **rPayload, uint16_t *rPayloadLen)
    {
        return msgLayer->DecodeMessage(msgBuf, sourceNodeId, con, msgInfo, rPayload, rPayloadLen);
    }
};

} // namespace nl
} // namespace Weave


static const uint8_t sMsgPayload[] =
{
    0x05, 0x26, 0xAD, 0xB7, 0xBB, 0xD7, 0x82, 0x52, 0x78, 0x2D, 0x60, 0xD6, 0x40, 0xFD, 0xE6, 0xF9,
    0x0A, 0x1A, 0x2A, 0x3A, 0x4A, 0x5A, 0x6A, 0x7A, 0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA,
    0x09, 0x19, 0x29, 0x39, 0x49, 0x59, 0x69, 0x79, 0x89, 0x99, 0xA9, 0xB9, 0xC9, 0xD9, 0xE9, 0xF9,
    0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x6A
};

static const uint8_t sMsgEncKey_DataKey[] =
{
    0xF7, 0xE7, 0xD7, 0xC7, 0xB7, 0xA7, 0x97, 0x87, 0x07, 0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77
};

static const uint8_t sMsgEncKey_IntegrityKey[] =
{
    0xFD, 0xED, 0xDD, 0xCD, 0xBD, 0xAD, 0x9D, 0x8D, 0x0D, 0x1D, 0x2D, 0x3D, 0x4D, 0x5D, 0x6D, 0x7D,
    0x82, 0x52, 0x78, 0x2D
};

static const uint8_t sEncodedMsg_V1[] =
{
    0x10, 0x1B, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x30, 0xB4, 0x18, 0x78, 0x56,
    0x34, 0x12, 0x00, 0x30, 0xB4, 0x18, 0x2A, 0x20, 0xAC, 0xBD, 0x69, 0x5C, 0x20, 0x44, 0x51, 0x71,
    0x16, 0x1C, 0x29, 0x48, 0xC6, 0x3D, 0xBD, 0x91, 0x33, 0x77, 0x7B, 0x58, 0xFA, 0x9C, 0x08, 0x6B,
    0x2B, 0xAF, 0x1D, 0x25, 0x04, 0x01, 0x4E, 0xE2, 0x1E, 0xB7, 0x2F, 0x41, 0x19, 0x2B, 0x81, 0x60,
    0x6B, 0xE8, 0xB9, 0x00, 0x08, 0xA1, 0x1C, 0x9C, 0x62, 0x3B, 0x23, 0x1D, 0x99, 0xBC, 0xA4, 0x7B,
    0x54, 0x00, 0xA2, 0x0B, 0x20, 0x3B, 0xA7, 0x80, 0x01, 0xAC, 0xF4, 0xF4, 0xF5, 0x50, 0x24, 0x63,
    0xF1, 0xBA, 0x50
};

static const uint8_t sEncodedMsg_V2[] =
{
    0x10, 0x2B, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x30, 0xB4, 0x18, 0x78, 0x56,
    0x34, 0x12, 0x00, 0x30, 0xB4, 0x18, 0x2A, 0x20, 0xAC, 0xBD, 0x69, 0x5C, 0x20, 0x44, 0x51, 0x71,
    0x16, 0x1C, 0x29, 0x48, 0xC6, 0x3D, 0xBD, 0x91, 0x33, 0x77, 0x7B, 0x58, 0xFA, 0x9C, 0x08, 0x6B,
    0x2B, 0xAF, 0x1D, 0x25, 0x04, 0x01, 0x4E, 0xE2, 0x1E, 0xB7, 0x2F, 0x41, 0x19, 0x2B, 0x81, 0x60,
    0x6B, 0xE8, 0xB9, 0x00, 0x08, 0xA1, 0x1C, 0x9C, 0x62, 0x3B, 0x23, 0x1D, 0x99, 0xBC, 0xA4, 0x96,
    0xD0, 0xAE, 0x61, 0xFD, 0x55, 0x05, 0x43, 0xEA, 0x6C, 0x49, 0x1D, 0xA5, 0x8D, 0x57, 0x3C, 0xF6,
    0xD9, 0x27, 0x9D
};

// Test input vector format.
struct TestContext {
    uint8_t        MsgVersion;
    const uint8_t *MsgPayload;
    uint16_t       MsgPayloadLen;
    const uint8_t *EncodedMsg;
    uint16_t       EncodedMsgLen;
    WEAVE_ERROR    EncodeError;
    WEAVE_ERROR    DecodeError;
};

// Test input data.
static struct TestContext sContext[] = {
    { kWeaveMessageVersion_V1, sMsgPayload, sizeof(sMsgPayload), sEncodedMsg_V1, sizeof(sEncodedMsg_V1), WEAVE_NO_ERROR, WEAVE_NO_ERROR },
    { kWeaveMessageVersion_V2, sMsgPayload, sizeof(sMsgPayload), sEncodedMsg_V2, sizeof(sEncodedMsg_V2), WEAVE_NO_ERROR, WEAVE_NO_ERROR },
    { kWeaveMessageVersion_V2, sMsgPayload, sizeof(sMsgPayload), sEncodedMsg_V2, sizeof(sEncodedMsg_V2), WEAVE_NO_ERROR, WEAVE_ERROR_WRONG_ENCRYPTION_TYPE },
    { kWeaveMessageVersion_V2, sMsgPayload, sizeof(sMsgPayload), sEncodedMsg_V2, sizeof(sEncodedMsg_V2), WEAVE_ERROR_WRONG_ENCRYPTION_TYPE, WEAVE_NO_ERROR },
};

// Number of test context examples.
static const size_t kTestElements = sizeof(sContext) / sizeof(struct TestContext);

void WeaveMessageEncryption_Test1(nlTestSuite *inSuite, void *inContext)
{
    static WeaveFabricState fabricState;
    static WeaveMessageLayer messageLayer;
    static WeaveMessageInfo msgInfo;

    WEAVE_ERROR err;
    PacketBuffer *msgBuf;
    uint64_t srcNodeId;
    uint64_t destNodeId = 0x18B4300012345678;
    uint32_t msgId = 3;
    uint8_t encType = kWeaveEncryptionType_AES128CTRSHA1;
    uint16_t sessionKeyId = sTestDefaultSessionKeyId;
    uint8_t *p;

    const char localAddrStr[] = "fd00:0:1:1:18B4:3000::2";
    IPAddress localIPv6Addr;
    NL_TEST_ASSERT(inSuite, ParseIPAddress(localAddrStr, localIPv6Addr));

    // Initialize the FabricState object.
    err = fabricState.Init();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    srcNodeId = localIPv6Addr.InterfaceId();
    fabricState.LocalNodeId = srcNodeId;
    fabricState.FabricId = localIPv6Addr.GlobalId();
    fabricState.DefaultSubnet = localIPv6Addr.Subnet();

    // Initialize message encryption session key.
    WeaveEncryptionKey msgEncSessionKey;
    WeaveAuthMode authMode = kWeaveAuthMode_CASE_Device;

    memcpy(msgEncSessionKey.AES128CTRSHA1.DataKey, sMsgEncKey_DataKey, sizeof(sMsgEncKey_DataKey));
    memcpy(msgEncSessionKey.AES128CTRSHA1.IntegrityKey, sMsgEncKey_IntegrityKey, sizeof(sMsgEncKey_IntegrityKey));

    // Initialize session key with destination node id.
    err = fabricState.AllocSessionKey(destNodeId, NULL, sessionKeyId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = fabricState.SetSessionKey(sessionKeyId, destNodeId, encType, authMode, &msgEncSessionKey);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Initialize the same session key with LocalNodeId as a destination node id.
    err = fabricState.AllocSessionKey(srcNodeId, NULL, sessionKeyId);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = fabricState.SetSessionKey(sessionKeyId, srcNodeId, encType, authMode, &msgEncSessionKey);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    // Initialize the MessageLayer object.
    messageLayer.FabricState = &fabricState;

    struct TestContext *theContext = (struct TestContext *)(inContext);

    for (size_t ith = 0; ith < kTestElements; ith++, theContext++)
    {
        uint8_t msgVersion = theContext->MsgVersion;
        const uint8_t *msgPayload = theContext->MsgPayload;
        uint16_t msgPayloadLen = theContext->MsgPayloadLen;
        const uint8_t *encodedMsg = theContext->EncodedMsg;
        uint16_t encodedMsgLen = theContext->EncodedMsgLen;
        WEAVE_ERROR encodeError = theContext->EncodeError;
        WEAVE_ERROR decodeError = theContext->DecodeError;
        uint8_t localMsgBuf[theContext->EncodedMsgLen];

        // Allocate buffer.
        msgBuf = PacketBuffer::New();
        NL_TEST_ASSERT(inSuite, msgBuf != NULL);
        if (msgBuf == NULL)
            continue;

        // Copy payload data.
        memcpy(msgBuf->Start(), msgPayload, msgPayloadLen);
        msgBuf->SetDataLength(msgPayloadLen);

        // Initialize the Message info object.
        msgInfo.Clear();
        msgInfo.SourceNodeId = srcNodeId;
        msgInfo.DestNodeId = destNodeId;
        msgInfo.MessageId = msgId;
        if (encodeError == WEAVE_ERROR_WRONG_ENCRYPTION_TYPE)
            msgInfo.KeyId = WeaveKeyId::kNone;
        else
            msgInfo.KeyId = sessionKeyId;
        msgInfo.Flags = kWeaveMessageFlag_DestNodeId |
                          kWeaveMessageFlag_SourceNodeId |
                          kWeaveMessageFlag_MsgCounterSyncReq |
                          kWeaveMessageFlag_ReuseMessageId;
        msgInfo.MessageVersion = msgVersion;
        msgInfo.EncryptionType = encType;

        // =====================================================================================================
        // Encode message using EncodeMessage() function.
        // =====================================================================================================
        err = messageLayer.EncodeMessage(&msgInfo, msgBuf, NULL, UINT16_MAX, 0);
        NL_TEST_ASSERT(inSuite, err == encodeError);

        if (err != WEAVE_NO_ERROR)
        {
            PacketBuffer::Free(msgBuf);
            msgBuf = NULL;

            continue;
        }

#if DEBUG_PRINT_ENABLE
        printf("Encoded message generated by EncodeMessage():\n");
        DumpMemoryCStyle(msgBuf->Start(), msgBuf->DataLength(), "    ", 16);
#endif

        // =====================================================================================================
        // Manually encode message and compare against the result generated by EncodeMessage() function.
        // =====================================================================================================
        p = localMsgBuf;

        // Write the header field.
        uint16_t headerVal = ((uint16_t) (msgInfo.Flags & 0xF0F) << 0) |
                             ((uint16_t) (msgInfo.EncryptionType & 0xF) << 4) |
                             ((uint16_t) (msgInfo.MessageVersion & 0xF) << 12);
        LittleEndian::Write16(p, headerVal);
        LittleEndian::Write32(p, msgId);
        LittleEndian::Write64(p, srcNodeId);
        LittleEndian::Write64(p, destNodeId);
        LittleEndian::Write16(p, sessionKeyId);

        // Message data to encrypt.
        uint8_t aesDataIn[msgPayloadLen + HMACSHA1::kDigestLength];
        memcpy(aesDataIn, msgPayload, msgPayloadLen);

        // Message data to authenticate.
        uint16_t sha1DataLen;
        uint8_t sha1DataIn[2 * sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint32_t) + msgPayloadLen];
        uint8_t *p2 = sha1DataIn;
        Encoding::LittleEndian::Write64(p2, srcNodeId);
        Encoding::LittleEndian::Write64(p2, destNodeId);
        sha1DataLen = 2 * sizeof(uint64_t);
        if (msgVersion == kWeaveMessageVersion_V2)
        {
            // Mask destination and source node Id flags.
            headerVal &= kMsgHeaderField_MessageHMACMask;

            // Encode the message header field and the message Id in a little-endian format.
            Encoding::LittleEndian::Write16(p2, headerVal);
            Encoding::LittleEndian::Write32(p2, msgId);

            sha1DataLen += sizeof(uint16_t) + sizeof(uint32_t);
        }
        memcpy(p2, msgPayload, msgPayloadLen);
        sha1DataLen += msgPayloadLen;

        // Compute integrity check.
        HMACSHA1 sha1;
        sha1.Begin(sMsgEncKey_IntegrityKey, sizeof(sMsgEncKey_IntegrityKey));
        sha1.AddData(sha1DataIn, sha1DataLen);
        sha1.Finish(aesDataIn + msgPayloadLen);

        // Encrypt the message payload and the integrity value.
        AES128CTRMode aes128CTR;
        aes128CTR.SetKey(sMsgEncKey_DataKey);
        aes128CTR.SetWeaveMessageCounter(srcNodeId, msgId);
        aes128CTR.EncryptData(aesDataIn, sizeof(aesDataIn), p);

#if DEBUG_PRINT_ENABLE
        printf("Manually generated encoded message:\n");
        DumpMemoryCStyle(localMsgBuf, sizeof(localMsgBuf), "    ", 16);
#endif

        // Compare the result.
        NL_TEST_ASSERT(inSuite, memcmp(msgBuf->Start(), localMsgBuf, sizeof(localMsgBuf)) == 0);

        // Compare the result against the static value saved in this test.
        NL_TEST_ASSERT(inSuite, msgBuf->DataLength() == encodedMsgLen);
        NL_TEST_ASSERT(inSuite, memcmp(msgBuf->Start(), encodedMsg, encodedMsgLen) == 0);

        // =====================================================================================================
        // Verify that DecodeMessage() generates original payload context.
        // =====================================================================================================
        WeaveMessageLayerTestObject msgLayerTestObject;
        uint8_t *payload;
        uint16_t payloadLen;

        // Inject wrong KeyId.
        if (decodeError == WEAVE_ERROR_WRONG_ENCRYPTION_TYPE)
        {
            uint8_t *injectKeyId = msgBuf->Start() + sizeof(headerVal) + sizeof(msgId) + sizeof(srcNodeId) + sizeof(destNodeId);

            LittleEndian::Write16(injectKeyId, WeaveKeyId::kNone);
        }

        msgLayerTestObject.msgLayer = &messageLayer;
        err = msgLayerTestObject.DecodeMessage(msgBuf, srcNodeId, NULL, &msgInfo, &payload, &payloadLen);
        NL_TEST_ASSERT(inSuite, err == decodeError);

        if (err == WEAVE_NO_ERROR)
        {
#if DEBUG_PRINT_ENABLE
            printf("Decoded Payload generated by DecodeMessage():\n");
            DumpMemoryCStyle(payload, payloadLen, "    ", 16);
#endif

            // Compare the result of DecodeMessage() to the original Payload message data.
            NL_TEST_ASSERT(inSuite, payloadLen == msgPayloadLen);
            NL_TEST_ASSERT(inSuite, memcmp(payload, msgPayload, msgPayloadLen) == 0);
        }

        // Release buffer.
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }
}


int main(int argc, char *argv[])
{
    static const nlTest tests[] = {
        NL_TEST_DEF("WeaveMessageEncryption",           WeaveMessageEncryption_Test1),
        NL_TEST_SENTINEL()
    };

    static nlTestSuite testSuite = {
        "provisioning-hash",
        &tests[0]
    };

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&testSuite, &sContext);

    return nlTestRunnerStats(&testSuite);
}

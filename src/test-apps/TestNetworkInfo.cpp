/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Unit tests for Weave network info serialization.
 *
 */

#include "ToolCommon.h"
#include <nlunit-test.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/network-provisioning/NetworkInfo.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::NetworkProvisioning;

#define TLV_DATA_SIZE 1000
static uint8_t tlvData[TLV_DATA_SIZE];

static uint8_t xpanid1[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
static uint8_t xpanid2[] = { 101, 102, 103, 104, 105, 106, 107, 108 };
static uint8_t key1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static uint8_t key2[] = { 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116 };

void WeaveTestNetworkInfo(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVReader reader;
    uint16_t elemCount = 2;
    int tlvLength;
    char *cbuf;
    uint8_t *buf;

    NetworkInfo elemArray[2];
    NetworkInfo decodeArray[2];
    NetworkInfo *dArray = decodeArray;

    // PAN-1 has old-style credentials (no PAN ID, channel)
    elemArray[0].NetworkType = kNetworkType_Thread;
    elemArray[0].NetworkId = 1;

    cbuf = new char[64];
    strcpy(cbuf, "PAN-1");
    elemArray[0].ThreadNetworkName = cbuf;

    buf = new uint8_t[8];
    memcpy(buf, xpanid1, 8);
    elemArray[0].ThreadExtendedPANId = buf;

    buf = new uint8_t[16];
    memcpy(buf, key1, 16);
    elemArray[0].ThreadNetworkKey = buf;

    // PAN-1 has new-style credentials (with PAN ID, channel)
    elemArray[1].NetworkType = kNetworkType_Thread;
    elemArray[1].NetworkId = 2;


    cbuf = new char[64];
    strcpy(cbuf, "PAN-2");
    elemArray[1].ThreadNetworkName = cbuf;

    buf = new uint8_t[8];
    memcpy(buf, xpanid2, 8);
    elemArray[1].ThreadExtendedPANId = buf;

    buf = new uint8_t[16];
    memcpy(buf, key2, 16);
    elemArray[1].ThreadNetworkKey = buf;
    elemArray[1].ThreadPANId = 0x1234;
    elemArray[1].ThreadChannel = 15;

    writer.Init(tlvData, TLV_DATA_SIZE);

    err = NetworkInfo::EncodeList(writer, 2, elemArray, kGetNetwork_IncludeCredentials);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    err = writer.Finalize();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    tlvLength = writer.GetLengthWritten();

    printf("%d TLV bytes written.\n", tlvLength);

    for (int i=0; i<tlvLength ; i+=8) {
        printf("tlv[%03d] = %02x %02x %02x %02x %02x %02x %02x %02x\n",
            i, tlvData[i], tlvData[i+1], tlvData[i+2], tlvData[i+3],
            tlvData[i+4], tlvData[i+5], tlvData[i+6], tlvData[i+7]);
    }

    reader.Init(tlvData, tlvLength);
    err = reader.Next();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    err = NetworkInfo::DecodeList(reader, elemCount, dArray);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, elemCount == 2);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Test optional Network Info TLVs",     WeaveTestNetworkInfo),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    nlTestSuite theSuite = {
        "network-info",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

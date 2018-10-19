/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *     This file implements a unit test suite for <tt>nl::Ble::Woble</tt>,
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>

#include <BleLayer/WoBle.h>
#include <BleLayer/BleLayer.h>
#include <nlunit-test.h>

#include "ToolCommon.h"

using namespace nl::Ble;

static nl::Ble::WoBle woble;

static void HandleCharacteristicReceivedOnePacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    SequenceNumber_t rcvd_ack;
    bool did_rcv_ack;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * data;

    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "Start HandleCharacteristicReceivedOnePacket Woble State:");
    woble.LogState();

    first_packet = PacketBuffer::NewWithAvailableSize(5);

    NL_TEST_ASSERT(inSuite, first_packet->AvailableDataLength() >= 5);

    first_packet->SetDataLength(5, NULL);
    data = first_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = 0x05;
    data[1] = 1;
    data[2] = 1;
    data[3] = 0;
    data[4] = 0xff; // payload

    err = woble.HandleCharacteristicReceived(first_packet, rcvd_ack, did_rcv_ack);
    first_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_Complete);

    data = NULL;
    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "HandleCharacteristicReceivedOnePacket with Woble State:");
    woble.LogState();
}

static void HandleCharacteristicReceivedTwoPacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    PacketBuffer * second_packet;
    SequenceNumber_t rcvd_ack;
    bool did_rcv_ack;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * data;

    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "Start HandleCharacteristicReceivedTwoPacket Woble State:");
    woble.LogState();

    first_packet = PacketBuffer::NewWithAvailableSize(10);
    first_packet->SetDataLength(5, NULL);

    data = first_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = WoBle::kHeaderFlag_StartMessage;
    data[1] = 1;
    data[2] = 2;
    data[3] = 0;
    data[4] = 0xfe; // payload

    err = woble.HandleCharacteristicReceived(first_packet, rcvd_ack, did_rcv_ack);
    first_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_InProgress);

    second_packet = PacketBuffer::NewWithAvailableSize(3);
    second_packet->SetDataLength(3, NULL);
    data = second_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = WoBle::kHeaderFlag_EndMessage;
    data[1] = 2;
    data[2] = 0xff; // payload

    err = woble.HandleCharacteristicReceived(second_packet, rcvd_ack, did_rcv_ack);
    second_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_Complete);

    data = NULL;
    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "HandleCharacteristicReceivedTwoPacket with Woble State:");
    woble.LogState();
}

static void HandleCharacteristicReceivedThreePacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    PacketBuffer * second_packet;
    PacketBuffer * last_packet;
    SequenceNumber_t rcvd_ack;
    bool did_rcv_ack;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * data;

    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "Start HandleCharacteristicReceivedThreePacket Woble State:");
    woble.LogState();

    first_packet = PacketBuffer::NewWithAvailableSize(10);
    first_packet->SetDataLength(5, NULL);
    data = first_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = WoBle::kHeaderFlag_StartMessage;
    data[1] = 1;
    data[2] = 3;
    data[3] = 0;
    data[4] = 0xfd; // payload

    err = woble.HandleCharacteristicReceived(first_packet, rcvd_ack, did_rcv_ack);
    first_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_InProgress);

    second_packet = PacketBuffer::NewWithAvailableSize(3);
    second_packet->SetDataLength(3, NULL);
    data = second_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = WoBle::kHeaderFlag_ContinueMessage;
    data[1] = 2;
    data[4] = 0xfe; // payload

    err = woble.HandleCharacteristicReceived(second_packet, rcvd_ack, did_rcv_ack);
    second_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_InProgress);

    last_packet = PacketBuffer::NewWithAvailableSize(3);
    last_packet->SetDataLength(3, NULL);
    data = last_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = WoBle::kHeaderFlag_EndMessage;
    data[1] = 3;
    data[4] = 0xff; // payload

    err = woble.HandleCharacteristicReceived(last_packet, rcvd_ack, did_rcv_ack);
    last_packet = NULL;
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, woble.RxState() == WoBle::kState_Complete);

    data = NULL;
    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "HandleCharacteristicReceivedThreePacket with Woble State:");
    woble.LogState();
}

static void HandleCharacteristicSendOnePacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    bool rc = false;
    uint8_t * data;

    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "Start HandleCharacteristicSendOnePacket Woble State:");
    woble.LogState();

    first_packet = PacketBuffer::NewWithAvailableSize(10);
    first_packet->SetDataLength(1, NULL);
    data = first_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    data[0] = 0xff; // payload

    rc = woble.HandleCharacteristicSend(first_packet, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 5);

    first_packet = NULL;

    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_Complete);

    NL_TEST_ASSERT(inSuite, rc);

    data = NULL;
    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "HandleCharacteristicSendOnePacket with Woble State:");
    woble.LogState();
}

static void HandleCharacteristicSendTwoPacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    uint8_t * data = NULL;
    bool rc = false;

    woble.Init(NULL, false);
    WeaveLogDetail(Ble, "Start HandleCharacteristicSendTwoPacket Woble State:");
    woble.LogState();
    first_packet = PacketBuffer::NewWithAvailableSize(30);
    first_packet->SetDataLength(30, NULL);
    data = first_packet->Start();
    NL_TEST_ASSERT(inSuite, data != NULL);

    for (int i = 0; i < 30; i++)
    {
        data[i] = i; // payload
    }

    rc = woble.HandleCharacteristicSend(first_packet, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 20);

    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_InProgress);

    rc = woble.HandleCharacteristicSend(NULL, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 16);

    woble.LogState();

    first_packet = NULL;
    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_Complete);

    data = NULL;
    woble.Init(NULL, false);
    WeaveLogDetail(Ble, "HandleCharacteristicSendTwoPacket with Woble State:");
    woble.LogState();
}

// Send 40-byte payload.
// First packet: 4 byte header + 16 byte payload
// Second packet: 2 byte header + 18 byte payload
// Third packet: 2 byte header + 6 byte payload
static void HandleCharacteristicSendThreePacket(nlTestSuite *inSuite, void *inContext)
{
    PacketBuffer * first_packet;
    bool rc = false;
    uint8_t * data = NULL;

    woble.Init(NULL, false);
    WeaveLogDetail(Ble, "Start HandleCharacteristicSendThreePacket Woble State:");
    woble.LogState();

    first_packet = PacketBuffer::NewWithAvailableSize(40);
    first_packet->SetDataLength(40, NULL);
    data = first_packet->Start();

    NL_TEST_ASSERT(inSuite, data != NULL);

    for (int i = 0; i < 40; i++)
    {
        data[i] = i; // payload
    }

    rc = woble.HandleCharacteristicSend(first_packet, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 20);

    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_InProgress);

    rc = woble.HandleCharacteristicSend(NULL, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 20);

    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_InProgress);

    rc = woble.HandleCharacteristicSend(NULL, false);
    NL_TEST_ASSERT(inSuite, rc);

    NL_TEST_ASSERT(inSuite, first_packet->DataLength() == 8);

    first_packet = NULL;

    NL_TEST_ASSERT(inSuite, woble.TxState() == WoBle::kState_Complete);

    data = NULL;
    woble.Init(NULL, false);

    WeaveLogDetail(Ble, "HandleCharacteristicSendThreePacket with Woble State:");
    woble.LogState();
}



/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicReceivedOnePacket",             HandleCharacteristicReceivedOnePacket),
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicReceivedTwoPacket",             HandleCharacteristicReceivedTwoPacket),
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicReceivedThreePacket",           HandleCharacteristicReceivedThreePacket),
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicSendOnePacket",                 HandleCharacteristicSendOnePacket),
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicSendTwoPacket",                 HandleCharacteristicSendTwoPacket),
    NL_TEST_DEF("Weave Over BLE HandleCharacteristicSendThreePacket",               HandleCharacteristicSendThreePacket),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 *  This is a work-around to initiate InetBuffer protected class instance's
 *  data and set it to a known state, before an instance is created.
 */
static int TestSetup(void *inContext)
{
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 *  Free memory reserved at TestSetup.
 */
static int TestTeardown(void *inContext)
{
    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    nlTestSuite theSuite = {
        "WeaveOverBle",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one context.
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

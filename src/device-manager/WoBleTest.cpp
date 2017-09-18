/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file contains the implementation of
 *      WoBle Control Path and Troughput Test
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <BuildConfig.h>
#if CONFIG_NETWORK_LAYER_BLE

#include <string.h>

#include <Weave/Core/WeaveConfig.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/FlagUtils.hpp>

#include <BleLayer/BleConfig.h>
#include <BleLayer/BLEEndPoint.h>
#include <BleLayer/BleLayer.h>
#include <BleLayer/WoBle.h>
#if WEAVE_ENABLE_WOBLE_TEST
#include "WoBleTest.h"
#endif

// Define below to enable extremely verbose, BLE end point-specific debug logging.
#undef NL_BLE_END_POINT_DEBUG_LOGGING_ENABLED

#ifdef NL_BLE_END_POINT_DEBUG_LOGGING_ENABLED
#define WeaveLogDebugBleEndPoint(MOD, MSG, ...) WeaveLogError(MOD, MSG, ## __VA_ARGS__)
#else
#define WeaveLogDebugBleEndPoint(MOD, MSG, ...)
#endif

namespace nl {
namespace Ble {

#if WEAVE_ENABLE_WOBLE_TEST

BLE_ERROR HandleCommandTest(void *ble, BLE_CONNECTION_OBJECT connObj, uint32_t packetCount,
        uint32_t duration, uint16_t txGap, uint8_t needAck, uint16_t payloadSize, bool reverse)
{
    BLE_ERROR err = BLE_NO_ERROR;
    BLEEndPoint *endPoint = ((nl::Ble::BleLayer *)ble)->mTestBleEndPoint;

    if (endPoint == NULL)
    {
        WeaveLogError(Ble, "no endpoint for BLE sent data");
        return BLE_ERROR_BAD_ARGS;
    }
    
    WeaveLogDebugBleEndPoint(Ble, "%s: Start count %u, duration %u, ack %u, size %u, reverse %u\n",
            __FUNCTION__, packetCount, duration, needAck, payloadSize, reverse);

    endPoint->mWoBleTest.mCommandTestRequest.PacketCount = packetCount;
    endPoint->mWoBleTest.mCommandTestRequest.Duration = duration;
    endPoint->mWoBleTest.mCommandTestRequest.TxGap = txGap;
    endPoint->mWoBleTest.mCommandTestRequest.NeedAck = needAck;
    if (payloadSize < COMMAND_TESTDATA_HDR_LEN)
        payloadSize = COMMAND_TESTDATA_HDR_LEN;
    // Actual payload includes the test data header
    endPoint->mWoBleTest.mCommandTestRequest.PayloadSize = payloadSize - COMMAND_TESTDATA_HDR_LEN;
    if (reverse == true)
    {
        err = endPoint->mWoBleTest.DoCommandTestRequest(endPoint);
    }
    else
    {
        err = endPoint->mWoBleTest.HandleCommandTest(endPoint);
    }

    return err;
}

BLE_ERROR HandleCommandTestResult(void *ble, BLE_CONNECTION_OBJECT connObj, bool local)
{
    BLE_ERROR err = BLE_NO_ERROR;
    BLEEndPoint *ep = ((nl::Ble::BleLayer *)ble)->mTestBleEndPoint;

    if (ep == NULL)
    {
        WeaveLogError(Ble, "no endpoint for BLE sent data");
        return BLE_ERROR_BAD_ARGS;
    }
    
    if (local == false)
        err = ep->mWoBleTest.DoCommandTestResult(kBleCommandTestResult_Request, 0);
    else
        WoBleTest::LogBleTestResult(&ep->mWoBleTest.mCommandTestResult);

    return err;
}

BLE_ERROR HandleCommandTestAbort(void *ble, BLE_CONNECTION_OBJECT connObj)
{
    BLE_ERROR err = BLE_NO_ERROR;
    BLEEndPoint *endPoint = ((nl::Ble::BleLayer *)ble)->mTestBleEndPoint;

    if (endPoint == NULL)
    {
        WeaveLogError(Ble, "no endpoint for BLE sent ABORT");
        return err;
    }
    
    err = endPoint->mWoBleTest.DoCommandTestAbort(-1);

    return err;
}

BLE_ERROR HandleCommandTxTiming(void *ble, BLE_CONNECTION_OBJECT connObj, bool enabled, bool remote)
{
    BLE_ERROR err = BLE_NO_ERROR;
    BLEEndPoint *ep = ((nl::Ble::BleLayer *)ble)->mTestBleEndPoint;

    if (ep == NULL)
    {
        WeaveLogError(Ble, "no endpoint for BLE sent data");
        return BLE_ERROR_BAD_ARGS;
    }

    if (remote)
        err = ep->mWoBleTest.DoCommandTxTiming(enabled);
    else
        err = ep->mWoBleTest.HandleCommandTxTiming(ep, enabled);

    return err;
}


// BleTransportCommandMessage implementation:

BLE_ERROR BleTransportCommandMessage::Encode(PacketBuffer *msgBuf, BleTransportCommandMessage& cmd)  const
{
    uint8_t *p = msgBuf->Start();
    BLE_ERROR err = BLE_NO_ERROR;

    // Verify we can write the fixed-length request without running into the end of the buffer.
    VerifyOrExit(msgBuf->MaxDataLength() > COMMAND_HEADER_LEN, err = BLE_ERROR_NO_MEMORY);

    nl::Weave::Encoding::LittleEndian::Write16(p, cmd.CmdHdr.PacketLength);
    nl::Weave::Encoding::Write8(p, cmd.CmdHdr.Version);

    nl::Weave::Encoding::Write8(p, cmd.CmdHdr.PacketType);

    switch (cmd.CmdHdr.PacketType) {
        case kBleCommandType_TestAck:
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTestAck.Type);
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTestAck.SequenceNumber);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestAck.ResultCode);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_ACK_HDR_LEN);
            break;
        case kBleCommandType_TestData:
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTestData.Type);
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTestData.NeedAck);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestData.Length);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestData.Sequence);
            VerifyOrExit(Data != NULL && cmd.Payload.MsgTestData.Length <= BLE_TEST_DATA_MAX_LEN, err = BLE_ERROR_NO_MEMORY);
            memcpy(p, Data, cmd.Payload.MsgTestData.Length);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_DATA_HDR_LEN + cmd.Payload.MsgTestData.Length);
            break;
        case kBleCommandType_TestRequest:
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestRequest.PacketCount);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestRequest.Duration);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestRequest.TxGap);
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTestRequest.NeedAck);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestRequest.PayloadSize);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_TESTREQ_HDR_LEN);
            break;
        case kBleCommandType_TestResult:
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TestResultOp);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.TestResult);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.PacketCount);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.Duration);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.AckCount);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.TxDrops);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxGap);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.PayloadSize);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxPktCount);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.TxTimeMs);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxTimeMax);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxTimeMin);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxAckCount);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.TxAckTimeMs);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxAckTimeMax);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxAckTimeMin);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.TxTimeLastMs);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgTestResult.PayloadLast);
            nl::Weave::Encoding::LittleEndian::Write32(p, cmd.Payload.MsgTestResult.PayloadBytes);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_TESTRESULT_HDR_LEN);
            break;
        case kBleCommandType_WobleMTU:
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgWobleMTU.Op);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgWobleMTU.TxFragmentSize);
            nl::Weave::Encoding::LittleEndian::Write16(p, cmd.Payload.MsgWobleMTU.RxFragmentSize);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_WOBLEMTU_HDR_LEN);
            break;
        case kBleCommandType_WobleWindowSize:
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgWobleWindowSize.Op);
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgWobleWindowSize.TxWindowSize);
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgWobleWindowSize.RxWindowSize);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_WINDOWSIZE_HDR_LEN);
            break;
        case kBleCommandType_TxTiming:
            nl::Weave::Encoding::Write8(p, cmd.Payload.MsgTxTiming.Enable);
            msgBuf->SetDataLength(COMMAND_HEADER_LEN + COMMAND_TXTIMING_HDR_LEN);
            break;
        default:
            WeaveLogError(Ble, "%s: Not yet support", __FUNCTION__);
            break;
    }

exit:
    return err;
}

BLE_ERROR BleTransportCommandMessage::Decode(const PacketBuffer &msgBuf, BleTransportCommandMessage& cmd)
{
    const uint8_t *p = msgBuf.Start();
    BLE_ERROR err = BLE_NO_ERROR;

    // Verify we can read the fixed-length response without running into the end of the buffer.
    VerifyOrExit(msgBuf.DataLength() >= COMMAND_HEADER_LEN, err = BLE_ERROR_MESSAGE_INCOMPLETE);

    cmd.CmdHdr.PacketLength = nl::Weave::Encoding::LittleEndian::Read16(p);
    cmd.CmdHdr.Version = nl::Weave::Encoding::Read8(p);
    cmd.CmdHdr.PacketType = nl::Weave::Encoding::Read8(p);

    switch (cmd.CmdHdr.PacketType) {
        case kBleCommandType_TestAck:
            cmd.Payload.MsgTestAck.Type = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgTestAck.SequenceNumber = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgTestAck.ResultCode = nl::Weave::Encoding::LittleEndian::Read32(p);
            break;
        case kBleCommandType_TestData:
            cmd.Payload.MsgTestData.Type = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgTestData.NeedAck = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgTestData.Length = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestData.Sequence = nl::Weave::Encoding::LittleEndian::Read32(p);
            VerifyOrExit(cmd.Data != NULL && cmd.Payload.MsgTestData.Length <= BLE_TEST_DATA_MAX_LEN,
                    err = BLE_ERROR_NO_MEMORY);
            memcpy(cmd.Data, p, cmd.Payload.MsgTestData.Length);
            break;
        case kBleCommandType_TestRequest:
            cmd.Payload.MsgTestRequest.PacketCount = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestRequest.Duration = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestRequest.TxGap = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestRequest.NeedAck = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgTestRequest.PayloadSize = nl::Weave::Encoding::LittleEndian::Read16(p);
            break;
        case kBleCommandType_TestResult:
            cmd.Payload.MsgTestResult.TestResultOp = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TestResult = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.PacketCount = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.Duration = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.AckCount = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.TxDrops = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.TxGap = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.PayloadSize = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxPktCount = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxTimeMs = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.TxTimeMax = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxTimeMin = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxAckCount = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxAckTimeMs = nl::Weave::Encoding::LittleEndian::Read32(p);
            cmd.Payload.MsgTestResult.TxAckTimeMax = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxAckTimeMin = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.TxTimeLastMs = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.PayloadLast = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgTestResult.PayloadBytes = nl::Weave::Encoding::LittleEndian::Read32(p);
            break;
        case kBleCommandType_WobleMTU:
            cmd.Payload.MsgWobleMTU.Op = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgWobleMTU.TxFragmentSize = nl::Weave::Encoding::LittleEndian::Read16(p);
            cmd.Payload.MsgWobleMTU.RxFragmentSize = nl::Weave::Encoding::LittleEndian::Read16(p);
            break;
        case kBleCommandType_WobleWindowSize:
            cmd.Payload.MsgWobleWindowSize.Op = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgWobleWindowSize.TxWindowSize = nl::Weave::Encoding::Read8(p);
            cmd.Payload.MsgWobleWindowSize.RxWindowSize = nl::Weave::Encoding::Read8(p);
            break;
        case kBleCommandType_TxTiming:
            cmd.Payload.MsgTxTiming.Enable = nl::Weave::Encoding::Read8(p);
            break;
        default:
            WeaveLogError(Ble, "%s: Not yet support", __FUNCTION__);
            break;
    }

exit:
    if (err != BLE_NO_ERROR)
            WeaveLogError(Ble, "%s: ERROR = %d", __FUNCTION__, err);

    return err;
}
    
BLE_ERROR WoBleTest::Init(BLEEndPoint *ep)
{
    BLE_ERROR err = BLE_NO_ERROR;

    mCommandUnderTest = WOBLE_TEST_NONE;
    mCommandTxTiming = false;
    mCommandReceiveQueue = NULL;
    mCommandSendQueue = NULL;
    mCommandAckToSend = NULL;
    memset(&mTestTxThread, 0, sizeof(mTestTxThread));
    memset(&mTxHistogram, 0, sizeof(mTxHistogram));

    mMainThread = pthread_self();
    mEp = ep;

    WeaveLogError(Ble, "%s: Initialize WoBleTest, ep->%#p, thread %x", __FUNCTION__, ep, mMainThread);

    // Register the command handler
    ep->SetOnCommandReceivedCB(HandleCommandReceived);

    return err;
}

void WoBleTest::Decode(uint8_t *src, uint8_t *dst, size_t size)
{
    switch (size)
    {
        case sizeof(int8_t):
            *dst = *src;
            break;
        case sizeof(int16_t):
            {
                int16_t i = nl::Weave::Encoding::LittleEndian::Read16(src);
                memcpy(dst, &i, size);
            }
            break;
        case sizeof(int32_t):
            {
                int32_t i = nl::Weave::Encoding::LittleEndian::Read32(src);
                memcpy(dst, &i, size);
            }
            break;
        case sizeof(int64_t):
            {
                int64_t i = nl::Weave::Encoding::LittleEndian::Read64(src);
                memcpy(dst, &i, size);
            }
            break;
        default:
            WeaveLogError(Ble, "%s: unsupported size %u\n", __FUNCTION__, size);
            break;
    }
}

// This is the WoBle Command Handler
void WoBleTest::HandleCommandReceived(BLEEndPoint *ep, PacketBuffer *data)
{
    BLE_ERROR err = BLE_NO_ERROR;
    Weave::System::Error timerErr;

    // Fail if any data missing
    VerifyOrExit(ep != NULL && data != NULL, err = BLE_ERROR_BAD_ARGS);

    // Add new message to send queue.
    if (ep->mWoBleTest.mCommandReceiveQueue == NULL)
    {
        WeaveLogDebugBleEndPoint(Ble, "set data as new mCommandReceiveQueue");
        ep->mWoBleTest.mCommandReceiveQueue = data;
    }
    else
    {
        WeaveLogDebugBleEndPoint(Ble, "added data to end");
        ep->mWoBleTest.mCommandReceiveQueue->AddToEnd(data);
    }
    data = NULL;

    // Handle BTP command in timer
    timerErr = ep->mBle->mSystemLayer->StartTimer(0, HandleCommandPacket, ep);
    VerifyOrExit(timerErr == BLE_NO_ERROR, err = BLE_ERROR_START_TIMER_FAILED);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: Error %d", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return;
}

// This is the WoBle TxTiming Handler
void WoBleTest::DoTxTiming(PacketBuffer *data, int stage)
{
    if (mCommandUnderTest == WOBLE_TEST_NONE && mCommandTxTiming == false)
            return;     // TxTiming is not enabled

    WeaveLogDebugBleEndPoint(Ble, "%s: stage %d, mode %u:%u, data->%p, type %u, len %u",
            __FUNCTION__, stage, mCommandUnderTest, mCommandTxTiming, data,
            mEp->mWoBle.TxPacketType(), data->DataLength());

    if (!mCommandTxTiming && (mCommandUnderTest && mEp->mWoBle.TxPacketType() != kType_Control))
            return;     // TxTiming is not required

    switch(stage) {
        case WOBLE_TX_START:
            mTimeStats.mTxStartMs = nl::Weave::System::Timer::GetCurrentEpoch();
            mTimeStats.mTxPayload = data->DataLength();
            break;

        case WOBLE_TX_DONE:
            if (mTimeStats.mTxStartMs)
            {
                // Compute Tx time and collect the statistics
                int diff = nl::Weave::System::Timer::GetCurrentEpoch() - mTimeStats.mTxStartMs;

                AddTxRecord(mTimeStats.mTxStartMs, diff, mTimeStats.mTxPayload);

                // Always keep the TxTime of last WoBle packet
                mCommandTestResult.TxTimeLastMs = diff;
                mCommandTestResult.PayloadLast = mTimeStats.mTxPayload;
                mCommandTestResult.TxTimeMs += diff;
                if (mCommandTestResult.TxTimeMax == 0)
                    mCommandTestResult.TxTimeMax =
                    mCommandTestResult.TxTimeMin = diff;
                else
                    if (diff > mCommandTestResult.TxTimeMax)
                        mCommandTestResult.TxTimeMax = diff;
                    else
                        if (diff < mCommandTestResult.TxTimeMin)
                            mCommandTestResult.TxTimeMin = diff;

                mCommandTestResult.TxPktCount++;
                mCommandTestResult.PayloadBytes += mCommandTestResult.PayloadLast;

                // Check if we're done with this packet
                if (mCommandTestRequest.NeedAck == true)
                    mTimeStats.mTxAckStartMs = mTimeStats.mTxStartMs;
                mTimeStats.mTxStartMs = 0;

                WeaveLogDebugBleEndPoint(Ble, "%s: TxTimeMs %u, TxPktCount %u, PayloadBytes %u",
                        __FUNCTION__, mCommandTestResult.TxTimeMs,
                        mCommandTestResult.TxPktCount, mCommandTestResult.PayloadBytes);
                WeaveLogDebugBleEndPoint(Ble, "%s: TxTimeLastMs %u, PayloadLast %u",
                        __FUNCTION__, mCommandTestResult.TxTimeLastMs, mCommandTestResult.PayloadLast);

                // Send the next packet if it's already past due, so
                // no need to wait for the next Tx time. It's to eliminate the Tx idle gap.
                if (mCommandUnderTest == WOBLE_TEST_TX && diff >= mCommandTestRequest.TxGap)
                    mEp->mBle->mSystemLayer->StartTimer(0, DoTestDataSend, (void *)mEp);
            }

            if (mCommandUnderTest == WOBLE_TEST_TX && mEp->mWoBleTest.mCommand.CommandTest_Duration <= 0
                    && mEp->mWoBleTest.mCommand.CommandTest_PacketCount == 0)
            {
                WeaveLogError(Ble, "%s: *** Finished sending last Tx test packet", __FUNCTION__);
                // We just finished sending the last test packet
                mCommandUnderTest = WOBLE_TEST_NONE;
            }
            break;

        case WOBLE_TX_DATA_ACK:
            if (mTimeStats.mTxAckStartMs)
            {
                // Compute Tx+Ack time and collect the statistics
                int diff = nl::Weave::System::Timer::GetCurrentEpoch() -
                    mTimeStats.mTxAckStartMs;

                mCommandTestResult.TxAckTimeMs += diff;
                if (mCommandTestResult.TxAckTimeMax == 0)
                    mCommandTestResult.TxAckTimeMax =
                    mCommandTestResult.TxAckTimeMin = diff;
                else
                    if (diff > mCommandTestResult.TxAckTimeMax)
                        mCommandTestResult.TxAckTimeMax = diff;
                    else
                        if (diff < mCommandTestResult.TxAckTimeMin)
                            mCommandTestResult.TxAckTimeMin = diff;

                mCommandTestResult.TxAckCount++;

                // Done with this packet
                mTimeStats.mTxAckStartMs = 0;
                WeaveLogDebugBleEndPoint(Ble, "%s: TxAckTimeMs %u, TxAckCount %u", __FUNCTION__,
                    mCommandTestResult.TxAckTimeMs, mCommandTestResult.TxAckCount);
            }
            break;

        default:
            break;
    }
}

/*
 * Check whether it's a null pointer or containing all 0s
 */
bool isEmptyData(char *data, size_t size)
{
    if (data == NULL)   return true;

    while (size-- > 0)
        if (*data++ != 0)
            return false;

    return true;
}

void WoBleTest::DoTestDataSend(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err)
{
    BLEEndPoint *ep = static_cast<BLEEndPoint *>(appState);
    PacketBuffer *data = NULL;
    uint8_t type;
    pthread_t curThread = pthread_self();

    if (ep == NULL || ep->mState == BLEEndPoint::kState_Closed
            || ep->mWoBleTest.mCommandUnderTest != WOBLE_TEST_TX)
    {
        WeaveLogDebugBleEndPoint(Ble, "%s: State = %d, mCommandUnderTest %d, thread %x",
                __FUNCTION__, ep? ep->mState : -1, ep->mWoBleTest.mCommandUnderTest, curThread);
        ExitNow();
    }

    if (!(isEmptyData((char *)&ep->mWoBleTest.mTestTxThread, sizeof(pthread_t))
        || pthread_equal(curThread, ep->mWoBleTest.mTestTxThread)
        || pthread_equal(curThread, ep->mWoBleTest.mMainThread)))
    {
        WeaveLogDebugBleEndPoint(Ble, "%s: Keep Tx thread (id %x) and stop %x",
            __FUNCTION__ , ep->mWoBleTest.mTestTxThread, curThread);
        pthread_exit(NULL); // Suicide is the safest way to kill a thread
    }

    if (ep->mWoBleTest.mCommandTestResult.PacketCount++ == 0)
    {
        memcpy(&ep->mWoBleTest.mTestTxThread, &curThread, sizeof(pthread_t));
        WeaveLogDebugBleEndPoint(Ble, "%s: Tx thread started (id %x)", __FUNCTION__, ep->mWoBleTest.mTestTxThread);
        ep->mWoBleTest.mCommand.CommandTest_Start = true;
        SetFlag(ep->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung, true);
        ep->mWoBleTest.mCommand.CommandTest_Duration = (int32_t)ep->mWoBleTest.mCommandTestRequest.Duration;
        ep->mWoBleTest.mCommand.CommandTest_PacketCount = (int32_t)ep->mWoBleTest.mCommandTestRequest.PacketCount;
        ep->mWoBleTest.mCommandTestResult.TxGap = ep->mWoBleTest.mCommandTestRequest.TxGap;
        ep->mWoBleTest.mCommandTestResult.PayloadSize = ep->mWoBleTest.mCommandTestRequest.PayloadSize;
        WeaveLogDebugBleEndPoint(Ble, "\n%s: Count %d, Duration %u, TxGap %u, Ack %d, Size %u",
                __FUNCTION__, ep->mWoBleTest.mCommandTestRequest.PacketCount,
                ep->mWoBleTest.mCommandTestRequest.Duration, ep->mWoBleTest.mCommandTestRequest.TxGap,
                ep->mWoBleTest.mCommandTestRequest.NeedAck, ep->mWoBleTest.mCommandTestRequest.PayloadSize);
    }
    else
        type = kDataType_CONTINUE;

    // Check if it's the last test packet
    if (ep->mWoBleTest.mCommand.CommandTest_Duration > 0)
    {
        ep->mWoBleTest.mCommand.CommandTest_Duration -= ep->mWoBleTest.mCommandTestRequest.TxGap;
        if ((int)ep->mWoBleTest.mCommand.CommandTest_Duration <= 0)
            type = kDataType_END;
    }
    if (ep->mWoBleTest.mCommand.CommandTest_PacketCount > 0)
    {
        if (--(ep->mWoBleTest.mCommand.CommandTest_PacketCount) == 0)
            type = kDataType_END;
    }

    if (ep->mSendQueue == NULL) // Allow Tx test data only when Tx queue is empty
    {
        if (ep->mWoBleTest.mCommand.CommandTest_Start == true)
        {
            type = kDataType_START;
            ep->mWoBleTest.mCommand.CommandTest_Start = false;
        }
        data = PacketBuffer::New();
        WeaveLogDebugBleEndPoint(Ble, "%s: Tx pkt# %d, data->%p, TxGap %d, duration %d, count %d",
                __FUNCTION__, ep->mWoBleTest.mCommandTestResult.PacketCount, data,
                ep->mWoBleTest.mCommandTestRequest.TxGap, ep->mWoBleTest.mCommand.CommandTest_Duration,
                ep->mWoBleTest.mCommand.CommandTest_PacketCount);
        VerifyOrExit(data != NULL, err = BLE_ERROR_NO_MEMORY);
        ep->mWoBleTest.mCommand.CmdHdr.PacketLength =
                sizeof(ep->mWoBleTest.mCommand.Payload.MsgTestData) +
                ep->mWoBleTest.mCommandTestRequest.PayloadSize;
        ep->mWoBleTest.mCommand.CmdHdr.Version = 0;
        ep->mWoBleTest.mCommand.CmdHdr.PacketType = kBleCommandType_TestData;
        ep->mWoBleTest.mCommand.Payload.MsgTestData.Type = type;
        ep->mWoBleTest.mCommand.Payload.MsgTestData.NeedAck = ep->mWoBleTest.mCommandTestRequest.NeedAck;
        ep->mWoBleTest.mCommand.Payload.MsgTestData.Length = ep->mWoBleTest.mCommandTestRequest.PayloadSize;
        ep->mWoBleTest.mCommand.Payload.MsgTestData.Sequence = ep->mWoBleTest.mCommandTestResult.PacketCount;

        err = ep->mWoBleTest.mCommand.Encode(data, ep->mWoBleTest.mCommand);
        SuccessOrExit(err);

        // Add new data to send queue.
        ep->QueueTx(data, kType_Control);
        data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

        err = ep->DriveSending();
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogDebugBleEndPoint(Ble, "%s: GATT ON, Dropping pkt %d, TxGap %d, duration %d",
                __FUNCTION__, ep->mWoBleTest.mCommandTestResult.PacketCount,
                ep->mWoBleTest.mCommandTestRequest.TxGap, ep->mWoBleTest.mCommand.CommandTest_Duration);
        ep->mWoBleTest.mCommandTestResult.TxDrops++;
    }

    ep->mWoBleTest.mCommandTestResult.Duration += ep->mWoBleTest.mCommandTestRequest.TxGap; // in ms

    // Check if it's the last packet
    if (type == kDataType_END)
    {
        SetFlag(ep->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung, false);
        ep->mWoBleTest.mCommand.CommandTest_Duration = ep->mWoBleTest.mCommand.CommandTest_PacketCount = 0;
        WeaveLogDebugBleEndPoint(Ble, "%s: Tx Test Done (id %x)", __FUNCTION__, curThread);
    }

    systemLayer->StartTimer(ep->mWoBleTest.mCommandTestRequest.TxGap, DoTestDataSend, (void *)ep);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return;
}

void * WoBleTest::StartTxThread(void * data)
{
    BLEEndPoint *ep = static_cast<BLEEndPoint *>(data);
    BLE_ERROR err = BLE_NO_ERROR;
    Weave::System::Error timerErr;

    timerErr = ep->mBle->mSystemLayer->StartTimer(0, DoTestDataSend, ep);
    VerifyOrExit(timerErr == BLE_NO_ERROR, err = BLE_ERROR_START_TIMER_FAILED);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: Error %d", __FUNCTION__, err);

    WeaveLogDebugBleEndPoint(Ble, "%s: Thread exited (id %x)", __FUNCTION__, pthread_self());

    return NULL;
}

BLE_ERROR WoBleTest::HandleCommandTest(BLEEndPoint *ep)
{
    BLE_ERROR err = BLE_NO_ERROR;
    pthread_t newThread;

    ep->mWoBleTest.mCommandUnderTest = WOBLE_TEST_TX;
    memset(&ep->mWoBleTest.mCommandTestResult, 0, sizeof(BTCommandTypeTestResult));
    memset((char *)&ep->mWoBleTest.mTimeStats, 0, sizeof(ep->mWoBleTest.mTimeStats));

    // Start the test
    memset(ep->mWoBleTest.mCommand.Data, 0xff, BLE_TEST_DATA_MAX_LEN);
    err = (BLE_ERROR)pthread_create(&newThread, NULL, StartTxThread, (void *)ep);
    WeaveLogDebugBleEndPoint(Ble, "%s: Started thread (id %x), err %d", __FUNCTION__, newThread, err);
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: Error %d", __FUNCTION__, err);

    return err;
}

// This function enables/disables the Tx Timing and Histogram
BLE_ERROR WoBleTest::HandleCommandTxTiming(BLEEndPoint *ep, bool enabled)
{
    WeaveLogDebugBleEndPoint(Ble, "%s: enabled = %d", __FUNCTION__, enabled);

    ep->mWoBleTest.mCommandTxTiming = enabled;
    if (enabled)
    {
        memset(&ep->mWoBleTest.mCommandTestResult, 0, sizeof(BTCommandTypeTestResult));
        memset((char *)&ep->mWoBleTest.mTimeStats, 0, sizeof(ep->mWoBleTest.mTimeStats));
        if (ep->mWoBleTest.InitTxHistogram(WOBLE_TX_HISTOGRAM_FILE, WOBLE_TX_RECORD_COUNT, true) < 0)
            WeaveLogError(Ble, "%s: Warning - No Tx Histogram\n", __FUNCTION__);
    }
    else
        ep->mWoBleTest.DoneTxHistogram(true);

    return BLE_NO_ERROR;
}

BLE_ERROR WoBleTest::DoCommandSendAck(SequenceNumber_t seqNum, int32_t result)
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTestAck);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TestAck;
    mCommand.Payload.MsgTestAck.Type = result? kAckType_OK : kAckType_NOK;
    mCommand.Payload.MsgTestAck.SequenceNumber = seqNum;
    mCommand.Payload.MsgTestAck.ResultCode = result;

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    mEp->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = mEp->DriveSending();
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

BLE_ERROR WoBleTest::DoCommandTestResult(int32_t op, int32_t result)
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTestResult);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TestResult;
    mCommand.Payload.MsgTestResult.TestResultOp = op;
    if (op == kBleCommandTestResult_Reply)
    {
        mCommand.Payload.MsgTestResult = mCommandTestResult;
        mCommand.Payload.MsgTestResult.TestResult = result;
    }

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    // Add packet to send queue head
    mEp->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = mEp->DriveSending();
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

BLE_ERROR WoBleTest::DoCommandTxTiming(bool enable)
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTxTiming);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TxTiming;
    mCommand.Payload.MsgTxTiming.Enable = enable;

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    // Add packet to send queue head
    mEp->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = mEp->DriveSending();
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

BLE_ERROR WoBleTest::DoCommandTestAbort(int32_t result)
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    WeaveLogDebugBleEndPoint(Ble, "%s: result %d\n", __FUNCTION__, result);
    if (mCommandUnderTest == WOBLE_TEST_NONE)
        return err;

    StopTestTimer();
    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTestData);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TestData;
    mCommand.Payload.MsgTestData.Type = kDataType_ABORT;
    mCommand.Payload.MsgTestData.NeedAck = false;
    mCommand.Payload.MsgTestData.Length = 0;

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    mEp->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = mEp->DriveSending();
    SuccessOrExit(err);

exit:
    mCommandUnderTest = WOBLE_TEST_NONE;    // always terminate local test

    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

BLE_ERROR WoBleTest::DoCommandRequestTestResult()
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTestResult);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TestResult;
    mCommand.Payload.MsgTestResult.TestResultOp = kBleCommandTestResult_Request;
    mCommand.Payload.MsgTestResult.PacketCount = 0;

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    // Add packet to send queue head
    mEp->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = mEp->DriveSending();
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

BLE_ERROR WoBleTest::DoCommandTestRequest(BLEEndPoint *ep)
{
    BLE_ERROR err = BLE_NO_ERROR;
    PacketBuffer *data;

    WeaveLogDebugBleEndPoint(Ble, "%s: Sending TestRequest to device...\n", __FUNCTION__);

    data = PacketBuffer::New();
    mCommand.CmdHdr.PacketLength = sizeof(mCommand.Payload.MsgTestRequest);
    mCommand.CmdHdr.Version = 0;
    mCommand.CmdHdr.PacketType = kBleCommandType_TestRequest;
    mCommand.Payload.MsgTestRequest.PacketCount = mCommandTestRequest.PacketCount;
    mCommand.Payload.MsgTestRequest.Duration = mCommandTestRequest.Duration;
    mCommand.Payload.MsgTestRequest.TxGap = mCommandTestRequest.TxGap;
    mCommand.Payload.MsgTestRequest.NeedAck = mCommandTestRequest.NeedAck;
    mCommand.Payload.MsgTestRequest.PayloadSize = mCommandTestRequest.PayloadSize;

    err = mCommand.Encode(data, mCommand);
    SuccessOrExit(err);

    ep->QueueTx(data, kType_Control);
    data = NULL; // Buffer freed when send queue freed on close, or on completion of current message transmission.

    err = ep->DriveSending();
    SuccessOrExit(err);

exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return err;
}

void WoBleTest::LogBleTestResult(BTCommandTypeTestResult *result)
{
    WeaveLogDebugBleEndPoint(Ble, "%s: PacketCount %u, TxPktCount %u, TxAckCount %u", __FUNCTION__, 
                result->PacketCount, result->TxPktCount, result->TxAckCount); 

    if (result->PacketCount > 0)
    {
        WeaveLogError(Ble, "TestResult  : %d", result->TestResult);
        WeaveLogError(Ble, "PacketCount : %u", result->PacketCount);
        WeaveLogError(Ble, "Duration    : %u", result->Duration);
        WeaveLogError(Ble, "AckCount    : %u", result->AckCount);
        WeaveLogError(Ble, "TxDrops     : %u", result->TxDrops);
        WeaveLogError(Ble, "TxGap       : %u", result->TxGap);
        // Actual payload includes the test data header
        WeaveLogError(Ble, "PayloadSize : %u", result->PayloadSize + COMMAND_TESTDATA_HDR_LEN);
    }
    if (result->TxPktCount > 0)
    {
        WeaveLogError(Ble, "=========================");
        WeaveLogError(Ble, "Last Tx time        : %u", result->TxTimeLastMs);
        WeaveLogError(Ble, "Last Payload Bytes  : %u", result->PayloadLast);
        WeaveLogError(Ble, "=========================");
        WeaveLogError(Ble, "Tx Packet Count     : %u", result->TxPktCount);
        WeaveLogError(Ble, "Total Payload Bytes : %u", result->PayloadBytes);
        WeaveLogError(Ble, "Average Tx time/pkt : %u", result->TxTimeMs / result->TxPktCount);
        WeaveLogError(Ble, "Max Tx time         : %u", result->TxTimeMax);
        WeaveLogError(Ble, "Min Tx time         : %u", result->TxTimeMin);
        if (result->TxAckCount)
        {
            WeaveLogError(Ble, "Ack Packet Count    : %u", result->TxAckCount);
            WeaveLogError(Ble, "Average Tx+Ack time : %u", result->TxAckTimeMs / result->TxAckCount);
            WeaveLogError(Ble, "Max Tx+Ack time     : %u", result->TxAckTimeMax);
            WeaveLogError(Ble, "Min Tx+Ack time     : %u", result->TxAckTimeMin);
        }
    }
}

void WoBleTest::HandleCommandPacket(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err)
{
    BLEEndPoint *ep = static_cast<BLEEndPoint *>(appState);
    BleTransportCommandMessage cmd;
    bool needAck = false;
    PacketBuffer *data = NULL;
    Weave::System::Error timerErr;

    if (ep == NULL)
    {
        WeaveLogError(Ble, "%s: ep->%p", __FUNCTION__, ep);
        err = BLE_ERROR_NO_ENDPOINTS;
        SuccessOrExit(err);
    }

    data = ep->mWoBleTest.mCommandReceiveQueue;
    ep->mWoBleTest.mCommandReceiveQueue = ep->mWoBleTest.mCommandReceiveQueue->DetachTail();

    if (data == NULL)
    {
        WeaveLogError(Ble, "%s: ep->%p, data->%p", __FUNCTION__, ep, data);
        err = BLE_ERROR_NO_ENDPOINTS;
        SuccessOrExit(err);
    }

    err = BleTransportCommandMessage::Decode(*data, cmd);
    SuccessOrExit(err);

    WeaveLogDebugBleEndPoint(Ble, "%s: packet seq# %u, type %u, len %u", __FUNCTION__, 
            ep->mWoBle.RxPacketSeq(), cmd.CmdHdr.PacketType, data->DataLength());

    switch (cmd.CmdHdr.PacketType) {
        case kBleCommandType_TestAck:
            ep->mWoBleTest.mCommandTestResult.AckCount++;
            if (ep->mWoBleTest.mCommandUnderTest == WOBLE_TEST_TX)
            {
                ep->mWoBleTest.DoTxTiming(data, WOBLE_TX_DATA_ACK);
                // Sender sends the next data packet
                timerErr = systemLayer->StartTimer(0, DoTestDataSend, (void *)ep);
                VerifyOrExit(timerErr == BLE_NO_ERROR, err = BLE_ERROR_START_TIMER_FAILED);
            }
            break;

        case kBleCommandType_TestData:
            switch (cmd.Payload.MsgTestData.Type)
            {
                case kDataType_START:
                    // reset counters if sender restarted
                    if (ep->mWoBleTest.mCommandUnderTest == WOBLE_TEST_RX)
                    {
                        memset(&ep->mWoBleTest.mCommandTestResult, 0, sizeof(BTCommandTypeTestResult));
                        ep->mWoBleTest.mCommandTestResult.PayloadSize = cmd.Payload.MsgTestData.Length;
                    }
                    // falls thru
                case kDataType_CONTINUE:
                    if (ep->mWoBleTest.mCommandUnderTest == WOBLE_TEST_NONE)
                    {
                        memset(&ep->mWoBleTest.mCommandTestResult, 0, sizeof(BTCommandTypeTestResult));
                        ep->mWoBleTest.mCommandTestResult.PayloadSize = cmd.Payload.MsgTestData.Length;
                        ep->mWoBleTest.mCommandUnderTest = WOBLE_TEST_RX;
                    }
                    ep->mWoBleTest.mCommandTestResult.PacketCount++;
                    break;
                case kDataType_END:
                    ep->mWoBleTest.mCommandTestResult.PacketCount++;
                    // falls thru
                case kDataType_ABORT:
                default:
                    WeaveLogDebugBleEndPoint(Ble, "%s: Test Ended with test data type %d",
                            __FUNCTION__, cmd.Payload.MsgTestData.Type);
                    err = ep->mWoBleTest.DoCommandTestResult(kBleCommandTestResult_Reply,
                                            ep->mWoBleTest.mCommandTestResult.TestResult);
                    ep->mWoBleTest.mCommandUnderTest = WOBLE_TEST_NONE;
                    break;
            }
            needAck = cmd.Payload.MsgTestData.NeedAck;
            break;

        case kBleCommandType_TestRequest:
            memcpy(&ep->mWoBleTest.mCommandTestRequest, &cmd.Payload.MsgTestRequest, sizeof(BTCommandTypeTestRequest));
            WeaveLogError(Ble, "%s: PacketCount %lu, Duration %lu, TxGap %d, NeedAck %d, PayLoadSize %u",
                    __FUNCTION__, ep->mWoBleTest.mCommandTestRequest.PacketCount, ep->mWoBleTest.mCommandTestRequest.Duration,
                    ep->mWoBleTest.mCommandTestRequest.TxGap, ep->mWoBleTest.mCommandTestRequest.NeedAck,
                    ep->mWoBleTest.mCommandTestRequest.PayloadSize);
            // Start the test
            err = ep->mWoBleTest.HandleCommandTest(ep);
            SuccessOrExit(err);
            break;

        case kBleCommandType_TestResult:
            WeaveLogError(Ble, "\nIncoming TestResultOp : %d", cmd.Payload.MsgTestResult.TestResultOp);
            if (cmd.Payload.MsgTestResult.TestResultOp == kBleCommandTestResult_Request)
            {
                err = ep->mWoBleTest.DoCommandTestResult(kBleCommandTestResult_Reply,
                                        ep->mWoBleTest.mCommandTestResult.TestResult);
                LogBleTestResult(&ep->mWoBleTest.mCommandTestResult);
                SuccessOrExit(err);
            }
            else
                LogBleTestResult(&cmd.Payload.MsgTestResult);
            break;

        case kBleCommandType_WobleMTU:
            WeaveLogError(Ble, "\nIncoming WobleMTU : Op %u, Tx/Rx fragment size %u/%u", cmd.Payload.MsgWobleMTU.Op,
                    cmd.Payload.MsgWobleMTU.TxFragmentSize, cmd.Payload.MsgWobleMTU.RxFragmentSize);
            if (cmd.Payload.MsgWobleMTU.Op == kCmdType_Set)
            {
                if (cmd.Payload.MsgWobleMTU.TxFragmentSize > 0)
                    ep->mWoBle.SetTxFragmentSize(cmd.Payload.MsgWobleMTU.TxFragmentSize);
                if (cmd.Payload.MsgWobleMTU.RxFragmentSize > 0)
                    ep->mWoBle.SetRxFragmentSize(cmd.Payload.MsgWobleMTU.RxFragmentSize);
            }
            break;

        case kBleCommandType_WobleWindowSize:
            WeaveLogError(Ble, "\nIncoming WobleWindowSize : Op %u, Tx/Rx Window Sizes %u/%u",
                    cmd.Payload.MsgWobleWindowSize.Op,
                    cmd.Payload.MsgWobleWindowSize.TxWindowSize, cmd.Payload.MsgWobleWindowSize.RxWindowSize);
            if (cmd.Payload.MsgWobleWindowSize.Op == kCmdType_Set)
            {
                if (cmd.Payload.MsgWobleWindowSize.TxWindowSize > 0)
                    ep->SetTxWindowSize(cmd.Payload.MsgWobleWindowSize.TxWindowSize);
                if (cmd.Payload.MsgWobleWindowSize.RxWindowSize > 0)
                    ep->SetRxWindowSize(cmd.Payload.MsgWobleWindowSize.RxWindowSize);
            }
            break;

        case kBleCommandType_TxTiming:
            err = ep->mWoBleTest.HandleCommandTxTiming(ep, cmd.Payload.MsgTxTiming.Enable);
            SuccessOrExit(err);
            break;

        default:
            WeaveLogError(Ble, "%s: Control type %d is not yet supported", __FUNCTION__, cmd.CmdHdr.PacketType);
            break;
    }

    if (needAck)
        err = ep->mWoBleTest.DoCommandSendAck(ep->mWoBle.RxPacketSeq(), err);
exit:
    if (err != BLE_NO_ERROR)
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);

    if (data != NULL)
    {
        PacketBuffer::Free(data);
    }

    return;
}

void WoBleTest::StopTestTimer()
{
    if (GetFlag(mEp->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung) == true)
    {
        // Cancel any existing test timer.
        mEp->mBle->mSystemLayer->CancelTimer(HandleTestClose, mEp);
        SetFlag(mEp->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung, false);
    }
}

void WoBleTest::HandleTestClose(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err)
{
    BLEEndPoint *ep = static_cast<BLEEndPoint *>(appState);

    // Check for event-based timer race condition.
    if (GetFlag(ep->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung) == true)
    {
        WeaveLogError(Ble, "Test closed, ble ep %p", ep);
        SetFlag(ep->mTimerStateFlags, BLEEndPoint::kTimerState_UnderTestTimerRunnung, false);
    }

    ep->mWoBleTest.mCommandUnderTest = WOBLE_TEST_NONE;
    ep->mWoBleTest.mCommandTxTiming = false;
}

// Initialize the Tx histogram table, if reset is true, the old content is discarded
int WoBleTest::InitTxHistogram(char *file, int count, bool reset)
{
    int err = 0;

    VerifyOrExit(count > 0, err = -1);

    // Check if it's called already
    if (mTxHistogram.File != NULL)
        fclose(mTxHistogram.File);

    mTxHistogram.File = fopen(file, reset? "w" : "a");
    VerifyOrExit(mTxHistogram.File, err = -2);

    mTxHistogram.Record = (WoBleTxRecord *)malloc(count * sizeof(WoBleTxRecord));
    VerifyOrExit(mTxHistogram.Record, err = -3);

    mTxHistogram.Total = count;
    mTxHistogram.Idx = 0;

exit:
    if (err)
    {
        WeaveLogError(Ble, "%s: err %d\n", __FUNCTION__, err);
        if (mTxHistogram.File)
        {
            fclose(mTxHistogram.File);
            mTxHistogram.File = NULL;
        }
    }

    return err;
}

// This function saves up to N records
void WoBleTest::SaveTxRecords(uint8_t N)
{
    if (N == 0 || mTxHistogram.Total == 0 || mTxHistogram.File == NULL)
    {
        WeaveLogDebugBleEndPoint(Ble, "%s: Nothing to write", __FUNCTION__);
        return;
    }

    if (N > mTxHistogram.Total)
        N = mTxHistogram.Total;

    WoBleTxRecord *record = &mTxHistogram.Record[0];

    while (N-- && record->TxStart)
    {
        // The records in histogram file: "TxStartTime  PayloadSize  TxTime"
        fprintf(mTxHistogram.File, "%u\t%u\t%u\n",
                record->TxStart, record->Payload, record->TxTime);
        record++;
    }

    // restart the recording
    mTxHistogram.Idx = 0;
}

void WoBleTest::AddTxRecord(uint32_t txStart, uint16_t txTime, uint16_t size)
{
    if (mTxHistogram.Total == 0)
    {
        WeaveLogDebugBleEndPoint(Ble, "%s: Tx Histogram was not enabled", __FUNCTION__);
        return;
    }

    WoBleTxRecord *record = &mTxHistogram.Record[mTxHistogram.Idx++];
    
    record->TxStart = txStart;
    record->TxTime = txTime;
    record->Payload = size;

    if (mTxHistogram.Idx == 0 || mTxHistogram.Idx >= mTxHistogram.Total)
        SaveTxRecords(mTxHistogram.Total);
}

void WoBleTest::DoneTxHistogram(bool final)
{
    // Save the last records
    if (mTxHistogram.Idx > 0)
        SaveTxRecords(mTxHistogram.Idx);

    if (final)
    {
        if (mTxHistogram.File)
            fclose(mTxHistogram.File);
        if (mTxHistogram.Record)
            free((void *)mTxHistogram.Record);
        memset(&mTxHistogram, 0, sizeof(mTxHistogram));
    }
    else
        mTxHistogram.Idx = 0;
}

#endif /* WEAVE_ENABLE_WOBLE_TEST */

} /* namespace Ble */
} /* namespace nl */

#endif /* CONFIG_NETWORK_LAYER_BLE */


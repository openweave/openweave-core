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
 *      This file contains the defined constants and data structures
 *      for WoBle Control Path and Troughput Test
 *
 */

#ifndef WOBLE_TEST_H_
#define WOBLE_TEST_H_

#include <stdio.h>
#include <pthread.h>
#include <SystemLayer/SystemLayer.h>
#include <SystemLayer/SystemTimer.h>

namespace nl {
namespace Ble {

using ::nl::Weave::System::PacketBuffer;

// Forward declarations
class BleLayer;
class BLEEndPoint;

// Definitions of the BLE test command formats and constants
#define COMMAND_VERSION_LEN 1
#define COMMAND_DESTINATIONS_LEN 1
#define COMMAND_TYPE_LEN 1
#define COMMAND_PAYLOAD_LEN 2
#define COMMAND_HEADER_LEN (    COMMAND_PAYLOAD_LEN + \
                                COMMAND_VERSION_LEN + \
                                COMMAND_DESTINATIONS_LEN + \
                                COMMAND_TYPE_LEN )

#define COMMAND_ACK_ACK_LEN 1
#define COMMAND_ACK_SEQUENCE_LEN 1
#define COMMAND_ACK_RESULT_LEN 2
#define COMMAND_ACK_HDR_LEN (   COMMAND_ACK_ACK_LEN + \
                                COMMAND_ACK_SEQUENCE_LEN + \
                                COMMAND_ACK_RESULT_LEN )

#define COMMAND_DATA_TYPE_LEN 1
#define COMMAND_DATA_ACK_LEN 1
#define COMMAND_DATA_LENGTH_LEN 2
#define COMMAND_DATA_SEQUENCE_LEN 4
#define COMMAND_DATA_HDR_LEN (  COMMAND_DATA_TYPE_LEN + \
                                COMMAND_DATA_ACK_LEN + \
                                COMMAND_DATA_LENGTH_LEN + \
                                COMMAND_DATA_SEQUENCE_LEN )

// The total header length(overhead) of a test data packet
#define COMMAND_TESTDATA_HDR_LEN    (COMMAND_HEADER_LEN + COMMAND_DATA_HDR_LEN)

#define COMMAND_TESTREQ_COUNT_LEN 4
#define COMMAND_TESTREQ_DURATION_LEN 4
#define COMMAND_TESTREQ_TXGAP_LEN 2
#define COMMAND_TESTREQ_ACK_LEN 1
#define COMMAND_TESTREQ_SIZE_LEN 2
#define COMMAND_TESTREQ_HDR_LEN (   COMMAND_TESTREQ_COUNT_LEN + \
                                    COMMAND_TESTREQ_DURATION_LEN + \
                                    COMMAND_TESTREQ_TXGAP_LEN + \
                                    COMMAND_TESTREQ_ACK_LEN + \
                                    COMMAND_TESTREQ_SIZE_LEN )

#define COMMAND_TESTRESULT_RESULTOP_LEN 2
#define COMMAND_TESTRESULT_RESULT_LEN 4
#define COMMAND_TESTRESULT_COUNT_LEN 4
#define COMMAND_TESTRESULT_DURATION_LEN 4
#define COMMAND_TESTRESULT_ACKCOUNT_LEN 4
#define COMMAND_TESTRESULT_TXDROPS_LEN 4
#define COMMAND_TESTRESULT_TXGAP_LEN 2
#define COMMAND_TESTRESULT_SIZE_LEN 2
#define COMMAND_TESTRESULT_PKTCOUNT_LEN 2
#define COMMAND_TESTRESULT_TXTIME_LEN 4
#define COMMAND_TESTRESULT_TXTIMEMAX_LEN 2
#define COMMAND_TESTRESULT_TXTIMEMIN_LEN 2
#define COMMAND_TESTRESULT_TXACKCOUNT_LEN 2
#define COMMAND_TESTRESULT_TXACKTIME_LEN 4
#define COMMAND_TESTRESULT_TXACKTIMEMAX_LEN 2
#define COMMAND_TESTRESULT_TXACKTIMEMIN_LEN 2
#define COMMAND_TESTRESULT_TXTIMELAST_LEN 2
#define COMMAND_TESTRESULT_PAYLOADLAST_LEN 2
#define COMMAND_TESTRESULT_PAYLOADBYTES_LEN 4
#define COMMAND_TESTRESULT_HDR_LEN (    COMMAND_TESTRESULT_RESULTOP_LEN + \
                                        COMMAND_TESTRESULT_RESULT_LEN + \
                                        COMMAND_TESTRESULT_COUNT_LEN + \
                                        COMMAND_TESTRESULT_DURATION_LEN + \
                                        COMMAND_TESTRESULT_ACKCOUNT_LEN + \
                                        COMMAND_TESTRESULT_TXGAP_LEN + \
                                        COMMAND_TESTRESULT_TXDROPS_LEN + \
                                        COMMAND_TESTRESULT_SIZE_LEN + \
                                        COMMAND_TESTRESULT_PKTCOUNT_LEN + \
                                        COMMAND_TESTRESULT_TXTIME_LEN + \
                                        COMMAND_TESTRESULT_TXTIMEMAX_LEN + \
                                        COMMAND_TESTRESULT_TXTIMEMIN_LEN + \
                                        COMMAND_TESTRESULT_TXACKCOUNT_LEN + \
                                        COMMAND_TESTRESULT_TXACKTIME_LEN + \
                                        COMMAND_TESTRESULT_TXACKTIMEMAX_LEN + \
                                        COMMAND_TESTRESULT_TXACKTIMEMIN_LEN + \
                                        COMMAND_TESTRESULT_TXTIMELAST_LEN + \
                                        COMMAND_TESTRESULT_PAYLOADLAST_LEN + \
                                        COMMAND_TESTRESULT_PAYLOADBYTES_LEN )

#define COMMAND_WOBLEMTU_OP_LEN 1
#define COMMAND_WOBLEMTU_TXFRAGSIZE_LEN 2
#define COMMAND_WOBLEMTU_RXFRAGSIZE_LEN 2
#define COMMAND_WOBLEMTU_HDR_LEN (      COMMAND_WOBLEMTU_OP_LEN + \
                                        COMMAND_WOBLEMTU_TXFRAGSIZE_LEN + \
                                        COMMAND_WOBLEMTU_RXFRAGSIZE_LEN)

#define COMMAND_WINDOWSIZE_OP_LEN 1
#define COMMAND_WINDOWSIZE_TXWINSIZE_LEN 1
#define COMMAND_WINDOWSIZE_RXWINSIZE_LEN 1
#define COMMAND_WINDOWSIZE_HDR_LEN (    COMMAND_WINDOWSIZE_OP_LEN + \
                                        COMMAND_WINDOWSIZE_TXWINSIZE_LEN + \
                                        COMMAND_WINDOWSIZE_RXWINSIZE_LEN)

#define COMMAND_TXTIMING_ENABLE_LEN 1
#define COMMAND_TXTIMING_HDR_LEN (      COMMAND_TXTIMING_ENABLE_LEN )

enum
{
    kAckType_OK             = 0,
    kAckType_NOK            = 1
};

class BTCommandTypeAck
{
public:
    uint8_t Type;                     // Ack type : 0 = OK, 1 = NOK
    SequenceNumber_t SequenceNumber;  // Acknowledged Packet Sequence Number
    int32_t ResultCode;               // Command Result Code
};

enum
{
    kDataType_CONTINUE      = 0,
    kDataType_START         = 1,
    kDataType_END           = 2,
    kDataType_ABORT         = 3
};

class BTCommandTypeTestData
{
public:
    uint8_t Type;                     // kDataType_xxx
    uint8_t NeedAck;                  // 1 : Ack is required
    uint16_t Length;                  // Packet data length
    uint32_t Sequence;                // Tx sequence number
    uint8_t Data[0];                  // Start of Data
};

class BTCommandTypeTestRequest
{
public:
    uint32_t PacketCount;             // Total Packet Count
    uint32_t Duration;                // Test Duration
    uint16_t TxGap;                   // Gap in ms between packets (min = 1 ms)
    uint8_t NeedAck;                  // Ack is required for each packet
    uint16_t PayloadSize;             // Payload size of each packet (0~2048 bytes)
};

enum
{
    kBleCommandTestResult_Reply         = 0,
    kBleCommandTestResult_Request       = 1,
};

class BTCommandTypeTestResult
{
public:
    uint16_t TestResultOp;          // Test Result Reuest or Reply
    uint32_t TestResult;            // Error or test result code
    uint32_t PacketCount;           // 0 means a request for the last test result
    uint32_t Duration;              // Test Duration
    uint32_t AckCount;              // Received Ack count during the test
    uint32_t TxDrops;               // Dropped Tx packets
    uint16_t TxGap;                 // Gap in ms between packets
    uint16_t PayloadSize;           // Payload Size of each packet (0~2048 bytes)
    uint16_t TxPktCount;            // Total sent WoBle packets
    uint32_t TxTimeMs;              // Total tx duration for the sent test WoBle packets
    uint16_t TxTimeMax;             // Longest Tx duration
    uint16_t TxTimeMin;             // Smallest Tx duration
    uint16_t TxAckCount;            // Total received Ack packets
    uint32_t TxAckTimeMs;           // Total Tx+Ack duration for the sent test WoBle packets
    uint16_t TxAckTimeMax;          // Longest Tx+Ack duration
    uint16_t TxAckTimeMin;          // Smallest Tx+Ack duration
    uint16_t TxTimeLastMs;          // Last Tx duration
    uint16_t PayloadLast;           // Last Payload bytes
    uint32_t PayloadBytes;          // Total Payload bytes
};

enum
{
    kCmdType_Reply  = 0,
    kCmdType_Get    = 1,
    kCmdType_Set    = 2
};

class BTCommandTypeWobleMTU
{
public:
    uint8_t Op;                       // 0 = Reply, 1 = Get, 2 = Set
    uint16_t TxFragmentSize;          // Tx fragment size in bytes
    uint16_t RxFragmentSize;          // Rx fragment size in bytes
};

class BTCommandTypeWindowSize
{
public:
    uint8_t Op;                       // 0 = Reply, 1 = Get, 2 = Set
    uint8_t TxWindowSize;             // unit is packet, 0 means no change
    uint8_t RxWindowSize;             // unit is packet, 0 means no change
};

class BTCommandTypeTxTiming
{
public:
    bool Enable;                      // 0 = disable
};

#define BLE_TEST_DATA_MAX_LEN       1024

#define kBleCommandDest_None    0
#define kBleCommandDest_Local   0x1
#define kBleCommandDest_Remote  0x2

#define BLE_COMMAND_OP_BASE     80
enum
{
    kBleCommandType_TestAck         =   (BLE_COMMAND_OP_BASE + 0),
    kBleCommandType_TestData        =   (BLE_COMMAND_OP_BASE + 1),
    kBleCommandType_TestRequest     =   (BLE_COMMAND_OP_BASE + 2),
    kBleCommandType_TestResult      =   (BLE_COMMAND_OP_BASE + 3),
    kBleCommandType_WobleMTU        =   (BLE_COMMAND_OP_BASE + 4),
    kBleCommandType_WobleWindowSize =   (BLE_COMMAND_OP_BASE + 5),
    kBleCommandType_TxTiming        =   (BLE_COMMAND_OP_BASE + 6)
};

class BTCommandHeader
{
public:
    uint16_t PacketLength;            // Control Packet Length
    uint8_t Version;                  // Control Protocol Version
    uint8_t PacketType;               // Control Packet Type
};

class BleTransportCommandMessage
{
public:
    BTCommandHeader CmdHdr;

    union {
        BTCommandTypeAck            MsgTestAck;
        BTCommandTypeTestData       MsgTestData;
        BTCommandTypeTestRequest    MsgTestRequest;
        BTCommandTypeTestResult     MsgTestResult;
        BTCommandTypeWobleMTU       MsgWobleMTU;
        BTCommandTypeWindowSize     MsgWobleWindowSize;
        BTCommandTypeTxTiming       MsgTxTiming;
    } Payload;

    uint32_t    CommandTest_Duration;
    uint32_t    CommandTest_PacketCount;
    bool        CommandTest_Start;

    uint8_t Data[BLE_TEST_DATA_MAX_LEN];    // Control Packet PayLoad

    // Must be able to reserve 20 byte data length in msgBuf.
    BLE_ERROR Encode(PacketBuffer *msgBuf, BleTransportCommandMessage& msg) const;

    static BLE_ERROR Decode(const PacketBuffer &msgBuf, BleTransportCommandMessage& msg);
};

enum
{
    WOBLE_TX_START      = 0,
    WOBLE_TX_DONE       = 1,
    WOBLE_TX_DATA_ACK   = 2
};

enum
{
    WOBLE_TEST_NONE = 0,
    WOBLE_TEST_TX   = 1,
    WOBLE_TEST_RX   = 2
};

class WoBleTxRecord
{
public:
    uint32_t    TxStart;    // Tx start time : Epoch(ms)
    uint16_t    TxTime;     // Tx time (ms)
    uint16_t    Payload;    // payload in bytes
};

class WoBleTxHistogram
{
public:
    uint8_t     Idx;            // indice to the last TxTime record
    uint8_t     Total;          // total number of records
    WoBleTxRecord   *Record;    // the TxTime records
    FILE *      File;           // the storage
};

/*
 * Declaration of WoBleTest class which is a friend of BleLayer and BLEEndPoint
 * This object contains all the related data and functions for conducting a WoBle throughput test.
 * It also keeps the last test request and result.
 */
class NL_DLL_EXPORT WoBleTest
{
    friend class BleLayer;
    friend class BLEEndPoint;

public:
    // Public data members:
    BTCommandTypeTestRequest    mCommandTestRequest;
    BTCommandTypeTestResult     mCommandTestResult;

    // Public functions:
    void Decode(uint8_t *src, uint8_t *dst, size_t size);
    void DoTxTiming(PacketBuffer *data, int stage);
    BLE_ERROR DoCommandTestRequest(BLEEndPoint *obj);
    BLE_ERROR DoCommandTestResult(int32_t op, int32_t result);
    BLE_ERROR DoCommandTestAbort(int32_t result);
    BLE_ERROR DoCommandTxTiming(bool enable);
    static BLE_ERROR HandleCommandTest(BLEEndPoint *obj);
    static BLE_ERROR HandleCommandTxTiming(BLEEndPoint *obj, bool enabled);
    static void LogBleTestResult(BTCommandTypeTestResult *result);

    // TxTime histogram functions
    int InitTxHistogram(char *file, int count, bool reset); // Initialize Tx histogram table
    void AddTxRecord(uint32_t ts, uint16_t tm, uint16_t size);
    void SaveTxRecords(uint8_t N);      // Save the first N records into file
    void DoneTxHistogram(bool final);   // Clean up the records (& storage)

private:
    // Private data members:
    pthread_t   mMainThread;            // Main thread
    pthread_t   mTestTxThread;          // Tx test thread
    BLEEndPoint *mEp;                   // The associated endpoint
    BleTransportCommandMessage  mCommand;   // buffer for encode/decode
    PacketBuffer *mCommandReceiveQueue; // for received control packets
    PacketBuffer *mCommandSendQueue;    // for sending command data packets
    PacketBuffer *mCommandAckToSend;    // for sending command ack packet
    int mCommandUnderTest;              // UnderTest mode
    bool mCommandTxTiming;              // Flag indicates whether Tx timing is enabled

    WoBleTxHistogram mTxHistogram;      // TxTiming Histogram

    // time statistics
    struct {
        uint32_t mTxStartMs;    // WoBle Tx start time in ms
        uint32_t mTxAckStartMs; // WoBle Tx start time for Test Data Ack
        uint16_t mTxPayload;    // WoBle payload in bytes
    } mTimeStats;               // Up to two outstanding Tx WoBle packets

    // Private functions:
    BLE_ERROR Init(BLEEndPoint *ep);

    static void HandleCommandReceived(BLEEndPoint *obj, PacketBuffer *data);
    static void HandleCommandPacket(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err);
    static void DoTestDataSend(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err);
    static void * StartTxThread(void * ep);
    BLE_ERROR DoCommandSendAck(SequenceNumber_t seqNum, int32_t result);
    BLE_ERROR DoCommandRequestTestResult();
    void StopTestTimer(void);
    static void HandleTestClose(Weave::System::Layer *systemLayer, void *appState, Weave::System::Error err);
};

/*
 * Here are the handlers for WoBle test commands
 *      ble-test
 *      ble-test-result
 *      ble-test-abort
 */
BLE_ERROR HandleCommandTest(void *ble, BLE_CONNECTION_OBJECT connObj, uint32_t packetCount, uint32_t duration, uint16_t txGap, uint8_t needAck, uint16_t payloadSize, bool reverse);
BLE_ERROR HandleCommandTestResult(void *ble, BLE_CONNECTION_OBJECT connObj, bool local);
BLE_ERROR HandleCommandTestAbort(void *ble, BLE_CONNECTION_OBJECT connObj);
BLE_ERROR HandleCommandTxTiming(void *ble, BLE_CONNECTION_OBJECT connObj, bool enabled, bool remote);

// Macro to get the gap between two uint8 sequence numbers: N is the newer
#define SeqNumGap(N, O) ((N >= O)? N - O : N + 256 - O)

// The name of WoBle Tx Histogram log file
#ifndef WOBLE_TX_HISTOGRAM_FILE
#ifdef ANDROID
#define WOBLE_TX_HISTOGRAM_FILE    "/data/misc/nldaemon/woble_tx_histogram.log"
#else
#define WOBLE_TX_HISTOGRAM_FILE    "/tmp/woble_tx_histogram.log"
#endif
#endif
#define WOBLE_TX_RECORD_COUNT   10

} /* namespace Ble */
} /* namespace nl */

#endif /* WOBLE_TEST_H_ */

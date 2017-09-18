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
 *      This file implements an automated Test suite for testing
 *      functionalities of WRMP.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <InetLayer/InetLayer.h>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include "TestWRMP.h"

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

#define TOOL_NAME "TestWRMP"

#ifndef WEAVE_CONFIG_SECURITY_TEST_MODE
#define WEAVE_CONFIG_SECURITY_TEST_MODE    1
#endif

#define TEST_INITIAL_RETRANS_TIMEOUT       (5000)
#define TEST_ACTIVE_RETRANS_TIMEOUT        (2000)
#define VerifyOrFail(TST, MSG) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fputs(MSG, stderr); \
        exit(EXIT_FAILURE); \
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
        exit(EXIT_FAILURE); \
    } \
} while (0)

extern uint32_t appContext;

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static void HandleEchoResponseReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static void HandleAckRcvd(ExchangeContext *ec, void *ctxt);
static void HandleDDRcvd(ExchangeContext *ec, uint32_t pauseTime);
static void HandleThrottleRcvd(ExchangeContext *ec, uint32_t pauseTime);
static void ThrottleTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static WEAVE_ERROR SendCustomMessage(ExchangeContext *ec, uint32_t ProfileId, uint8_t msgType, uint16_t sendFlags, PacketBuffer *payload, uint32_t *lAppContext = &appContext);
static void ParseDestAddress();

using nl::StatusReportStr;
using namespace nl::Weave::Profiles::Security;
nl::Weave::WeaveExchangeManager *globalExchMgr = 0;
uint32_t appContext = 0xcafebabe;
uint32_t appContext2 = 0xbaddcafe;
uint32_t ThrottlePeriodicMsgCount = 0;
uint32_t PeriodicMsgCount = 0;
uint32_t DDTestCount = 0;
uint32_t ThrottlePauseTime = 2000;
uint64_t NodeId = 0xdeadbeefcafebabe;
uint64_t FirstDDTestTime = 0;
uint64_t SecondDDTestTime = 0;
bool isAckRcvd = false;
uint8_t ackCount = 0;
bool throttleRcvd = false;
bool DDRcvd = false;
bool FlowThrottled = false;
bool ThrottleTimeoutFired = false;
bool Listening = false;
int32_t MaxEchoCount = 1;
int32_t RetransInterval = 0;
int32_t MaxAckReceiptInterval = 3000000;
int32_t EchoInterval = 1000000;
int32_t EchoLength = -1;
bool UseTCP = true;
bool UsePASE = false;
bool UseCASE = false;
bool UseGroupKeyEnc = false;
bool Debug = false;
uint64_t DestNodeId;
const char *DestAddr = NULL;
uint32_t  TestNum  = 0;
IPAddress DestIPAddr; // only used for UDP
uint16_t DestPort; // only used for UDP
InterfaceId DestIntf = INET_NULL_INTERFACEID; // only used for UDP
uint64_t LastEchoTime = 0;
bool WaitingForEchoResp = false;
uint64_t EchoCount = 0;
uint64_t EchoRespCount = 0;
uint64_t CloseECMsgCount = 0;
uint8_t  EncryptionType = kWeaveEncryptionType_None;
uint16_t KeyId = WeaveKeyId::kNone;
WRMPTestClient WRMPClient;
WRMPTestServer WRMPServer;
bool AllowDuplicateMsgs = false;

enum
{
    kToolOpt_Listen                         = 1000,
    kToolOpt_Count,
    kToolOpt_AllowDups,
};

static OptionDef gToolOptionDefs[] =
{
    { "listen",     kNoArgument,        kToolOpt_Listen      },
    { "dest-addr",  kArgumentRequired,  'D'                  },
    { "count",      kArgumentRequired,  kToolOpt_Count       },
    { "allow-dups", kNoArgument,        kToolOpt_AllowDups   },
    { "test",       kArgumentRequired,  'T'                  },
    { "wait",       kArgumentRequired,  'W'                  },
    { "retrans",    kArgumentRequired,  'R'                  },
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    { "group-enc",  kNoArgument,        'G'                  },
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>][%<interface>]\n"
    "       Send Echo Requests to a specific address rather than one\n"
    "       derived from the destination node id. <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address. If <port> is specified, Echo\n"
    "       requests will be sent to the specified port. If <interface> is\n"
    "       specified, Echo Requests will be sent over the specified local\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -T, --test <num>\n"
    "       Execute the corresponding test with the specified number. \n"
    "       TestWRMPTimeoutSolitaryAckReceipt---------------------[1] \n"
    "       TestWRMPTimeoutSolitaryAckReceiptNoInitator-----------[2] \n"
    "       TestWRMPFlushedSolitaryAckReceipt --------------------[3] \n"
    "       TestWRMPPiggybackedAckReceipt-------------------------[4] \n"
    "       TestWRMPRetransmitMessage-----------------------------[5] \n"
    "       TestWRMPTwoStageRetransmitTimeout---------------------[6] \n"
    "       TestWRMPSendThrottleFlowMessage-----------------------[7] \n"
    "       TestWRMPSendDelayedDeliveryMessage--------------------[8] \n"
    "       TestWRMPThrottleFlowBehavior--------------------------[9] \n"
    "       TestWRMPDelayedDeliveryBehavior-----------------------[10] \n"
    "       TestWRMPSendVer2AfterVer1-----------------------------[11] \n"
    "       TestWRMPDuplicateMsgAcking----------------------------[12]\n"
    "       TestWRMPDuplicateMsgLostAck---------------------------[13]\n"
    "       TestWRMPDuplicateMsgAckOnClosedExResponder------------[14]\n"
    "       TestWRMPDuplicateMsgAckOnClosedExInitiator------------[15]\n"
    "       TestWRMPDuplicateMsgDetection-------------------------[16]\n"
    "\n"
    "  -W, --wait <TestWaitTime>\n"
    "\n"
    "  -R, --retrans <MaxRetransInterval>\n"
    "\n"
    "  --count <num>\n"
    "       Send the specified number of Echo Requests and exit.\n"
    "\n"
    "  --allow-dups\n"
    "       Allow reception of duplicate messages.\n"
    "\n"
    "  --listen\n"
    "       Listen and respond to Echo Requests sent from another node.\n"
    "\n"
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    "  -G, --group-enc\n"
    "       Use a group key to encrypt messages.\n"
    "       When group key encryption option is chosen the key id should be also specified.\n"
    "       Below are two examples how group key id can be specified:\n"
    "           --group-enc-key-id 0x00005536\n"
    "           --group-enc-key-type r --group-enc-root-key c --group-enc-epoch-key-num 2 --group-enc-app-key-num 54\n"
    "       Note that both examples describe the same rotating group key derived from client\n"
    "       root key, epoch key number 4 and app group master key number 54 (0x36).\n"
    "\n"
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    ;

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>[:<dest-port>][%%<interface>]]\n"
    "       " TOOL_NAME " [<options...>] --listen\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gGroupKeyEncOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

void PrepareNewBuf(PacketBuffer **buf)
{
    *buf = PacketBuffer::New();
    if (*buf == NULL)
    {
        printf("Unable to allocate PacketBuffer\n");
        LastEchoTime = Now();
        return;
    }

    char *p = (char *) (*buf)->Start();
    int32_t len = sprintf(p, "WRMP Echo Message %" PRIu64 "\n", EchoCount);

    if (EchoLength > (*buf)->MaxDataLength())
        EchoLength = (*buf)->MaxDataLength();

    if (EchoLength != -1)
    {
        if (len > EchoLength)
            len = EchoLength;
        else
            while (len < EchoLength)
            {
                int32_t copyLen = EchoLength - len;
                if (copyLen > len)
                    copyLen = len;
                memcpy(p + len, p, copyLen);
                len += copyLen;
            }
    }

    (*buf)->SetDataLength((uint16_t) len);

}

static bool IsRetransOutsideWindow(uint64_t transmitTime, uint32_t retransTimeout)
{
    int32_t AckReceiptBufferTime = 600 * System::kTimerFactor_micro_per_milli; // 600 msec

    return (Now() < (transmitTime + retransTimeout * System::kTimerFactor_micro_per_milli - AckReceiptBufferTime) ||
            Now() > (transmitTime + retransTimeout * System::kTimerFactor_micro_per_milli + AckReceiptBufferTime));
}

//Send Echo Request
//Wait for Ack piggybacked on Echo Response
testStatus_t TestWRMPPiggybackedAckReceipt(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    err = WRMPClient.SendEchoRequest(payloadBuf);
    if (err == WEAVE_NO_ERROR)
    {
        WaitingForEchoResp = true;
        EchoCount++;
    }
    else
    {
        printf("WRMPTestClient.SendEchoRequest() failed: %s\n", ErrorStr(err));
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (isAckRcvd && !WaitingForEchoResp)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (isAckRcvd && !WaitingForEchoResp)
            {
                return TEST_PASS;
            }
            else
            {
                printf("No response received\n");
                WaitingForEchoResp = false;
                Done = true;
                return TEST_FAIL;
            }
        }
    }
    return TEST_FAIL;
}

//Send message that does not solicit reply
//Allow recipient to Ack timeout and send ack back
testStatus_t TestWRMPTimeoutSolitaryAckReceipt(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint16_t sendFlags = ExchangeContext::kSendFlag_RequestAck;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response, sendFlags, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (isAckRcvd)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (isAckRcvd)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }

        }
    }
    return TEST_FAIL;
}

//Send message without the initiator flag.
//The responder will drop it because it won't find a matching EC, but
//it should still send back an ACK.
//For example, this scenario happens when a responder sends a response after
//the EC has timedout on the initiator.
testStatus_t TestWRMPTimeoutSolitaryAckReceiptNoInitiator(void)
{
    testStatus_t testStatus = TEST_PASS;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint16_t sendFlags = ExchangeContext::kSendFlag_RequestAck;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout to a well known value
    RetransInterval = 10000;
    WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
    WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;

    WRMPClient.ExchangeCtx->SetInitiator(false);
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response, sendFlags, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        testStatus = TEST_FAIL;
        goto exit;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < LastEchoTime + (RetransInterval - 1000) * 1000)
            {
                // We want an ACK on the first transmission
                if (isAckRcvd)
                {
                    testStatus = TEST_PASS;
                    break;
                }
                else
                {
                    continue;
                }
            }

            Done = true;
            testStatus = TEST_FAIL;
        }
    }

exit:
    WRMPClient.ExchangeCtx->SetInitiator(true);
    return testStatus;
}

//Send 2 back-to-back messages that require no response from recipient application.
//Receiving Exchange layer would replace first pending Ack with the second
//one and flush the first by sending a solitary Ack back.
testStatus_t TestWRMPFlushedSolitaryAckReceipt(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint16_t sendFlags = ExchangeContext::kSendFlag_RequestAck;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response, sendFlags, payloadBuf);
    if (err == WEAVE_NO_ERROR)
    {
        err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response, sendFlags, payloadBuf);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
            Done = true;
            return TEST_FAIL;
        }
    }
    else
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (isAckRcvd)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (isAckRcvd)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }
        }
    }
    return TEST_FAIL;
}

//Formulate message and then drop it at Message layer
//Then timeout and retransmit and wait for ack
testStatus_t TestWRMPRetransmitMessage(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = true;

    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Generate_Response,
                                       ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }
    WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = false;

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (isAckRcvd)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (isAckRcvd)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }

        }
    }
    return TEST_FAIL;
}

//Force retransmissions while sending a message twice on the same exchange and
//verify that the times of receipt of Acks conform to the 2 stage retransmit
//timeouts.
testStatus_t TestWRMPTwoStageRetransmitTimeout(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int32_t MaxTestInterval = (TEST_INITIAL_RETRANS_TIMEOUT + TEST_ACTIVE_RETRANS_TIMEOUT) *
                               System::kTimerFactor_micro_per_milli +
                               System::kTimerFactor_micro_per_unit; // extra 1 second
    uint64_t FirstTransmitTime = 0;
    uint64_t SecondTransmitTime = 0;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    isAckRcvd = false;
    int twoStageAckCount = 0;
    Done = false;

    //Set the initial and active retrans timeout
    WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = TEST_INITIAL_RETRANS_TIMEOUT;
    WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = TEST_ACTIVE_RETRANS_TIMEOUT;

    WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = true;

    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Generate_Response,
                                       ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    FirstTransmitTime = Now();

    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }

    WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = false;

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening) {
            if (Now() < FirstTransmitTime + MaxTestInterval)
            {
                if (isAckRcvd)
                {
                    twoStageAckCount++;

                    // Time check for first Ack
                    if (twoStageAckCount == 1 && IsRetransOutsideWindow(FirstTransmitTime, TEST_INITIAL_RETRANS_TIMEOUT))
                    {
                        return TEST_FAIL;
                    }

                    if (twoStageAckCount == 1)
                    {
                        // Reset isAckRcvd for second Ack reception
                        isAckRcvd = false;

                        // Reset the data length of the bufer
                        PrepareNewBuf(&payloadBuf);
                        payloadBuf->SetDataLength(0);

                        // Drop message to force retransmit
                        WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = true;

                        // Send the second message which should have the updated
                        // active retransmit timeout.
                        err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test,
                                                kWeaveTestMessageType_Generate_Response,
                                                ExchangeContext::kSendFlag_RequestAck, payloadBuf);

                        // Update last transmit time
                        SecondTransmitTime = Now();

                        WRMPClient.ExchangeMgr->MessageLayer->mDropMessage = false;

                        if (err != WEAVE_NO_ERROR)
                        {
                            printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
                            return TEST_FAIL;
                        }

                        continue;
                    }

                    // Time check for second Ack
                    if (twoStageAckCount == 2 && IsRetransOutsideWindow(SecondTransmitTime, TEST_ACTIVE_RETRANS_TIMEOUT))
                    {
                        return TEST_FAIL;
                    }

                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            Done = true;
        }
    }

    return TEST_FAIL;
}

testStatus_t TestWRMPSendThrottleFlowMessage(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    //Request a Throttle message
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Request_Throttle,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (throttleRcvd)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (throttleRcvd)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }
        }
    }
    return TEST_FAIL;
}

//Send periodic messages to peer prompting a throttle message from Peer
//Start a timer for the throttle time and check on expiry that no messages
//were transmitted during that time.
testStatus_t TestWRMPThrottleFlowBehavior(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    isAckRcvd = false;
    Done = false;
    LastEchoTime = Now();
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Request_Periodic,
                                       ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (ThrottleTimeoutFired)
            {
                //Allow the chance of a second periodic message being sent
                if (PeriodicMsgCount <= 2)
                {
                    return TEST_PASS;
                }
                else
                {
                    return TEST_FAIL;
                }
            }
        }
    }
    return TEST_FAIL;
}

//Send a Request for a Delayed Delivery and check on receipt
testStatus_t TestWRMPSendDelayedDeliveryMessage(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    DDRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    //Kick off the Throttle test by sending a periodic response probe
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Request_DD,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (DDRcvd)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (DDRcvd)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }
        }
    }
    return TEST_FAIL;
}

testStatus_t TestWRMPDelayedDeliveryBehavior(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);
    //Prepare Echo Request
    DDRcvd = false;
    Done = false;
    LastEchoTime = Now();

    //Kick off the Throttle test by sending a periodic response probe
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_DD_Test,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    if (err != WEAVE_NO_ERROR)
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        Done = true;
        return TEST_FAIL;
    }
    //Set Drop Ack
    WRMPClient.ExchangeCtx->SetDropAck(true);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (SecondDDTestTime)
            {
                //Allow the chance of a second periodic message being sent
                printf("Delay is %" PRIx64 "\n", (SecondDDTestTime - FirstDDTestTime)/1000);
                if ((SecondDDTestTime - FirstDDTestTime) >= (ThrottlePauseTime * 1000))
                {
                    return TEST_PASS;
                }
                else
                {
                    return TEST_FAIL;
                }
            }
            else if (Now() > LastEchoTime + MaxAckReceiptInterval + RetransInterval + ThrottlePauseTime * 1000)
            {
                return TEST_FAIL;
            }
        }
    }
    return TEST_FAIL;
}

testStatus_t TestWRMPSendVer2AfterVer1(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    PrepareNewBuf(&payloadBuf);

    //Kick off the Throttle test by sending a periodic response probe
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response, 0,
                            payloadBuf);
    //err = WRMPClient.SendEchoRequest(payloadBuf);
    if (err == WEAVE_NO_ERROR)
    {
        PrepareNewBuf(&payloadBuf);
        err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response,
                                ExchangeContext::kSendFlag_RequestAck, payloadBuf);
        if (err != WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE)
        {
            printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
            return TEST_FAIL;
        }
        else
        {
            printf("Received expected error %d while trying to send a version 2 message on a version 1 Exchange\n", err);
            return TEST_PASS;
        }
    }
    else
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        return TEST_FAIL;
    }
}

testStatus_t TestWRMPDuplicateMsgAcking(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint8_t *p = NULL;
    uint16_t len = 0;
    Done = false;
    isAckRcvd = false;
    ackCount = 0;
    LastEchoTime = Now();


    PrepareNewBuf(&payloadBuf);

    //Form the message
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Msg Detection");
    memcpy(p + len, p, len);
    payloadBuf->SetDataLength(len);

    //Set the retrans timeout
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    //Send the first message and then immediately send a duplicate message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response,
                            ExchangeContext::kSendFlag_RequestAck | ExchangeContext::kSendFlag_RetainBuffer,
                            payloadBuf);
    if (err == WEAVE_NO_ERROR)
    {
        payloadBuf->SetStart(p);
        payloadBuf->SetDataLength(len);
        err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_No_Response,
                                ExchangeContext::kSendFlag_RequestAck | ExchangeContext::kSendFlag_ReuseMessageId,
                                payloadBuf);

        if (err != WEAVE_NO_ERROR)
        {
            printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
            return TEST_FAIL;
        }
    }
    else
    {
        printf("WRMPTestClient.SendCustomMessage failed: %s\n", ErrorStr(err));
        return TEST_FAIL;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if (isAckRcvd && ackCount == 2)
                {
                    return TEST_PASS;
                }
                else
                {
                    continue;
                }
            }

            if (isAckRcvd && ackCount == 2)
            {
                return TEST_PASS;
            }
            else
            {
                Done = true;
                return TEST_FAIL;
            }
        }
    }

    return TEST_FAIL;
}

// Test lost ack scenario.
//
// Steps for Initiator (I) and Responder (R) of the exchange:
//  - I sends SetDropAck msg.
//  - R receives SetDropAck msg: it sets DropAck flag so the next received msg is not acked.
//  - I sends LostAck msg.
//  - R receives CLostAck msg: it clears DropAck flag so the next received msg is acked.
//  - I retransmits CloseEC msg because it didn't receive ack.
//  - R receives retransmission of the CloseEC msg: the ack is sent.
//  - I receives ack for the CloseEC msg.
testStatus_t TestWRMPDuplicateMsgLostAck(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint8_t *p = NULL;
    uint16_t len = 0;

    Done = false;
    ackCount = 0;
    LastEchoTime = Now();

    // Set the retrans timeout.
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    // Form SetDropAck messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection SetDropAck Msg");
    payloadBuf->SetDataLength(len);

    // Send this message with command to drop ack for the next received message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_SetDropAck,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send SetDropAck message\n");

    // Form LostAck messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection LostAck Msg");
    payloadBuf->SetDataLength(len);

    // Send second message.
    // The receiver of this message should drop ack and then ack retransmitted message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_Lost_Ack,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf, &appContext2);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send LostAck message\n");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (ackCount == 2)
            {
                return TEST_PASS;
            }

            if (Now() >= LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                return TEST_FAIL;
            }
        }
    }

    return TEST_FAIL;
}

// Responder receives duplicate message on a closed exchange and it should ack the message.
//
// Steps for Initiator (I) and Responder (R) of the exchange:
//  - I sends SetDropAck msg.
//  - R receives SetDropAck msg: it sets DropAck flag so the next received msg is not acked.
//  - I sends CloseEC msg.
//  - R receives CloseEC msg: it clears DropAck flag so the next received msg is acked
//    and it closes the exchange.
//  - I retransmits CloseEC msg because it didn't receive ack.
//  - R receives retransmission of the CloseEC msg: it is detected as a duplicate for which the
//    new exchange is created to send the ack and that exchange is immediately closed.
//  - I receives ack for the CloseEC msg.
testStatus_t TestWRMPDuplicateMsgAckOnClosedExResponder(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint8_t *p = NULL;
    uint16_t len = 0;

    Done = false;
    ackCount = 0;
    LastEchoTime = Now();

    // Set the retrans timeout.
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    // Form SetDropAck messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection SetDropAck Msg");
    payloadBuf->SetDataLength(len);

    // Send this message with command to drop ack for the next received message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_SetDropAck,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send SetDropAck message\n");

    // Form CloseEC messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection CloseEC Msg");
    payloadBuf->SetDataLength(len);

    // Send second message.
    // The receiver of this message should drop ack and close exchange context.
    // The receiver then should receive retransmission of this message, for which a new EC
    // will be created and closed only to send ack.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_CloseEC,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf, &appContext2);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send LostAck message\n");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (ackCount == 2)
            {
                return TEST_PASS;
            }

            VerifyOrFail(Now() < LastEchoTime + MaxAckReceiptInterval + RetransInterval,
                         "TestWRMPDuplicateMsgAckOnClosedExResponder FAILED\n");
        }
    }

    return TEST_FAIL;
}

// Initiator receives duplicate message on a closed exchange and it should ack the message.
//
// Steps for Initiator (I) and Responder (R) of the exchange:
//  - I sends RequestCloseEC msg. It also sets DropAck flag so the next received msg
//    from R is not acked.
//  - R receives RequestCloseEC msg: it sends CloseEC msg.
//  - I receives CloseEC msg: it clears DropAck flag so the next received msg is acked and it
//    closes the exchange.
//  - R retransmits CloseEC msg because it didn't receive ack.
//  - I receives retransmission of the CloseEC msg: it is detected as a duplicate for which the
//    new exchange is created to send the ack and that exchange is immediately closed.
//  - R receives ack for the CloseEC msg.
testStatus_t TestWRMPDuplicateMsgAckOnClosedExInitiator(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint8_t *p = NULL;
    uint16_t len = 0;

    Done = false;
    ackCount = 0;
    LastEchoTime = Now();

    // Set the retrans timeout.
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    // Set DropAck flag.
    WRMPClient.ExchangeCtx->SetDropAck(true);

    // Form RequestCloseEC message.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection RequestCloseEC Msg");
    payloadBuf->SetDataLength(len);

    // Send RequestCloseEC message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_RequestCloseEC,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send RequestCloseEC message\n");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (Now() > LastEchoTime + MaxAckReceiptInterval + RetransInterval)
            {
                if ((ackCount == 1) && (CloseECMsgCount == 1))
                {
                    return TEST_PASS;
                }
                else
                {
                    printf("TestWRMPDuplicateMsgAckOnClosedExInitiator FAILED\n");
                    return TEST_FAIL;
                }
            }
        }
    }

    return TEST_FAIL;
}

// Test duplicate message detection mechanism.
testStatus_t TestWRMPDuplicateMsgDetection(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payloadBuf = NULL;
    uint8_t *p = NULL;
    uint16_t len = 0;
    uint16_t msgType;
    uint64_t expectedEchoRespCount;

    Done = false;
    EchoRespCount = 0;
    LastEchoTime = Now();

    // Set the retrans timeout.
    if (RetransInterval)
    {
        WRMPClient.ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = RetransInterval;
        WRMPClient.ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = RetransInterval;
    }

    // Form AllowDup/DontAllowDup message.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection Set Allow Dup Flag Msg");
    payloadBuf->SetDataLength(len);

    // Set test message type.
    msgType = AllowDuplicateMsgs ? kWeaveTestMessageType_AllowDup : kWeaveTestMessageType_DontAllowDup;

    // Sent this message to enable/disable duplicate message on the related peer exchange.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, msgType,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send AllowDup/DontAllowDup message\n");

    // Form SetDropAck messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection SetDropAck Msg");
    payloadBuf->SetDataLength(len);

    // Send this message with command to drop ack for the next received message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_SetDropAck,
                            ExchangeContext::kSendFlag_RequestAck, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send SetDropAck message\n");

    for (uint8_t i = 0; i < MaxEchoCount; i++)
    {
        // Form EchoRequest message.
        PrepareNewBuf(&payloadBuf);
        p = payloadBuf->Start();
        len = sprintf((char *)p, "Dup Detection Send Echo Request Msg");
        payloadBuf->SetDataLength(len - MaxEchoCount + i);

        if (i % 2 == 0)
        {
            // Send this message with command to drop ack for the next received message.
            err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_EchoRequestForDup,
                                    ExchangeContext::kSendFlag_RequestAck, payloadBuf);
            SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send EchoRequestForDup message\n");
        }
        else
        {
            // Send an Echo Request message.
            err = WRMPClient.SendEchoRequest(payloadBuf, 0);
            SuccessOrFail(err, "WRMPTestClient.SendEchoRequest failed to send EchoRequest message\n");
        }
    }

    // Form ClearDropAck messages.
    PrepareNewBuf(&payloadBuf);
    p = payloadBuf->Start();
    len = sprintf((char *)p, "Dup Detection ClearDropAck Msg");
    payloadBuf->SetDataLength(len);

    // Send this message with command to drop ack for the next received message.
    err = SendCustomMessage(WRMPClient.ExchangeCtx, kWeaveProfile_Test, kWeaveTestMessageType_ClearDropAck, 0, payloadBuf);
    SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send ClearDropAck message\n");

    if (UseGroupKeyEnc || MaxEchoCount < 16)
    {
        if (AllowDuplicateMsgs)
            expectedEchoRespCount = MaxEchoCount;
        else
            expectedEchoRespCount = MaxEchoCount / 2;
    }
    // Unencrypted messages that fall before the reorder window (last 16 ids) is treated as
    // new messages that cause the window to reset. Therefore, such message is not detected
    // as duplicate and echo response is not sent for that message in out test scenario.
    else
    {
        expectedEchoRespCount = MaxEchoCount / 2;
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (!Listening)
        {
            if (Now() > LastEchoTime + MaxAckReceiptInterval + 5 * RetransInterval)
            {
                printf("\nEchoRespCount = %" PRIu64 "; Expected EchoRespCount = %" PRIu64 "\n\n", EchoRespCount, expectedEchoRespCount);
                if (EchoRespCount == expectedEchoRespCount)
                {
                    return TEST_PASS;
                }
                else
                {
                    return TEST_FAIL;
                }
            }
        }
    }

    return TEST_FAIL;
}

struct Tests {
    testStatus_t (*mTest)(void);
    const char * mTestName;
};

Tests gTests[] =
{
    { .mTest = TestWRMPTimeoutSolitaryAckReceipt, .mTestName = "TestWRMPTimeoutSolitaryAckReceipt" },
    { .mTest = TestWRMPTimeoutSolitaryAckReceiptNoInitiator, .mTestName = "TestWRMPTimeoutSolitaryAckReceiptNoInitiator," },
    { .mTest = TestWRMPFlushedSolitaryAckReceipt, .mTestName = "TestWRMPFlushedSolitaryAckReceipt" },
    { .mTest = TestWRMPPiggybackedAckReceipt, .mTestName = "TestWRMPPiggybackedAckReceipt" },
    { .mTest = TestWRMPRetransmitMessage, .mTestName = "TestWRMPRetransmitMessage" },
    { .mTest = TestWRMPTwoStageRetransmitTimeout, .mTestName = "TestWRMPTwoStageRetransmitTimeout" },
    { .mTest = TestWRMPSendThrottleFlowMessage, .mTestName = "TestWRMPSendThrottleFlowMessage" },
    { .mTest = TestWRMPSendDelayedDeliveryMessage, .mTestName = "TestWRMPSendDelayedDeliveryMessage" },
    { .mTest = TestWRMPThrottleFlowBehavior, .mTestName = "TestWRMPThrottleFlowBehavior" },
    { .mTest = TestWRMPDelayedDeliveryBehavior, .mTestName = "TestWRMPDelayedDeliveryBehavior" },
    { .mTest = TestWRMPSendVer2AfterVer1, .mTestName = "TestWRMPSendVer2AfterVer1" },
    { .mTest = TestWRMPDuplicateMsgAcking, .mTestName = "TestWRMPDuplicateMsgAcking" },
    { .mTest = TestWRMPDuplicateMsgLostAck, .mTestName = "TestWRMPDuplicateMsgLostAck" },
    { .mTest = TestWRMPDuplicateMsgAckOnClosedExResponder, .mTestName = "TestWRMPDuplicateMsgAckOnClosedExResponder" },
    { .mTest = TestWRMPDuplicateMsgAckOnClosedExInitiator, .mTestName = "TestWRMPDuplicateMsgAckOnClosedExInitiator" },
    { .mTest = TestWRMPDuplicateMsgDetection, .mTestName = "TestWRMPDuplicateMsgDetection" }
};

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

int main(int argc, char *argv[])
{
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    WEAVE_ERROR err  = WEAVE_NO_ERROR;
    testStatus_t res = TEST_FAIL;
    //+++++++++++++Initialization+++++++++++++++//

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    if (UseGroupKeyEnc)
    {
        EncryptionType = kWeaveEncryptionType_AES128CTRSHA1;
        KeyId = gGroupKeyEncOptions.GetEncKeyId();
        if (KeyId == WeaveKeyId::kNone)
        {
            PrintArgError("%s: Please specify a group encryption key id using the --group-enc-... options.\n", TOOL_NAME);
            exit(EXIT_FAILURE);
        }
    }

    UseStdoutLineBuffering();
    SetSIGUSR1Handler();

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, true);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnReceiveError = HandleMessageReceiveError;

    if (!Listening)
    {
        globalExchMgr = &ExchangeMgr;
        if (DestAddr != NULL)
            ParseDestAddress();
        // Initialize the EchoClient application.
        err = WRMPClient.Init(&ExchangeMgr, DestNodeId, DestIPAddr, DestPort, DestIntf);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WRMPTestClient.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Arrange to get a callback whenever an Echo Response is received.
        WRMPClient.OnEchoResponseReceived = HandleEchoResponseReceived;
    }
    else
    {
        globalExchMgr = &ExchangeMgr;
        // Initialize the EchoServer application.
        err = WRMPServer.Init(&ExchangeMgr);
        if (err)
        {
            printf("WRMPTestServer.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Arrange to get a callback whenever an Echo Request is received.
        WRMPServer.OnEchoRequestReceived = HandleEchoRequestReceived;

        //SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
        //SecurityMgr.OnSessionError = HandleSecureSessionError;
    }

    PrintNodeConfig();

    if (!Listening)
    {
        if (DestNodeId == 0)
            printf("Sending WRMP Messages to node at %s\n", DestAddr);
        else if (DestAddr == NULL)
            printf("Sending WRMP Messages to node %" PRIX64 "\n", DestNodeId);
        else
            printf("Sending WRMP Messages to node %" PRIX64 " at %s\n", DestNodeId, DestAddr);

        TestNum --;
        if (TestNum >= (sizeof(gTests) / sizeof(Tests)))
        {

            printf("Wrong WRMP Test Num %d\n", (TestNum+1));
            printf("Should be one of set of Tests below\n");
            size_t numTests = (sizeof(gTests) / sizeof(Tests));
            for (size_t testIndex = 0; testIndex < numTests; testIndex++)
            {
                printf("%-55s [%2zu]\n", gTests[testIndex].mTestName, testIndex+1);
            }
            exit(EXIT_FAILURE);
        }
        else
        {
            res = gTests[TestNum].mTest();
            printf("%s %s\n", gTests[TestNum].mTestName, res == TEST_PASS ? "Passed" : "Failed");
        }
    }
    else
    {
        printf("Listening for WRMP Messages...\n");
    }
    if (Listening)
    {
        while (!Done)
        {
            struct timeval sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_usec = 100000;

            ServiceNetwork(sleepTime);

        }
    }
    WRMPClient.Shutdown();
    WRMPServer.Shutdown();
    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();
    if (res == TEST_PASS)
        return 0;
    else
        return -1;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
void HandleEchoRequestReceived(uint64_t nodeId,
                               IPAddress nodeAddr,
                               PacketBuffer *payload)
{
    if (Listening)
    {
        char ipAddrStr[64];
        nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        printf("WRMP Echo Request from node %" PRIX64 " (%s): len=%u ... sending response.\n",
                nodeId, ipAddrStr,
                payload->DataLength());

        if (Debug)
            DumpMemory(payload->Start(), payload->DataLength(), "    ", 16);
    }
}

void HandleEchoResponseReceived(uint64_t nodeId,
                                IPAddress nodeAddr,
                                PacketBuffer *payload)
{
    uint32_t respTime = Now();
    uint32_t transitTime = respTime - LastEchoTime;

    WaitingForEchoResp = false;
    EchoRespCount++;

    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("WRMP Echo Response from node %" PRIX64 " (%s): %" PRIu64 "/%" PRIu64 "(%.2f%%) len=%u time=%.3fms\n",
            nodeId, ipAddrStr,
            EchoRespCount, EchoCount, ((double) EchoRespCount) * 100 / EchoCount,
            payload->DataLength(),
            ((double) transitTime) / 1000);

    if (Debug)
        DumpMemory(payload->Start(), payload->DataLength(), "    ", 16);
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    case 'G':
        UseGroupKeyEnc = true;
        break;
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    case kToolOpt_AllowDups:
        AllowDuplicateMsgs = true;
        break;
    case kToolOpt_Listen:
        Listening = true;
        break;
    case kToolOpt_Count:
        if (!ParseInt(arg, MaxEchoCount) || MaxEchoCount < 0 || MaxEchoCount > 30)
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'D':
        DestAddr = arg;
        break;
    case 'T':
        if (!arg || !ParseInt(arg, TestNum))
        {
            PrintArgError("%s: Invalid value specified for Test number: %s\n", progName, arg);
            return false;
        }
        break;
    case 'W':
        if (!arg || !ParseInt(arg, MaxAckReceiptInterval))
        {
            PrintArgError("%s: Invalid value specified for MaxAckReceiptInterval: %s\n", progName, arg);
            return false;
        }
        break;
    case 'R':
        if (!arg || !ParseInt(arg, RetransInterval))
        {
            PrintArgError("%s: Invalid value specified for RetransInterval: %s\n", progName, arg);
            return false;
        }
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc > 0)
    {
        if (argc > 1)
        {
            PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
            return false;
        }

        if (Listening)
        {
            PrintArgError("%s: Please specify either a node id or --listen\n", progName);
            return false;
        }

        const char *nodeId = argv[0];
        char *p = (char *)strchr(nodeId, '@');
        if (p != NULL)
        {
            *p = 0;
            DestAddr = p+1;
        }

        if (!ParseNodeId(nodeId, DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
            return false;
        }
    }

    else
    {
        if (!Listening)
        {
            PrintArgError("%s: Please specify either a node id or --listen\n", progName);
            return false;
        }
    }

    return true;
}

void ParseDestAddress()
{
    WEAVE_ERROR err;
    const char *addr;
    uint16_t addrLen;
    const char *intfName;
    uint16_t intfNameLen;

    err = ParseHostPortAndInterface(DestAddr, strlen(DestAddr), addr, addrLen, DestPort, intfName, intfNameLen);
    if (err != INET_NO_ERROR)
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (!IPAddress::FromString(addr, DestIPAddr))
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (intfName != NULL)
    {
        err = InterfaceNameToId(intfName, DestIntf);
        if (err != INET_NO_ERROR)
        {
            printf("Invalid interface name: %s\n", intfName);
            exit(EXIT_FAILURE);
        }
    }
}

//++++++++WRMPTestClient Class+++++++++++++++++++++//
WRMPTestClient::WRMPTestClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnEchoResponseReceived = NULL;
    ExchangeCtx = NULL;
}

WEAVE_ERROR WRMPTestClient::Init(WeaveExchangeManager *exchangeMgr, uint64_t nodeId, IPAddress nodeAddr, uint16_t port, InterfaceId sendIntfId)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    OnEchoResponseReceived = NULL;

    ExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, WEAVE_PORT, sendIntfId, this);
    if (ExchangeCtx == NULL)
    {
        return WEAVE_ERROR_NO_MEMORY;
    }

    //Set callbacks
    ExchangeCtx->OnAckRcvd      = HandleAckRcvd;
    ExchangeCtx->OnDDRcvd       = HandleDDRcvd;
    ExchangeCtx->OnThrottleRcvd = HandleThrottleRcvd;

    return WEAVE_NO_ERROR;
}

void ThrottleTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    //Check the global ThrottlePeriodicMsgCount
    printf("Throttle Timeout: Periodic message count is %d\n", PeriodicMsgCount);
    FlowThrottled = false;
    ThrottleTimeoutFired = true;
}

WEAVE_ERROR WRMPTestClient::Shutdown()
{
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    ExchangeMgr = NULL;
    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WRMPTestClient::SendEchoRequest(PacketBuffer *payload)
{
    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleResponse;

    // Send an Echo Request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payload, ExchangeContext::kSendFlag_RequestAck, &appContext);
    if (err != WEAVE_NO_ERROR)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    return err;
}

WEAVE_ERROR WRMPTestClient::SendEchoRequest(PacketBuffer *payload, uint16_t sendFlags)
{
    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleResponse;

    // Send an Echo Request message. Discard the exchange context if the send fails.
    WEAVE_ERROR err = ExchangeCtx->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payload, sendFlags, &appContext);
    if (err != WEAVE_NO_ERROR)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    return err;
}

WEAVE_ERROR SendCustomMessage(ExchangeContext *ec, uint32_t ProfileId, uint8_t msgType, uint16_t sendFlags, PacketBuffer *payload, uint32_t *lAppContext)
{
    // Configure the encryption and signature types to be used to send the request.
    ec->EncryptionType = EncryptionType;
    ec->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    if (!Listening)
    {
        ec->OnMessageReceived = WRMPTestClient::HandleResponse;
    }
    else
    {
        ec->OnMessageReceived = WRMPTestServer::HandleRcvdMessage;
    }

    return ec->SendMessage(ProfileId, msgType, payload, sendFlags, lAppContext);
}

void WRMPTestClient::HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WRMPTestClient *wrmpClientApp = (WRMPTestClient *)ec->AppState;
    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != wrmpClientApp->ExchangeCtx)
    {
        return;
    }

    if (profileId == kWeaveProfile_Echo && msgType == kEchoMessageType_EchoResponse)
    {
        // Call the registered OnEchoResponseReceived handler, if any.
        if (wrmpClientApp->OnEchoResponseReceived != NULL)
            wrmpClientApp->OnEchoResponseReceived(msgInfo->SourceNodeId, pktInfo->SrcAddress, payload);
    }
    if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Periodic)
    {
        PeriodicMsgCount++;
        printf("Received Request for Throttle; Sending Throttle Msg with PauseTime %d\n", ThrottlePauseTime);
        if (PeriodicMsgCount == 1)
        {
            ec->WRMPSendThrottleFlow(ThrottlePauseTime);
            //Start the timer
            globalExchMgr->MessageLayer->SystemLayer->StartTimer(ThrottlePauseTime, ThrottleTimeout, NULL);
        }
    }
    if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_DD_Test)
    {
        if (!DDTestCount)
        {
            FirstDDTestTime = Now();
            DDTestCount++;
            //Reset DropAck
            ec->SetDropAck(false);
            // Allow duplicates for this exchange so we can process second DD_Test message.
            ec->AllowDuplicateMsgs = true;
            //Send Delayed Delivery
            ec->WRMPSendDelayedDelivery(ThrottlePauseTime, globalExchMgr->FabricState->LocalNodeId);
        }
        else
        {
            SecondDDTestTime = Now();
        }
        //Note the time and wait for second one and compare with ThrottlePauseTime

    }
    if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_CloseEC)
    {
        if (!(msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage))
        {
            printf("TestWRMP: Received Test Msg Type CloseEC; Closing exchange and clearing DropAck flag\n");
            ec->SetDropAck(false);
            ec->Release();
            wrmpClientApp->ExchangeCtx = NULL;
            CloseECMsgCount++;
        }
        else
        {
            printf("TestWRMP: Received Duplicate of a Test Msg Type CloseEC; Sending Ack\n");
            CloseECMsgCount++;
        }
    }

    // Free the payload buffer.
    PacketBuffer::Free(payload);
}

//++++++++WRMPTestServer Class+++++++++++++++++++++//
WRMPTestServer::WRMPTestServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnEchoRequestReceived = NULL;
}

WEAVE_ERROR WRMPTestServer::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    OnEchoRequestReceived = NULL;

    // Register to receive unsolicited messages from the exchange manager.
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_Generate_Response, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_No_Response, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_Request_Throttle, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_Request_Periodic, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_Request_DD, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_DD_Test, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_SetDropAck, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_RequestCloseEC, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_AllowDup, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Test, kWeaveTestMessageType_DontAllowDup, HandleRcvdMessage,
                                                   AllowDuplicateMsgs, this);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WRMPTestServer::Shutdown()
{
    if (ExchangeMgr != NULL)
    {
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Echo, kEchoMessageType_EchoRequest);
        ExchangeMgr = NULL;
    }

    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WRMPTestServer::GeneratePeriodicMessage(int MaxCount, ExchangeContext *ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    struct timeval sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 100000;
    PacketBuffer *payloadBuf = NULL;
    uint32_t msgCount = 0;

    printf("Send max of %d Periodic Messages\n", MaxCount);
    for (int i = 0; i < MaxCount; i++)
    {
        if (!FlowThrottled)
        {
            PrepareNewBuf(&payloadBuf);
            err = SendCustomMessage(ec,
                                    kWeaveProfile_Test,
                                    kWeaveTestMessageType_Periodic,
                                    ExchangeContext::kSendFlag_RequestAck,
                                    payloadBuf);

            payloadBuf = NULL;
            if (err != WEAVE_NO_ERROR)
            {
                return err;
            }
            else
            {
                msgCount++;
                printf("Sent Periodic Message #%d\n", msgCount);
            }
            ServiceNetwork(sleepTime);
        }
    }

    return err;
}

void WRMPTestServer::HandleRcvdMessage(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WRMPTestServer *wrmpServApp = (WRMPTestServer *) ec->AppState;

    //Set the application callbacks first
    ec->OnAckRcvd      = HandleAckRcvd;
    ec->OnDDRcvd       = HandleDDRcvd;
    ec->OnThrottleRcvd = HandleThrottleRcvd;
    // WeaveExchangeManager installs a default handler in OnMessageReceived.
    // This test overrides that default and uses this method (i.e.
    // HandleRcvdMessage) as the handler not only for the first unsolicited
    // message that causes the allocation of the exchange context but also for
    // the subsequent messages received on that exchange context.
    ec->OnMessageReceived = HandleRcvdMessage;

    if (profileId == kWeaveProfile_Echo && msgType == kEchoMessageType_EchoRequest)
    {
        // Call the registered OnEchoRequestReceived handler, if any.
        if (wrmpServApp->OnEchoRequestReceived != NULL)
            wrmpServApp->OnEchoRequestReceived(ec->PeerNodeId, ec->PeerAddr, payload);

        // Send an Echo Response back to the sender.
        ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload, 0);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Generate_Response)
    {
        printf("Received Test Msg Type Generate_Response; Send Solitary Ack\n");
        ec->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common,
                        nl::Weave::Profiles::Common::kMsgType_Null,
                        payload);

    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_No_Response)
    {
        printf("Received Test Msg Type No_Response\n");

    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Request_Throttle)
    {
        printf("Received Request for Throttle; Sending Throttle Msg with PauseTime %d\n", ThrottlePauseTime);
        ec->WRMPSendThrottleFlow(ThrottlePauseTime);

    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Request_DD)
    {
        //Send the DD message.
        printf("Received Request for Delayed Delivery; Sending Delayed Delivery Msg with PauseTime %d and NodeId 0x%" PRIx64 "\n",
                ThrottlePauseTime, globalExchMgr->FabricState->LocalNodeId);
        ec->WRMPSendDelayedDelivery(ThrottlePauseTime, globalExchMgr->FabricState->LocalNodeId);

    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Request_Periodic)
    {
        printf("Received Request for Periodic Messages; Generate a set of periodic messages\n");
        wrmpServApp->GeneratePeriodicMessage(10, ec);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_DD_Test)
    {
        printf("Received Test Msg Type DD_Test; Send back DD_Test\n");
        WEAVE_ERROR err = SendCustomMessage(ec, kWeaveProfile_Test, kWeaveTestMessageType_DD_Test, ExchangeContext::kSendFlag_RequestAck, payload);
        SuccessOrFail(err, "WRMPTestClient.SendCustomMessage failed to send DD_Test message\n");
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_SetDropAck)
    {
        printf("TestWRMP: Received Test Msg Type SetDropAck; Setting DropAck flag\n");
        ec->SetDropAck(true);
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_ClearDropAck)
    {
        printf("TestWRMP: Received Test Msg Type ClearDropAck; Clearing DropAck flag\n");
        ec->SetDropAck(false);
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_Lost_Ack)
    {
        if (!(msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage))
        {
            printf("TestWRMP: Received Test Msg Type Lost_Ack; Clearing DropAck flag not sending ack because it is not a duplicate\n");
            ec->SetDropAck(false);
        }
        else
        {
            printf("TestWRMP: Received Duplicate of a Test Msg Type Lost_Ack; Sending Ack\n");
        }
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_CloseEC)
    {
        if (!(msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage))
        {
            printf("TestWRMP: Received Test Msg Type CloseEC; Closing exchange and clearing DropAck flag\n");
            ec->SetDropAck(false);
            ec->Release();
        }
        else
        {
            printf("TestWRMP: Received Duplicate of a Test Msg Type CloseEC; Sending Ack\n");
        }
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_RequestCloseEC)
    {
        printf("TestWRMP: Received Test Msg Type RequestCloseEC; Sending CloseEC msg as requested\n");

        // Form CloseEC messages.
        uint16_t len = sprintf((char *)(payload->Start()), "Dup Detection CloseEC Msg");
        payload->SetDataLength(len);

        // Send CloseEC message.
        WEAVE_ERROR err = ec->SendMessage(kWeaveProfile_Test, kWeaveTestMessageType_CloseEC, payload,
                                          ExchangeContext::kSendFlag_RequestAck, &appContext2);
        SuccessOrFail(err, "ec->SendMessage failed to send CloseEC message\n");
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_AllowDup)
    {
        printf("TestWRMP: Received Test Msg Type AllowDup; Setting AllowDuplicateMsgs flag\n");
        ec->AllowDuplicateMsgs = true;
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_DontAllowDup)
    {
        printf("TestWRMP: Received Test Msg Type DontAllowDup; Clearing AllowDuplicateMsgs flag\n");
        ec->AllowDuplicateMsgs = false;
        PacketBuffer::Free(payload);
    }
    else if (profileId == kWeaveProfile_Test && msgType == kWeaveTestMessageType_EchoRequestForDup)
    {
        // If test echo request message is a duplicate send echo response.
        if (msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage)
        {
            printf("TestWRMP: Received Duplicate of a Test Msg Type EchoRequestForDup; Sending echo response\n");
            HandleEchoRequestReceived(ec->PeerNodeId, ec->PeerAddr, payload);

            // Send an Echo Response back to the sender.
            ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload, 0);
        }
        else
        {
            printf("TestWRMP: Received Test Msg Type EchoRequestForDup; Not sending response because the message is not a duplicate\n");
            PacketBuffer::Free(payload);
        }
    }
}

void HandleAckRcvd(ExchangeContext *ec, void *msgCtxt)
{
    uint32_t context;
    if (msgCtxt)
    {
        context = *((uint32_t *)(msgCtxt));
        if (context == appContext || context == appContext2)
        {
            printf("Received Ack for Context: %X\n", context);
            isAckRcvd = true;
            ackCount++;
        }
    }
    else
    {
        printf("No context for received Ack\n");
    }
}

void HandleDDRcvd(ExchangeContext *ec, uint32_t pauseTime)
{
    printf("Received Delayed Delivery Msg for node Id 0x%" PRIx64 " with pauseTime %d\n", ec->PeerNodeId, pauseTime);
    DDRcvd = true;
}

void HandleThrottleRcvd(ExchangeContext *ec, uint32_t pauseTime)
{
    printf("Received Throttle Msg with pauseTime %d from peer %" PRId64 "\n", pauseTime, ec->PeerNodeId);
    throttleRcvd = true;
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

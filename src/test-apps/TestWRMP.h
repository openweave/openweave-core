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
 *      This file defines an automated test suite for testing
 *      functionalities of the Nest Weave Reliable Messaging Protocol
 *      (WRMP).
 *
 */

#ifndef TEST_WRMP_H_
#define TEST_WRMP_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
using namespace ::nl::Inet;
using namespace ::nl::Weave;
enum
{
    kWeaveProfile_Test                            = 101
};

enum
{
    kWeaveTestMessageType_Generate_Response       = 1,
    kWeaveTestMessageType_No_Response             = 2,
    kWeaveTestMessageType_Periodic                = 3,
    kWeaveTestMessageType_Request_Periodic        = 4,
    kWeaveTestMessageType_Request_Throttle        = 5,
    kWeaveTestMessageType_Request_DD              = 6,
    kWeaveTestMessageType_DD_Test                 = 7,
    kWeaveTestMessageType_SetDropAck              = 8,
    kWeaveTestMessageType_ClearDropAck            = 9,
    kWeaveTestMessageType_Lost_Ack                = 10,
    kWeaveTestMessageType_RequestCloseEC          = 11,
    kWeaveTestMessageType_CloseEC                 = 12,
    kWeaveTestMessageType_AllowDup                = 13,
    kWeaveTestMessageType_DontAllowDup            = 14,
    kWeaveTestMessageType_EchoRequestForDup       = 15,
    kWeaveTestMessageType_Response                = 16,
};

typedef enum
{
    TEST_PASS                                     = 0,
    TEST_FAIL                                     = 1,
} testStatus_t;

class WRMPTestClient
{
public:
        WRMPTestClient();

        const WeaveFabricState *FabricState;    // [READ ONLY] Fabric state object
        WeaveExchangeManager *ExchangeMgr;              // [READ ONLY] Exchange manager object

        WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr, uint64_t nodeId, IPAddress nodeAddr, uint16_t port, InterfaceId sendIntfId);
        WEAVE_ERROR Shutdown();

        WEAVE_ERROR SendEchoRequest(WeaveConnection *con, PacketBuffer *payload);
        WEAVE_ERROR SendEchoRequest(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);

        WEAVE_ERROR SendEchoRequest(PacketBuffer *payload);
        WEAVE_ERROR SendEchoRequest(PacketBuffer *payload, uint16_t sendFlags);
        typedef void (*EchoFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
        EchoFunct OnEchoResponseReceived;
        //WEAVE_ERROR GeneratePeriodicMessage(void);

        ExchangeContext *ExchangeCtx;                   // The exchange context for the most recently started exchange.

        static void HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

};


class WRMPTestServer : public WeaveServerBase
{
public:
        WRMPTestServer();

        WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
        WEAVE_ERROR Shutdown();
        WEAVE_ERROR GeneratePeriodicMessage(int MaxCount, ExchangeContext *ec);

        typedef void (*EchoFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
        EchoFunct OnEchoRequestReceived;

        static void HandleRcvdMessage(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
        static void HandleRcvdMessageForAckDelete(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

        static void HandleThrottleFlow(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                       const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
};
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#endif // TEST_WRMP_H_

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
 *      This file tests the basic functionality of Weave/core/WeaveMessageLayer.cpp
 *      includes basic single message sending/receiving via tcp/udp.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"

#define TOOL_NAME "TestWeaveMessageLayer"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void DriveSending();
static PacketBuffer *MakeWeaveMessage(WeaveMessageInfo *msgInfo);
static void HandleMessageReceived(WeaveMessageLayer *msgLayer, WeaveMessageInfo *msgInfo, PacketBuffer *payload);
static void HandleMessageReceived(WeaveConnection *con, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf);
static void HandleReceiveError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err, const IPPacketInfo *pktInfo);
static void HandleReceiveError(WeaveConnection *con, WEAVE_ERROR err);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleOutboundConnectionClosed(WeaveConnection *con, WEAVE_ERROR err);
static void HandleInboundConnectionClosed(WeaveConnection *con, WEAVE_ERROR err);


bool SendMsgs = false;
uint64_t DestNodeId;
IPAddress DestAddr = IPAddress::Any;
uint64_t LastSendTime = 0;
int32_t MaxSendCount = -1;
int32_t SendInterval = 1000000;
int32_t SendLength = -1;
bool UseTCP = false;
bool UseSessionKey = false;

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",          kArgumentRequired,  'D' },
    { "count",              kArgumentRequired,  'c' },
    { "length",             kArgumentRequired,  'l' },
    { "interval",           kArgumentRequired,  'i' },
    { "tcp",                kNoArgument,        't' },
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    { "use-session-key",    kNoArgument,        'S' },
#endif
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <dest-node-ip-addr>\n"
    "       Send weave messages to a specific IPv4/IPv6 address rather than one\n"
    "       derived the destination node id.\n"
    "\n"
    "  -c, --count <num>\n"
    "       Send the specified number of weave messages and exit.\n"
    "\n"
    "  -l, --length <num>\n"
    "       Send weave messages with the specified number of bytes in the payload.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send weave messages at the specified interval in milliseconds.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send weave messages. Defaults to using UDP.\n"
    "\n"
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    "  -S, --use-session-key\n"
    "       Use a session key when encrypting weave messages.\n"
    "\n"
#endif
    "";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options>] <dest-node-id>\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
    SetSIGUSR1Handler();

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(-1);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = gNetworkOptions.LocalIPv6Addr.InterfaceId();
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, false);

    if (DestAddr == IPAddress::Any)
        DestAddr = FabricState.SelectNodeAddress(DestNodeId);

    MessageLayer.OnMessageReceived = HandleMessageReceived;
    MessageLayer.OnReceiveError = HandleReceiveError;
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;

    PrintNodeConfig();

    if (!SendMsgs)
        printf("Waiting for incoming messages...\n");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (SendMsgs)
            DriveSending();
    }

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    case 'S':
        UseSessionKey = true;
        break;
#endif
    case 't':
        UseTCP = true;
        break;
    case 'c':
        if (!ParseInt(arg, MaxSendCount) || MaxSendCount < 0)
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'l':
        if (!ParseInt(arg, SendLength) || SendLength < 0 || SendLength > UINT16_MAX)
        {
            PrintArgError("%s: Invalid value specified for data length: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, SendInterval) || SendInterval < 0 || SendInterval > (INT32_MAX / 1000))
        {
            PrintArgError("%s: Invalid value specified for send interval: %s\n", progName, arg);
            return false;
        }
        SendInterval = SendInterval * 1000;
        break;
    case 'D':
        if (!ParseIPAddress(arg, DestAddr))
        {
            PrintArgError("%s: Invalid value specified for destination IP address: %s\n", progName, arg);
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

        if (!ParseNodeId(argv[0], DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, argv[0]);
            return false;
        }

        SendMsgs = true;
    }

    return true;
}

void DriveSending()
{
    static WeaveConnection* con         = NULL;
    static int              sendCount   = 0;

    WEAVE_ERROR             res;
    PacketBuffer*           msgBuf;
    WeaveMessageInfo        msgInfo;
    bool                    isTimeToSend;

    isTimeToSend = (Now() >= LastSendTime + SendInterval);
    if (!isTimeToSend)
        return;

    if (MaxSendCount != -1 && sendCount >= MaxSendCount)
    {
        if (con != NULL)
        {
            con->Close();

            char nodeAddrStr[64];
            con->PeerAddr.ToString(nodeAddrStr, sizeof(nodeAddrStr));

            printf("Connection to node %" PRIX64 " (%s) closed\n", con->PeerNodeId, nodeAddrStr);

            con = NULL;
        }
        Done = true;
        return;
    }

    if (UseTCP)
    {
        if (con != NULL && con->State == WeaveConnection::kState_Closed)
        {
            con->Close();
            con = NULL;
        }

        if (con == NULL)
        {
            con = MessageLayer.NewConnection();
            con->OnConnectionComplete = HandleConnectionComplete;
            con->OnConnectionClosed = HandleOutboundConnectionClosed;
            con->OnMessageReceived = HandleMessageReceived;
            con->OnReceiveError = HandleReceiveError;

            res = con->Connect(DestNodeId, DestAddr);
            if (res != WEAVE_NO_ERROR)
            {
                printf("WeaveConnection.Connect failed: %d\n", (int) res);
                LastSendTime = Now();
                return;
            }
        }

        if (con->State != WeaveConnection::kState_Connected)
            return;

        msgBuf = MakeWeaveMessage(&msgInfo);

        if (UseSessionKey)
        {
            msgInfo.EncryptionType = kWeaveEncryptionType_AES128CTRSHA1;
            msgInfo.KeyId = sTestDefaultTCPSessionKeyId;
        }

        sendCount++;
        LastSendTime = Now();

        res = con->SendMessage(&msgInfo, msgBuf);
        if (res != WEAVE_NO_ERROR)
        {
            printf("WeaveConnection.SendMessage failed: %d\n", (int) res);
            return;
        }
    }

    else
    {
        msgBuf = MakeWeaveMessage(&msgInfo);

        if (UseSessionKey)
        {
            msgInfo.EncryptionType = kWeaveEncryptionType_AES128CTRSHA1;
            msgInfo.KeyId = sTestDefaultUDPSessionKeyId;
        }

        sendCount++;
        LastSendTime = Now();

        res = MessageLayer.SendMessage(DestAddr, &msgInfo, msgBuf);
        if (res != WEAVE_NO_ERROR)
        {
            printf("WeaveMessageLayer.SendMessage failed: %d\n", (int) res);

            return;
        }
    }

    {
        char nodeAddrStr[64];
        DestAddr.ToString(nodeAddrStr, sizeof(nodeAddrStr));

        printf("Weave message sent to node %" PRIX64 " (%s)\n", DestNodeId, nodeAddrStr);
    }
}

PacketBuffer *MakeWeaveMessage(WeaveMessageInfo *msgInfo)
{
    PacketBuffer *msgBuf;

    static uint16_t lastMsgNum = 0;

    msgBuf = PacketBuffer::New();
    if (msgBuf == NULL)
        return NULL;

    char *p = (char *) msgBuf->Start();
    uint16_t len = (uint16_t) sprintf(p, "This is weave message %ld\n", (long) ++lastMsgNum);

    if (SendLength > msgBuf->MaxDataLength())
        SendLength = msgBuf->MaxDataLength();

    if (SendLength != -1)
    {
        if (len > SendLength)
            len = SendLength;
        else
            while (len < SendLength)
            {
                int32_t copyLen = SendLength - len;
                if (copyLen > len)
                    copyLen = len;
                memcpy(p + len, p, copyLen);
                len += copyLen;
            }
    }

    msgBuf->SetDataLength(len);

    msgInfo->Clear();
    msgInfo->MessageVersion = kWeaveMessageVersion_V1;
    msgInfo->Flags = 0;
    msgInfo->SourceNodeId = FabricState.LocalNodeId;
    msgInfo->DestNodeId = DestNodeId;
    msgInfo->EncryptionType = kWeaveEncryptionType_None;
    msgInfo->KeyId = WeaveKeyId::kNone;

    return msgBuf;
}

void HandleMessageReceived(WeaveMessageLayer *msgLayer, WeaveMessageInfo *msgInfo, PacketBuffer *payload)
{
    const char *encType;
    switch (msgInfo->EncryptionType)
    {
    case kWeaveEncryptionType_None:
        encType = "none";
        break;
    case kWeaveEncryptionType_AES128CTRSHA1:
        encType = "AES-128-CTR-SHA1";
        break;
    default:
        encType = "unknown";
        break;
    }

    char nodeAddrStr[64];
    msgInfo->InPacketInfo->SrcAddress.ToString(nodeAddrStr, sizeof(nodeAddrStr));

    char intfStr[64];
    if (msgInfo->InPacketInfo->Interface != INET_NULL_INTERFACEID)
        GetInterfaceName(msgInfo->InPacketInfo->Interface, intfStr, sizeof(intfStr));
    else
    {
        intfStr[0] = '-';
        intfStr[1] = 0;
    }

    printf("Weave message received from node %" PRIX64 " ([%s]:%u, %s)\n  Message Id: %u\n  Encryption Type: %s\n  Key id: %04X\n  Payload Length: %u\n  Payload: %.*s\n",
            msgInfo->SourceNodeId, nodeAddrStr, (unsigned)msgInfo->InPacketInfo->SrcPort, intfStr,
            msgInfo->MessageId, encType, msgInfo->KeyId,
            payload->DataLength(), payload->DataLength(), payload->Start());

    // Release the message buffer.
    PacketBuffer::Free(payload);
}

void HandleMessageReceived(WeaveConnection *con, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    HandleMessageReceived(&MessageLayer, msgInfo, msgBuf);
}

void HandleReceiveError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err, const IPPacketInfo *pktInfo)
{
    printf("WEAVE MESSAGE RECEIVE ERROR: %s\n", ErrorStr(err));
}

void HandleReceiveError(WeaveConnection *con, WEAVE_ERROR err)
{
    HandleReceiveError(&MessageLayer, err, NULL);
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char nodeAddrStr[64];
    con->PeerAddr.ToString(nodeAddrStr, sizeof(nodeAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, nodeAddrStr);

    con->OnConnectionClosed = HandleInboundConnectionClosed;
    con->OnMessageReceived = HandleMessageReceived;
    con->OnReceiveError = HandleReceiveError;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char nodeAddrStr[64];
    con->PeerAddr.ToString(nodeAddrStr, sizeof(nodeAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, nodeAddrStr);
    else
    {
        printf("Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, nodeAddrStr, ErrorStr(conErr));
        LastSendTime = Now();
    }
}

void HandleOutboundConnectionClosed(WeaveConnection *con, WEAVE_ERROR err)
{
    char nodeAddrStr[64];
    con->PeerAddr.ToString(nodeAddrStr, sizeof(nodeAddrStr));

    if (err == WEAVE_NO_ERROR)
        printf("Connection to node %" PRIX64 " (%s) closed\n", con->PeerNodeId, nodeAddrStr);
    else
        printf("Connection to node %" PRIX64 " (%s) ABORTED: %s\n", con->PeerNodeId, nodeAddrStr, ErrorStr(err));
}

void HandleInboundConnectionClosed(WeaveConnection *con, WEAVE_ERROR err)
{
    HandleOutboundConnectionClosed(con, err);
    con->Close();
}

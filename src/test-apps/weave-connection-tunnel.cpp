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
 *      This file aims to test the WeaveConnectionTunnel functionality, three kinds of nodes:
 *          - ConnectionTunnelAgent: create connections to Tunnel Source and Destination, establish tunnel between them
 *          - ConnectionTunnelSource: wait for connection from Tunnel Agent, act as sender to verify tunnel link
 *          - ConnectionTunnelDestination: wait for connection from Tunnel Agent, act as receiver to verify tunnel link
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"

#define TOOL_NAME "weave-connection-tunnel"

enum
{
    ConnectionTunnelAgent = 0,
    ConnectionTunnelSource,
    ConnectionTunnelDest,
};

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void StartConnections();
static void DriveSending();
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleMessageReceived(WeaveConnection *con, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf);

uint8_t Role = ConnectionTunnelAgent;
WeaveConnectionTunnel *tun = NULL;
WeaveConnection *conSource = NULL;
WeaveConnection *conDest = NULL;
WeaveConnection *connection = NULL;
uint64_t TunnelSourceNodeId;
uint64_t TunnelDestNodeId;
IPAddress TunnelSourceAddr = IPAddress::Any;
IPAddress TunnelDestAddr = IPAddress::Any;

static OptionDef gToolOptionDefs[] =
{
    { "tunnel-source",          kNoArgument, 'S' },
    { "tunnel-destination",     kNoArgument, 'D' },
    { "tunnel-agent",           kNoArgument, 'A' },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -S, --tunnel-source\n"
    "       Specify the node as tunnel source, act as sender to vefify tunnel link\n"
    "\n"
    "  -D, --tunnel-destination\n"
    "       Specify the node as tunnel destination, act as receiver to vefify tunnel link\n"
    "\n"
    "  -A, --tunnel-agent\n"
    "       Specify the node as tunnel agent, establish connection tunnel between source node and destination node\n"
    "\n";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>] --tunnel-source\n"
    "       " TOOL_NAME " [<options...>] --tunnel-destination\n"
    "       " TOOL_NAME " [<options...>] --tunnel-agent <source-node-id> <dest-node-id>\n",
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
    InitToolCommon();

    SetSIGUSR1Handler();

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

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = gNetworkOptions.LocalIPv6Addr.InterfaceId();
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, false);

    MessageLayer.OnConnectionReceived = HandleConnectionReceived;

    PrintNodeConfig();

    // Tunnel Agent: create connections to Tunnel Source and Destination
    if (Role == ConnectionTunnelAgent)
       StartConnections();

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (Role == ConnectionTunnelSource)
            DriveSending();
    }

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

void StartConnections()
{
    WEAVE_ERROR err;

    conSource = MessageLayer.NewConnection();
    conDest = MessageLayer.NewConnection();
    VerifyOrExit(conSource != NULL && conDest != NULL, err = WEAVE_ERROR_NO_MEMORY);

    conSource->OnConnectionComplete = conDest->OnConnectionComplete = HandleConnectionComplete;

    TunnelSourceAddr = FabricState.SelectNodeAddress(TunnelSourceNodeId);
    TunnelDestAddr = FabricState.SelectNodeAddress(TunnelDestNodeId);

    err = conSource->Connect(TunnelSourceNodeId, TunnelSourceAddr);
    SuccessOrExit(err);

    err = conDest->Connect(TunnelDestNodeId, TunnelDestAddr);
    SuccessOrExit(err);
    return;

exit:
    printf("Tunnel Agent: failed to create connections\n");
    exit(EXIT_FAILURE);
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'A':
        Role = ConnectionTunnelAgent;
        break;
    case 'S':
        Role = ConnectionTunnelSource;
        break;
    case 'D':
        Role = ConnectionTunnelDest;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (Role == ConnectionTunnelAgent)
    {
        if (argc < 2)
        {
            PrintArgError("%s: weave-connection-tunnel: Please specify the tunnel source and destination node-id\n", progName);
            return false;
        }

        if (argc > 2)
        {
            PrintArgError("%s: weave-connection-tunnel: Unexpected argument: %s\n", argv[2]);
            return false;
        }

        if (!ParseNodeId(argv[0], TunnelSourceNodeId))
        {
            PrintArgError("%s: weave-connection-tunnel: Invalid value specified for tunnel source node id: %s\n", progName, argv[0]);
            return false;
        }

        if (!ParseNodeId(argv[1], TunnelDestNodeId))
        {
            PrintArgError("%s: weave-connection-tunnel: Invalid value specified for tunnel destination node id: %s\n", progName, argv[1]);
            return false;
        }
    }

    else
    {
        if (argc > 0)
        {
            PrintArgError("%s: weave-connection-tunnel: Unexpected argument: %s\n", progName, argv[0]);
            return false;
        }
    }

    return true;
}

void HandleMessageReceived(WeaveConnection *con, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    if (Role == ConnectionTunnelDest)
    {
        printf("%s\n", msgBuf->Start());
        con->Close();
        Done = true;
    }
    PacketBuffer::Free(msgBuf);
}

// Tunnel Source and Destination: will receive connection from Tunnel Agent.
void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    connection = con;
    connection->OnMessageReceived = HandleMessageReceived;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    if ((con == conSource) && (conErr == WEAVE_NO_ERROR))
        printf("Tunnel Agent: source connected\n");

    if ((con == conDest) && (conErr == WEAVE_NO_ERROR))
        printf("Tunnel Agent: destination connected\n");

    // Tunnel Agent: establish tunnel when the two connections are completed
    if ((conSource->State == WeaveConnection::kState_Connected) && (conDest->State == WeaveConnection::kState_Connected))
    {
        res = MessageLayer.CreateTunnel(&tun, *conSource, *conDest, 1000000);
        if (res != WEAVE_NO_ERROR)
        {
            printf("Tunnel Agent: failed to establish tunnel\n");
            exit(-1);
        }
    }
}

void DriveSending()
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    WeaveMessageInfo msgInfo;
    uint32_t len = 0;
    char *p;

    if ((connection != NULL) && (connection->State == WeaveConnection::kState_Connected))
    {
        msgBuf = PacketBuffer::New();
        if (msgBuf == NULL)
        {
            printf("Tunnel Source: PacketBuffer alloc failed\n");
            exit(-1);
        }
        p = (char *)msgBuf->Start();
        len = sprintf(p, "Message from tunnel source node to destination node\n");
        msgBuf->SetDataLength(len);

        msgInfo.Clear();
        msgInfo.MessageVersion = kWeaveMessageVersion_V2;
        msgInfo.Flags = 0;
        msgInfo.SourceNodeId = FabricState.LocalNodeId;
        msgInfo.DestNodeId = kNodeIdNotSpecified;
        msgInfo.EncryptionType = kWeaveEncryptionType_None;
        msgInfo.KeyId = WeaveKeyId::kNone;

        res = connection->SendMessage(&msgInfo, msgBuf);
        if (res != WEAVE_NO_ERROR)
        {
            printf("Tunnel Source: failed to send message\n");
            exit(-1);
        }
    }
}

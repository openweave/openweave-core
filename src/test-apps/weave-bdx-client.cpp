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
 *      This file implements a process to effect a functional test for
 *      a client for the Weave Bulk Data Transfer (BDX) profile.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include "nlweavebdxclient.h"

using nl::StatusReportStr;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;

#define TOOL_NAME "weave-bdx-client"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void StartClientConnection();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

const char *DestIPAddrStr = NULL;
const char *DestFileName = NULL;
BulkDataTransferClient BDXClient;
bool ClientConEstablished = false;
bool DestHostNameResolved = false;  // only used for UDP

//Globals used by BDX-client
bool WaitingForBDXResp = false;
uint64_t DestNodeId = 1;
IPAddress DestIPAddr;
WeaveConnection *Con = NULL;

static OptionDef gToolOptionDefs[] =
{
    { "output-file", kArgumentRequired, 'o' },
    { "dest-addr",   kArgumentRequired, 'D' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -o, --output-file <filename>\n"
    "       File to which bulk data will be written. Bulk data not written to disk\n"
    "       by default. Accepts paths relative to current working directory.\n"
    "\n"
    "  -D, --dest-addr <ip-addr>\n"
    "       Send ReceiveInit requests to a specific address rather than one\n"
    "       derived from the destination node id.  <ip-addr> can be an IPv4\n"
    "       address or an IPv6 address.\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-ip-addr>\n",
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
    WEAVE_ERROR err;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv);
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
            exit(-1);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(false, true);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    // Initialize the BDX-client application.
    err = BDXClient.Init(&ExchangeMgr, DestFileName);
    if (err != WEAVE_NO_ERROR)
    {
        printf("BulkDataTransferClient::Init failed: %s\n", ErrorStr(err));
        exit(-1);
    }

    PrintNodeConfig();

    if (DestNodeId == 0)
        printf("Sending BDX requests to node at %s\n", DestIPAddrStr);
    else if (DestIPAddrStr == NULL)
        printf("Sending BDX requests to node %" PRIX64 "\n", DestNodeId);
    else
        printf("Sending BDX requests to node %" PRIX64 " at %s\n", DestNodeId, DestIPAddrStr);

    //Set up connection and connect callbacks to handle success/failure cases
    StartClientConnection();

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
    }

    BDXClient.Shutdown();
    ShutdownWeaveStack();

    return 0;
}

void StartClientConnection()
{
    printf("@@@ 0 StartClientConnection entering (Con: %p)\n", Con);

    if (Con != NULL && Con->State == WeaveConnection::kState_Closed)
    {
        printf("@@@ 1 remove previous con (currently closed)\n");
        Con->Close();
        Con = NULL;
    }

    // Do nothing if a connect attempt is already in progress.
    if (Con != NULL)
    {
        printf("@@@ 2 (Con: %p) previous Con likely hanging\n", Con);
        return;
    }

    Con = MessageLayer.NewConnection();
    if (Con == NULL)
    {
        printf("@@@ 3 WeaveConnection.Connect failed: no memory\n");
        return;
    }
    printf("@@@ 3+ (Con: %p)\n", Con);
    Con->OnConnectionComplete = HandleConnectionComplete;
    Con->OnConnectionClosed = HandleConnectionClosed;

    printf("@@@ 3++ (DestNodeId: %ld, DestIPAddrStr: %s)\n", (long)DestNodeId, DestIPAddrStr);
    IPAddress::FromString(DestIPAddrStr, DestIPAddr);
    WEAVE_ERROR err = Con->Connect(DestNodeId, kWeaveAuthMode_Unauthenticated, DestIPAddr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("@@@ 4 WeaveConnection.Connect failed: %X (%s)\n", err, ErrorStr(err));
        Con->Close();
        Con = NULL;
        return;
    }

    BDXClient.setCon(Con);

    printf("@@@ 5 StartClientConnection exiting\n");
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'o':
        DestFileName = arg;
        break;
    case 'S':
        UsePASE = true;
        break;
    case 'D':
        DestIPAddrStr = arg;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc < 1)
    {
        PrintArgError("%s: Please specify either the destination node id\n", progName);
        return false;
    }

    if (argc > 1)
    {
        PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
        return false;
    }

    const char *nodeId = argv[0];
    char *p = (char *)strchr(nodeId, '@');
    if (p != NULL)
    {
        *p = 0;
        DestIPAddrStr = p+1;
    }

    if (!ParseNodeId(nodeId, DestNodeId))
    {
        PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
        return false;
    }

    return true;
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    printf("@@@ 1 HandleConnectionComplete entering\n");

    WEAVE_ERROR err;
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();
        Con = NULL;
        BDXClient.setCon(NULL);
        ClientConEstablished = false;
        return;
    }

    printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    ClientConEstablished = true;

    PacketBuffer *payloadBuf = PacketBuffer::New();
    if (payloadBuf == NULL)
    {
        printf("@@@ 3 Unable to allocate PacketBuffer\n");
        return;
    }

    //Send the ReceiveInit request
    if (Con != NULL)
    {
        printf("@@@ 4 Sending TCP bdx request\n");
        err = BDXClient.SendReceiveInitRequest(Con);
    }
    else
    {
        char buffer[64];
        printf("@@@ 5 (destIPAddr: %s (printed into a string))\n", DestIPAddr.ToString(buffer, strlen(buffer)));
        err = BDXClient.SendReceiveInitRequest(DestNodeId, DestIPAddr);
    }
    if (err == WEAVE_NO_ERROR)
    {
        WaitingForBDXResp = true;
    }
    else
    {
        printf("@@@ 6 BDXClient.SendRequest() failed: %X\n", err);
        Con->Close();
        Con = NULL;
    }

    printf("@@@ 7 HandleConnectionComplete exiting\n");
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    WaitingForBDXResp = false;

    if (con == Con)
    {
        con->Close();
        Con = NULL;
    }
}


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
 *      a client for the Weave Software Update (SWU) profile.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include "nlweaveswuclient.h"
#include "MockIAServer.h"

using nl::StatusReportStr;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;

#define TOOL_NAME "weave-swu-client"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void StartClientConnection();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

bool Listening = false;
bool UseTCP = true;
bool Debug = false;
const char *DestIPAddrStr = NULL;
uint16_t DestPort; // only used for UDP
SoftwareUpdateClient SWUClient;
MockImageAnnounceServer MIAServer;

//Globals used by SWU-client
bool WaitingForSWUResp = false;
static uint64_t DestNodeId = 1;
IPAddress DestIPAddr;
WeaveConnection *Con = NULL;

static OptionDef gToolOptionDefs[] =
{
    { "listen",     kNoArgument,       'L' },
    { "dest-addr",  kArgumentRequired, 'D' },
    { "debug",      kArgumentRequired, 'd' },
    { "tcp",        kNoArgument,       't' },
    { "udp",        kNoArgument,       'u' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>]\n"
    "       Send an ImageQuery request to a specific address rather than one\n"
    "       derived from the destination node id.  <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address.  If <port> is specified, ImageQuery\n"
    "       requests will be sent to the specified port.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send SWU Requests. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to send SWU Requests.\n"
    "\n"
    "  -L, --listen\n"
    "       Listen and respond to ImageAnnounce notifications sent from another node.\n"
    "\n"
    "  -d, --debug\n"
    "       Enable debug messages.\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>[:<dest-port>][%%<interface>]]\n"
    "       " TOOL_NAME " [<options...>] --listen\n",
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

static void HandleImageAnnounce(ExchangeContext *ec)
{
    WEAVE_ERROR err;

    printf("0 SWU HandleImageAnnounce entering\n");

    if (Listening)
    {
        printf("1 SWU HandleImageAnnounce (while listening, Con: %p)\n", Con);

        if (Con != NULL)
        {
            printf("2 SWU HandleImageAnnounce Sending TCP ImageQuery request\n");
            err = SWUClient.SendImageQueryRequest(Con);
        }
        else
        {
            char buffer[64];
            printf("3 SWU HandleImageAnnounce  (destIPAddr: %s (printed as a string))\n", DestIPAddr.ToString(buffer, strlen(buffer)));
            err = SWUClient.SendImageQueryRequest(DestNodeId, DestIPAddr);
        }
        if (err == WEAVE_NO_ERROR)
        {
            WaitingForSWUResp = true;
        }
        else
        {
            printf("4 SWUClient.SendRequest() failed: %X\n", err);
            Con->Close();
            Con = NULL;
        }
    }
    else
    {
        printf("5 SWU HandleImageAnnounce (while not listening)\n");
    }

    printf("6 SWU HandleImageAnnounce exiting\n");
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

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
            exit(-1);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(Listening || !UseTCP, true);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    // Initialize the SWU-client application.
    err = SWUClient.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("SoftwareUpdateClient::Init failed: %s\n", ErrorStr(err));
        exit(-1);
    }

    err = MIAServer.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("MockImageAnnounceServer::Init failed: %s\n", ErrorStr(err));
        exit(-1);
    }
    MIAServer.OnImageAnnounceReceived = HandleImageAnnounce;

    PrintNodeConfig();

    if (!Listening)
    {
        if (DestNodeId == 0)
            printf("Sending SWU requests to node at %s\n", DestIPAddrStr);
        else if (DestIPAddrStr == NULL)
            printf("Sending SWU requests to node %" PRIX64 "\n", DestNodeId);
        else
            printf("Sending SWU requests to node %" PRIX64 " at %s\n", DestNodeId, DestIPAddrStr);

        //Set up connection and connect callbacks to handle success/failure cases
        StartClientConnection();
    }
    else
    {
        if (!UseTCP)
        {
            MIAServer.CreateExchangeCtx(DestNodeId, DestIPAddr);
        }
        printf("Listening for ImageAnnounce notifications...\n");
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
    }

    if (Con != NULL)
    {
        Con->Close();
        Con = NULL;
    }

    MIAServer.Shutdown();
    SWUClient.Shutdown();
    printf("Completed the SWU interactive protocol test!\n");
    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

void StartClientConnection()
{
    printf("0 StartClientConnection entering (Con: %p)\n", Con);

    if (Con != NULL && Con->State == WeaveConnection::kState_Closed)
    {
        printf("  1 remove previous con (currently closed)\n");
        Con->Close();
        Con = NULL;
    }

    // Create a new connection unless there is already one in progress
    // (probably started via an ImageAnnounce notification.)
    if (Con == NULL)
    {
        printf("  2 no existing connection (probably no ImageAnnounce received)\n");
        Con = MessageLayer.NewConnection();
        if (Con == NULL)
        {
            printf("  3 WeaveConnection.Connect failed: no memory\n");
            return;
        }
        Con->OnConnectionComplete = HandleConnectionComplete;
        Con->OnConnectionClosed = HandleConnectionClosed;
        printf("  4 Con: %p\n", Con);

        printf("  5 (DestNodeId: %ld, DestIPAddrStr: %s)\n", (long)DestNodeId, DestIPAddrStr);
        IPAddress::FromString(DestIPAddrStr, DestIPAddr);
        WEAVE_ERROR err = Con->Connect(DestNodeId, kWeaveAuthMode_Unauthenticated, DestIPAddr);
        if (err != WEAVE_NO_ERROR)
        {
            printf("  6 WeaveConnection.Connect failed: %X (%s)\n", err, ErrorStr(err));
            Con->Close();
            Con = NULL;
            return;
        }
    }
    else
    {
        printf("  7 existing connection (probably ImageAnnounce received)\n");
        HandleConnectionComplete(Con, WEAVE_NO_ERROR);
    }

    printf("8 StartClientConnection exiting\n");
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 't':
        UseTCP = true;
        break;
    case 'u':
        UseTCP = false;
        break;
    case 'L':
        Listening = true;
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
            DestIPAddrStr = p+1;
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

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
    Con = con;
    MIAServer.CreateExchangeCtx(con);
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    printf("0 HandleConnectionComplete entering\n");

    WEAVE_ERROR err;
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("  1 Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();
        Con = NULL;
        return;
    }

    printf("  2 Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    printf("  4 PacketBuffer for ImageQuery request\n");

    if (Con != NULL)
    {
        printf("  5 Sending TCP ImageQuery request\n");
        err = SWUClient.SendImageQueryRequest(Con);
    }
    else
    {
        char buffer[64];
        printf("  6 (destIPAddr: %s (printed into a string))\n", DestIPAddr.ToString(buffer, strlen(buffer)));
        err = SWUClient.SendImageQueryRequest(DestNodeId, DestIPAddr);
    }
    if (err == WEAVE_NO_ERROR)
    {
        WaitingForSWUResp = true;
    }
    else
    {
        printf("7 SWUClient.SendRequest() failed: %X\n", err);
        Con->Close();
        Con = NULL;
    }

    printf("8 HandleConnectionComplete exiting\n");
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    WaitingForSWUResp = false;

    if (Listening)
        con->Close();
    else if (con == Con)
    {
        con->Close();
        Con = NULL;
    }
}

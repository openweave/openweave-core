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
 *      This file runs a BDX-v0 Server that will listen for incoming ReceiveInit.
 *
 *      Command:
 *          ./weave-bdx-server-v0 -a fd00:0:1:1::1 -r /path/requested-file
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>

#include "ToolCommon.h"
#include "nlweavebdxserver.h"

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::BulkDataTransfer;

#define TOOL_NAME "weave-bdx-server-v0"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

BulkDataTransferServer BDXServer;

const char *RequestedFileName = NULL;
const char *ReceivedFileLocation = NULL;

static OptionDef gToolOptionDefs[] =
{
    { "requested-file", kArgumentRequired, 'r' },
    { "received-loc",   kArgumentRequired, 'R' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -r, --requested-file <filename>\n"
    "       File to send for a download.\n"
    "       Normally a URL for upload (ex. www.google.com), and a local path for download\n"
    "       (ex. testing.txt). Accepts paths relative to current working directory\n"
    "\n"
    "  -R, --received-loc <path>\n"
    "       Location to save a file from a receive transfer.\n"
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
    "Usage: " TOOL_NAME " [<options...>]\n",
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
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;

    InitToolCommon();

    UseStdoutLineBuffering();
    SetupFaultInjectionContext(argc, argv);
    SetSignalHandler(DoneOnHandleSIGUSR1);

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
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
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(true, true);
    MessageLayer.RefreshEndpoints();

    // This test program enables faults and stats prints always (no option taken from the CLI)
    gFaultInjectionOptions.DebugResourceUsage = true;
    gFaultInjectionOptions.PrintFaultCounters = true;

    nl::Weave::Stats::UpdateSnapshot(before);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    // Initialize the BDX-server application.
    err = BDXServer.Init(&ExchangeMgr, NULL, RequestedFileName, ReceivedFileLocation);
    if (err != WEAVE_NO_ERROR)
    {
        printf("BulkDataTransferServer::Init failed: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    PrintNodeConfig();

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
        fflush(stdout);
    }

    if (gSigusr1Received)
    {
        printf("Sigusr1Received\n");
        fflush(stdout);
    }

    BDXServer.Shutdown();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'r':
        RequestedFileName = arg;
        break;
    case 'R':
        ReceivedFileLocation = arg;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
}

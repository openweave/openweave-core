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
 *      This file runs a BDX Server that will listen for incoming SendInit or ReceiveInit
 *      messages.  The callbacks used for the BDXTrasfer's application logic are from
 *      weave-bdx-common.*
 *
 *      NOTE: to run it on a local machine along with a test client, use this command:
 *          ./weave-bdx-server -a 127.0.0.1
 *      This will bind the BDXServer's Weave stack to this address so that the client
 *      can share the same Weave port.
 *
 */

#define __STDC_FORMAT_MACROS

#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development

#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>

#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "ToolCommon.h"
#include "weave-bdx-common-development.h"

//#define BDX_TEST_USE_TEST_APP_IMPL
#ifdef BDX_TEST_USE_TEST_APP_IMPL
#include "nlweavebdxserver.h"
#else
#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#endif


using nl::StatusReportStr;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::BulkDataTransfer;
using namespace nl::Weave::Profiles::Common;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Logging;

#define TOOL_NAME "weave-bdx-server-development"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

// Global instances for this test program
#ifdef BDX_TEST_USE_TEST_APP_IMPL
BulkDataTransferServer BDXServer;
#else
BdxServer BDXServer;
#endif

const char *SaveFileLocation = NULL;
const char *TempFileLocation = NULL;

static OptionDef gToolOptionDefs[] =
{
    { "received-loc", kArgumentRequired, 'R' },
    { "temp-loc",     kArgumentRequired, 'T' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -R, --received-loc <path>\n"
    "       Location to save a transferred file.\n"
    "\n"
    "  -T, --temp-loc <path>\n"
    "       Location to keep temporary files.\n"
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

#if !(WEAVE_CONFIG_BDX_SERVER_SUPPORT)
    printf("ERROR: Running BDX server with WEAVE_CONFIG_BDX_SERVER_SUPPORT disabled does not make sense.\n");
    exit(EXIT_FAILURE);
#endif

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
    ResetAppStates();

    // This test program enables faults and stats prints always (no option taken from the CLI)
    gFaultInjectionOptions.DebugResourceUsage = true;
    gFaultInjectionOptions.PrintFaultCounters = true;

    nl::Weave::Stats::UpdateSnapshot(before);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    // Initialize the BDX-server application.
#ifdef BDX_TEST_USE_TEST_APP_IMPL
    err = BDXServer.Init(&ExchangeMgr, NULL);
#else
    err = BDXServer.Init(&ExchangeMgr);
#endif
    if (err != WEAVE_NO_ERROR)
    {
        printf("BulkDataTransferServer::Init failed: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    // New implementation sets this when init is called
#ifdef BDX_TEST_USE_TEST_APP_IMPL
    BDXServer.AllowBDXServerToRun(true);
#endif

    PrintNodeConfig();

#if !defined(BDX_TEST_USE_TEST_APP_IMPL) && WEAVE_CONFIG_BDX_SERVER_SUPPORT
    BDXServer.AwaitBdxSendInit(BdxSendInitHandler);
    BDXServer.AwaitBdxReceiveInit(BdxReceiveInitHandler);
#endif

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
    }

#ifdef BDX_TEST_USE_TEST_APP_IMPL
    BDXServer.Shutdown();
#else
    BDXServer.Shutdown();
#endif

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'R':
        SaveFileLocation = arg;
        SetReceivedFileLocation(SaveFileLocation);
        break;
    case 'T':
        TempFileLocation = arg;
        SetTempLocation(TempFileLocation);
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

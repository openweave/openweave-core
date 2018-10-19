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
 *      This file runs a BDXServer acting as a simple client that will upload/download
 *      the specified file by connecting to a server that you specify.  The callbacks
 *      used to define application logic are defined in weave-bdx-common.*
 *      To run the client on the same local machine as the server for testing purposes,
 *      use this command:
 *          ./weave-bdx-client 1@127.0.0.1 [...]
 *      If you used the same advice given in weave-bdx-server.cpp, the server will be
 *      bound to the localhost address and so contacting that IP address will properly
 *      route messages to it to that process while allowing both processes to use the
 *      same Weave port.
 *
 */

#define __STDC_FORMAT_MACROS

#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "ToolCommon.h"
#include "weave-bdx-common-development.h"

#define TOOL_NAME "weave-bdx-client-development"

#define BDX_CLIENT_DEFAULT_START_OFFSET    0
#define BDX_CLIENT_DEFAULT_FILE_LENGTH     0
#define BDX_CLIENT_DEFAULT_MAX_BLOCK_SIZE  512

using nl::StatusReportStr;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development);
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Logging;

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void PreTest();
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void StartClientConnection(System::Layer* lSystemLayer, void* aAppState, System::Error aError);
static void StartUDPUpload();
static void StartUDPDownload();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleTransferTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static WEAVE_ERROR PrepareBinding();
static void HandleBindingEvent(void *const ctx, const Binding::EventType event, const Binding::InEventParam &inParam, Binding::OutEventParam &outParam);

BdxClient BDXClient;
BdxAppState * appState;

bool Listening = false;
uint32_t ConnectInterval = 200;  //ms
uint32_t TransferTimeout = 3000;  //ms
uint32_t ConnectTry = 0;
uint32_t ConnectMaxTry = 3;
uint64_t StartOffset = BDX_CLIENT_DEFAULT_START_OFFSET;
uint64_t FileLength = BDX_CLIENT_DEFAULT_FILE_LENGTH;
uint64_t MaxBlockSize = BDX_CLIENT_DEFAULT_MAX_BLOCK_SIZE;
bool Upload = false; // download by default
bool UseTCP = true;
const char *DestIPAddrStr = NULL;
const char *RequestedFileName = NULL;
const char *ReceivedFileLocation = NULL;
bool ClientConEstablished = false;
bool Pretest = false;

//Globals used by BDX-client
bool WaitingForBDXResp = false;
uint64_t DestNodeId = 1;
IPAddress DestIPAddr;
WeaveConnection *Con = NULL;
nl::Weave::Binding *TheBinding = NULL;


static OptionDef gToolOptionDefs[] =
{
    { "requested-file", kArgumentRequired, 'r' },
    { "start-offset",   kArgumentRequired, 's' },
    { "length",         kArgumentRequired, 'l' },
    { "block-size",     kArgumentRequired, 'b' },
    { "dest-addr",      kArgumentRequired, 'D' },
    { "received-loc",   kArgumentRequired, 'R' },
    { "debug",          kArgumentRequired, 'd' },
    { "upload",         kNoArgument,       'p' },
    { "tcp",            kNoArgument,       't' },
    { "udp",            kNoArgument,       'u' },
    { "pretest",        kNoArgument,       'T' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -r, --requested-file <filename>\n"
    "       File to request from the sender for an upload, or file to send for a download.\n"
    "       Normally a URL for upload (ex. www.google.com), and a local path for download\n"
    "       (ex. testing.txt). Accepts paths relative to current working directory\n"
    "\n"
    "  -s, --start-offset <num>\n"
    "       Start offset to request when downloading (in bytes)\n"
    "\n"
    "  -l, --length <num>\n"
    "       Length of file to request when downloading (in bytes). 0 means indefinite (whole file).\n"
    "\n"
    "  -b, --block-size <num>\n"
    "       Max block size to propose in a transfer. Defaults to 512.\n"
    "\n"
    "  -D, --dest-addr <ip-addr>\n"
    "       Send ReceiveInit requests to a specific address rather than one\n"
    "       derived from the destination node id.  <ip-addr> can be an IPv4 or IPv6 address.\n"
    "\n"
    "  -R, --received-loc <path>\n"
    "       Location to save a file from a receive transfer.\n"
    "\n"
    "  -p, --upload\n"
    "       Upload a file to the BDX server rather than download one from it, which is the default.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send BDX Requests. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to send BDX Requests.\n"
    "\n"
    "  -T, --pretest\n"
    "       Perform initial unit tests.\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-ip-addr>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWRMPOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void ResetTestContext(void)
{
    appState->mDone = false;
}

static bool sTransferTimerIsRunning = false;

static int32_t GetNumAsyncEventsAvailable(void)
{
    int32_t retval = 0;

    if (sTransferTimerIsRunning)
    {
        retval = 1;
    }

    return retval;
}

static void ExpireTimer(int32_t argument)
{
    (void)argument;

    SystemLayer.StartTimer(0, HandleTransferTimeout, NULL);
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;
    uint32_t iter;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv, GetNumAsyncEventsAvailable, ExpireTimer);
    SetSIGUSR1Handler();

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    // This test program enables faults and stats prints always (no option taken from the CLI)
    gFaultInjectionOptions.DebugResourceUsage = true;
    gFaultInjectionOptions.PrintFaultCounters = true;

    if (Pretest)
        PreTest();

    if (RequestedFileName == NULL)
    {
        printf("No destination file name given in -r argument\n");
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
    InitWeaveStack(Listening || !UseTCP, true);
    ResetAppStates();

    nl::Weave::Stats::UpdateSnapshot(before);

    appState = NewAppState();

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    PrintNodeConfig();

    if (DestNodeId == 0)
        printf("Sending BDX requests to node at %s\n", DestIPAddrStr);
    else if (DestIPAddrStr == NULL)
        printf("Sending BDX requests to node %" PRIX64 "\n", DestNodeId);
    else
        printf("Sending BDX requests to node %" PRIX64 " at %s\n", DestNodeId, DestIPAddrStr);

    appState->mDone = false;

#if !(WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT)
    if (Upload)
    {
        printf("Cannot upload with WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT disabled.\n");
        exit(EXIT_FAILURE);
    }
#endif

#if !(WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT)
    if (!Upload)
    {
        printf("Cannot download with WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT disabled.\n");
        exit(EXIT_FAILURE);
    }
#endif

    err = BDXClient.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("BulkDataTransferClient::Init failed: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    for (iter = 0; iter < gFaultInjectionOptions.TestIterations; iter++)
    {
        printf("Iteration %u\n", iter);

        // Init the client again in case the previous iteration failed with a timeout
        (void)BDXClient.Init(&ExchangeMgr);

        if (UseTCP)
        {
            err = SystemLayer.StartTimer(ConnectInterval, StartClientConnection, NULL);
            if (err != WEAVE_NO_ERROR)
            {
                printf("Inet.StartTimer failed\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            err = PrepareBinding();
            if (err != WEAVE_NO_ERROR)
            {
                appState->mDone = true;
            }
        }

        while (!appState->mDone)
        {
            struct timeval sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_usec = 100000;

            ServiceNetwork(sleepTime);
        }

        if (appState->mFile)
        {
            fclose(appState->mFile);
            appState->mFile = NULL;
        }

        if (Con)
        {
            Con->Close();
            Con = NULL;
        }

        if (TheBinding)
        {
            TheBinding->Release();
            TheBinding = NULL;
        }

        SystemLayer.CancelTimer(HandleTransferTimeout, NULL);
        sTransferTimerIsRunning = false;

        ResetTestContext();
    }

    BDXClient.Shutdown();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();

    return EXIT_SUCCESS;
}

static void HandleTransferTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    printf("transfer timeout\n");

    sTransferTimerIsRunning = false;
    appState->mDone = true;
    BDXClient.Shutdown();
}

static void StartClientConnection(System::Layer* lSystemLayer, void* aAppState, System::Error aError)
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

    //TODO: move this to BDX logic
    Con = MessageLayer.NewConnection();
    if (Con == NULL)
    {
        printf("@@@ 3 WeaveConnection.Connect failed: no memory\n");
        return;
    }
    printf("@@@ 3+ (Con: %p)\n", Con);
    Con->OnConnectionComplete = HandleConnectionComplete;
    Con->OnConnectionClosed = HandleConnectionClosed;

    printf("@@@ 3++ (DestNodeId: %" PRIX64 ", DestIPAddrStr: %s)\n", DestNodeId, DestIPAddrStr);

    WEAVE_ERROR err;
    if (DestIPAddrStr)
    {
        IPAddress::FromString(DestIPAddrStr, DestIPAddr);
        err = Con->Connect(DestNodeId, kWeaveAuthMode_Unauthenticated, DestIPAddr);
    }
    else // not specified, derive from NodeID
    {
        err = Con->Connect(DestNodeId);
    }

    if (err != WEAVE_NO_ERROR)
    {
        printf("@@@ 4 WeaveConnection.Connect failed: %X (%s)\n", err, ErrorStr(err));
        Con->Close();
        Con = NULL;
        return;
    }

    ConnectTry++;
    printf("@@@ 5 StartClientConnection exiting\n");
}

void StartUDPUpload()
{
#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BDXTransfer *xfer;
    ReferencedString refRequestedFileName, refFileName;
    char *filename = NULL;

    refRequestedFileName.init((uint16_t)strlen(RequestedFileName), (char *)RequestedFileName);

    BDXHandlers handlers =
    {
        BdxSendAcceptHandler, // SendAcceptHandler
        NULL,                 // ReceiveAcceptHandler
        BdxRejectHandler,     // RejectHandler
        BdxGetBlockHandler,   // GetBlockHandler
        NULL,                 // PutBlockHandler
        BdxXferErrorHandler,  // XferErrorHandler
        BdxXferDoneHandler,   // XferDoneHandler
        BdxErrorHandler       // ErrorHandler
    };

    err = BDXClient.NewTransfer(TheBinding, handlers, refRequestedFileName, appState, xfer);

    if (err != WEAVE_NO_ERROR)
    {
        printf("@@@ 6 BDXClient.NewTransfer() failed: %d\n", err);
        appState->mDone = true;
        TheBinding->Release();
        TheBinding = NULL;
        return;
    }

    xfer->mMaxBlockSize = MaxBlockSize;
    xfer->mStartOffset = StartOffset;
    xfer->mLength = FileLength;

    if (err == WEAVE_NO_ERROR)
    {
        // In the test-app, we need to make sure we only send the file name
        // in the mFileDesignator.
        filename = strrchr(refRequestedFileName.theString, '/');
        if (filename != NULL)
        {
            filename++; //skip over '/'
            refFileName.init((uint16_t)strlen(filename), filename);
            xfer->mFileDesignator = refFileName;
        }

        err = BDXClient.InitBdxSend(*xfer, true, false, false, NULL);

        // Set it back to what it was before so we can grab it when we're sending
        xfer->mFileDesignator = refRequestedFileName;

        WaitingForBDXResp = true;
    }
    else
    {
        printf("@@@ 6 BDXClient.StartUDPUpload() failed: %d\n", err);
        appState->mDone = true;
        BDXClient.ShutdownTransfer(xfer);
        TheBinding->Release();
        TheBinding = NULL;
    }
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
}

void StartUDPDownload()
{
#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BDXTransfer *xfer;
    ReferencedString refRequestedFileName;

    refRequestedFileName.init((uint16_t)strlen(RequestedFileName), (char *)RequestedFileName);

    BDXHandlers handlers =
    {
        NULL,                    // SendAcceptHandler
        BdxReceiveAcceptHandler, // ReceiveAcceptHandler
        BdxRejectHandler,        // RejectHandler
        NULL,                    // GetBlockHandler
        BdxPutBlockHandler,      // PutBlockHandler
        BdxXferErrorHandler,     // XferErrorHandler
        BdxXferDoneHandler,      // XferDoneHandler
        BdxErrorHandler          // ErrorHandler
    };

    err = BDXClient.NewTransfer(TheBinding, handlers, refRequestedFileName, appState, xfer);

    if (err != WEAVE_NO_ERROR)
    {
        printf("@@@ 6 BDXClient.NewTransfer() failed: %d\n", err);
        appState->mDone = true;
        TheBinding->Release();
        TheBinding = NULL;
        return;
    }

    xfer->mMaxBlockSize = MaxBlockSize;
    xfer->mStartOffset = StartOffset;
    xfer->mLength = FileLength;

    err = BDXClient.InitBdxReceive(*xfer, true, false, false, NULL);

    if (err == WEAVE_NO_ERROR)
    {
        WaitingForBDXResp = true;
    }
    else
    {
        printf("@@@ 6 BDXClient.StartUDPDownload() failed: %d\n", err);
        appState->mDone = true;
        BDXClient.ShutdownTransfer(xfer);
        TheBinding->Release();
        TheBinding = NULL;
    }
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'r':
        RequestedFileName = arg;
        break;
    case 's':
        if (!ParseInt(arg, StartOffset))
        {
            PrintArgError("%s: Invalid value specified for start offset: %s\n", progName, arg);
            return false;
        }
        break;
    case 'l':
        if (!ParseInt(arg, FileLength))
        {
            PrintArgError("%s: Invalid value specified for length: %s\n", progName, arg);
            return false;
        }
        break;
    case 'b':
        if (!ParseInt(arg, MaxBlockSize))
        {
            PrintArgError("%s: Invalid value specified for max block size: %s\n", progName, arg);
            return false;
        }
        break;
    case 'R':
        ReceivedFileLocation = arg;
        SetReceivedFileLocation(ReceivedFileLocation);
        break;
    case 'p':
        Upload = true;
        break;
    case 'T':
        Pretest = true;
        break;
    case 't':
        UseTCP = true;
        break;
    case 'u':
        UseTCP = false;
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
        PrintArgError("%s: Please specify a destination node id\n", progName);
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

        if (ConnectTry < ConnectMaxTry)
        {
            err = SystemLayer.StartTimer(ConnectInterval, StartClientConnection, NULL);
            if (err != WEAVE_NO_ERROR)
            {
                printf("Inet.StartTimer failed\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Connection FAILED to node %" PRIX64 " (%s) after %d attempts\n", con->PeerNodeId, ipAddrStr, ConnectTry);
            exit(EXIT_FAILURE);
        }

        ClientConEstablished = false;
        return;
    }

    printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    err = SystemLayer.StartTimer(TransferTimeout, HandleTransferTimeout, NULL);
    if (err != WEAVE_NO_ERROR)
    {
        printf("Inet.StartTimer failed\n");
        exit(EXIT_FAILURE);
    }
    sTransferTimerIsRunning = true;

    ClientConEstablished = true;

    //Send the ReceiveInit or SendInit request
    if (Con != NULL)
    {
        ReferencedString refRequestedFileName, refFileName;
        refRequestedFileName.init((uint16_t)strlen(RequestedFileName), (char *)RequestedFileName);

        // Initialize the BDX-client application.
        appState->mFile = NULL;

        printf("@@@ 4 Sending TCP bdx request\n");
        if (Upload)
        {
#if WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
            char *filename = NULL;
            BDXTransfer * xfer;
            BDXHandlers handlers =
            {
                BdxSendAcceptHandler, // SendAcceptHandler
                NULL,                 // ReceiveAcceptHandler
                BdxRejectHandler,     // RejectHandler
                BdxGetBlockHandler,   // GetBlockHandler
                NULL,                 // PutBlockHandler
                BdxXferErrorHandler,  // XferErrorHandler
                BdxXferDoneHandler,   // XferDoneHandler
                BdxErrorHandler       // ErrorHandler
            };

            err = BDXClient.NewTransfer(Con, handlers, refRequestedFileName, appState, xfer);

            if (err == WEAVE_NO_ERROR)
            {
                // In the test-app, we need to make sure we only send the file name
                // in the mFileDesignator.
                WeaveLogDetail(BDX, "%s", refRequestedFileName.theString);

                filename = strrchr(refRequestedFileName.theString, '/');
                if (filename != NULL)
                {
                    filename++; //skip over '/'
                    refFileName.init((uint16_t)strlen(filename), filename);
                    xfer->mFileDesignator = refFileName;
                }

                err = BDXClient.InitBdxSend(*xfer, true, false, false, NULL);

                // Set it back to what it was before so we can grab it when we're sending
                xfer->mFileDesignator = refRequestedFileName;
            }
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
        }
        else
        {
#if WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
            BDXTransfer * xfer;
            BDXHandlers handlers =
            {
                NULL,                    // SendAcceptHandler
                BdxReceiveAcceptHandler, // ReceiveAcceptHandler
                BdxRejectHandler,        // RejectHandler
                NULL,                    // GetBlockHandler
                BdxPutBlockHandler,      // PutBlockHandler
                BdxXferErrorHandler,     // XferErrorHandler
                BdxXferDoneHandler,      // XferDoneHandler
                BdxErrorHandler          // ErrorHandler
            };

            err = BDXClient.NewTransfer(Con, handlers, refRequestedFileName, appState, xfer);

            if (err == WEAVE_NO_ERROR)
            {
                err = BDXClient.InitBdxReceive(*xfer, true, false, false, NULL);
            }
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
        }
    }
    else
    {
        printf("Non-connection Init Requests not supported!\n");
        exit(EXIT_FAILURE);
    }

    if (err == WEAVE_NO_ERROR)
    {
        WaitingForBDXResp = true;
    }
    else
    {
        printf("@@@ 6 BDXClient.SendRequest() failed: %X\n", err);
        if (Con)
        {
            Con->Close();
            Con = NULL;
        }
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

    if (Listening)
        con->Close();
    else if (con == Con)
    {
        con->Close();
        Con = NULL;
    }
}

WEAVE_ERROR PrepareBinding()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Get the binding.
    TheBinding = ExchangeMgr.NewBinding(HandleBindingEvent, NULL);
    if (TheBinding == NULL)
    {
        printf("NewBinding failed: no memory\n");
        err = WEAVE_ERROR_NO_MEMORY;
        ExitNow();
    }
    else
    {
        // Configure the binding.
        nl::Weave::Binding::Configuration bindingConfig = TheBinding->BeginConfiguration()
            .Target_NodeId(DestNodeId)
            .Transport_UDP()
            .Security_None();

        if (DestIPAddrStr)
        {
            IPAddress::FromString(DestIPAddrStr, DestIPAddr);
            bindingConfig.TargetAddress_IP(DestIPAddr);
        }

        // Prepare the binding. Will finish asynchronously.
        err = bindingConfig.PrepareBinding();
        VerifyOrExit(err == WEAVE_NO_ERROR, printf("PrepareBinding failed\n"));
    }

exit:
    return err;
}

void HandleBindingEvent(void *const ctx, const Binding::EventType event, const Binding::InEventParam &inParam, Binding::OutEventParam &outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (event)
    {
    case nl::Weave::Binding::kEvent_BindingReady:
        err = SystemLayer.StartTimer(TransferTimeout, HandleTransferTimeout, NULL);
        if (err != WEAVE_NO_ERROR)
        {
            printf("Inet.StartTimer failed\n");
            return;
        }
        sTransferTimerIsRunning = true;

        if (Upload)
        {
            StartUDPUpload();
        }
        else
        {
            StartUDPDownload();
        }

        break;
    case nl::Weave::Binding::kEvent_PrepareFailed:
        printf("Binding prepare failed\n");
        break;
    default:
        nl::Weave::Binding::DefaultEventHandler(ctx, event, inParam, outParam);
    }
}

// unit tests to cover the codes that functional test failed to cover
void PreTest()
{
    SendInit sendInit;
    SendAccept sendAccept;
    ReceiveAccept receiveAccept;
    BlockQuery blockQuery;
    BlockSend blockSend;
    BlockSendV1 blockSendV1;
    BlockQueryV1 blockQueryV1;

    if (!(sendInit == sendInit)) {
        printf("SendAccept::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of SendInit is %d\n", sendInit.packedLength());

    if (!(sendAccept == sendAccept)) {
        printf("SendAccept::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of SendAccept is %d\n", sendAccept.packedLength());

    if (!(receiveAccept == receiveAccept)) {
        printf("ReceiveAccept::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of ReceiveAccept is %d\n", receiveAccept.packedLength());

    if (!(blockQuery == blockQuery)) {
        printf("BlockQuery::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of BlockQuery is %d\n", blockQuery.packedLength());

    if (!(blockSend == blockSend)) {
        printf("BlockSend::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of BlockSend is %d\n", blockSend.packedLength());

    if (!(blockSendV1 == blockSendV1)) {
        printf("BlockSendV1::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of BlockSendV1 is %d\n", blockSendV1.packedLength());

    if (!(blockQueryV1 == blockQueryV1)) {
        printf("BlockQueryV1::operator== failed\n");
        exit(EXIT_FAILURE);
    }
    printf("the default length of BlockQueryV1 is %d\n", blockQueryV1.packedLength());
}

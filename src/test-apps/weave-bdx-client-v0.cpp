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
 *      This file runs a BDX-v0 Client that will download the specified file from the specified server;
 *
 *      Command:
 *          ./weave-bdx-server-v0 1@fd00:0:1:1::1 -a fd00:0:1:1::2 -r /path/requested-file -R /received-file-path
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/bulk-data-transfer/BulkDataTransfer.h>

#define TOOL_NAME "weave-bdx-client-v0"

#define BDX_CLIENT_DEFAULT_START_OFFSET    0
#define BDX_CLIENT_DEFAULT_FILE_LENGTH     0
#define BDX_CLIENT_DEFAULT_MAX_BLOCK_SIZE  100

using namespace ::nl::Weave::Profiles::BulkDataTransfer;

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void OpenDestFile();
static void InitiateConnection(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static void HandleTransferTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static void PreTest();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

// handlers for the BDX client
static void handleReceiveAccept(ReceiveAccept *aReceiveAcceptMsg);
static void handleSendAccept(void *aAppState, SendAccept *aSendAcceptMsg);
static void handleReject(void *aAppState, StatusReport *aReport);
static void handleXferError(void *aAppState, StatusReport *aXferError);
static void handleDone(void *aAppState);
static void handleError(void *aAppState, WEAVE_ERROR anErrorCode);
static void handlePutBlock(uint64_t length, uint8_t *dataBlock, bool isLastBlock);
static void handleGetBlock(void *aAppState, uint64_t *pLength, uint8_t **aDataBlock, bool *isLastBlock);

uint64_t DestNodeId;
IPAddress DestAddr = IPAddress::Any;
bool Upload = false; // download by default
const char *RequestedFileName = NULL;
const char *ReceivedFileLocation = NULL;
FILE *DestFile = NULL;
uint32_t ConnectInterval = 200;  //ms
uint32_t TransferTimeout = 3000;  //ms
uint32_t ConnectTry = 0;
uint32_t ConnectMaxTry = 3;
FILE *SrcFile = NULL;
uint64_t StartOffset = BDX_CLIENT_DEFAULT_START_OFFSET;
uint64_t FileLength = BDX_CLIENT_DEFAULT_FILE_LENGTH;
uint64_t MaxBlockSize = BDX_CLIENT_DEFAULT_MAX_BLOCK_SIZE;
PacketBuffer *blockBuf = NULL;
bool Pretest = false;

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",      kArgumentRequired, 'D' },
    { "start-offset",   kArgumentRequired, 's' },
    { "length",         kArgumentRequired, 'l' },
    { "requested-file", kArgumentRequired, 'r' },
    { "received-loc",   kArgumentRequired, 'R' },
    { "block-size",     kArgumentRequired, 'b' },
    { "upload",         kNoArgument,       'p' },
    { "pretest",        kNoArgument,       'T' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <dest-ip-addr>\n"
    "       Connect to the specific IPv4/IPv6 address rather than one derived from the\n"
    "       destination node id.\n"
    "\n"
    "  -s, --start-offset <int>\n"
    "       Starting offset for file transfer.\n"
    "\n"
    "  -l, --length <int>\n"
    "       Length for file transfer.\n"
    "\n"
    "  -R, --received-loc <path>\n"
    "       Location to save a file from a receive transfer.\n"
    "\n"
    "  -r, --requested-file <filename>\n"
    "       File to request from the sender for an upload, or file to send for a download.\n"
    "       Normally a URL for upload (ex. www.google.com), and a local path for download\n"
    "       (ex. testing.txt). Accepts paths relative to current working directory\n"
    "\n"
    "  -b, --block-size <num>\n"
    "       Max block size to propose in a transfer. Defaults to 100.\n"
    "\n"
    "  -p, --upload\n"
    "       Upload a file to the BDX server rather than download one from it, which is the default.\n"
    "\n"
    "  -T, --pretest\n"
    "       Perform initial unit tests.\n"
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
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void ResetTestContext(void)
{
    Done = false;
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

    if (Pretest)
        PreTest();

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

    // This test program enables faults and stats prints always (no option taken from the CLI)
    gFaultInjectionOptions.DebugResourceUsage = true;
    gFaultInjectionOptions.PrintFaultCounters = true;

    nl::Weave::Stats::UpdateSnapshot(before);

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    // set up the BDX client
    WeaveBdxClient bdxClient;
    ReferencedString designator;

    const char *filename = strrchr(RequestedFileName, '/');
    if ((filename != NULL) && Upload) // only filename in sendinit
    {
        filename++; //skip over '/'
        designator.init((uint16_t)strlen(filename), (char *)filename);
    }
    else
        designator.init((uint16_t)strlen(RequestedFileName), (char *)RequestedFileName);

    if (DestAddr == IPAddress::Any)
        DestAddr = FabricState.SelectNodeAddress(DestNodeId);

    for (iter = 0; iter < gFaultInjectionOptions.TestIterations; iter++)
    {
        printf("Iteration %u\n", iter);

        bdxClient.initClient(&ExchangeMgr, NULL, designator, MaxBlockSize, StartOffset, FileLength, false);

        err = SystemLayer.StartTimer(ConnectInterval, InitiateConnection, &bdxClient);
        if (err != WEAVE_NO_ERROR)
        {
            printf("Inet.StartTimer failed\n");
            exit(EXIT_FAILURE);
        }

        PrintNodeConfig();

        while (!Done)
        {
            struct timeval sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_usec = 100000;

            ServiceNetwork(sleepTime);
        }

        if (blockBuf)
        {
            InetBuffer::Free(blockBuf);
            blockBuf = NULL;
        }

        SystemLayer.CancelTimer(HandleTransferTimeout, &bdxClient);
        sTransferTimerIsRunning = false;

        ResetTestContext();

        /* In BDXv0, this method closes the connection
         */
        bdxClient.shutdownClient();
    }

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

static void InitiateConnection(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveBdxClient* client = reinterpret_cast<WeaveBdxClient*>(aAppState);

    client->theConnection = MessageLayer.NewConnection();
    if (client->theConnection == NULL)
    {
       printf("MessageLayer.NewConnection failed\n");
       exit(EXIT_FAILURE);
    }
    client->theConnection->AppState = client;
    client->theConnection->OnConnectionComplete = HandleConnectionComplete;
    client->theConnection->OnConnectionClosed = HandleConnectionClosed;
    WEAVE_ERROR error = client->theConnection->Connect(DestNodeId, DestAddr);
    ConnectTry++;
    if (aError != WEAVE_SYSTEM_NO_ERROR)
        HandleConnectionComplete(client->theConnection, error);
}

static void HandleTransferTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    printf("transfer timeout\n");
    sTransferTimerIsRunning = false;
    Done = true;
}

static void handleReject(void *aAppState, StatusReport *aReport)
{
    printf("received reject message\n");
    Done = true;
}

static void handleXferError(void *aAppState, StatusReport *aXferError)
{
    printf("handled transfer error\n");
    Done = true;
}

static void handleDone(void *aAppState)
{
    printf("WEAVE:BDX: Transfer complete!\n");
    Done = true;
}

static void handleError(void *aAppState, WEAVE_ERROR anErrorCode)
{
    printf("handled internal BDX error - %d\n", anErrorCode);
    Done = true;
}

static void handleReceiveAccept(ReceiveAccept *aReceiveAcceptMsg)
{
    printf("received receive accept message: %d\n", aReceiveAcceptMsg->theMaxBlockSize);
}

static void handlePutBlock(uint64_t length, uint8_t *dataBlock, bool isLastBlock)
{
    uint64_t len = fwrite(dataBlock + 1, 1, length - 1, DestFile); // skip the one byte block counter
    if (len != length - 1)
    {
        printf("ERROR: failed to write file\n");
        exit(EXIT_FAILURE);
    }
    if (isLastBlock)
        fclose(DestFile);
}

static void handleGetBlock(void *aAppState, uint64_t *pLength, uint8_t **aDataBlock, bool *isLastBlock)
{
    if (SrcFile  == NULL)
    {
        SrcFile = fopen(RequestedFileName, "r");
        if (SrcFile == NULL)
        {
            printf("ERROR: failed to open the file for upload\n");
            Done = true;
            return;
        }
    }

    if (blockBuf == NULL)
    {
        blockBuf = PacketBuffer::New();
        if (blockBuf == NULL)
        {
            printf("ERROR: failed to malloc new buffer\n");
            Done = true;
            return;
        }
    }

    (*pLength) = fread(blockBuf->Start(), 1, MaxBlockSize, SrcFile);
    (*aDataBlock) = blockBuf->Start();
    (*isLastBlock) = (*pLength < MaxBlockSize)?true:false;
    printf("handle get block, length=%llu\n", (unsigned long long)*pLength);
}

static void handleSendAccept(void *aAppState, SendAccept *aSendAcceptMsg)
{
  printf("received send accept message\n");
}

static void OpenDestFile()
{
    const char * filename = strrchr(RequestedFileName, '/');
    if (filename == NULL)
    {
        filename = RequestedFileName;
    }
    else
    {
        filename++; //skip over '/'
    }

    char fileDesignator[strlen(filename) + strlen(ReceivedFileLocation) + 2];
    uint8_t offset = 0;

    memcpy(fileDesignator, ReceivedFileLocation, strlen(ReceivedFileLocation));
    if (ReceivedFileLocation[strlen(ReceivedFileLocation) - 1] != '/')
    {
        // if it doesn't end with '/', add one
        fileDesignator[strlen(ReceivedFileLocation)] = '/';
        offset++;
    }
    memcpy(fileDesignator + strlen(ReceivedFileLocation) + offset, filename, strlen(filename));
    fileDesignator[strlen(ReceivedFileLocation) + offset + strlen(filename)] = '\0';

    printf("File being saved to: %s\n", fileDesignator);
    DestFile = fopen(fileDesignator, "w");
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
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
    case 'D':
        if (!ParseIPAddress(arg, DestAddr))
        {
            PrintArgError("%s: Invalid value specified for destination IP address: %s\n", progName, arg);
            return false;
        }
        break;
    case 'r':
        RequestedFileName = arg;
        break;
    case 'R':
        ReceivedFileLocation = arg;
        break;
    case 'b':
        if (!ParseInt(arg, MaxBlockSize))
        {
            PrintArgError("%s: Invalid value specified for max block size: %s\n", progName, arg);
            return false;
        }
        break;
    case 'p':
        Upload = true;
        break;
    case 'T':
        Pretest = true;
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
        if (!ParseIPAddress(p+1, DestAddr))
        {
            PrintArgError("%s: Invalid value specified for destination IP address: %s\n", progName, p+1);
            return false;
        }
    }

    if (!ParseNodeId(nodeId, DestNodeId))
    {
        PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
        return false;
    }

    return true;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    WeaveBdxClient *client = static_cast<WeaveBdxClient *>(con->AppState);
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    WEAVE_ERROR err;

    if (conErr == WEAVE_NO_ERROR)
    {
        printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

        if (Upload) {
            client->initBdxSend(true, false, false,
                                &handleSendAccept, &handleReject,
                                &handleGetBlock, &handleXferError,
                                &handleDone, &handleError);
        }
        else {
            OpenDestFile();
            client->initBdxReceive(true, &handleReceiveAccept, &handleReject,
                               &handlePutBlock, &handleXferError,
                               &handleDone, &handleError);
        }
        err = SystemLayer.StartTimer(TransferTimeout, HandleTransferTimeout, client);
        if (err != WEAVE_NO_ERROR)
        {
            printf("Inet.StartTimer failed\n");
            exit(EXIT_FAILURE);
        }
        sTransferTimerIsRunning = true;
    }
    else
    {
        printf("Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();

        if (ConnectTry < ConnectMaxTry)
        {
            err = SystemLayer.StartTimer(ConnectInterval, InitiateConnection, client);
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
    }
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    con->Close();
}

// unit tests to cover the codes that functional test failed to cover
void PreTest()
{
    SendInit sendInit;
    SendAccept sendAccept;
    ReceiveAccept receiveAccept;
    BlockQuery blockQuery;
    BlockSend blockSend;

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
}

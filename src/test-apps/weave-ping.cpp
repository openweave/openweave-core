/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file implements a command line tool, weave-ping, for the
 *      Weave Echo Profile.
 *
 *      The Weave Echo Profile implements two simple methods, in the
 *      style of ICMP ECHO REQUEST and ECHO REPLY, in which a sent
 *      payload is turned around by the responder and echoed back to
 *      the originator.
 *
 *      The weave-ping tool implements a facility for acting as either
 *      the originator or responder for the Echo Profile, with a
 *      variety of options.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <signal.h>

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>

using nl::StatusReportStr;
using namespace nl::Weave::Profiles::Security;

#define TOOL_NAME "weave-ping"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void DriveSending();
static void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static void HandleEchoResponseReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void StartClientConnection();
static void StartSecureSession();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);
static void ParseDestAddress();
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
static void HandleServiceMgrStatus(void *appState, WEAVE_ERROR anError, StatusReport *aReport);
#endif

bool Listening = false;
int32_t MaxEchoCount = -1;
int32_t EchoInterval = 1000000;
int32_t EchoLength = -1;
bool UseTCP = true;
bool Debug = false;
uint64_t DestNodeId;
const char *DestAddr = NULL;
IPAddress DestIPAddr; // only used for UDP
uint16_t DestPort; // only used for UDP
InterfaceId DestIntf = INET_NULL_INTERFACEID; // only used for UDP
uint64_t LastEchoTime = 0;
bool WaitingForEchoResp = false;
int64_t EchoCount = 0;
int64_t EchoRespCount = 0;
WeaveEchoClient EchoClient;
WeaveEchoServer EchoServer;
WeaveConnection *Con = NULL;
bool ClientConInProgress = false;
bool ClientConEstablished = false;
bool ClientSecureSessionInProgress = false;
bool ClientSecureSessionEstablished = false;
WeaveAuthMode AuthMode = kWeaveAuthMode_Unauthenticated;

// The server should not reply a StatusReport with kStatus_Busy for more than 30 seconds.
// See WeaveSecurityManager::StartSessionTimer()
uint32_t SenderBusyRespCount = 0;
const uint32_t MaxSenderBusyRespCount = 10;
// In case of SenderBusy, wait 10 seconds before trying again to establish a secure session
const uint64_t SenderBusyRespDelay = 10*nl::kMicrosecondsPerSecond;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
bool UseServiceDir = false;
ServiceDirectory::WeaveServiceManager ServiceMgr;
uint8_t ServiceDirCache[300];
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
bool UseWRMP = false;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

enum NameResolutionStateEnum
{
    kNameResolutionState_NotStarted,
    kNameResolutionState_InProgress,
    kNameResolutionState_Complete
} NameResolutionState = kNameResolutionState_NotStarted;

enum
{
    kToolOpt_UseServiceDir                          = 1000,
};

static OptionDef gToolOptionDefs[] =
{
    { "listen",       kNoArgument,       'L' },
    { "dest-addr",    kArgumentRequired, 'D' },
    { "count",        kArgumentRequired, 'c' },
    { "length",       kArgumentRequired, 'l' },
    { "interval",     kArgumentRequired, 'i' },
    { "tcp",          kNoArgument,       't' },
    { "udp",          kNoArgument,       'u' },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "wrmp",         kNoArgument,       'w' },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "service-dir",  kNoArgument,       kToolOpt_UseServiceDir },
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { NULL }
};

static const char *const gToolOptionHelp =
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
    "  -L, --listen\n"
    "       Listen and respond to Echo Requests sent from another node.\n"
    "\n"
    "  -c, --count <num>\n"
    "       Send the specified number of Echo Requests and exit.\n"
    "\n"
    "  -l, --length <num>\n"
    "       Send Echo Requests with the specified number of bytes in the payload.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send Echo Requests at the specified interval in milliseconds.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send Echo Requests. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to send Echo Requests.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    "  -w, --wrmp\n"
    "       Use UDP with Weave reliable messaging to send Echo requests.\n"
    "\n"
#endif
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    "  --service-dir\n"
    "       Use service directory to lookup the destination node address.\n"
    "\n"
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>[:<dest-port>][%<interface>]]\n"
    "       " TOOL_NAME " [<options...>] --listen\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Send and receive Weave Echo profile messages.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWRMPOptions,
    &gWeaveSecurityMode,
    &gCASEOptions,
    &gTAKEOptions,
    &gGroupKeyEncOptions,
    &gDeviceDescOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    &gGeneralSecurityOptions,
    NULL
};


#if WEAVE_CONFIG_TEST
static void ResetTestContext(void)
{
    Done = false;
    WaitingForEchoResp = false;
    EchoCount = 0;
    EchoRespCount = 0;
    SenderBusyRespCount = 0;
}
#endif // WEAVE_CONFIG_TEST

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
#if WEAVE_CONFIG_TEST
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;
    uint64_t lastListeningPrintTimeMs = 0;
    uint32_t iter;
#endif

    InitToolCommon();

#if WEAVE_CONFIG_TEST
    SetupFaultInjectionContext(argc, argv);
    SetSignalHandler(DoneOnHandleSIGUSR1);
#endif

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

    if (WeaveSecurityMode::kGroupEnc == gWeaveSecurityMode.SecurityMode && gGroupKeyEncOptions.GetEncKeyId() == WeaveKeyId::kNone)
    {
        PrintArgError("%s: Please specify a group encryption key id using the --group-enc-... options.\n", TOOL_NAME);
        exit(EXIT_FAILURE);
    }

    // TODO (arg clean up): generalize code that infers node ids from local address
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

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    err = ServiceMgr.init(&ExchangeMgr, ServiceDirCache, sizeof(ServiceDirCache),
                          GetRootServiceDirectoryEntry, kWeaveAuthMode_CASE_ServiceEndPoint,
                          NULL, NULL, OverrideServiceConnectArguments);
    if (err != WEAVE_NO_ERROR)
    {
        printf("ServiceMgr.init() failed with error: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }
#endif

#if WEAVE_CONFIG_TEST
    nl::Weave::Stats::UpdateSnapshot(before);
#endif

    // Arrange to get called for various activities in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    if (!Listening)
    {
        // Initialize the EchoClient application.
        err = EchoClient.Init(&ExchangeMgr);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WeaveEchoClient.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Arrange to get a callback whenever an Echo Response is received.
        EchoClient.OnEchoResponseReceived = HandleEchoResponseReceived;

        if (!UseTCP && (WeaveSecurityMode::kPASE == gWeaveSecurityMode.SecurityMode || WeaveSecurityMode::kTAKE == gWeaveSecurityMode.SecurityMode))
        {
            printf("PASE/TAKE not supported for UDP.\n");
            exit(EXIT_FAILURE);
        }

#if !WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (!UseTCP && (WeaveSecurityMode::kCASE == gWeaveSecurityMode.SecurityMode ||
                        WeaveSecurityMode::kCASEShared == gWeaveSecurityMode.SecurityMode))
        {
            printf("CASE not supported for UDP without WRMP support.\n");
            exit(EXIT_FAILURE);
        }
#endif

        if (WeaveSecurityMode::kPASE == gWeaveSecurityMode.SecurityMode)
            AuthMode = kWeaveAuthMode_PASE_PairingCode;
        else if (WeaveSecurityMode::kCASE == gWeaveSecurityMode.SecurityMode ||
                 WeaveSecurityMode::kCASEShared == gWeaveSecurityMode.SecurityMode)
            AuthMode = kWeaveAuthMode_CASE_AnyCert;
        else if (WeaveSecurityMode::kTAKE == gWeaveSecurityMode.SecurityMode)
            AuthMode = kWeaveAuthMode_TAKE_IdentificationKey;
        else
            AuthMode = kWeaveAuthMode_Unauthenticated;
    }
    else
    {
        // Initialize the EchoServer application.
        err = EchoServer.Init(&ExchangeMgr);
        if (err)
        {
            printf("WeaveEchoServer.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Arrange to get a callback whenever an Echo Request is received.
        EchoServer.OnEchoRequestReceived = HandleEchoRequestReceived;

        SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
        SecurityMgr.OnSessionError = HandleSecureSessionError;
    }

    PrintNodeConfig();

    if (!Listening)
    {
        if (!UseTCP && DestAddr != NULL)
            ParseDestAddress();

        if (DestNodeId == 0)
            printf("Sending Echo requests to node at %s\n", DestAddr);
        else if (DestAddr == NULL)
            printf("Sending Echo requests to node %" PRIX64 "\n", DestNodeId);
        else
            printf("Sending Echo requests to node %" PRIX64 " at %s\n", DestNodeId, DestAddr);
    }
    else
    {
        printf("Listening for Echo requests...\n");
    }

#if WEAVE_CONFIG_TEST
    for (iter = 0; iter < gFaultInjectionOptions.TestIterations; iter++)
    {
        printf("Iteration %u\n", iter);
#endif // WEAVE_CONFIG_TEST

        while (!Done)
        {
            struct timeval sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_usec = 100000;

            ServiceNetwork(sleepTime);

            if (!Listening && !Done)
                DriveSending();

#if WEAVE_CONFIG_TEST
            if (Listening)
            {
                uint64_t nowMs = NowMs();

                if (nowMs - lastListeningPrintTimeMs > gGeneralSecurityOptions.GetIdleSessionTimeout())
                {
                    // Print something to show progress to the harness.
                    // The harness gives enough time to the listening node for the
                    // idle session timer to expire twice and remove idle keys; the harness
                    // needs the node to log something regularly to measure the
                    // time elapsed by parsing the timestamps, since the tests can
                    // be run at faster than real time.
                    // TODO (WEAV-2199) mark this log line as special
                    WeaveLogProgress(Echo, "Listening...");
                    lastListeningPrintTimeMs = nowMs;
                }
            }
#endif

            fflush(stdout);
        }

#if WEAVE_CONFIG_TEST
        if (!Listening && EchoCount == MaxEchoCount && EchoCount == EchoRespCount)
        {
            printf("The ping test was successful, no more iterations needed\n");
            break;
        }

        ResetTestContext();

        if (gSigusr1Received)
        {
            printf("Sigusr1Received\n");
            break;
        }
    }
#endif // WEAVE_CONFIG_TEST

    EchoClient.Shutdown();
    EchoServer.Shutdown();

#if WEAVE_CONFIG_TEST
    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();
#endif // WEAVE_CONFIG_TEST

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
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
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    case 'w':
        UseTCP = false;
        UseWRMP = true;
        break;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kToolOpt_UseServiceDir:
        UseServiceDir = true;
        break;
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case 'L':
        Listening = true;
        break;
    case 'c':
        if (!ParseInt(arg, MaxEchoCount) || MaxEchoCount < 0)
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'l':
        if (!ParseInt(arg, EchoLength) || EchoLength < 0 || EchoLength > UINT16_MAX)
        {
            PrintArgError("%s: Invalid value specified for data length: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, EchoInterval) || EchoInterval < 0 || EchoInterval > (INT32_MAX / 1000))
        {
            PrintArgError("%s: Invalid value specified for send interval: %s\n", progName, arg);
            return false;
        }
        EchoInterval = EchoInterval * 1000;
        break;
    case 'D':
        DestAddr = arg;
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

        // TODO (arg clean up): generalize parsing of destination node ids and addresses.

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

void DriveSending()
{
    WEAVE_ERROR err;

    if (Now() < LastEchoTime + EchoInterval)
        return;

    if (WaitingForEchoResp)
    {
        printf("No response received\n");

        WaitingForEchoResp = false;

        // Rescan interfaces to see if we got any new IP addresses
        if (!UseTCP)
        {
            printf("Refreshing endpoints\n");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
                printf("WeaveMessageLayer.RefreshEndpoints() failed: %s\n", ErrorStr(err));
        }
    }

    if (MaxEchoCount != -1 && EchoCount >= MaxEchoCount)
    {
        if (Con != NULL)
        {
            printf("Connection closed\n");
            Con->Close();
            Con = NULL;
            ClientConEstablished = false;
            ClientConInProgress = false;
        }

        Done = true;
        return;
    }

    if (UseTCP)
    {
        if (!ClientConEstablished)
        {
            StartClientConnection();
            return;
        }
    }
    else if (WeaveSecurityMode::kPASE == gWeaveSecurityMode.SecurityMode ||
             WeaveSecurityMode::kCASE == gWeaveSecurityMode.SecurityMode ||
             WeaveSecurityMode::kCASEShared == gWeaveSecurityMode.SecurityMode)
    {
        if (!ClientSecureSessionEstablished)
        {
            StartSecureSession();
            return;
        }
    }

    PacketBuffer *payloadBuf = PacketBuffer::New();
    if (payloadBuf == NULL)
    {
        printf("Unable to allocate PacketBuffer\n");
        LastEchoTime = Now();
        return;
    }

    char *p = (char *) payloadBuf->Start();
    int32_t len = sprintf(p, "Echo Message %" PRIi64 "\n", EchoCount);

    if (EchoLength > payloadBuf->MaxDataLength())
        EchoLength = payloadBuf->MaxDataLength();

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
    payloadBuf->SetDataLength((uint16_t) len);

    LastEchoTime = Now();

    if (UseTCP)
    {
        VerifyOrDie(Con != NULL && ClientConEstablished);
    }
    else if (WeaveSecurityMode::kCASE == gWeaveSecurityMode.SecurityMode || WeaveSecurityMode::kPASE == gWeaveSecurityMode.SecurityMode)
    {
        VerifyOrDie(ClientSecureSessionEstablished);
    }

    if (Con != NULL)
    {
        err = EchoClient.SendEchoRequest(Con, payloadBuf);
        payloadBuf = NULL;
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        EchoClient.SetRequestAck(UseWRMP);
        EchoClient.SetWRMPACKDelay(gWRMPOptions.ACKDelay);
        EchoClient.SetWRMPRetransInterval(gWRMPOptions.RetransInterval);
        EchoClient.SetWRMPRetransCount(gWRMPOptions.RetransCount);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

        if (WeaveSecurityMode::kGroupEnc == gWeaveSecurityMode.SecurityMode)
        {
            EchoClient.EncryptionType = kWeaveEncryptionType_AES128CTRSHA1;
            EchoClient.KeyId = gGroupKeyEncOptions.GetEncKeyId();
        }

        err = EchoClient.SendEchoRequest(DestNodeId, DestIPAddr, DestPort, DestIntf, payloadBuf);
        payloadBuf = NULL;
    }

    if (err == WEAVE_NO_ERROR)
    {
        WaitingForEchoResp = true;
        EchoCount++;
    }
    else
    {
        printf("WeaveEchoClient.SendEchoRequest() failed: %s\n", ErrorStr(err));
        if (err == WEAVE_ERROR_KEY_NOT_FOUND)
        {
            ClientSecureSessionEstablished = false;
        }
    }
}

void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    if (Listening)
    {
        char ipAddrStr[64];
        nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        printf("Echo Request from node %" PRIX64 " (%s): len=%u ... sending response.\n", nodeId, ipAddrStr,
                payload->DataLength());

        if (Debug)
            DumpMemory(payload->Start(), payload->DataLength(), "    ", 16);
    }
}

void HandleEchoResponseReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    uint32_t respTime = Now();
    uint32_t transitTime = respTime - LastEchoTime;

    WaitingForEchoResp = false;
    EchoRespCount++;

    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Echo Response from node %" PRIX64 " (%s): %" PRIi64 "/%" PRIi64 "(%.2f%%) len=%u time=%.3fms\n", nodeId, ipAddrStr,
            EchoRespCount, EchoCount, ((double) EchoRespCount) * 100 / EchoCount, payload->DataLength(),
            ((double) transitTime) / 1000);

    if (Debug)
        DumpMemory(payload->Start(), payload->DataLength(), "    ", 16);
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void StartSecureSession()
{
    WEAVE_ERROR err;
    IPAddress coreRouterAddress;

    // Do nothing if a secure session attempt is already in progress.
    if (ClientSecureSessionInProgress)
    {
        return;
    }

    ClientSecureSessionEstablished = false;

    // Set the InProgress flag to true now, because
    // StartSecureSession can invoke HandleSecureSessionError, which clears
    // the InProgress flag.
    ClientSecureSessionInProgress = true;

    switch (gWeaveSecurityMode.SecurityMode)
    {
        case WeaveSecurityMode::kPASE:
            err = SecurityMgr.StartPASESession(Con, AuthMode, NULL,
                                               HandleSecureSessionEstablished, HandleSecureSessionError);
            break;
        case WeaveSecurityMode::kCASE:
            err = SecurityMgr.StartCASESession(Con, DestNodeId, DestIPAddr, WEAVE_PORT, AuthMode,
                                               NULL, HandleSecureSessionEstablished, HandleSecureSessionError);
            break;
        case WeaveSecurityMode::kCASEShared:
            coreRouterAddress = IPAddress::MakeULA(WeaveFabricIdToIPv6GlobalId(FabricState.FabricId),
                                                   nl::Weave::kWeaveSubnetId_Service,
                                                   nl::Weave::WeaveNodeIdToIPv6InterfaceId(kServiceEndpoint_CoreRouter));

            err = SecurityMgr.StartCASESession(Con, DestNodeId, coreRouterAddress, WEAVE_PORT, AuthMode,
                                               NULL, HandleSecureSessionEstablished, HandleSecureSessionError,
                                               NULL, kServiceEndpoint_CoreRouter);
            break;
        default:
            err = WEAVE_ERROR_UNSUPPORTED_AUTH_MODE;
    }

    if (err != WEAVE_NO_ERROR)
    {
        printf("SecurityMgr.StartSecureSession() failed: %s\n", ErrorStr(err));
        LastEchoTime = Now();
        ClientSecureSessionInProgress = false;
        return;
    }
}

void StartClientConnection()
{
    WEAVE_ERROR err;

    if (Con != NULL && Con->State == WeaveConnection::kState_Closed)
    {
        Con->Close();
        Con = NULL;
    }

    // Do nothing if a connect attempt is already in progress.
    if (ClientConInProgress)
        return;

    ClientConEstablished = false;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (UseServiceDir)
    {
        err = ServiceMgr.connect(DestNodeId,
                                 AuthMode,
                                 NULL,
                                 HandleServiceMgrStatus,
                                 HandleConnectionComplete);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WeaveServiceManager.Connect(): failed: %s\n", ErrorStr(err));
            LastEchoTime = Now();
            return;
        }
    }
    else
#endif
    {
        Con = MessageLayer.NewConnection();
        if (Con == NULL)
        {
            printf("WeaveConnection.Connect failed: %s\n", ErrorStr(WEAVE_ERROR_NO_MEMORY));
            LastEchoTime = Now();
            Done = true;
            return;
        }
        Con->OnConnectionComplete = HandleConnectionComplete;
        Con->OnConnectionClosed = HandleConnectionClosed;

        err = Con->Connect(DestNodeId, AuthMode, DestAddr);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WeaveConnection.Connect failed: %s\n", ErrorStr(err));
            Con->Close();
            Con = NULL;
            LastEchoTime = Now();
            Done = true;
            return;
        }
    }

    ClientConInProgress = true;
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection FAILED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));
        con->Close();
        Con = NULL;
        LastEchoTime = Now();
        ClientConEstablished = false;
        ClientConInProgress = false;
        Done = true;
        return;
    }

    printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    Con = con;

    con->OnConnectionClosed = HandleConnectionClosed;

    EchoClient.EncryptionType = con->DefaultEncryptionType;
    EchoClient.KeyId = con->DefaultKeyId;

    ClientConEstablished = true;
    ClientConInProgress = false;
}

void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (conErr == WEAVE_NO_ERROR)
        printf("Connection closed to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
    else
        printf("Connection ABORTED to node %" PRIX64 " (%s): %s\n", con->PeerNodeId, ipAddrStr, ErrorStr(conErr));

    if (Listening)
        con->Close();
    else if (con == Con)
    {
        con->Close();
        Con = NULL;
    }

    WaitingForEchoResp = false;
    ClientConEstablished = false;
    ClientConInProgress = false;
}

void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64];

    if (con != NULL)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    else
    {
        DestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        EchoClient.EncryptionType = encType;
        EchoClient.KeyId = sessionKeyId;

        ClientSecureSessionEstablished = true;
        ClientSecureSessionInProgress = false;
    }
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    printf("Secure session established with node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport)
{
    char ipAddrStr[64];
    bool isSenderBusy = false;

    if (con != NULL)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    else
    {
        DestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        ClientSecureSessionInProgress = false;
        ClientSecureSessionEstablished = false;
    }
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        printf("FAILED to establish secure session to node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        printf("FAILED to establish secure session to node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, ErrorStr(localErr));

    isSenderBusy = (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED &&
                    statusReport != NULL &&
                    statusReport->mProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
                    statusReport->mStatusCode == nl::Weave::Profiles::Common::kStatus_Busy);

    if (isSenderBusy)
    {
        // Force the main loop not to retry too soon
        LastEchoTime = Now() + SenderBusyRespDelay;
        SenderBusyRespCount++;
    }

    if (!Listening && (!isSenderBusy || SenderBusyRespCount > MaxSenderBusyRespCount))
    {
        Done = true;
    }
}

void ParseDestAddress()
{
    // NOTE: This function is only used when communicating over UDP.  Code in the WeaveConnection object handles
    // parsing the destination node address for TCP connections.

    WEAVE_ERROR err;
    const char *addr;
    uint16_t addrLen = 0;
    const char *intfName;
    uint16_t intfNameLen;

    err = ParseHostPortAndInterface(DestAddr, strlen(DestAddr), addr, addrLen, DestPort, intfName, intfNameLen);
    if (err != INET_NO_ERROR)
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (!IPAddress::FromString(addr, addrLen, DestIPAddr))
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

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
void HandleServiceMgrStatus(void* anAppState, WEAVE_ERROR anError, StatusReport *aReport)
{
    if (aReport)
        printf("service directory status report [%" PRIx32 ", %" PRIx32 "]", aReport->mProfileId, aReport->mStatusCode);

    else
        printf("service directory error %" PRIx32 "", static_cast<uint32_t>(anError));

    LastEchoTime = Now();
    ClientConEstablished = false;
    ClientConInProgress = false;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

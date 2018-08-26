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
 *      This file implements a command line tool, weave-key-export, for the
 *      Weave Key Export Protocol (Security Profile).
 *
 *      The weave-kye-export tool implements a facility for acting as either
 *      the originator or responder for the key export request, with a
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
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>

using nl::StatusReportStr;
using namespace nl::Weave::Profiles::Security;

#define TOOL_NAME "weave-key-export"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void DriveSending();
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void StartClientConnection();
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleKeyExportComplete(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint32_t exportedKeyId, const uint8_t *exportedKey, uint16_t exportedKeyLen);
static void HandleKeyExportError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, StatusReport *statusReport);
static void ParseDestAddress();
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
static void HandleServiceMgrStatus(void *appState, WEAVE_ERROR anError, StatusReport *aReport);
#endif

int32_t MaxKeyExportCount = -1;
int32_t KeyExportInterval = 1000000;
bool UseTCP = true;
bool Debug = false;
bool SignKeyExportMsgs = true;
uint32_t ExportKeyId = WeaveKeyId::kClientRootKey;
uint64_t DestNodeId;
const char *DestAddr = NULL;
IPAddress DestIPAddr; // only used for UDP
uint16_t DestPort = WEAVE_PORT; // only used for UDP
InterfaceId DestIntf = INET_NULL_INTERFACEID; // only used for UDP
uint64_t LastKeyExportTime = 0;
bool WaitingForKeyExportResponse = false;
uint64_t KeyExportRequestCount = 0;
uint64_t KeyExportResponseCount = 0;
WeaveConnection *Con = NULL;
bool ClientConInProgress = false;
bool ClientConEstablished = false;
WeaveAuthMode AuthMode = kWeaveAuthMode_Unauthenticated;
uint8_t InitiatorKeyExportConfig = kKeyExportConfig_Unspecified;

uint32_t SenderBusyRespCount = 0;
const uint32_t MaxSenderBusyRespCount = 10;
// In case of SenderBusy, wait 10 seconds before trying again to establish a secure session
const uint64_t SenderBusyRespDelay = 10 * nl::kMicrosecondsPerSecond;

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

bool ParseKeyExportConfig(const char *str, uint8_t& output)
{
    uint32_t configNum;

    if (!ParseInt(str, configNum))
        return false;

    switch (configNum)
    {
    case 1:
        output = kKeyExportConfig_Config1;
        return true;
    case 2:
        output = kKeyExportConfig_Config2;
        return true;
    default:
        return false;
    }
}

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",         kArgumentRequired, 'D' },
    { "key-id",            kArgumentRequired, 'K' },
    { "key-export-config", kArgumentRequired, 'k' },
    { "dont-sign-msgs",    kNoArgument,       'd' },
    { "count",             kArgumentRequired, 'c' },
    { "interval",          kArgumentRequired, 'i' },
    { "tcp",               kNoArgument,       't' },
    { "udp",               kNoArgument,       'u' },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "wrmp",              kNoArgument,       'w' },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "service-dir",       kNoArgument,       kToolOpt_UseServiceDir },
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>][%<interface>]\n"
    "       Send Key Export Requests to a specific address rather than one\n"
    "       derived from the destination node id. <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address. If <port> is specified, Key Export\n"
    "       Requests will be sent to the specified port. If <interface> is\n"
    "       specified, Key Export Requests will be sent over the specified local\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -K, --key-id <num>\n"
    "       Identifier of the key to be exported. If not specified the client\n"
    "       root key is exported by default.\n"
    "\n"
    "  -k, --key-export-config <num>\n"
    "       Proposed the specified key export configuration when initiating a key\n"
    "       export request. If not specified the dafult value provided by\n"
    "       WeaveSecurityManager is used.\n"
    "\n"
    "  -d, --dont-sign-msgs\n"
    "       Don't sign Key Export Request/Response messages. If not specified,\n"
    "       by default the messages are signed with ECDSA signature using device\n"
    "       private key.\n"
    "\n"
    "  -c, --count <num>\n"
    "       Send the specified number of Key Export Requests and exit.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send Key Export Requests at the specified interval in milliseconds.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send Key Export Requests. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to send Key Export Requests.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    "  -w, --wrmp\n"
    "       Use UDP with Weave reliable messaging to send Key Export Requests.\n"
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
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Send key export request and receive key export response messages.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWRMPOptions,
    &gCASEOptions,
    &gKeyExportOptions,
    &gDeviceDescOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void ResetTestContext(void)
{
    Done = false;
    WaitingForKeyExportResponse = false;
    KeyExportRequestCount = 0;
    KeyExportResponseCount = 0;
    SenderBusyRespCount = 0;
}

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;
    uint32_t iter;

#if WEAVE_CONFIG_TEST
    SetupFaultInjectionContext(argc, argv);
    SetSignalHandler(DoneOnHandleSIGUSR1);
#endif

    {
        unsigned int seed;
        err = nl::Weave::Platform::Security::GetSecureRandomData((uint8_t *)&seed, sizeof(seed));
        FAIL_ERROR(err, "Random number generator seeding failed");
        srand(seed);
    }

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

    InitWeaveStack(!UseTCP, true);

    if (InitiatorKeyExportConfig != kKeyExportConfig_Unspecified)
        SecurityMgr.InitiatorKeyExportConfig = InitiatorKeyExportConfig;

    if (gKeyExportOptions.AllowedKeyExportConfigs != 0)
        SecurityMgr.InitiatorAllowedKeyExportConfigs = gKeyExportOptions.AllowedKeyExportConfigs;

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

    PrintNodeConfig();

    if (!UseTCP && DestAddr != NULL)
        ParseDestAddress();

    if (DestNodeId == 0)
        printf("Sending key export request to node at %s\n", DestAddr);
    else if (DestAddr == NULL)
        printf("Sending key export request to node %" PRIX64 "\n", DestNodeId);
    else
        printf("Sending key export request to node %" PRIX64 " at %s\n", DestNodeId, DestAddr);

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

            if (!Done)
                DriveSending();

            fflush(stdout);
        }

        ResetTestContext();

#if WEAVE_CONFIG_TEST
        if (gSigusr1Received)
        {
            printf("Sigusr1Received\n");
            break;
        }
    }
#endif // WEAVE_CONFIG_TEST

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
    case 'K':
        if (!ParseInt(arg, ExportKeyId, 0) || !WeaveKeyId::IsValidKeyId(ExportKeyId))
        {
            PrintArgError("%s: Invalid value specified for key identifier: %s\n", progName, arg);
            return false;
        }
        break;
    case 'k':
        if (!ParseKeyExportConfig(arg, InitiatorKeyExportConfig))
        {
            PrintArgError("%s: Invalid value specified for KeyExport config: %s\n", progName, arg);
            return false;
        }
        break;
    case 'd':
        SignKeyExportMsgs = false;
        break;
    case 'c':
        if (!ParseInt(arg, MaxKeyExportCount) || MaxKeyExportCount < 0)
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, KeyExportInterval) || KeyExportInterval < 0 || KeyExportInterval > (INT32_MAX / 1000))
        {
            PrintArgError("%s: Invalid value specified for send interval: %s\n", progName, arg);
            return false;
        }
        KeyExportInterval = KeyExportInterval * 1000;
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
            PrintArgError("%s: Invalid value specified for destination node Id: %s\n", progName, nodeId);
            return false;
        }
    }
    else
    {
        PrintArgError("%s: Please specify destination node Id\n", progName);
        return false;
    }

    return true;
}

void DriveSending()
{
    WEAVE_ERROR err;

    if (Now() < LastKeyExportTime + KeyExportInterval)
        return;

    if (WaitingForKeyExportResponse)
    {
        printf("No key export response received\n");

        WaitingForKeyExportResponse = false;

        // Rescan interfaces to see if we got any new IP addresses
        if (!UseTCP)
        {
            printf("Refreshing endpoints\n");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
                printf("WeaveMessageLayer.RefreshEndpoints() failed: %s\n", ErrorStr(err));
        }
    }

    if (MaxKeyExportCount != -1 && KeyExportRequestCount >= (uint64_t) MaxKeyExportCount)
    {
        if (Con != NULL && Con->State != WeaveConnection::kState_Closed)
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
        else
        {
            VerifyOrDie(Con != NULL && ClientConEstablished);
        }
    }

    err = SecurityMgr.StartKeyExport(Con, DestNodeId, DestIPAddr, DestPort, ExportKeyId, SignKeyExportMsgs,
                                     NULL, HandleKeyExportComplete, HandleKeyExportError);
    LastKeyExportTime = Now();
    if (err == WEAVE_NO_ERROR)
    {
        KeyExportRequestCount++;
        WaitingForKeyExportResponse = true;
    }
    else
    {
        printf("SecurityMgr.StartKeyExport() failed: %s\n", ErrorStr(err));
    }
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
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
            LastKeyExportTime = Now();
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
            LastKeyExportTime = Now();
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
            LastKeyExportTime = Now();
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
        LastKeyExportTime = Now();
        ClientConEstablished = false;
        ClientConInProgress = false;
        Done = true;
        return;
    }

    printf("Connection established to node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    Con = con;

    con->OnConnectionClosed = HandleConnectionClosed;

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

    if (con == Con)
    {
        con->Close();
        Con = NULL;
    }

    WaitingForKeyExportResponse = false;
    ClientConEstablished = false;
    ClientConInProgress = false;
}

void HandleKeyExportComplete(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint32_t exportedKeyId, const uint8_t *exportedKey, uint16_t exportedKeyLen)
{
    uint64_t peerNodeId;
    char ipAddrStr[64];

    WaitingForKeyExportResponse = false;
    KeyExportResponseCount++;
    LastKeyExportTime = Now();
    SenderBusyRespCount = 0;

    if (con != NULL)
    {
        peerNodeId = con->PeerNodeId;
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }
    else
    {
        peerNodeId = DestNodeId;
        DestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }

    printf("Received Key Export Response from node %" PRIX64 " (%s) for requested keyId = 0x%08X.\n", peerNodeId, ipAddrStr, ExportKeyId);
    printf("Exported Key 0x%08X (%d bytes):\n", exportedKeyId, exportedKeyLen);
    DumpMemoryCStyle(exportedKey, exportedKeyLen, "  ", 16);
}

void HandleKeyExportError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, StatusReport *statusReport)
{
    uint64_t peerNodeId;
    char ipAddrStr[64];
    bool isSenderBusy = false;

    WaitingForKeyExportResponse = false;

    if (con != NULL)
    {
        peerNodeId = con->PeerNodeId;
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }
    else
    {
        peerNodeId = DestNodeId;
        DestIPAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    }

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        printf("FAILED to export key (keyId = 0x%08X) from node %" PRIX64 " (%s): %s\n", ExportKeyId, peerNodeId, ipAddrStr, StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        printf("FAILED to export key (keyId = 0x%08X) from node %" PRIX64 " (%s): %s\n", ExportKeyId, peerNodeId, ipAddrStr, ErrorStr(localErr));

    isSenderBusy = (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED &&
                    statusReport != NULL &&
                    statusReport->mProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
                    statusReport->mStatusCode == nl::Weave::Profiles::Common::kStatus_Busy);

    if (isSenderBusy)
    {
        // Force the main loop not to retry too soon.
        LastKeyExportTime = Now() + SenderBusyRespDelay;
        SenderBusyRespCount++;
    }

    if (!isSenderBusy || SenderBusyRespCount > MaxSenderBusyRespCount)
    {
        if (Con != NULL && Con->State != WeaveConnection::kState_Closed)
        {
            printf("Connection closed\n");
            Con->Close();
            Con = NULL;
            ClientConEstablished = false;
            ClientConInProgress = false;
        }
        Done = true;
    }
}

void ParseDestAddress()
{
    // NOTE: This function is only used when communicating over UDP.  Code in the WeaveConnection object handles
    // parsing the destination node address for TCP connections.

    WEAVE_ERROR err;
    const char *addr;
    uint16_t addrLen;
    const char *intfName;
    uint16_t intfNameLen;

    err = ParseHostPortAndInterface(DestAddr, strlen(DestAddr), addr, addrLen, DestPort, intfName, intfNameLen);
    if (err != INET_NO_ERROR)
    {
        printf("Invalid destination address: %s\n", DestAddr);
        exit(EXIT_FAILURE);
    }

    if (!IPAddress::FromString(addr, DestIPAddr))
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

    LastKeyExportTime = Now();
    ClientConEstablished = false;
    ClientConInProgress = false;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements a command line tool, weave-cert-prov-client, for the
 *      Weave Certificate Provisioning Protocol (Security Profile).
 *
 *      The weave-cert-prov-client tool implements a facility for acting as a client
 *      (originator) for the certificate provisioning request, with a variety of options.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <signal.h>

#include "ToolCommon.h"
#include "CertProvOptions.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>

using nl::StatusReportStr;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;

#define TOOL_NAME "weave-cert-prov-client"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void DriveSending();
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void ParseDestAddress();
static void BindingEventHandler(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam);

const nl::Weave::ExchangeContext::Timeout kResponseTimeoutMsec = 5000;

uint32_t MaxGetCertCount = UINT32_MAX;
uint32_t GetCertInterval = 5000; // 5 sec
bool UseTCP = true;
bool Debug = false;
uint64_t DestNodeId;
const char *DestAddr = NULL;
IPAddress DestIPAddr;
uint16_t DestPort = WEAVE_PORT;
InterfaceId DestIntf = INET_NULL_INTERFACEID;
uint64_t LastGetCertTime = 0;
bool WaitingForGetCertResponse = false;
uint32_t GetCertRequestCount = 0;
uint32_t GetCertResponseCount = 0;

WeaveCertProvEngine  CertProvClient;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
bool UseWRMP = false;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",         kArgumentRequired, 'D' },
    { "count",             kArgumentRequired, 'c' },
    { "interval",          kArgumentRequired, 'i' },
    { "tcp",               kNoArgument,       't' },
    { "udp",               kNoArgument,       'u' },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "wrmp",              kNoArgument,       'w' },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>][%<interface>]\n"
    "       Send Get Certificate Requests to a specific address rather than one\n"
    "       derived from the destination node id. <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address. If <port> is specified, Get Certificate\n"
    "       Requests will be sent to the specified port. If <interface> is\n"
    "       specified, Get Certificate Requests will be sent over the specified local\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -c, --count <num>\n"
    "       Send the specified number of Get Certificate Requests and exit.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send Get Certificate Requests at the specified interval in milliseconds.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to send Get Certificate Requests. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to send Get Certificate Requests.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    "  -w, --wrmp\n"
    "       Use UDP with Weave reliable messaging to send Get Certificate Requests.\n"
    "\n"
#endif
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
    &gCertProvOptions,
    &gDeviceDescOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void ResetTestContext(void)
{
    Done = false;
    WaitingForGetCertResponse = false;
    GetCertRequestCount = 0;
    GetCertResponseCount = 0;
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
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs) ||
        !ResolveWeaveNetworkOptions(TOOL_NAME, gWeaveNodeOptions, gNetworkOptions))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(!UseTCP, true);

    // Create a binding for the CertProvClient.
    Binding *binding = ExchangeMgr.NewBinding(BindingEventHandler, NULL);

    // Initialize the CertProvClient object.
    err = CertProvClient.Init(binding, &gCertProvOptions, &gCertProvOptions, CertProvClientEventHandler, NULL);
    FAIL_ERROR(err, "WeaveCertProvEngine.Init failed");

    // Release the binding (the CertProvClient retains its own reference).
    binding->Release();

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

    printf("Sending");
    if (MaxGetCertCount != UINT32_MAX)
        printf(" %u", MaxGetCertCount);
    printf(" Get Certificate Requests via %s to node %" PRIX64, UseTCP ? "TCP" : (UseWRMP ? "UDP with WRMP" : "UDP"), DestNodeId);
    if (DestAddr != NULL)
        printf(" (%s)", DestAddr);
    printf(" every %" PRId32 " ms\n", GetCertInterval);

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

    CertProvClient.Shutdown();

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
    case 'c':
        if (!ParseInt(arg, MaxGetCertCount))
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, GetCertInterval))
        {
            PrintArgError("%s: Invalid value specified for send interval: %s\n", progName, arg);
            return false;
        }
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

    if (Now() < LastGetCertTime + GetCertInterval)
        return;

    if (WaitingForGetCertResponse)
    {
        printf("No get certificate response received\n");

        WaitingForGetCertResponse = false;

        // Rescan interfaces to see if we got any new IP addresses
        if (!UseTCP)
        {
            printf("Refreshing endpoints\n");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
                printf("WeaveMessageLayer.RefreshEndpoints() failed: %s\n", ErrorStr(err));
        }
    }

    if (MaxGetCertCount != UINT32_MAX && GetCertRequestCount >= MaxGetCertCount)
    {
        Done = true;
        return;
    }

    err = CertProvClient.StartCertificateProvisioning(gCertProvOptions.RequestType, (gCertProvOptions.RequestType == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert));
    LastGetCertTime = Now();
    if (err == WEAVE_NO_ERROR)
    {
        GetCertRequestCount++;
        WaitingForGetCertResponse = true;
    }
    else
    {
        printf("CertProvClient.StartCertificateProvisioning() failed: %s\n", ErrorStr(err));
    }
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);
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

void BindingEventHandler(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam)
{
    switch (eventType)
    {
    case Binding::kEvent_PrepareRequested:
    {
        Binding::Configuration bindingConfig = inParam.Source->BeginConfiguration();

        // Configure the target node Id.
        bindingConfig.Target_NodeId(DestNodeId);

        // Configure the target address.
        if (DestAddr != NULL)
        {
            bindingConfig.TargetAddress_IP(DestIPAddr, DestPort, DestIntf);
        }

        // Configure the transport.
        if (UseTCP)
        {
            bindingConfig.Transport_TCP();
        }
        else if (!UseWRMP)
        {
            bindingConfig.Transport_UDP();
        }
        else
        {
            bindingConfig.Transport_UDP_WRM();
            bindingConfig.Transport_DefaultWRMPConfig(gWRMPOptions.GetWRMPConfig());
        }

        // Configure the security mode.
        switch (gWeaveSecurityMode.SecurityMode)
        {
        case WeaveSecurityMode::kNone:
        default:
            bindingConfig.Security_None();
            break;
        case WeaveSecurityMode::kCASE:
            bindingConfig.Security_CASESession();
            break;
        case WeaveSecurityMode::kCASEShared:
            bindingConfig.Security_SharedCASESession();
            break;
        case WeaveSecurityMode::kGroupEnc:
            bindingConfig.Security_Key(gGroupKeyEncOptions.GetEncKeyId());
            break;
        }

        bindingConfig.Exchange_ResponseTimeoutMsec(kResponseTimeoutMsec);

        outParam.PrepareRequested.PrepareError = bindingConfig.PrepareBinding();
        break;
    }
    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
        break;
    }
}

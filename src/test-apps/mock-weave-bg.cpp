/*
 *
 *    Copyright (c) 2020 Google LLC.
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file implements the Weave Mock Weave Border Gateway.
 *
 *      This is used to instantiate a Tunnel Agent which opens a
 *      tunnel endpoint and forwards IPv6 packets between the
 *      Service connection and the tunnel endpoint.
 *
 */

#define __STDC_FORMAT_MACROS

#define DEFAULT_BG_NODE_ID 0xBADCAFE

#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#include "ToolCommon.h"
#include "CASEOptions.h"
#include <Weave/Support/logging/DecodedIPPacket.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <InetLayer/InetInterface.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>
#include "MockCPClient.h"

MockCertificateProvisioningClient MockCPClient;

#if WEAVE_CONFIG_ENABLE_TUNNELING
using namespace ::nl::Weave::Profiles::WeaveTunnel;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::DeviceDescription;
#endif

#define TOOL_NAME "mock-weave-bg"

#define DEFAULT_TFE_NODE_ID 0xC0FFEE

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);

static WeaveTunnelAgent gTunAgent;

static bool gUseCASE = false;
static bool gTunnelLogging = false;
static IPAddress gDestAddr = IPAddress::Any;
static uint16_t gDestPort = 0;
static uint64_t gDestNodeId = DEFAULT_TFE_NODE_ID;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
static bool gUseServiceDirForTunnel = false;
static ServiceDirectory::WeaveServiceManager gServiceMgr;
static uint8_t gServiceDirCache[500];
#endif

static uint8_t gRole = kClientRole_BorderGateway; //Default Value

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
static const char *gPrimaryIntf;
static const char *gBackupIntf;
static bool gEnableBackup = false;
#endif

enum
{
    kToolOpt_ConnectTo          = 1000,
    kToolOpt_UseServiceDir,
};

static OptionDef gToolOptionDefs[] =
{
    { "role",                kArgumentRequired, 'r' },
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    { "primary-intf",        kArgumentRequired, 'P' },
    { "backup-intf",         kArgumentRequired, 'B' },
    { "enable-backup",       kNoArgument,       'e' },
#endif
    { "connect-to",          kArgumentRequired, kToolOpt_ConnectTo },
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "service-dir",         kNoArgument,       kToolOpt_UseServiceDir },
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "case",                kNoArgument,       'C' },
#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    { "tunnel-log",          kNoArgument,       'l' },
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    { }
};

static const char *const gToolOptionHelp =
    "  -r, --role <num>\n"
    "       Role for local client node, i.e., 1) Border Gateway or 2) Mobile Device.\n"
    "\n"
    "  --connect-to <addr>[:<port>][%<interface>]\n"
    "       Connect to the tunnel service at the supplied address.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    "  --service-dir\n"
    "       Use service directory to lookup the address of the tunnel server.\n"
    "\n"
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    "  -C, --case\n"
    "       Use CASE to create an authenticated session with the tunnel server.\n"
    "\n"
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    "  -P, --primary-intf <interface-name>\n"
    "       Interface name for primary tunnel.\n"
    "\n"
    "  -B, --backup-intf <interface-name>\n"
    "       Interface name for backup tunnel.\n"
    "\n"
    "  -e, --enable-backup\n"
    "       Enable the use of a backup tunnel.\n"
    "\n"
#endif
#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    "  -l, --tunnel-log\n"
    "       Use detailed logging of Tunneled IP packet\n"
    "\n"
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    "";

static OptionSet gToolOptions =
{
    HandleOption,
    gToolOptionDefs,
    "GENERAL OPTIONS",
    gToolOptionHelp
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " <options>\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWRMPOptions,
    &gCASEOptions,
    &gDeviceDescOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'r':
        if (!ParseInt(arg, gRole) ||
            (gRole != kClientRole_BorderGateway && gRole != kClientRole_MobileDevice))
        {
            PrintArgError("%s: Invalid value specified for device role: %s. Possible values: (1)BorderGateway and (2)MobileDevice\n", progName, arg);
            return false;
        }
        break;
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    case 'P':
        gPrimaryIntf = arg;
        break;
    case 'B':
        gBackupIntf = arg;
        break;
    case 'e':
        gEnableBackup = true;
        break;
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    case kToolOpt_ConnectTo:
    {
        const char *host;
        uint16_t hostLen;
        if (ParseHostAndPort(arg, strlen(arg), host, hostLen, gDestPort) != WEAVE_NO_ERROR)
        {
            PrintArgError("%s: Invalid value specified for --connect-to: %s\n", progName, arg);
            return false;
        }
        char *hostCopy = strndup(host, hostLen);
        bool isValidAddr = IPAddress::FromString(hostCopy, gDestAddr);
        free(hostCopy);
        if (!isValidAddr)
        {
            PrintArgError("%s: Invalid value specified for --connect-to (expected IP address): %s\n", progName, arg);
            return false;
        }
        break;
    }
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kToolOpt_UseServiceDir:
        gUseServiceDirForTunnel = true;
        break;
#endif
    case 'C':
        gUseCASE = true;
        break;
    case 'l':
        gTunnelLogging = true;
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

        if (!ParseNodeId(argv[0], gDestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, argv[0]);
            return false;
        }
    }

    return true;
}

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
void TunneledPacketTransitHandler(const PacketBuffer &pkt, TunnelPktDirection pktDir, TunnelType tunnelType, bool &toDrop)
{
    DecodedIPPacket decodedPkt;
    char InOrOut[9];
    char tunTypeStr[9];

    // Decode the packet; skip the tunnel header and pass the IP packet

    decodedPkt.PacketHeaderDecode(pkt.Start() + TUN_HDR_SIZE_IN_BYTES, pkt.DataLength() - TUN_HDR_SIZE_IN_BYTES);

    strncpy(InOrOut, (pktDir == kDir_Outbound) ? "Outbound" : "Inbound", sizeof(InOrOut));
    strncpy(tunTypeStr, (tunnelType == kType_TunnelPrimary) ? "primary" : (tunnelType == kType_TunnelBackup) ? "backup" : "shortcut", sizeof(tunTypeStr));

    WeaveLogDetail(WeaveTunnel, "Tun: %s over %s", InOrOut, tunTypeStr);

    // Log the header fields

    LogPacket(decodedPkt, true);

    // Inject a packet drop by the application.
    WEAVE_FAULT_INJECT(FaultInjection::kFault_TunnelPacketDropByPolicy,
                       toDrop = true);

}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

#endif // WEAVE_CONFIG_ENABLE_TUNNELING

int main(int argc, char *argv[])
{
#if WEAVE_CONFIG_ENABLE_TUNNELING
    WEAVE_ERROR err;
    gWeaveNodeOptions.LocalNodeId = DEFAULT_BG_NODE_ID;
    WeaveAuthMode authMode = kWeaveAuthMode_Unauthenticated;

    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv);
    UseStdoutLineBuffering();
    SetSignalHandler(DoneOnHandleSIGUSR1);

    // Configure some alternate defaults for the device descriptor values.
    gDeviceDescOptions.BaseDeviceDesc.ProductId = nl::Weave::Profiles::Vendor::Nestlabs::DeviceDescription::kNestWeaveProduct_Onyx;
    strcpy(gDeviceDescOptions.BaseDeviceDesc.SerialNumber, "mock-weave-bg");
    strcpy(gDeviceDescOptions.BaseDeviceDesc.SoftwareVersion, "mock-weave-bg/1.0");
    gDeviceDescOptions.BaseDeviceDesc.DeviceFeatures = WeaveDeviceDescriptor::kFeature_LinePowered;

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

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDirForTunnel && gDestAddr != IPAddress::Any)
    {
        printf("ERROR: Please specify only one of --connect-to or --service-dir\n");
        exit(EXIT_FAILURE);
    }
    if (!gUseServiceDirForTunnel && gDestAddr == IPAddress::Any)
    {
        printf("ERROR: Please specify how to find the tunnel server using either --connect-to or --service-dir\n");
        exit(EXIT_FAILURE);
    }
#else
    if (gDestAddr == IPAddress::Any)
    {
        printf("ERROR: Please specify the address of the tunnel server using --connect-to\n");
        exit(EXIT_FAILURE);
    }
#endif

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(false, true);

    printf("Weave Node Configuration:\n");
    printf("  Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    printf("  Subnet Number: %X\n", FabricState.DefaultSubnet);
    printf("  Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

	nl::Weave::Stats::UpdateSnapshot(before);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    err = gServiceMgr.init(&ExchangeMgr, gServiceDirCache, sizeof(gServiceDirCache),
            GetRootServiceDirectoryEntry, kWeaveAuthMode_CASE_ServiceEndPoint,
            NULL, NULL, OverrideServiceConnectArguments);
    FAIL_ERROR(err, "gServiceMgr.Init failed");
#endif

    if (gUseCASE)
        authMode = kWeaveAuthMode_CASE_AnyCert;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDirForTunnel)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            authMode, &gServiceMgr,
                            "weave-tun0", gRole);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            authMode,
                            "weave-tun0", gRole);
    }

    FAIL_ERROR(err, "TunnelAgent.Init failed");

    err = MockCPClient.GenerateAndStoreOperationalDeviceCredentials(FabricState.LocalNodeId);

    if (gDestAddr != IPAddress::Any)
    {
        gTunAgent.SetDestination(gDestNodeId, gDestAddr, gDestPort);
    }

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    if (gPrimaryIntf)
    {
        gTunAgent.SetPrimaryTunnelInterface(gPrimaryIntf);
    }

    if (gBackupIntf)
    {
        gTunAgent.SetBackupTunnelInterface(gBackupIntf);
    }

    if (gEnableBackup)
    {
        gTunAgent.EnableBackupTunnel();
    }
#endif

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    if (gTunnelLogging)
    {
        gTunAgent.OnTunneledPacketTransit = TunneledPacketTransitHandler;
    }
    else
    {
        gTunAgent.OnTunneledPacketTransit = NULL;
    }
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

    err = gTunAgent.StartServiceTunnel();
    FAIL_ERROR(err, "TunnelAgent.StartServiceTunnel failed");

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

    }

    if (gSigusr1Received) {
        printf("SIGUSR1 received: proceed to exit gracefully\n");
    }

    gTunAgent.StopServiceTunnel();
    gTunAgent.Shutdown();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
    return EXIT_SUCCESS;
}

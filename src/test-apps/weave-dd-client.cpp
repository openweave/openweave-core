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
 *      This file implements a process to effect a functional test for
 *      a client for the Weave Device Description profile.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdio.h>
#include <inttypes.h>
#include "ToolCommon.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

using nl::Inet::IPAddress;
using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::DeviceDescription;

#define TOOL_NAME "weave-dd-client"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void SendIdentifyRequest(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static void HandleIdentifyResponse(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyResponseMessage& respMsg);

DeviceDescriptionClient DDClient;
static uint64_t DestNodeId = 1;
const char *DestIPAddrStr = NULL;
IPAddress DestIPAddr;
uint32_t ResendInterval = 200;  //ms
uint32_t ResendCnt = 0;
uint32_t ResendMaxCnt = 3;

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr", kArgumentRequired, 'D' },
    { NULL }
};

static const char *gToolOptionHelp =
    "  -D, --dest-addr <host>\n"
    "       Send an ImageQuery request to a specific address rather than one\n"
    "       derived from the destination node id.  <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address.\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<dest-host>]\n",
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

    InitWeaveStack(true, true);

    // Initialize the mock-device-description-client application.
    err = DDClient.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        printf("DeviceDescriptionClient::Init failed: %s\n", ErrorStr(err));
        exit(-1);
    }

    DDClient.OnIdentifyResponseReceived = HandleIdentifyResponse;

    PrintNodeConfig();

    err = SystemLayer.StartTimer(ResendInterval, SendIdentifyRequest, NULL);
    if (err != WEAVE_NO_ERROR)
    {
        printf("Inet.StartTimer failed: %s\n", ErrorStr(err));
        exit(EXIT_FAILURE);
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);
    }
    printf("over!\n");
    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    DDClient.CancelExchange();
    DDClient.Shutdown();
    return EXIT_SUCCESS;
}

void SendIdentifyRequest(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    printf("Sending identify requests to node %" PRIX64 " at %s\n", DestNodeId, DestIPAddrStr);

    IdentifyRequestMessage identifyReqMsg;
    identifyReqMsg.TargetFabricId = gWeaveNodeOptions.FabricId;
    identifyReqMsg.TargetModes= kTargetDeviceMode_Any;
    identifyReqMsg.TargetVendorId = 0xFFFF;
    identifyReqMsg.TargetProductId = 0xFFFF;
    identifyReqMsg.TargetDeviceId = DestNodeId;

    IPAddress::FromString(DestIPAddrStr, DestIPAddr);
    DDClient.SendIdentifyRequest(DestIPAddr, identifyReqMsg);
    ResendCnt++;
    if (ResendCnt <= ResendMaxCnt)
    {
        SystemLayer.StartTimer(ResendInterval, SendIdentifyRequest, NULL);
    }
    else
    {
        Done = true;
    }
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
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
        PrintArgError("%s: Please specify a node id\n", progName);
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

static void HandleIdentifyResponse(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyResponseMessage& respMsg)
{
    WeaveDeviceDescriptor deviceDesc = respMsg.DeviceDesc;
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    printf("IdentifyResponse received from node %" PRIX64 " (%s)\n", nodeId, ipAddrStr);
    printf("  Source Fabric Id: %016" PRIX64 "\n", deviceDesc.FabricId);
    printf("  Source Vendor Id: %04X\n", (unsigned)deviceDesc.VendorId);
    printf("  Source Product Id: %04X\n", (unsigned)deviceDesc.ProductId);
    printf("  Source Product Revision: %04X\n", (unsigned)deviceDesc.ProductRevision);
    printf("Received IdentifyResponse\n");
    printf("Device Description Operation Completed\n");
    DDClient.CancelExchange();
    DDClient.Shutdown();
    Done = true;
}

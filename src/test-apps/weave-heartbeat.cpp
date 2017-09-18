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
 *      This file implements a command line tool, weave-heartbeat, for the
 *      Weave Heartbeat Profile.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <limits.h>

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Core/WeaveBinding.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Profiles/heartbeat/WeaveHeartbeat.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

using nl::StatusReportStr;
using namespace nl::Weave::Profiles::Heartbeat;
using namespace nl::Weave::Profiles::Security;

#define TOOL_NAME "weave-heartbeat"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void HandleHeartbeatReceived(const WeaveMessageInfo *aMsgInfo, uint8_t nodeState, WEAVE_ERROR err);
static bool ParseDestAddress(const char *progName, const char *arg);
static void HeartbeatSenderEventHandler(void *appState, WeaveHeartbeatSender::EventType eventType, const WeaveHeartbeatSender::InEventParam& inParam, WeaveHeartbeatSender::OutEventParam& outParam);
static void BindingEventHandler(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam);

bool Listening = false;
uint32_t MaxHeartbeatCount = UINT32_MAX;
uint32_t HeartbeatInterval = 1000; // 1 second
uint32_t HeartbeatWindow = 0;
bool Debug = false;
uint64_t DestNodeId;
const char *DestAddr = NULL;
IPAddress DestIPAddr;
uint16_t DestPort;
InterfaceId DestIntf = INET_NULL_INTERFACEID;
uint32_t HeartbeatCount = 0;
WeaveHeartbeatSender HeartbeatSender;
WeaveHeartbeatReceiver HeartbeatReceiver;
bool RequestAck = false;

static OptionDef gToolOptionDefs[] =
{
    { "listen",       kNoArgument,       'L' },
    { "dest-addr",    kArgumentRequired, 'D' },
    { "count",        kArgumentRequired, 'c' },
    { "interval",     kArgumentRequired, 'i' },
    { "window",       kArgumentRequired, 'W' },
    { "request-ack",  kNoArgument,       'r' },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -D, --dest-addr <host>[:<port>][%<interface>]\n"
    "       Send Heartbeats to a specific address rather than one\n"
    "       derived from the destination node id. <host> can be a hostname,\n"
    "       an IPv4 address or an IPv6 address. If <port> is specified, Heartbeat\n"
    "       requests will be sent to the specified port. If <interface> is\n"
    "       specified, Heartbeats will be sent over the specified local\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -c, --count <num>\n"
    "       Send the specified number of Heartbeats and exit.\n"
    "\n"
    "  -i, --interval <ms>\n"
    "       Send Heartbeats at the specified interval in milliseconds.\n"
    "\n"
    "  -W, --window <ms>\n"
    "       Randomize the sending of Heartbeats over the specified interval in milliseconds.\n"
    "\n"
    "  -L, --listen\n"
    "       Listen and respond to Heartbeats sent from another node.\n"
    "\n"
    "  -r, --request-ack\n"
    "       Use Weave Reliable Messaging when sending heartbeats over UDP.\n"
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
    &gWRMPOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    InitToolCommon();

    SetSIGUSR1Handler();

    // Use a smaller default WRMP retransmission interval and count so that the total retry time
    // does not exceed the default heartbeat interval of 1 second.
    gWRMPOptions.RetransInterval = 200;
    gWRMPOptions.RetransCount = 2;

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
            exit(EXIT_FAILURE);
        }

        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, true);

    if (!Listening) // heartbeat sender
    {
        // Create a binding for the HeartbeatSender.
        Binding *binding = ExchangeMgr.NewBinding(BindingEventHandler, NULL);

        // Initialize the HeartbeatSender object.
        err = HeartbeatSender.Init(&ExchangeMgr, binding, HeartbeatSenderEventHandler, NULL);
        if (err != WEAVE_NO_ERROR)
        {
            printf("WeaveHeartbeatSender.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Release the binding (the HeartbeatSender will retain its own reference).
        binding->Release();
    }
    else // heartbeat receiver
    {
        // Initialize the HeartbeatReceiver application.
        err = HeartbeatReceiver.Init(&ExchangeMgr);
        if (err)
        {
            printf("WeaveHeartbeatReceiver.Init failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }

        // Arrange to get a callback whenever a Heartbeat is received.
        HeartbeatReceiver.OnHeartbeatReceived = HandleHeartbeatReceived;
    }

    PrintNodeConfig();

    if (!Listening) // heartbeat sender
    {
        printf("Sending");
        if (MaxHeartbeatCount != UINT32_MAX)
            printf(" %u", MaxHeartbeatCount);
        printf(" Heartbeats via %s to node %" PRIX64, (RequestAck) ? "UDP with WRMP" : "UDP", DestNodeId);
        if (DestAddr != NULL)
            printf(" (%s)", DestAddr);
        printf(" every %" PRId32 " ms", HeartbeatInterval);
        if (HeartbeatWindow > 0)
            printf(", with a randomization window of %" PRId32 " ms", HeartbeatWindow);
        printf("\n");

        HeartbeatSender.SetConfiguration(HeartbeatInterval, 0, HeartbeatWindow);
        HeartbeatSender.SetRequestAck(RequestAck);
        HeartbeatSender.SetSubscriptionState(0x01);

        err = HeartbeatSender.StartHeartbeat();
        if (err != WEAVE_NO_ERROR)
        {
            printf("HeartbeatSender.StartHeartbeat failed: %s\n", ErrorStr(err));
            exit(EXIT_FAILURE);
        }
    }
    else // heartbeat receiver
    {
        printf("Listening for Heartbeats...\n");
    }

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (MaxHeartbeatCount != UINT32_MAX && HeartbeatCount >= MaxHeartbeatCount)
            Done = true;
    }

    HeartbeatSender.Shutdown();
    HeartbeatReceiver.Shutdown();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'L':
        Listening = true;
        break;
    case 'c':
        if (!ParseInt(arg, MaxHeartbeatCount))
        {
            PrintArgError("%s: Invalid value specified for send count: %s\n", progName, arg);
            return false;
        }
        break;
    case 'i':
        if (!ParseInt(arg, HeartbeatInterval))
        {
            PrintArgError("%s: Invalid value specified for heartbeat interval: %s\n", progName, arg);
            return false;
        }
        break;
    case 'W':
        if (!ParseInt(arg, HeartbeatWindow))
        {
            PrintArgError("%s: Invalid value specified for heartbeat randomization window: %s\n", progName, arg);
            return false;
        }
        break;
    case 'D':
        return ParseDestAddress(progName, arg);
        break;
    case 'r':
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        RequestAck = true;
        break;
#else
        PrintArgError("%s: WRMP not supported: %s\n", progName, name);
        return false;
#endif
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    const char *destAddr = NULL;

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
            destAddr = p+1;
        }

        if (!ParseNodeId(nodeId, DestNodeId))
        {
            PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
            return false;
        }

        if (destAddr != NULL && !ParseDestAddress(progName, destAddr))
            return false;
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

void HandleHeartbeatSent(uint64_t destId, IPAddress destAddr, uint8_t state, WEAVE_ERROR err)
{
    char ipAddrStr[64];
    destAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Heartbeat sent to node %" PRIX64 " (%s): state=%u, err=%s\n", destId, ipAddrStr, state, ErrorStr(err));

    HeartbeatCount++;
}

void HandleHeartbeatReceived(const WeaveMessageInfo *aMsgInfo, uint8_t nodeState, WEAVE_ERROR err)
{
    char ipAddrStr[64];
    aMsgInfo->InPacketInfo->SrcAddress.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Heartbeat from node %" PRIX64 " (%s): state=%u, err=%s\n", aMsgInfo->SourceNodeId, ipAddrStr, nodeState, ErrorStr(err));

    HeartbeatCount++;
}

bool ParseDestAddress(const char *progName, const char *arg)
{
    WEAVE_ERROR err;
    const char *addr;
    uint16_t addrLen;
    const char *intfName;
    uint16_t intfNameLen;

    err = ParseHostPortAndInterface(arg, strlen(arg), addr, addrLen, DestPort, intfName, intfNameLen);
    if (err != INET_NO_ERROR)
    {
        PrintArgError("%s: Invalid destination address: %s\n", progName, arg);
        return false;
    }

    if (!IPAddress::FromString(addr, DestIPAddr))
    {
        PrintArgError("%s: Invalid destination address: %s\n", progName, arg);
        return false;
    }

    if (intfName != NULL)
    {
        err = InterfaceNameToId(intfName, DestIntf);
        if (err != INET_NO_ERROR)
        {
            PrintArgError("%s: Invalid interface name: %s\n", progName, intfName);
            return false;
        }
    }

    DestAddr = arg;

    return true;
}

void HeartbeatSenderEventHandler(void *appState, WeaveHeartbeatSender::EventType eventType, const WeaveHeartbeatSender::InEventParam& inParam, WeaveHeartbeatSender::OutEventParam& outParam)
{
    WeaveHeartbeatSender *sender = inParam.Source;
    Binding *binding = sender->GetBinding();

    switch (eventType)
    {
    case WeaveHeartbeatSender::kEvent_HeartbeatSent:
        printf("Heartbeat sent to node %" PRIX64 ": state=%u\n",
               binding->GetPeerNodeId(), sender->GetSubscriptionState());
        HeartbeatCount++;
        break;
    case WeaveHeartbeatSender::kEvent_HeartbeatFailed:
        printf("Heartbeat FAILED to node %" PRIX64 ": state=%u, err=%s\n",
               binding->GetPeerNodeId(), sender->GetSubscriptionState(),
               ErrorStr(inParam.HeartbeatFailed.Reason));
        HeartbeatCount++;
        break;
    default:
        WeaveHeartbeatSender::DefaultEventHandler(appState, eventType, inParam, outParam);
        break;
    }
}

void BindingEventHandler(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam)
{
    switch (eventType)
    {
    case Binding::kEvent_PrepareRequested:
    {
        Binding::Configuration bindingConfig = inParam.Source->BeginConfiguration()
            .Target_NodeId(DestNodeId)
            .Transport_UDP()
            .Transport_DefaultWRMPConfig(gWRMPOptions.GetWRMPConfig())
            .Security_None();
        if (DestAddr != NULL)
        {
            bindingConfig.TargetAddress_IP(DestIPAddr, DestPort, DestIntf);
        }
        outParam.PrepareRequested.PrepareError = bindingConfig.PrepareBinding();
        break;
    }
    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
        break;
    }
}

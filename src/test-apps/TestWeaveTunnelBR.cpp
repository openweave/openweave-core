/*
 *
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

#include "TestWeaveTunnel.h"

#define DEFAULT_BG_NODE_ID 0xBADCAFE

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#include "ToolCommon.h"
#include <SystemLayer/SystemTimer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <SystemLayer/SystemTimer.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING
using namespace ::nl::Weave::Profiles::WeaveTunnel;

#define TOOL_NAME "TestWeaveTunnelBR"

#define DEFAULT_TFE_NODE_ID 0x18b4300200000011

/* Proc file system to read IPv6 routing table. */
#ifndef NL_PATH_PROCNET_IPV6_ROUTE
#define NL_PATH_PROCNET_IPV6_ROUTE     "/proc/net/ipv6_route"
#endif /* NL_PATH_PROCNET_IPV6_ROUTE */

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void
WeaveTunnelOnStatusNotifyHandlerCB(WeaveTunnelConnectionMgr::TunnelConnNotifyReasons reason,
                                   WEAVE_ERROR aErr, void *appCtxt);
#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
static void
WeaveTunnelTCPIdleNotifyHandlerCB(TunnelType tunType,
                                  bool aIsIdle, void *appCtxt);
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
static WEAVE_ERROR SendWeavePingMessage(void);
static void WeaveTunnelOnReconnectNotifyCB(TunnelType tunType,
                                           const char *reconnectHost,
                                           const uint16_t reconnectPort,
                                           void *appCtxt);
static WEAVE_ERROR SendTunnelTestMessage(ExchangeContext *ec, uint32_t profileId, uint8_t msgType,
                                         uint16_t sendFlags);
static void HandleTunnelTestResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                     const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                     uint8_t msgType, PacketBuffer *payload);
static void ResetReconnectTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
static int AddDeleteIPv4Address(InterfaceId intf, const char *ipAddr, bool isAdd);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS
WeaveTunnelAgent gTunAgent;

bool gUseCASE = false;
bool gServiceConnDropSent = false;
const char *gConnectToAddr = NULL;
IPAddress gDestAddr = IPAddress::Any;
IPAddress gRemoteDataAddr = IPAddress::Any;
uint64_t gDestNodeId = DEFAULT_TFE_NODE_ID;
uint32_t gConnectIntervalMS = 2000;
WeaveAuthMode gAuthMode = kWeaveAuthMode_Unauthenticated;
uint64_t gTestStartTime = 0;
uint32_t gCurrTestNum = 0;
uint64_t gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
bool gTestSucceeded = false;
uint8_t  gEncryptionType = kWeaveEncryptionType_None;
uint16_t gKeyId = WeaveKeyId::kNone;
uint8_t gTunUpCount = 0;
uint8_t gConnAttemptsBeforeReset = 0;
bool gReconnectResetArmed = false;
uint64_t gReconnectResetArmTime = 0;
bool gtestDataSent = false;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
bool gUseServiceDir = false;
ServiceDirectory::WeaveServiceManager gServiceMgr;
uint8_t gServiceDirCache[100];
const char *gDirectoryServerURL = "frontdoor.integration.nestlabs.com";
#endif

#define TEST_MAX_TIMEOUT_SECS (30) // Set TCP_USER_TIMEOUT to 30 seconds
#define TEST_KEEPALIVE_INTERVAL_SECS (5) // Set TCP_KEEPALIVE INTERVAL to 5 seconds
#define TEST_GRACE_PERIOD_SECS  (4)

#define TEST_TUNNEL_LIVENESS_INTERVAL_SECS  (10)

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT
// Used for storing the IP address upon removal to restore it at the end of the
// test case.
IPAddress gLocalIPAddr = IPAddress::Any;
InterfaceId gIntf = INET_NULL_INTERFACEID;
uint64_t gTCPUserTimeoutStartTime = 0;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
bool gLivenessTestTunnelUp = false;
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

uint8_t gTunnelingDeviceRole = kClientRole_BorderGateway; //Default Value

enum
{
    kToolOpt_ConnectTo                  = 1000,
    kToolOpt_ConnectToInterval,
    kToolOpt_UseServiceDir,
    kToolOpt_UseCASE,
};

static OptionDef gToolOptionDefs[] =
{
    { "dest-addr",           kArgumentRequired, 'D' },
    { "service-addr",        kArgumentRequired, 'S' },
    { "role",                kArgumentRequired, 'r' },
    { "connect-to",          kArgumentRequired, kToolOpt_ConnectTo },
    { "connect-to-interval", kArgumentRequired, kToolOpt_ConnectToInterval },
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "service-dir",         kNoArgument,       kToolOpt_UseServiceDir },
#endif
    { "case",                kNoArgument,       kToolOpt_UseCASE },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -r, --role <num>\n"
    "       Role for local client node, i.e., 1) Border Gateway or 2) Mobile Device.\n"
    "\n"
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
    "  --connect-to <addr>[:<port>][%<interface>]\n"
    "       Create a Weave connection to the specified address on start up. This\n"
    "       can be used to initiate a passive rendezvous with remote device manager.\n"
    "\n"
    "  --connect-to-interval <ms>\n"
    "       Interval at which to perform connect attempts to the connect-to address.\n"
    "       Defaults to 2 seconds.\n"
    "\n"
    "  -S, --service-addr <remote-ipv6-addr>\n"
    "       Remote destination IPv6 address for sending data traffic over tunnel.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    "  --service-dir\n"
    "       Use service directory to lookup destination node address.\n"
    "\n"
#endif
    "  --case\n"
    "       Use CASE to create an authenticated session and encrypt messages using\n"
    "       the negotiated session key.\n"
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
    "Usage: " TOOL_NAME " [<options...>] [<dest-node-id>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
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
        if (!arg || !ParseInt(arg, gTunnelingDeviceRole) ||
            (gTunnelingDeviceRole != kClientRole_BorderGateway &&
             gTunnelingDeviceRole != kClientRole_MobileDevice))
        {
            PrintArgError("%s: Invalid value specified for device role: %s. Possible values: (1)BorderGateway and (2)MobileDevice\n", progName, arg);
            return false;
        }

        break;

    case kToolOpt_ConnectTo:
        gConnectToAddr = arg;

        break;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kToolOpt_UseServiceDir:
        gUseServiceDir = true;

        break;

#endif
    case kToolOpt_UseCASE:
        gUseCASE = true;

        break;

    case 'D':
        if (!ParseIPAddress(arg, gDestAddr))
        {
            PrintArgError("%s: Invalid value specified for destination IP address: %s\n", progName, arg);
            return false;
        }

        break;

    case 'S':
        if (!ParseIPAddress(arg, gRemoteDataAddr))
        {
            PrintArgError("%s: Invalid value specified for remote destination IPv6 address: %s\n", progName, arg);
            return false;
        }
        if (!gRemoteDataAddr.IsIPv6ULA())
        {
            PrintArgError("%s: Remote IP address %s should be IPv6 ULA. \n", progName, arg);
            return false;
        }

        break;

    case kToolOpt_ConnectToInterval:
        if (!ParseInt(arg, gConnectIntervalMS))
        {
            PrintArgError("%s: Invalid value specified for connect-to interval: %s\n", progName, arg);
            return false;
        }

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

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
WEAVE_ERROR GetRootDirectoryEntry (uint8_t *buf, uint16_t bufSize)
{
    WEAVE_ERROR err;
    const char *host;
    uint16_t hostLen, port;

    using namespace nl::Weave::Encoding;

    err = ParseHostAndPort(gDirectoryServerURL, strlen(gDirectoryServerURL), host, hostLen, port);
    if (err != WEAVE_NO_ERROR)
    {
        return err;
    }
    if (port == 0)
    {
        port = WEAVE_PORT;
    }

    //TODO: Wrong values: Replace with correct ones when Service has Tunnel FrontEnd defined.
    Write8(buf, 0x41);
    LittleEndian::Write64(buf, 0x18B4300200000001ULL);  // Service Endpoint Id = Directory Service
    Write8(buf, 0x80);
    Write8(buf, hostLen);
    memcpy(buf, host, hostLen); buf += hostLen;
    LittleEndian::Write16(buf, port);

    return WEAVE_NO_ERROR;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/**
 * Send an appropriate test message to synchronize with the Server.
 */
WEAVE_ERROR SendTunnelTestMessage(ExchangeContext *ec, uint32_t profileId,
                                  uint8_t msgType,
                                  uint16_t sendFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msg = NULL;

    msg = PacketBuffer::New();
    VerifyOrExit(msg, err = WEAVE_ERROR_NO_MEMORY);

    // Configure the encryption and signature types to be used to send the request.
    ec->EncryptionType = gEncryptionType;
    ec->KeyId = gKeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ec->OnMessageReceived = HandleTunnelTestResponse;

    // Send a Test message. Discard the exchange context if the send fails.
    err = ec->SendMessage(profileId, msgType, msg, sendFlags);
    msg = NULL;

exit:
    return err;
}

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

int AddDeleteIPv4Address(InterfaceId intf, const char *ipAddr, bool isAdd)
{
    char intfStr[32];
    char command[64];
    char addOrDel[4];

    GetInterfaceName(intf, intfStr, sizeof(intfStr));

    strncpy(addOrDel, isAdd ? "add" : "del", sizeof(addOrDel));

    snprintf(command, sizeof(command), "ip addr %s %s/24 dev %s", addOrDel, ipAddr, intfStr);

    return system(command);
}

bool GetIPAddressOfWeaveTCPConnection(char **ip)
{
    FILE *fp = NULL;
    char buf [1024];
    bool found = false;

    // Command to get Weave TCP connections from netstat output
    const char *command = "netstat -an 2>/dev/null | grep 11095";
    fp = popen(command, "r");
    if (fp == NULL)
    {
        WeaveLogError(WeaveTunnel, "Can't open pipe %s : error %ld", command,
                      (long)errno);
        exit(-1);
    }

    while (fgets (buf, 1024, fp) != NULL)
    {
        char proto[4];
        int recvq, sendq;
        char localAddrPort[32];
        char foreignAddrPort[32];
        char *foreignPort;
        char state[16];
        char *str;
        sscanf(buf, "%s %04x %04x %s %s %s",
               proto, &recvq, &sendq, localAddrPort,
               foreignAddrPort, state);

        // Match entry for proto == tcp AND destPort == WEAVE_PORT
        foreignPort = strstr(foreignAddrPort, ":");
        if ((strcmp(foreignPort, ":11095") == 0) &&
            (strcmp(proto, "tcp") == 0))
        {
            str = strtok(localAddrPort, ":");
            strncpy(*ip, str, strlen(str));
            (*ip)[strlen(str)] = '\0';
            found = true;
            break;
        }
        else
        {
            continue;
        }

    }

    pclose(fp);

    return found;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

bool Is48BitIPv6FabricRoutePresent(void)
{
    FILE *fp = NULL;
    char buf [256];
    bool found = false;

    /* Open /proc/net/ipv6_route */
    fp = fopen(NL_PATH_PROCNET_IPV6_ROUTE, "r");
    if (fp == NULL)
    {
        WeaveLogError(WeaveTunnel, "Can't open %s : error %ld", NL_PATH_PROCNET_IPV6_ROUTE,
                      (long)errno);
        exit(-1);
    }

    /* Skip the title line. */
    while (fgets (buf, 256, fp) != NULL)
    {
        int n;
        char dest[33], src[33], gw[33];
        char iface[17];
        int destPrefixLen, srcPrefixLen;
        int metric, use, refCnt, flags;

        n = sscanf (buf, "%32s %02x %32s %02x %32s %08x %08x %08x %08x %s",
                    dest, &destPrefixLen, src, &srcPrefixLen, gw,
                    &metric, &use, &refCnt, &flags, iface);

        if (n != 10)
        {
            continue;
        }

        if (destPrefixLen == 48 && strstr(iface, "weav"))
        {
            found = true;
            break;
        }
    }

    fclose(fp);

    return found;
}

/**
 * Test successful WeaveTunnelAgent Initialization.
 */
static void TestWeaveTunnelAgentInit(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    gCurrTestNum = kTestNum_TestWeaveTunnelAgentInit;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.Shutdown();

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

/**
 * Test successful WeaveTunnelAgent configuration.
 */
static void TestWeaveTunnelAgentConfigure(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IPAddress bogusDestIPAddr = IPAddress::Any;
    uint64_t bogusDestNodeId = 0x1001;

    gCurrTestNum = kTestNum_TestWeaveTunnelAgentConfigure;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    // Set Auth Mode

    gTunAgent.SetAuthMode(kWeaveAuthMode_Unauthenticated);

    // Set bogus destination configuration

    gTunAgent.SetDestination(bogusDestNodeId, bogusDestIPAddr);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    // Start Service Tunnel should fail and return an error
    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.Shutdown();
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);
}

/**
 * Test successful WeaveTunnelAgent Initialization.
 */
static void TestWeaveTunnelAgentShutdown(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    gCurrTestNum = kTestNum_TestWeaveTunnelAgentShutdown;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }
    SuccessOrExit(err);

    err = gTunAgent.Shutdown();

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

/**
 * Test WeaveTunnelAgent StartServiceTunnel without Initialization.
 */
static void TestStartTunnelWithoutInit(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    gCurrTestNum = kTestNum_TestStartTunnelWithoutInit;
    gTestStartTime = Now();

    // Start Service Tunnel should fail with error WEAVE_ERROR_INCORRECT_STATE
    err = gTunAgent.StartServiceTunnel();

    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INCORRECT_STATE);
}

/**
 * Test back to back Start Stop and then Start Weave tunnel.
 */
static void TestBackToBackStartStopStart(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gCurrTestNum = kTestNum_TestBackToBackStartStopStart;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    gTunAgent.StopServiceTunnel();

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test Back to back Stop and Start after a Start completes
 * by receiving a StatusReport for a TunnelOpen message.
 */
static void TestTunnelOpenCompleteThenStopStart(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gTunUpCount = 0;
    gCurrTestNum = kTestNum_TestTunnelOpenCompleteThenStopStart;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    // Start the WeaveTunnel and when it completes do the Stop and Start
    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test sending a TunnelOpen and receiving a StatusReport in response successfully.
 */
static void TestReceiveStatusReportForTunnelOpen(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gCurrTestNum = kTestNum_TestReceiveStatusReportForTunnelOpen;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test a successful Tunnel Open followed by a successful Tunnel Close.
 */
static void TestTunnelOpenThenTunnelClose(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelOpenThenTunnelClose;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test setting up a Standalone Tunnel.
 */
static void TestStandaloneTunnelSetup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gCurrTestNum = kTestNum_TestStandaloneTunnelSetup;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.SetTunnelingDeviceRole(kClientRole_StandaloneDevice);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test a successful Tunnel reconnect attempt on NOT receiving a StatusReport in
 * response to a TunnelOpen.
 */
static void TestTunnelNoStatusReportReconnect(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelNoStatusReportReconnect;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelNoStatusReportReconnect,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelNoStatusReportReconnect,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

void ResetReconnectTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveLogDetail(WeaveTunnel, "Triggering a ResetReconnect Backoff after TunnelOpen sent\n");
    /* Try resetting the connection and issuing a reconnect */
    gTunAgent.ResetPrimaryReconnectBackoff(true);
}

/**
 * Test that a Tunnel reset reconnect backoff does not close an existing tunnel
 * open operation.
 * 1. Send Tunnel Open. The Mock Service is expected not to respond and the
 *    TunnelOpen should timeout.
 * 2. Schedule ResetReconnect before TunnelOpen response timeout happens.
 * 3. Verify that the TunnelOpen response timeout happens normally without
 *    the ResetReconnect re-establishing the connection.
 */
static void TestTunnelNoStatusReportResetReconnectBackoff(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;
    uint32_t delayForResetReconnect = 0;
    Done = false;
    gTestSucceeded = false;
    /* Set the test timeout to be a little longer than the Tunnel Control ExchangeContext
     * timeout */
    gMaxTestDurationMillisecs = (WEAVE_CONFIG_TUNNELING_CTRL_RESPONSE_TIMEOUT_SECS + 1 ) * System::kTimerFactor_milli_per_unit;
    gCurrTestNum = kTestNum_TestTunnelNoStatusReportResetReconnectBackoff;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelNoStatusReportResetReconnectBackoff,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    /* Wait for some time for tunnel connection to be established but before TunnelOpen
     * response timeout happens for triggering a reconnect */
    delayForResetReconnect = (WEAVE_CONFIG_TUNNELING_CTRL_RESPONSE_TIMEOUT_SECS - 1) * System::kTimerFactor_milli_per_unit;

    ExchangeMgr.MessageLayer->SystemLayer->StartTimer(delayForResetReconnect, ResetReconnectTimeout, NULL);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelNoStatusReportResetReconnectBackoff,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test a successful Tunnel reconnect attempt on receiving a StatusReport with
 * an Error status code in response to a TunnelOpen.
 */
static void TestTunnelErrorStatusReportReconnect(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelErrorStatusReportReconnect;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelErrorStatusReportReconnect,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelErrorStatusReportReconnect,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * When a StatusReport with an Error status code is received in response to a TunnelClose,
 * shutdown the tunnel and notify application.
 */
static void TestTunnelErrorStatusReportOnTunnelClose(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelErrorStatusReportOnTunnelClose;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelErrorStatusReportOnTunnelClose,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelErrorStatusReportOnTunnelClose,
                                        0);
            SuccessOrExit(err);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test to ensure the WeaveTunnelAgent tries to reconnect when a connection goes down.
 */
static void TestTunnelConnectionDownReconnect(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelConnectionDownReconnect;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelConnectionDownReconnect,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelConnectionDownReconnect,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test to ensure that the WeaveTunnelAgent notifies the application about the Tunnel
 * being down after the maximum number of reconnect attempts have been made.
 */
static void TestCallTunnelDownAfterMaxReconnects(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = 21000;
    IPAddress fakeAddr;
    gCurrTestNum = kTestNum_TestCallTunnelDownAfterMaxReconnects;
    gTestStartTime = Now();

    // Assign a fake address for the tunnel Service. Loopback should be good enough.

    IPAddress::FromString("127.0.0.1", fakeAddr);

    err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, fakeAddr,
                        gAuthMode);

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;


    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test receving of a Tunnel Reconnect control message and have the border gateway bring down
 * the connection and reconnect.
 */
static void TestReceiveReconnectFromService(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestReceiveReconnectFromService;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;
    gTunAgent.OnServiceTunReconnectNotify = WeaveTunnelOnReconnectNotifyCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestReceiveReconnectFromService,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestReceiveReconnectFromService,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test adding of the fabric default route when the Tunnel is established
 */
static void TestWARMRouteAddWhenTunnelEstablished(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool fabricRouteFound = false;

    Done = false;
    gTestSucceeded = false;
    gCurrTestNum = kTestNum_TestWARMRouteAddWhenTunnelEstablished;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;

    fabricRouteFound = Is48BitIPv6FabricRoutePresent();

    NL_TEST_ASSERT(inSuite, fabricRouteFound == false);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test deleting of the fabric default route when the Tunnel is stopped
 */
static void TestWARMRouteDeleteWhenTunnelStopped(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestWARMRouteDeleteWhenTunnelStopped;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test to successfully send a Weave Ping data message over the Weave Tunnel.
 */
static void TestWeavePingOverTunnel(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestWeavePingOverTunnel;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test to ensure that the WeaveTunnelAgent queues data packets when it is tryng to
 * do fast reconnect attempts to the Service.
 */
static void TestQueueingOfTunneledPackets(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    // Set a longer duration for the queueing test. 3 times the default test
    // duration(~15 seconds) is sufficient for the completion of this test with
    // close to 100% confidence.
    gMaxTestDurationMillisecs = (DEFAULT_TEST_DURATION_MILLISECS * 3);
    gCurrTestNum = kTestNum_TestQueueingOfTunneledPackets;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestQueueingOfTunneledPackets,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {

            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestQueueingOfTunneledPackets,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
/**
 * Test gathering of tunnel statistics after performing a few tunnel operations.
 */
static void TestTunnelStatistics(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveTunnelStatistics tunnelStats;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelStatistics;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                             gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                             gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }
    }

    // Check statistics

    err = gTunAgent.GetWeaveTunnelStatistics(tunnelStats);
    SuccessOrExit(err);

    // Log statistics
    WeaveLogDetail(WeaveTunnel, "Current Timestamp = %" PRIu64 "\n", gTunAgent.GetTimeMsec());

    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelDownCount = %u\n", tunnelStats.mPrimaryStats.mTunnelDownCount);
    WeaveLogDetail(WeaveTunnel, "LastPrimaryTunnelDownWeaveError = %u\n", tunnelStats.mPrimaryStats.mLastTunnelDownError);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelConnAttemptCount = %u\n", tunnelStats.mPrimaryStats.mTunnelConnAttemptCount);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelTxBytes = %" PRIu64 "\n", tunnelStats.mPrimaryStats.mTxBytesToService);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelRxBytes = %" PRIu64 "\n", tunnelStats.mPrimaryStats.mRxBytesFromService);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelTxMessages = %u\n", tunnelStats.mPrimaryStats.mTxMessagesToService);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelRxMessages = %u\n", tunnelStats.mPrimaryStats.mRxMessagesFromService);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelUpTimeStamp = %" PRIu64 "\n", tunnelStats.mPrimaryStats.mLastTimeTunnelEstablished);
    WeaveLogDetail(WeaveTunnel, "PrimaryTunnelDownTimeStamp = %" PRIu64 "\n", tunnelStats.mPrimaryStats.mLastTimeTunnelWentDown);
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    WeaveLogDetail(WeaveTunnel, "BackupTunnelDownCount = %u\n", tunnelStats.mBackupStats.mTunnelDownCount);
    WeaveLogDetail(WeaveTunnel, "LastBackupTunnelDownWeaveError = %u\n", tunnelStats.mBackupStats.mLastTunnelDownError);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelConnAttemptCount = %u\n", tunnelStats.mBackupStats.mTunnelConnAttemptCount);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelTxBytes = %" PRIu64 "\n", tunnelStats.mBackupStats.mTxBytesToService);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelRxBytes = %" PRIu64 "\n", tunnelStats.mBackupStats.mRxBytesFromService);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelTxMessages = %u\n", tunnelStats.mBackupStats.mTxMessagesToService);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelUpTimeStamp = %" PRIu64 "\n", tunnelStats.mBackupStats.mLastTimeTunnelEstablished);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelDownTimeStamp = %" PRIu64 "\n", tunnelStats.mBackupStats.mLastTimeTunnelWentDown);
    WeaveLogDetail(WeaveTunnel, "BackupTunnelRxMessages = %u\n", tunnelStats.mBackupStats.mRxMessagesFromService);
    WeaveLogDetail(WeaveTunnel, "TunnelFailoverCount = %u\n", tunnelStats.mTunnelFailoverCount);
    WeaveLogDetail(WeaveTunnel, "TunnelFailoverTimestamp = %" PRIu64 "\n", tunnelStats.mLastTimeForTunnelFailover);
    WeaveLogDetail(WeaveTunnel, "PrimaryAndBackupTunnelDownTimeStamp = %" PRIu64 "\n", tunnelStats.mLastTimeWhenPrimaryAndBackupWentDown);
    WeaveLogDetail(WeaveTunnel, "LastTunnelFailoverWeaveError = %u\n", tunnelStats.mLastTunnelFailoverError);
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    WeaveLogDetail(WeaveTunnel, "DroppedMessageCount = %u\n", tunnelStats.mDroppedMessagesCount);

    NL_TEST_ASSERT(inSuite, tunnelStats.mPrimaryStats.mTunnelDownCount == 1);
    NL_TEST_ASSERT(inSuite, tunnelStats.mPrimaryStats.mTunnelConnAttemptCount == 1);
    NL_TEST_ASSERT(inSuite, tunnelStats.mPrimaryStats.mTxMessagesToService == 1);
    NL_TEST_ASSERT(inSuite, tunnelStats.mPrimaryStats.mRxMessagesFromService == 1);
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    NL_TEST_ASSERT(inSuite, tunnelStats.mTunnelFailoverCount == 0);
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Test to successfully send a Tunnel Liveness Probe and receive a Status Report.
 */
static void TestTunnelLivenessSendAndRecvResponse(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gMaxTestDurationMillisecs = TEST_TUNNEL_LIVENESS_INTERVAL_SECS * nl::Weave::System::kTimerFactor_milli_per_unit + 2 * DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelLivenessSendAndRecvResponse;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    gTunAgent.ConfigurePrimaryTunnelLivenessInterval(TEST_TUNNEL_LIVENESS_INTERVAL_SECS);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

/**
 * Test Closing of Tunnel when a Liveness Probe receives no response.
 */
static void TestTunnelLivenessDisconnectOnNoResponse(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gLivenessTestTunnelUp = false;
    gMaxTestDurationMillisecs = TEST_TUNNEL_LIVENESS_INTERVAL_SECS * nl::Weave::System::kTimerFactor_milli_per_unit + 2 * DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelLivenessDisconnectOnNoResponse;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelLivenessDisconnectOnNoResponse,
                                0);
    SuccessOrExit(err);

    gTunAgent.ConfigurePrimaryTunnelLivenessInterval(TEST_TUNNEL_LIVENESS_INTERVAL_SECS);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelLivenessDisconnectOnNoResponse,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

static void TestTunnelRestrictedRoutingOnTunnelOpen(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gLivenessTestTunnelUp = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

static void TestTunnelRestrictedRoutingOnStandaloneTunnelOpen(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gCurrTestNum = kTestNum_TestTunnelRestrictedRoutingOnStandaloneTunnelOpen;
    gTestStartTime = Now();
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    gTunAgent.SetTunnelingDeviceRole(kClientRole_StandaloneDevice);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

static void TestTunnelResetReconnectBackoffImmediately(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gConnAttemptsBeforeReset = 0;
    gReconnectResetArmed = false;
    gReconnectResetArmTime = 0;
    gMaxTestDurationMillisecs = RECONNECT_RESET_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelResetReconnectBackoffImmediately;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelResetReconnectBackoffImmediately,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelResetReconnectBackoffImmediately,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

static void TestTunnelResetReconnectBackoffRandomized(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    Done = false;
    gTestSucceeded = false;
    gConnAttemptsBeforeReset = 0;
    gReconnectResetArmed = false;
    gReconnectResetArmTime = 0;
    gMaxTestDurationMillisecs = RECONNECT_RESET_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelResetReconnectBackoffRandomized;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_Start,
                                kTestNum_TestTunnelResetReconnectBackoffRandomized,
                                0);
    SuccessOrExit(err);

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_End,
                                        kTestNum_TestTunnelResetReconnectBackoffRandomized,
                                        0);
            SuccessOrExit(err);

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_NOT_CONNECTED);
        }
    }

exit:
    if (exchangeCtxt)
    {
        exchangeCtxt->Close();
        exchangeCtxt = NULL;
    }

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT
/**
 * Test to verify that the TCP User Timeout is enforced when the IP address
 * on the border gateway interface is removed.
 */
static void TestTCPUserTimeoutOnAddrRemoval(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    char ip[32];
    char *strPtr = ip;
    gMaxTestDurationMillisecs = 4*RECONNECT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTCPUserTimeoutOnAddrRemoval;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
    gTunAgent.OnServiceTunTCPIdleNotify = WeaveTunnelTCPIdleNotifyHandlerCB;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK


    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gLocalIPAddr.ToString(strPtr, sizeof(ip));

            // Add the IP Address back on interface
            if (AddDeleteIPv4Address(gIntf, strPtr, true) < 0)
            {
               ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
            }

            gTunAgent.StopServiceTunnel(WEAVE_ERROR_TUNNEL_FORCE_ABORT);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT

#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
/**
 * Test to verify that sent TCP data is acknowledged.
 */
static void TestTunnelTCPIdle(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Done = false;
    gTestSucceeded = false;
    gtestDataSent = false;
    gMaxTestDurationMillisecs = DEFAULT_TEST_DURATION_MILLISECS;
    gCurrTestNum = kTestNum_TestTunnelTCPIdle;
    gTestStartTime = Now();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (gUseServiceDir)
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId,
                            gAuthMode, &gServiceMgr);
    }
    else
#endif
    {
        err = gTunAgent.Init(&Inet, &ExchangeMgr, gDestNodeId, gDestAddr,
                            gAuthMode);
    }

    SuccessOrExit(err);

    gTunAgent.OnServiceTunStatusNotify = WeaveTunnelOnStatusNotifyHandlerCB;

    gTunAgent.OnServiceTunTCPIdleNotify = WeaveTunnelTCPIdleNotifyHandlerCB;

    err = gTunAgent.StartServiceTunnel();
    SuccessOrExit(err);

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = TEST_SLEEP_TIME_WITHIN_LOOP_SECS;
        sleepTime.tv_usec = TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS;

        ServiceNetwork(sleepTime);

        if (Now() < gTestStartTime + gMaxTestDurationMillisecs * System::kTimerFactor_micro_per_milli)
        {
            if (gTestSucceeded)
            {
                Done = true;
            }
            else
            {
                continue;
            }
        }
        else // Time's up
        {
            gTestSucceeded = false;
            Done = true;
        }

        if (Done)
        {
            gTunAgent.StopServiceTunnel(WEAVE_ERROR_TUNNEL_FORCE_ABORT);
        }
    }

exit:
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, gTestSucceeded == true);

    gTunAgent.Shutdown();
}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK

void HandleTunnelTestResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                              const WeaveMessageInfo *msgInfo, uint32_t profileId,
                              uint8_t msgType, PacketBuffer *payload)
{
    switch (gCurrTestNum)
    {
      case  kTestNum_TestWeavePingOverTunnel:
      case kTestNum_TestQueueingOfTunneledPackets:
        if (profileId == kWeaveProfile_Echo && msgType == kEchoMessageType_EchoResponse)
        {
            gTestSucceeded = true;
        }
        else
        {
            gTestSucceeded = false;
        }

        break;

      case  kTestNum_TestTunnelStatistics:
        if (profileId == kWeaveProfile_Echo && msgType == kEchoMessageType_EchoResponse)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
        else
        {
            gTestSucceeded = false;
        }

        break;

      default:

        break;

    }

    // Free the payload buffer.
    PacketBuffer::Free(payload);

    ec->Close();
}

void WeaveTunnelOnReconnectNotifyCB(TunnelType tunType,
                                    const char *reconnectHost,
                                    const uint16_t reconnectPort,
                                    void *appCtxt)
{
    if (gCurrTestNum == kTestNum_TestReceiveReconnectFromService)
    {
        if (0 == strncmp(reconnectHost, TEST_TUNNEL_RECONNECT_HOSTNAME, sizeof(TEST_TUNNEL_RECONNECT_HOSTNAME)))
        {
            WeaveLogDetail(WeaveTunnel, "Tunnel Reconnect received from Service for Tunnel type %d, to %s:%d\n", tunType,
                           reconnectHost, reconnectPort);
            gTestSucceeded = true;
        }
    }
}

WEAVE_ERROR SendWeavePingMessage(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gRemoteDataAddr, &gTunAgent);
    VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

    // Send Weave ping over tunnel.
    err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_Echo,
                                kEchoMessageType_EchoRequest,
                                0);
exit:
    return err;
}

#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
void
WeaveTunnelTCPIdleNotifyHandlerCB(TunnelType tunType,
                                  bool aIsIdle, void *appCtxt)
{
    switch (gCurrTestNum)
    {
      case kTestNum_TestTunnelTCPIdle:
        if (aIsIdle)
        {
            WeaveLogDetail(WeaveTunnel, "Tunnel sent data flushed for tunnel type %d\n", tunType);
            if (gtestDataSent)
            {
                gTestSucceeded = true;
            }
        }
        else
        {
            WeaveLogDetail(WeaveTunnel, "Tunnel data transmitted for tunnel type %d: TCP channel not Idle yet.\n",
                           tunType);
        }

        break;

      case kTestNum_TestTCPUserTimeoutOnAddrRemoval:
        if (aIsIdle)
        {
            WeaveLogDetail(WeaveTunnel, "Tunnel sent data flushed for tunnel type %d\n", tunType);
            gTestSucceeded = false;
        }
        else
        {
            WeaveLogDetail(WeaveTunnel, "Tunnel data transmitted for tunnel type %d: TCP channel not Idle yet.\n",
                           tunType);
        }
    }
}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK

void
WeaveTunnelOnStatusNotifyHandlerCB(WeaveTunnelConnectionMgr::TunnelConnNotifyReasons reason,
                                   WEAVE_ERROR aErr, void *appCtxt)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *exchangeCtxt = NULL;

    WeaveLogDetail(WeaveTunnel, "WeaveTunnelAgent notification reason code is %d", reason);

    switch (gCurrTestNum)
    {
      case  kTestNum_TestReceiveStatusReportForTunnelOpen:
      case  kTestNum_TestStandaloneTunnelSetup:
      case  kTestNum_TestBackToBackStartStopStart:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            gTestSucceeded = true;
        }
        else
        {
            gTestSucceeded = false;
        }

        break;
      case  kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen:
      case  kTestNum_TestTunnelRestrictedRoutingOnStandaloneTunnelOpen:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            // Check if we got the correct error code and the Fabric tunnel
            // route is installed.
            if (aErr == WEAVE_ERROR_TUNNEL_ROUTING_RESTRICTED && Is48BitIPv6FabricRoutePresent())
            {
                gTestSucceeded = true;
            }
            else
            {
                gTestSucceeded = false;
            }
        }

        break;
      case  kTestNum_TestTunnelOpenCompleteThenStopStart:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            if (gTunUpCount < 1)
            {
                gTunUpCount++;

                gTunAgent.StopServiceTunnel();

                gTunAgent.StartServiceTunnel();
            }
            else
            {
                gTestSucceeded = true;
            }
        }
        else
        {
            gTestSucceeded = false;
        }

        break;

      case  kTestNum_TestTunnelOpenThenTunnelClose:
      case  kTestNum_TestTunnelErrorStatusReportOnTunnelClose:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunDown)
        {
            gTestSucceeded = true;
        }

        break;

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
      case  kTestNum_TestTunnelStatistics:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            err = SendWeavePingMessage();
            SuccessOrExit(err);
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunDown)
        {
            gTestSucceeded = true;
        }

        break;

#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
      case kTestNum_TestTunnelNoStatusReportReconnect:
      case kTestNum_TestTunnelConnectionDownReconnect:
      case kTestNum_TestTunnelErrorStatusReportReconnect:
      case kTestNum_TestWeaveTunnelAgentConfigure:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError)
        {
            gTestSucceeded = true;
        }

        break;

      case kTestNum_TestTunnelNoStatusReportResetReconnectBackoff:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError)
        {
            if (aErr == WEAVE_ERROR_TIMEOUT)
            {
                WeaveLogDetail(WeaveTunnel, "Tun Open Timeout error");
                gTestSucceeded = true;
            }
            else
            {
                WeaveLogDetail(WeaveTunnel, "Connect error received with error %s", ErrorStr(aErr));
                gTestSucceeded = false;
            }
        }
        break;

      case kTestNum_TestCallTunnelDownAfterMaxReconnects:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunDown)
        {
            gTestSucceeded = true;
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError)
        {
            WeaveLogDetail(WeaveTunnel, "Tun Connection Error");
        }

        break;

      case kTestNum_TestWeavePingOverTunnel:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            err = SendWeavePingMessage();
            SuccessOrExit(err);
        }
        else
        {
            gTestSucceeded = false;
        }

        break;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT
      case kTestNum_TestTCPUserTimeoutOnAddrRemoval:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            char ip[32];
            char *strPtr = ip;
            bool found = GetIPAddressOfWeaveTCPConnection(&strPtr);
            if (!found)
            {
                gTestSucceeded = false;
                break;
            }

            IPAddress::FromString(strPtr, gLocalIPAddr);

            // Configure the TCP User Timeout
            err = gTunAgent.ConfigurePrimaryTunnelTimeout(TEST_MAX_TIMEOUT_SECS);
            SuccessOrExit(err);

            // Get the interface matching the IP
            err = Inet.GetInterfaceFromAddr(gLocalIPAddr, gIntf);
            SuccessOrExit(err);

            // Remove IP Address on interface
            if (AddDeleteIPv4Address(gIntf, strPtr, false) < 0)
            {
               ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
            }

            // Send some data
            err = SendWeavePingMessage();
            SuccessOrExit(err);

            // Mark the time for getting a connection reconnect up call when
            // TCP User timeout happens.
            gTCPUserTimeoutStartTime = Now();
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError &&
                 aErr == INET_ERROR_TCP_USER_TIMEOUT)
        {
            if ((Now() - gTCPUserTimeoutStartTime > TEST_MAX_TIMEOUT_SECS * nl::Weave::System::kTimerFactor_micro_per_unit) ||
                (Now() - gTCPUserTimeoutStartTime < (TEST_MAX_TIMEOUT_SECS + TEST_GRACE_PERIOD_SECS) * nl::Weave::System::kTimerFactor_micro_per_unit))
            {
                gTestSucceeded = true;
            }
        }

        break;
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT

#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
      case kTestNum_TestTunnelTCPIdle:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            // Configure the TCP User Timeout
            err = gTunAgent.ConfigurePrimaryTunnelTimeout(TEST_MAX_TIMEOUT_SECS);
            SuccessOrExit(err);

            // Send some data
            err = SendWeavePingMessage();
            SuccessOrExit(err);

            gtestDataSent = true;
        }

        break;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
      case kTestNum_TestTunnelLivenessSendAndRecvResponse:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryLiveness &&
            aErr == WEAVE_NO_ERROR)
        {
            gTestSucceeded = true;
        }

        break;

      case kTestNum_TestTunnelLivenessDisconnectOnNoResponse:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            gLivenessTestTunnelUp = true;
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError &&
                 (aErr == WEAVE_ERROR_TIMEOUT || aErr == INET_ERROR_TCP_USER_TIMEOUT))
        {
            if (gLivenessTestTunnelUp)
            {
                gTestSucceeded = true;
            }
            else
            {
                gTestSucceeded = false;
            }
        }

        break;

#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
      case kTestNum_TestWARMRouteAddWhenTunnelEstablished:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            if (Is48BitIPv6FabricRoutePresent())
            {
                gTestSucceeded = true;
            }
            else
            {
                gTestSucceeded = false;
            }
        }

        break;

      case kTestNum_TestWARMRouteDeleteWhenTunnelStopped:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            gTunAgent.StopServiceTunnel(WEAVE_NO_ERROR);
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunDown)
        {
            if (Is48BitIPv6FabricRoutePresent())
            {
                gTestSucceeded = false;
            }
            else
            {
                gTestSucceeded = true;
            }
        }

        break;

      case kTestNum_TestReceiveReconnectFromService:
        WeaveLogDetail(WeaveTunnel, "Tunnel established; Expecting a Reconnect\n");

        break;

      case kTestNum_TestQueueingOfTunneledPackets:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp)
        {
            if (!gServiceConnDropSent)
            {
                IPAddress fakeAddr;

                // Configure wrong IP address to generate connection error

                IPAddress::FromString("127.0.0.1", fakeAddr);

                gTunAgent.SetDestination(gDestNodeId, fakeAddr);

                // Now, instruct Service to drop connection to trigger reconnect attempt

                exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gDestAddr, &gTunAgent);
                VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

                err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_TunnelTest_RequestTunnelConnDrop,
                                            kTestNum_TestQueueingOfTunneledPackets,
                                            0);
                SuccessOrExit(err);

                gServiceConnDropSent = true;
            }
        }
        else if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError)
        {
            if (gServiceConnDropSent)
            {

                // Send Weave ping over tunnel which should get queued.
                exchangeCtxt = ExchangeMgr.NewContext(gDestNodeId, gRemoteDataAddr, &gTunAgent);
                VerifyOrExit(exchangeCtxt, err = WEAVE_ERROR_NO_MEMORY);

                err = SendTunnelTestMessage(exchangeCtxt, kWeaveProfile_Echo,
                                            kEchoMessageType_EchoRequest,
                                            0);
                SuccessOrExit(err);

                // Set correct Destination address configuration for subsequent successful reconnection
                // and delivery of queued ping request.

                gTunAgent.SetDestination(gDestNodeId, gDestAddr);
            }
            else
            {
                gTestSucceeded = false;
            }
        }

        break;

      case kTestNum_TestTunnelResetReconnectBackoffImmediately:
      case kTestNum_TestTunnelResetReconnectBackoffRandomized:
        if (reason == WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError)
        {
            if (gReconnectResetArmed)
            {
                WeaveLogDetail(WeaveTunnel, "Tunnel Connect error after reset armed\n");

                if (gCurrTestNum == kTestNum_TestTunnelResetReconnectBackoffImmediately)
                {
                    if (Now() - gReconnectResetArmTime < (BACKOFF_RESET_IMMEDIATE_THRESHOLD_SECS *
                                                          nl::Weave::System::kTimerFactor_micro_per_unit))
                    {
                        gTestSucceeded = true;
                    }
                    else
                    {
                        gTestSucceeded = false;
                    }
                }
                else
                {
                    if (Now() - gReconnectResetArmTime < (BACKOFF_RESET_RANDOMIZED_THRESHOLD_SECS *
                                                          nl::Weave::System::kTimerFactor_micro_per_unit))
                    {
                        gTestSucceeded = true;
                    }
                    else
                    {
                        gTestSucceeded = false;
                    }

                }
            }
            else
            {
                gConnAttemptsBeforeReset++;

                if (gConnAttemptsBeforeReset == TEST_CONN_ATTEMPTS_BEFORE_RESET)
                {
                    if (gCurrTestNum == kTestNum_TestTunnelResetReconnectBackoffImmediately)
                    {
                        gTunAgent.ResetPrimaryReconnectBackoff(true);
                    }
                    else
                    {
                        gTunAgent.ResetPrimaryReconnectBackoff(false);
                    }

                    gReconnectResetArmTime = Now();

                    gReconnectResetArmed = true;
                    WeaveLogDetail(WeaveTunnel, "Tunnel Reconnect Reset armed\n");
                }
            }
        }

        break;

      default:

        break;

    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (exchangeCtxt)
        {
            exchangeCtxt->Close();
            exchangeCtxt = NULL;
        }

        gTestSucceeded = false;
    }
}

static const nlTest tunnelTests[] = {
    NL_TEST_DEF("TestWeaveTunnelAgentInit", TestWeaveTunnelAgentInit),
    NL_TEST_DEF("TestWeaveTunnelAgentConfigure", TestWeaveTunnelAgentConfigure),
    NL_TEST_DEF("TestWeaveTunnelAgentShutdown", TestWeaveTunnelAgentShutdown),
    NL_TEST_DEF("TestStartTunnelWithoutInit", TestStartTunnelWithoutInit),
    NL_TEST_DEF("TestBackToBackStartStopStart", TestBackToBackStartStopStart),
    NL_TEST_DEF("TestTunnelOpenCompleteThenStopStart", TestTunnelOpenCompleteThenStopStart),
    NL_TEST_DEF("TestReceiveStatusReportForTunnelOpen", TestReceiveStatusReportForTunnelOpen),
    NL_TEST_DEF("TestTunnelOpenThenTunnelClose", TestTunnelOpenThenTunnelClose),
    NL_TEST_DEF("TestStandaloneTunnelSetup", TestStandaloneTunnelSetup),
    NL_TEST_DEF("TestTunnelNoStatusReportReconnect", TestTunnelNoStatusReportReconnect),
    NL_TEST_DEF("TestTunnelErrorStatusReportReconnect", TestTunnelErrorStatusReportReconnect),
    NL_TEST_DEF("TestTunnelErrorStatusReportOnTunnelClose", TestTunnelErrorStatusReportOnTunnelClose),
    NL_TEST_DEF("TestTunnelConnectionDownReconnect", TestTunnelConnectionDownReconnect),
    NL_TEST_DEF("TestCallTunnelDownAfterMaxReconnects", TestCallTunnelDownAfterMaxReconnects),
    NL_TEST_DEF("TestReceiveReconnectFromService", TestReceiveReconnectFromService),
    NL_TEST_DEF("TestWARMRouteAddWhenTunnelEstablished", TestWARMRouteAddWhenTunnelEstablished),
    NL_TEST_DEF("TestWARMRouteDeleteWhenTunnelStopped", TestWARMRouteDeleteWhenTunnelStopped),
    NL_TEST_DEF("TestWeavePingOverTunnel", TestWeavePingOverTunnel),
    NL_TEST_DEF("TestQueueingOfTunneledPackets", TestQueueingOfTunneledPackets),
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    NL_TEST_DEF("TestTunnelStatistics", TestTunnelStatistics),
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    NL_TEST_DEF("TestTunnelLivenessSendAndRecvResponse", TestTunnelLivenessSendAndRecvResponse),
    NL_TEST_DEF("TestTunnelLivenessDisconnectOnNoResponse", TestTunnelLivenessDisconnectOnNoResponse),
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    NL_TEST_DEF("TestTunnelRestrictedRoutingOnTunnelOpen", TestTunnelRestrictedRoutingOnTunnelOpen),
    NL_TEST_DEF("TestTunnelRestrictedRoutingOnStandaloneTunnelOpen", TestTunnelRestrictedRoutingOnStandaloneTunnelOpen),
    NL_TEST_DEF("TestTunnelResetReconnectBackoffImmediately", TestTunnelResetReconnectBackoffImmediately),
    NL_TEST_DEF("TestTunnelResetReconnectBackoffRandomized", TestTunnelResetReconnectBackoffRandomized),
    NL_TEST_DEF("TestTunnelNoStatusReportResetReconnectBackoff", TestTunnelNoStatusReportResetReconnectBackoff),
#if WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
    NL_TEST_DEF("TestTunnelTCPIdle", TestTunnelTCPIdle),
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TCP_IDLE_CALLBACK
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT
    NL_TEST_DEF("TestTCPUserTimeoutOnAddrRemoval", TestTCPUserTimeoutOnAddrRemoval),
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS && WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED && INET_CONFIG_OVERRIDE_SYSTEM_TCP_USER_TIMEOUT
    NL_TEST_SENTINEL()
};
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

int main(int argc, char *argv[])
{
#if WEAVE_CONFIG_ENABLE_TUNNELING
    WEAVE_ERROR err;
    gWeaveNodeOptions.LocalNodeId = DEFAULT_BG_NODE_ID;

    nlTestSuite tunnelTestSuite = {
        "WeaveTunnel",
        &tunnelTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    UseStdoutLineBuffering();
    SetSIGUSR1Handler();

    // Set default Remote data IP address to be of the Service Tunnel Endpoint;

    IPAddress::FromString("fd00:0:1:5:1ab4:3002:0000:0011", gRemoteDataAddr);

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
            WeaveLogError(WeaveTunnel, "Local address must be an IPv6 ULA\n");
            exit(EXIT_FAILURE);
        }
        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();

    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(false, true);

    if (gDestAddr == IPAddress::Any)
    {
        gDestAddr = FabricState.SelectNodeAddress(gDestNodeId);
    }

    WeaveLogDetail(WeaveTunnel, "Weave Node Configuration:\n");
    WeaveLogDetail(WeaveTunnel, "Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    WeaveLogDetail(WeaveTunnel, "Subnet Number: %X\n", FabricState.DefaultSubnet);
    WeaveLogDetail(WeaveTunnel, "Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

    if (gConnectToAddr != NULL)
    {
        IPAddress::FromString(gConnectToAddr, gDestAddr);
    }

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    err = gServiceMgr.init(&ExchangeMgr, gServiceDirCache, sizeof(gServiceDirCache),
            GetRootDirectoryEntry, kWeaveAuthMode_CASE_ServiceEndPoint);
    FAIL_ERROR(err, "gServiceMgr.Init failed");
#endif

    if (gUseCASE)
        gAuthMode = kWeaveAuthMode_CASE_AnyCert;

    // Run all tests in Suite

    nlTestRunner(&tunnelTestSuite, NULL);

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return nlTestRunnerStats(&tunnelTestSuite);
#endif // WEAVE_CONFIG_ENABLE_TUNNELING
}

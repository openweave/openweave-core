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
 *      This file implements the Weave Mock Device command line tool.
 *
 *      The Weave Mock Device command line tool is a used, primarily,
 *      as a test target for various Weave profiles, protocols, and
 *      server implementations.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <unistd.h>

#include <Weave/Core/WeaveConfig.h>
#if WEAVE_CONFIG_LEGACY_WDM
// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/ProfileDatabase.h>
#include "TestProfile.h"
#include "MockDMPublisher.h"
#endif // WEAVE_CONFIG_LEGACY_WDM

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkInfo.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/heartbeat/WeaveHeartbeat.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include "MockDCLPServer.h"
#include "MockNPServer.h"
#include "MockSPServer.h"
#include "MockFPServer.h"
#include "MockPairingServer.h"
#include "MockDDServer.h"
#include "MockDCServer.h"
#include "MockOpActions.h"
#include "MockTokenPairingServer.h"
#include "MockWdmViewClient.h"
#include "MockWdmViewServer.h"

#include "MockWdmSubscriptionInitiator.h"
#include "MockWdmSubscriptionResponder.h"
#include "MockLoggingManager.h"
#include "MockWdmNodeOptions.h"

#if WEAVE_CONFIG_TIME
#include "MockTimeSyncUtil.h"
#include "MockTimeSyncClient.h"
#endif // WEAVE_CONFIG_TIME

#if WEAVE_CONFIG_ENABLE_TUNNELING
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

using namespace nl::Weave::Profiles;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::NetworkProvisioning;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace nl::Weave::Profiles::FabricProvisioning;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Profiles::Heartbeat;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::ServiceDirectory;
#endif

using namespace ::nl::Weave::Profiles::StatusReporting;

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static void HandleHeartbeatReceived(const WeaveMessageInfo *aMsgInfo, uint8_t nodeState, WEAVE_ERROR err);
static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);
static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
static void InitiateConnection(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
static void HandleWdmCompleteTest();
static void HandleError();

#if CONFIG_BLE_PLATFORM_BLUEZ
bool EnableWeaveBluezPeripheral = false;
static pthread_t sBLEThread = PTHREAD_NULL;
#endif

#if WEAVE_CONFIG_ENABLE_TUNNELING
using namespace ::nl::Weave::Profiles::WeaveTunnel;

#define DEFAULT_TFE_NODE_ID 0xC0FFEE

WeaveTunnelAgent TunAgent;
const char *TunnelConnectToAddr = NULL;
IPAddress TunnelDestAddr = IPAddress::Any;
uint64_t TunnelDestNodeId = DEFAULT_TFE_NODE_ID;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
bool UseServiceDirForTunnel = false;
ServiceDirectory::WeaveServiceManager ServiceMgr;
uint8_t ServiceDirCache[100];
#endif

int32_t  tunnelingDeviceRole = 0;
#endif

// only for wdm next test
char * TestCaseId = NULL;
bool EnableStopTest = false;
char * NumDataChangeBeforeCancellation = NULL;
char * FinalStatus = NULL;
char * TimeBetweenDataChangeMsec = NULL;
bool EnableDataFlip = true;
char * TimeBetweenLivenessCheckSec = NULL;
bool EnableDictionaryTest = false;

#if CONFIG_BLE_PLATFORM_BLUEZ
// only for woble bluez peripheral
static char * BleName = NULL;
static char * BleAddress = NULL;
#endif

// events
EventGenerator * gEventGenerator = NULL;
int TimeBetweenEvents = 1000;

// only for wdm next test
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
int WdmRoleInTest = 0;
bool EnableRetry = false;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

bool Debug = false;
bool Preconfig = false;
bool UseCASE = false;
const char *ConnectToAddr = NULL;
uint32_t ConnectIntervalMS = 2000;
WeaveEchoServer EchoServer;
WeaveHeartbeatReceiver HeartbeatReceiver;
MockNetworkProvisioningServer MockNPServer;
MockDropcamLegacyPairingServer MockDCLPServer;
MockServiceProvisioningServer MockSPServer;
MockFabricProvisioningServer MockFPServer;
MockPairingServer MockPairingEPServer;
MockDeviceDescriptionServer MockDDServer;
MockDeviceControlServer MockDCServer;
MockTokenPairingServer MockTPServer;

#if WEAVE_CONFIG_TIME
MockTimeSync MockTimeNode;

uint64_t TimeSyncServerNodeId = kAnyNodeId;
const char *TimeSyncServerNodeAddr = NULL;
uint16_t TimeSyncServerSubnetId = kWeaveSubnetId_NotSpecified;
MockSingleSourceTimeSyncClient simpleTimeSyncClient;
bool ShouldEnableSimpleTimeSyncClient = false;
#endif // WEAVE_CONFIG_TIME

#if WEAVE_CONFIG_LEGACY_WDM
MockDMPublisher MockDMPublisher;
#endif

MockOpActions OpActions;

uint32_t RespDelayTime = 10000;

const char *PairingServer = NULL;

uint64_t PairingEndPointIdArg = kServiceEndpoint_ServiceProvisioning;
int PairingTransportArg = kPairingTransport_TCP;

#define TOOL_NAME "mock-device"

enum
{
    kToolOpt_ConnectTo                          = 1000,
    kToolOpt_ConnectToInterval                  = 1001,
    kToolOpt_TimeSyncServer                     = 1003, // specify that this mock device should run a Time Sync Server
    kToolOpt_TimeSyncClient                     = 1004, // specify that this mock device should run a Time Sync Client
    kToolOpt_TimeSyncCoordinator                = 1005, // specify that this mock device should run a Time Sync Coordinator
    kToolOpt_TimeSyncModeLocal                  = 1006, // specify that the Time Client Sync mode is Local (time sync with local nodes via UDP)
    kToolOpt_TimeSyncModeService                = 1007, // specify that the Time Client Sync mode is Service (time sync with Service via TCP)
    kToolOpt_TimeSyncModeAuto                   = 1008, // specify that the Time Client Sync mode is Auto (time sync via multicast)
    kToolOpt_TunnelBorderGw                     = 1012, // specify that this mock device should run as a Border gateway capable of Tunneling
    kToolOpt_TunnelMobDevice                    = 1013, // specify that this mock device should run as a Mobile Device capable of Tunneling
    kToolOpt_TunnelConnectAddr                  = 1014,
    kToolOpt_TunnelDestNodeId                   = 1015,

    kToolOpt_PairViaWRM                         = 1035,
    kToolOpt_PairingEndPointId                  = 1037,

    kToolOpt_TimeSyncSimpleClient               = 1038, // specify that this mock device should run a Simple Time Sync Client
    kToolOpt_TimeSyncServerNodeId               = 1039, // Specify the node ID of the Time Sync Server we should contact with
    kToolOpt_TimeSyncServerSubnetId             = 1040, // Specify the subnet ID of the Time Sync Server we should contact with
    kToolOpt_TimeSyncServerNodeAddr             = 1041, // Specify the node address of the Time Sync Server we should contact
    kToolOpt_TimeSyncModeServiceOverTunnel      = 1042, // specify that the Time Client Sync mode is Service (time sync with Service over a tunnel)
    kToolOpt_UseServiceDir,
    kToolOpt_SuppressAccessControl,

// only for weave over bluez peripheral
#if CONFIG_BLE_PLATFORM_BLUEZ
    kToolOpt_EnableWeaveBluezPeripheral,
    kToolOpt_WeaveBluezPeripheralName,
    kToolOpt_WeaveBluezPeripheralAddress,
#endif

};

static OptionDef gToolOptionDefs[] =
{
#if WEAVE_CONFIG_ENABLE_TUNNELING
    { "tun-border-gw",              kNoArgument,        kToolOpt_TunnelBorderGw },
    { "tun-mob-device",             kNoArgument,        kToolOpt_TunnelMobDevice },
    { "tun-connect-to",             kArgumentRequired,  kToolOpt_TunnelConnectAddr },
    { "tun-dest-node-id",           kArgumentRequired,  kToolOpt_TunnelDestNodeId },
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    { "service-dir",                kNoArgument,        kToolOpt_UseServiceDir },
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#if CONFIG_BLE_PLATFORM_BLUEZ
    { "enable-bluez-peripheral",    kNoArgument,        kToolOpt_EnableWeaveBluezPeripheral },
    { "peripheral-name",            kArgumentRequired,  kToolOpt_WeaveBluezPeripheralName },
    { "peripheral-address",         kArgumentRequired,  kToolOpt_WeaveBluezPeripheralAddress },
#endif
#endif //WEAVE_CONFIG_ENABLE_TUNNELING
    { "pairing-server",             kArgumentRequired,  'p' },
    { "wrm-pairing",                kNoArgument,        kToolOpt_PairViaWRM },
    { "pairing-endpoint-id",        kArgumentRequired,  kToolOpt_PairingEndPointId },
    { "delay",                      kArgumentRequired,  'r' },
    { "delay-time",                 kArgumentRequired,  't' },
    { "preconfig",                  kNoArgument,        'c' },
    { "suppress-ac",                kNoArgument,        kToolOpt_SuppressAccessControl },
    { "connect-to",                 kArgumentRequired,  kToolOpt_ConnectTo },
    { "connect-to-interval",        kArgumentRequired,  kToolOpt_ConnectToInterval },
#if WEAVE_CONFIG_TIME
    { "time-sync-server",           kNoArgument,        kToolOpt_TimeSyncServer },
    { "time-sync-client",           kNoArgument,        kToolOpt_TimeSyncClient },
    { "time-sync-coordinator",      kNoArgument,        kToolOpt_TimeSyncCoordinator },
    { "time-sync-mode-local",       kNoArgument,        kToolOpt_TimeSyncModeLocal },
    { "time-sync-mode-service",     kNoArgument,        kToolOpt_TimeSyncModeService },
    { "time-sync-mode-service-over-tunnel",     kNoArgument,        kToolOpt_TimeSyncModeServiceOverTunnel },
    { "time-sync-mode-auto",        kNoArgument,        kToolOpt_TimeSyncModeAuto },

    { "ts-simple-client",           kNoArgument,        kToolOpt_TimeSyncSimpleClient },
    { "ts-server-node-id",          kArgumentRequired,  kToolOpt_TimeSyncServerNodeId },
    { "ts-server-node-addr",        kArgumentRequired,  kToolOpt_TimeSyncServerNodeAddr },
    { "ts-server-subnet-id",        kArgumentRequired,  kToolOpt_TimeSyncServerSubnetId },
#endif // WEAVE_CONFIG_TIME
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -c, --preconfig\n"
    "       Initialize the mock device as if it had already been configured.\n"
    "\n"
    "  -P, --pairing-code <code>\n"
    "       Pairing code to use when authenticating clients. Defaults to 'TEST'.\n"
    "\n"
    "  -r, --delay <op-name>\n"
    "       Insert a delay before returning the success response for a particular\n"
    "       operation.\n"
    "\n"
    "  -t, --delay-time <ms>\n"
    "       Set the response delay time. Defaults to 10 seconds.\n"
    "\n"
    "  -p, --pairing-server <hostname>\n"
    "       Hostname/IP address of pairing server.\n"
    "\n"
    "  --wrm-pairing\n"
    "       Send PairDeviceToAccount message via WRM (default is TCP).\n"
    "\n"
    "  --pairing-endpoint-id\n"
    "       The node id of the pairing service endpoint.\n"
    "\n"
    "  --connect-to <addr>[:<port>][%<interface>]\n"
    "       Create a Weave connection to the specified address on start up. This\n"
    "       can be used to initiate a passive rendezvous with remote device manager.\n"
    "\n"
    "  --connect-to-interval <ms>\n"
    "       Interval at which to perform connect attempts to the connect-to address.\n"
    "       Defaults to 2 seconds.\n"
    "\n"
#if WEAVE_CONFIG_TIME
    "  --time-sync-server\n"
    "       Enable Time Sync Server.\n"
    "\n"
    "  --time-sync-client\n"
    "       Enable Time Sync Client.\n"
    "\n"
    "  --time-sync-coordinator\n"
    "       Enable Time Sync Coordinator.\n"
    "\n"
    "  --time-sync-mode-local\n"
    "       specify that the Time Client Sync mode is Local (time sync with local nodes via UDP)\n"
    "\n"
    "  --time-sync-mode-service\n"
    "       specify that the Time Client Sync mode is Service (time sync with Service via TCP)\n"
    "\n"
    "  --time-sync-mode-service-over-tunnel\n"
    "       specify that the Time Client Sync mode is Service (time sync with Service via WRM over a Tunnel)\n"
    "\n"
    "  --time-sync-mode-auto\n"
    "       specify that the Time Client Sync mode is Auto (time sync with via Multicast)\n"
    "\n"
    "  --ts-simple-client\n"
    "       Initiate the single source time sync client\n"
    "\n"
    "  --ts-server-node-id\n"
    "       Set server node id for the time sync client to send request to\n"
    "\n"
    "  --ts-server-node-addr\n"
    "       Set server node addr for the time sync client to send request to\n"
    "\n"
    "  --ts-server-subnet-id\n"
    "       Set subnet id for the time sync client to send request to\n"
    "\n"
#endif // WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_ENABLE_TUNNELING
    "  --tun-border-gw\n"
    "       Assume the role of a Border Gateway capable of Tunneling Weave data traffic.\n"
    "\n"
    "  --tun-mob-device\n"
    "       Assume the role of a Mobile Device capable of Tunneling Weave data traffic.\n"
    "\n"
    "  --tun-connect-to <addr>[:<port>][%<interface>]\n"
    "       Create a Tunnel Border gateway connection to the specified address on start up.\n"
    "\n"
    "  --tun-dest-node-id <num>\n"
    "       Node id for Tunnel peer node. Defaults to 0xc0ffee.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    "  --service-dir\n"
    "       Use service directory to lookup the destination node address for the tunnel server.\n"
    "\n"
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#if CONFIG_BLE_PLATFORM_BLUEZ
    "  --enable-bluez-peripheral\n"
    "       enable weave over bluez peripheral\n"
    "\n"
    "  --enable-inet\n"
    "       enable inet\n"
    "\n"
    "  --peripheral-name\n"
    "       Bluez periheral name\n"
    "\n"
    "  --peripheral-address\n"
    "       Bluez peripheral mac address\n"
    "\n"
#endif
    "  -C, --case\n"
    "       Use CASE to create an authenticated session and encrypt messages using\n"
    "       the negotiated session key.\n"
    "\n"
    "  --suppress-ac\n"
    "       Suppress access controls when responding to incoming requests.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    "  --wdm-publisher <publisher node id>\n"
    "       Configure the node ID for WDM Next publisher\n"
    "\n"
    "  --wdm-subnet <subnet of the publisher in hex>\n"
    "       Predefined service subnet ID is 5\n"
    "\n"
    "  --wdm-simple-view-client\n"
    "       Initiate a simple WDM Next view client\n"
    "\n"
    "  --wdm-simple-view-server\n"
    "       Initiate a simple WDM Next view server\n"
    "\n"
    "  --wdm-one-way-sub-client\n"
    "       Initiate a subscription to some WDM Next publisher\n"
    "\n"
    "  --wdm-one-way-sub-publisher\n"
    "       Respond to a number of WDM Next subscriptions as a publisher\n"
    "\n"
    "  --wdm-init-mutual-sub\n"
    "       Initiate a subscription to some WDM Next publisher, while publishing at the same time \n"
    "\n"
    "  --wdm-resp-mutual-sub\n"
    "       Respond to WDM Next subscription as a publisher with a mutual subscription\n"
    "\n"
    "  --wdm-liveness-check-period\n"
    "       Specify the time, in seconds, between liveness check in WDM Next subscription as a publisher\n"
    "\n"
    "  --wdm-enable-retry"
    "       Enable automatic retries by WDM\n"
    "\n"
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
// only for wdm next test
    "  --event-generator [None | Debug | Liveness | Security | Telemetry | TestTrait]\n"
    "       Generate structured Weave events using a particular generator:"
    "         None: no events\n"
    "         Debug: Freeform strings, from helloweave-app.  Uses debug_trait to emit messages at \n"
    "                   Production level\n"
    "         Liveness: Liveness events, using liveness_trait at Production level.\n"
    "         Security: Multi-trait scenario emitting events from debug_trait, open_close_trait,\n"
    "                   pincode_input_trait and bolt_lock_trait\n"
    "         Telemetry: WiFi telemetry events at Production level.\n"
    "         TestTrait: TestETrait events which cover a range of types.\n"
    "\n"
    "  --inter-event-period <ms>"
    "       Delay between emitting consecutive events (default 1s)\n"
    "\n"
    "  --test-case <test case id>\n"
    "       Further configure device behavior with this test case id\n"
    "\n"
    "  --enable-stop\n"
    "       Terminate WDM Next test in advance for Happy test\n"
    "\n"
    "  --total-count\n"
    "      when it is -1, mutate trait instance for unlimited iterations, when it is X,\n"
    "      mutate trait instance for X iterations\n"
    "\n"
    "  --final-status\n"
    "      When Final Status is\n"
    "      0: Client Cancel,\n"
    "      1: Publisher Cancel,\n"
    "      2: Client Abort,\n"
    "      3: Publisher Abort,\n"
    "      4: Idle\n"
    "\n"
    "  --enable-dictionary-test\n"
    "      Enable/disable dictionary test\n"
    "\n"
    "  --timer-period\n"
    "      Every timer-period, the mutate timer handler is triggered\n"
    "\n"
    "  --enable-flip <true|false|yes|no|1|0>\n"
    "      Enable/disable flip trait data in HandleDataFlipTimeout\n"
// only for wdm next test
    "\n"
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
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Generic Weave device simulator.\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gMockWdmNodeOptions,
    &gWRMPOptions,
    &gWeaveSecurityMode,
    &gCASEOptions,
    &gKeyExportOptions,
    &gDeviceDescOptions,
    &gServiceDirClientOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    &gGroupKeyEncOptions,
    NULL
};


int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    WeaveAuthMode authMode = kWeaveAuthMode_Unauthenticated;

#if CONFIG_BLE_PLATFORM_BLUEZ
    nl::Ble::Platform::BlueZ::BluezPeripheralArgs Bluez_PeripheralArgs;
#endif

#if WEAVE_CONFIG_TEST
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;
#endif

    InitToolCommon();

#if WEAVE_CONFIG_TEST
    SetupFaultInjectionContext(argc, argv);
#endif

    SetSignalHandler(DoneOnHandleSIGUSR1);

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets) ||
        !ResolveWeaveNetworkOptions(TOOL_NAME, gWeaveNodeOptions, gNetworkOptions))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();

    InitNetwork();

    InitWeaveStack(true, true);

#if WEAVE_CONFIG_ENABLE_TUNNELING
    if (TunnelDestAddr == IPAddress::Any)
    {
        TunnelDestAddr = FabricState.SelectNodeAddress(TunnelDestNodeId);
    }

    printf("Weave Node Configuration:\n");
    printf("  Fabric Id: %" PRIX64 "\n", FabricState.FabricId);
    printf("  Subnet Number: %X\n", FabricState.DefaultSubnet);
    printf("  Node Id: %" PRIX64 "\n", FabricState.LocalNodeId);

    if (TunnelConnectToAddr != NULL)
    {
        IPAddress::FromString(TunnelConnectToAddr, TunnelDestAddr);
    }

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    err = ServiceMgr.init(&ExchangeMgr, ServiceDirCache, sizeof(ServiceDirCache),
            GetRootServiceDirectoryEntry, kWeaveAuthMode_CASE_ServiceEndPoint,
            NULL, NULL, OverrideServiceConnectArguments);
    FAIL_ERROR(err, "ServiceMgr.Init failed");
#endif

    if (UseCASE)
        authMode = kWeaveAuthMode_CASE_AnyCert;

    if (tunnelingDeviceRole)
    {
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        if (UseServiceDirForTunnel)
        {
            err = TunAgent.Init(&Inet, &ExchangeMgr, TunnelDestNodeId,
                                authMode, &ServiceMgr,
                                "weave-tun0", tunnelingDeviceRole);
        }
        else
#endif
        {
            err = TunAgent.Init(&Inet, &ExchangeMgr, TunnelDestNodeId, TunnelDestAddr,
                                authMode,
                                "weave-tun0", tunnelingDeviceRole);
        }
        FAIL_ERROR(err, "TunnelAgent.Init failed");
        err = TunAgent.StartServiceTunnel();
        FAIL_ERROR(err, "TunnelAgent.StartServiceTunnel failed");
    }
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

    // Arrange to get called for various activity in the message layer.
    MessageLayer.OnConnectionReceived = HandleConnectionReceived;
    MessageLayer.OnReceiveError = HandleMessageReceiveError;
    MessageLayer.OnAcceptError = HandleAcceptConnectionError;

    SecurityMgr.OnSessionEstablished = HandleSecureSessionEstablished;
    SecurityMgr.OnSessionError = HandleSecureSessionError;

    // Initialize the EchoServer application.
    err = EchoServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "WeaveEchoServer.Init failed");

    // Arrange to get a callback whenever an Echo Request is received.
    EchoServer.OnEchoRequestReceived = HandleEchoRequestReceived;

    // Initialize the Heartbeat receiver
    err = HeartbeatReceiver.Init(&ExchangeMgr);
    FAIL_ERROR(err, "WeaveHeartbeatReceiver.Init failed");

    // Arrange to get a callback whenever a Heartbeat is received.
    HeartbeatReceiver.OnHeartbeatReceived = HandleHeartbeatReceived;

    // Initialize the mock network provisioning server
    err = MockNPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockNetworkProvisioningServer.Init failed");

    // Initialize the mock Dropcam legacy pairing server
    err = MockDCLPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockDropcamLegacyPairingServer.Init failed");

    // Initialize the mock service provisioning server
    err = MockSPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockServiceProvisioningServer.Init failed");
    MockSPServer.PairingTransport = PairingTransportArg;
    MockSPServer.PairingEndPointId = PairingEndPointIdArg;
    if (PairingServer != NULL)
    {
        MockSPServer.PairingServerAddr = PairingServer;
    }

    // Initialize the mock fabric provisioning server
    err = MockFPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockFabricProvisioningServer.Init failed");

    // Initialize the mock pairing server
    err = MockPairingEPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockPairingServer.Init failed");

    // Initialize the mock device description server.
    err = MockDDServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockDDServer.Init failed");

    // Initialize the mock device control server.
    err = MockDCServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockDCServer.Init failed");

    // Initialize the mock token pairing server.
    err = MockTPServer.Init(&ExchangeMgr);
    FAIL_ERROR(err, "MockTPServer.Init failed");


    InitializeEventLogging(&ExchangeMgr);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    switch (WdmRoleInTest)
    {
    case 0:
        break;
    case kToolOpt_WdmInitMutualSubscription:
    case kToolOpt_WdmSubscriptionClient:

        if (gMockWdmNodeOptions.mWdmPublisherNodeId != kAnyNodeId)
        {
            err = MockWdmSubscriptionInitiator::GetInstance()->Init(&ExchangeMgr,
                                                                    gGroupKeyEncOptions.GetEncKeyId(),
                                                                    gWeaveSecurityMode.SecurityMode,
                                                                    gMockWdmNodeOptions);
            FAIL_ERROR(err, "MockWdmSubscriptionInitiator.Init failed");
            err = MockWdmSubscriptionInitiator::GetInstance()->StartTesting(gMockWdmNodeOptions.mWdmPublisherNodeId,
                                                                            gMockWdmNodeOptions.mWdmUseSubnetId);
            FAIL_ERROR(err, "MockWdmSubscriptionInitiator.StartTesting failed");
            MockWdmSubscriptionInitiator::GetInstance()->onCompleteTest = HandleWdmCompleteTest;
        }
        else
        {
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            FAIL_ERROR(err, "MockWdmSubscriptionInitiator requires node ID to some publisher");
        }
        break;
    case kToolOpt_WdmRespMutualSubscription:
    case kToolOpt_WdmSubscriptionPublisher:
        if (EnableRetry)
        {
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            FAIL_ERROR(err, "MockWdmSubcriptionResponder is incompatible with --wdm-enable-retry");
        }

        err = MockWdmSubscriptionResponder::GetInstance()->Init(&ExchangeMgr,
                                                                gMockWdmNodeOptions);
        FAIL_ERROR(err, "MockWdmSubscriptionResponder.Init failed");
        MockWdmSubscriptionResponder::GetInstance()->onCompleteTest = HandleWdmCompleteTest;
        break;
    default:
        err = WEAVE_ERROR_INVALID_ARGUMENT;
        FAIL_ERROR(err, "WdmRoleInTest is invalid");
    };
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    if (Preconfig)
    {
        MockNPServer.Preconfig();
        MockFPServer.Preconfig();
        MockSPServer.Preconfig();
    }

    PrintNodeConfig();

    printf("  Pairing Server: %s\n", MockSPServer.PairingServerAddr);

    // If instructed to initiate a connection to a remote address, arm a timer that will
    // fire as soon as we enter the network service loop.
    if (ConnectToAddr != NULL)
    {
        err = SystemLayer.StartTimer(1, InitiateConnection, NULL);
        FAIL_ERROR(err, "SystemLayer.StartTimer failed");
    }

#if WEAVE_CONFIG_LEGACY_WDM
    // always set up a mock DM publisher

    err = MockDMPublisher.Init(&ExchangeMgr, kDefaultDMResponseTimeout);
    FAIL_ERROR(err, "could not start DM publisher");

    if (gEventGenerator != NULL)
    {
        printf("Starting Event Generator\n");
        MockEventGenerator::GetInstance()->Init(&ExchangeMgr, gEventGenerator, TimeBetweenEvents, true);
    }
#endif // WEAVE_CONFIG_LEGACY_WDM

#if WEAVE_CONFIG_TEST
    nl::Weave::Stats::UpdateSnapshot(before);
#endif

#if WEAVE_CONFIG_TIME
    // MockTimeNode is inited after taking the snapshot of Stats and shutdown
    // before the leak check because while it runs it allocs resources that are
    // freed only by its Shutdown method. Those resources would be counted as
    // leaked by ProcessStats.
    err = MockTimeNode.Init(&ExchangeMgr, TimeSyncServerNodeId, TimeSyncServerNodeAddr);
    FAIL_ERROR(err, "init_mock_time_sync failed");

    if (ShouldEnableSimpleTimeSyncClient)
    {
        err = simpleTimeSyncClient.Init(&ExchangeMgr, TimeSyncServerNodeId, TimeSyncServerSubnetId);
        FAIL_ERROR(err, "init_mock_simple_time_sync failed");
    }
#endif // WEAVE_CONFIG_TIME

    printf("Listening for requests...\n");

#if CONFIG_BLE_PLATFORM_BLUEZ
    if (EnableWeaveBluezPeripheral)
    {

        if (BleName != NULL && BleAddress != NULL)
        {
            printf("BLE Peripheral name is %s.\n", BleName);
            printf("BLE Peripheral mac address is %s.\n", BleAddress);
            Bluez_PeripheralArgs.bleName = BleName;
            Bluez_PeripheralArgs.bleAddress = BleAddress;
            Bluez_PeripheralArgs.bluezBleApplicationDelegate = getBluezApplicationDelegate();
            Bluez_PeripheralArgs.bluezBlePlatformDelegate = getBluezPlatformDelegate();
            int pthreadErr = 0;
            pthreadErr = pthread_create(&sBLEThread, NULL, WeaveBleIOLoop, (void *)&Bluez_PeripheralArgs);
            if (pthreadErr)
            {
                printf("pthread_create() failed for BLE IO thread, err: %d\n", pthreadErr);
                exit(EXIT_FAILURE);
            }

            printf("Weave stack IO loops is running\n");
        }
        else
        {
            printf("Expect BLE Peripheral name and BLE mac address\n");
            exit(EXIT_FAILURE);
        }
    }

#endif // CONFIG_BLE_PLATFORM_BLUEZ

    InitializeEventLogging(&ExchangeMgr);

    switch (gMockWdmNodeOptions.mWdmRoleInTest)
    {
        case 0:
            break;
        case kToolOpt_WdmInitMutualSubscription:
        case kToolOpt_WdmSubscriptionClient:

            if (gMockWdmNodeOptions.mWdmPublisherNodeId != kAnyNodeId)
            {
                err = MockWdmSubscriptionInitiator::GetInstance()->Init(&ExchangeMgr,
                                                                        gGroupKeyEncOptions.GetEncKeyId(),
                                                                        gWeaveSecurityMode.SecurityMode,
                                                                        gMockWdmNodeOptions);
                FAIL_ERROR(err, "MockWdmSubscriptionInitiator.Init failed");
                MockWdmSubscriptionInitiator::GetInstance()->onCompleteTest = HandleWdmCompleteTest;
                MockWdmSubscriptionInitiator::GetInstance()->onError = HandleError;

            }
            else
            {
                err = WEAVE_ERROR_INVALID_ARGUMENT;
                FAIL_ERROR(err, "MockWdmSubscriptionInitiator requires node ID to some publisher");
            }

            break;
        case kToolOpt_WdmRespMutualSubscription:
        case kToolOpt_WdmSubscriptionPublisher:
            if (gMockWdmNodeOptions.mEnableRetry)
            {
                err = WEAVE_ERROR_INVALID_ARGUMENT;
                FAIL_ERROR(err, "MockWdmSubcriptionResponder is incompatible with --enable-retry");
            }

            err = MockWdmSubscriptionResponder::GetInstance()->Init(&ExchangeMgr,
                                                                    gMockWdmNodeOptions
                                                                    );
            FAIL_ERROR(err, "MockWdmSubscriptionResponder.Init failed");
            MockWdmSubscriptionResponder::GetInstance()->onCompleteTest = HandleWdmCompleteTest;
            MockWdmSubscriptionResponder::GetInstance()->onError = HandleError;
            if (gTestWdmNextOptions.mClearDataSinkState)
            {
                MockWdmSubscriptionResponder::GetInstance()->ClearDataSinkState();
            }
            break;
        default:
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            FAIL_ERROR(err, "WdmRoleInTest is invalid");
    };

    for (uint32_t iteration = 1; iteration <= gTestWdmNextOptions.mTestIterations; iteration++) {

        switch (gMockWdmNodeOptions.mWdmRoleInTest) {
            case 0:
                break;
            case kToolOpt_WdmInitMutualSubscription:
            case kToolOpt_WdmSubscriptionClient:
                if (gTestWdmNextOptions.mClearDataSinkState) {
                    MockWdmSubscriptionInitiator::GetInstance()->ClearDataSinkState();
                }
                err = MockWdmSubscriptionInitiator::GetInstance()->StartTesting(gMockWdmNodeOptions.mWdmPublisherNodeId,
                                                                                gMockWdmNodeOptions.mWdmUseSubnetId);
                if (err != WEAVE_NO_ERROR) {
                    printf("\nMockWdmSubscriptionInitiator.StartTesting failed: %s\n", ErrorStr(err));
                    Done = true;
                }
                //FAIL_ERROR(err, "MockWdmSubscriptionInitiator.StartTesting failed");
                break;
            default:
                printf("TestWdmNext server is ready\n");
        };

        switch (gMockWdmNodeOptions.mEventGeneratorType) {
            case MockWdmNodeOptions::kGenerator_None:
                gEventGenerator = NULL;
                break;
            case MockWdmNodeOptions::kGenerator_TestDebug:
                gEventGenerator = GetTestDebugGenerator();
                break;
            case MockWdmNodeOptions::kGenerator_TestLiveness:
                gEventGenerator = GetTestLivenessGenerator();
                break;
            case MockWdmNodeOptions::kGenerator_TestSecurity:
                gEventGenerator = GetTestSecurityGenerator();
                break;
            case MockWdmNodeOptions::kGenerator_TestTelemetry:
                gEventGenerator = GetTestTelemetryGenerator();
                break;
            case MockWdmNodeOptions::kGenerator_TestTrait:
                gEventGenerator = GetTestTraitGenerator();
                break;
            case MockWdmNodeOptions::kGenerator_NumItems:
            default:
                gEventGenerator = NULL;
                break;
        }

        if (gEventGenerator != NULL) {
            printf("Starting Event Generator\n");
            MockEventGenerator::GetInstance()->Init(&ExchangeMgr, gEventGenerator,
                                                    gMockWdmNodeOptions.mTimeBetweenEvents, true);
        }

    }
    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

#if WEAVE_CONFIG_LEGACY_WDM
#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

        MockDMPublisher.Republish();

#endif //WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
#endif // WEAVE_CONFIG_LEGACY_WDM

    }

#if WEAVE_CONFIG_TIME
    MockTimeNode.Shutdown();
#endif // WEAVE_CONFIG_TIME

#if WEAVE_CONFIG_TEST
    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();
#endif

    SystemLayer.CancelTimer(InitiateConnection, NULL);
    MockNPServer.Shutdown();
    MockDCLPServer.Shutdown();
    EchoServer.Shutdown();
    HeartbeatReceiver.Shutdown();
    MockDCServer.Shutdown();
    MockDDServer.Shutdown();
    MockFPServer.Shutdown();
    MockPairingEPServer.Shutdown();
    MockTPServer.Shutdown();
    MockSPServer.Shutdown();
#if WEAVE_CONFIG_LEGACY_WDM
    MockDMPublisher.Finalize();
#endif

#if WEAVE_CONFIG_ENABLE_TUNNELING
    if (tunnelingDeviceRole)
    {
        TunAgent.Shutdown();
    }
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

    ShutdownWeaveStack();

    return EXIT_SUCCESS;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case 'p':
        PairingServer = arg;
        break;
    case 'c':
        Preconfig = true;
        break;
    case 'r':
        if (!OpActions.SetDelay(arg, RespDelayTime))
        {
            PrintArgError("%s: Invalid value specified for response delay name: %s\n", progName, arg);
            return false;
        }
        break;
    case 't':
        if (!ParseInt(arg, RespDelayTime))
        {
            PrintArgError("%s: Invalid value specified for response delay time: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_PairViaWRM:
        PairingTransportArg = kPairingTransport_WRM;
        break;
    case kToolOpt_PairingEndPointId:
        if (!ParseNodeId(arg, PairingEndPointIdArg))
        {
            PrintArgError("%s: Invalid value specified for pairing endpoint node id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_ConnectTo:
        ConnectToAddr = arg;
        break;
    case kToolOpt_ConnectToInterval:
        if (!ParseInt(arg, ConnectIntervalMS))
        {
            PrintArgError("%s: Invalid value specified for connect-to interval: %s\n", progName, arg);
            return false;
        }
        break;
#if WEAVE_CONFIG_TIME
    case kToolOpt_TimeSyncServer:
        MockTimeNode.SetRole(kMockTimeSyncRole_Server);
        break;
    case kToolOpt_TimeSyncClient:
        MockTimeNode.SetRole(kMockTimeSyncRole_Client);
        break;
    case kToolOpt_TimeSyncCoordinator:
        MockTimeNode.SetRole(kMockTimeSyncRole_Coordinator);
        break;
    case kToolOpt_TimeSyncModeLocal:
        MockTimeNode.SetMode(kOperatingMode_AssignedLocalNodes);
        break;
    case kToolOpt_TimeSyncModeService:
        MockTimeNode.SetMode(kOperatingMode_Service);
        break;
    case kToolOpt_TimeSyncModeServiceOverTunnel:
        MockTimeNode.SetMode(kOperatingMode_ServiceOverTunnel);
        break;
    case kToolOpt_TimeSyncModeAuto:
        MockTimeNode.SetMode(kOperatingMode_Auto);
        break;
    case kToolOpt_TimeSyncSimpleClient:
        ShouldEnableSimpleTimeSyncClient = true;
        break;
    case kToolOpt_TimeSyncServerNodeId:
        if (!ParseNodeId(arg, TimeSyncServerNodeId))
        {
            PrintArgError("%s: Invalid value specified for TimeSyncServer node id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_TimeSyncServerNodeAddr:
        TimeSyncServerNodeAddr = arg;
        break;
    case kToolOpt_TimeSyncServerSubnetId:
        if (!ParseSubnetId(arg, TimeSyncServerSubnetId))
        {
            PrintArgError("%s: Invalid value specified for TimeSyncServer subnet id: %s\n", progName, arg);
            return false;
        }
        break;
#endif // WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_ENABLE_TUNNELING
    case kToolOpt_TunnelBorderGw:
        tunnelingDeviceRole = kClientRole_BorderGateway;
        break;
    case kToolOpt_TunnelMobDevice:
        tunnelingDeviceRole = kClientRole_MobileDevice;
        break;
    case kToolOpt_TunnelConnectAddr:
        TunnelConnectToAddr = arg;
        break;
    case kToolOpt_TunnelDestNodeId:
        if (!ParseNodeId(arg, TunnelDestNodeId))
        {
            PrintArgError("%s: Invalid value specified for hush target node id: %s\n", progName, arg);
            return false;
        }
        break;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kToolOpt_UseServiceDir:
        UseServiceDirForTunnel = true;
        break;
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#if CONFIG_BLE_PLATFORM_BLUEZ
    case kToolOpt_EnableWeaveBluezPeripheral:
        EnableWeaveBluezPeripheral = true;
        break;
    case kToolOpt_WeaveBluezPeripheralName:
        BleName = strdup(arg);
        break;
    case kToolOpt_WeaveBluezPeripheralAddress:
        BleAddress = strdup(arg);
        break;
#endif
    case 'C':
        UseCASE = true;
        break;
    case kToolOpt_SuppressAccessControl:
        sSuppressAccessControls = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Echo Request from node %" PRIX64 " (%s): len=%u ... sending response.\n", nodeId, ipAddrStr,
            payload->DataLength());

    if (Debug)
        DumpMemory(payload->Start(), payload->DataLength(), "    ", 16);
}

void HandleHeartbeatReceived(const WeaveMessageInfo *aMsgInfo, uint8_t nodeState, WEAVE_ERROR err)
{
    char ipAddrStr[64];
    aMsgInfo->InPacketInfo->SrcAddress.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Heartbeat from node %" PRIX64 " (%s): state=%u, err=%s\n", aMsgInfo->SourceNodeId, ipAddrStr, nodeState, ErrorStr(err));
}

void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    char ipAddrStr[64];
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection received from node %" PRIX64 " (%s)\n", con->PeerNodeId, ipAddrStr);

    con->OnConnectionClosed = HandleConnectionClosed;
}

void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    char ipAddrStr[64] = "";

    if (con)
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Secure session established with node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport)
{
    char ipAddrStr[64] = "";

    if (con)
    {
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        con->Close();
    }

    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        printf("FAILED to establish secure session with node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, nl::StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    else
        printf("FAILED to establish secure session with node %" PRIX64 " (%s): %s\n", peerNodeId, ipAddrStr, ErrorStr(localErr));
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

void InitiateConnection(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err;
    WeaveConnection *con;

    con = MessageLayer.NewConnection();
    if (con == NULL)
        FAIL_ERROR(WEAVE_ERROR_NO_MEMORY, "MessageLayer.NewConnection failed");

    con->OnConnectionComplete = HandleConnectionComplete;
    con->OnConnectionClosed = HandleConnectionClosed;

    err = con->Connect(kNodeIdNotSpecified, kWeaveAuthMode_Unauthenticated, ConnectToAddr, strlen(ConnectToAddr), WEAVE_UNSECURED_PORT);
    if (err != WEAVE_NO_ERROR)
        HandleConnectionComplete(con, err);
}

void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR err)
{
    if (err == WEAVE_NO_ERROR)
    {
        char ipAddrStr[64];
        con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        printf("Connection established to %s\n", ConnectToAddr);

        ExchangeMgr.AllowUnsolicitedMessages(con);
    }
    else
    {
        printf("Failed to establish connection to %s: %s\n", ConnectToAddr, ErrorStr(err));
        con->Close();

        err = SystemLayer.StartTimer(ConnectIntervalMS, InitiateConnection, NULL);
        FAIL_ERROR(err, "SystemLayer.StartTimer failed");
    }
}

static void HandleWdmCompleteTest()
{
    if (EnableStopTest)
    {
        Done = true;
    }
}

static void HandleError()
{
    Done = true;
}

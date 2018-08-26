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
 *       A tool for exercising the Weave Binding interface.
 *
 */

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <signal.h>

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveBinding.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/echo/WeaveEcho.h>

#define VerifyOrQuit(TST) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "CHECK FAILED: %s at %s:%d\n", #TST, __FUNCTION__, __LINE__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define TOOL_NAME "TestBinding"

enum TestMode
{
    kTestMode_Sequential,
    kTestMode_Simultaneous,
};

class BindingTestDriver
{
public:
    nl::Weave::Binding *binding;
    ExchangeContext *ec;
    uint32_t echosSent;
    bool defaultCheckDelivered;

    BindingTestDriver()
    {
        binding = NULL;
        ec = NULL;
        echosSent = 0;
        defaultCheckDelivered = false;
    }

    void Start(uint32_t startDelay);

private:
    void DoStart();
    void PrepareBinding();
    void SendEcho();
    void Complete();
    void Failed(WEAVE_ERROR err, const char *desc);

    static void AsyncDoStart(System::Layer* aLayer, void* aAppState, System::Error aError);
    static void DoOnDemandPrepare(System::Layer* aLayer, void* aAppState, System::Error aError);
    static void BindingEventCallback(void *apAppState, Binding::EventType aEvent, const Binding::InEventParam& aInParam, Binding::OutEventParam& aOutParam);
    static void SendDelayComplete(System::Layer* aLayer, void* aAppState, System::Error aError);
    static void OnEchoResponseReceived(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload);
    static void OnResponseTimeout(ExchangeContext *ec);
    static void OnMessageSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt);
};

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static bool ParseDestAddress(const char *destAddr);
static void StartTest();

static bool gUseTCP = true;
static bool gUseUDP = false;
static bool gUseWRMP = false;
static uint64_t gDestNodeId = kNodeIdNotSpecified;
static char gDestHost[NL_DNS_HOSTNAME_MAX_LEN + 10];  // +10 for padding characters
static uint16_t gDestHostLen = 0;
static uint16_t gDestPort = WEAVE_PORT;
static InterfaceId gDestIntf = INET_NULL_INTERFACEID;
static uint32_t gTestCount = 1;
static uint32_t gEchoCount = 5;
static uint32_t gEchoSendDelay = 100; // in ms
static uint32_t gEchoResponseTimeout = 5000; // in ms
static uint32_t gStartDelay = 0; // in ms
static bool gOnDemandPrepare = false;
static bool gCloseBindingDuringRequest = false;
static uint8_t gDNSOptions = ::nl::Inet::kDNSOption_Default;

static TestMode gSelectedTestMode = kTestMode_Sequential;
static uint32_t gTestDriversStarted = 0;
static uint32_t gTestDriversActive = 0;

enum
{
    kToolOpt_EchoCount               = 1000,
    kToolOpt_EchoResponseTimeout     = 1001,
    kToolOpt_OnDemandPrepare         = 1002,
    kToolOpt_StartDelay              = 1003,
    kToolOpt_DNSOptions              = 1004,
};

static OptionDef gToolOptionDefs[] =
{
    { "test-mode",              kArgumentRequired, 'm'                          },
    { "test-count",             kArgumentRequired, 'C'                          },
    { "echo-count",             kArgumentRequired, kToolOpt_EchoCount           },
    { "resp-timeout",           kArgumentRequired, kToolOpt_EchoResponseTimeout },
    { "on-demand-prepare",      kNoArgument,       kToolOpt_OnDemandPrepare     },
    { "start-delay",            kArgumentRequired, kToolOpt_StartDelay          },
    { "dest-addr",              kArgumentRequired, 'D'                          },
    { "tcp",                    kNoArgument,       't'                          },
    { "udp",                    kNoArgument,       'u'                          },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "wrmp",                   kNoArgument,       'w'                          },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "dns-options",            kArgumentRequired, kToolOpt_DNSOptions          },
    { NULL }
};

static const char *const gToolOptionHelp =
    "  -m, --test-mode <mode>\n"
    "       Binding test mode.  The following modes are available:\n"
    "\n"
    "         sequential   -- Perform test sequences sequentially.\n"
    "         simultaneous -- Perform test sequences simultaneously.\n"
    "\n"
    "  -C, --test-count <int>\n"
    "       The number of test sequences to be executed. Defaults to 1.\n"
    "\n"
    "  --echo-count <int>\n"
    "       The number of Echo requests to be sent. Defaults to 5.\n"
    "\n"
    "  --resp-timeout <ms>\n"
    "       The amount of time to wait for an echo response from the peer. Defaults\n"
    "       to 5 seconds.\n"
    "\n"
    "  --on-demand-prepare\n"
    "       Test the \"on demand\" prepare pattern using the Binding::RequestPrepare() method.\n"
    "\n"
    "  --start-delay <ms>\n"
    "       The amount of time to wait between performing test sequences. In sequential mode\n"
    "       this value governs the time between the end of one test sequence and the start of\n"
    "       the next. In simultaneous mode, this value governs the time between the initiation\n"
    "       of individual test sequences, which may overlap in execution thereafter.\n"
    "       Defaults to 0 ms.\n"
    "\n"
    "  -D, --dest-addr <host-or-ip-addr>[:<port>][%<interface>]\n"
    "       Send echo requests to the peer at the specified host name/address, port number and\n"
    "       interface.\n"
    "\n"
    "       NOTE: When specifying a port number with an IPv6 address, the IPv6 address\n"
    "       must be enclosed in brackets, e.g. [fd00:0:1:1::1]:11095.\n"
    "\n"
    "  -t, --tcp\n"
    "       Use TCP to interact with the peer. This is the default.\n"
    "\n"
    "  -u, --udp\n"
    "       Use UDP to interact with the peer.\n"
    "\n"
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    "  -w, --wrmp\n"
    "       Use UDP with Weave Reliable Messaging to interact with the peer.\n"
    "\n"
#endif
    "  --dns-options <dns-options>\n"
    "       Use the specified DNS options when resolving hostnames.  <dns-options> can be one\n"
    "       of the following keywords:\n"
    "           Any (the default)\n"
    "              - Resolve IPv4 and/or IPv6 addresses in the native order\n"
    "                returned by the name server.\n"
    "           IPv4Only\n"
    "              - Resolve IPv4 addresses only.\n"
    "           IPv6Only\n"
    "              - Resolve IPv6 addresses only.\n"
    "           IPv4Preferred\n"
    "              - Resolve IPv4 and/or IPv6 addresses, with IPv4 addresses\n"
    "                given preference over IPv6.\n"
    "           IPv6Preferred\n"
    "              - Resolve IPv4 and/or IPv6 addresses, with IPv6 addresses\n"
    "                given preference over IPv4.\n"
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
    "Usage: " TOOL_NAME " [<options...>] <dest-node-id>[@<ip-addr>[:<port>][%<interface>]]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT,
    "Test the Weave Binding interface.\n"
    "\n"
    "This tool performs one or more test sequences involving the use of a Weave Binding object.\n"
    "Each test sequence performs the following steps:\n"
    "\n"
    "    - Create and prepare a Binding object\n"
    "    - Use the binding to send and receive a sequence of Weave Echo request/responses\n"
    "    - Close the binding\n"
    "\n"
    "Command line options can be used to configure the behavior of the test sequence and/or\n"
    "introduce failures.  At each step, various checks are made to ensure correct operation\n"
    "of the Binding object.\n"
    "\n"
    "The " TOOL_NAME " tool is typically used in conjunction with the weave-ping tool acting\n"
    "as a responder.\n"
    "\n"
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
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

int main(int argc, char *argv[])
{
#if WEAVE_CONFIG_TEST
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
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

    switch (gWeaveSecurityMode.SecurityMode)
    {
    case WeaveSecurityMode::kNone:
    case WeaveSecurityMode::kCASE:
    case WeaveSecurityMode::kCASEShared:
    case WeaveSecurityMode::kGroupEnc:
        break;
    default:
        printf("ERROR: Unsupported security mode specified\n");
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(false, true);

#if WEAVE_CONFIG_TEST
    nl::Weave::Stats::UpdateSnapshot(before);

    for (uint32_t iter = 0; iter < gFaultInjectionOptions.TestIterations; iter++)
    {
        printf("FI Iteration %u\n", iter);
#endif // WEAVE_CONFIG_TEST

        StartTest();
        ServiceNetworkUntil(&Done, NULL);

#if WEAVE_CONFIG_TEST
        if (gSigusr1Received)
        {
            printf("Sigusr1Received\n");
            break;
        }
    }

    ProcessStats(before, after, true, NULL);
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
    case 'm':
        if (strcasecmp(arg, "sequential") == 0)
        {
            gSelectedTestMode = kTestMode_Sequential;
        }
        else if (strcasecmp(arg, "simultaneous") == 0)
        {
            gSelectedTestMode = kTestMode_Simultaneous;
        }
        else
        {
            PrintArgError("%s: Invalid value specified for test mode: %s\n", progName, arg);
            return false;
        }
        break;
    case 'C':
        if (!ParseInt(arg, gTestCount))
        {
            PrintArgError("%s: Invalid value specified for test count: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_EchoCount:
        if (!ParseInt(arg, gEchoCount))
        {
            PrintArgError("%s: Invalid value specified for echo count: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_EchoResponseTimeout:
        if (!ParseInt(arg, gEchoResponseTimeout))
        {
            PrintArgError("%s: Invalid value specified for response timeout: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_OnDemandPrepare:
        gOnDemandPrepare = true;
        break;
    case kToolOpt_StartDelay:
        if (!ParseInt(arg, gStartDelay))
        {
            PrintArgError("%s: Invalid value specified for start delay: %s\n", progName, arg);
            return false;
        }
        break;
    case 't':
        gUseTCP = true;
        gUseUDP = false;
        gUseWRMP = false;
        break;
    case 'u':
        gUseTCP = false;
        gUseUDP = true;
        gUseWRMP = false;
        break;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    case 'w':
        gUseTCP = false;
        gUseUDP = false;
        gUseWRMP = true;
        break;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    case 'D':
        if (!ParseDestAddress(arg))
        {
            PrintArgError("%s: Invalid value specified for destination address: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_DNSOptions:
        if (!ParseDNSOptions(progName, name, arg, gDNSOptions))
        {
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
    if (argc == 0)
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
    char *destAddr = (char *)strchr(nodeId, '@');
    if (destAddr != NULL)
    {
        *destAddr = 0;
        destAddr++;
    }

    if (!ParseNodeId(nodeId, gDestNodeId))
    {
        PrintArgError("%s: Invalid value specified for destination node-id: %s\n", progName, nodeId);
        return false;
    }

    if (destAddr != NULL && !ParseDestAddress(destAddr))
    {
        PrintArgError("%s: Invalid value specified for destination address: %s\n", progName, destAddr);
        return false;
    }

    return true;
}

bool ParseDestAddress(const char *destAddr)
{
    const char *host;

    if (ParseHostAndPort(destAddr, strlen(destAddr), host, gDestHostLen, gDestPort) != WEAVE_NO_ERROR)
    {
        return false;
    }

    if (gDestHostLen > NL_DNS_HOSTNAME_MAX_LEN)
    {
        return false;
    }

    // Pad the host name buffer with extraneous characters to verify that the Binding API properly
    // honors the host name length argument.
    memset(gDestHost, '0', sizeof(gDestHost));
    memcpy(gDestHost, host, gDestHostLen);

    return true;
}

void StartTest()
{
    BindingTestDriver *bindingProc;

    gTestDriversStarted = 0;
    gTestDriversActive = 0;

    switch (gSelectedTestMode)
    {
    case kTestMode_Sequential:
        bindingProc = new BindingTestDriver();
        bindingProc->Start(0);
        break;
    case kTestMode_Simultaneous:
        for (uint32_t i = 0; i < gTestCount; i++)
        {
            bindingProc = new BindingTestDriver();
            bindingProc->Start(gStartDelay * i);
        }
        break;
    }
}

void BindingTestDriver::Start(uint32_t startDelay)
{
    if (startDelay == 0)
    {
        DoStart();
    }
    else
    {
        SystemLayer.StartTimer(startDelay, AsyncDoStart, this);
    }
}

void BindingTestDriver::AsyncDoStart(System::Layer* aLayer, void* aAppState, System::Error aError)
{
    BindingTestDriver *_this = (BindingTestDriver *)aAppState;
    _this->DoStart();
}

void BindingTestDriver::DoStart()
{
    gTestDriversStarted++;
    gTestDriversActive++;

    // Construct a new binding object.
    binding = ExchangeMgr.NewBinding(BindingEventCallback, this);
    if (binding == NULL)
    {
        Failed(WEAVE_ERROR_NO_MEMORY, "WeaveExchangeManager::NewBinding() failed");
        return;
    }

    VerifyOrQuit(binding->GetState() == Binding::kState_NotConfigured);
    VerifyOrQuit(!binding->IsReady());
    VerifyOrQuit(!binding->IsPreparing());
    VerifyOrQuit(binding->CanBePrepared());

    VerifyOrQuit(defaultCheckDelivered);

    if (gOnDemandPrepare)
    {
        SystemLayer.ScheduleWork(DoOnDemandPrepare, this);
    }
    else
    {
        PrepareBinding();
    }
}

void BindingTestDriver::PrepareBinding()
{
    WEAVE_ERROR err;

    VerifyOrQuit(binding->CanBePrepared());

    // Begin configuring the binding.
    Binding::Configuration bindingConf = binding->BeginConfiguration();

    VerifyOrQuit(binding->GetState() == Binding::kState_Configuring);

    // Configure the target node id
    bindingConf.Target_NodeId(gDestNodeId);

    // Configure the target address.
    if (gDestHostLen != 0)
    {
        bindingConf.TargetAddress_IP(gDestHost, gDestHostLen, gDestPort, gDestIntf);
    }

    // Configure the transport.
    if (gUseTCP)
    {
        bindingConf.Transport_TCP();
    }
    else if (gUseUDP)
    {
        bindingConf.Transport_UDP();
    }
    else if (gUseWRMP)
    {
        bindingConf.Transport_UDP_WRM();
        bindingConf.Transport_DefaultWRMPConfig(gWRMPOptions.GetWRMPConfig());
    }

    bindingConf.DNS_Options(gDNSOptions);

    // Configure the security mode.
    switch (gWeaveSecurityMode.SecurityMode)
    {
    case WeaveSecurityMode::kNone:
    default:
        bindingConf.Security_None();
        break;
    case WeaveSecurityMode::kCASE:
        bindingConf.Security_CASESession();
        break;
    case WeaveSecurityMode::kCASEShared:
        bindingConf.Security_SharedCASESession();
        break;
    case WeaveSecurityMode::kGroupEnc:
        bindingConf.Security_Key(gGroupKeyEncOptions.GetEncKeyId());
        break;
    }

    bindingConf.Exchange_ResponseTimeoutMsec(gEchoResponseTimeout);

    // Prepare the binding.
    err = bindingConf.PrepareBinding();
    if (err != WEAVE_NO_ERROR)
    {
        Failed(err, "Binding::Configuration::PrepareBinding() failed");
        return;
    }

    VerifyOrQuit(binding->GetState() != Binding::kState_Configuring);
}

void BindingTestDriver::DoOnDemandPrepare(System::Layer* aLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err;
    BindingTestDriver *_this = (BindingTestDriver *)aAppState;

    err = _this->binding->RequestPrepare();
    if (err != WEAVE_NO_ERROR)
    {
        _this->Failed(err, "Binding::RequestPrepare() failed");
    }
}

void BindingTestDriver::BindingEventCallback(void *apAppState, Binding::EventType aEvent, const Binding::InEventParam& aInParam, Binding::OutEventParam& aOutParam)
{
    BindingTestDriver *_this = (BindingTestDriver *)apAppState;
    Binding *binding = aInParam.Source;

    switch (aEvent)
    {
    case Binding::kEvent_ConnectionEstablished:
        VerifyOrQuit(binding->GetState() == Binding::kState_PreparingTransport_TCPConnect);
        VerifyOrQuit(!binding->IsReady());
        VerifyOrQuit(binding->IsPreparing());
        VerifyOrQuit(!binding->CanBePrepared());
        VerifyOrQuit(binding->GetConnection() != NULL);
        break;
    case Binding::kEvent_BindingReady:
        VerifyOrQuit(binding->GetState() == Binding::kState_Ready);
        VerifyOrQuit(binding->IsReady());
        VerifyOrQuit(!binding->IsPreparing());
        VerifyOrQuit(!binding->CanBePrepared());
        _this->SendEcho();
        break;
    case Binding::kEvent_PrepareFailed:
        VerifyOrQuit(binding->GetState() == Binding::kState_Failed);
        VerifyOrQuit(!binding->IsReady());
        VerifyOrQuit(!binding->IsPreparing());
        VerifyOrQuit(binding->CanBePrepared());
        _this->Failed(aInParam.PrepareFailed.Reason, "Prepare failed");
        break;
    case Binding::kEvent_BindingFailed:
        VerifyOrQuit(binding->GetState() == Binding::kState_Failed);
        VerifyOrQuit(!binding->IsReady());
        VerifyOrQuit(!binding->IsPreparing());
        VerifyOrQuit(binding->CanBePrepared());
        _this->Failed(aInParam.PrepareFailed.Reason, "Binding failed");
        break;
    case Binding::kEvent_PrepareRequested:
        VerifyOrQuit(binding->GetState() == Binding::kState_NotConfigured);
        VerifyOrQuit(!binding->IsReady());
        VerifyOrQuit(!binding->IsPreparing());
        VerifyOrQuit(binding->CanBePrepared());
        _this->PrepareBinding();
        break;
    case Binding::kEvent_DefaultCheck:
        _this->defaultCheckDelivered = true;
        binding->DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
        break;
    default:
        fprintf(stderr, "UNEXPECTED BINDING EVENT: %d\n", (int)aEvent);
        exit(EXIT_FAILURE);
    }
}

void BindingTestDriver::SendEcho()
{
    WEAVE_ERROR err;
    PacketBuffer *msgBuf = NULL;

    err = binding->NewExchangeContext(ec);
    if (err != WEAVE_NO_ERROR)
    {
        Failed(err, "Binding::NewExchangeContext() failed");
        return;
    }

    ec->AppState = this;

    ec->OnMessageReceived = OnEchoResponseReceived;
    ec->OnResponseTimeout = OnResponseTimeout;
    ec->OnSendError = OnMessageSendError;

    // Allocate a buffer for the echo request message
    msgBuf = PacketBuffer::NewWithAvailableSize(0);
    if (msgBuf == NULL)
    {
        Failed(WEAVE_ERROR_NO_MEMORY, "PacketBuffer::NewWithAvailableSize() failed");
        return;
    }

    // Send the null message
    err = ec->SendMessage(nl::Weave::Profiles::kWeaveProfile_Echo,
                          nl::Weave::Profiles::kEchoMessageType_EchoRequest,
                          msgBuf,
                          ExchangeContext::kSendFlag_ExpectResponse);
    if (err != WEAVE_NO_ERROR)
    {
        Failed(err, "ExchangeContext::SendMessage() failed");
        return;
    }

    if (gCloseBindingDuringRequest)
    {
        binding->Close();
        binding = NULL;
    }
}

void BindingTestDriver::SendDelayComplete(System::Layer* aLayer, void* aAppState, System::Error aError)
{
    BindingTestDriver *_this = (BindingTestDriver *)aAppState;
    _this->SendEcho();
}

void BindingTestDriver::OnEchoResponseReceived(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
        uint8_t msgType, PacketBuffer *payload)
{
    BindingTestDriver *_this = (BindingTestDriver *)ec->AppState;

    PacketBuffer::Free(payload);

    ec->Close();
    _this->ec = NULL;

    _this->echosSent++;

    if (_this->binding != NULL)
    {
        VerifyOrQuit(_this->binding->IsAuthenticMessageFromPeer(msgInfo));

        if (_this->echosSent < gEchoCount)
        {
            if (gEchoSendDelay == 0)
            {
                _this->SendEcho();
            }
            else
            {
                SystemLayer.StartTimer(gEchoSendDelay, SendDelayComplete, _this);
            }
        }
        else
        {
            _this->Complete();
        }
    }
    else
    {
        _this->Complete();
    }
}

void BindingTestDriver::Complete()
{
    gTestDriversActive--;

    if (binding != NULL)
    {
        binding->Close();
        binding = NULL;
    }

    switch (gSelectedTestMode)
    {
    case kTestMode_Sequential:
        if (gTestDriversStarted < gTestCount)
        {
            BindingTestDriver *testDriver = new BindingTestDriver();
            testDriver->Start(gStartDelay);
        }
        else
        {
            Done = true;
        }
        break;
    case kTestMode_Simultaneous:
        if (gTestDriversStarted == gTestCount && gTestDriversActive == 0)
        {
            Done = true;
        }
        break;
    }

    delete this;
}

void BindingTestDriver::Failed(WEAVE_ERROR err, const char *reason)
{
    gTestDriversActive--;

    if (binding != NULL)
    {
        binding->Close();
        binding = NULL;
    }

    if (ec != NULL)
    {
        ec->Abort();
        ec = NULL;
    }

    fprintf(stderr, "Test Failed: %s: %s\n", reason, ErrorStr(err));
    exit(EXIT_FAILURE);
}

void BindingTestDriver::OnResponseTimeout(ExchangeContext *ec)
{
    BindingTestDriver *_this = (BindingTestDriver *)ec->AppState;
    _this->Failed(WEAVE_ERROR_TIMEOUT, "Failed to receive response for Echo request");
}

void BindingTestDriver::OnMessageSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt)
{
    BindingTestDriver *_this = (BindingTestDriver *)ec->AppState;
    _this->Failed(err, "Failed to receive ACK for Echo request");
}

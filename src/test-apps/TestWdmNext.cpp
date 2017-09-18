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
 *      This file implements test app for the Weave Data Management(WDM) Next Profile.
 *
 *
 */

#define __STDC_FORMAT_MACROS

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/WeaveVersion.h>
#include "MockWdmViewClient.h"
#include "MockLoggingManager.h"
#include "MockWdmSubscriptionInitiator.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <SystemLayer/SystemTimer.h>
#include <time.h>
#include "MockLoggingManager.h"

#include "MockWdmSubscriptionInitiator.h"
#include "MockWdmSubscriptionResponder.h"
#include "WdmNextPerfUtility.h"

using nl::Inet::IPAddress;
using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using nl::Weave::WeaveExchangeManager;

#define TOOL_NAME "TestWdmNext"

static bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static void HandleWdmCompleteTest();
static void HandleError();

char * TestCaseId = NULL;
bool EnableStopTest = false;
bool EnableRetry = false;
bool EnableMockTimestampInitialCounter = false;
char * NumDataChangeBeforeCancellation = NULL;
char * FinalStatus = NULL;
char * TimeBetweenDataChangeMsec = NULL;
uint32_t TestIterations = 1;
int WdmRoleInTest = 0;
uint32_t TestDelayBetweenIterationMsec = 0;
bool EnableDataFlip = true;
bool EnableDictionaryTest = false;
char *  TimeBetweenLivenessCheckSec = NULL;
bool SavePerfData = false;
bool gClearDataSinkState = false;
// events
EventGenerator * gEventGenerator = NULL;

int TimeBetweenEvents = 1000;

uint64_t WdmPublisherNodeId = kAnyNodeId;
uint16_t WdmUseSubnetId = kWeaveSubnetId_NotSpecified;

enum
{
    kToolOpt_WdmPublisherNodeId                        = 1000,  // Specify the node ID of the WDM Publisher we should connect to
    kToolOpt_WdmUseSubnetId,                                    // True if the publisher is within the specified subnet
    //kToolOpt_WdmSimpleViewClient,
    //kToolOpt_WdmSimpleViewServer,
    kToolOpt_WdmSubscriptionClient,
    kToolOpt_WdmSubscriptionPublisher,
    kToolOpt_WdmInitMutualSubscription,
    kToolOpt_WdmRespMutualSubscription,
    kToolOpt_TestCaseId,
    kToolOpt_EnableStopTest,
    kToolOpt_NumDataChangeBeforeCancellation,
    kToolOpt_FinalStatus,
    kToolOpt_TimeBetweenDataChangeMsec,
    kToolOpt_TestIterations,
    kToolOpt_TestDelayBetweenIterationMsec,
    kToolOpt_EnableDataFlip,
    kToolOpt_EnableDictionaryTest,
    kToolOpt_SavePerfData,
    kToolOpt_EventGenerator,
    kToolOpt_TimeBetweenEvents,
    kToolOpt_ClearDataSinkStateBetweenTests,
    kToolOpt_TimeBetweenLivenessCheckSec,
    kToolOpt_WdmEnableRetry,
    kToolopt_EnableMockTimestampInitialCounter
};

static OptionDef gToolOptionDefs[] =
{
    { "test-case",                                      kArgumentRequired,  kToolOpt_TestCaseId },
    { "enable-stop",                                    kNoArgument,        kToolOpt_EnableStopTest },
    { "total-count",                                    kArgumentRequired,  kToolOpt_NumDataChangeBeforeCancellation },
    { "final-status",                                   kArgumentRequired,  kToolOpt_FinalStatus },
    { "timer-period",                                   kArgumentRequired,  kToolOpt_TimeBetweenDataChangeMsec },
    { "test-iterations",                                kArgumentRequired,  kToolOpt_TestIterations },
    { "test-delay",                                     kArgumentRequired,  kToolOpt_TestDelayBetweenIterationMsec },
    { "enable-flip",                                    kArgumentRequired,  kToolOpt_EnableDataFlip },
    { "enable-dictionary-test",                         kNoArgument,        kToolOpt_EnableDictionaryTest },
    { "save-perf",                                      kNoArgument,        kToolOpt_SavePerfData },
    { "event-generator",                                kArgumentRequired,  kToolOpt_EventGenerator },
    { "inter-event-period",                             kArgumentRequired,  kToolOpt_TimeBetweenEvents },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { "wdm-publisher",                                  kArgumentRequired,  kToolOpt_WdmPublisherNodeId },
    { "wdm-subnet",                                     kArgumentRequired,  kToolOpt_WdmUseSubnetId },
    //{ "wdm-simple-view-client",                       kNoArgument,        kToolOpt_WdmSimpleViewClient },
    //{ "wdm-simple-view-server",                       kNoArgument,        kToolOpt_WdmSimpleViewServer },
    { "wdm-one-way-sub-client",                         kNoArgument,        kToolOpt_WdmSubscriptionClient },
    { "wdm-one-way-sub-publisher",                      kNoArgument,        kToolOpt_WdmSubscriptionPublisher },
    { "wdm-init-mutual-sub",                            kNoArgument,        kToolOpt_WdmInitMutualSubscription },
    { "wdm-resp-mutual-sub",                            kNoArgument,        kToolOpt_WdmRespMutualSubscription },
    { "clear-state-between-iterations",                 kNoArgument,        kToolOpt_ClearDataSinkStateBetweenTests },
    { "wdm-liveness-check-period",                      kArgumentRequired,  kToolOpt_TimeBetweenLivenessCheckSec },
    { "enable-retry",                                   kNoArgument,        kToolOpt_WdmEnableRetry },
    { "enable-mock-event-timestamp-initial-counter",    kNoArgument,        kToolopt_EnableMockTimestampInitialCounter },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    { NULL }
};

static const char *const gToolOptionHelp =
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
    "  --timer-period\n"
    "      Every timer-period, the timer handler is triggered to mutate the trait instance\n"
    "\n"
    "  --test-iterations\n"
    "      control the number of wdm test iterations\n"
    "\n"
    "  --clear-state-between-iterations\n"
    "      Clear data sink state between WDM test iterations. Default: state of the data \n"
    "      sinks is unchanged between iterations.\n"
    "\n"
    "  --test-delay\n"
    "      control the delay period among wdm test iterations\n"
    "\n"
    "  --enable-flip <true|false|yes|no|1|0>\n"
    "      Enable/disable flip trait data in HandleDataFlipTimeout\n"
    "\n"
    "  --enable-dictionary-test\n"
    "      Enable/disable dictionary tests\n"
    "\n"
    "  --save-perf\n"
    "      save wdm perf data in files\n"
    "\n"
    "  --event-generator [None | Debug | Livenesss | Security | Telemetry | TestTrait]\n"
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
    "  --enable-retry"
    "       Enable automatic retries by WDM\n"
    "\n"
    "  --enable-mock-event-timestamp-initial-counter"
    "       Enable mock event initial counter using timestamp\n"
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
    "Usage: " TOOL_NAME " [<options>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gToolOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWeaveSecurityMode,
    &gFaultInjectionOptions,
    &gHelpOptions,
    &gCASEOptions,
    &gGroupKeyEncOptions,
    NULL
};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;
    WdmNextPerfUtility &TimeRef = *WdmNextPerfUtility::Instance();
    struct timeval sleepTime;
    time_t begin, end;
    double seconds = 0;
    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 100000;
    nl::Weave::System::Stats::Snapshot before;
    nl::Weave::System::Stats::Snapshot after;
    const bool printStats = true;
    uint32_t KeyId = WeaveKeyId::kNone;

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv);
    SetSignalHandler(DoneOnHandleSIGUSR1);

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    // This test program enables faults and stats prints always (no option taken from the CLI)
    gFaultInjectionOptions.DebugResourceUsage = true;
    gFaultInjectionOptions.PrintFaultCounters = true;

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

    if (EnableMockTimestampInitialCounter)
    {
        EnableMockEventTimestampInitialCounter();
    }

    InitializeEventLogging(&ExchangeMgr);

    KeyId = gGroupKeyEncOptions.GetEncKeyId();

    switch (WdmRoleInTest)
    {
        case 0:
            break;

#if ENABLE_VIEW_TEST

        case kToolOpt_WdmSimpleViewClient:
                if (WdmPublisherNodeId != kAnyNodeId)
                {
                    err = MockWdmViewClient::GetInstance()->Init(&ExchangeMgr, TestCaseId);
                    FAIL_ERROR(err, "MockWdmViewClient.Init failed");
                    MockWdmViewClient::GetInstance()-> onCompleteTest = HandleWdmCompleteTest;
                }
                else
                {
                    err = WEAVE_ERROR_INVALID_ARGUMENT;
                    FAIL_ERROR(err, "Simple View Client requires node ID to some publisher");
                }

                break;
            case kToolOpt_WdmSimpleViewServer:
                err = MockWdmViewServer::GetInstance()->Init(&ExchangeMgr, TestCaseId);
                FAIL_ERROR(err, "MockWdmViewServer.Init failed");
                break;

#endif // ENABLE_VIEW_TEST

        case kToolOpt_WdmInitMutualSubscription:
        case kToolOpt_WdmSubscriptionClient:

            if (WdmPublisherNodeId != kAnyNodeId)
            {
                err = MockWdmSubscriptionInitiator::GetInstance()->Init(&ExchangeMgr,
                                                                        kToolOpt_WdmInitMutualSubscription ==
                                                                        WdmRoleInTest,
                                                                        TestCaseId, NumDataChangeBeforeCancellation,
                                                                        FinalStatus, TimeBetweenDataChangeMsec,
                                                                        EnableDataFlip, TimeBetweenLivenessCheckSec,
                                                                        EnableDictionaryTest, gWeaveSecurityMode.SecurityMode,
                                                                        KeyId, EnableRetry);
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
            if (EnableRetry)
            {
                err = WEAVE_ERROR_INVALID_ARGUMENT;
                FAIL_ERROR(err, "MockWdmSubcriptionResponder is incompatible with --enable-retry");
            }

            err = MockWdmSubscriptionResponder::GetInstance()->Init(&ExchangeMgr,
                                                                    kToolOpt_WdmRespMutualSubscription == WdmRoleInTest,
                                                                    TestCaseId, NumDataChangeBeforeCancellation,
                                                                    FinalStatus, TimeBetweenDataChangeMsec,
                                                                    EnableDataFlip, TimeBetweenLivenessCheckSec);
            FAIL_ERROR(err, "MockWdmSubscriptionResponder.Init failed");
            MockWdmSubscriptionResponder::GetInstance()->onCompleteTest = HandleWdmCompleteTest;
            MockWdmSubscriptionResponder::GetInstance()->onError = HandleError;
            if (gClearDataSinkState)
            {
                MockWdmSubscriptionResponder::GetInstance()->ClearDataSinkState();
            }
            break;
        default:
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            FAIL_ERROR(err, "WdmRoleInTest is invalid");
    };

    nl::Weave::Stats::UpdateSnapshot(before);

    for (uint32_t iteration = 1; iteration <= TestIterations; iteration++)
    {

#ifdef ENABLE_WDMPERFDATA

        TimeRef();

#endif //ENABLE_WDMPERFDATA

        switch (WdmRoleInTest)
        {
        case 0:
            break;

#if ENABLE_VIEW_TEST

        case kToolOpt_WdmSimpleViewClient:
            if (gClearDataSinkState)
            {
                MockWdmViewClient::GetInstance()->ClearDataSinkState();
            }

            err = MockWdmViewClient::GetInstance()->StartTesting(WdmPublisherNodeId, WdmUseSubnetId);
            FAIL_ERROR(err, "MockWdmViewClient.StartTesting failed");
            break;

#endif

        case kToolOpt_WdmInitMutualSubscription:
        case kToolOpt_WdmSubscriptionClient:
            if (gClearDataSinkState)
            {
                MockWdmSubscriptionInitiator::GetInstance()->ClearDataSinkState();
            }
            err = MockWdmSubscriptionInitiator::GetInstance()->StartTesting(WdmPublisherNodeId, WdmUseSubnetId);
            if (err != WEAVE_NO_ERROR)
            {
                printf("\nMockWdmSubscriptionInitiator.StartTesting failed: %s\n", ErrorStr(err));
                Done = true;
            }
            //FAIL_ERROR(err, "MockWdmSubscriptionInitiator.StartTesting failed");
            break;
        default:
            printf("TestWdmNext server is ready\n");
        };

        PrintNodeConfig();

        if (gEventGenerator != NULL)
        {
            printf("Starting Event Generator\n");
            MockEventGenerator::GetInstance()->Init(&ExchangeMgr, gEventGenerator, TimeBetweenEvents, true);
        }

        while (!Done)
        {
            ServiceNetwork(sleepTime);
        }
        MockEventGenerator::GetInstance()->SetEventGeneratorStop();

        if (gEventGenerator != NULL)
        {
            while (!MockEventGenerator::GetInstance()->IsEventGeneratorStopped())
            {
                ServiceNetwork(sleepTime);
            }
        }


        printf("Current completed test iteration is %d\n", iteration);
        Done = false;

#ifdef ENABLE_WDMPERFDATA

        TimeRef();
        TimeRef.SetPerf();
        TimeRef.ReportPerf();

#endif //ENABLE_WDMPERFDATA

        if (gSigusr1Received)
        {
            printf("gSigusr1Received\n");
            break;
        }

        time(&begin);
        if (TestDelayBetweenIterationMsec != 0)
        {
            while (seconds * 1000 < TestDelayBetweenIterationMsec)
            {
                ServiceNetwork(sleepTime);
                time(&end);
                seconds = difftime(end, begin);
            }
            printf("delay %d milliseconds\n", TestDelayBetweenIterationMsec);
            seconds = 0;
        }
        else
        {
            printf("no delay\n");
        }

    }

    MockWdmSubscriptionInitiator::GetInstance()->PrintVersionsLog();

    MockWdmSubscriptionResponder::GetInstance()->PrintVersionsLog();


    if (SavePerfData)
    {
        TimeRef.SaveToFile();
    }

    TimeRef.Remove();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolOpt_WdmPublisherNodeId:
        if (!ParseNodeId(arg, WdmPublisherNodeId))
        {
            PrintArgError("%s: Invalid value specified for WDM publisher node id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_WdmUseSubnetId:
        if (!ParseSubnetId(arg, WdmUseSubnetId))
        {
            PrintArgError("%s: Invalid value specified for publisher subnet id: %s\n", progName, arg);
            return false;
        }
        break;

#if ENABLE_VIEW_TEST

    case kToolOpt_WdmSimpleViewClient:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmSimpleViewClient;
        break;
    case kToolOpt_WdmSimpleViewServer:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmSimpleViewServer;
        break;

#endif
    case kToolOpt_ClearDataSinkStateBetweenTests:
        gClearDataSinkState = true;
        break;

    case kToolOpt_WdmSubscriptionClient:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmSubscriptionClient;
        break;
    case kToolOpt_WdmSubscriptionPublisher:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmSubscriptionPublisher;
        break;
    case kToolOpt_WdmInitMutualSubscription:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmInitMutualSubscription;
        break;
    case kToolOpt_WdmRespMutualSubscription:
        if (0 != WdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        WdmRoleInTest = kToolOpt_WdmRespMutualSubscription;
        break;

    case kToolOpt_WdmEnableRetry:
        EnableRetry = true;
        break;

    case kToolopt_EnableMockTimestampInitialCounter:
        EnableMockTimestampInitialCounter = true;
        break;

    case kToolOpt_TestCaseId:
        if (NULL != TestCaseId)
        {
            free(TestCaseId);
        }
        TestCaseId = strdup(arg);
        break;
    case kToolOpt_EnableStopTest:
        EnableStopTest = true;
        break;
    case kToolOpt_NumDataChangeBeforeCancellation:
        if (NULL != NumDataChangeBeforeCancellation)
        {
            free(NumDataChangeBeforeCancellation);
        }
        NumDataChangeBeforeCancellation = strdup(arg);
        break;
    case kToolOpt_TimeBetweenLivenessCheckSec:
        if (NULL != TimeBetweenLivenessCheckSec)
        {
            free(TimeBetweenLivenessCheckSec);
        }
        TimeBetweenLivenessCheckSec = strdup(arg);
        break;
    case kToolOpt_FinalStatus:
        if (NULL != FinalStatus)
        {
            free(FinalStatus);
        }
        FinalStatus = strdup(arg);
        break;
    case kToolOpt_TimeBetweenDataChangeMsec:
        if (NULL != TimeBetweenDataChangeMsec)
        {
            free(TimeBetweenDataChangeMsec);
        }
        TimeBetweenDataChangeMsec = strdup(arg);
        break;
    case kToolOpt_TestIterations:
        if (!ParseInt(arg, TestIterations))
        {
            PrintArgError("%s: Invalid value specified for test iterations: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_TestDelayBetweenIterationMsec:
        if (!ParseInt(arg, TestDelayBetweenIterationMsec))
        {
            PrintArgError("%s: Invalid value specified for test delay between iterations: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_EnableDataFlip:
        if (!ParseBoolean(arg, EnableDataFlip))
        {
            PrintArgError("%s: Invalid value specified for enable data flip: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_EnableDictionaryTest:
        EnableDictionaryTest = true;
        break;
    case kToolOpt_SavePerfData:
        SavePerfData = true;
        break;
    case kToolOpt_EventGenerator:
        if (strncmp(arg, "None", strlen("None")) == 0)
            gEventGenerator = NULL;
        else if (strncmp(arg, "Debug", strlen("Debug")) == 0)
            gEventGenerator = GetTestDebugGenerator();
        else if (strncmp(arg, "Liveness", strlen("Liveness")) == 0)
            gEventGenerator = GetTestLivenessGenerator();
        else if (strncmp(arg, "Security", strlen("Security")) == 0)
            gEventGenerator = GetTestSecurityGenerator();
        else if (strncmp(arg, "Telemetry", strlen("Telemetry")) == 0)
            gEventGenerator = GetTestTelemetryGenerator();
        else if (strncmp(arg, "TestTrait", strlen("TestTrait")) == 0)
            gEventGenerator = GetTestTraitGenerator();
        else
        {
            PrintArgError("%s: Unrecognized event generator name\n", progName);
            return false;
        }
        break;
    case kToolOpt_TimeBetweenEvents:
    {
        char *endptr;
        TimeBetweenEvents = strtoul(arg, &endptr, 0);
        if (endptr == arg)
        {
            PrintArgError("%s: Invalid inter-event timeout\n", progName);
            return false;
        }
        break;
    }
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
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

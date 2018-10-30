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
#include "MockWdmViewClient.h"
#include "MockWdmViewServer.h"
#include "WdmNextPerfUtility.h"
#include "TestWdmSubscriptionlessNotification.h"
#include "MockWdmNodeOptions.h"

using nl::Inet::IPAddress;
using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using nl::Weave::WeaveExchangeManager;

#define TOOL_NAME "TestWdmNext"

static void HandleWdmCompleteTest();
static void HandleError();


// events
EventGenerator * gEventGenerator = NULL;
uint32_t TestWdmSublessNotifyDelayMsec = 6000;

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gTestWdmNextOptions,
    &gMockWdmNodeOptions,
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gWeaveSecurityMode,
    &gFaultInjectionOptions,
    &gHelpOptions,
    &gCASEOptions,
    &gGroupKeyEncOptions,
    NULL
};

static int32_t GetNumFaultInjectionEventsAvailable(void)
{
    int32_t retval = 0;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    MockWdmSubscriptionInitiator *initiator = MockWdmSubscriptionInitiator::GetInstance();

    retval = initiator->GetNumFaultInjectionEventsAvailable();
#endif

    return retval;

}
static void ExpireTimer(int32_t argument)
{
    ExchangeMgr.ExpireExchangeTimers();
}


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

    gMockWdmNodeOptions.mWdmUpdateMaxNumberOfTraits = MockWdmSubscriptionInitiator::GetNumUpdatableTraits();

    InitToolCommon();

    SetupFaultInjectionContext(argc, argv,
            GetNumFaultInjectionEventsAvailable, ExpireTimer);

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

    if (gTestWdmNextOptions.mEnableMockTimestampInitialCounter)
    {
        EnableMockEventTimestampInitialCounter();
    }

    InitializeEventLogging(&ExchangeMgr);

    switch (gMockWdmNodeOptions.mWdmRoleInTest)
    {
        case 0:
            break;

#if ENABLE_VIEW_TEST

        case kToolOpt_WdmSimpleViewClient:
                if (gMockWdmNodeOptions.mWdmPublisherNodeId != kAnyNodeId)
                {
                    err = MockWdmViewClient::GetInstance()->Init(&ExchangeMgr, gMockWdmNodeOptions.mTestCaseId);
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
                err = MockWdmViewServer::GetInstance()->Init(&ExchangeMgr, gMockWdmNodeOptions.mTestCaseId);
                FAIL_ERROR(err, "MockWdmViewServer.Init failed");
                break;

#endif // ENABLE_VIEW_TEST

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        case kToolOpt_WdmSimpleSublessNotifyClient:
                err = TestWdmSubscriptionlessNotificationReceiver::GetInstance()->Init(&ExchangeMgr);
                FAIL_ERROR(err, "TestWdmSubscriptionlessNotificationReceiver.Init failed");

                TestWdmSubscriptionlessNotificationReceiver::GetInstance()->OnTestComplete = HandleWdmCompleteTest;

                TestWdmSubscriptionlessNotificationReceiver::GetInstance()->OnError = HandleError;
            break;
        case kToolOpt_WdmSimpleSublessNotifyServer:
                time(&begin);

                while (seconds * 1000 < TestWdmSublessNotifyDelayMsec)
                {
                    ServiceNetwork(sleepTime);
                    time(&end);
                    seconds = difftime(end, begin);
                }
                printf("delay %d milliseconds\n", TestWdmSublessNotifyDelayMsec);
                seconds = 0;

                if (gMockWdmNodeOptions.mWdmSublessNotifyDestNodeId != kAnyNodeId)
                {
                    err = TestWdmSubscriptionlessNotificationSender::GetInstance()->Init(&ExchangeMgr,
                                                                                         gMockWdmNodeOptions.mWdmUseSubnetId,
                                                                                         gMockWdmNodeOptions.mWdmSublessNotifyDestNodeId);
                    FAIL_ERROR(err, "TestWdmSubscriptionlessNotificationSender.Init failed");
                }
            break;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
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
                                                                    gMockWdmNodeOptions);
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

    nl::Weave::Stats::UpdateSnapshot(before);

    for (uint32_t iteration = 1; iteration <= gTestWdmNextOptions.mTestIterations; iteration++)
    {

#ifdef ENABLE_WDMPERFDATA

        TimeRef();

#endif //ENABLE_WDMPERFDATA

        switch (gMockWdmNodeOptions.mWdmRoleInTest)
        {
        case 0:
            break;

#if ENABLE_VIEW_TEST

        case kToolOpt_WdmSimpleViewClient:
            if (gTestWdmNextOptions.mClearDataSinkState)
            {
                MockWdmViewClient::GetInstance()->ClearDataSinkState();
            }

            err = MockWdmViewClient::GetInstance()->StartTesting(WdmPublisherNodeId, WdmUseSubnetId);
            FAIL_ERROR(err, "MockWdmViewClient.StartTesting failed");
            break;

#endif

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        case kToolOpt_WdmSimpleSublessNotifyClient:
            break;
        case kToolOpt_WdmSimpleSublessNotifyServer:
                if (gMockWdmNodeOptions.mWdmSublessNotifyDestNodeId != kAnyNodeId)
                {
                    err = TestWdmSubscriptionlessNotificationSender::GetInstance()->SendSubscriptionlessNotify();
                    Done = true;
                    FAIL_ERROR(err, "TestWdmSubscriptionlessNotificationSender.SendSubscriptionlessNotify failed");
                }
            break;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        case kToolOpt_WdmInitMutualSubscription:
        case kToolOpt_WdmSubscriptionClient:
            if (gTestWdmNextOptions.mClearDataSinkState)
            {
                MockWdmSubscriptionInitiator::GetInstance()->ClearDataSinkState();
            }
            err = MockWdmSubscriptionInitiator::GetInstance()->StartTesting(gMockWdmNodeOptions.mWdmPublisherNodeId, gMockWdmNodeOptions.mWdmUseSubnetId);
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

        switch (gMockWdmNodeOptions.mEventGeneratorType)
        {
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

        if (gEventGenerator != NULL)
        {
            printf("Starting Event Generator\n");
            MockEventGenerator::GetInstance()->Init(&ExchangeMgr, gEventGenerator, gMockWdmNodeOptions.mTimeBetweenEvents, true);
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

        switch (gMockWdmNodeOptions.mWdmRoleInTest)
        {
            case kToolOpt_WdmInitMutualSubscription:
            case kToolOpt_WdmSubscriptionClient:
                if (gTestWdmNextOptions.mClearDataSinkState)
                {
                    MockWdmSubscriptionInitiator::GetInstance()->Cleanup();
                }
                break;
            default:
                break;
        }


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
        if (gTestWdmNextOptions.mTestDelayBetweenIterationMsec != 0)
        {
            while (seconds * 1000 < gTestWdmNextOptions.mTestDelayBetweenIterationMsec)
            {
                ServiceNetwork(sleepTime);
                time(&end);
                seconds = difftime(end, begin);
            }
            printf("delay %d milliseconds\n", gTestWdmNextOptions.mTestDelayBetweenIterationMsec);
            seconds = 0;
        }
        else
        {
            printf("no delay\n");
        }

        printf("Current completed test iteration is %d\n", iteration);

    }

    MockWdmSubscriptionInitiator::GetInstance()->PrintVersionsLog();
    MockWdmSubscriptionInitiator::GetInstance()->Cleanup();

    MockWdmSubscriptionResponder::GetInstance()->PrintVersionsLog();


    if (gTestWdmNextOptions.mSavePerfData)
    {
        TimeRef.SaveToFile();
    }

    TimeRef.Remove();

    ProcessStats(before, after, printStats, NULL);
    PrintFaultInjectionCounters();

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    if (gMockWdmNodeOptions.mWdmRoleInTest == kToolOpt_WdmSimpleSublessNotifyServer)
    {
        err = TestWdmSubscriptionlessNotificationSender::GetInstance()->Shutdown();
        FAIL_ERROR(err, "TestWdmSubscriptionlessNotificationSender.Shutdown failed");
    }
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    ShutdownWeaveStack();
    ShutdownNetwork();
    ShutdownSystemLayer();

    return 0;
}

static void HandleWdmCompleteTest()
{
    if (gMockWdmNodeOptions.mEnableStopTest)
    {
        Done = true;
    }
}

static void HandleError()
{
    Done = true;
}

/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      This file implements the CLI option handler for Mock WDM client
 *      and server objects.
 *
 */
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveFabricState.h>

#include "MockSinkTraits.h"

#include "MockWdmNodeOptions.h"

MockWdmNodeOptions::MockWdmNodeOptions() :
    mWdmPublisherNodeId(nl::Weave::kAnyNodeId),
    mWdmUseSubnetId(nl::Weave::kWeaveSubnetId_NotSpecified),
    mWdmRoleInTest(0),
    mEnableMutualSubscription(false),
    mTestCaseId(NULL),
    mEnableStopTest(false),
    mNumDataChangeBeforeCancellation(NULL),
    mFinalStatus(NULL),
    mTimeBetweenDataChangeMsec(NULL),
    mEnableDataFlip(true),
    mEventGeneratorType(kGenerator_None),
    mTimeBetweenEvents(1000),
    mTimeBetweenLivenessCheckSec(NULL),
    mEnableDictionaryTest(false),
    mEnableRetry(false),
#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    mWdmSublessNotifyDestNodeId(nl::Weave::kAnyNodeId),
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    mWdmUpdateConditionality(kConditionality_Conditional),
    mWdmUpdateMutation(kMutation_OneLeaf),
    mWdmUpdateNumberOfTraits(1),
    mWdmUpdateNumberOfMutations(1),
    mWdmUpdateNumberOfRepeatedMutations(1),
    mWdmUpdateTiming(kTiming_AfterSub),
    mWdmUpdateMaxNumberOfTraits(1)
{
    static OptionDef optionDefs[] =
    {
        { "test-case",                                      kArgumentRequired,  kToolOpt_TestCaseId },
        { "enable-stop",                                    kNoArgument,        kToolOpt_EnableStopTest },
        { "total-count",                                    kArgumentRequired,  kToolOpt_NumDataChangeBeforeCancellation },
        { "final-status",                                   kArgumentRequired,  kToolOpt_FinalStatus },
        { "timer-period",                                   kArgumentRequired,  kToolOpt_TimeBetweenDataChangeMsec },
        { "enable-flip",                                    kArgumentRequired,  kToolOpt_EnableDataFlip },
        { "enable-dictionary-test",                         kNoArgument,        kToolOpt_EnableDictionaryTest },
        { "event-generator",                                kArgumentRequired,  kToolOpt_EventGenerator },
        { "inter-event-period",                             kArgumentRequired,  kToolOpt_TimeBetweenEvents },
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        { "wdm-publisher",                                  kArgumentRequired,  kToolOpt_WdmPublisherNodeId },
        { "wdm-subless-notify-dest-node",                   kArgumentRequired,  kToolOpt_WdmSublessNotifyDestNodeId },
        { "wdm-subnet",                                     kArgumentRequired,  kToolOpt_WdmUseSubnetId },
        //{ "wdm-simple-view-client",                       kNoArgument,        kToolOpt_WdmSimpleViewClient },
        //{ "wdm-simple-view-server",                       kNoArgument,        kToolOpt_WdmSimpleViewServer },
        { "wdm-simple-subless-notify-client",               kNoArgument,        kToolOpt_WdmSimpleSublessNotifyClient },
        { "wdm-simple-subless-notify-server",               kNoArgument,        kToolOpt_WdmSimpleSublessNotifyServer },
        { "wdm-one-way-sub-client",                         kNoArgument,        kToolOpt_WdmSubscriptionClient },
        { "wdm-one-way-sub-publisher",                      kNoArgument,        kToolOpt_WdmSubscriptionPublisher },
        { "wdm-init-mutual-sub",                            kNoArgument,        kToolOpt_WdmInitMutualSubscription },
        { "wdm-resp-mutual-sub",                            kNoArgument,        kToolOpt_WdmRespMutualSubscription },
        { "wdm-liveness-check-period",                      kArgumentRequired,  kToolOpt_TimeBetweenLivenessCheckSec },
        { "enable-retry",                                   kNoArgument,        kToolOpt_WdmEnableRetry },
        { "wdm-update-mutation",                            kArgumentRequired,  kToolOpt_WdmUpdateMutation },
        { "wdm-update-number-of-mutations",                 kArgumentRequired,  kToolOpt_WdmUpdateNumberOfMutations },
        { "wdm-update-number-of-repeated-mutations",        kArgumentRequired,  kToolOpt_WdmUpdateNumberOfRepeatedMutations },
        { "wdm-update-number-of-traits",                    kArgumentRequired,  kToolOpt_WdmUpdateNumberOfTraits },
        { "wdm-update-conditionality",                      kArgumentRequired,  kToolOpt_WdmUpdateConditionality },
        { "wdm-update-timing",                              kArgumentRequired,  kToolOpt_WdmUpdateTiming },
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        { NULL }
    };

    OptionDefs = optionDefs;

    HelpGroupName = "MockWdmNode OPTIONS";

    OptionHelp =
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
        "  --wdm-simple-subless-notify-client\n"
        "       Initiate a simple WDM Next Subscriptionless Notify Client\n"
        "\n"
        "  --wdm-simple-subless-notify-server\n"
        "       Initiate a simple WDM Next Subscriptionless Notify Server\n"
        "\n"
        "  --wdm-subless-notify-dest-node <dest-node-id>\n"
        "       The node id of the destination node\n"
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
        "  --enable-flip <true|false|yes|no|1|0>\n"
        "      Enable/disable flip trait data in HandleDataFlipTimeout\n"
        "\n"
        "  --enable-dictionary-test\n"
        "      Enable/disable dictionary tests\n"
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
        "  --inter-event-period <ms>\n"
        "       Delay between emitting consecutive events (default 1s)\n"
        "\n"
        "  --enable-retry\n"
        "       Enable automatic subscription retries by WDM\n"
        "\n"
        "  --wdm-update-mutation <mutation>\n"
        "       The first mutation to apply to each trait instance.\n"
        "       For every cycle up to total-count, the mutations are applied in order.\n"
        "       Only TestATrait supports all mutations. The other trait handlers revert to\n"
        "       default one (OneLeaf) in case of a mutation they don't support.\n"
        "\n"
        "  --wdm-update-number-of-mutations <int>\n"
        "       Number of mutations (and therefore calls to FlushUpdate) performed in the same context\n"
        "       The first mutation is decided by --wdm-update-mutation. The following ones increment from there\n"
        "       but the same mutation is used as many times as specified with --wdm-update-number-of-repeated-mutations.\n"
        "       Default: 1\n"
        "\n"
        "  --wdm-update-number-of-repeated-mutations <int>\n"
        "       How many times the same mutation should be applied before moving to the next one\n"
        "       Default: 1\n"
        "\n"
        "  --wdm-update-number-of-traits <int>\n"
        "       Number of traits to mutate. Default is 1, max is 4.\n"
        "         1: TestATraitUpdatableDataSink (default resource id)\n"
        "         2: All of the above, plus LocaleSettingsTrait\n"
        "         3: All of the above, plus TestBTrait\n"
        "         4: All of the above, plus TestATraitUpdatableDataSink (resource id 1)\n"
        "\n"
        "  --wdm-update-conditionality <conditional, unconditional, mixed, alternate>\n"
        "       The conditionality of the update:\n"
        "         conditional: all trait updates are conditional\n"
        "         unconditional: all trait updates are unconditional\n"
        "         mixed: TestATraitUpdatableDataSink is updated conditionally; the others unconditionally\n"
        "         alternate: like mixed, but inverting the conditionality at every mutation\n"
        "       Default is conditional\n"
        "\n"
        "  TODO: --wdm-update-timing <before-sub, during-sub, after-sub>\n"
        "       Controls when the first mutation is applied and flushed:\n"
        "         before-sub: before the subscription is started\n"
        "         during-sub: right after the subscription has been started, but without waiting for the\n"
        "                     subscription to be established\n"
        "         after-sub:  after the subscription has been established\n"
        "       Default is after-sub\n"
        "\n"
        "  TODO: --wdm-update-trigger <timer, notification, update-response>\n"
        "       Controls what triggers mutations after the first one:\n"
        "         before-sub: before the subscription is started\n"
        "         during-sub: right after the subscription has been started, but without waiting for the\n"
        "                     subscription to be established\n"
        "         after-sub:  after the subscription has been established\n"
        "       Default is after-sub\n"
        "\n";

}

const char **MockWdmNodeOptions::GetMutationStrings(void)
{
    static const char *mutationStrings[kMutation_NumItems] =
    {
        "OneLeaf",
        "SameLevelLeaves",
        "DiffLevelLeaves",
        "WholeDictionary",
        "WholeLargeDictionary",
        "FewDictionaryItems",
        "ManyDictionaryItems",
        "WholeDictionaryAndLeaf",
        "OneStructure",
        "OneLeafOneStructure",
        "Root",
        "RootWithLargeDictionary",
    };

    return mutationStrings;
}

const char **MockWdmNodeOptions::GetGeneratorStrings(void)
{
    static const char *generatorStrings[kGenerator_NumItems] = {
        "None",
        "Debug",
        "Liveness",
        "Security",
        "Telemetry",
        "TestTrait"
    };

    return generatorStrings;
}

const char **MockWdmNodeOptions::GetConditionalityStrings(void)
{
    static const char *conditionalityStrings[kConditionality_NumItems] = {
        "Conditional",
        "Unconditional",
        "Mixed",
        "Alternate"
    };

    return conditionalityStrings;
}

const char **MockWdmNodeOptions::GetUpdateTimingStrings(void)
{
    static const char *updateTimingStrings[kTiming_NumItems] = {
        "BeforeSub",
        "DuringSub",
        "AfterSub"
    };

    return updateTimingStrings;
}

static bool FindStringInArray(const char *aTarget, const char **aArray, size_t aArrayLength, size_t &aIndex)
{
    for (aIndex = 0; aIndex < aArrayLength && (strcmp(aTarget, aArray[aIndex]) != 0); aIndex++)
    {
        continue;
    }

    if (aIndex == aArrayLength)
    {
        return false;
    }

    return true;
}

bool MockWdmNodeOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolOpt_WdmPublisherNodeId:
        if (!ParseNodeId(arg, mWdmPublisherNodeId))
        {
            PrintArgError("%s: Invalid value specified for WDM publisher node id: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_WdmUseSubnetId:
        if (!ParseSubnetId(arg, mWdmUseSubnetId))
        {
            PrintArgError("%s: Invalid value specified for publisher subnet id: %s\n", progName, arg);
            return false;
        }
        break;

#if ENABLE_VIEW_TEST

    case kToolOpt_WdmSimpleViewClient:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSimpleViewClient;
        break;
    case kToolOpt_WdmSimpleViewServer:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSimpleViewServer;
        break;

#endif

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    case kToolOpt_WdmSimpleSublessNotifyClient:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSimpleSublessNotifyClient;
        break;
    case kToolOpt_WdmSimpleSublessNotifyServer:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSimpleSublessNotifyServer;
        break;
    case kToolOpt_WdmSublessNotifyDestNodeId:
        if (!ParseNodeId(arg, mWdmSublessNotifyDestNodeId))
        {
            PrintArgError("%s: Invalid value specified for WDM publisher node id: %s\n", progName, arg);
            return false;
        }
        break;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    case kToolOpt_WdmSubscriptionClient:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSubscriptionClient;
        break;
    case kToolOpt_WdmSubscriptionPublisher:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmSubscriptionPublisher;
        break;
    case kToolOpt_WdmInitMutualSubscription:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmInitMutualSubscription;
        mEnableMutualSubscription = true;
        break;
    case kToolOpt_WdmRespMutualSubscription:
        if (0 != mWdmRoleInTest)
        {
            PrintArgError("%s: Mock WDM device can only play one role in WDM tests (%s)\n", progName, arg);
            return false;
        }
        mWdmRoleInTest = kToolOpt_WdmRespMutualSubscription;
        mEnableMutualSubscription = true;
        break;

    case kToolOpt_WdmEnableRetry:
        mEnableRetry = true;
        break;

    case kToolOpt_TestCaseId:
        if (NULL != mTestCaseId)
        {
            free(const_cast<char *>(mTestCaseId));
        }
        mTestCaseId = strdup(arg);
        break;
    case kToolOpt_EnableStopTest:
        mEnableStopTest = true;
        break;
    case kToolOpt_NumDataChangeBeforeCancellation:
        if (NULL != mNumDataChangeBeforeCancellation)
        {
            free(const_cast<char *>(mNumDataChangeBeforeCancellation));
        }
        mNumDataChangeBeforeCancellation = strdup(arg);
        break;
    case kToolOpt_TimeBetweenLivenessCheckSec:
        if (NULL != mTimeBetweenLivenessCheckSec)
        {
            free(const_cast<char *>(mTimeBetweenLivenessCheckSec));
        }
        mTimeBetweenLivenessCheckSec = strdup(arg);
        break;
    case kToolOpt_FinalStatus:
        if (NULL != mFinalStatus)
        {
            free(const_cast<char *>(mFinalStatus));
        }
        mFinalStatus = strdup(arg);
        break;
    case kToolOpt_TimeBetweenDataChangeMsec:
        if (NULL != mTimeBetweenDataChangeMsec)
        {
            free(const_cast<char *>(mTimeBetweenDataChangeMsec));
        }
        mTimeBetweenDataChangeMsec = strdup(arg);
        break;
    case kToolOpt_EnableDataFlip:
        if (!ParseBoolean(arg, mEnableDataFlip))
        {
            PrintArgError("%s: Invalid value specified for enable data flip: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_EnableDictionaryTest:
        mEnableDictionaryTest = true;
        break;

    case kToolOpt_EventGenerator:
    {
        size_t i;

        if (!FindStringInArray(arg, GetGeneratorStrings(), kGenerator_NumItems, i))
        {
            PrintArgError("%s: Unrecognized event generator name\n", progName);
            return false;
        }
        mEventGeneratorType = static_cast<EventGeneratorType>(i);
        break;
    }
    case kToolOpt_TimeBetweenEvents:
    {
        char *endptr;
        mTimeBetweenEvents = strtoul(arg, &endptr, 0);
        if (endptr == arg)
        {
            PrintArgError("%s: Invalid inter-event timeout\n", progName);
            return false;
        }
        break;
    }
    case kToolOpt_WdmUpdateMutation:
    {
        size_t i;

        if (!FindStringInArray(arg, GetMutationStrings(), kMutation_NumItems, i))
        {
            printf("%s: Invalid value specified for --wdm-update-mutation: %s\n", progName, arg);
            PrintArgError("%s: Invalid value specified for --wdm-update-mutation: %s\n", progName, arg);
            return false;
        }
        mWdmUpdateMutation = static_cast<WdmUpdateMutation>(i);
        break;
    }
    case kToolOpt_WdmUpdateNumberOfMutations:
    {
        int tmp;

        if ((!ParseInt(arg, tmp)) || (tmp < 1))
        {
            PrintArgError("%s: Invalid value specified for --wdm-update-number-of-mutations: %s; min 1\n", progName, arg);
            return false;
        }
        mWdmUpdateNumberOfMutations = static_cast<uint32_t>(tmp);
        break;
    }
    case kToolOpt_WdmUpdateNumberOfRepeatedMutations:
    {
        int tmp;

        if ((!ParseInt(arg, tmp)) || (tmp < 1))
        {
            PrintArgError("%s: Invalid value specified for --wdm-update-number-of-repeated-mutations: %s; min 1\n", progName, arg);
            return false;
        }
        mWdmUpdateNumberOfRepeatedMutations = static_cast<uint32_t>(tmp);
        break;
    }
    case kToolOpt_WdmUpdateNumberOfTraits:
    {
        uint32_t tmp;

        if ((!ParseInt(arg, tmp)) || (tmp < 1) || (tmp > mWdmUpdateMaxNumberOfTraits))
        {
            PrintArgError("%s: Invalid value specified for --wdm-update-number-of-traits: %s; min 1, max %u\n", progName, arg, mWdmUpdateMaxNumberOfTraits);
            return false;
        }
        mWdmUpdateNumberOfTraits = static_cast<uint32_t>(tmp);
        break;
    }
    case kToolOpt_WdmUpdateConditionality:
    {
        size_t i;

        if (!FindStringInArray(arg, GetConditionalityStrings(), kConditionality_NumItems, i))
        {
            PrintArgError("%s: Invalid value specified for --wdm-update-conditionality: %s\n", progName, arg);
            return false;
        }
        mWdmUpdateConditionality = static_cast<WdmUpdateConditionality>(i);
        break;
    }
    case kToolOpt_WdmUpdateTiming:
    {
        size_t i;

        if (!FindStringInArray(arg, GetUpdateTimingStrings(), kTiming_NumItems, i))
        {
            PrintArgError("%s: Invalid value specified for --wdm-update-timing: %s\n", progName, arg);
            return false;
        }
        mWdmUpdateTiming = static_cast<WdmUpdateTiming>(i);
        break;
    }
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

MockWdmNodeOptions gMockWdmNodeOptions;

TestWdmNextOptions::TestWdmNextOptions(void) :
    mEnableMockTimestampInitialCounter(false),
    mTestIterations(1),
    mTestDelayBetweenIterationMsec(0),
    mSavePerfData(false),
    mClearDataSinkState(false)
{
    static OptionDef optionDefs[] =
    {
        { "enable-mock-event-timestamp-initial-counter",    kNoArgument,        kToolOpt_EnableMockTimestampInitialCounter },
        { "test-iterations",                                kArgumentRequired,  kToolOpt_TestIterations },
        { "test-delay",                                     kArgumentRequired,  kToolOpt_TestDelayBetweenIterationMsec },
        { "save-perf",                                      kNoArgument,        kToolOpt_SavePerfData },
        { "clear-state-between-iterations",                 kNoArgument,        kToolOpt_ClearDataSinkStateBetweenTests },
        { NULL }
    };

    OptionDefs = optionDefs;

    HelpGroupName = "TestWdmNext OPTIONS";

    OptionHelp =
        "  --enable-mock-event-timestamp-initial-counter\n"
        "       Enable mock event initial counter using timestamp\n"
        "\n"
        "  --test-iterations\n"
        "      control the number of wdm test iterations\n"
        "\n"
        "  --test-delay\n"
        "      control the delay period among wdm test iterations\n"
        "\n"
        "  --save-perf\n"
        "      save wdm perf data in files\n"
        "\n"
        "  --clear-state-between-iterations\n"
        "      Clear data sink state between WDM test iterations. Default: state of the data \n"
        "      sinks is unchanged between iterations.\n"
        "\n";
}

bool TestWdmNextOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolOpt_EnableMockTimestampInitialCounter:
        mEnableMockTimestampInitialCounter = true;
        break;
    case kToolOpt_TestIterations:
        if (!ParseInt(arg, mTestIterations))
        {
            PrintArgError("%s: Invalid value specified for test iterations: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_TestDelayBetweenIterationMsec:
        if (!ParseInt(arg, mTestDelayBetweenIterationMsec))
        {
            PrintArgError("%s: Invalid value specified for test delay between iterations: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolOpt_SavePerfData:
        mSavePerfData = true;
        break;
    case kToolOpt_ClearDataSinkStateBetweenTests:
        mClearDataSinkState = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

TestWdmNextOptions gTestWdmNextOptions;

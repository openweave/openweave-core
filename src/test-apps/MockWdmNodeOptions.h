/*
 *
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
 *      This file declares the CLI option handler for Mock WDM client
 *      and server objects.
 *
 */
#ifndef MOCKWDMNODEOPTIONS_H_
#define MOCKWDMNODEOPTIONS_H_

#include <inttypes.h>

#include <Weave/Support/nlargparser.hpp>

using namespace nl::ArgParser;

enum
{
    kToolOpt_WdmPublisherNodeId         = 1000,  // Specify the node ID of the WDM Publisher we should connect to
    kToolOpt_WdmUseSubnetId,                     // True if the publisher is within the specified subnet
    //kToolOpt_WdmSimpleViewClient,
    //kToolOpt_WdmSimpleViewServer,
    kToolOpt_WdmSubscriptionClient,
    kToolOpt_WdmSubscriptionPublisher,
    kToolOpt_WdmInitMutualSubscription,
    kToolOpt_WdmRespMutualSubscription,
    kToolOpt_TestCaseId,
    kToolOpt_UseTCP,
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
    kToolOpt_EnableMockTimestampInitialCounter,
    kToolOpt_WdmSimpleSublessNotifyClient,
    kToolOpt_WdmSimpleSublessNotifyServer,
    kToolOpt_WdmSublessNotifyDestNodeId,
    kToolOpt_WdmUpdateMutation,
    kToolOpt_WdmUpdateNumberOfMutations,
    kToolOpt_WdmUpdateNumberOfRepeatedMutations,
    kToolOpt_WdmUpdateNumberOfTraits,
    kToolOpt_WdmUpdateConditionality,
    kToolOpt_WdmUpdateTiming,
    kToolOpt_WdmUpdateDiscardOnError,
};

class MockWdmNodeOptions : public OptionSetBase
{
public:
    enum WdmUpdateConditionality {
        kConditionality_Conditional = 0,
        kConditionality_Unconditional,
        kConditionality_Mixed,
        kConditionality_Alternate,

        kConditionality_NumItems
    };

    static const char **GetConditionalityStrings(void);

    enum WdmUpdateTiming {
        kTiming_BeforeSub = 0,
        kTiming_DuringSub,
        kTiming_AfterSub,
        kTiming_NoSub,
        kTiming_NumItems
    };

    static const char **GetUpdateTimingStrings(void);

    enum WdmUpdateMutation {
        kMutation_OneLeaf = 0,
        kMutation_SameLevelLeaves,
        kMutation_DiffLevelLeaves,
        kMutation_WholeDictionary,
        kMutation_WholeLargeDictionary,
        kMutation_FewDictionaryItems,
        kMutation_ManyDictionaryItems,
        kMutation_WholeDictionaryAndLeaf,
        kMutation_OneStructure,
        kMutation_OneLeafOneStructure,
        kMutation_Root,
        kMutation_RootWithLargeDictionary,

        kMutation_NumItems
    };

    static const char **GetMutationStrings(void);

    enum EventGeneratorType {
        kGenerator_None = 0,
        kGenerator_TestDebug,
        kGenerator_TestLiveness,
        kGenerator_TestSecurity,
        kGenerator_TestTelemetry,
        kGenerator_TestTrait,

        kGenerator_NumItems
    };

    static const char **GetGeneratorStrings(void);

    MockWdmNodeOptions();

    uint64_t mWdmPublisherNodeId;
    uint16_t mWdmUseSubnetId;
    int mWdmRoleInTest;
    bool mEnableMutualSubscription;
    const char * mTestCaseId;
    bool mUseTCP;
    bool mEnableStopTest;
    const char * mNumDataChangeBeforeCancellation;
    const char * mFinalStatus;
    const char * mTimeBetweenDataChangeMsec;
    bool mEnableDataFlip;
    EventGeneratorType mEventGeneratorType;
    int mTimeBetweenEvents;
    const char * mTimeBetweenLivenessCheckSec;
    bool mEnableDictionaryTest;
    bool mEnableRetry;
#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    uint64_t mWdmSublessNotifyDestNodeId;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    WdmUpdateConditionality mWdmUpdateConditionality;
    WdmUpdateMutation mWdmUpdateMutation;
    uint32_t mWdmUpdateNumberOfTraits;
    uint32_t mWdmUpdateNumberOfMutations;
    uint32_t mWdmUpdateNumberOfRepeatedMutations;
    WdmUpdateTiming mWdmUpdateTiming;
    bool mWdmUpdateDiscardOnError;

    uint32_t mWdmUpdateMaxNumberOfTraits;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern MockWdmNodeOptions gMockWdmNodeOptions;

class TestWdmNextOptions : public OptionSetBase
{
public:

    TestWdmNextOptions();

    bool mEnableMockTimestampInitialCounter;
    uint32_t mTestIterations;
    uint32_t mTestDelayBetweenIterationMsec;
    bool mSavePerfData;
    bool mClearDataSinkState;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern TestWdmNextOptions gTestWdmNextOptions;

#endif // MOCKWDMNODEOPTIONS_H_

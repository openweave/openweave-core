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
    kToolOpt_WdmUpdateNumberOfTraits,
    kToolOpt_WdmUpdateConditionality,
    kToolOpt_WdmUpdateTiming,
};

class MockWdmNodeOptions : public OptionSetBase
{
public:
    typedef enum {
        kConditional = 0,
        kUnconditional,
        kMixed,
        kAlternate,
    } WdmUpdateConditionality;

    typedef enum {
        kBeforeSub = 0,
        kDuringSub,
        kAfterSub,
    } WdmUpdateTiming;

    typedef enum {
        kGenerator_None,
        kGenerator_TestDebug,
        kGenerator_TestLiveness,
        kGenerator_TestSecurity,
        kGenerator_TestTelemetry,
        kGenerator_TestTrait,
    } EventGeneratorType;

    MockWdmNodeOptions();

    uint64_t mWdmPublisherNodeId;
    uint16_t mWdmUseSubnetId;
    int mWdmRoleInTest;
    bool mEnableMutualSubscription;
    const char * mTestCaseId;
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
    WdmUpdateConditionality mWdmUpdateConditionality;
    uint32_t mWdmUpdateMutation;
    uint32_t mWdmUpdateNumberOfTraits;
    WdmUpdateTiming mWdmUpdateTiming;
#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    uint64_t mWdmSublessNotifyDestNodeId;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

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

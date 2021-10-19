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
 *      This is a unit test suite for
 *      <tt>nl::Weave::System::Timer</tt>, * the part of the Weave
 *      System Layer that implements timers.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <SystemLayer/SystemConfig.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/tcpip.h>
#include <lwip/sys.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <poll.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#include <SystemLayer/SystemError.h>
#include <SystemLayer/SystemLayer.h>
#include <SystemLayer/SystemTimer.h>

#include <Weave/Support/ErrorStr.h>

#include <nlunit-test.h>

using nl::ErrorStr;
using namespace nl::Weave::System;

static void ServiceEvents(Layer& aLayer, ::timeval& aSleepTime)
{
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    int sleepTime = aSleepTime.tv_usec / 1000 + aSleepTime.tv_usec * 1000;
    struct pollfd pollFDs[WEAVE_CONFIG_MAX_POLL_FDS];
    int numPollFDs = 0;

    if (aLayer.State() == kLayerState_Initialized)
        aLayer.PrepareSelect(pollFDs, numPollFDs, sleepTime);

    int pollRes = poll(pollFDs, numPollFDs, sleepTime);
    if (pollRes < 0)
    {
        printf("poll failed: %s\n", ErrorStr(MapErrorPOSIX(errno)));
        return;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    if (aLayer.State() == kLayerState_Initialized)
    {
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
        aLayer.HandleSelectResult(pollFDs, numPollFDs);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
        if (aLayer.State() == kLayerState_Initialized)
        {
            // TODO: Currently timers are delayed by aSleepTime above. A improved solution would have a mechanism to reduce
            // aSleepTime according to the next timer.
            aLayer.HandlePlatformTimer();
        }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
    }
}

// Test input vector format.


struct TestContext {
    Layer* mLayer;
    nlTestSuite* mTestSuite;
};

// Test input data.


static struct TestContext sContext;

static volatile bool sOverflowTestDone;

void HandleTimer0Failed(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, false);
    sOverflowTestDone = true;
}

void HandleTimer1Failed(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, false);
    sOverflowTestDone = true;
}

void HandleTimer10Success(Layer* inetLayer, void* aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, true);
    sOverflowTestDone = true;
}

static void CheckOverflow(nlTestSuite* inSuite, void* aContext)
{
    uint32_t timeout_overflow_0ms = 652835029;
    uint32_t timeout_overflow_1ms = 1958505088;
    uint32_t timeout_10ms = 10;

    TestContext& lContext = *static_cast<TestContext*>(aContext);
    Layer& lSys = *lContext.mLayer;

    sOverflowTestDone = false;

    lSys.StartTimer(timeout_overflow_0ms, HandleTimer0Failed, aContext);
    lSys.StartTimer(timeout_overflow_1ms, HandleTimer1Failed, aContext);
    lSys.StartTimer(timeout_10ms, HandleTimer10Success, aContext);

    while (!sOverflowTestDone)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 1000; // 1 ms tick
        ServiceEvents(lSys, sleepTime);
    }

    lSys.CancelTimer(HandleTimer0Failed, aContext);
    lSys.CancelTimer(HandleTimer1Failed, aContext);
    lSys.CancelTimer(HandleTimer10Success, aContext);
}

static uint32_t sNumTimersHandled = 0;
static const uint32_t MAX_NUM_TIMERS = 1000;


void HandleGreedyTimer(Layer *aLayer, void * aState, Error aError)
{
    TestContext& lContext = *static_cast<TestContext*>(aState);
    NL_TEST_ASSERT(lContext.mTestSuite, sNumTimersHandled < MAX_NUM_TIMERS);

    if (sNumTimersHandled >= MAX_NUM_TIMERS)
    {
        return;
    }

    aLayer->StartTimer(0, HandleGreedyTimer, aState);
    sNumTimersHandled ++;

}

static void CheckStarvation(nlTestSuite* inSuite, void* aContext)
{
    TestContext& lContext = *static_cast<TestContext*>(aContext);
    Layer& lSys = *lContext.mLayer;
    struct timeval sleepTime;

    lSys.StartTimer(0, HandleGreedyTimer, aContext);

    sleepTime.tv_sec = 0;
    sleepTime.tv_usec = 1000; // 1 ms tick
    ServiceEvents(lSys, sleepTime);
}


// Test Suite


/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Timer::TestOverflow",             CheckOverflow),
    NL_TEST_DEF("Timer::TestTimerStarvation",      CheckStarvation),
    NL_TEST_SENTINEL()
};

static int TestSetup(void* aContext);
static int TestTeardown(void* aContext);

static nlTestSuite kTheSuite = {
    "weave-system-timer",
    &sTests[0],
    TestSetup,
    TestTeardown
};

/**
 *  Set up the test suite.
 */
static int TestSetup(void* aContext)
{
    static Layer sLayer;

    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);
    void* lLayerContext = NULL;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    static sys_mbox* sLwIPEventQueue = NULL;

    sys_mbox_new(&sLwIPEventQueue, 100);
    tcpip_init(NULL, NULL);
    lLayerContext = &sLwIPEventQueue;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    sLayer.Init(lLayerContext);

    lContext.mLayer = &sLayer;
    lContext.mTestSuite = &kTheSuite;

    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 *  Free memory reserved at TestSetup.
 */
static int TestTeardown(void* aContext)
{
    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);

    lContext.mLayer->Shutdown();

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_finish(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one lContext.
    nlTestRunner(&kTheSuite, &sContext);

    return nlTestRunnerStats(&kTheSuite);
}

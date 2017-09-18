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
 *      This is a unit test suite for <tt>nl::Weave::System::Object</tt>, * the part of the Weave System Layer that implements
 *      objects and their static allocation pools.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <SystemLayer/SystemLayer.h>

#include <Weave/Support/ErrorStr.h>

#include <nltest.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/tcpip.h>
#include <lwip/sys.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
#include <pthread.h>
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// Test context

namespace {

extern nlTestSuite kTheSuite;

using nl::ErrorStr;
using namespace nl::Weave::System;

class TestObject : public Object
{
public:
    Error Init(void);

    static void CheckRetention(nlTestSuite* inSuite, void* aContext);
    static void CheckConcurrency(nlTestSuite* inSuite, void* aContext);

private:
    enum { kPoolSize = 1024 };
    static ObjectPool<TestObject, kPoolSize> sPool;

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
    unsigned int mDelay;

    enum { kNumThreads = 16, kLoopIterations = 100000, kMaxDelayIterations = 3 };
    void Delay(volatile unsigned int& aAccumulator);
    static void* CheckConcurrencyThread(void* aContext);
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

    // Not defined
    TestObject(const TestObject&);
    TestObject& operator =(const TestObject&);
};

ObjectPool<TestObject, TestObject::kPoolSize> TestObject::sPool;

Error TestObject::Init(void)
{
#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
    this->mDelay = kMaxDelayIterations > 0 ? 1 : 0;
    if (kMaxDelayIterations > 1)
    {
        this->mDelay += static_cast<unsigned int>(rand() % kMaxDelayIterations);
    }
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

    return WEAVE_SYSTEM_NO_ERROR;
}

struct TestContext {
    nlTestSuite*            mTestSuite;
    void*                   mLayerContext;
    volatile unsigned int   mAccumulator;
};

struct TestContext sContext;


// Test Object retention

void TestObject::CheckRetention(nlTestSuite* inSuite, void* aContext)
{
    TestContext&    lContext = *static_cast<TestContext*>(aContext);
    Layer           lLayer;
    unsigned int    i, j;

    lLayer.Init(lContext.mLayerContext);
    memset(&sPool, 0, sizeof(sPool));

    for (i = 0; i < kPoolSize; ++i)
    {
        TestObject* lCreated = sPool.TryCreate(lLayer);

        NL_TEST_ASSERT(lContext.mTestSuite, lCreated != NULL);
        if (lCreated == NULL)
            continue;
        NL_TEST_ASSERT(lContext.mTestSuite, lCreated->IsRetained(lLayer));
        NL_TEST_ASSERT(lContext.mTestSuite, &(lCreated->SystemLayer()) == &lLayer);

        lCreated->Init();

        for (j = 0; j < kPoolSize; ++j)
        {
            TestObject* lGotten = sPool.Get(lLayer, j);

            if (j > i)
            {
                NL_TEST_ASSERT(lContext.mTestSuite, lGotten == NULL);
            }
            else
            {
                NL_TEST_ASSERT(lContext.mTestSuite, lGotten != NULL);
                lGotten->Retain();
            }
        }
    }

    for (i = 0; i < kPoolSize; ++i)
    {
        TestObject* lGotten = sPool.Get(lLayer, i);

        NL_TEST_ASSERT(lContext.mTestSuite, lGotten != NULL);

        for (j = kPoolSize; j > i; --j)
        {
            NL_TEST_ASSERT(lContext.mTestSuite, lGotten->IsRetained(lLayer));
            lGotten->Release();
        }

        NL_TEST_ASSERT(lContext.mTestSuite, lGotten->IsRetained(lLayer));
        lGotten->Release();
        NL_TEST_ASSERT(lContext.mTestSuite, !lGotten->IsRetained(lLayer));
    }

    for (i = 0; i < kPoolSize; ++i)
    {
        TestObject* lGotten = sPool.Get(lLayer, i);

        NL_TEST_ASSERT(lContext.mTestSuite, lGotten == NULL);
    }

    lLayer.Shutdown();
}


// Test Object concurrency

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
void TestObject::Delay(volatile unsigned int& aAccumulator)
{
    unsigned int lSum = 0;

    if (kMaxDelayIterations > 0)
    {
        for (unsigned int z = 0; z < this->mDelay; ++z)
        {
            lSum += rand();
        }

        lSum = lSum / this->mDelay;
    }

    aAccumulator = lSum;
}

void* TestObject::CheckConcurrencyThread(void* aContext)
{
    const unsigned int  kNumObjects     = kPoolSize / kNumThreads;
    TestObject*         lObject         = NULL;
    TestContext&        lContext        = *static_cast<TestContext*>(aContext);
    Layer               lLayer;
    unsigned int        i;

    lLayer.Init(lContext.mLayerContext);

    for (i = 0; i < kNumObjects; ++i)
    {
        while (lObject == NULL)
        {
            lObject = sPool.TryCreate(lLayer);
        }

        NL_TEST_ASSERT(lContext.mTestSuite, lObject->IsRetained(lLayer));
        NL_TEST_ASSERT(lContext.mTestSuite, &(lObject->SystemLayer()) == &lLayer);

        lObject->Init();
        lObject->Delay(lContext.mAccumulator);
    }

    lObject = sPool.Get(lLayer, kPoolSize - 1);

    if (lObject != NULL)
    {
        lObject->Release();
        NL_TEST_ASSERT(lContext.mTestSuite, !lObject->IsRetained(lLayer));
    }

    for (i = 0; i < kLoopIterations; ++i)
    {
        unsigned int j;

        lObject = NULL;
        while (lObject == NULL)
        {
            lObject = sPool.TryCreate(lLayer);
        }

        NL_TEST_ASSERT(lContext.mTestSuite, lObject->IsRetained(lLayer));
        NL_TEST_ASSERT(lContext.mTestSuite, &(lObject->SystemLayer()) == &lLayer);

        lObject->Init();
        lObject->Delay(lContext.mAccumulator);

        j = kPoolSize;
        lObject = NULL;
        while (j-- > 0)
        {
            lObject = sPool.Get(lLayer, j);

            if (lObject == NULL) continue;

            lObject->Release();
            NL_TEST_ASSERT(lContext.mTestSuite, !lObject->IsRetained(lLayer));
            break;
        }

        NL_TEST_ASSERT(lContext.mTestSuite, lObject != NULL);
    }

    for (i = 0; i < kPoolSize; ++i)
    {
        lObject = sPool.Get(lLayer, i);

        if (lObject == NULL) continue;

        lObject->Release();
        NL_TEST_ASSERT(lContext.mTestSuite, !lObject->IsRetained(lLayer));
    }

    lLayer.Shutdown();

    return aContext;
}
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

void TestObject::CheckConcurrency(nlTestSuite* inSuite, void* aContext)
{
    TestContext& lContext = *static_cast<TestContext*>(aContext);
#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
    pthread_t lThread[kNumThreads];
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

    memset(&sPool, 0, sizeof(sPool));

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
    for (unsigned int i = 0; i < kNumThreads; ++i)
    {
        int lError = pthread_create(&lThread[i], NULL, CheckConcurrencyThread, &lContext);

        NL_TEST_ASSERT(lContext.mTestSuite, lError == 0);
    }

    for (unsigned int i = 0; i < kNumThreads; ++i)
    {
        int lError = pthread_join(lThread[i], NULL);

        NL_TEST_ASSERT(lContext.mTestSuite, lError == 0);
    }
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
}


// Test Suite


/**
 *   Test Suite. It lists all the test functions.
 */
const nlTest sTests[] = {
    NL_TEST_DEF("Retention", TestObject::CheckRetention),
    NL_TEST_DEF("Concurrency", TestObject::CheckConcurrency),
    NL_TEST_SENTINEL()
};

/**
 *  Initialize the test suite.
 */
int Initialize(void* aContext)
{
    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);
    void* lLayerContext = NULL;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    static sys_mbox* sLwIPEventQueue = NULL;

    if (sLwIPEventQueue == NULL)
    {
        sys_mbox_new(&sLwIPEventQueue, 100);
    }

    lLayerContext = &sLwIPEventQueue;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    lContext.mTestSuite = &kTheSuite;
    lContext.mLayerContext = lLayerContext;
    lContext.mAccumulator = 0;

    return SUCCESS;
}

/**
 *  Finalize the test suite.
 */
int Finalize(void* aContext)
{
    TestContext& lContext = *reinterpret_cast<TestContext*>(aContext);

    lContext.mTestSuite = NULL;

    return SUCCESS;
}

nlTestSuite kTheSuite = {
    "weave-system-object",
    &sTests[0],
    Initialize,
    Finalize
};

} // end anonymous namespace

int main(int argc, char *argv[])
{
    // Initialize standard pseudo-random number generator
    srand(0);

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one lContext.
    nlTestRunner(&kTheSuite, &sContext);

    return nlTestRunnerStats(&kTheSuite);
}

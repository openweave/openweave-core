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
 *      This file implements unit tests for the SubscriptionClient::PathStore
 *      class.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nltest.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <nest/test/trait/TestHTrait.h>

#include <new>
#include <map>
#include <set>
#include <algorithm>
#include <set>
#include <string>
#include <iterator>

using namespace nl;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Test::Trait;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// System/Platform definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {
namespace Platform {
    // for unit tests, the dummy critical section is sufficient.
    void CriticalSectionEnter()
    {
        return;
    }

    void CriticalSectionExit()
    {
        return;
    }
} // Platform

class PathStoreTest {
    public:
        PathStoreTest() :
            mTDH1(1), mSchemaEngine(&TestHTrait::TraitSchema) {}
        ~PathStoreTest() {}

        SubscriptionClient::PathStore mStore;
        TraitPath mPath;
        TraitDataHandle mTDH1;
        const TraitSchemaEngine *mSchemaEngine;

        void TestInitCleanup(nlTestSuite *inSuite, void *inContext);
        void TestAddGet(nlTestSuite *inSuite, void *inContext);
        void TestIntersects(nlTestSuite *inSuite, void *inContext);
};

void PathStoreTest::TestInitCleanup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = kRootPropertyPathHandle;

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == 1);

    mStore.Clear();

    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == 0);
}

void PathStoreTest::TestAddGet(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = kRootPropertyPathHandle;

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == 1);
    NL_TEST_ASSERT(inSuite, mStore.IsItemValid(0));
    NL_TEST_ASSERT(inSuite, mStore.IsPresent(mPath));

    mStore.GetItemAt(0, tp);
    NL_TEST_ASSERT(inSuite, tp == mPath);

    mStore.Clear();
}

void PathStoreTest::TestIntersects(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = TestHTrait::kPropertyHandle_K;

    err = mStore.AddItem(mPath);

    tp.mTraitDataHandle = mTDH1;
    tp.mPropertyPathHandle = TestHTrait::kPropertyHandle_Root;
    NL_TEST_ASSERT(inSuite, mStore.Intersects(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.Intersects(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa), 1);
    NL_TEST_ASSERT(inSuite, mStore.Intersects(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, false == mStore.Intersects(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L), 1);
    NL_TEST_ASSERT(inSuite, false == mStore.Intersects(tp, mSchemaEngine));

    mStore.Clear();
}


} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}
}
}

static SubscriptionEngine *gSubscriptionEngine;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return gSubscriptionEngine;
}

PathStoreTest gPathStoreTest;



void PathStoreTest_InitCleanup(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestInitCleanup(inSuite, inContext);
}

void PathStoreTest_AddGet(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestInitCleanup(inSuite, inContext);
}

void PathStoreTest_Intersects(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestIntersects(inSuite, inContext);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Init and cleanup",  PathStoreTest_InitCleanup),
    NL_TEST_DEF("AddItem and GetItem",  PathStoreTest_AddGet),
    NL_TEST_DEF("Intersects",  PathStoreTest_Intersects),


    NL_TEST_SENTINEL()
};

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {



} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl


/**
 *  Set up the test suite.
 */
static int TestSetup(void *inContext)
{
    gSubscriptionEngine = NULL;

    return 0;
}

/**
 *  Tear down the test suite.
 */
static int TestTeardown(void *inContext)
{
    return 0;
}

/**
 *  Main
 */
int main(int argc, char *argv[])
{
    nlTestSuite theSuite = {
        "weave-pathstore",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit against one context
    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

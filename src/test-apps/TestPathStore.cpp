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
 *      This file implements unit tests for the TraiPathStore
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

class TraitPathStoreTest {
    public:
        enum {
            kFlag_BadFlag = 0x1,
            kFlag_GoodFlag = 0x4,
            kFlag_GoodFlag2 = 0x8,
        };
        TraitPathStoreTest();
        ~TraitPathStoreTest() { }

        TraitPathStore mStore;
        TraitPathStore::Record mStorage[10];

        TraitPath mPath;
        TraitDataHandle mTDH1;
        TraitDataHandle mTDH2;
        const TraitSchemaEngine *mSchemaEngine;

        void TestInitCleanup(nlTestSuite *inSuite, void *inContext);
        void TestAddGet(nlTestSuite *inSuite, void *inContext);
        void TestFull(nlTestSuite *inSuite, void *inContext);
        void TestIncludes(nlTestSuite *inSuite, void *inContext);
        void TestIntersects(nlTestSuite *inSuite, void *inContext);
        void TestIsPresent(nlTestSuite *inSuite, void *inContext);
        void TestRemoveAndCompact(nlTestSuite *inSuite, void *inContext);
        void TestAddItemDedup(nlTestSuite *inSuite, void *inContext);
        void TestGetFirstGetNext(nlTestSuite *inSuite, void *inContext);
        void TestFlags(nlTestSuite *inSuite, void *inContext);
};

TraitPathStoreTest::TraitPathStoreTest() :
            mTDH1(1), mTDH2(2), mSchemaEngine(&TestHTrait::TraitSchema)
{
    mStore.Init(mStorage, ArraySize(mStorage));
}

void TraitPathStoreTest::TestInitCleanup(nlTestSuite *inSuite, void *inContext)
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

void TraitPathStoreTest::TestAddGet(nlTestSuite *inSuite, void *inContext)
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

void TraitPathStoreTest::TestFull(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    for (size_t i = 0; i < mStore.GetPathStoreSize(); i++)
    {
        NL_TEST_ASSERT(inSuite, false == mStore.IsFull());

        mPath.mTraitDataHandle = mTDH1;
        mPath.mPropertyPathHandle = kRootPropertyPathHandle+i;

        err = mStore.AddItem(mPath);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
        NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == i+1);
        NL_TEST_ASSERT(inSuite, mStore.IsPresent(mPath));
    }

    NL_TEST_ASSERT(inSuite, mStore.IsFull());
    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize());

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_NO_MEMORY);

    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize());

    mStore.Clear();

    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == 0);
}

void TraitPathStoreTest::TestIncludes(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    tp.mTraitDataHandle = mTDH1;
    tp.mPropertyPathHandle = TestHTrait::kPropertyHandle_Root;
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa), 1);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L), 1);
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    // Now add root as well; everything should be "included".
    mPath.mPropertyPathHandle = kRootPropertyPathHandle;

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    tp.mPropertyPathHandle = TestHTrait::kPropertyHandle_Root;
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa), 1);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L), 1);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    // A TraitPath for a different trait handler is not
    tp.mTraitDataHandle = mTDH2;
    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    mStore.Clear();
}

void TraitPathStoreTest::TestIntersects(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = TestHTrait::kPropertyHandle_K;

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

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

    tp.mTraitDataHandle = mTDH2;
    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, false == mStore.Intersects(tp, mSchemaEngine));

    mStore.Clear();
}

void TraitPathStoreTest::TestIsPresent(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;

    mStore.Clear();

    tp.mTraitDataHandle = mTDH1;
    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);

    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(mPath));

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.IsPresent(tp));
    NL_TEST_ASSERT(inSuite, mStore.IsTraitPresent(mTDH1));
    NL_TEST_ASSERT(inSuite, false == mStore.IsTraitPresent(mTDH2));

    tp.mPropertyPathHandle = kRootPropertyPathHandle;
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa), 1);
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_L), 1);
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));

    mStore.Clear();

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));
}

void TraitPathStoreTest::TestRemoveAndCompact(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;


    mStore.Clear();

    for (size_t i = 0; i < mStore.GetPathStoreSize(); i++)
    {
        mPath.mTraitDataHandle = mTDH1+i;
        mPath.mPropertyPathHandle = kRootPropertyPathHandle+i;

        err = mStore.AddItem(mPath);
        NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    }

    tp.mTraitDataHandle = mTDH1 + 1;
    tp.mPropertyPathHandle = kRootPropertyPathHandle + 1;
    NL_TEST_ASSERT(inSuite, mStore.IsPresent(tp));

    mStore.RemoveItemAt(1);

    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));
    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize()-1);

    mStore.RemoveTrait(mTDH1+2);

    tp.mTraitDataHandle = mTDH1 + 2;
    tp.mPropertyPathHandle = kRootPropertyPathHandle + 2;
    NL_TEST_ASSERT(inSuite, false == mStore.IsPresent(tp));
    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize()-2);

    mStore.RemoveItemAt(4);
    mStore.RemoveItemAt(5);
    mStore.RemoveItemAt(6);

    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize()-5);

    mStore.Compact();

    NL_TEST_ASSERT(inSuite, mStore.GetNumItems() == mStore.GetPathStoreSize()-5);

    for (size_t i = 0; i < mStore.GetPathStoreSize(); i++)
    {
        if (i < mStore.GetNumItems())
        {
            NL_TEST_ASSERT(inSuite, mStore.IsItemValid(i));
        }
        else
        {
            NL_TEST_ASSERT(inSuite, false == mStore.IsItemValid(i));
        }
    }

    tp.mTraitDataHandle = mTDH1 + 9;
    tp.mPropertyPathHandle = kRootPropertyPathHandle + 9;
    NL_TEST_ASSERT(inSuite, mStore.IsPresent(tp));

    mStore.Clear();
}

void TraitPathStoreTest::TestAddItemDedup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath tp;
    size_t numItems = 0;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    numItems = mStore.GetNumItems();

    tp.mTraitDataHandle = mTDH1;

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    err = mStore.AddItemDedup(tp, mSchemaEngine);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));
    NL_TEST_ASSERT(inSuite, numItems == mStore.GetNumItems());

    tp.mPropertyPathHandle = mSchemaEngine->GetDictionaryItemHandle(CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K_Sa), 1);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    err = mStore.AddItemDedup(tp, mSchemaEngine);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));
    NL_TEST_ASSERT(inSuite, numItems == mStore.GetNumItems());

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    err = mStore.AddItemDedup(tp, mSchemaEngine);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));
    NL_TEST_ASSERT(inSuite, (numItems+1) == mStore.GetNumItems());

    numItems = mStore.GetNumItems();


    // Add root: the number of items goes down to 1 and the previous two are still included
    tp.mPropertyPathHandle = TestHTrait::kPropertyHandle_Root;
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));

    err = mStore.AddItemDedup(tp, mSchemaEngine);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));
    NL_TEST_ASSERT(inSuite, (numItems-1) == mStore.GetNumItems());

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_I);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    tp.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));

    numItems = mStore.GetNumItems();

    // A TraitPath for a different trait handler
    tp.mTraitDataHandle = mTDH2;
    NL_TEST_ASSERT(inSuite, false == mStore.Includes(tp, mSchemaEngine));
    err = mStore.AddItemDedup(tp, mSchemaEngine);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    NL_TEST_ASSERT(inSuite, mStore.Includes(tp, mSchemaEngine));
    NL_TEST_ASSERT(inSuite, (numItems+1) == mStore.GetNumItems());

    mStore.Clear();
}

void TraitPathStoreTest::TestGetFirstGetNext(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t numItems = 0;
    size_t i;

    mStore.Clear();

    NL_TEST_ASSERT(inSuite, mStore.GetFirstValidItem() == mStore.GetPathStoreSize());
    NL_TEST_ASSERT(inSuite, mStore.GetFirstValidItem(mTDH1) == mStore.GetPathStoreSize());

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    i = mStore.GetFirstValidItem();
    NL_TEST_ASSERT(inSuite, i < mStore.GetPathStoreSize());
    NL_TEST_ASSERT(inSuite, mStore.GetNextValidItem(i) == mStore.GetPathStoreSize());

    NL_TEST_ASSERT(inSuite, mStore.GetFirstValidItem(mTDH2) == mStore.GetPathStoreSize());

    i = mStore.GetFirstValidItem(mTDH1);
    NL_TEST_ASSERT(inSuite, i < mStore.GetPathStoreSize());
    NL_TEST_ASSERT(inSuite, mStore.GetNextValidItem(i, mTDH1) == mStore.GetPathStoreSize());

    mStore.Clear();
}

void TraitPathStoreTest::TestFlags(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t numItems = 0;
    size_t i;

    mStore.Clear();

    mPath.mTraitDataHandle = mTDH1;
    mPath.mPropertyPathHandle = CreatePropertyPathHandle(TestHTrait::kPropertyHandle_K);

    err = mStore.AddItem(mPath);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    i = mStore.GetFirstValidItem();
    NL_TEST_ASSERT(inSuite, i < mStore.GetPathStoreSize());

    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_GoodFlag));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_GoodFlag2));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_BadFlag));

    mStore.Clear();

    err = mStore.AddItem(mPath, kFlag_BadFlag);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

    err = mStore.AddItem(mPath, kFlag_BadFlag | kFlag_GoodFlag);
    NL_TEST_ASSERT(inSuite, err == WEAVE_ERROR_INVALID_ARGUMENT);

    err = mStore.AddItem(mPath, kFlag_GoodFlag);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mStore.AreFlagsSet(i, kFlag_GoodFlag));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_GoodFlag2));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_GoodFlag | kFlag_GoodFlag2));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_BadFlag));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_GoodFlag | kFlag_BadFlag));

    mStore.Clear();

    err = mStore.AddItem(mPath, kFlag_GoodFlag | kFlag_GoodFlag2);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, mStore.AreFlagsSet(i, kFlag_GoodFlag));
    NL_TEST_ASSERT(inSuite, mStore.AreFlagsSet(i, kFlag_GoodFlag2));
    NL_TEST_ASSERT(inSuite, mStore.AreFlagsSet(i, kFlag_GoodFlag | kFlag_GoodFlag2));
    NL_TEST_ASSERT(inSuite, false == mStore.AreFlagsSet(i, kFlag_BadFlag));

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

TraitPathStoreTest gPathStoreTest;



void TraitPathStoreTest_InitCleanup(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestInitCleanup(inSuite, inContext);
}

void TraitPathStoreTest_AddGet(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestAddGet(inSuite, inContext);
}

void TraitPathStoreTest_Full(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestFull(inSuite, inContext);
}

void TraitPathStoreTest_Includes(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestIncludes(inSuite, inContext);
}

void TraitPathStoreTest_Intersects(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestIntersects(inSuite, inContext);
}

void TraitPathStoreTest_IsPresent(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestIsPresent(inSuite, inContext);
}

void TraitPathStoreTest_RemoveAndCompact(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestRemoveAndCompact(inSuite, inContext);
}

void TraitPathStoreTest_AddItemDedup(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestAddItemDedup(inSuite, inContext);
}

void TraitPathStoreTest_GetFirstGetNext(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestGetFirstGetNext(inSuite, inContext);
}

void TraitPathStoreTest_Flags(nlTestSuite *inSuite, void *inContext)
{
    gPathStoreTest.TestFlags(inSuite, inContext);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Init and cleanup",  TraitPathStoreTest_InitCleanup),
    NL_TEST_DEF("AddItem and GetItem",  TraitPathStoreTest_AddGet),
    NL_TEST_DEF("Full store",  TraitPathStoreTest_Full),
    NL_TEST_DEF("Includes",  TraitPathStoreTest_Includes),
    NL_TEST_DEF("Intersects",  TraitPathStoreTest_Intersects),
    NL_TEST_DEF("IsPresent",  TraitPathStoreTest_IsPresent),
    NL_TEST_DEF("Remove and Compact",  TraitPathStoreTest_RemoveAndCompact),
    NL_TEST_DEF("AddItemDedup",  TraitPathStoreTest_AddItemDedup),
    NL_TEST_DEF("GetFirstGetNext",  TraitPathStoreTest_GetFirstGetNext),
    NL_TEST_DEF("Flags",  TraitPathStoreTest_Flags),

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
        "weave-TraitPathStore",
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

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
 *      This file implements unit tests for the encoding of WDM UpdateRequest payloads.
 *
 */

#include "ToolCommon.h"

#include <nlbyteorder.h>
#include <nltest.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <nest/test/trait/TestATrait.h>

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

class WdmUpdateEncoderTest {
    public:
        WdmUpdateEncoderTest();
        ~WdmUpdateEncoderTest() { }

        TraitPathStore mPathList;
        TraitPathStore::Record mStorage[10];

        const TraitSchemaEngine *mSchemaEngine;

        void TestInitCleanup(nlTestSuite *inSuite, void *inContext);
};

WdmUpdateEncoderTest::WdmUpdateEncoderTest() :
            mSchemaEngine(&TestATrait::TraitSchema)
{
    mPathList.Init(mStorage, ArraySize(mStorage));
}

void WdmUpdateEncoderTest::TestInitCleanup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mPathList.Clear();

    NL_TEST_ASSERT(inSuite, mPathList.GetNumItems() == 0);
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

WdmUpdateEncoderTest gWdmUpdateEncoderTest;



void WdmUpdateEncoderTest_InitCleanup(nlTestSuite *inSuite, void *inContext)
{
    gWdmUpdateEncoderTest.TestInitCleanup(inSuite, inContext);
}

// Test Suite

/**
 *  Test Suite that lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Init and cleanup",  WdmUpdateEncoderTest_InitCleanup),

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
        "weave-WdmUpdateEncoder",
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

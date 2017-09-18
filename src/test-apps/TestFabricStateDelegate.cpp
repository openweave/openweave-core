/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file implements a unit test suite for the WeaveStateFabric
 *      StateChangedDelegate methods.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>

#include <nltest.h>

#include "ToolCommon.h"
#include "TestGroupKeyStore.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveConfig.h>

using namespace nl::Weave;

const uint64_t kTestNodeId = 0x18B43000002DCF71ULL;

static WeaveFabricState sFabricState;

class TestDelegate
        : public FabricStateDelegate
{
public:
    TestDelegate()
    {
        ClearState();
    }

    virtual void DidJoinFabric(WeaveFabricState *fabricState, uint64_t newFabricId)
    {
        mDidJoinFabricCalled = true;
        mNewFabricId = newFabricId;

    }
    virtual void DidLeaveFabric(WeaveFabricState *fabricState, uint64_t oldFabricId)
    {
        mDidLeaveFabricCalled = true;
        mOldFabricId = oldFabricId;
    }

    void ClearState(void)
    {
        mDidJoinFabricCalled = false;
        mDidLeaveFabricCalled = false;
        mOldFabricId = 0;
        mNewFabricId = 0;
    }

    bool CheckStateIsClear(void)
    {
        // Neither callback is invoked
        return (!mDidJoinFabricCalled  && !mDidLeaveFabricCalled);
    }

    // Indicate which callback was invoked
    bool mDidJoinFabricCalled;
    bool mDidLeaveFabricCalled;
    uint64_t mOldFabricId;
    uint64_t mNewFabricId;
};


static void Setup(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    static DEFINE_ALIGNED_VAR(sTestGroupKeyStore, sizeof(TestGroupKeyStore), void*);

    err = sFabricState.Init(new (&sTestGroupKeyStore) TestGroupKeyStore());

    sFabricState.LocalNodeId = kTestNodeId;

    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
}

static void CheckDelegateCallbacks(nlTestSuite *inSuite, void *inContext)
{
    WEAVE_ERROR err;
    TestDelegate delegate;
    uint64_t fabricId;
    uint8_t fabricStateBuffer[512];
    uint32_t fabricStateLen;

    // Set the delegate
    sFabricState.SetDelegate(&delegate);

    //-------------------------------------------------------------------
    // Check callback after CreateFabric()

    // Test the callback are invoked when creating a new fabric
    sFabricState.CreateFabric();
    NL_TEST_ASSERT(inSuite, delegate.mDidJoinFabricCalled == true);
    NL_TEST_ASSERT(inSuite, delegate.mDidLeaveFabricCalled == false);
    fabricId = delegate.mNewFabricId;

    // Clear the fabric, we should only get a didLeave callback now
    delegate.ClearState();
    NL_TEST_ASSERT(inSuite, delegate.CheckStateIsClear());

    sFabricState.ClearFabricState();

    NL_TEST_ASSERT(inSuite, delegate.mDidLeaveFabricCalled == true);
    NL_TEST_ASSERT(inSuite, delegate.mDidJoinFabricCalled == false);
    NL_TEST_ASSERT(inSuite, delegate.mOldFabricId == fabricId);

    // Clear again, should get no callback now
    delegate.ClearState();
    NL_TEST_ASSERT(inSuite, delegate.CheckStateIsClear());

    sFabricState.ClearFabricState();

    NL_TEST_ASSERT(inSuite, delegate.mDidLeaveFabricCalled == false);
    NL_TEST_ASSERT(inSuite, delegate.mDidJoinFabricCalled == false);

    //-------------------------------------------------------------------
    // Test for JoinExistingFabric

    // Create a random fabric and get the fabric state, then leave fabric

    err = sFabricState.CreateFabric();
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);
    fabricId = sFabricState.FabricId;

    err = sFabricState.GetFabricState(fabricStateBuffer, sizeof(fabricStateBuffer), fabricStateLen);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    sFabricState.ClearFabricState();

    // Now test the callbacks when joining existing fabric, the fabric id matches
    delegate.ClearState();
    NL_TEST_ASSERT(inSuite, delegate.CheckStateIsClear());

    err = sFabricState.JoinExistingFabric(fabricStateBuffer, fabricStateLen);
    NL_TEST_ASSERT(inSuite, err == WEAVE_NO_ERROR);

    NL_TEST_ASSERT(inSuite, delegate.mDidLeaveFabricCalled == false);
    NL_TEST_ASSERT(inSuite, delegate.mDidJoinFabricCalled == true);
    NL_TEST_ASSERT(inSuite, delegate.mNewFabricId == fabricId);

}

static const nlTest sTests[] = {
    NL_TEST_DEF("Setup",                     Setup),
    NL_TEST_DEF("DelegateCallback",          CheckDelegateCallbacks),
    NL_TEST_SENTINEL()
};

int main(void)
{
    WEAVE_ERROR err;

    nlTestSuite theSuite = {
        "FabricStateDelegate",
        &sTests[0]
    };

    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    FAIL_ERROR(err, "InitSecureRandomDataSource() failed");

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}

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
 *     This file implements a unit test suite for <tt>nl::Weave::Profiles::Vendor::Nestlabs::Thermostat</tt>,
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>

#include <Weave/Profiles/vendor/nestlabs/thermostat/NestThermostatWeaveConstants.h>

#include <nltest.h>

#include "ToolCommon.h"

using namespace nl::Weave::Profiles::Vendor::Nestlabs::Thermostat;

// Test input data.

static InFieldJoiningStatus sContext[] = {
    kStatus_InFieldJoining_Unknown,
    kStatus_InFieldJoining_Succeeded,
    kStatus_InFieldJoining_CannotLocateAssistingDevice,
    kStatus_InFieldJoining_CannotConnectAssistingDevice,
    kStatus_InFieldJoining_CannotAuthAssistingDevice,
    kStatus_InFieldJoining_ConfigExtractionError,
    kStatus_InFieldJoining_PANFormError,
    kStatus_InFieldJoining_PANJoinError,
    kStatus_InFieldJoining_HVACCycleInProgress,
    kStatus_InFieldJoining_HeatLinkJoinInProgress,
    kStatus_InFieldJoining_HeatLinkUpdateInProgress,
    kStatus_InFieldJoining_HeatLinkManualHeatActive,
    kStatus_InFieldJoining_IncorrectHeatLinkSoftwareVersion,
    kStatus_InFieldJoining_FailureToFetchAccessToken,
    kStatus_InFieldJoining_DeviceNotWeaveProvisioned,
    kStatus_InFieldJoining_HeatLinkResetFailed,
    kStatus_InFieldJoining_DestroyFabricFailed,
    kStatus_InFieldJoining_CannotJoinExistingFabric,
    kStatus_InFieldJoining_CannotCreateFabric,
    kStatus_InFieldJoining_NetworkReset,
    kStatus_InFieldJoining_JoiningInProgress,
    kStatus_InFieldJoining_FailureToMakePanJoinable,
    kStatus_InFieldJoining_WeaveConnectionTimeoutStillActive,
    kStatus_InFieldJoining_HeatLinkNotJoined,
    kStatus_InFieldJoining_HeatLinkNotInContact,
    kStatus_InFieldJoining_WiFiTechNotEnabled,
    kStatus_InFieldJoining_15_4_TechNotEnabled,
    kStatus_InFieldJoining_StandaloneFabricCreationInProgress,
    kStatus_InFieldJoining_NotConnectedToPower,
    kStatus_InFieldJoining_OperationNotPermitted,
    kStatus_InFieldJoining_ServiceTimedOut,
    kStatus_InFieldJoining_DeviceTimedOut,
    kStatus_InFieldJoining_InternalError
};

static const size_t kTestElements = sizeof(sContext) / sizeof(sContext[0]);


// Test Suite

static void CheckStatus(nlTestSuite *inSuite, void *inContext)
{
    char statusStr[50];

    // init statusStr with default status sting, which is for the status that does not have a well defined description string
    // then compare the actural IfjStatusStr(status) with statusStr to ensure that each status has its description sting
    for (size_t ith = 0; ith < kTestElements; ith++)
    {
        snprintf(statusStr, sizeof(statusStr), "IFJ Status %d: Invalid status", sContext[ith]);
        NL_TEST_ASSERT(inSuite, ((strcmp(IfjStatusStr(sContext[ith]), statusStr) != 0)));
    }
}

/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("Thermostat::Status",             CheckStatus),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite.
 *  This is a work-around to initiate InetBuffer protected class instance's
 *  data and set it to a known state, before an instance is created.
 */
static int TestSetup(void *inContext)
{
    return (SUCCESS);
}

/**
 *  Tear down the test suite.
 *  Free memory reserved at TestSetup.
 */
static int TestTeardown(void *inContext)
{
    return (SUCCESS);
}

int main(int argc, char *argv[])
{
    nlTestSuite theSuite = {
        "thermostat-status",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    // Generate machine-readable, comma-separated value (CSV) output.
    nl_test_set_output_style(OUTPUT_CSV);

    // Run test suit againt one context.
    nlTestRunner(&theSuite, &sContext);

    return nlTestRunnerStats(&theSuite);
}

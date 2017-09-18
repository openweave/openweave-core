/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX to be defined for pre-C++11 clib
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

// __STDC_CONSTANT_MACROS must be defined for INT64_C and UINT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

// it is important for this first inclusion of stdint.h to have all the right switches turned ON
#include <stdint.h>

// __STDC_FORMAT_MACROS must be defined for PRIX64 to be defined for pre-C++11 clib
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

// it is important for this first inclusion of inttypes.h to have all the right switches turned ON
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

// we might have to use gettimeofday for remote proximiation of clock_gettime(CLOCK_BOOTTIME)
#include <sys/time.h>

#include <InetLayer/InetError.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "TestPlatformTime.h"

#include "MockTimeSyncUtil.h"
#include "MockTimeSyncServer.h"
#include "MockTimeSyncClient.h"
#include "MockTimeSyncCoordinator.h"

#if WEAVE_CONFIG_TIME

using namespace nl::Weave::Profiles::Time;
using nl::Weave::WeaveExchangeManager;

// role is set to None, forcing it to be chosen before calling init
static MockTimeSyncRole gTimeSyncRole = kMockTimeSyncRole_None;

// objects for all three roles are all constructed, but not initialized
// this is for simplicity of implementation

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
static MockTimeSyncServer gMockServer;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
static MockTimeSyncClient gMockClient;
static OperatingMode gTimeSyncMode = kOperatingMode_AssignedLocalNodes;
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
static MockTimeSyncCoordinator gMockCoordinator;
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

WEAVE_ERROR MockTimeSync::Init(nl::Weave::WeaveExchangeManager * const exchangeMgr, uint64_t serviceNodeId, const char * serviceNodeAddr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (kMockTimeSyncRole_Server == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_SERVER
        printf("Initializing Mock Time Sync Server\n");
        err = gMockServer.Init(exchangeMgr);
#else // WEAVE_CONFIG_TIME_ENABLE_SERVER
        printf("Mock Time Sync Server not supported\n");
        err = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER
        SuccessOrExit(err);
    }
    else if (kMockTimeSyncRole_Client == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
        printf("Initializing Mock Time Sync Client\n");
        err = gMockClient.Init(exchangeMgr, gTimeSyncMode, serviceNodeId, serviceNodeAddr);
#else // WEAVE_CONFIG_TIME_ENABLE_CLIENT
        printf("Mock Time Sync Client not supported\n");
        err = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
        SuccessOrExit(err);
    }
    else if (kMockTimeSyncRole_Coordinator == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        printf("Initializing Mock Time Sync Coordinator\n");
        err = gMockCoordinator.Init(exchangeMgr);
#else // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        printf("Mock Time Sync Coordinator not supported\n");
        err = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        SuccessOrExit(err);
    }
    else if (kMockTimeSyncRole_None == gTimeSyncRole)
    {
        printf("Mock Time Sync is disabled and not initialized\n");
    }
    else
    {
        printf("ERROR: Mock Time Sync Role is unknown\n");
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // seed the random number generation with microseconds, for we almost always test with multiple mock-devices
    // all started in short time period. the common practice of time(NULL) would give the same reading for all of them.
    (void) nl::Weave::Platform::Time::GetSleepCompensatedMonotonicTime(&nl::Weave::MockPlatform::gTestOffsetToSystemTime_usec);
    srand(nl::Weave::MockPlatform::gTestOffsetToSystemTime_usec % UINT32_MAX);
    // modify this if you want a fixed for adjustable initial offset
    nl::Weave::MockPlatform::gTestOffsetToSystemTime_usec = (rand() % 10000000) - 5000000;
    printf("Mock System Time Offset initialized to: %f sec\n", nl::Weave::MockPlatform::gTestOffsetToSystemTime_usec * 1e-6);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR MockTimeSync::SetMode(const OperatingMode mode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit((kOperatingMode_Auto == mode) ||
                 (kOperatingMode_AssignedLocalNodes == mode) ||
                 (kOperatingMode_Service == mode) ||
                 (kOperatingMode_ServiceOverTunnel == mode),
                 err = WEAVE_ERROR_INCORRECT_STATE);

    gTimeSyncMode = mode;

exit:
    return err;
}

WEAVE_ERROR MockTimeSync::SetRole(const MockTimeSyncRole role)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // we only allow the role to be set once
    if (kMockTimeSyncRole_None != gTimeSyncRole)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }
    gTimeSyncRole = role;

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR MockTimeSync::Shutdown()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (kMockTimeSyncRole_Server == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_SERVER
        printf("Shutting down Mock Time Sync Server\n");
        err = gMockServer.Shutdown();
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER
    }
    else if (kMockTimeSyncRole_Client == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
        printf("Shutting down Mock Time Sync Client\n");
        err = gMockClient.Shutdown();
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
    }
    else if (kMockTimeSyncRole_Coordinator == gTimeSyncRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        printf("Shutting down Mock Time Sync Coordinator\n");
        err = gMockCoordinator.Shutdown();
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
    }
    else if (kMockTimeSyncRole_None == gTimeSyncRole)
    {
        printf("Mock Time Sync is not initialized so no shutdown is necessary\n");
    }
    else
    {
        printf("ERROR: Mock Time Sync Role is unknown\n");
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_TIME

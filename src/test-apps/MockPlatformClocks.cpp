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

/**
 *    @file
 *      This file contains a sample implementation of platform-provided timing routines
 *      under the Weave::Platform::Time namespace. This sample implementation provides
 *      extra test hooks.
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>

#include "MockPlatformClocks.h"
#include <SystemLayer/SystemClock.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>


namespace nl {
namespace Weave {
namespace MockPlatform {

static uint64_t GetClock_Monotonic(void);
static uint64_t GetClock_MonotonicMS(void);
static uint64_t GetClock_MonotonicHiRes(void);
static System::Error GetClock_RealTime(uint64_t & curTime);
static System::Error GetClock_RealTimeMS(uint64_t & curTimeMS);
static System::Error SetClock_RealTime(uint64_t newCurTime);

/**
 * The System Layer platform clock functions are decoupled from their
 * implementation using this structure of function pointers.
 * This allows test applications to override the implementation of
 * any of the functions with a private one.
 */
struct MockPlatformClocks gMockPlatformClocks =
{
    GetClock_Monotonic,
    GetClock_MonotonicMS,
    GetClock_MonotonicHiRes,
    GetClock_RealTime,
    GetClock_RealTimeMS,
    SetClock_RealTime,
    0,
    false,
    false
};

void MockPlatformClocks::SetRandomRealTimeOffset()
{
    RealTimeOffset_usec = (rand() % 10000000) - 5000000;
    WeaveLogProgress(TimeService, "Mock real time offset set to: %" PRId64 " usec", RealTimeOffset_usec);
}

uint64_t GetClock_Monotonic(void)
{
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    VerifyOrDie(res == 0);
    return (tv.tv_sec * UINT64_C(1000000)) + tv.tv_usec;
}

uint64_t GetClock_MonotonicMS(void)
{
    return gMockPlatformClocks.GetClock_Monotonic() / 1000;
}

uint64_t GetClock_MonotonicHiRes(void)
{
    return gMockPlatformClocks.GetClock_Monotonic();
}

System::Error GetClock_RealTime(uint64_t & curTime)
{
    if (gMockPlatformClocks.RealTimeUnavailable)
    {
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
    curTime = GetClock_Monotonic();
    curTime += gMockPlatformClocks.RealTimeOffset_usec;
    WeaveLogProgress(TimeService, "Mock real time %f sec", curTime * 1e-6);
    WeaveLogProgress(TimeService, "Mock real time offset: %" PRId64 " usec", gMockPlatformClocks.RealTimeOffset_usec);
    return WEAVE_SYSTEM_NO_ERROR;
}

System::Error GetClock_RealTimeMS(uint64_t & curTime)
{
    System::Error err = gMockPlatformClocks.GetClock_RealTime(curTime);
    if (err == WEAVE_SYSTEM_NO_ERROR)
    {
        curTime /= 1000;
    }
    return err;
}

System::Error SetClock_RealTime(uint64_t newCurTime)
{
    System::Error err = WEAVE_SYSTEM_NO_ERROR;
    uint64_t curTime;
    int64_t delta;

    gMockPlatformClocks.SetRealTimeCalled = true;

    err = gMockPlatformClocks.GetClock_RealTime(curTime);
    SuccessOrExit(err);

    delta = (curTime <= newCurTime) ? static_cast<int64_t>(newCurTime - curTime) : -static_cast<int64_t>(curTime - newCurTime);

    gMockPlatformClocks.RealTimeOffset_usec += delta;

    if (newCurTime != 0)
    {
        gMockPlatformClocks.RealTimeUnavailable = false;
        WeaveLogProgress(TimeService, "Mock real time set to %f sec", newCurTime * 1e-6);
    }
    else
    {
        gMockPlatformClocks.RealTimeUnavailable = true;
        WeaveLogProgress(TimeService, "Mock real time set to UNAVAILABLE");
    }
    WeaveLogProgress(TimeService, "New mock real time offset: %" PRId64 " usec", gMockPlatformClocks.RealTimeOffset_usec);
    WeaveLogProgress(TimeService, "Adjustment to mock real time offset %f sec", delta * 1e-6);

exit:
    return err;
}

} // namespace MockPlatform
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

using namespace ::nl::Weave::MockPlatform;

uint64_t GetClock_Monotonic(void)
{
    return gMockPlatformClocks.GetClock_Monotonic();
}

uint64_t GetClock_MonotonicMS(void)
{
    return gMockPlatformClocks.GetClock_MonotonicMS();
}

uint64_t GetClock_MonotonicHiRes(void)
{
    return gMockPlatformClocks.GetClock_MonotonicHiRes();
}

System::Error GetClock_RealTime(uint64_t & curTime)
{
    return gMockPlatformClocks.GetClock_RealTime(curTime);
}

System::Error GetClock_RealTimeMS(uint64_t & curTime)
{
    return gMockPlatformClocks.GetClock_RealTimeMS(curTime);
}

System::Error SetClock_RealTime(uint64_t newCurTime)
{
    return gMockPlatformClocks.SetClock_RealTime(newCurTime);
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl

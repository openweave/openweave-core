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

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "TestPlatformTime.h"

#if WEAVE_CONFIG_TIME

using namespace nl::Weave::Profiles::Time;

namespace nl {
namespace Weave {
namespace MockPlatform {

/**
 * Offset to system time for testing purposes
 */
timesync_t gTestOffsetToSystemTime_usec;

/**
 * The Platform::Time external functions are decoupled from their
 * implementation using this structure of function pointers.
 * This allows test applications to override the implementation of
 * any of the functions with a private one.
 */
struct TestPlatformTimeFns gTestPlatformTimeFns = {
    nl::Weave::MockPlatform::GetMonotonicRawTime,
    nl::Weave::MockPlatform::GetSystemTime,
    nl::Weave::MockPlatform::GetSystemTimeMs,
    nl::Weave::MockPlatform::GetSleepCompensatedMonotonicTime,
    nl::Weave::MockPlatform::SetSystemTime
};


#if HAVE_CLOCK_GETTIME && HAVE_DECL_CLOCK_BOOTTIME

/**
 * Convert timespec, the data structure used in clock_gettime, to timesync_t, which we use
 *
 * @param[in] src           The timespec instance
 * @param[out] p_dst_usec   Pointer to the destination timesync_t instance
 *
 * @return WEAVE_ERROR_INVALID_ARGUMENT is the input is out of range
 */
static WEAVE_ERROR ConvertTimespecToTimesync(const timespec & src, timesync_t * const p_dst_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((src.tv_nsec < 0) || (src.tv_nsec > 1000000000LL) || (src.tv_sec < 0) || (src.tv_sec > MAX_TIMESYNC_SEC))
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    *p_dst_usec = timesync_t(src.tv_sec) * 1000000;
    *p_dst_usec += (src.tv_nsec / 1000);

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSleepCompensatedMonotonicTime
 */
WEAVE_ERROR GetSleepCompensatedMonotonicTime(timesync_t * const p_timestamp_usec)
{
    const int clock_id = CLOCK_BOOTTIME;

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    timespec ts_now;

    if (0 != clock_gettime(clock_id, &ts_now))
    {
        err = nl::Weave::System::MapErrorPOSIX(errno);
        goto exit;
    }

    err = ConvertTimespecToTimesync(ts_now, p_timestamp_usec);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetMonotonicRawTime
 */
WEAVE_ERROR GetMonotonicRawTime(timesync_t * const p_timestamp_usec)
{
    // monotonic_raw is slightly more stable than monotonic, as it is not corrected/adjusted
#if defined(CLOCK_MONOTONIC_RAW)
    const int clock_id = CLOCK_MONOTONIC_RAW;
#else
    const int clock_id = CLOCK_MONOTONIC;
#endif

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    timespec ts_now;

    if (0 != clock_gettime(clock_id, &ts_now))
    {
        err = nl::Weave::System::MapErrorPOSIX(errno);
        goto exit;
    }

    err = ConvertTimespecToTimesync(ts_now, p_timestamp_usec);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSystemTime
 */
WEAVE_ERROR GetSystemTime(timesync_t * const p_timestamp_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    static timesync_t offsetMonotonicRawToSystemTime = TIMESYNC_INVALID;
    timesync_t rawNow_usec;

    err = nl::Weave::Platform::Time::GetMonotonicRawTime(&rawNow_usec);
    SuccessOrExit(err);

    WeaveLogProgress(TimeService, "Now (monotonic raw): %" PRId64 " usec", rawNow_usec);

    if (TIMESYNC_INVALID == offsetMonotonicRawToSystemTime)
    {
        timespec ts_now;


        if (0 != clock_gettime(CLOCK_REALTIME, &ts_now))
        {
            err = nl::Weave::System::MapErrorPOSIX(errno);
            goto exit;
        }

        err = ConvertTimespecToTimesync(ts_now, p_timestamp_usec);
        SuccessOrExit(err);

        offsetMonotonicRawToSystemTime = *p_timestamp_usec - rawNow_usec;
    }
    else
    {
        *p_timestamp_usec = rawNow_usec + offsetMonotonicRawToSystemTime;
    }

    // apply fake offset for testing purpose
    *p_timestamp_usec += gTestOffsetToSystemTime_usec;

    WeaveLogProgress(TimeService, "Mock offset: %" PRId64 " usec", gTestOffsetToSystemTime_usec);

    WeaveLogProgress(TimeService, "Mock System Time %f sec", (*p_timestamp_usec) * 1e-6);

exit:
    WeaveLogFunctError(err);

    return err;
}

#elif HAVE_GETTIMEOFDAY // HAVE_CLOCK_GETTIME && HAVE_DECL_CLOCK_BOOTTIME

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSleepCompensatedMonotonicTime
 */
WEAVE_ERROR GetSleepCompensatedMonotonicTime(timesync_t * const p_timestamp_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    timeval ts_now;

    if (0 != gettimeofday(&ts_now, NULL))
    {
        err = nl::Weave::System::MapErrorPOSIX(errno);
        goto exit;
    }

    *p_timestamp_usec = timesync_t(ts_now.tv_sec) * 1000000;
    *p_timestamp_usec += ts_now.tv_usec;

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetMonotonicRawTime
 */
WEAVE_ERROR GetMonotonicRawTime(timesync_t * const p_timestamp_usec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.GetSleepCompensatedMonotonicTime(p_timestamp_usec);
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSystemTime
 */
WEAVE_ERROR GetSystemTime(timesync_t * const p_timestamp_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = gTestPlatformTimeFns.GetSleepCompensatedMonotonicTime(p_timestamp_usec);
    SuccessOrExit(err);

    // apply fake offset for testing purpose
    *p_timestamp_usec += gTestOffsetToSystemTime_usec;
    WeaveLogProgress(TimeService, "Mock System Time %f sec", (*p_timestamp_usec) * 1e-6);

exit:
    WeaveLogFunctError(err);

    return err;
}

#else // HAVE_CLOCK_GETTIME && HAVE_DECL_CLOCK_BOOTTIME

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSleepCompensatedMonotonicTime
 */
WEAVE_ERROR GetSleepCompensatedMonotonicTime(timesync_t * const p_timestamp_usec)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetMonotonicRawTime
 */
WEAVE_ERROR GetMonotonicRawTime(timesync_t * const p_timestamp_usec)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSystemTime
 */
WEAVE_ERROR GetSystemTime(timesync_t * const p_timestamp_usec)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

#endif // HAVE_CLOCK_GETTIME && HAVE_DECL_CLOCK_BOOTTIME

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::GetSystemTimeMs
 */
WEAVE_ERROR GetSystemTimeMs(timesync_t * const p_timestamp_msec)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

/**
 * Mock platform internal implementation of @see nl::Weave::Platform::Time::SetSystemTime
 */
WEAVE_ERROR SetSystemTime(const timesync_t timestamp_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    timesync_t modifiedSystemTime_usec = 0;

    if ((timestamp_usec < 0) || (timestamp_usec & MASK_INVALID_TIMESYNC))
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // apply correction to the mock system time
    err = gTestPlatformTimeFns.GetSystemTime(&modifiedSystemTime_usec);
    SuccessOrExit(err);
    gTestOffsetToSystemTime_usec += (timestamp_usec - modifiedSystemTime_usec);

    WeaveLogProgress(TimeService, "Correction to system time %f sec", (timestamp_usec - modifiedSystemTime_usec) * 1e-6);

exit:
    WeaveLogFunctError(err);

    return err;
}

} // namespace MockPlatform
} // namespace Weave
} // namespace nl

WEAVE_ERROR nl::Weave::Platform::Time::GetMonotonicRawTime(Profiles::Time::timesync_t * const p_timestamp_usec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.GetMonotonicRawTime(p_timestamp_usec);
}

WEAVE_ERROR nl::Weave::Platform::Time::GetSystemTime(Profiles::Time::timesync_t * const p_timestamp_usec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.GetSystemTime(p_timestamp_usec);
}

WEAVE_ERROR nl::Weave::Platform::Time::GetSystemTimeMs(Profiles::Time::timesync_t * const p_timestamp_msec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.GetSystemTimeMs(p_timestamp_msec);
}

WEAVE_ERROR nl::Weave::Platform::Time::SetSystemTime(const Profiles::Time::timesync_t timestamp_usec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.SetSystemTime(timestamp_usec);
}

WEAVE_ERROR nl::Weave::Platform::Time::GetSleepCompensatedMonotonicTime(Profiles::Time::timesync_t * const p_timestamp_usec)
{
    return nl::Weave::MockPlatform::gTestPlatformTimeFns.GetSleepCompensatedMonotonicTime(p_timestamp_usec);
}

#endif // WEAVE_CONFIG_TIME

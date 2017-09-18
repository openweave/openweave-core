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
 *      This file declares test hooks provided by
 *      the test implementation of the Weave::Platform::Time API
 */

#ifndef __TEST_PLATFORM_TIME_HPP
#define __TEST_PLATFORM_TIME_HPP

#if WEAVE_CONFIG_TIME
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/time/WeaveTime.h>

namespace nl {
namespace Weave {
namespace MockPlatform {

extern nl::Weave::Profiles::Time::timesync_t gTestOffsetToSystemTime_usec;

typedef WEAVE_ERROR (*GetTimeFn)(nl::Weave::Profiles::Time::timesync_t * const p_timestamp_msec);
typedef WEAVE_ERROR (*SetTimeFn)(const nl::Weave::Profiles::Time::timesync_t timestamp_usec);

struct TestPlatformTimeFns {
    GetTimeFn GetMonotonicRawTime;
    GetTimeFn GetSystemTime;
    GetTimeFn GetSystemTimeMs;
    GetTimeFn GetSleepCompensatedMonotonicTime;
    SetTimeFn SetSystemTime;
};

extern struct TestPlatformTimeFns gTestPlatformTimeFns;

NL_DLL_EXPORT WEAVE_ERROR GetMonotonicRawTime(nl::Weave::Profiles::Time::timesync_t * const p_timestamp_usec);
NL_DLL_EXPORT WEAVE_ERROR GetSystemTime(nl::Weave::Profiles::Time::timesync_t * const p_timestamp_usec);
NL_DLL_EXPORT WEAVE_ERROR GetSystemTimeMs(nl::Weave::Profiles::Time::timesync_t * const p_timestamp_msec);
NL_DLL_EXPORT WEAVE_ERROR SetSystemTime(const nl::Weave::Profiles::Time::timesync_t timestamp_usec);
NL_DLL_EXPORT WEAVE_ERROR GetSleepCompensatedMonotonicTime(nl::Weave::Profiles::Time::timesync_t * const p_timestamp_usec);

} // namespace MockPlatform
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_TIME

#endif // __TEST_PLATFORM_TIME_HPP

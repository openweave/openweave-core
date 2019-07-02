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

#ifndef WEAVE_DEVICE_SYSTEM_CLOCK_UNIT_TEST_H
#define WEAVE_DEVICE_SYSTEM_CLOCK_UNIT_TEST_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/TimeUtils.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

inline void RunSystemClockUnitTest(void)
{
    WEAVE_ERROR err;
    uint64_t clock, clock2;

    constexpr const uint64_t kEpochTime_20180101 = 1514764800;
    constexpr const uint64_t kEpochTime_20500101 = 2524608000;

    // Sanity check microsecond monotonic clock tick
    clock = ::nl::Weave::System::Layer::GetClock_Monotonic();
    while (clock == ::nl::Weave::System::Layer::GetClock_Monotonic());

    // Sanity check millisecond monotonic clock tick
    clock = ::nl::Weave::System::Layer::GetClock_MonotonicMS();
    while (clock == ::nl::Weave::System::Layer::GetClock_Monotonic());

    // Sanity check hi-res monotonic clock tick
    clock = ::nl::Weave::System::Layer::GetClock_MonotonicMS();
    while (clock == ::nl::Weave::System::Layer::GetClock_MonotonicMS());

    // Set the real-time clock value to a "contemporary" value.
    err = ::nl::Weave::System::Layer::SetClock_RealTime(kEpochTime_20180101 * kMicrosecondsPerSecond);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Fetch the real-time clock and verify the value.
    err = ::nl::Weave::System::Layer::GetClock_RealTime(clock);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    clock /= kMicrosecondsPerSecond;
    VerifyOrDie(clock >= (kEpochTime_20180101 - 1) && clock <= (kEpochTime_20180101 + 1));

    // Set the real-time clock value to a far future value.
    err = ::nl::Weave::System::Layer::SetClock_RealTime(kEpochTime_20500101 * kMicrosecondsPerSecond);
    VerifyOrDie(err == WEAVE_NO_ERROR);

    // Fetch the real-time clock and verify the value.
    err = ::nl::Weave::System::Layer::GetClock_RealTime(clock);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    clock /= kMicrosecondsPerSecond;
    VerifyOrDie(clock >= (kEpochTime_20500101 - 1) && clock <= (kEpochTime_20500101 + 1));

    // Sanity check the real-time clock tick
    err = ::nl::Weave::System::Layer::GetClock_RealTime(clock);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    do
    {
        err = ::nl::Weave::System::Layer::GetClock_RealTime(clock2);
        VerifyOrDie(err == WEAVE_NO_ERROR);
    } while (clock2 == clock);

    // Check the millisecond real-time clock value.
    err = ::nl::Weave::System::Layer::GetClock_RealTime(clock);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    err = ::nl::Weave::System::Layer::GetClock_RealTimeMS(clock2);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    clock /= 1000;
    VerifyOrDie(clock >= (clock2 - 1) && clock <= (clock2 + 1));

    // Reset the real-time clock and check for "TIME NOT SYNCED" error.
    err = ::nl::Weave::System::Layer::SetClock_RealTime(0);
    VerifyOrDie(err == WEAVE_NO_ERROR);
    err = ::nl::Weave::System::Layer::GetClock_RealTime(clock);
    VerifyOrDie(err == WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED);
}

} // namespace internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // WEAVE_DEVICE_SYSTEM_CLOCK_UNIT_TEST_H

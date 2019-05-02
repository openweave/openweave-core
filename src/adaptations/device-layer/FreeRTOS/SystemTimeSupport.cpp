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
 *          Provides implementations of the Weave System Layer platform
 *          time/clock functions based on the FreeRTOS tick counter.
 */


#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Support/TimeUtils.h>

#include "FreeRTOS.h"

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

namespace {

constexpr uint32_t kTicksOverflowShift = (configUSE_16_BIT_TICKS) ? 16 : 32;

uint64_t sBootTimeUS = 0;

uint64_t TicksSinceBoot(void)
{
    TimeOut_t timeOut;
    vTaskSetTimeOutState(&timeOut);
    return static_cast<uint64_t>(timeOut.xTimeOnEntering) +
          (static_cast<uint64_t>(timeOut.xOverflowCount) << kTicksOverflowShift);
}

} // unnamed namespace

uint64_t GetClock_Monotonic(void)
{
    return (TicksSinceBoot() * kMicrosecondsPerSecond) / configTICK_RATE_HZ;
}

uint64_t GetClock_MonotonicMS(void)
{
    return (TicksSinceBoot() * kMillisecondPerSecond) / configTICK_RATE_HZ;
}

uint64_t GetClock_MonotonicHiRes(void)
{
    return GetClock_Monotonic();
}

Error GetClock_RealTime(uint64_t & curTime)
{
    if (sBootTimeUS == 0)
    {
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
    curTime = sBootTimeUS + GetClock_Monotonic();
    return WEAVE_SYSTEM_NO_ERROR;
}

Error GetClock_RealTimeMS(uint64_t & curTime)
{
    if (sBootTimeUS == 0)
    {
        return WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED;
    }
    curTime = (sBootTimeUS + GetClock_Monotonic()) / 1000;
    return WEAVE_SYSTEM_NO_ERROR;
}

Error SetClock_RealTime(uint64_t newCurTime)
{
    uint64_t timeSinceBootUS = GetClock_Monotonic();
    if (newCurTime > timeSinceBootUS)
    {
        sBootTimeUS = newCurTime - timeSinceBootUS;
    }
    else
    {
        sBootTimeUS = 0;
    }
    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl

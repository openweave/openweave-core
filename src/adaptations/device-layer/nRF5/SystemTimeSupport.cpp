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
 *          time/clock functions for use on Nordic nRF5* platforms.
 */


#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Support/TimeUtils.h>

namespace nl {
namespace Weave {
namespace System {
namespace Platform {
namespace Layer {

uint64_t GetClock_Monotonic(void)
{
    // TODO: implement me
    return 0;
}

uint64_t GetClock_MonotonicMS(void)
{
    // TODO: implement me
    return 0;
}

uint64_t GetClock_MonotonicHiRes(void)
{
    // TODO: implement me
    return 0;
}

Error GetClock_RealTime(uint64_t & curTime)
{
    // TODO: implement me
    curTime = 0;
    return WEAVE_SYSTEM_NO_ERROR;
}

Error GetClock_RealTimeMS(uint64_t & curTime)
{
    // TODO: implement me
    curTime = 0;
    return WEAVE_SYSTEM_NO_ERROR;
}

Error SetClock_RealTime(uint64_t newCurTime)
{
    // TODO: implement me
    return WEAVE_SYSTEM_NO_ERROR;
}

} // namespace Layer
} // namespace Platform
} // namespace System
} // namespace Weave
} // namespace nl

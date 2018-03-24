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

#include <Weave/Core/WeaveCore.h>

namespace nl {
namespace Weave {
namespace MockPlatform {

struct MockPlatformClocks
{
    uint64_t (*GetClock_Monotonic)(void);
    uint64_t (*GetClock_MonotonicMS)(void);
    uint64_t (*GetClock_MonotonicHiRes)(void);
    System::Error (*GetClock_RealTime)(uint64_t & curTime);
    System::Error (*GetClock_RealTimeMS)(uint64_t & curTimeMS);
    System::Error (*SetClock_RealTime)(uint64_t newCurTime);

    int64_t RealTimeOffset_usec;
    bool RealTimeUnavailable;
    bool SetRealTimeCalled;

    void SetRandomRealTimeOffset(void);
};

extern struct MockPlatformClocks gMockPlatformClocks;

} // namespace MockPlatform
} // namespace Weave
} // namespace nl

#endif // __TEST_PLATFORM_TIME_HPP

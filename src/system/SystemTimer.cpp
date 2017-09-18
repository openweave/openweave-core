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
 *      This file defines the member functions and private data for
 *      the nl::Weave::System::Timer class, which is used for
 *      representing an in-progress one-shot timer.
 */

// Include module header
#include <SystemLayer/SystemTimer.h>

// Include common private header
#include "SystemLayerPrivate.h"

// Include local headers
#include <string.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/sys.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#if !(HAVE_CLOCK_GETTIME) && HAVE_GETTIMEOFDAY

#include <errno.h>
#include <sys/time.h>

#if !(HAVE_CLOCKID_T)
typedef int clockid_t;
#endif

extern "C" int clock_gettime(clockid_t clk_id, struct timespec* t);

#endif // !(HAVE_CLOCK_GETTIME) && HAVE_GETTIMEOFDAY
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#include <SystemLayer/SystemError.h>
#include <SystemLayer/SystemLayer.h>
#include <SystemLayer/SystemFaultInjection.h>

#include <Weave/Support/CodeUtils.h>

/*******************************************************************************
 * Timer state
 *
 * There are two fundamental state-change variables: Object::mSystemLayer and
 * Timer::OnComplete. These must be checked and changed atomically. The state
 * of the timer is governed by the following state machine:
 *
 *  INITIAL STATE: mSystemLayer == NULL, OnComplete == NULL
 *      |
 *      V
 *  UNALLOCATED<-----------------------------+
 *      |                                    |
 *  (set mSystemLayer != NULL)               |
 *      |                                    |
 *      V                                    |
 *  ALLOCATED-------(set mSystemLayer NULL)--+
 *      |    \-----------------------------+
 *      |                                  |
 *  (set OnComplete != NULL)               |
 *      |                                  |
 *      V                                  |
 *    ARMED ---------( clear OnComplete )--+
 *
 * When in the ARMED state:
 *
 *     * None of the member variables may mutate.
 *     * OnComplete must only be cleared by Cancel() or HandleComplete()
 *     * Cancel() and HandleComplete() will test that they are the one to
 *       successfully set OnComplete NULL. And if so, that will be the
 *       thread that must call Object::Release().
 *
 *******************************************************************************
 */

namespace nl {
namespace Weave {
namespace System {

ObjectPool<Timer, WEAVE_SYSTEM_CONFIG_NUM_TIMERS> Timer::sPool;

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if HAVE_DECL_CLOCK_BOOTTIME
// CLOCK_BOOTTIME is a Linux-specific option to clock_gettime for a clock which compensates for system sleep
#define NL_SYSTEM_TIMER_CLOCK_ID CLOCK_BOOTTIME
#elif HAVE_DECL_CLOCK_MONOTONIC
// CLOCK_MONOTONIC is defined in POSIX and hence is the default choice
#define NL_SYSTEM_TIMER_CLOCK_ID CLOCK_MONOTONIC
#else
// in case there is no POSIX-compliant clock_gettime, we're most likely going to use the emulation
// implementation provided in this file, which only provides emulation for 1 clock
#define NL_SYSTEM_TIMER_CLOCK_ID 0
#endif // HAVE_DECL_CLOCK_BOOTTIME

#if !(HAVE_CLOCK_GETTIME) && HAVE_GETTIMEOFDAY

/**
 *  This implements a version of the of the POSIX clock_gettime method based on gettimeofday
 *
 *  @param[in]  clk_id      The identifier of the particular clock on which to get the time.
 *  @param[out] t           A timespec structure that will be filled in on success.
 *
 *  @retval 0 on success; otherwise, -1 on failure (in which case errno is set appropriately).
 *
 */
extern "C" int clock_gettime(clockid_t clk_id, struct timespec* t)
{
    struct timeval now;
    int            retval = 0;

    if (clk_id != NL_SYSTEM_TIMER_CLOCK_ID)
    {
        errno = EINVAL;
        retval = -1;
    }
    else
    {
        retval = gettimeofday(&now, NULL);

        if (retval == 0)
        {
            t->tv_sec  = now.tv_sec;
            t->tv_nsec = now.tv_usec * 1000;
        }
    }

    return retval;
}
#endif // !(HAVE_CLOCK_GETTIME) && HAVE_GETTIMEOFDAY

/**
 *  This method returns the current epoch, corrected by system sleep with the system timescale, in milliseconds.
 *
 *  @return A timestamp in milliseconds.
 */
Timer::Epoch Timer::GetCurrentEpoch()
{
    struct timespec tv;
    clock_gettime(NL_SYSTEM_TIMER_CLOCK_ID, &tv);

    return (static_cast<Timer::Epoch>(tv.tv_sec) * kTimerFactor_milli_per_unit) +
        (static_cast<Timer::Epoch>(tv.tv_nsec) / kTimerFactor_nano_per_milli);
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS
Timer::Epoch Timer::GetCurrentEpoch()
{
    static volatile Timer::Epoch overflow = 0;
    static volatile u32_t lastSample = 0;
    static volatile uint8_t lock = 0;
    static const Timer::Epoch kOverflowIncrement = static_cast<Timer::Epoch>(0x100000000);

    Timer::Epoch overflowSample;
    u32_t sample;

    // Tracking timer wrap assumes that this function gets called with
    // a period that is less than 1/2 the timer range.
    if (__sync_bool_compare_and_swap(&lock, 0, 1))
    {
        sample = sys_now();

        if (lastSample > sample)
        {
            overflow += kOverflowIncrement;
        }

        lastSample = sample;
        overflowSample = overflow;

        __sync_bool_compare_and_swap(&lock, 1, 0);
    }
    else
    {
        // a lower priority task is in the block above. Depending where that
        // lower task is blocked can spell trouble in a timer wrap condition.
        // the question here is what this task should use as an overflow value.
        // To fix this race requires a platform api that can be used to
        // protect critical sections.
        overflowSample = overflow;
        sample = sys_now();
    }

    return static_cast<Timer::Epoch>(overflowSample | static_cast<Timer::Epoch>(sample));
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP && !WEAVE_SYSTEM_CONFIG_USE_SOCKETS

/**
 *  Compares two Timer::Epoch values and returns true if the first value is earlier than the second value.
 *
 *  @brief
 *      A static API that gets called to compare 2 time values.  This API attempts to account for timer wrap by assuming that the
 *      difference between the 2 input values will only be more than half the Epoch scalar range if a timer wrap has occurred
 *      between the 2 samples.
 *
 *  @note
 *      This implementation assumes that Timer::Epoch is an unsigned scalar type.
 *
 *  @return true if the first param is earlier than the second, false otherwise.
 */
bool Timer::IsEarlierEpoch(const Timer::Epoch &inFirst, const Timer::Epoch &inSecond)
{
    static const Epoch kMaxTime_2 = static_cast<Epoch>((static_cast<Epoch>(0) - static_cast<Epoch>(1)) / 2);

    // account for timer wrap with the assumption that no two input times will "naturally"
    // be more than half the timer range apart.
    return (((inFirst < inSecond) && (inSecond - inFirst < kMaxTime_2)) ||
            ((inFirst > inSecond) && (inFirst - inSecond > kMaxTime_2)));
}

/**
 *  This method registers an one-shot timer with the underlying timer mechanism provided by the platform.
 *
 *  @param[in]  aDelayMilliseconds  The number of milliseconds before this timer fires
 *  @param[in]  onComplete          A pointer to the callback function when this timer fires
 *  @param[in]  appState            An arbitrary pointer to be passed into onComplete when this timer fires
 *
 *  @retval #WEAVE_SYSTEM_NO_ERROR Unconditionally.
 *
 */
Error Timer::Start(uint32_t aDelayMilliseconds, OnCompleteFunct aOnComplete, void* aAppState)
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    Layer& lLayer = this->SystemLayer();
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    WEAVE_SYSTEM_FAULT_INJECT(FaultInjection::kFault_TimeoutImmediate, aDelayMilliseconds = 0);

    this->AppState = aAppState;
    this->mAwakenEpoch = Timer::GetCurrentEpoch() + static_cast<Epoch>(aDelayMilliseconds);
    if (!__sync_bool_compare_and_swap(&this->OnComplete, NULL, aOnComplete))
    {
        WeaveDie();
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    // add to the sorted list of timers. Earliest timer appears first.
    if (lLayer.mTimerList == NULL ||
        this->IsEarlierEpoch(this->mAwakenEpoch, lLayer.mTimerList->mAwakenEpoch))
    {
        this->mNextTimer = lLayer.mTimerList;
        lLayer.mTimerList = this;

        // this is the new eariest timer and so the timer needs (re-)starting provided that
        // the system is not currently processing expired timers, in which case it is left to
        // HandleExpiredTimers() to re-start the timer.
        if (!lLayer.mTimerComplete)
        {
            lLayer.StartPlatformTimer(aDelayMilliseconds);
        }
    }
    else
    {
        Timer* lTimer = lLayer.mTimerList;

        while (lTimer->mNextTimer)
        {
            if (this->IsEarlierEpoch(this->mAwakenEpoch, lTimer->mNextTimer->mAwakenEpoch))
            {
                // found the insert location.
                break;
            }

            lTimer = lTimer->mNextTimer;
        }

        this->mNextTimer = lTimer->mNextTimer;
        lTimer->mNextTimer = this;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    return WEAVE_SYSTEM_NO_ERROR;
}

Error Timer::ScheduleWork(OnCompleteFunct aOnComplete, void* aAppState)
{
    Error err = WEAVE_SYSTEM_NO_ERROR;
    Layer& lLayer = this->SystemLayer();

    this->AppState = aAppState;
    this->mAwakenEpoch = Timer::GetCurrentEpoch();
    if (!__sync_bool_compare_and_swap(&this->OnComplete, NULL, aOnComplete))
    {
        WeaveDie();
    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = lLayer.PostEvent(*this, Weave::System::kEvent_ScheduleWork, 0);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    lLayer.WakeSelect();
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    return err;
}

/**
 *  This method de-initializes the timer object, and prevents this timer from firing if it hasn't done so.
 *
 *  @retval #WEAVE_SYSTEM_NO_ERROR Unconditionally.
 */
Error Timer::Cancel()
{
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    Layer& lLayer = this->SystemLayer();
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP
    OnCompleteFunct lOnComplete = this->OnComplete;

    // Check if the timer is armed
    VerifyOrExit(lOnComplete != NULL, );
    // Atomically disarm if the value has not changed
    VerifyOrExit(__sync_bool_compare_and_swap(&this->OnComplete, lOnComplete, NULL), );

    // Since this thread changed the state of OnComplete, release the timer.
    this->AppState = NULL;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    if (lLayer.mTimerList)
    {
        if (this == lLayer.mTimerList)
        {
            lLayer.mTimerList = this->mNextTimer;
        }
        else
        {
            Timer* lTimer = lLayer.mTimerList;

            while (lTimer->mNextTimer)
            {
                if (this == lTimer->mNextTimer)
                {
                    lTimer->mNextTimer = this->mNextTimer;
                    break;
                }

                lTimer = lTimer->mNextTimer;
            }
        }

        this->mNextTimer = NULL;
    }
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    this->Release();
exit:
    return WEAVE_SYSTEM_NO_ERROR;
}

/**
 *  This method is called by the underlying timer mechanism provided by the platform when the timer fires.
 */
void Timer::HandleComplete()
{
    // Save information needed to perform the callback.
    Layer& lLayer = this->SystemLayer();
    const OnCompleteFunct lOnComplete = this->OnComplete;
    void* lAppState = this->AppState;

    // Check if timer is armed
    VerifyOrExit(lOnComplete != NULL, );
    // Atomically disarm if the value has not changed.
    VerifyOrExit(__sync_bool_compare_and_swap(&this->OnComplete, lOnComplete, NULL), );

    // Since this thread changed the state of OnComplete, release the timer.
    AppState = NULL;
    this->Release();

    // Invoke the app's callback, if it's still valid.
    if (lOnComplete != NULL)
        lOnComplete(&lLayer, lAppState, WEAVE_SYSTEM_NO_ERROR);

exit:
    return;
}

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
/**
 * Completes any timers that have expired.
 *
 *  @brief
 *      A static API that gets called when the platform timer expires. Any expired timers are completed and removed from the list
 *      of active timers in the layer object. If unexpired timers remain on completion, StartPlatformTimer will be called to
 *      restart the platform timer.
 *
 *  @note
 *      It's harmless if this API gets called and there are no expired timers.
 *
 *  @return WEAVE_SYSTEM_NO_ERROR on success, error code otherwise.
 *
 */
Error Timer::HandleExpiredTimers(Layer& aLayer)
{
    // expire each timer in turn until an unexpired timer is reached or the timerlist is emptied.
    while (aLayer.mTimerList)
    {
        const Epoch kCurrentEpoch = Timer::GetCurrentEpoch();

        // The platform timer API has MSEC resolution so expire any timer with less than 1 msec remaining.
        if (Timer::IsEarlierEpoch(aLayer.mTimerList->mAwakenEpoch, kCurrentEpoch + 1))
        {
            Timer& lTimer = *aLayer.mTimerList;
            aLayer.mTimerList = lTimer.mNextTimer;
            lTimer.mNextTimer = NULL;

            aLayer.mTimerComplete = true;
            lTimer.HandleComplete();
            aLayer.mTimerComplete = false;
        }
        else
        {
            // timers still exist so restart the platform timer.
            const uint64_t kDelayMilliseconds = aLayer.mTimerList->mAwakenEpoch - kCurrentEpoch;

            /*
             * Original kDelayMilliseconds was a 32 bit value.  The only way in which this could overflow is if time went backwards
             * (e.g. as a result of a time adjustment from time synchronization).  Verify that the timer can still be executed
             * (even if it is very late) and exit if that is the case.  Note: if the time sync ever ends up adjusting the clock, we
             * should implement a method that deals with all the timers in the system.
             */
            VerifyOrDie(kDelayMilliseconds <= UINT32_MAX);

            aLayer.StartPlatformTimer(static_cast<uint32_t>(kDelayMilliseconds));
            break; // all remaining timers are still ticking.
        }
    }

    return WEAVE_SYSTEM_NO_ERROR;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

} // namespace System
} // namespace Weave
} // namespace nl

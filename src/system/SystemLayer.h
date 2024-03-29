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
 *      This file contains declarations of the
 *      nl::Weave::System::Layer class and its related types, data and
 *      functions.
 */

#ifndef SYSTEMLAYER_H
#define SYSTEMLAYER_H

// Include configuration headers
#include <SystemLayer/SystemConfig.h>

// Include dependent headers
#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
#include <poll.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
#include <pthread.h>
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING

#include <Weave/Support/NLDLLUtil.h>

#include <SystemLayer/SystemError.h>
#include <SystemLayer/SystemObject.h>
#include <SystemLayer/SystemEvent.h>

#if WEAVE_SYSTEM_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES

namespace nl {
namespace Inet {

class InetLayer;

} // namespace Inet
} // namespace nl

#endif // WEAVE_SYSTEM_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES

namespace nl {
namespace Weave {
namespace System {

class Layer;
class Timer;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
class Object;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

namespace Platform {
namespace Layer {

using ::nl::Weave::System::Error;
using ::nl::Weave::System::Layer;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
using ::nl::Weave::System::Object;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

extern Error WillInit(Layer& aLayer, void* aContext);
extern Error WillShutdown(Layer& aLayer, void* aContext);

extern void DidInit(Layer& aLayer, void* aContext, Error aStatus);
extern void DidShutdown(Layer& aLayer, void* aContext, Error aStatus);

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
extern Error PostEvent(Layer& aLayer, void* aContext, Object& aTarget, EventType aType, uintptr_t aArgument);
extern Error DispatchEvents(Layer& aLayer, void* aContext);
extern Error DispatchEvent(Layer& aLayer, void* aContext, Event aEvent);
extern Error StartTimer(Layer& aLayer, void* aContext, uint32_t aMilliseconds);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

} // namespace Layer
} // namespace Platform

/**
 *  @enum LayerState
 *
 *  The state of a Layer object.
 */
enum LayerState
{
    kLayerState_NotInitialized = 0,  /**< Not initialized state. */
    kLayerState_Initialized = 1      /**< Initialized state. */
};

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
typedef Error (*LwIPEventHandlerFunction)(Object& aTarget, EventType aEventType, uintptr_t aArgument);

class LwIPEventHandlerDelegate
{
    friend class Layer;

public:
    bool IsInitialized(void) const;
    void Init(LwIPEventHandlerFunction aFunction);
    void Prepend(const LwIPEventHandlerDelegate*& aDelegateList);

private:
    LwIPEventHandlerFunction mFunction;
    const LwIPEventHandlerDelegate* mNextDelegate;
};
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

/**
 *  @class Layer
 *
 *  @brief
 *      This provides access to timers according to the configured event handling model.
 *
 *      For \c WEAVE_SYSTEM_CONFIG_USE_SOCKETS, event readiness notification is handled via traditional poll/select implementation on
 *      the platform adaptation.
 *
 *      For \c WEAVE_SYSTEM_CONFIG_USE_LWIP, event readiness notification is handle via events / messages and platform- and
 *      system-specific hooks for the event/message system.
 */
class NL_DLL_EXPORT Layer
{
public:
    Layer(void);

    Error Init(void* aContext);
    Error Shutdown(void);

    void *GetPlatformData(void) const;
    void SetPlatformData(void *aPlatformData);

    LayerState State(void) const;

    Error NewTimer(Timer*& aTimerPtr);

    typedef void (*TimerCompleteFunct)(Layer* aLayer, void* aAppState, Error aError);
    Error StartTimer(uint32_t aMilliseconds, TimerCompleteFunct aComplete, void* aAppState);
    void CancelTimer(TimerCompleteFunct aOnComplete, void* aAppState);

    Error ScheduleWork(TimerCompleteFunct aComplete, void* aAppState);

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    void PrepareSelect(struct pollfd * pollFDs, int& numPollFDs, int& timeoutMS);
    void HandleSelectResult(const struct pollfd * pollFDs, int numPollFDs);
    void WakeSelect(void);
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    typedef Error (*EventHandler)(Object& aTarget, EventType aEventType, uintptr_t aArgument);
    Error AddEventHandlerDelegate(LwIPEventHandlerDelegate& aDelegate);

    // Event Handling
    Error PostEvent(Object& aTarget, EventType aEventType, uintptr_t aArgument);
    Error DispatchEvents(void);
    Error DispatchEvent(Event aEvent);
    Error HandleEvent(Object& aTarget, EventType aEventType, uintptr_t aArgument);

    // Timer Management
    Error HandlePlatformTimer(void);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    static uint64_t GetClock_Monotonic(void);
    static uint64_t GetClock_MonotonicMS(void);
    static uint64_t GetClock_MonotonicHiRes(void);
    static Error GetClock_RealTime(uint64_t & curTime);
    static Error GetClock_RealTimeMS(uint64_t & curTimeMS);
    static Error SetClock_RealTime(uint64_t newCurTime);

private:
    LayerState mLayerState;
    void* mContext;
    void* mPlatformData;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    static LwIPEventHandlerDelegate sSystemEventHandlerDelegate;

    const LwIPEventHandlerDelegate* mEventDelegateList;
    Timer* mTimerList;
    bool mTimerComplete;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    int mWakePipeIn;
    int mWakePipeOut;

#if WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
    pthread_t mHandleSelectThread;
#endif // WEAVE_SYSTEM_CONFIG_POSIX_LOCKING
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    static Error HandleSystemLayerEvent(Object& aTarget, EventType aEventType, uintptr_t aArgument);

    Error StartPlatformTimer(uint32_t aDelayMilliseconds);

    friend Error Platform::Layer::PostEvent(Layer& aLayer, void* aContext, Object& aTarget, EventType aType, uintptr_t aArgument);
    friend Error Platform::Layer::DispatchEvents(Layer& aLayer, void* aContext);
    friend Error Platform::Layer::DispatchEvent(Layer& aLayer, void* aContext, Event aEvent);
    friend Error Platform::Layer::StartTimer(Layer& aLayer, void* aContext, uint32_t aMilliseconds);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Copy and assignment NOT DEFINED
    Layer(const Layer&);
    Layer& operator =(const Layer&);

    friend class Timer;

#if WEAVE_SYSTEM_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
    friend class Inet::InetLayer;
    void CancelAllMatchingInetTimers(Inet::InetLayer& aInetLayer, void* aOnCompleteInetLayer, void* aAppState);
#endif // WEAVE_SYSTEM_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
};

/**
 * This returns the current state of the layer object.
 */
inline LayerState Layer::State(void) const
{
    return this->mLayerState;
}

} // namespace System
} // namespace Weave
} // namespace nl

#endif // defined(SYSTEMLAYER_H)

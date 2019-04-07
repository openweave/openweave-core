/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Defines the public interface for the Device Layer ThreadStackManager object.
 */

#ifndef THREAD_STACK_MANAGER_H
#define THREAD_STACK_MANAGER_H

namespace nl {
namespace Weave {
namespace DeviceLayer {

class ThreadStackManagerImpl;

/**
 * Provides features for initializing and interacting with the Thread stack on
 * a Weave-enabled device.
 */
class ThreadStackManager
{
    using ImplClass = ::nl::Weave::DeviceLayer::ThreadStackManagerImpl;

public:

    // ===== Members that define the public interface of the ThreadStackManager

    WEAVE_ERROR InitThreadStack(void);
    void ProcessThreadActivity(void);
    WEAVE_ERROR StartThreadTask(void);
    void LockThreadStack(void);
    bool TryLockThreadStack(void);
    void UnlockThreadStack(void);
    bool HaveRouteToAddress(const IPAddress & destAddr);

private:

    // ===== Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManagerImpl;
    template<class> friend class ::nl::Weave::DeviceLayer::Internal::GenericPlatformManagerImpl;
    template<class> friend class ::nl::Weave::DeviceLayer::Internal::GenericPlatformManagerImpl_FreeRTOS;

    void OnPlatformEvent(const WeaveDeviceEvent * event);
};

/**
 * Returns the public interface of the ThreadStackManager singleton object.
 *
 * Weave applications should use this to access features of the ThreadStackManager object
 * that are common to all platforms.
 */
extern ThreadStackManager & ThreadStackMgr(void);

/**
 * Returns the platform-specific implementation of the ThreadStackManager singleton object.
 *
 * Weave applications can use this to gain access to features of the ThreadStackManager
 * that are specific to the selected platform.
 */
extern ThreadStackManagerImpl & ThreadStackMgrImpl(void);

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/* Include a header file containing the implementation of the ThreadStackManager
 * object for the selected platform.
 */
#ifdef EXTERNAL_THREADSTACKMANAGERIMPL_HEADER
#include EXTERNAL_THREADSTACKMANAGERIMPL_HEADER
#else
#define THREADSTACKMANAGERIMPL_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/ThreadStackManagerImpl.h>
#include THREADSTACKMANAGERIMPL_HEADER
#endif

namespace nl {
namespace Weave {
namespace DeviceLayer {

inline WEAVE_ERROR ThreadStackManager::InitThreadStack()
{
    return static_cast<ImplClass*>(this)->_InitThreadStack();
}

inline void ThreadStackManager::ProcessThreadActivity()
{
    static_cast<ImplClass*>(this)->_ProcessThreadActivity();
}

inline WEAVE_ERROR ThreadStackManager::StartThreadTask()
{
    return static_cast<ImplClass*>(this)->_StartThreadTask();
}

inline void ThreadStackManager::LockThreadStack()
{
    static_cast<ImplClass*>(this)->_LockThreadStack();
}

inline bool ThreadStackManager::TryLockThreadStack()
{
    return static_cast<ImplClass*>(this)->_TryLockThreadStack();
}

inline void ThreadStackManager::UnlockThreadStack()
{
    static_cast<ImplClass*>(this)->_UnlockThreadStack();
}

/**
 * Determines whether a route exists via the Thread interface to the specified destination address.
 */
inline bool ThreadStackManager::HaveRouteToAddress(const IPAddress & destAddr)
{
    return static_cast<ImplClass*>(this)->_HaveRouteToAddress(destAddr);
}

inline void ThreadStackManager::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    static_cast<ImplClass*>(this)->_OnPlatformEvent(event);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // THREAD_STACK_MANAGER_H

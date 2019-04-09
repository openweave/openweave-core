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

class PlatformManagerImpl;
class ThreadStackManagerImpl;

namespace Internal {
class NetworkInfo;
template<class> class GenericPlatformManagerImpl;
template<class> class GenericPlatformManagerImpl_FreeRTOS;
template<class> class GenericConnectivityManagerImpl_Thread;
template<class> class GenericThreadStackManagerImpl_OpenThread;
template<class> class GenericThreadStackManagerImpl_OpenThread_LwIP;
template<class> class GenericThreadStackManagerImpl_FreeRTOS;
template<class> class GenericNetworkProvisioningServerImpl;
} // namespace Internal

/**
 * Provides features for initializing and interacting with the Thread stack on
 * a Weave-enabled device.
 */
class ThreadStackManager
{
    using ImplClass = ThreadStackManagerImpl;

public:

    // ===== Members that define the public interface of the ThreadStackManager

    WEAVE_ERROR InitThreadStack(void);
    void ProcessThreadActivity(void);
    WEAVE_ERROR StartThreadTask(void);
    void LockThreadStack(void);
    bool TryLockThreadStack(void);
    void UnlockThreadStack(void);
    bool HaveRouteToAddress(const IPAddress & destAddr);

protected:

    // ===== Members available to the implementation subclass.

    ThreadStackManager() = default;

private:

    // ===== Members for internal use by the following friends.

    friend class PlatformManagerImpl;
    template<class> friend class Internal::GenericPlatformManagerImpl;
    template<class> friend class Internal::GenericPlatformManagerImpl_FreeRTOS;
    template<class> friend class Internal::GenericConnectivityManagerImpl_Thread;
    template<class> friend class Internal::GenericThreadStackManagerImpl_OpenThread;
    template<class> friend class Internal::GenericThreadStackManagerImpl_OpenThread_LwIP;
    template<class> friend class Internal::GenericThreadStackManagerImpl_FreeRTOS;
    template<class> friend class Internal::GenericNetworkProvisioningServerImpl;

    void OnPlatformEvent(const WeaveDeviceEvent * event);
    bool IsThreadEnabled(void);
    WEAVE_ERROR SetThreadEnabled(bool val);
    bool IsThreadProvisioned(void);
    bool IsThreadAttached(void);
    WEAVE_ERROR GetThreadProvision(Internal::NetworkInfo & netInfo, bool includeCredentials);
    WEAVE_ERROR SetThreadProvision(const Internal::NetworkInfo & netInfo);
    void ClearThreadProvision(void);
    bool HaveMeshConnectivity(void);
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

inline bool ThreadStackManager::IsThreadEnabled(void)
{
    return static_cast<ImplClass*>(this)->_IsThreadEnabled();
}

inline WEAVE_ERROR ThreadStackManager::SetThreadEnabled(bool val)
{
    return static_cast<ImplClass*>(this)->_SetThreadEnabled(val);
}

inline bool ThreadStackManager::IsThreadProvisioned(void)
{
    return static_cast<ImplClass*>(this)->_IsThreadProvisioned();
}

inline bool ThreadStackManager::IsThreadAttached(void)
{
    return static_cast<ImplClass*>(this)->_IsThreadAttached();
}

inline WEAVE_ERROR ThreadStackManager::GetThreadProvision(Internal::NetworkInfo & netInfo, bool includeCredentials)
{
    return static_cast<ImplClass*>(this)->_GetThreadProvision(netInfo, includeCredentials);
}

inline WEAVE_ERROR ThreadStackManager::SetThreadProvision(const Internal::NetworkInfo & netInfo)
{
    return static_cast<ImplClass*>(this)->_SetThreadProvision(netInfo);
}

inline void ThreadStackManager::ClearThreadProvision(void)
{
    static_cast<ImplClass*>(this)->_ClearThreadProvision();
}

inline bool ThreadStackManager::HaveMeshConnectivity(void)
{
    return static_cast<ImplClass*>(this)->_HaveMeshConnectivity();
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // THREAD_STACK_MANAGER_H

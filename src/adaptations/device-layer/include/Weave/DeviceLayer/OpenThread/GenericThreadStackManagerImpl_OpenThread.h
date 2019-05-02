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
 *          Provides a generic implementation of ThreadStackManager features
 *          for use on platforms that use OpenThread.
 */


#ifndef GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_H
#define GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_H

#include <openthread/instance.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

class ThreadStackManagerImpl;

namespace Internal {

class DeviceNetworkInfo;

/**
 * Provides a generic implementation of ThreadStackManager features that works in conjunction
 * with OpenThread.
 *
 * This class contains implementations of select features from the ThreadStackManager abstract
 * interface that are suitable for use on devices that employ OpenThread.  It is intended to
 * be inherited, directly or indirectly, by the ThreadStackManagerImpl class, which also appears
 * as the template's ImplClass parameter.
 *
 * The class is designed to be independent of the choice of host OS (e.g. RTOS or posix) and
 * network stack (e.g. LwIP or other IP stack).
 */
template<class ImplClass>
class GenericThreadStackManagerImpl_OpenThread
{
public:
    // ===== Platform-specific methods directly callable by the application.

    otInstance * OTInstance() const;
    static void OnOpenThreadStateChange(uint32_t flags, void * context);

protected:

    // ===== Methods that implement the ThreadStackManager abstract interface.

    void _ProcessThreadActivity(void);
    bool _HaveRouteToAddress(const IPAddress & destAddr);
    void _OnPlatformEvent(const WeaveDeviceEvent * event);
    bool _IsThreadEnabled(void);
    WEAVE_ERROR _SetThreadEnabled(bool val);
    bool _IsThreadProvisioned(void);
    bool _IsThreadAttached(void);
    WEAVE_ERROR _GetThreadProvision(DeviceNetworkInfo & netInfo, bool includeCredentials);
    WEAVE_ERROR _SetThreadProvision(const DeviceNetworkInfo & netInfo);
    void _ClearThreadProvision(void);
    bool _HaveMeshConnectivity(void);
    WEAVE_ERROR _GetAndLogThreadStatsCounters(void);
    WEAVE_ERROR _GetAndLogThreadTopologyMinimal(void);
    WEAVE_ERROR _GetAndLogThreadTopologyFull(void);

    // ===== Members available to the implementation subclass.

    WEAVE_ERROR DoInit(otInstance * otInst);
    bool IsThreadAttachedNoLock(void);

private:

    // ===== Private members for use by this class only.

    otInstance * mOTInst;

    inline ImplClass * Impl() { return static_cast<ImplClass*>(this); }
};

// Instruct the compiler to instantiate the template only when explicitly told to do so.
extern template class GenericThreadStackManagerImpl_OpenThread<ThreadStackManagerImpl>;

/**
 * Returns the underlying OpenThread instance object.
 */
template<class ImplClass>
inline otInstance * GenericThreadStackManagerImpl_OpenThread<ImplClass>::OTInstance() const
{
    return mOTInst;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_H

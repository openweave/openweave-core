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
#include <openthread/platform/openthread-system.h>


namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Provides a generic implementation of ThreadStackManager features that works in conjunction
 * with OpenThread.
 *
 * This template contains implementations of select features from the ThreadStackManager abstract
 * interface that are suitable for use on devices that employ OpenThread.  It is intended to
 * be inherited, directly or indirectly, by the ThreadStackManagerImpl class, which also appears
 * as the template's ImplClass parameter.
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

    // ===== Members available to the implementation subclass.

    WEAVE_ERROR Init(otInstance * otInst);

private:

    // ===== Private members for use by this class only.

    otInstance * mOTInst;

    inline ImplClass * Impl() { return static_cast<ImplClass*>(this); }
};

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

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
 *          Provides an generic implementation of ConnectivityManager features
 *          for use on platforms that do NOT support Thread.
 */

#ifndef GENERIC_CONNECTIVITY_MANAGER_IMPL_NO_THREAD_H
#define GENERIC_CONNECTIVITY_MANAGER_IMPL_NO_THREAD_H

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Provides a generic implementation of WiFi-specific ConnectivityManager features for
 * use on platforms that do NOT support Thread.
 *
 * This class is intended to be inherited (directly or indirectly) by the ConnectivityManagerImpl
 * class, which also appears as the template's ImplClass parameter.
 *
 */
template<class ImplClass>
class GenericConnectivityManagerImpl_NoThread
{
protected:

    // ===== Methods that implement the ConnectivityManager abstract interface.

    bool _HaveServiceConnectivityViaThread(void);

    ImplClass * Impl() { return static_cast<ImplClass *>(this); }
};

template<class ImplClass>
inline void GenericConnectivityManagerImpl_Thread::_Init(void)
{
    /* nothing to do */
}

template<class ImplClass>
inline void GenericConnectivityManagerImpl_Thread::_OnPlatformEvent(const WeaveDeviceEvent *)
{
    /* nothing to do */
}

template<class ImplClass>
inline bool _HaveServiceConnectivityViaThread(void)
{
    return false;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // GENERIC_CONNECTIVITY_MANAGER_IMPL_NO_THREAD_H

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
 *          Contains non-inline method definitions for the
 *          GenericThreadStackManagerImpl_OpenThread<> template.
 */

#ifndef GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP
#define GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>

#include <openthread/thread.h>
#include <openthread/tasklet.h>
#include <openthread/link.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// Fully instantiate the generic implementation class in whatever compilation unit includes this file.
template class GenericThreadStackManagerImpl_OpenThread<ThreadStackManagerImpl>;

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::DoInit(otInstance * otInst)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;

    // If an OpenThread instance hasn't been supplied, call otInstanceInitSingle() to
    // create or acquire a singleton instance of OpenThread.
    if (otInst == NULL)
    {
        otInst = otInstanceInitSingle();
        VerifyOrExit(otInst != NULL, err = MapOpenThreadError(OT_ERROR_FAILED));
    }

    mOTInst = otInst;

    // Arrange for OpenThread to call the OnOpenThreadStateChange method whenever a
    // state change occurs.  Note that we reference the OnOpenThreadStateChange method
    // on the concrete implementation class so that that class can override the default
    // method implementation if it chooses to.
    otErr = otSetStateChangedCallback(otInst, ImplClass::OnOpenThreadStateChange, NULL);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    // TODO: generalize this
    {
        otLinkModeConfig linkMode;

        memset(&linkMode, 0, sizeof(linkMode));
        linkMode.mRxOnWhenIdle       = true;
        linkMode.mSecureDataRequests = true;
        linkMode.mDeviceType         = true;
        linkMode.mNetworkData        = true;

        otErr = otThreadSetLinkMode(otInst, linkMode);
        VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));
    }

    // TODO: not supported in old version of OpenThread used by Nordic SDK.
    // otIp6SetSlaacEnabled(otInst, false);

    otErr = otIp6SetEnabled(otInst, true);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    if (otDatasetIsCommissioned(otInst))
    {
        otErr = otThreadSetEnabled(otInst, true);
        VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));
    }

exit:
    return err;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    if (event->Type == DeviceEventType::kOpenThreadStateChange)
    {
#if WEAVE_DETAIL_LOGGING

        Impl()->LockThreadStack();

        LogOpenThreadStateChange(mOTInst, event->OpenThreadStateChange.flags);

        Impl()->UnlockThreadStack();

#endif // WEAVE_DETAIL_LOGGING
    }
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_ProcessThreadActivity(void)
{
    otTaskletsProcess(mOTInst);
    otSysProcessDrivers(mOTInst);
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_HaveRouteToAddress(const IPAddress & destAddr)
{
    bool res = false;

    // Lock OpenThread
    Impl()->LockThreadStack();

    // No routing of IPv4 over Thread.
    VerifyOrExit(!destAddr.IsIPv4(), res = false);

    // If the device is attached to a Thread network...
    if (IsAttached())
    {
        // Link-local addresses are always presumed to be routable, provided the device is attached.
        if (destAddr.IsIPv6LinkLocal())
        {
            ExitNow(res = true);
        }

        // Iterate over the routes known to the OpenThread stack looking for a route that covers the
        // destination address.  If found, consider the address routable.
        // Ignore any routes advertised by this device.
        // If the destination address is a ULA, ignore default routes. Border routers advertising
        // default routes are not expected to be capable of routing Weave fabric ULAs unless they
        // advertise those routes specifically.
        {
            otError otErr;
            otNetworkDataIterator routeIter = OT_NETWORK_DATA_ITERATOR_INIT;
            otExternalRouteConfig routeConfig;
            const bool destIsULA = destAddr.IsIPv6ULA();

            while ((otErr = otNetDataGetNextRoute(Impl()->OTInstance(), &routeIter, &routeConfig)) == OT_ERROR_NONE)
            {
                const IPPrefix prefix = ToIPPrefix(routeConfig.mPrefix);
                char addrStr[64];
                prefix.IPAddr.ToString(addrStr, sizeof(addrStr));
                WeaveLogDetail(DeviceLayer, "Prefix: %s/%d", addrStr, (int)prefix.Length);
                if (!routeConfig.mNextHopIsThisDevice &&
                    (!destIsULA || routeConfig.mPrefix.mLength > 0) &&
                    ToIPPrefix(routeConfig.mPrefix).MatchAddress(destAddr))
                {
                    ExitNow(res = true);
                }
            }
        }
    }

exit:

    // Unlock OpenThread
    Impl()->UnlockThreadStack();

    return res;
}

/**
 * Determine whether the device is attached to a Thread network based on the current OpenThread device role.
 *
 * NB: This method *must* be called with the OpenThread lock held.
 */
template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::IsAttached()
{
    otDeviceRole curRole = otThreadGetDeviceRole(mOTInst);
    return (curRole != OT_DEVICE_ROLE_DISABLED && curRole != OT_DEVICE_ROLE_DETACHED);
}

/**
 * Called by OpenThread to alert the ThreadStackManager of a change in the state of the Thread stack.
 *
 * By default, applications never need to call this method directly.  However, applications that
 * wish to receive OpenThread state change call-backs directly from OpenThread (e.g. by calling
 * otSetStateChangedCallback() with their own callback function) can call this method to pass
 * state change events to the ThreadStackManager.
 */
template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::OnOpenThreadStateChange(uint32_t flags, void * context)
{
    WeaveDeviceEvent event;
    event.Type = DeviceEventType::kOpenThreadStateChange;
    event.OpenThreadStateChange.flags = flags;
    PlatformMgr().PostEvent(&event);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP

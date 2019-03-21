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

int foo;
void StopHere()
{
    foo = 42;
}

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::Init(otInstance * otInst)
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
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_ProcessThreadActivity(void)
{
    otTaskletsProcess(mOTInst);
    otSysProcessDrivers(mOTInst);
    StopHere();
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::OnOpenThreadStateChange(uint32_t flags, void * context)
{
#if 1 // TODO: remove me
    WeaveDeviceEvent event;
    event.Type = DeviceEventType::kOpenThreadStateChange;
    event.OpenThreadStateChange.flags = flags;
#else
    WeaveDeviceEvent event;
    event.Type = DeviceEventType::kNoOp;
#endif

    PlatformMgr().PostEvent(&event);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP

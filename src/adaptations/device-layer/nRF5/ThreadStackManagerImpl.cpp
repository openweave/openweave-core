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
 *          Provides an implementation of the ThreadStackManager object for
 *          nRF52 platforms using the Nordic nRF5 SDK and the OpenThread
 *          stack.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/GenericThreadStackManagerImpl_OpenThread.ipp>
#include <Weave/DeviceLayer/FreeRTOS/GenericThreadStackManagerImpl_FreeRTOS.ipp>
#include <Weave/DeviceLayer/LwIP/GenericThreadStackManagerImpl_LwIP.ipp>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>


#include "boards.h"
#include "nrf_log.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {

using namespace ::nl::Weave::DeviceLayer::Internal;

// Fully instantiate the template classes on which the nRF52 PlatformManager depends.
template class Internal::GenericThreadStackManagerImpl_OpenThread<ThreadStackManagerImpl>;
template class Internal::GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl>;
template class Internal::GenericThreadStackManagerImpl_LwIP<ThreadStackManagerImpl>;


ThreadStackManagerImpl ThreadStackManagerImpl::sInstance;

WEAVE_ERROR ThreadStackManagerImpl::_InitThreadStack(void)
{
    return InitThreadStack(NULL);
}

WEAVE_ERROR ThreadStackManagerImpl::InitThreadStack(otInstance * otInst)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl>::Init();
    SuccessOrExit(err);

    err = GenericThreadStackManagerImpl_OpenThread<ThreadStackManagerImpl>::Init(otInst);
    SuccessOrExit(err);

    err = GenericThreadStackManagerImpl_LwIP<ThreadStackManagerImpl>::InitThreadNetIf();
    SuccessOrExit(err);

exit:
    return err;
}

void ThreadStackManagerImpl::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (event->Type == DeviceEventType::kOpenThreadStateChange)
    {
#if WEAVE_DETAIL_LOGGING

        LockThreadStack();

        otInstance * otInst = OTInstance();

        WeaveLogDetail(DeviceLayer, "OpenThread State Changed (Flags: 0x%08x)", event->OpenThreadStateChange.flags);
        if ((event->OpenThreadStateChange.flags & OT_CHANGED_THREAD_ROLE) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Device Role: %s", OpenThreadRoleToStr(otThreadGetDeviceRole(otInst)));
        }
        if ((event->OpenThreadStateChange.flags & OT_CHANGED_THREAD_NETWORK_NAME) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Network Name: %s", otThreadGetNetworkName(otInst));
        }
        if ((event->OpenThreadStateChange.flags & OT_CHANGED_THREAD_PANID) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   PAN Id: 0x%04X", otLinkGetPanId(otInst));
        }
        if ((event->OpenThreadStateChange.flags & OT_CHANGED_THREAD_EXT_PANID) != 0)
        {
            const otExtendedPanId * exPanId = otThreadGetExtendedPanId(otInst);
            static char exPanIdStr[32];
            snprintf(exPanIdStr, sizeof(exPanIdStr), "0x%02X%02X%02X%02X%02X%02X%02X%02X",
                     exPanId->m8[0], exPanId->m8[1], exPanId->m8[2], exPanId->m8[3],
                     exPanId->m8[4], exPanId->m8[5], exPanId->m8[6], exPanId->m8[7]);
            WeaveLogDetail(DeviceLayer, "   Extended PAN Id: %s", exPanIdStr);
        }
        if ((event->OpenThreadStateChange.flags & OT_CHANGED_THREAD_CHANNEL) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Channel: %d", otLinkGetChannel(otInst));
        }
        if ((event->OpenThreadStateChange.flags & (OT_CHANGED_IP6_ADDRESS_ADDED|OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Interface Addresses:");
            for (const otNetifAddress * addr = otIp6GetUnicastAddresses(otInst); addr != NULL; addr = addr->mNext)
            {
                static char ipAddrStr[64];
                nl::Inet::IPAddress ipAddr;

                memcpy(ipAddr.Addr, addr->mAddress.mFields.m32, sizeof(ipAddr.Addr));

                ipAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

                WeaveLogDetail(DeviceLayer, "        %s/%d%s%s%s", ipAddrStr,
                                 addr->mPrefixLength,
                                 addr->mValid ? " valid" : "",
                                 addr->mPreferred ? " preferred" : "",
                                 addr->mRloc ? " rloc" : "");
            }
        }

        UnlockThreadStack();

#endif // WEAVE_DETAIL_LOGGING

        // If the Thread device role has changed, or an IPv6 address has been added or removed from
        // the Thread stack, update the state and configuration of the LwIP netif.
        if ((event->OpenThreadStateChange.flags & (OT_CHANGED_THREAD_ROLE|OT_CHANGED_IP6_ADDRESS_ADDED|OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
        {
            err = UpdateThreadNetIfState();
            SuccessOrExit(err);
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceLayer, "ThreadStackManagerImpl::_OnPlatformEvent() failed: %s", ErrorStr(err));
    }

    // TODO: handle error.
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

using namespace ::nl::Weave::DeviceLayer;

/**
 * Glue function called directly by the OpenThread stack when tasklet processing work
 * is pending.
 */
extern "C"
void otTaskletsSignalPending(otInstance * p_instance)
{
    ThreadStackMgrImpl().SignalThreadActivityPending();
}

/**
 * Glue function called directly by the OpenThread stack when system event processing work
 * is pending.
 */
extern "C"
void otSysEventSignalPending(void)
{
    BaseType_t yieldRequired = ThreadStackMgrImpl().SignalThreadActivityPendingFromISR();
    portYIELD_FROM_ISR(yieldRequired);
}


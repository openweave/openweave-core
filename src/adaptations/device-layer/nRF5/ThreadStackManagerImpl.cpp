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
 *          Provides an implementation of the ThreadStackManager object
 *          for nRF52 platforms using the Nordic SDK and the OpenThread
 *          stack.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/GenericThreadStackManagerImpl_OpenThread.ipp>
#include <Weave/DeviceLayer/FreeRTOS/GenericThreadStackManagerImpl_FreeRTOS.ipp>
#include <Weave/DeviceLayer/LwIP/GenericThreadStackManagerImpl_LwIP.ipp>


#include "boards.h"
#include "nrf_log.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {

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
    WEAVE_ERROR err;

    if (event->Type == DeviceEventType::kOpenThreadStateChange)
    {
        LockThreadStack();

        otInstance * otInst = OTInstance();

        WeaveLogProgress(DeviceLayer, "OpenThread State Changed (Flags: 0x%08x)", event->OpenThreadStateChange.flags);
        WeaveLogProgress(DeviceLayer, "   Device Role: %d", otThreadGetDeviceRole(otInst));
        WeaveLogProgress(DeviceLayer, "   Network Name: %s", otThreadGetNetworkName(otInst));
        WeaveLogProgress(DeviceLayer, "   PAN Id: 0x%04X", otLinkGetPanId(otInst));
        {
            const otExtendedPanId * exPanId = otThreadGetExtendedPanId(otInst);
            static char exPanIdStr[32];
            snprintf(exPanIdStr, sizeof(exPanIdStr), "0x%02X%02X%02X%02X%02X%02X%02X%02X",
                     exPanId->m8[0], exPanId->m8[1], exPanId->m8[2], exPanId->m8[3],
                     exPanId->m8[4], exPanId->m8[5], exPanId->m8[6], exPanId->m8[7]);
            WeaveLogProgress(DeviceLayer, "   Extended PAN Id: %s", exPanIdStr);
        }
        WeaveLogProgress(DeviceLayer, "   Channel: %d", otLinkGetChannel(otInst));
        WeaveLogProgress(DeviceLayer, "   Interface Addresses:");
        for (const otNetifAddress * addr = otIp6GetUnicastAddresses(otInst); addr != NULL; addr = addr->mNext)
        {
            static char ipAddrStr[64];
            nl::Inet::IPAddress ipAddr;

            memcpy(ipAddr.Addr, addr->mAddress.mFields.m32, sizeof(ipAddr.Addr));

            ipAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

            WeaveLogProgress(DeviceLayer, "        %s/%d%s%s", ipAddrStr,
                             addr->mPrefixLength,
                             addr->mValid ? " valid" : "",
                             addr->mPreferred ? " preferred" : "");
        }

        UnlockThreadStack();

#if 1 // TODO: remove me
        if ((event->OpenThreadStateChange.flags & (OT_CHANGED_IP6_ADDRESS_ADDED|OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
        {
            err = UpdateNetIfAddresses();
            // TODO: handle error.
        }
#endif
    }
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


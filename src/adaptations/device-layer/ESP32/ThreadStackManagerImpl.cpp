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
 *          ESP32 platforms using the ESP-IDF SDK and the OpenThread
 *          stack.
 *
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD

#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>

#include <Weave/DeviceLayer/FreeRTOS/GenericThreadStackManagerImpl_FreeRTOS.ipp>
#include <Weave/DeviceLayer/OpenThread/GenericThreadStackManagerImpl_OpenThread_LwIP.ipp>

#include <openthread/cli.h>

#include <openthread/openthread-esp32.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

using namespace ::nl::Weave::DeviceLayer::Internal;

namespace Internal {

// Specialize ESP32 ThreadStackManagerImpl's ThreadTaskMain since OpenThread on ESP32 is using a RCP for thread support.
template<>
void GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl>::ThreadTaskMain(void * arg)
{
    GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl> * self =
            static_cast<GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl>*>(arg);

    VerifyOrDie(self->mThreadTask == NULL);

    WeaveLogDetail(DeviceLayer, "Thread task running");

    // Capture the Thread task handle.
    self->mThreadTask = xTaskGetCurrentTaskHandle();

    ThreadStackMgr().LockThreadStack();
    otInstance *instance = ThreadStackMgrImpl().OTInstance();
    ThreadStackMgr().UnlockThreadStack();
    

    while (true)
    {
        otSysMainloopContext mainloop;
 
        otSysMainloopInit(&mainloop);

        ThreadStackMgr().LockThreadStack();
        otTaskletsProcess(instance);
        otSysMainloopUpdate(instance, &mainloop);
        ThreadStackMgr().UnlockThreadStack();
 
        if (otSysMainloopPoll(&mainloop) >= 0)
        {
            ThreadStackMgr().LockThreadStack();
            otSysMainloopProcess(instance, &mainloop);
            ThreadStackMgr().UnlockThreadStack();
        }
        else
        {
            WeaveLogError(DeviceLayer, "OpenThread system polling failed");
            abort();
        }
    }
}

}

ThreadStackManagerImpl ThreadStackManagerImpl::sInstance;

WEAVE_ERROR ThreadStackManagerImpl::_InitThreadStack(void)
{
    return InitThreadStack(NULL);
}

WEAVE_ERROR ThreadStackManagerImpl::InitThreadStack(otInstance *otInst)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Initialize the generic implementation base classes.
    err = GenericThreadStackManagerImpl_FreeRTOS<ThreadStackManagerImpl>::DoInit();
    SuccessOrExit(err);
    err = GenericThreadStackManagerImpl_OpenThread_LwIP<ThreadStackManagerImpl>::DoInit(otInst);
    SuccessOrExit(err);

exit:
    return err;
}

bool ThreadStackManagerImpl::IsInitialized()
{
    return sInstance.mThreadStackLock != NULL;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

using namespace ::nl::Weave::DeviceLayer;

/**
 * Glue function called directly by the OpenThread stack when tasklet processing work
 * is pending.
 */
extern "C" void otTaskletsSignalPending(otInstance *p_instance)
{
    ThreadStackMgrImpl().SignalThreadActivityPending();
}

/**
 * Glue function called directly by the OpenThread stack when system event processing work
 * is pending.
 */
extern "C" void otSysEventSignalPending(void)
{
    BaseType_t yieldRequired = ThreadStackMgrImpl().SignalThreadActivityPendingFromISR();
    if (yieldRequired) portYIELD_FROM_ISR();
}

#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD

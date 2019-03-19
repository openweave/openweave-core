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
 *          GenericThreadStackManagerImpl_FreeRTOS<> template.
 */

#ifndef GENERIC_THREAD_STACK_MANAGER_IMPL_FREERTOS_IPP
#define GENERIC_THREAD_STACK_MANAGER_IMPL_FREERTOS_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/FreeRTOS/GenericThreadStackManagerImpl_FreeRTOS.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::Init(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mThreadStackLock = xSemaphoreCreateMutex();
    if (mThreadStackLock == NULL)
    {
        WeaveLogError(DeviceLayer, "Failed to create Thread stack lock");
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    mThreadTask = NULL;

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::_StartThreadTask(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BaseType_t res;

    VerifyOrExit(mThreadTask == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    res = xTaskCreate(ThreadTaskMain,
                WEAVE_DEVICE_CONFIG_THREAD_TASK_NAME,
                WEAVE_DEVICE_CONFIG_THREAD_TASK_STACK_SIZE / sizeof(StackType_t),
                this,
                WEAVE_DEVICE_CONFIG_THREAD_TASK_PRIORITY,
                NULL);
    VerifyOrExit(res == pdPASS, err = WEAVE_ERROR_NO_MEMORY);

exit:
    return err;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::_LockThreadStack(void)
{
    xSemaphoreTake(mThreadStackLock, portMAX_DELAY);
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::_TryLockThreadStack(void)
{
    return xSemaphoreTake(mThreadStackLock, 0) == pdTRUE;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::_UnlockThreadStack(void)
{
    xSemaphoreGive(mThreadStackLock);
}

template<class ImplClass>
void GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::SignalThreadActivityPending()
{
    if (mThreadTask != NULL)
    {
        xTaskNotifyGive(mThreadTask);
    }
}

template<class ImplClass>
BaseType_t GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::SignalThreadActivityPendingFromISR()
{
    BaseType_t yieldRequired = pdFALSE;

    if (mThreadTask != NULL)
    {
        vTaskNotifyGiveFromISR(mThreadTask, &yieldRequired);
    }

    return yieldRequired;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_FreeRTOS<ImplClass>::ThreadTaskMain(void * arg)
{
    GenericThreadStackManagerImpl_FreeRTOS<ImplClass> * self =
            static_cast<GenericThreadStackManagerImpl_FreeRTOS<ImplClass>*>(arg);

    VerifyOrDie(self->mThreadTask == NULL);

    // Capture the Thread task handle.
    self->mThreadTask = xTaskGetCurrentTaskHandle();

    while (true)
    {
        // Lock the Thread stack.
        self->Impl()->LockThreadStack();

        // Process any pending Thread activity.
        self->Impl()->ProcessThreadActivity();

        // Unlock the Thread stack.
        self->Impl()->UnlockThreadStack();

        // Wait for a signal that more activity is pending.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_FREERTOS_IPP

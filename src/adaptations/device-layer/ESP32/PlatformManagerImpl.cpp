/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides an implementation of the PlatformManager object
 *          for the ESP32 platform.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/PlatformManager.h>
#include <Weave/DeviceLayer/internal/GenericPlatformManagerImpl_FreeRTOS.ipp>

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace Internal {
extern WEAVE_ERROR InitLwIPCoreLock(void);
}

// Fully instantiate the template classes on which the ESP32 PlatformManager depends.
template class Internal::GenericPlatformManagerImpl<PlatformManagerImpl>;
template class Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>;

PlatformManagerImpl PlatformManagerImpl::sInstance;

WEAVE_ERROR PlatformManagerImpl::_InitWeaveStack(void)
{
    WEAVE_ERROR err;

    // Make sure the LwIP core lock has been initialized
    err = Internal::InitLwIPCoreLock();
    SuccessOrExit(err);

    // Call _InitWeaveStack() on the generic implementation base class
    // to finish the initialization process.
    err = Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_InitWeaveStack();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR PlatformManagerImpl::InitLwIPCoreLock(void)
{
    return Internal::InitLwIPCoreLock();
}

esp_err_t PlatformManagerImpl::HandleESPSystemEvent(void * ctx, system_event_t * espEvent)
{
    WeaveDeviceEvent event;
    event.Type = DeviceEventType::kESPSystemEvent;
    event.Platform.ESPSystemEvent = *espEvent;

    sInstance.PostEvent(&event);

    return ESP_OK;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

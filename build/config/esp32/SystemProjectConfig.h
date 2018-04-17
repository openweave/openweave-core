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
 *      System project configuration for the ESP32 platform.
 *
 */

#include <stdint.h>

#ifndef SYSTEMPROJECTCONFIG_H
#define SYSTEMPROJECTCONFIG_H

#include "esp_err.h"

namespace nl {
namespace Weave {
namespace Device {
struct WeaveDeviceEvent;
} // namespace Device
} // namespace Weave
} // namespace nl

// NOTE: The values that are mapped to CONFIG_ #defines are settable via the ESP-IDF Kconfig mechanism.

// ==================== General Configuration ====================

#define WEAVE_SYSTEM_CONFIG_NUM_TIMERS CONFIG_NUM_TIMERS

// ==================== Platform Adaptations ====================

#define WEAVE_SYSTEM_CONFIG_POSIX_LOCKING 0
#define WEAVE_SYSTEM_CONFIG_FREERTOS_LOCKING 0
#define WEAVE_SYSTEM_CONFIG_NO_LOCKING 1
#define WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_EVENT_FUNCTIONS 1
#define WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_TIME 1
#define WEAVE_SYSTEM_CONFIG_LWIP_EVENT_TYPE int
#define WEAVE_SYSTEM_CONFIG_LWIP_EVENT_OBJECT_TYPE const struct ::nl::Weave::Device::WeaveDeviceEvent *

#define WEAVE_SYSTEM_CONFIG_ERROR_TYPE esp_err_t
#define WEAVE_SYSTEM_CONFIG_NO_ERROR ESP_OK
#define WEAVE_SYSTEM_CONFIG_ERROR_MIN 7000000
#define WEAVE_SYSTEM_CONFIG_ERROR_MAX 7000999
#define _WEAVE_SYSTEM_CONFIG_ERROR(e) (WEAVE_SYSTEM_CONFIG_ERROR_MIN + (e))
#define WEAVE_SYSTEM_LWIP_ERROR_MIN 8000000
#define WEAVE_SYSTEM_LWIP_ERROR_MAX 8000999

#endif // SYSTEMPROJECTCONFIG_H

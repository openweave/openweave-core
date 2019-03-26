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
 *      System project configuration for the Nordic nRF5* platform.
 *
 */

#include <stdint.h>

#ifndef SYSTEMPROJECTCONFIG_H
#define SYSTEMPROJECTCONFIG_H

#include "sdk_errors.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {
struct WeaveDeviceEvent;
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

// TODO: move this to Kconfig?
#define CONFIG_NUM_TIMERS 32

// ==================== General Configuration ====================

#define WEAVE_SYSTEM_CONFIG_NUM_TIMERS CONFIG_NUM_TIMERS

// ==================== Platform Adaptations ====================

#define WEAVE_SYSTEM_CONFIG_POSIX_LOCKING 0
#define WEAVE_SYSTEM_CONFIG_FREERTOS_LOCKING 0
#define WEAVE_SYSTEM_CONFIG_NO_LOCKING 1
#define WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_EVENT_FUNCTIONS 1
#define WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_TIME 1
#define WEAVE_SYSTEM_CONFIG_LWIP_EVENT_TYPE int
#define WEAVE_SYSTEM_CONFIG_LWIP_EVENT_OBJECT_TYPE const struct ::nl::Weave::DeviceLayer::WeaveDeviceEvent *

#define WEAVE_SYSTEM_CONFIG_ERROR_TYPE ret_code_t
#define WEAVE_SYSTEM_CONFIG_NO_ERROR NRF_SUCCESS
#define WEAVE_SYSTEM_CONFIG_ERROR_MIN 7000000
#define WEAVE_SYSTEM_CONFIG_ERROR_MAX 7000999
#define _WEAVE_SYSTEM_CONFIG_ERROR(e) (WEAVE_SYSTEM_CONFIG_ERROR_MIN + (e))
#define WEAVE_SYSTEM_LWIP_ERROR_MIN 8000000
#define WEAVE_SYSTEM_LWIP_ERROR_MAX 8000999

#endif // SYSTEMPROJECTCONFIG_H

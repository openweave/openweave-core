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

#ifndef WEAVE_DEVICE_INTERNAL_H
#define WEAVE_DEVICE_INTERNAL_H

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "arch/sys_arch.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"


using namespace ::nl::Weave;

namespace nl {
namespace Weave {
namespace Logging {

enum
{
    kLogModule_DeviceLayer = 255,
};

} // namespace Logging
} // namespace Weave
} // namespace nl

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

extern const char * const TAG;

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
extern const uint64_t TestDeviceId;
extern const uint8_t TestDeviceCert[];
extern const uint8_t TestDevicePrivateKey[];
extern const uint16_t TestDeviceCertLength;
extern const uint16_t TestDevicePrivateKeyLength;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_INTERNAL_H

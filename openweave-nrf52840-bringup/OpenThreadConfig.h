/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      Overrides to default OpenThread configuration.
 *
 */

#ifndef OPENTHREAD_PLATFORM_CONFIG_H
#define OPENTHREAD_PLATFORM_CONFIG_H

// Disable the Nordic-supplied OpenThread logging facilities and use
// the facilities provided by the OpenWeave Device Layer (see
// openweave/src/adaptations/device-layer/nRF5/Logging.cpp).
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_APP

// Turn on a moderate level of logging in OpenThread
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_NOTE

// Use the Nordic-supplied default platform configuration for remainder
// of OpenThread config options.
//
// NB: This file gets included during the build of OpenThread.  Hence
// it cannot use "openthread" in the path to the included file.
//
#include "openthread-core-nrf52840-config.h"

#endif // OPENTHREAD_PLATFORM_CONFIG_H



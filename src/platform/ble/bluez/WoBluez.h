/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file defines WoBluez peripheral interfaces that hands the wrapper talking with BleLayer via the
 *      corresponding platform interface function when the application passively receives an incoming BLE connection.
 *
 */

#ifndef WOBLEZ_H_
#define WOBLEZ_H_

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "Weave/Support/logging/WeaveLogging.h"
#include "BluezHelperCode.h"

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

// Driven by BlueZ IO, calling into BleLayer:
void WoBLEz_NewConnection(void *user_data);
void WoBLEz_WriteReceived(void *user_data, const uint8_t *value, size_t len);
void WoBLEz_ConnectionClosed(void *user_data);
void WoBLEz_SubscriptionChange(void *user_data);
void WoBLEz_IndicationConfirmation(void *user_data);
bool WoBLEz_TimerCb(void *user_data);

// Called by BlePlatformDelegate:
bool WoBLEz_SendIndication(void *user_data, uint8_t *buffer, size_t len);

} /* namespace Bluez */
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif  /* WOBLEZ_H_ */

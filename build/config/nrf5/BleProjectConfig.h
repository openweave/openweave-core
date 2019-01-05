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
 *      Weave BleLayer project configuration for the nRF5 platforms.
 *
 */

#ifndef BLE_PROJECT_CONFIG_H
#define BLE_PROJECT_CONFIG_H

#include <ble_types.h>

// ==================== Platform Adaptations ====================

#define BLE_CONNECTION_OBJECT uint16_t
#define BLE_CONNECTION_UNINITIALIZED ((uint16_t)BLE_CONN_HANDLE_INVALID)
#define BLE_MAX_RECEIVE_WINDOW_SIZE 5

#define BLE_CONFIG_ERROR_TYPE ret_code_t
#define BLE_CONFIG_NO_ERROR NRF_SUCCESS
#define BLE_CONFIG_ERROR_MIN 6000000
#define BLE_CONFIG_ERROR_MAX 6000999
#define _BLE_CONFIG_ERROR(e) (BLE_CONFIG_ERROR_MIN + (e))


#endif // BLE_PROJECT_CONFIG_H

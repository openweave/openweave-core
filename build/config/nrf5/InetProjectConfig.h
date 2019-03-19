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
 *      Inet project configuration for the Nordic nRF52 platform.
 *
 */
#ifndef INETPROJECTCONFIG_H
#define INETPROJECTCONFIG_H

#include "sdk_errors.h"

// TODO: move this to Kconfig?
#define CONFIG_NUM_TCP_ENDPOINTS 8
#define CONFIG_NUM_UDP_ENDPOINTS 8

// ==================== General Configuration ====================

#define INET_CONFIG_ENABLE_IPV4 0
#define INET_CONFIG_NUM_TCP_ENDPOINTS CONFIG_NUM_TCP_ENDPOINTS
#define INET_CONFIG_NUM_UDP_ENDPOINTS CONFIG_NUM_UDP_ENDPOINTS
#define INET_CONFIG_NUM_TUN_ENDPOINTS 1

// ==================== Platform Adaptations ====================

#define INET_CONFIG_ERROR_TYPE ret_code_t
#define INET_CONFIG_NO_ERROR NRF_SUCCESS
#define INET_CONFIG_ERROR_MIN 1000000
#define INET_CONFIG_ERROR_MAX 1000999

#endif // INETPROJECTCONFIG_H

/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      Inet project configuration for esp32 builds.
 *
 */
#ifndef INETPROJECTCONFIG_H
#define INETPROJECTCONFIG_H

#include "esp_err.h"

#define INET_CONFIG_ERROR_TYPE esp_err_t
#define INET_CONFIG_NO_ERROR ESP_OK
#define INET_CONFIG_ERROR_MIN 1000000
#define INET_CONFIG_ERROR_MAX 1000999



#endif // INETPROJECTCONFIG_H

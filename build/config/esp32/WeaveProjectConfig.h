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
 *      Weave project configuration for the esp32 platform.
 *
 */
#ifndef WEAVEPROJECTCONFIG_H
#define WEAVEPROJECTCONFIG_H

#include "esp_err.h"

#define WEAVE_CONFIG_ERROR_TYPE esp_err_t
#define WEAVE_CONFIG_NO_ERROR ESP_OK
#define WEAVE_CONFIG_ERROR_MIN 4000000
#define WEAVE_CONFIG_ERROR_MAX 4000999

#define ASN1_CONFIG_ERROR_TYPE esp_err_t
#define ASN1_CONFIG_NO_ERROR ESP_OK
#define ASN1_CONFIG_ERROR_MIN 5000000
#define ASN1_CONFIG_ERROR_MAX 5000999

// Max number of Bindings per WeaveExchangeManager
#define WEAVE_CONFIG_MAX_BINDINGS 8

#define WEAVE_CONFIG_SECURITY_TEST_MODE 1

#define WEAVE_CONFIG_USE_OPENSSL_ECC 0
#define WEAVE_CONFIG_USE_MICRO_ECC 1
#define WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT 1
#define WEAVE_CONFIG_RNG_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG 1
#define WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI 0
#define WEAVE_CONFIG_AES_IMPLEMENTATION_PLATFORM 1
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG0 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG2 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG3 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG4 1
#define WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT 0

#define WEAVE_CONFIG_ENABLE_TUNNELING 1

#define WeaveDie() abort()

// TODO: not needed?
// #define WEAVE_CONFIG_EVENT_LOGGING_WDM_OFFLOAD 1

// #define WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS 1

// #define WEAVE_CONFIG_EVENT_LOGGING_NUM_EXTERNAL_CALLBACKS 2

// #define WDM_ENFORCE_EXPIRY_TIME 1

// #define WEAVE_CONFIG_ENABLE_WDM_UPDATE 1

// #define WEAVE_CONFIG_ENABLE_TAKE_INITIATOR 1

// #define WEAVE_CONFIG_ENABLE_TAKE_RESPONDER 1

#endif /* WEAVEPROJECTCONFIG_H */

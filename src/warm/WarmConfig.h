/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file defines default compile-time configuration constants
 *      for the Nest Weave Address and Routing Module (WARM).
 *
 *      Package integrators that wish to override these values should
 *      either use preprocessor definitions or create a project-
 *      specific WarmProjectConfig.h header and then assert
 *      HAVE_WARMPROJECTCONFIG_H.
 *
 */

#ifndef WARMCONFIG_H_
#define WARMCONFIG_H_

#include <BuildConfig.h>

#if HAVE_WARMPROJECTCONFIG_H
#include "WarmProjectConfig.h"
#endif

/**
 *  @note
 *    Configurations for expected devices.
 *
 *    Thread device.
 *       WARM_CONFIG_SUPPORT_CELLULAR              - 0
 *       WARM_CONFIG_SUPPORT_WIFI                  - 0
 *       WARM_CONFIG_SUPPORT_THREAD                - 1
 *       WARM_CONFIG_SUPPORT_THREAD_ROUTING        - 0
 *       WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK - 0
 *       WARM_CONFIG_SUPPORT_WEAVE_TUNNEL          - 0
 *       WARM_CONFIG_SUPPORT_BORDER_ROUTING        - 0
 *
 *    Thread + Router device.
 *       WARM_CONFIG_SUPPORT_CELLULAR              - 0
 *       WARM_CONFIG_SUPPORT_WIFI                  - 0
 *       WARM_CONFIG_SUPPORT_THREAD                - 1
 *       WARM_CONFIG_SUPPORT_THREAD_ROUTING        - 1
 *       WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK - 0
 *       WARM_CONFIG_SUPPORT_WEAVE_TUNNEL          - 0
 *       WARM_CONFIG_SUPPORT_BORDER_ROUTING        - 0
 *
 *    WiFi + Thread + Tunnel device.
 *       WARM_CONFIG_SUPPORT_CELLULAR              - 0
 *       WARM_CONFIG_SUPPORT_WIFI                  - 1
 *       WARM_CONFIG_SUPPORT_THREAD                - 1
 *       WARM_CONFIG_SUPPORT_THREAD_ROUTING        - 0
 *       WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK - 0
 *       WARM_CONFIG_SUPPORT_WEAVE_TUNNEL          - 1
 *       WARM_CONFIG_SUPPORT_BORDER_ROUTING        - 0
 *
 *    WiFi + Thread + Tunnel + Router device.
 *       WARM_CONFIG_SUPPORT_CELLULAR              - 0
 *       WARM_CONFIG_SUPPORT_WIFI                  - 1
 *       WARM_CONFIG_SUPPORT_THREAD                - 1
 *       WARM_CONFIG_SUPPORT_THREAD_ROUTING        - 1
 *       WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK - 0
 *       WARM_CONFIG_SUPPORT_WEAVE_TUNNEL          - 1
 *       WARM_CONFIG_SUPPORT_BORDER_ROUTING        - 1
 *
 *    Cellular + WiFi + Thread + Tunnel + Router device.
 *       WARM_CONFIG_SUPPORT_CELLULAR              - 1
 *       WARM_CONFIG_SUPPORT_WIFI                  - 1
 *       WARM_CONFIG_SUPPORT_THREAD                - 1
 *       WARM_CONFIG_SUPPORT_THREAD_ROUTING        - 1
 *       WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK - 0
 *       WARM_CONFIG_SUPPORT_WEAVE_TUNNEL          - 1
 *       WARM_CONFIG_SUPPORT_BORDER_ROUTING        - 1
 */

/**
 *  @def WARM_CONFIG_SUPPORT_CELLULAR
 *
 *  @brief
 *    Device supports cellular address and routing.
 *
 *    If a product has a Cellular interface and it is desired
 *    to communicate over that network using the Weave
 *    then this configuration should be enabled.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_CELLULAR
#define WARM_CONFIG_SUPPORT_CELLULAR                0
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_WIFI
 *
 *  @brief
 *    Support WiFi address and routing.
 *
 *    If a product has a WiFi interface and it is desired
 *    to use that interface for Weave routing then this
 *    configuration should be enabled.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_WIFI
#define WARM_CONFIG_SUPPORT_WIFI                    1
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_THREAD
 *
 *  @brief
 *    Support Thread address and routing.
 *
 *    If a product has a Thread interface and it is desired
 *    to use that interface for Weave routing then this
 *    configuration should be enabled.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_THREAD
#define WARM_CONFIG_SUPPORT_THREAD                  1
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_THREAD_ROUTING
 *
 *  @brief
 *    Device can act as a thread router.
 *
 *    If a product has a Thread interface and it is desired
 *    to act as a Thread router in the network then this
 *    configuration should be enabled. This feature depends
 *    on Thread support.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_THREAD_ROUTING
#define WARM_CONFIG_SUPPORT_THREAD_ROUTING          WARM_CONFIG_SUPPORT_THREAD
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_LEGACY6LoWPAN_NETWORK
 *
 *  @brief
 *    Device supports legacy 15.4 network communication.
 *
 *    If a product has a Thread interface and it is desired
 *    to communicate over that network using the legacy
 *    protocol then this configuration should be enabled.
 *    This feature depends on Thread support.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK
#define WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK   WARM_CONFIG_SUPPORT_THREAD
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
 *
 *  @brief
 *    Device supports a Weave tunnel to the cloud based Service.
 *
 *    If the product is expected to provide a Weave tunnel to
 *    the Service then this configuration should be enabled.
 *    This feature is depends on Weave tunneling and one of
 *    WiFi or Cellular Support.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
#define WARM_CONFIG_SUPPORT_WEAVE_TUNNEL            (WEAVE_CONFIG_ENABLE_TUNNELING && (WARM_CONFIG_SUPPORT_WIFI || WARM_CONFIG_SUPPORT_CELLULAR))
#endif

/**
 *  @def WARM_CONFIG_SUPPORT_BORDER_ROUTING
 *
 *  @brief
 *    Support Weave Border Routing functionality.
 *
 *    If a product has a WiFi interface and it is desired
 *    for this product to act as a Border router then this
 *    configuration should be enabled.  This feature depends
 *    on Thread support, Tunnel support and one of WiFi or Cellular
 *    support.
 *
 */
#ifndef WARM_CONFIG_SUPPORT_BORDER_ROUTING
#define WARM_CONFIG_SUPPORT_BORDER_ROUTING          (WARM_CONFIG_SUPPORT_THREAD && WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && (WARM_CONFIG_SUPPORT_WIFI || WARM_CONFIG_SUPPORT_CELLULAR))
#endif

#endif /* WARMCONFIG_H_ */

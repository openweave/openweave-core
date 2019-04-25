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

/* Include a project-specific configuration file, if defined.
 *
 * An application or module that incorporates Weave can define a project configuration
 * file to override standard WARM configuration with application-specific values.
 * The project config file is typically located outside the OpenWeave source tree,
 * along side the source code for the application.
 */
#ifdef WARM_PROJECT_CONFIG_INCLUDE
#include WARM_PROJECT_CONFIG_INCLUDE
#endif

// clang-format off

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
 *  @def WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK
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

/**
 *  @def WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING
 *
 *  @brief
 *    Enable the use of fabric-default /48 routes
 *    for routing external traffic for
 *    unknown/non-local subnets to the Nest service.
 *
 *    When enabled, the WARM layer will install a
 *    /48 route, with the fabric prefix, that points
 *    at the service tunnel interface whenever a tunnel
 *    connection is established with the service. This
 *    results in traffic to unknown subnets being routed
 *    over the tunnel connection. Additionally, if
 *    #WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD is
 *    also enabled, WARM will assign an identical /48
 *    route, at low priority, to the Thread interface,
 *    causing fabric-default traffic to route across
 *    the Thread network(to another potential border
 *    gateway) whenever the local service tunnel
 *    is down.
 *
 *    Disabling this option disables all fabric default
 *    routing, resulting in traffic to unknown subnets
 *    dying in the local network stack. Traffic to known
 *    subnets (WiFi, Thread, Service, etc.) is unaffected.
 *
 *    This option exists primarily to support legacy device
 *    behavior and should be disabled by default on new
 *    devices.
 *
 */
#ifndef WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING
#define WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING                 1
#endif // WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING

/**
 *  @def WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD
 *
 *  @brief
 *    Enable routing of service traffic (and possibly
 *    traffic to unknown subnets) over the Thread interface
 *    as a fallback option when the tunnel to the Service
 *    is down.
 *
 *    When enabled, WARM assigns low-priority routes to
 *    the Thread interface that result in traffic destined
 *    to the service(or other external subnets) being routed
 *    across the Thread network whenever the local service
 *    tunnel is down. This allows, for example, a device
 *    with an off-line WiFi interface to route its service
 *    traffic through another border gateway in the network
 *    that has connectivity to the service.
 *
 *    If #WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING is also
 *    enabled, traffic to unknown/non-local subnets will also
 *    route across Thread in the event the service tunnel is
 *    down. More specifically, if both options are enabled,
 *    WARM will assign a low-priority /48 fabric route to
 *    the Thread interface. If only
 *    #WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD is enabled,
 *    WARM will assign a low-priority /64 route for just the
 *    Service subnet to the Thread interface.
 *
 *    Disabling this option results in traffic to the service
 *    (or to unknown subnets) dying in the local network stack
 *    whenever the tunnel to the service down.
 *
 *    Device implementers should enable this option only if
 *    they know that the volume of traffic exchanged with the
 *    service is small enough to be accommodated by the Thread
 *    network.
 *
 */
#ifndef WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD
#define WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD             1
#endif // WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD

// clang-format on

#endif /* WARMCONFIG_H_ */

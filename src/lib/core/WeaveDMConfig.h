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

// clang-format off

/**
 *    @file
 *      This file defines default compile-time configuration constants
 *      for the Nest Weave Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_CONFIG_H
#define _WEAVE_DATA_MANAGEMENT_CONFIG_H

#include <BuildConfig.h>

/****************************************************************/
/* WDM Legacy                                                   */
/****************************************************************/

/**
 *  @def WEAVE_CONFIG_LEGACY_WDM
 *
 *  @brief
 *    Enable or disable the legacy versions of WDM
 *
 *  This configuration option is controlled by the
 *  configure script. See --disable-legacy-wdm
 */

#ifndef WEAVE_CONFIG_LEGACY_WDM
#define WEAVE_CONFIG_LEGACY_WDM    1
#endif

/**
 *  @def WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES
 *
 *  @brief
 *    Allow or disallow subscription on WDM clients.
 *
 * Between WDM 1.2 and WDM 2.0 there was a global revision of the
 * message types in order to support changes in message format. For
 * some time in the future, certain application - e.g., Amber - will
 * require limited support for the old message types as well.
 */

#ifndef WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES
#define WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES    1
#endif

/**
 *  @def WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES
 *
 *  @brief
 *    Allow or disallow subscription on WDM publishers.
 *
 * Between WDM 1.2 and WDM 2.0 there was a global revision of the
 * message types in order to support changes in message format. For
 * some time in the future, certain application - e.g., Amber - will
 * require limited support for the old message types as well.
 */

#ifndef WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES
#define WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES 1
#endif

/**
 *  @def WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION
 *
 *  @brief
 *    Allow or disallow subscription on WDM clients.
 *
 * For clients with constrained resources, there may be no reason to
 * set up a subscription table. Subscription is enabled by default but
 * may be disabled in the project config file.
 */

#ifndef WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION
#define WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION            1
#endif

/**
 *  @def WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
 *
 *  @brief
 *    Allow or disallow subscription on WDM publishers.
 *
 * For publishers with constrained resources, there may be no reason
 * to set up a subscription table. Subscription is enabled by default
 * but may be disabled in the project config file.
 */

#ifndef WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
#define WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION         1
#endif

/*
 * the protocol engine maintains a group of pools to keep track of
 * state in protocol exchanges. here are the sizes.
 */

/**
 *  @def WEAVE_CONFIG_WDM_VIEW_POOL_SIZE
 *
 *  @brief
 *    The default size of the view transaction pool in DMClient.
 */

#ifndef WEAVE_CONFIG_WDM_VIEW_POOL_SIZE
#define WEAVE_CONFIG_WDM_VIEW_POOL_SIZE                       2
#endif

/**
 *  @def WEAVE_CONFIG_WDM_SUBSCRIBE_POOL_SIZE
 *
 *  @brief
 *    The default size of the subscribe transaction pool in DMClient.
 */

#ifndef WEAVE_CONFIG_WDM_SUBSCRIBE_POOL_SIZE
#define WEAVE_CONFIG_WDM_SUBSCRIBE_POOL_SIZE                  2
#endif

/**
 *  @def WEAVE_CONFIG_WDM_CANCEL_SUBSCRIPTION_POOL_SIZE
 *
 *  @brief
 *    The default size of the cancel subscription transaction pool in
 *    DMClient.
 */

#ifndef WEAVE_CONFIG_WDM_CANCEL_SUBSCRIPTION_POOL_SIZE
#define WEAVE_CONFIG_WDM_CANCEL_SUBSCRIPTION_POOL_SIZE        2
#endif

/**
 *  @def WEAVE_CONFIG_WDM_UPDATE_POOL_SIZE
 *
 *  @brief
 *    The default size of the update transaction pool in DMClient.
 */

#ifndef WEAVE_CONFIG_WDM_UPDATE_POOL_SIZE
#define WEAVE_CONFIG_WDM_UPDATE_POOL_SIZE                     2
#endif

/**
 *  @def WEAVE_CONFIG_WDM_NOTIFY_POOL_SIZE
 *
 *  @brief
 *    The default size of the cancel subscription pool in DMPublisher.
 */

#ifndef WEAVE_CONFIG_WDM_NOTIFY_POOL_SIZE
#define WEAVE_CONFIG_WDM_NOTIFY_POOL_SIZE                     2
#endif

/*
 * and here is the related size of the transaction table and binding
 * table managed by ProtocolEngine objects.
 */

/**
 *  @def WEAVE_CONFIG_WDM_TRANSACTION_TABLE_SIZE
 *
 *  @brief
 *    The default size of the transaction table in ProtocolEngine.
 */

#ifndef WEAVE_CONFIG_WDM_TRANSACTION_TABLE_SIZE
#define WEAVE_CONFIG_WDM_TRANSACTION_TABLE_SIZE               2
#endif

/**
 *  @def WEAVE_CONFIG_WDM_BINDING_TABLE_SIZE
 *
 *  @brief
 *    The default size of the binding table in ProtocolEngine.
 */

#ifndef WEAVE_CONFIG_WDM_BINDING_TABLE_SIZE
#define WEAVE_CONFIG_WDM_BINDING_TABLE_SIZE                   1
#endif

/*
 * similarly, the client notifier and publisher subscription manager
 * keep subscription tables (with slightly different formats). here
 * are the sizes.
 */

/**
 *  @def WEAVE_CONFIG_WDM_NOTIFIER_TABLE_SIZE
 *
 *  @brief
 *    The default size of the notification table in the singleton
 *    ClientNotifier.
 */

#ifndef WEAVE_CONFIG_WDM_NOTIFIER_TABLE_SIZE
#define WEAVE_CONFIG_WDM_NOTIFIER_TABLE_SIZE                  8
#endif

/**
 *  @def WEAVE_CONFIG_WDM_SUBSCRIPTION_MGR_TABLE_SIZE
 *
 *  @brief
 *    The default size of the subscription table used by DMPublisher.
 */

#ifndef WEAVE_CONFIG_WDM_SUBSCRIPTION_MGR_TABLE_SIZE
#define WEAVE_CONFIG_WDM_SUBSCRIPTION_MGR_TABLE_SIZE          8
#endif

/****************************************************************/
/* WDM Current                                                  */
/****************************************************************/

/**
 *  @def WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
 *
 *  @brief
 *    Enable (1) or disable (0) pre-flight schema validation
 *    for protocol messages used in Weave Data Management Next profile.
 *
 */
#ifndef WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
#define WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK 1
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

/**
 *  @def WDM_MAX_NUM_SUBSCRIPTION_CLIENTS
 *
 *  @brief
 *    Enable (1) or disable (0) number of active client side
 *    subscriptions supported by Weave Data Management Next
 *    profile.
 *
 */
#ifndef WDM_MAX_NUM_SUBSCRIPTION_CLIENTS
#define WDM_MAX_NUM_SUBSCRIPTION_CLIENTS 2
#endif // WDM_MAX_NUM_SUBSCRIPTION_CLIENTS

/**
 *  @def WDM_MAX_NUM_SUBSCRIPTION_HANDLERS
 *
 *  @brief
 *    Enable (1) or disable (0) number of active publisher side
 *    subscriptions supported by Weave Data Management Next
 *    profile.
 *
 */
#ifndef WDM_MAX_NUM_SUBSCRIPTION_HANDLERS
#define WDM_MAX_NUM_SUBSCRIPTION_HANDLERS 2
#endif // WDM_MAX_NUM_SUBSCRIPTION_HANDLERS

/**
 *  @def WDM_ENABLE_SUBSCRIPTION_CANCEL
 *
 *  @brief
 *    Enable (1) or disable (0) sending/handling cancel request
 *    in Weave Data Management Next profile.
 *
 */
#ifndef WDM_ENABLE_SUBSCRIPTION_CANCEL
#define WDM_ENABLE_SUBSCRIPTION_CANCEL 1
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

/**
 *  @def WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
 *
 *  @brief
 *    Enable (1) or disable (0) sending/handling  subscriptionless
 *    notifications in Weave Data Management Next profile.
 *
 */
#ifndef WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
#define WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION 1
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

/**
 *  @def WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
 *
 *  @brief
 *    Enable (1) or disable (0) sending/handling update response
 *    in Weave Data Management Next profile.
 *
 */
#ifndef WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
#define WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT 1
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

/**
 *  @def WDM_PUBLISHER_ENABLE_VIEW
 *
 *  @brief
 *    Enable (1) or disable (0) handling view request
 *    in Weave Data Management Next profile.
 *
 */
#if WDM_PUBLISHER_ENABLE_VIEW
#error "WDM_PUBLISHER_ENABLE_VIEW is set to 1 but the feature is not yet implemented"
#endif
#ifndef WDM_PUBLISHER_ENABLE_VIEW
#define WDM_PUBLISHER_ENABLE_VIEW 0
#endif // WDM_PUBLISHER_ENABLE_VIEW

/**
 *  @def WDM_PUBLISHER_MAX_NUM_PATH_GROUPS
 *
 *  @brief
 *    Sum number of paths, group by trait instance for each subscription,
 *    in all active subscriptions, on a publisher in Weave Data Management Next profile.
 *
 */
#ifndef WDM_PUBLISHER_MAX_NUM_PATH_GROUPS
#define WDM_PUBLISHER_MAX_NUM_PATH_GROUPS 8
#endif // WDM_PUBLISHER_MAX_NUM_PATH_GROUPS

/**
 *  @def WDM_CLIENT_MAX_NUM_UPDATABLE_TRAITS
 *
 *  @brief
 *    The number of updatable trait instances for each
 *    Weave Data Management subscription.
 *    SubscriptionClient asserts if it is initialized with a higher
 *    number of updatable trait instances than this value.
 *
 */
#ifndef WDM_CLIENT_MAX_NUM_UPDATABLE_TRAITS
#define WDM_CLIENT_MAX_NUM_UPDATABLE_TRAITS 8
#endif // WDM_CLIENT_MAX_NUM_UPDATABLE_TRAITS

/**
 *  @def WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES
 *
 *  @brief
 *    Sum number of non-root paths in all active subscriptions
 *    on a publisher in Weave Data Management Next profile.
 *
 */
#ifndef WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES
#define WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES 1
#endif // WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES

/**
 *  @def WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES_PER_GROUP
 *
 *  @brief
 *    Max number of non-root paths to the same trait instance per subscription
 *    on a publisher in Weave Data Management Next profile.
 *
 */
#ifndef WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES_PER_GROUP
#define WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES_PER_GROUP 1
#endif // WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES_PER_GROUP

/**
 *  @def WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER
 *
 *  @brief
 *    Selects the right solver for the notification engine to use.
 *    Options:
 *
 *      BasicSolver:
 *          Enables the basic solver for the notification engine. Use this only
 *          as a means to test/debug issues with the notification engine, or if you are really
 *          constrained in flash/ram on your device. The resultant notifications will be very large
 *          and un-optimal.
 *
 *      IntermediateSolver:
 *          The nominal solver for most applications. Generates reasonably compact notifies in most
 *          cases.
 */
#ifndef WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER
#define WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER IntermediateGraphSolver
#endif

/**
 *  @def WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE
 *
 *  @brief
 *    Determines the maximum number of dirty items that can be stored in the granular trait data
 *    dirty/delete stores. This is a function of the peak # of handles across all trait instances that can be made dirty
 *    within any given evaluation cycle.
 */
#ifndef WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE
#define WDM_PUBLISHER_MAX_ITEMS_IN_TRAIT_DIRTY_STORE  10
#endif

/**
 *  @def WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET
 *
 *  @brief
 *    Determines the maximum number of mergeable handles to a parent handle for a notify that can be computed.
 *    Given most of our traits are flat, this really becomes a function of the peak # of dirty handles within a trait instance
 *    that can be made dirty within any given evaluation cycle.
 *
 */
#ifndef WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET
#define WDM_PUBLISHER_INTERMEDIATE_SOLVER_MAX_MERGE_HANDLE_SET 4
#endif

/**
 *  @def WDM_UPDATE_MAX_ITEMS_IN_TRAIT_DIRTY_PATH_STORE
 *
 *  @brief
 *    Determines the maximum number of dirty items that can be stored in the granular trait data
 *    dirty stores. This is a function of the peak # of handles across all trait instances that can be made dirty
 *    within any given evaluation cycle.
 */
#ifndef WDM_UPDATE_MAX_ITEMS_IN_TRAIT_DIRTY_PATH_STORE
#define WDM_UPDATE_MAX_ITEMS_IN_TRAIT_DIRTY_PATH_STORE  10
#endif

/**
 *  @def WDM_PUBLISHER_MAX_NOTIFIES_IN_FLIGHT
 *
 *  @brief
 *    Controls the maximum number of notifies across all subscriptions that can be in flight at any given time. This might be useful
 *    to rate-limit the usage of resources on devices where the subscriber pool is large and generating notifies to all of them at the same
 *    time can overwhelm resources on the device or induce congestion in the network.
 *
 */
#ifndef WDM_PUBLISHER_MAX_NOTIFIES_IN_FLIGHT
#define WDM_PUBLISHER_MAX_NOTIFIES_IN_FLIGHT 4
#endif

/**
 * The auto-generated schema tables key off this define to enable/disable certain fields in the tables. Enable this for now, but remove this define
 * once it has been similarly removed from the auto-generated code since all products are expected to need dictionary support, so the savings in flash/ram
 * by keeping dictionary support out is not needed.
 */
#ifndef TDM_DICTIONARY_SUPPORT
#define TDM_DICTIONARY_SUPPORT 1
#endif

/**
 * This controls whether publisher support for dictionaries is compiled in to TDM. If this is not enabled, you will not be able to support publishing dictionaries
 * on devices. This is enabled by default, so it's important that platforms don't desire this disable it to get significant flash savings.
 */
#ifndef TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
#define TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT 1
#endif

/**
 * @def TDM_EXTENSION_SUPPORT
 *
 * @brief Enable (1) or disable (0) support for sharing of the schema
 *   tables between traits where one trait extends another trait.
 */
#ifndef TDM_EXTENSION_SUPPORT
#define TDM_EXTENSION_SUPPORT 0
#endif

/**
 * @def TDM_VERSIONING_SUPPORT
 *
 * @brief Enable (1) or disable (0) support for embedding trait
 * schema version information in the generated schema tables.
 */
#ifndef TDM_VERSIONING_SUPPORT
#define TDM_VERSIONING_SUPPORT 1
#endif

/**
 *  @def WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS
 *
 *  @brief
 *    Enable (1) or disable (0) custom command handler support
 *    in Weave Data Management Next profile. This feature is
 *    optional and could be disabled, for example, to save code space.
 *
 */
#ifndef WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS
#define WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS 1
#endif

#if WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS
#ifndef WDM_MAX_NUM_COMMAND_OBJECTS
#define WDM_MAX_NUM_COMMAND_OBJECTS (WDM_MAX_NUM_SUBSCRIPTION_HANDLERS)
#endif // WDM_MAX_NUM_COMMAND_OBJECTS
#endif // WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS

/**
 *  @def WEAVE_CONFIG_ENABLE_WDM_UPDATE
 *
 *  @brief
 *    Enable (1) or disable (0) update support
 *    in Weave Data Management Next profile. This feature is
 *    optional and could be disabled.
 *
 */
#ifndef WEAVE_CONFIG_ENABLE_WDM_UPDATE
#define WEAVE_CONFIG_ENABLE_WDM_UPDATE 0
#endif

/**
 *  @def WDM_ENABLE_SUBSCRIPTION_PUBLISHER
 *
 *  @brief
 *    Enable (1) or disable (0) publisher support
 *    in Weave Data Management Next profile.
 *
 */
#if WDM_MAX_NUM_SUBSCRIPTION_HANDLERS > 0
#define WDM_ENABLE_SUBSCRIPTION_PUBLISHER 1
#else
#define WDM_ENABLE_SUBSCRIPTION_PUBLISHER 0
#endif // WDM_MAX_NUM_SUBSCRIPTION_HANDLERS > 0

/**
 *  @def WDM_ENABLE_SUBSCRIPTION_CLIENT
 *
 *  @brief
 *    Enable (1) or disable (0) subscription client role support
 *    in Weave Data Management Next profile.
 *
 */
#if WDM_MAX_NUM_SUBSCRIPTION_CLIENTS > 0
#define WDM_ENABLE_SUBSCRIPTION_CLIENT 1
#else
#define WDM_ENABLE_SUBSCRIPTION_CLIENT 0
#endif // WDM_MAX_NUM_SUBSCRIPTION_CLIENTS > 0

/**
 * @def WDM_MAX_NOTIFICATION_SIZE
 *
 * @brief
 *   Specify the maximum size (in bytes) of a WDM notification
 *   payload. Note that the WDM notification payload is also limited
 *   by the size of `nl::Weave::System::PacketBuffer`
 */
#ifndef WDM_MAX_NOTIFICATION_SIZE
#define WDM_MAX_NOTIFICATION_SIZE 2048
#endif /* WDM_MAX_NOTIFICATION_SIZE */

/**
 * @def WDM_MIN_NOTIFICATION_SIZE
 *
 * @brief
 *   Specify the minimum size (in bytes) of a WDM notification
 *   payload.
 */
#ifndef WDM_MIN_NOTIFICATION_SIZE
#define WDM_MIN_NOTIFICATION_SIZE 1024
#endif /* WDM_MIN_NOTIFICATION_SIZE */

/**
 * @def WDM_MAX_UPDATE_SIZE
 *
 * @brief
 *   Specify the maximum size (in bytes) of a WDM update
 *   payload. Note that the WDM update payload is also limited
 *   by the size of `nl::Weave::System::PacketBuffer`
 */
#ifndef WDM_MAX_UPDATE_SIZE
#define WDM_MAX_UPDATE_SIZE 2048
#endif /* WDM_MAX_UPDATE_SIZE */

/**
 * @def WDM_MIN_UPDATE_SIZE
 *
 * @brief
 *   Specify the minimum size (in bytes) of a WDM update
 *   payload.
 */
#ifndef WDM_MIN_UPDATE_SIZE
#define WDM_MIN_UPDATE_SIZE 1024
#endif /* WDM_MIN_UPDATE_SIZE */

/**
 * @def WDM_MAX_UPDATE_RESPONSE_SIZE
 *
 * @brief
 *   Specify the maximum size (in bytes) of a WDM update response
 *   payload. Note that the WDM update payload is also limited
 *   by the size of `nl::Weave::System::PacketBuffer`
 */
#ifndef WDM_MAX_UPDATE_RESPONSE_SIZE
#define WDM_MAX_UPDATE_RESPONSE_SIZE 2048
#endif /* WDM_MAX_UPDATE_SIZE */

/**
 * @def WDM_MIN_UPDATE_RESPONSE_SIZE
 *
 * @brief
 *   Specify the minimum size (in bytes) of a WDM update response
 *   payload.
 */
#ifndef WDM_MIN_UPDATE_RESPONSE_SIZE
#define WDM_MIN_UPDATE_RESPONSE_SIZE 1024
#endif /* WDM_MIN_UPDATE_SIZE */

/**
 *  @def TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
 *
 *  @brief
 *    Disables strict compliance to trait schemas to allow
 *    rapid development when schemas are prone to a high
 *    rate of change
 *
 */
#ifndef TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
#define TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE 1
#endif

/**
 *  @def WDM_ENABLE_PROTOCOL_CHECKS
 *
 *  @brief
 *    Enables aggressive checking of inbound data
 *    to ensure it's compliant against the latest WDM protocol specification
 *
 */
#ifndef WDM_ENABLE_PROTOCOL_CHECKS
#define WDM_ENABLE_PROTOCOL_CHECKS 1
#endif

/**
 *  @def WDM_ENFORCE_EXPIRY_TIME
 *
 *  @brief
 *    Enables cecking the expiry time of WDM commands
 *    before the command is handed over to the application.
 */
#ifndef WDM_ENFORCE_EXPIRY_TIME
#define WDM_ENFORCE_EXPIRY_TIME 0
#endif

/**
 *  @def WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS
 *
 *  @brief
 *    If auto resubscribe is enabled & default resubscription policy is used,
 *    specify the max wait time.
 *    This value was chosen so that the average wait time is 3600000
 *    ((100 - WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP) % of WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS) / 2 +
 *    (WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP % of WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS) = average wait is 3600000
 */
#ifndef WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS
#define WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS 5538000
#endif

/**
 *  @def WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX
 *
 *  @brief
 *    If auto resubscribe is enabled & default resubscription policy is used,
 *    specify the max fibonacci step index.
 *    This index must satisfy below conditions:
 *    1 . Fibonacci(WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX + 1) * WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS > WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS
 *    2 . Fibonacci(WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX) * WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS < WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS
 *
 */
#ifndef WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX
#define WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX 14
#endif

/**
 *  @def WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP
 *
 *  @brief
 *    If auto resubscribe is enabled & default resubscription policy is used,
 *    specify the minimum wait
 *    time as a percentage of the max wait interval for that step.
 *
 */
#ifndef WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP
#define WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP 30
#endif

/**
 *  @def WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS
 *
 *  @brief
 *    If auto resubscribe is enabled & default resubscription policy is used,
 *    specify the multiplier that multiplies the result of a fibonacci computation
 *    based on a specific index to provide a max wait time for
 *    a step.
 *
 */
#ifndef WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS
#define WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS 10000
#endif

// clang-format on

#endif // _WEAVE_DATA_MANAGEMENT_CONFIG_H

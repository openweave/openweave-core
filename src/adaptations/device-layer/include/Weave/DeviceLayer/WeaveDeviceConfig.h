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
 *          Defines compile-time configuration values for the Weave Device Layer.
 */



#ifndef WEAVE_DEVICE_CONFIG_H
#define WEAVE_DEVICE_CONFIG_H


// Include a header file containing the platform-specific configuration overrides.

#ifdef EXTERNAL_WEAVEDEVICEPLATFORMCONFIG_HEADER
#include EXTERNAL_WEAVEDEVICEPLATFORMCONFIG_HEADER
#else
#define WEAVEDEVICEPLATFORMCONFIG_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/WeaveDevicePlatformConfig.h>
#include WEAVEDEVICEPLATFORMCONFIG_HEADER
#endif


// -------------------- General Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_WEAVE_TASK_NAME
 *
 * The name of the Weave task.
 */
#ifndef WEAVE_DEVICE_CONFIG_WEAVE_TASK_NAME
#define WEAVE_DEVICE_CONFIG_WEAVE_TASK_NAME "weave"
#endif

/**
 * WEAVE_DEVICE_CONFIG_WEAVE_TASK_STACK_SIZE
 *
 * The size (in bytes) of the Weave task stack.
 */
#ifndef WEAVE_DEVICE_CONFIG_WEAVE_TASK_STACK_SIZE
#define WEAVE_DEVICE_CONFIG_WEAVE_TASK_STACK_SIZE 4096
#endif

/**
 * WEAVE_DEVICE_CONFIG_WEAVE_TASK_PRIORITY
 *
 * The priority of the Weave task.
 */
#ifndef WEAVE_DEVICE_CONFIG_WEAVE_TASK_PRIORITY
#define WEAVE_DEVICE_CONFIG_WEAVE_TASK_PRIORITY 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE
 *
 * The maximum number of events that can be held in the Weave Platform event queue.
 */
#ifndef WEAVE_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE
#define WEAVE_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE 100
#endif

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE
 *
 * The size (in bytes) of the service directory cache.
 */
#ifndef WEAVE_DEVICE_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE
#define WEAVE_DEVICE_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE 512
#endif

// -------------------- Device Identification Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID
 *
 * The Nest-assigned vendor id for the organization responsible for producing the device.
 */
#ifndef WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID
#define WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID 9050
#endif

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * The unique id assigned by the device vendor to identify the product or device type.  This
 * number is scoped to the device vendor id.
 */
#ifndef WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID
#define WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID 65279
#endif

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION
 *
 * The product revision number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software revisions.
 */
#ifndef WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION
#define WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
 *
 * A string identifying the firmware revision running on the device.
 */
#ifndef WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
#define WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION "prerelease"
#endif

// -------------------- WiFi Station Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL
 *
 * The interval at which the Weave platform will attempt to reconnect to the configured WiFi
 * network (in milliseconds).
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL
#define WEAVE_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL 5000
#endif

/**
 * WEAVE_DEVICE_CONFIG_MAX_SCAN_NETWORKS_RESULTS
 *
 * The maximum number of networks to return as a result of a NetworkProvisioning:ScanNetworks request.
 */
#ifndef WEAVE_DEVICE_CONFIG_MAX_SCAN_NETWORKS_RESULTS
#define WEAVE_DEVICE_CONFIG_MAX_SCAN_NETWORKS_RESULTS 10
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
 *
 * The amount of time (in milliseconds) after which the Weave platform will timeout a WiFi scan
 * operation that hasn't completed.  A value of 0 will disable the timeout logic.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
#define WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT 10000
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_CONNECTIVITY_TIMEOUT
 *
 * The amount of time (in milliseconds) to wait for Internet connectivity to be established on
 * the device's WiFi station interface during a Network Provisioning TestConnectivity operation.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_CONNECTIVITY_TIMEOUT
#define WEAVE_DEVICE_CONFIG_WIFI_CONNECTIVITY_TIMEOUT 30000
#endif

// -------------------- WiFi AP Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX
 *
 * A prefix string used in forming the WiFi soft-AP SSID.  The remainder of the SSID
 * consists of the final two bytes of the device's primary WiFi MAC address in hex.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX
#define WEAVE_DEVICE_CONFIG_WIFI_AP_SSID_PREFIX "NEST-"
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_AP_CHANNEL
 *
 * The WiFi channel number to be used by the soft-AP.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_AP_CHANNEL
#define WEAVE_DEVICE_CONFIG_WIFI_AP_CHANNEL 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_AP_MAX_STATIONS
 *
 * The maximum number of stations allowed to connect to the soft-AP.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_AP_MAX_STATIONS
#define WEAVE_DEVICE_CONFIG_WIFI_AP_MAX_STATIONS 4
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_AP_BEACON_INTERVAL
 *
 * The beacon interval (in milliseconds) for the WiFi soft-AP.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_AP_BEACON_INTERVAL
#define WEAVE_DEVICE_CONFIG_WIFI_AP_BEACON_INTERVAL 100
#endif

/**
 * WEAVE_DEVICE_CONFIG_WIFI_AP_IDLE_TIMEOUT
 *
 * The amount of time (in milliseconds) after which the Weave platform will deactivate the soft-AP
 * if it has been idle.
 */
#ifndef WEAVE_DEVICE_CONFIG_WIFI_AP_IDLE_TIMEOUT
#define WEAVE_DEVICE_CONFIG_WIFI_AP_IDLE_TIMEOUT 120000
#endif

// -------------------- BLE/WoBLE Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
 *
 * Enable support for Weave-over-BLE (WoBLE).
 */
#ifndef WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
#define WEAVE_DEVICE_CONFIG_ENABLE_WOBLE 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX
 *
 * A prefix string used in forming the BLE device name.  The remainder of the name
 * consists of the final two bytes of the device's Weave node id in hex.
 *
 * NOTE: The device layer limits the total length of a device name to 16 characters.
 * However, due to other data sent in WoBLE advertise packets, the device name
 * may need to be shorter.
 */
#ifndef WEAVE_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX
#define WEAVE_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX "NEST-"
#endif

/**
 * WEAVE_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL
 *
 * The interval (in units of 0.625ms) at which the device will send BLE advertisements while
 * in fast advertising mode.
 *
 * Defaults to 800 (500ms).
 */
#ifndef WEAVE_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL
#define WEAVE_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL 800
#endif

/**
 * WEAVE_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL
 *
 * The interval (in units of 0.625ms) at which the device will send BLE advertisements while
 * in slow advertisement mode.
 *
 * Defaults to 3200 (20000ms).
 */
#ifndef WEAVE_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL
#define WEAVE_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL 3200
#endif



// -------------------- Time Sync Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
 *
 * Enables synchronizing the device real-time clock using information returned during
 * a Weave service end point query.  For any device that uses the Weave service directory
 * to lookup a tunnel server, enabling this option will result in the real time clock being
 * synchronized every time the service tunnel is established.
 */
#ifndef WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
#define WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
 *
 * Enables synchronizing the device's real time clock with a remote Weave Time service
 * using the Weave Time Sync protocol.
 */
#ifndef WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
#define WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC 1
#endif

/**
 * WEAVE_DEVICE_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID
 *
 * Specifies the service endpoint id of the Weave Time Sync service to be used to synchronize time.
 *
 * This value is only meaningful if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_DEVICE_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID
#define WEAVE_DEVICE_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID 0x18B4300200000005ULL
#endif

/**
 * WEAVE_DEVICE_CONFIG_DEFAULT_TIME_SYNC_INTERVAL
 *
 * Specifies the minimum interval (in seconds) at which the device should synchronize its real time
 * clock with the configured Weave Time Sync server.
 *
 * This value is only meaningful if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_DEVICE_CONFIG_DEFAULT_TIME_SYNC_INTERVAL
#define WEAVE_DEVICE_CONFIG_DEFAULT_TIME_SYNC_INTERVAL 180
#endif

/**
 * WEAVE_DEVICE_CONFIG_TIME_SYNC_TIMEOUT
 *
 * Specifies the maximum amount of time (in milliseconds) to wait for a response from a
 * Weave Time Sync server.
 *
 * This value is only meaningful if WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_DEVICE_CONFIG_TIME_SYNC_TIMEOUT
#define WEAVE_DEVICE_CONFIG_TIME_SYNC_TIMEOUT 10000
#endif


// -------------------- Service Provisioning Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_ENDPOINT_ID
 *
 * Specifies the service endpoint id of the Weave Service Provisioning service.  When a device
 * undergoes service provisioning, this is the endpoint to which it will send its Pair Device
 * to Account request.
 */
#ifndef WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_ENDPOINT_ID
#define WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_ENDPOINT_ID 0x18B4300200000010ULL
#endif

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT
 *
 * The maximum amount of time (in milliseconds) to wait for service connectivity during the device
 * service provisioning step.  More specifically, this is the maximum amount of time the device will
 * wait for connectivity to be established with the service at the point where the device waiting
 * to send a Pair Device to Account request to the Service Provisioning service.
 */
#ifndef WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT
#define WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT 10000
#endif

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT
 *
 * Specifies the maximum amount of time (in milliseconds) to wait for a response from the Service
 * Provisioning service.
 */
#ifndef WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT
#define WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT 10000
#endif


// -------------------- Test Configuration --------------------

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
 *
 * Enables the use of a hard-coded default Weave device id and credentials if no device id
 * is found in Weave NV storage.  The value specifies which of 10 identities, numbered 1 through 10,
 * is to be used.  A value of 0 disables use of a default identity.
 *
 * This option is for testing only and should be disabled in production releases.
 */
#ifndef WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
#define WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY 0
#endif

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER
 *
 * Forces the use of a service tunnel server at a fixed IP address and port.  This
 * bypasses the need for a directory query to the service directory endpoint to
 * determine the tunnel server address.  When enabled, this option allows devices
 * that haven't been service provisioned to establish a service tunnel.
 *
 * When this option is enabled, WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS must
 * be set to the address of the tunnel server.
 */
#ifndef WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER
#define WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER 0
#endif


/** WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS
 *
 * The address of the server to which the device should establish a service tunnel.
 *
 * This value is only meaningful if WEAVE_DEVICE_CONFIG_ENABLE_FIXED_TUNNEL_SERVER
 * has been enabled.
 *
 * Note: Currently this must be a dot-notation IP address--not a host name.
 */
#ifndef WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS
#define WEAVE_DEVICE_CONFIG_TUNNEL_SERVER_ADDRESS ""
#endif

/**
 * WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
 *
 * Disables sending the PairDeviceToAccount request to the service during a RegisterServicePairAccount
 * operation.  When this option is enabled, the device will perform all local operations associated
 * with registering a service, but will not request the service to add the device to the user's account.
 */
#ifndef WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
#define WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING 0
#endif

#endif // WEAVE_DEVICE_CONFIG_H

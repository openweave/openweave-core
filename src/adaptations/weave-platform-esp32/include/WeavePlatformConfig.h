#ifndef WEAVE_PLATFORM_CONFIG_H
#define WEAVE_PLATFORM_CONFIG_H

// -------------------- General Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_MAX_EVENT_QUEUE_SIZE
 *
 * The maximum number of events that can be held in the Weave Platform event queue.
 */
#ifndef WEAVE_PLATFORM_CONFIG_MAX_EVENT_QUEUE_SIZE
#define WEAVE_PLATFORM_CONFIG_MAX_EVENT_QUEUE_SIZE CONFIG_MAX_EVENT_QUEUE_SIZE
#endif

/**
 * WEAVE_PLATFORM_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE
 *
 * The size (in bytes) of the service directory cache.
 */
#ifndef WEAVE_PLATFORM_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE
#define WEAVE_PLATFORM_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE CONFIG_SERVICE_DIRECTORY_CACHE_SIZE
#endif

// -------------------- Device Identification Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_DEVICE_VENDOR_ID
 *
 * The Nest-assigned vendor id for the organization responsible for producing the device.
 */
#ifndef WEAVE_PLATFORM_CONFIG_DEVICE_VENDOR_ID
#define WEAVE_PLATFORM_CONFIG_DEVICE_VENDOR_ID CONFIG_DEVICE_VENDOR_ID
#endif

/**
 * WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_ID
 *
 * The unique id assigned by the device vendor to identify the product or device type.  This
 * number is scoped to the device vendor id.
 */
#ifndef WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_ID
#define WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_ID CONFIG_DEVICE_PRODUCT_ID
#endif

/**
 * WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_REVISION
 *
 * The product revision number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software revisions.
 */
#ifndef WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_REVISION
#define WEAVE_PLATFORM_CONFIG_DEVICE_PRODUCT_REVISION CONFIG_DEVICE_PRODUCT_REVISION
#endif

/**
 * WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION
 *
 * A string identifying the firmware revision running on the device.
 */
#ifndef WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION
#define WEAVE_PLATFORM_CONFIG_DEVICE_FIRMWARE_REVISION CONFIG_DEVICE_FIRMWARE_REVISION
#endif

// -------------------- WiFi Station Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_STATION_RECONNECT_INTERVAL
 *
 * The interval at which the Weave platform will attempt to reconnect to the configured WiFi
 * network (in milliseconds).
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_STATION_RECONNECT_INTERVAL
#define WEAVE_PLATFORM_CONFIG_WIFI_STATION_RECONNECT_INTERVAL CONFIG_WIFI_STATION_RECONNECT_INTERVAL
#endif

/**
 * WEAVE_PLATFORM_CONFIG_MAX_SCAN_NETWORKS_RESULTS
 *
 * The maximum number of networks to return as a result of a NetworkProvisioning:ScanNetworks request.
 */
#ifndef WEAVE_PLATFORM_CONFIG_MAX_SCAN_NETWORKS_RESULTS
#define WEAVE_PLATFORM_CONFIG_MAX_SCAN_NETWORKS_RESULTS CONFIG_MAX_SCAN_NETWORKS_RESULTS
#endif

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
 *
 * The amount of time (in milliseconds) after which the Weave platform will timeout a WiFi scan
 * operation that hasn't completed.  A value of 0 will disable the timeout logic.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
#define WEAVE_PLATFORM_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
#endif

// -------------------- WiFi AP Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_AP_SSID_PREFIX
 *
 * A prefix string used in forming the WiFi soft-AP SSID.  The remainder of the SSID
 * consists of the final two bytes of the device's primary WiFi MAC address in hex.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_AP_SSID_PREFIX
#define WEAVE_PLATFORM_CONFIG_WIFI_AP_SSID_PREFIX CONFIG_WIFI_AP_SSID_PREFIX
#endif

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_AP_CHANNEL
 *
 * The WiFi channel number to be used by the soft-AP.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_AP_CHANNEL
#define WEAVE_PLATFORM_CONFIG_WIFI_AP_CHANNEL CONFIG_WIFI_AP_CHANNEL
#endif

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_AP_MAX_STATIONS
 *
 * The maximum number of stations allowed to connect to the soft-AP.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_AP_MAX_STATIONS
#define WEAVE_PLATFORM_CONFIG_WIFI_AP_MAX_STATIONS CONFIG_WIFI_AP_MAX_STATIONS
#endif

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_AP_BEACON_INTERVAL
 *
 * The beacon interval (in milliseconds) for the WiFi soft-AP.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_AP_BEACON_INTERVAL
#define WEAVE_PLATFORM_CONFIG_WIFI_AP_BEACON_INTERVAL CONFIG_WIFI_AP_BEACON_INTERVAL
#endif

/**
 * WEAVE_PLATFORM_CONFIG_WIFI_AP_IDLE_TIMEOUT
 *
 * The amount of time (in milliseconds) after which the Weave platform will deactivate the soft-AP
 * if it has been idle.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WIFI_AP_IDLE_TIMEOUT
#define WEAVE_PLATFORM_CONFIG_WIFI_AP_IDLE_TIMEOUT CONFIG_WIFI_AP_IDLE_TIMEOUT
#endif

// -------------------- Time Sync Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
 *
 * Enables synchronizing the device real-time clock using information returned during
 * a Weave service end point query.  For any device that uses the Weave service directory
 * to lookup a tunnel server, enabling this option will result in the real time clock being
 * synchronized every time the service tunnel is established.
 */
#ifndef WEAVE_PLATFORM_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
#define WEAVE_PLATFORM_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
#endif // WEAVE_PLATFORM_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC

/**
 * WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
 *
 * Enables synchronizing the device's real time clock with a remote Weave Time service
 * using the Weave Time Sync protocol.
 */
#ifndef WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
#define WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
#endif // WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC

/**
 * WEAVE_PLATFORM_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID
 *
 * Specifies the service endpoint id of the Weave Time Sync service to be used to synchronize time.
 *
 * This value is only meaningful if WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_PLATFORM_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID
#define WEAVE_PLATFORM_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID
#endif // WEAVE_PLATFORM_CONFIG_WEAVE_TIME_SERVICE_ENDPOINT_ID

/**
 * WEAVE_PLATFORM_CONFIG_DEFAULT_TIME_SYNC_INTERVAL
 *
 * Specifies the minimum interval (in seconds) at which the device should synchronize its real time
 * clock with the configured Weave Time Sync server.
 *
 * This value is only meaningful if WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_PLATFORM_CONFIG_DEFAULT_TIME_SYNC_INTERVAL
#define WEAVE_PLATFORM_CONFIG_DEFAULT_TIME_SYNC_INTERVAL CONFIG_DEFAULT_TIME_SYNC_INTERVAL
#endif // WEAVE_PLATFORM_CONFIG_DEFAULT_TIME_SYNC_INTERVAL

/**
 * WEAVE_PLATFORM_CONFIG_TIME_SYNC_TIMEOUT
 *
 * Specifies the maximum amount of time (in milliseconds) to wait for a response from a
 * Weave Time Sync server.
 *
 * This value is only meaningful if WEAVE_PLATFORM_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC has
 * been enabled.
 */
#ifndef WEAVE_PLATFORM_CONFIG_TIME_SYNC_TIMEOUT
#define WEAVE_PLATFORM_CONFIG_TIME_SYNC_TIMEOUT CONFIG_TIME_SYNC_TIMEOUT
#endif // WEAVE_PLATFORM_CONFIG_TIME_SYNC_TIMEOUT

// -------------------- Test Configuration --------------------

/**
 * WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
 *
 * Enables the use of a hard-coded default Weave device id and credentials if no device id
 * is found in Weave NV storage.  The value specifies which of 10 identities, numbered 1 through 10,
 * is to be used.  A value of 0 disables use of a default identity.
 *
 * This option is for testing only and should be disabled in production releases.
 */
#ifndef WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
#define WEAVE_PLATFORM_CONFIG_ENABLE_TEST_DEVICE_IDENTITY CONFIG_ENABLE_TEST_DEVICE_IDENTITY
#endif

#endif // WEAVE_PLATFORM_CONFIG_H

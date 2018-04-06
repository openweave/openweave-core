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
 * WEAVE_PLATFORM_CONFIG_MAX_GROUP_KEYS
 *
 * The maximum number of group keys that can be stored on the device.  This count includes
 * root keys, epoch keys, application master keys and the fabric secret.
 */
#ifndef WEAVE_PLATFORM_CONFIG_MAX_GROUP_KEYS
#define WEAVE_PLATFORM_CONFIG_MAX_GROUP_KEYS CONFIG_MAX_GROUP_KEYS
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

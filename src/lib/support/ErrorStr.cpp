/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements functions to translate error codes used
 *      throughout the Weave package into human-readable strings.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdio.h>

#include <SystemLayer/SystemError.h>

#if CONFIG_NETWORK_LAYER_BLE
#include <BleLayer/BleError.h>
#endif // CONFIG_NETWORK_LAYER_BLE

#include <InetLayer/InetError.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>

#include "ASN1Error.h"

namespace nl {

#if WEAVE_CONFIG_SHORT_ERROR_STR

/**
 *  @def ERROR_STR_SIZE
 *
 *  @brief
 *    This defines the size of the buffer to store the formatted error string.
 *
 *    Static buffer to store the short version of error string, which includes
 *    subsystem name and 32-bit error code in hex
 *
 *    At max 22 bytes would be needed according to this calculation/formating:
 *    - 10 characters for subsystem name,
 *    - ':',
 *    - "0x", 8 characters for error code in hex,
 *    - '\0'
 *
 *        An example would be "Weave:0xDEAFBEEF"
 *        - The output string would be truncated if the subsystem name is longer
 *      than 10
 */
#define ERROR_STR_SIZE (10 + 3 + 8 + 1)

/**
 *  @def _SubsystemFormatError(name, err, desc)
 *
 *  @brief
 *    This defines a function that formats the specified error and string
 *    describing the subsystem into a platform- or system-specific manner.
 *
 */
#define _SubsystemFormatError(name, err, desc) SubsystemFormatError(name, err)

#else // #if WEAVE_CONFIG_SHORT_ERROR_STR

/**
 *  @def ERROR_STR_SIZE
 *
 *  @brief
 *    This defines the size of the buffer to store the formatted error string.
 *
 *    Note that the size is arbitrary and probably doesn't have to be this big.
 *    On platforms where 1kB RAM matters, it is probably better to choose the
 *    short version, or don't call ErrorStr at all.
 *
 */
#define ERROR_STR_SIZE (1000)

/**
 *  @def _SubsystemFormatError(name, err, desc)
 *
 *  @brief
 *    This defines a function that formats the specified error and string
 *    describing the subsystem into a platform- or system-specific manner.
 *
 */
#define _SubsystemFormatError(name, err, desc) SubsystemFormatError(name, err, desc)

#endif // #if WEAVE_CONFIG_SHORT_ERROR_STR


/**
 *  @var static char sErrorStr[ERROR_STR_SIZE]
 *
 *  @brief
 *    Static buffer to store the formated error string
 *
 */
static char sErrorStr[ERROR_STR_SIZE];

/**
 * This routine formats the specified error into a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] err    The error for formatting and description.
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *FormatError(int32_t err)
{
#if WEAVE_CONFIG_SHORT_ERROR_STR
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "Error 0x%" PRIx32, err);
#else
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "Error %" PRId32, err);
#endif
    return sErrorStr;
}

/**
 * This routine formats the specified error and string describing the
 * subsystem into the provided format string and returns a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] subsystem  A pointer to a NULL-terminated C string representing
 *                       the package subsystem to which the error corresponds.
 * @param[in] err        The error for formatting and description.
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *SubsystemFormatError(const char *subsystem, int32_t err)
{
    // note that snprintf shall zero-terminate the string as long as sizeof(sErrorStr) > 0
    // note that the return value of snprintf is ignored for an error is very unlikely to happen and we do not care about truncation
#if WEAVE_CONFIG_SHORT_ERROR_STR
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "%s:0x%" PRIx32, subsystem, err);
#else
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "%s Error %" PRId32, subsystem, err);
#endif
    return sErrorStr;
}

#if !(WEAVE_CONFIG_SHORT_ERROR_STR)

/**
 * This routine formats the specified error and string describing the
 * subsystem into the provided format string and returns a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] subsystem    A pointer to a NULL-terminated C string representing
 *                         the package subsystem to which the error corresponds.
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *SubsystemFormatError(const char *subsystem, int32_t err, const char *description)
{
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "%s Error %" PRId32 ": %s", subsystem, err, description);
    return sErrorStr;
}

/**
 * This routine formats the specified error and string describing the
 * System layer into the provided format string and returns a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *SystemLayerFormatError(int32_t err, const char *description)
{
    return SubsystemFormatError("Sys", err, description);
}

/**
 * This routine formats the specified error and string describing the
 * Inet layer into the provided format string and returns a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *InetFormatError(int32_t err, const char *description)
{
    return SubsystemFormatError("Inet", err, description);
}

#if CONFIG_NETWORK_LAYER_BLE
/**
 * This routine formats the specified error and string describing the
 * BLE layer into the provided format string and returns a human-readable
 * NULL-terminated C string describing the provided error.
 *
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *BleFormatError(int32_t err, const char *description)
{
    return SubsystemFormatError("Ble", err, description);
}
#endif // CONFIG_NETWORK_LAYER_BLE

/**
 * This routine formats the specified error and string describing Weave
 * into the provided format string and returns a human-readable NULL-terminated
 * C string describing the provided error.
 *
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *WeaveFormatError(int32_t err, const char *description)
{
    return SubsystemFormatError("Weave", err, description);
}

/**
 * This routine formats the specified error and string describing ASN1
 * into the provided format string and returns a human-readable NULL-terminated
 * C string describing the provided error.
 *
 * @param[in] err          The error for formatting and description.
 * @param[in] description  A pointer to a NULL-terminated C string describing
 *                         the error. Ignored when WEAVE_CONFIG_SHORT_ERROR_STR is defined
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
static const char *ASN1FormatError(int32_t err, const char *description)
{
    return SubsystemFormatError("ASN1", err, description);
}
#endif // #if !(WEAVE_CONFIG_SHORT_ERROR_STR)

/**
 * This routine returns a human-readable NULL-terminated C string
 * describing the provided error.
 *
 * @param[in] err     The error for format and describe.
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
NL_DLL_EXPORT const char *ErrorStr(int32_t err)
{
    switch (err)
    {
    case 0                                                      : return "No Error";

#if !WEAVE_CONFIG_SHORT_ERROR_STR
    // ----- SystemLayer Errors ----
    case WEAVE_SYSTEM_ERROR_NOT_IMPLEMENTED                     : return SystemLayerFormatError(err, "Not implemented");
    case WEAVE_SYSTEM_ERROR_NOT_SUPPORTED                       : return SystemLayerFormatError(err, "Not supported");
    case WEAVE_SYSTEM_ERROR_BAD_ARGS                            : return SystemLayerFormatError(err, "Bad arguments");
    case WEAVE_SYSTEM_ERROR_UNEXPECTED_STATE                    : return SystemLayerFormatError(err, "Unexpected state");
    case WEAVE_SYSTEM_ERROR_UNEXPECTED_EVENT                    : return SystemLayerFormatError(err, "Unexpected event");
    case WEAVE_SYSTEM_ERROR_NO_MEMORY                           : return SystemLayerFormatError(err, "No memory");

    // ----- InetLayer Errors -----
    case INET_ERROR_WRONG_ADDRESS_TYPE                          : return InetFormatError(err, "Wrong address type");
    case INET_ERROR_CONNECTION_ABORTED                          : return InetFormatError(err, "TCP connection aborted");
    case INET_ERROR_PEER_DISCONNECTED                           : return InetFormatError(err, "Peer disconnected");
    case INET_ERROR_INCORRECT_STATE                             : return InetFormatError(err, "Incorrect state");
    case INET_ERROR_MESSAGE_TOO_LONG                            : return InetFormatError(err, "Message too long");
    case INET_ERROR_NO_CONNECTION_HANDLER                       : return InetFormatError(err, "No TCP connection handler");
    case INET_ERROR_NO_MEMORY                                   : return InetFormatError(err, "No memory");
    case INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED                  : return InetFormatError(err, "Outbound message truncated");
    case INET_ERROR_INBOUND_MESSAGE_TOO_BIG                     : return InetFormatError(err, "Inbound message too big");
    case INET_ERROR_HOST_NOT_FOUND                              : return InetFormatError(err, "Host not found");
    case INET_ERROR_DNS_TRY_AGAIN                               : return InetFormatError(err, "DNS try again");
    case INET_ERROR_DNS_NO_RECOVERY                             : return InetFormatError(err, "DNS no recovery");
    case INET_ERROR_BAD_ARGS                                    : return InetFormatError(err, "Bad arguments");
    case INET_ERROR_WRONG_PROTOCOL_TYPE                         : return InetFormatError(err, "Wrong protocol type");
    case INET_ERROR_UNKNOWN_INTERFACE                           : return InetFormatError(err, "Unknown interface");
    case INET_ERROR_NOT_IMPLEMENTED                             : return InetFormatError(err, "Not implemented");
    case INET_ERROR_ADDRESS_NOT_FOUND                           : return InetFormatError(err, "Address not found");
    case INET_ERROR_HOST_NAME_TOO_LONG                          : return InetFormatError(err, "Host name too long");
    case INET_ERROR_INVALID_HOST_NAME                           : return InetFormatError(err, "Invalid host name");
    case INET_ERROR_NOT_SUPPORTED                               : return InetFormatError(err, "Not supported");
    case INET_ERROR_NO_ENDPOINTS                                : return InetFormatError(err, "No more TCP endpoints");
    case INET_ERROR_IDLE_TIMEOUT                                : return InetFormatError(err, "Idle timeout");
    case INET_ERROR_UNEXPECTED_EVENT                            : return InetFormatError(err, "Unexpected event");
    case INET_ERROR_INVALID_IPV6_PKT                            : return InetFormatError(err, "Invalid IPv6 Packet");
    case INET_ERROR_INTERFACE_INIT_FAILURE                      : return InetFormatError(err, "Failure to initialize interface");
    case INET_ERROR_TCP_USER_TIMEOUT                            : return InetFormatError(err, "TCP User Timeout");
    case INET_ERROR_TCP_CONNECT_TIMEOUT                         : return InetFormatError(err, "TCP Connect Timeout");

#if CONFIG_NETWORK_LAYER_BLE
    // ----- BleLayer Errors -----
    case BLE_ERROR_BAD_ARGS                                     : return BleFormatError(err, "Bad arguments");
    case BLE_ERROR_INCORRECT_STATE                              : return BleFormatError(err, "Incorrect state");
    case BLE_ERROR_NO_ENDPOINTS                                 : return BleFormatError(err, "No more BLE endpoints");
    case BLE_ERROR_NO_CONNECTION_RECEIVED_CALLBACK              : return BleFormatError(err, "No Weave over BLE connection received callback set");
    case BLE_ERROR_CENTRAL_UNSUBSCRIBED                         : return BleFormatError(err, "BLE central unsubscribed");
    case BLE_ERROR_GATT_SUBSCRIBE_FAILED                        : return BleFormatError(err, "GATT subscribe operation failed");
    case BLE_ERROR_GATT_UNSUBSCRIBE_FAILED                      : return BleFormatError(err, "GATT unsubscribe operation failed");
    case BLE_ERROR_GATT_WRITE_FAILED                            : return BleFormatError(err, "GATT write characteristic operation failed");
    case BLE_ERROR_GATT_INDICATE_FAILED                         : return BleFormatError(err, "GATT indicate characteristic operation failed");
    case BLE_ERROR_NOT_IMPLEMENTED                              : return BleFormatError(err, "Not implemented");
    case BLE_ERROR_INVALID_ROLE                                 : return BleFormatError(err, "Invalid BLE device role specified");
    case BLE_ERROR_WOBLE_PROTOCOL_ABORT                         : return BleFormatError(err, "BLE transport protocol fired abort");
    case BLE_ERROR_REMOTE_DEVICE_DISCONNECTED                   : return BleFormatError(err, "Remote device closed BLE connection");
    case BLE_ERROR_APP_CLOSED_CONNECTION                        : return BleFormatError(err, "Application closed BLE connection");
    case BLE_ERROR_OUTBOUND_MESSAGE_TOO_BIG                     : return BleFormatError(err, "Outbound message too big");
    case BLE_ERROR_NOT_WEAVE_DEVICE                             : return BleFormatError(err, "BLE device doesn't seem to support Weave");
    case BLE_ERROR_INCOMPATIBLE_PROTOCOL_VERSIONS               : return BleFormatError(err, "Incompatible BLE transport protocol versions");
    case BLE_ERROR_NO_MEMORY                                    : return BleFormatError(err, "No memory");
    case BLE_ERROR_MESSAGE_INCOMPLETE                           : return BleFormatError(err, "Message incomplete");
    case BLE_ERROR_INVALID_FRAGMENT_SIZE                        : return BleFormatError(err, "Invalid fragment size");
    case BLE_ERROR_START_TIMER_FAILED                           : return BleFormatError(err, "Start timer failed");
    case BLE_ERROR_CONNECT_TIMED_OUT                            : return BleFormatError(err, "Connect handshake timed out");
    case BLE_ERROR_RECEIVE_TIMED_OUT                            : return BleFormatError(err, "Receive handshake timed out");
    case BLE_ERROR_INVALID_MESSAGE                              : return BleFormatError(err, "Invalid message");
    case BLE_ERROR_FRAGMENT_ACK_TIMED_OUT                       : return BleFormatError(err, "Message fragment acknowledgement timed out");
    case BLE_ERROR_KEEP_ALIVE_TIMED_OUT                         : return BleFormatError(err, "Keep-alive receipt timed out");
    case BLE_ERRROR_NO_CONNECT_COMPLETE_CALLBACK                : return BleFormatError(err, "Missing required callback");
    case BLE_ERROR_INVALID_ACK                                  : return BleFormatError(err, "Received invalid BLE transport protocol fragment acknowledgement");
    case BLE_ERROR_REASSEMBLER_MISSING_DATA                     : return BleFormatError(err, "BLE message reassembler did not receive enough data");
    case BLE_ERROR_INVALID_BTP_HEADER_FLAGS                     : return BleFormatError(err, "Received invalid BLE transport protocol header flags");
    case BLE_ERROR_INVALID_BTP_SEQUENCE_NUMBER                  : return BleFormatError(err, "Received invalid BLE transport protocol sequence number");
    case BLE_ERROR_REASSEMBLER_INCORRECT_STATE                  : return BleFormatError(err, "BLE message reassembler received packet in incorrect state");
    case BLE_ERROR_RECEIVED_MESSAGE_TOO_BIG                     : return BleFormatError(err, "Message received by BLE message reassembler was too large");
#endif // CONFIG_NETWORK_LAYER_BLE

    // ----- Weave Errors -----
    case WEAVE_ERROR_TOO_MANY_CONNECTIONS                       : return WeaveFormatError(err, "Too many connections");
    case WEAVE_ERROR_SENDING_BLOCKED                            : return WeaveFormatError(err, "Sending blocked");
    case WEAVE_ERROR_CONNECTION_ABORTED                         : return WeaveFormatError(err, "Connection aborted");
    case WEAVE_ERROR_INCORRECT_STATE                            : return WeaveFormatError(err, "Incorrect state");
    case WEAVE_ERROR_MESSAGE_TOO_LONG                           : return WeaveFormatError(err, "Message too long");
    case WEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION               : return WeaveFormatError(err, "Unsupported exchange version");
    case WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS      : return WeaveFormatError(err, "Too many unsolicited message handlers");
    case WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER             : return WeaveFormatError(err, "No unsolicited message handler");
    case WEAVE_ERROR_NO_CONNECTION_HANDLER                      : return WeaveFormatError(err, "No connection handler");
    case WEAVE_ERROR_TOO_MANY_PEER_NODES                        : return WeaveFormatError(err, "Too many peer nodes");
    case WEAVE_ERROR_NO_MEMORY                                  : return WeaveFormatError(err, "No memory");
    case WEAVE_ERROR_NO_MESSAGE_HANDLER                         : return WeaveFormatError(err, "No message handler");
    case WEAVE_ERROR_MESSAGE_INCOMPLETE                         : return WeaveFormatError(err, "Message incomplete");
    case WEAVE_ERROR_DATA_NOT_ALIGNED                           : return WeaveFormatError(err, "Data not aligned");
    case WEAVE_ERROR_UNKNOWN_KEY_TYPE                           : return WeaveFormatError(err, "Unknown key type");
    case WEAVE_ERROR_KEY_NOT_FOUND                              : return WeaveFormatError(err, "Key not found");
    case WEAVE_ERROR_WRONG_ENCRYPTION_TYPE                      : return WeaveFormatError(err, "Wrong encryption type");
    case WEAVE_ERROR_TOO_MANY_KEYS                              : return WeaveFormatError(err, "Too many keys");
    case WEAVE_ERROR_INTEGRITY_CHECK_FAILED                     : return WeaveFormatError(err, "Integrity check failed");
    case WEAVE_ERROR_INVALID_SIGNATURE                          : return WeaveFormatError(err, "Invalid signature");
    case WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION                : return WeaveFormatError(err, "Unsupported message version");
    case WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE                : return WeaveFormatError(err, "Unsupported encryption type");
    case WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE                 : return WeaveFormatError(err, "Unsupported signature type");
    case WEAVE_ERROR_INVALID_MESSAGE_LENGTH                     : return WeaveFormatError(err, "Invalid message length");
    case WEAVE_ERROR_BUFFER_TOO_SMALL                           : return WeaveFormatError(err, "Buffer too small");
    case WEAVE_ERROR_DUPLICATE_KEY_ID                           : return WeaveFormatError(err, "Duplicate key id");
    case WEAVE_ERROR_WRONG_KEY_TYPE                             : return WeaveFormatError(err, "Wrong key type");
    case WEAVE_ERROR_WELL_UNINITIALIZED                         : return WeaveFormatError(err, "Well uninitialized");
    case WEAVE_ERROR_WELL_EMPTY                                 : return WeaveFormatError(err, "Well empty");
    case WEAVE_ERROR_INVALID_STRING_LENGTH                      : return WeaveFormatError(err, "Invalid string length");
    case WEAVE_ERROR_INVALID_LIST_LENGTH                        : return WeaveFormatError(err, "invalid list length");
    case WEAVE_ERROR_INVALID_INTEGRITY_TYPE                     : return WeaveFormatError(err, "Invalid integrity type");
    case WEAVE_END_OF_TLV                                       : return WeaveFormatError(err, "End of TLV");
    case WEAVE_ERROR_TLV_UNDERRUN                               : return WeaveFormatError(err, "TLV underrun");
    case WEAVE_ERROR_INVALID_TLV_ELEMENT                        : return WeaveFormatError(err, "Invalid TLV element");
    case WEAVE_ERROR_INVALID_TLV_TAG                            : return WeaveFormatError(err, "Invalid TLV tag");
    case WEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG                   : return WeaveFormatError(err, "Unknown implicit TLV tag");
    case WEAVE_ERROR_WRONG_TLV_TYPE                             : return WeaveFormatError(err, "Wrong TLV type");
    case WEAVE_ERROR_TLV_CONTAINER_OPEN                         : return WeaveFormatError(err, "TLV container open");
    case WEAVE_ERROR_INVALID_TRANSFER_MODE                      : return WeaveFormatError(err, "Invalid transfer mode");
    case WEAVE_ERROR_INVALID_PROFILE_ID                         : return WeaveFormatError(err, "Invalid profile id");
    case WEAVE_ERROR_INVALID_MESSAGE_TYPE                       : return WeaveFormatError(err, "Invalid message type");
    case WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT                     : return WeaveFormatError(err, "Unexpected TLV element");
    case WEAVE_ERROR_STATUS_REPORT_RECEIVED                     : return WeaveFormatError(err, "Status Report received from peer");
    case WEAVE_ERROR_NOT_IMPLEMENTED                            : return WeaveFormatError(err, "Not Implemented");
    case WEAVE_ERROR_INVALID_ADDRESS                            : return WeaveFormatError(err, "Invalid address");
    case WEAVE_ERROR_INVALID_ARGUMENT                           : return WeaveFormatError(err, "Invalid argument");
    case WEAVE_ERROR_TLV_TAG_NOT_FOUND                          : return WeaveFormatError(err, "TLV tag not found");

    case WEAVE_ERROR_INVALID_PATH_LIST                          : return WeaveFormatError(err, "Invalid TLV path list");
    case WEAVE_ERROR_INVALID_DATA_LIST                          : return WeaveFormatError(err, "Invalid TLV data list");
    case WEAVE_ERROR_TRANSACTION_CANCELED                       : return WeaveFormatError(err, "Transaction canceled");
    case WEAVE_ERROR_LISTENER_ALREADY_STARTED                   : return WeaveFormatError(err, "Listener already started");
    case WEAVE_ERROR_LISTENER_ALREADY_STOPPED                   : return WeaveFormatError(err, "Listener already stopped");
    case WEAVE_ERROR_UNKNOWN_TOPIC                              : return WeaveFormatError(err, "Unknown Topic");

    case WEAVE_ERROR_TIMEOUT                                    : return WeaveFormatError(err, "Timeout");
    case WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR                  : return WeaveFormatError(err, "Invalid device descriptor");
    case WEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION      : return WeaveFormatError(err, "Unsupported device descriptor version");
    case WEAVE_END_OF_INPUT                                     : return WeaveFormatError(err, "End of input");
    case WEAVE_ERROR_RATE_LIMIT_EXCEEDED                        : return WeaveFormatError(err, "Rate limit exceeded");
    case WEAVE_ERROR_SECURITY_MANAGER_BUSY                      : return WeaveFormatError(err, "Security manager busy");
    case WEAVE_ERROR_INVALID_PASE_PARAMETER                     : return WeaveFormatError(err, "Invalid PASE parameter");
    case WEAVE_ERROR_PASE_SUPPORTS_ONLY_CONFIG1                 : return WeaveFormatError(err, "PASE supports only Config1");
    case WEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS              : return WeaveFormatError(err, "No supported PASE configurations in common");
    case WEAVE_ERROR_INVALID_PASE_CONFIGURATION                 : return WeaveFormatError(err, "Invalid PASE configuration");
    case WEAVE_ERROR_KEY_CONFIRMATION_FAILED                    : return WeaveFormatError(err, "Key confirmation failed");
    case WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY                 : return WeaveFormatError(err, "Invalid use of session key");
    case WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY             : return WeaveFormatError(err, "Connection closed unexpectedly");
    case WEAVE_ERROR_MISSING_TLV_ELEMENT                        : return WeaveFormatError(err, "Missing TLV element");
    case WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE                    : return WeaveFormatError(err, "Random data unavailable");
    case WEAVE_ERROR_UNSUPPORTED_HOST_PORT_ELEMENT              : return WeaveFormatError(err, "Unsupported type in host/port list");
    case WEAVE_ERROR_INVALID_HOST_SUFFIX_INDEX                  : return WeaveFormatError(err, "Invalid suffix index in host/port list");
    case WEAVE_ERROR_HOST_PORT_LIST_EMPTY                       : return WeaveFormatError(err, "Host/port empty");
    case WEAVE_ERROR_UNSUPPORTED_AUTH_MODE                      : return WeaveFormatError(err, "Unsupported authentication mode");

    case WEAVE_ERROR_INVALID_SERVICE_EP                         : return WeaveFormatError(err, "Invalid service endpoint");
    case WEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE               : return WeaveFormatError(err, "Invalid directory entry type");
    case WEAVE_ERROR_FORCED_RESET                               : return WeaveFormatError(err, "Service manager forced reset");
    case WEAVE_ERROR_NO_ENDPOINT                                : return WeaveFormatError(err, "No endpoint was available to send the message");
    case WEAVE_ERROR_INVALID_DESTINATION_NODE_ID                : return WeaveFormatError(err, "Invalid destination node id");
    case WEAVE_ERROR_NOT_CONNECTED                              : return WeaveFormatError(err, "Not connected");
    case WEAVE_ERROR_NO_SW_UPDATE_AVAILABLE                     : return WeaveFormatError(err, "No SW update available");

    case WEAVE_ERROR_CA_CERT_NOT_FOUND                          : return WeaveFormatError(err, "CA certificate not found");
    case WEAVE_ERROR_CERT_PATH_LEN_CONSTRAINT_EXCEEDED          : return WeaveFormatError(err, "Certificate path length constraint exceeded");
    case WEAVE_ERROR_CERT_PATH_TOO_LONG                         : return WeaveFormatError(err, "Certificate path too long");
    case WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED                     : return WeaveFormatError(err, "Requested certificate usage is not allowed");
    case WEAVE_ERROR_CERT_EXPIRED                               : return WeaveFormatError(err, "Certificate expired");
    case WEAVE_ERROR_CERT_NOT_VALID_YET                         : return WeaveFormatError(err, "Certificate not yet valid");
    case WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT                    : return WeaveFormatError(err, "Unsupported certificate format");
    case WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE                 : return WeaveFormatError(err, "Unsupported elliptic curve");
    case WEAVE_CERT_NOT_USED                                    : return WeaveFormatError(err, "Certificate was not used in chain validation");
    case WEAVE_ERROR_CERT_NOT_FOUND                             : return WeaveFormatError(err, "Certificate not found");
    case WEAVE_ERROR_INVALID_CASE_PARAMETER                     : return WeaveFormatError(err, "Invalid CASE parameter");
    case WEAVE_ERROR_UNSUPPORTED_CASE_CONFIGURATION             : return WeaveFormatError(err, "Unsupported CASE configuration");
    case WEAVE_ERROR_CERT_LOAD_FAIL                             : return WeaveFormatError(err, "Unable to load certificate");
    case WEAVE_ERROR_CERT_NOT_TRUSTED                           : return WeaveFormatError(err, "Certificate not trusted");
    case WEAVE_ERROR_INVALID_ACCESS_TOKEN                       : return WeaveFormatError(err, "Invalid access token");
    case WEAVE_ERROR_WRONG_CERT_SUBJECT                         : return WeaveFormatError(err, "Wrong certificate subject");
    case WEAVE_ERROR_WRONG_NODE_ID                              : return WeaveFormatError(err, "Wrong node ID");
    case WEAVE_ERROR_CONN_ACCEPTED_ON_WRONG_PORT                : return WeaveFormatError(err, "Connection accepted on wrong port");
    case WEAVE_ERROR_CALLBACK_REPLACED                          : return WeaveFormatError(err, "Application callback replaced");
    case WEAVE_ERROR_NO_CASE_AUTH_DELEGATE                      : return WeaveFormatError(err, "No CASE auth delegate set");
    case WEAVE_ERROR_DEVICE_LOCATE_TIMEOUT                      : return WeaveFormatError(err, "Timeout attempting to locate device");
    case WEAVE_ERROR_DEVICE_CONNECT_TIMEOUT                     : return WeaveFormatError(err, "Timeout connecting to device");
    case WEAVE_ERROR_DEVICE_AUTH_TIMEOUT                        : return WeaveFormatError(err, "Timeout authenticating device");
    case WEAVE_ERROR_MESSAGE_NOT_ACKNOWLEDGED                   : return WeaveFormatError(err, "Message not acknowledged after max retries");
    case WEAVE_ERROR_RETRANS_TABLE_FULL                         : return WeaveFormatError(err, "Retransmit Table is already full");
    case WEAVE_ERROR_INVALID_ACK_ID                             : return WeaveFormatError(err, "Invalid Acknowledgment Id");
    case WEAVE_ERROR_SEND_THROTTLED                             : return WeaveFormatError(err, "Sending to peer is throttled on this Exchange");
    case WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE             : return WeaveFormatError(err, "Message version not supported by current exchange context");
    case WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE                  : return WeaveFormatError(err, "Required feature not supported by this configuration");
    case WEAVE_ERROR_UNSOLICITED_MSG_NO_ORIGINATOR              : return WeaveFormatError(err, "Unsolicited msg with originator bit clear");
    case WEAVE_ERROR_UNSUPPORTED_TUNNEL_VERSION                 : return WeaveFormatError(err, "Unsupported Tunnel version");
    case WEAVE_ERROR_INVALID_FABRIC_ID                          : return WeaveFormatError(err, "Invalid Fabric Id");
    case WEAVE_ERROR_TUNNEL_NEXTHOP_TABLE_FULL                  : return WeaveFormatError(err, "Local tunnel nexthop table full");
    case WEAVE_ERROR_TUNNEL_SERVICE_QUEUE_FULL                  : return WeaveFormatError(err, "Service queue full");
    case WEAVE_ERROR_TUNNEL_PEER_ENTRY_NOT_FOUND                : return WeaveFormatError(err, "Shortcut Tunnel peer entry not found");
    case WEAVE_ERROR_TUNNEL_FORCE_ABORT                         : return WeaveFormatError(err, "Forced Tunnel Abort.");
    case WEAVE_ERROR_DRBG_ENTROPY_SOURCE_FAILED                 : return WeaveFormatError(err, "DRBG entropy source failed to generate entropy data");
    case WEAVE_ERROR_NO_TAKE_AUTH_DELEGATE                      : return WeaveFormatError(err, "No TAKE auth delegate set");
    case WEAVE_ERROR_TAKE_RECONFIGURE_REQUIRED                  : return WeaveFormatError(err, "TAKE requires a reconfigure");
    case WEAVE_ERROR_TAKE_REAUTH_POSSIBLE                       : return WeaveFormatError(err, "TAKE can do a reauthentication");
    case WEAVE_ERROR_INVALID_TAKE_PARAMETER                     : return WeaveFormatError(err, "TAKE received an invalid parameter");
    case WEAVE_ERROR_UNSUPPORTED_TAKE_CONFIGURATION             : return WeaveFormatError(err, "TAKE Unsupported configuration");
    case WEAVE_ERROR_TAKE_TOKEN_IDENTIFICATION_FAILED           : return WeaveFormatError(err, "TAKE token identification failed");
    case WEAVE_ERROR_INVALID_TOKENPAIRINGBUNDLE                 : return WeaveFormatError(err, "Invalid Token Pairing Bundle");
    case WEAVE_ERROR_UNSUPPORTED_TOKENPAIRINGBUNDLE_VERSION     : return WeaveFormatError(err, "Unsupported Token Pairing Bundle version");
    case WEAVE_ERROR_KEY_NOT_FOUND_FROM_PEER                    : return WeaveFormatError(err, "Key not found error code received from peer");
    case WEAVE_ERROR_WRONG_ENCRYPTION_TYPE_FROM_PEER            : return WeaveFormatError(err, "Wrong encryption type error code received from peer");
    case WEAVE_ERROR_UNKNOWN_KEY_TYPE_FROM_PEER                 : return WeaveFormatError(err, "Unknown key type error code received from peer");
    case WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY_FROM_PEER       : return WeaveFormatError(err, "Invalid use of session key error code received from peer");
    case WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE_FROM_PEER      : return WeaveFormatError(err, "Unsupported encryption type error code received from peer");
    case WEAVE_ERROR_INTERNAL_KEY_ERROR_FROM_PEER               : return WeaveFormatError(err, "Internal key error code received from peer");
    case WEAVE_ERROR_INVALID_KEY_ID                             : return WeaveFormatError(err, "Invalid key identifier");
    case WEAVE_ERROR_INVALID_TIME                               : return WeaveFormatError(err, "Valid time value is not available");
    case WEAVE_ERROR_LOCKING_FAILURE                            : return WeaveFormatError(err, "Failure to lock/unlock OS-provided lock");
    case WEAVE_ERROR_UNSUPPORTED_PASSCODE_CONFIG                : return WeaveFormatError(err, "Unsupported passcode encryption configuration.");
    case WEAVE_ERROR_PASSCODE_AUTHENTICATION_FAILED             : return WeaveFormatError(err, "Passcode authentication failed.");
    case WEAVE_ERROR_PASSCODE_FINGERPRINT_FAILED                : return WeaveFormatError(err, "Passcode fingerprint failed.");
    case WEAVE_ERROR_SERIALIZATION_ELEMENT_NULL                 : return WeaveFormatError(err, "Element requested is null.");
    case WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM             : return WeaveFormatError(err, "Certificate not signed with required signature algorithm");
    case WEAVE_ERROR_WRONG_WEAVE_SIGNATURE_ALGORITHM            : return WeaveFormatError(err, "Weave signature not signed with required signature algorithm");
    case WEAVE_ERROR_WDM_SCHEMA_MISMATCH                        : return WeaveFormatError(err, "Schema mismatch in WDM.");
    case WEAVE_ERROR_INVALID_INTEGER_VALUE                      : return WeaveFormatError(err, "Invalid integer value.");
    case WEAVE_ERROR_TOO_MANY_CASE_RECONFIGURATIONS             : return WeaveFormatError(err, "Too many CASE reconfigurations were received.");
    case WEAVE_ERROR_INVALID_MESSAGE_FLAG                       : return WeaveFormatError(err, "Invalid message flag.");
    case WEAVE_ERROR_NO_COMMON_KEY_EXPORT_CONFIGURATIONS        : return WeaveFormatError(err, "No supported key export protocol configurations in common");
    case WEAVE_ERROR_INVALID_KEY_EXPORT_CONFIGURATION           : return WeaveFormatError(err, "Invalid key export protocol configuration");
    case WEAVE_ERROR_NO_KEY_EXPORT_DELEGATE                     : return WeaveFormatError(err, "No key export protocol delegate set");
    case WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST            : return WeaveFormatError(err, "Unauthorized key export request");
    case WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE           : return WeaveFormatError(err, "Unauthorized key export response");
    case WEAVE_ERROR_EXPORTED_KEY_AUTHENTICATION_FAILED         : return WeaveFormatError(err, "Exported key authentication failed");
    case WEAVE_ERROR_TOO_MANY_SHARED_SESSION_END_NODES          : return WeaveFormatError(err, "Too many shared session end nodes");
    case WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT                 : return WeaveFormatError(err, "Malformed WDM DataElement");
    case WEAVE_ERROR_WRONG_CERT_TYPE                            : return WeaveFormatError(err, "Wrong certificate type");
    case WEAVE_ERROR_TIME_NOT_SYNCED_YET                        : return WeaveFormatError(err, "Time not synced yet");
    case WEAVE_ERROR_UNSUPPORTED_CLOCK                          : return WeaveFormatError(err, "Unsupported clock");
    case WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED           : return WeaveFormatError(err, "Default event handler not called");
    case WEAVE_ERROR_PERSISTED_STORAGE_FAIL                     : return WeaveFormatError(err, "Persisted storage failed");
    case WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND          : return WeaveFormatError(err, "Value not found in the persisted storage");
    case WEAVE_ERROR_PROFILE_STRING_CONTEXT_ALREADY_REGISTERED  : return WeaveFormatError(err, "String context already registered");
    case WEAVE_ERROR_PROFILE_STRING_CONTEXT_NOT_REGISTERED      : return WeaveFormatError(err, "String context not registered");
    case WEAVE_ERROR_INCOMPATIBLE_SCHEMA_VERSION                : return WeaveFormatError(err, "Incompatible data schema version");

    // ----- ASN1 Errors -----
    case ASN1_END                                               : return ASN1FormatError(err, "End of input");
    case ASN1_ERROR_UNDERRUN                                    : return ASN1FormatError(err, "Reader underrun");
    case ASN1_ERROR_OVERFLOW                                    : return ASN1FormatError(err, "Writer overflow");
    case ASN1_ERROR_INVALID_STATE                               : return ASN1FormatError(err, "Invalid state");
    case ASN1_ERROR_MAX_DEPTH_EXCEEDED                          : return ASN1FormatError(err, "Max depth exceeded");
    case ASN1_ERROR_INVALID_ENCODING                            : return ASN1FormatError(err, "Invalid encoding");
    case ASN1_ERROR_UNSUPPORTED_ENCODING                        : return ASN1FormatError(err, "Unsupported encoding");
    case ASN1_ERROR_TAG_OVERFLOW                                : return ASN1FormatError(err, "Tag overflow");
    case ASN1_ERROR_LENGTH_OVERFLOW                             : return ASN1FormatError(err, "Length overflow");
    case ASN1_ERROR_VALUE_OVERFLOW                              : return ASN1FormatError(err, "Value overflow");
    case ASN1_ERROR_UNKNOWN_OBJECT_ID                           : return ASN1FormatError(err, "Unknown object id");
#endif // if !WEAVE_CONFIG_SHORT_ERROR_STR

    }

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    // Handle any Weave System Layer LwIP stack-specific errors.
    // description is ignored, and the actual printout is different when WEAVE_CONFIG_SHORT_ERROR_STR is defined
    if (Weave::System::IsErrorLwIP(static_cast<Weave::System::Error>(err)))
        return _SubsystemFormatError("LwIP", err, Weave::System::DescribeErrorLwIP(static_cast<Weave::System::Error>(err)));
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
    // Handle any Weave System Layer POSIX stack-specific errors.
    // description is ignored, and the actual printout is different when WEAVE_CONFIG_SHORT_ERROR_STR is defined
    if (Weave::System::IsErrorPOSIX(static_cast<Weave::System::Error>(err)))
        return _SubsystemFormatError("OS", err, Weave::System::DescribeErrorPOSIX(static_cast<Weave::System::Error>(err)));
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

    // Handle any default cases not explicitly handled in the switch
    // statement above.
    // Note this is the only case, and the actual printout is different when WEAVE_CONFIG_SHORT_ERROR_STR is defined

    if (err >= INET_ERROR_MIN && err <= INET_ERROR_MAX)
        return SubsystemFormatError("Inet", (int)err);

    if (err >= WEAVE_ERROR_MIN && err <= WEAVE_ERROR_MAX)
        return SubsystemFormatError("Weave", (int)err);

    if (err >= ASN1_ERROR_MIN && err <= ASN1_ERROR_MAX)
        return SubsystemFormatError("ASN1", (int)err);

#if CONFIG_NETWORK_LAYER_BLE
    if (err >= BLE_ERROR_MIN && err <= BLE_ERROR_MAX)
        return SubsystemFormatError("Ble", (int)err);
#endif // CONFIG_NETWORK_LAYER_BLE

    // note the actual printout is different when WEAVE_CONFIG_SHORT_ERROR_STR is defined
    return FormatError((int)err);
}

} // namespace nl

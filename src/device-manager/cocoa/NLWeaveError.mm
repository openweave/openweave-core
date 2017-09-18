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
 *      Translation of Weave errors into native Objective C types.
 *
 */

#import "NLWeaveError_Protected.h"
#import "NLWeaveDeviceManager.h"
#import "NLWeaveASN1ErrorCodes.h"
#import "NLWeaveInetErrorCodes.h"
#import "NLWeaveErrorCodes.h"

#include <InetLayer/InetError.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Support/ASN1Error.h>

@implementation NLWeaveError

- (id)initWithWeaveError:(WEAVE_ERROR)weaveError report:(NSString*)report
{
    if (self = [super init])
    {
        _errorCode = (NSInteger)weaveError;
        _errorReport = report;
    }
    return self;

}

- (NSString *)description
{
    NSString *emptyErrorReport = [NSString stringWithFormat:@"No Error information available. errorCode = %ld (0x%lx)", (long)self.errorCode, (long)self.errorCode];

    return _errorReport ? ([NSString stringWithFormat:@"%@ - errorCode: %ld", _errorReport, (long)self.errorCode]) : emptyErrorReport;
}

- (NSInteger)translateErrorCode
{
	switch (self.errorCode)
    {
    case 0                                                      : return NLWEAVE_NO_ERROR;

    // ----- InetLayer Errors -----
    case INET_ERROR_WRONG_ADDRESS_TYPE                          : return NLINET_ERROR_WRONG_ADDRESS_TYPE;
    case INET_ERROR_CONNECTION_ABORTED                          : return NLINET_ERROR_CONNECTION_ABORTED;
    case INET_ERROR_PEER_DISCONNECTED                           : return NLINET_ERROR_PEER_DISCONNECTED;
    case INET_ERROR_INCORRECT_STATE                             : return NLINET_ERROR_INCORRECT_STATE;
    case INET_ERROR_MESSAGE_TOO_LONG                            : return NLINET_ERROR_MESSAGE_TOO_LONG;
    case INET_ERROR_NO_CONNECTION_HANDLER                       : return NLINET_ERROR_NO_CONNECTION_HANDLER;
    case INET_ERROR_NO_MEMORY                                   : return NLINET_ERROR_NO_MEMORY;
    case INET_ERROR_OUTBOUND_MESSAGE_TRUNCATED                  : return NLINET_ERROR_OUTBOUND_MESSAGE_TRUNCATED;
    case INET_ERROR_INBOUND_MESSAGE_TOO_BIG                     : return NLINET_ERROR_INBOUND_MESSAGE_TOO_BIG;
    case INET_ERROR_HOST_NOT_FOUND                              : return NLINET_ERROR_HOST_NOT_FOUND;
    case INET_ERROR_DNS_TRY_AGAIN                               : return NLINET_ERROR_DNS_TRY_AGAIN;
    case INET_ERROR_DNS_NO_RECOVERY                             : return NLINET_ERROR_DNS_NO_RECOVERY;
    case INET_ERROR_BAD_ARGS                                    : return NLINET_ERROR_BAD_ARGS;
    case INET_ERROR_WRONG_PROTOCOL_TYPE                         : return NLINET_ERROR_WRONG_PROTOCOL_TYPE;
    case INET_ERROR_UNKNOWN_INTERFACE                           : return NLINET_ERROR_UNKNOWN_INTERFACE;
    case INET_ERROR_NOT_IMPLEMENTED                             : return NLINET_ERROR_NOT_IMPLEMENTED;
    case INET_ERROR_ADDRESS_NOT_FOUND                           : return NLINET_ERROR_ADDRESS_NOT_FOUND;
    case INET_ERROR_HOST_NAME_TOO_LONG                          : return NLINET_ERROR_HOST_NAME_TOO_LONG;
    case INET_ERROR_INVALID_HOST_NAME                           : return NLINET_ERROR_INVALID_HOST_NAME;

    // ----- Weave Errors -----
    case WEAVE_ERROR_TOO_MANY_CONNECTIONS                       : return NLWEAVE_ERROR_TOO_MANY_CONNECTIONS;
    case WEAVE_ERROR_SENDING_BLOCKED                            : return NLWEAVE_ERROR_SENDING_BLOCKED;
    case WEAVE_ERROR_CONNECTION_ABORTED                         : return NLWEAVE_ERROR_CONNECTION_ABORTED;
    case WEAVE_ERROR_INCORRECT_STATE                            : return NLWEAVE_ERROR_INCORRECT_STATE;
    case WEAVE_ERROR_MESSAGE_TOO_LONG                           : return NLWEAVE_ERROR_MESSAGE_TOO_LONG;
    case WEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION               : return NLWEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION;
    case WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS      : return NLWEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS;
    case WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER             : return NLWEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER;
    case WEAVE_ERROR_NO_CONNECTION_HANDLER                      : return NLWEAVE_ERROR_NO_CONNECTION_HANDLER;
    case WEAVE_ERROR_TOO_MANY_PEER_NODES                        : return NLWEAVE_ERROR_TOO_MANY_PEER_NODES;
    case WEAVE_ERROR_NO_MEMORY                                  : return NLWEAVE_ERROR_NO_MEMORY;
    case WEAVE_ERROR_NO_MESSAGE_HANDLER                         : return NLWEAVE_ERROR_NO_MESSAGE_HANDLER;
    case WEAVE_ERROR_MESSAGE_INCOMPLETE                         : return NLWEAVE_ERROR_MESSAGE_INCOMPLETE;
    case WEAVE_ERROR_DATA_NOT_ALIGNED                           : return NLWEAVE_ERROR_DATA_NOT_ALIGNED;
    case WEAVE_ERROR_UNKNOWN_KEY_TYPE                           : return NLWEAVE_ERROR_UNKNOWN_KEY_TYPE;
    case WEAVE_ERROR_KEY_NOT_FOUND                              : return NLWEAVE_ERROR_KEY_NOT_FOUND;
    case WEAVE_ERROR_WRONG_ENCRYPTION_TYPE                      : return NLWEAVE_ERROR_WRONG_ENCRYPTION_TYPE;
    case WEAVE_ERROR_TOO_MANY_KEYS                              : return NLWEAVE_ERROR_TOO_MANY_KEYS;
    case WEAVE_ERROR_INTEGRITY_CHECK_FAILED                     : return NLWEAVE_ERROR_INTEGRITY_CHECK_FAILED;
    case WEAVE_ERROR_INVALID_SIGNATURE                          : return NLWEAVE_ERROR_INVALID_SIGNATURE;
    case WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION                : return NLWEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION;
    case WEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE                : return NLWEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE;
    case WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE                 : return NLWEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE;
    case WEAVE_ERROR_INVALID_MESSAGE_LENGTH                     : return NLWEAVE_ERROR_INVALID_MESSAGE_LENGTH;
    case WEAVE_ERROR_BUFFER_TOO_SMALL                           : return NLWEAVE_ERROR_BUFFER_TOO_SMALL;
    case WEAVE_ERROR_DUPLICATE_KEY_ID                           : return NLWEAVE_ERROR_DUPLICATE_KEY_ID;
    case WEAVE_ERROR_WRONG_KEY_TYPE                             : return NLWEAVE_ERROR_WRONG_KEY_TYPE;
    case WEAVE_ERROR_WELL_UNINITIALIZED                         : return NLWEAVE_ERROR_WELL_UNINITIALIZED;
    case WEAVE_ERROR_WELL_EMPTY                                 : return NLWEAVE_ERROR_WELL_EMPTY;
    case WEAVE_ERROR_INVALID_STRING_LENGTH                      : return NLWEAVE_ERROR_INVALID_STRING_LENGTH;
    case WEAVE_ERROR_INVALID_LIST_LENGTH                        : return NLWEAVE_ERROR_INVALID_LIST_LENGTH;
    case WEAVE_ERROR_INVALID_INTEGRITY_TYPE                     : return NLWEAVE_ERROR_INVALID_INTEGRITY_TYPE;
    case WEAVE_END_OF_TLV                                       : return NLWEAVE_END_OF_TLV;
    case WEAVE_ERROR_TLV_UNDERRUN                               : return NLWEAVE_ERROR_TLV_UNDERRUN;
    case WEAVE_ERROR_INVALID_TLV_ELEMENT                        : return NLWEAVE_ERROR_INVALID_TLV_ELEMENT;
    case WEAVE_ERROR_INVALID_TLV_TAG                            : return NLWEAVE_ERROR_INVALID_TLV_TAG;
    case WEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG                   : return NLWEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG;
    case WEAVE_ERROR_WRONG_TLV_TYPE                             : return NLWEAVE_ERROR_WRONG_TLV_TYPE;
    case WEAVE_ERROR_TLV_CONTAINER_OPEN                         : return NLWEAVE_ERROR_TLV_CONTAINER_OPEN;
    case WEAVE_ERROR_INVALID_TRANSFER_MODE                      : return NLWEAVE_ERROR_INVALID_TRANSFER_MODE;
    case WEAVE_ERROR_INVALID_PROFILE_ID                         : return NLWEAVE_ERROR_INVALID_PROFILE_ID;
    case WEAVE_ERROR_INVALID_MESSAGE_TYPE                       : return NLWEAVE_ERROR_INVALID_MESSAGE_TYPE;
    case WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT                     : return NLWEAVE_ERROR_UNEXPECTED_TLV_ELEMENT;
    case WEAVE_ERROR_STATUS_REPORT_RECEIVED                     : return NLWEAVE_ERROR_STATUS_REPORT_RECEIVED;
    case WEAVE_ERROR_NOT_IMPLEMENTED                            : return NLWEAVE_ERROR_NOT_IMPLEMENTED;
    case WEAVE_ERROR_INVALID_ADDRESS                            : return NLWEAVE_ERROR_INVALID_ADDRESS;
    case WEAVE_ERROR_INVALID_ARGUMENT                           : return NLWEAVE_ERROR_INVALID_ARGUMENT;

    case WEAVE_ERROR_INVALID_PATH_LIST                          : return NLWEAVE_ERROR_INVALID_PATH_LIST;
    case WEAVE_ERROR_INVALID_DATA_LIST                          : return NLWEAVE_ERROR_INVALID_DATA_LIST;

    case WEAVE_ERROR_TIMEOUT                                    : return NLWEAVE_ERROR_TIMEOUT;
    case WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR                  : return NLWEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
    case WEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION      : return NLWEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION;
    case WEAVE_END_OF_INPUT                                     : return NLWEAVE_END_OF_INPUT;
    case WEAVE_ERROR_RATE_LIMIT_EXCEEDED                        : return NLWEAVE_ERROR_RATE_LIMIT_EXCEEDED;
    case WEAVE_ERROR_SECURITY_MANAGER_BUSY                      : return NLWEAVE_ERROR_SECURITY_MANAGER_BUSY;
    case WEAVE_ERROR_INVALID_PASE_PARAMETER                     : return NLWEAVE_ERROR_INVALID_PASE_PARAMETER;
    case WEAVE_ERROR_PASE_SUPPORTS_ONLY_CONFIG1                 : return NLWEAVE_ERROR_PASE_SUPPORTS_ONLY_CONFIG1;
    case WEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS              : return NLWEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS;
    case WEAVE_ERROR_KEY_CONFIRMATION_FAILED                    : return NLWEAVE_ERROR_KEY_CONFIRMATION_FAILED;
    case WEAVE_ERROR_INVALID_USE_OF_SESSION_KEY                 : return NLWEAVE_ERROR_INVALID_USE_OF_SESSION_KEY;
    case WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY             : return NLWEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    case WEAVE_ERROR_MISSING_TLV_ELEMENT                        : return NLWEAVE_ERROR_MISSING_TLV_ELEMENT;
    case WEAVE_ERROR_RANDOM_DATA_UNAVAILABLE                    : return NLWEAVE_ERROR_RANDOM_DATA_UNAVAILABLE;
    case WEAVE_ERROR_UNSUPPORTED_HOST_PORT_ELEMENT              : return NLWEAVE_ERROR_UNSUPPORTED_HOST_PORT_ELEMENT;
    case WEAVE_ERROR_INVALID_HOST_SUFFIX_INDEX                  : return NLWEAVE_ERROR_INVALID_HOST_SUFFIX_INDEX;
    case WEAVE_ERROR_HOST_PORT_LIST_EMPTY                       : return NLWEAVE_ERROR_HOST_PORT_LIST_EMPTY;
    case WEAVE_ERROR_UNSUPPORTED_AUTH_MODE                      : return NLWEAVE_ERROR_UNSUPPORTED_AUTH_MODE;

    case WEAVE_ERROR_INVALID_SERVICE_EP                         : return NLWEAVE_ERROR_INVALID_SERVICE_EP;
    case WEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE               : return NLWEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE;
    case WEAVE_ERROR_FORCED_RESET                               : return NLWEAVE_ERROR_FORCED_RESET;
    case WEAVE_ERROR_NO_ENDPOINT                                : return NLWEAVE_ERROR_NO_ENDPOINT;
    case WEAVE_ERROR_INVALID_DESTINATION_NODE_ID                : return NLWEAVE_ERROR_INVALID_DESTINATION_NODE_ID;
    case WEAVE_ERROR_NOT_CONNECTED                              : return NLWEAVE_ERROR_NOT_CONNECTED;

    // ----- ASN1 Errors -----
    case ASN1_END                                               : return NLASN1_END;
    case ASN1_ERROR_UNDERRUN                                    : return NLASN1_ERROR_UNDERRUN;
    case ASN1_ERROR_OVERFLOW                                    : return NLASN1_ERROR_OVERFLOW;
    case ASN1_ERROR_INVALID_STATE                               : return NLASN1_ERROR_INVALID_STATE;
    case ASN1_ERROR_MAX_DEPTH_EXCEEDED                          : return NLASN1_ERROR_MAX_DEPTH_EXCEEDED;
    case ASN1_ERROR_INVALID_ENCODING                            : return NLASN1_ERROR_INVALID_ENCODING;
    case ASN1_ERROR_UNSUPPORTED_ENCODING                        : return NLASN1_ERROR_UNSUPPORTED_ENCODING;
    case ASN1_ERROR_TAG_OVERFLOW                                : return NLASN1_ERROR_TAG_OVERFLOW;
    case ASN1_ERROR_LENGTH_OVERFLOW                             : return NLASN1_ERROR_LENGTH_OVERFLOW;
    case ASN1_ERROR_VALUE_OVERFLOW                              : return NLASN1_ERROR_VALUE_OVERFLOW;

    default                                                     : return self.errorCode;
    }
}

@end

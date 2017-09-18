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
 *      Constant definitions bringing various Weave errors into Objective C
 *
 */

#import <Foundation/Foundation.h>

#ifndef __NLWEAVE_ERROR_CODES_H__
#define __NLWEAVE_ERROR_CODES_H__

typedef int32_t WEAVE_ERROR;

#define NLWEAVE_NO_ERROR 0


// Macro for defining Weave errors
#define NLWEAVE_ERROR_BASE 4000
#define NLWEAVE_ERROR_MAX  4999

#define _NLWEAVE_ERROR(e) (NLWEAVE_ERROR_BASE + (e))

// Weave Error Definitions
//
#define NLWEAVE_ERROR_TOO_MANY_CONNECTIONS                        _NLWEAVE_ERROR(0)
#define NLWEAVE_ERROR_SENDING_BLOCKED                             _NLWEAVE_ERROR(1)
#define NLWEAVE_ERROR_CONNECTION_ABORTED                          _NLWEAVE_ERROR(2)
#define NLWEAVE_ERROR_INCORRECT_STATE                             _NLWEAVE_ERROR(3)
#define NLWEAVE_ERROR_MESSAGE_TOO_LONG                            _NLWEAVE_ERROR(4)
#define NLWEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION                _NLWEAVE_ERROR(5)
#define NLWEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS       _NLWEAVE_ERROR(6)
#define NLWEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER              _NLWEAVE_ERROR(7)
#define NLWEAVE_ERROR_NO_CONNECTION_HANDLER                       _NLWEAVE_ERROR(8)
#define NLWEAVE_ERROR_TOO_MANY_PEER_NODES                         _NLWEAVE_ERROR(9)
#define NLWEAVE_ERROR_NO_MEMORY                                   _NLWEAVE_ERROR(11)
#define NLWEAVE_ERROR_NO_MESSAGE_HANDLER                          _NLWEAVE_ERROR(12)
#define NLWEAVE_ERROR_MESSAGE_INCOMPLETE                          _NLWEAVE_ERROR(13)
#define NLWEAVE_ERROR_DATA_NOT_ALIGNED                            _NLWEAVE_ERROR(14)
#define NLWEAVE_ERROR_UNKNOWN_KEY_TYPE                            _NLWEAVE_ERROR(15)
#define NLWEAVE_ERROR_KEY_NOT_FOUND                               _NLWEAVE_ERROR(16)
#define NLWEAVE_ERROR_WRONG_ENCRYPTION_TYPE                       _NLWEAVE_ERROR(17)
#define NLWEAVE_ERROR_TOO_MANY_KEYS                               _NLWEAVE_ERROR(18)
#define NLWEAVE_ERROR_INTEGRITY_CHECK_FAILED                      _NLWEAVE_ERROR(19)
#define NLWEAVE_ERROR_INVALID_SIGNATURE                           _NLWEAVE_ERROR(20)
#define NLWEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION                 _NLWEAVE_ERROR(21)
#define NLWEAVE_ERROR_UNSUPPORTED_ENCRYPTION_TYPE                 _NLWEAVE_ERROR(22)
#define NLWEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE                  _NLWEAVE_ERROR(23)
#define NLWEAVE_ERROR_INVALID_MESSAGE_LENGTH                      _NLWEAVE_ERROR(24)
#define NLWEAVE_ERROR_BUFFER_TOO_SMALL                            _NLWEAVE_ERROR(25)
#define NLWEAVE_ERROR_DUPLICATE_KEY_ID                            _NLWEAVE_ERROR(26)
#define NLWEAVE_ERROR_WRONG_KEY_TYPE                              _NLWEAVE_ERROR(27)
#define NLWEAVE_ERROR_WELL_UNINITIALIZED                          _NLWEAVE_ERROR(28)
#define NLWEAVE_ERROR_WELL_EMPTY                                  _NLWEAVE_ERROR(29)
#define NLWEAVE_ERROR_INVALID_STRING_LENGTH                       _NLWEAVE_ERROR(30)
#define NLWEAVE_ERROR_INVALID_LIST_LENGTH                         _NLWEAVE_ERROR(31)
#define NLWEAVE_ERROR_INVALID_INTEGRITY_TYPE                      _NLWEAVE_ERROR(32)
#define NLWEAVE_END_OF_TLV                                        _NLWEAVE_ERROR(33)
#define NLWEAVE_ERROR_TLV_UNDERRUN                                _NLWEAVE_ERROR(34)
#define NLWEAVE_ERROR_INVALID_TLV_ELEMENT                         _NLWEAVE_ERROR(35)
#define NLWEAVE_ERROR_INVALID_TLV_TAG                             _NLWEAVE_ERROR(36)
#define NLWEAVE_ERROR_UNKNOWN_IMPLICIT_TLV_TAG                    _NLWEAVE_ERROR(37)
#define NLWEAVE_ERROR_WRONG_TLV_TYPE                              _NLWEAVE_ERROR(38)
#define NLWEAVE_ERROR_TLV_CONTAINER_OPEN                          _NLWEAVE_ERROR(39)
#define NLWEAVE_ERROR_INVALID_TRANSFER_MODE                       _NLWEAVE_ERROR(40)
#define NLWEAVE_ERROR_INVALID_PROFILE_ID                          _NLWEAVE_ERROR(41)
#define NLWEAVE_ERROR_INVALID_MESSAGE_TYPE                        _NLWEAVE_ERROR(42)
#define NLWEAVE_ERROR_UNEXPECTED_TLV_ELEMENT                      _NLWEAVE_ERROR(43)
#define NLWEAVE_ERROR_STATUS_REPORT_RECEIVED                      _NLWEAVE_ERROR(44)
#define NLWEAVE_ERROR_NOT_IMPLEMENTED                             _NLWEAVE_ERROR(45)
#define NLWEAVE_ERROR_INVALID_ADDRESS                             _NLWEAVE_ERROR(46)
#define NLWEAVE_ERROR_INVALID_ARGUMENT                            _NLWEAVE_ERROR(47)
#define NLWEAVE_ERROR_INVALID_PATH_LIST                           _NLWEAVE_ERROR(48)
#define NLWEAVE_ERROR_INVALID_DATA_LIST                           _NLWEAVE_ERROR(49)
#define NLWEAVE_ERROR_TIMEOUT                                     _NLWEAVE_ERROR(50)
#define NLWEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR                   _NLWEAVE_ERROR(51)
#define NLWEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION       _NLWEAVE_ERROR(52)
#define NLWEAVE_END_OF_INPUT                                      _NLWEAVE_ERROR(53)
#define NLWEAVE_ERROR_RATE_LIMIT_EXCEEDED                         _NLWEAVE_ERROR(54)
#define NLWEAVE_ERROR_SECURITY_MANAGER_BUSY                       _NLWEAVE_ERROR(55)
#define NLWEAVE_ERROR_INVALID_PASE_PARAMETER                      _NLWEAVE_ERROR(56)
#define NLWEAVE_ERROR_PASE_SUPPORTS_ONLY_CONFIG1                  _NLWEAVE_ERROR(57)
#define NLWEAVE_ERROR_KEY_CONFIRMATION_FAILED                     _NLWEAVE_ERROR(58)
#define NLWEAVE_ERROR_INVALID_USE_OF_SESSION_KEY                  _NLWEAVE_ERROR(59)
#define NLWEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY              _NLWEAVE_ERROR(60)
#define NLWEAVE_ERROR_MISSING_TLV_ELEMENT                         _NLWEAVE_ERROR(61)
#define NLWEAVE_ERROR_RANDOM_DATA_UNAVAILABLE                     _NLWEAVE_ERROR(62)
#define NLWEAVE_ERROR_UNSUPPORTED_HOST_PORT_ELEMENT               _NLWEAVE_ERROR(63)
#define NLWEAVE_ERROR_INVALID_HOST_SUFFIX_INDEX                   _NLWEAVE_ERROR(64)
#define NLWEAVE_ERROR_HOST_PORT_LIST_EMPTY                        _NLWEAVE_ERROR(65)
#define NLWEAVE_ERROR_UNSUPPORTED_AUTH_MODE                       _NLWEAVE_ERROR(66)
#define NLWEAVE_ERROR_INVALID_SERVICE_EP                          _NLWEAVE_ERROR(67)
#define NLWEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE                _NLWEAVE_ERROR(68)
#define NLWEAVE_ERROR_FORCED_RESET                                _NLWEAVE_ERROR(69)
#define NLWEAVE_ERROR_NO_ENDPOINT                                 _NLWEAVE_ERROR(70)
#define NLWEAVE_ERROR_INVALID_DESTINATION_NODE_ID                 _NLWEAVE_ERROR(71)
#define NLWEAVE_ERROR_NOT_CONNECTED                               _NLWEAVE_ERROR(72)
#define NLWEAVE_ERROR_INVALID_PASE_CONFIGURATION                  _NLWEAVE_ERROR(109)
#define NLWEAVE_ERROR_NO_COMMON_PASE_CONFIGURATIONS               _NLWEAVE_ERROR(110)

#endif // __NLWEAVE_ERROR_CODES_H__

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
 *      Definitions of errors from the Weave Inet library.
 *
 */

#import <Foundation/Foundation.h>

#ifndef __NLWEAVE_NLINET_ERROR_CODES_H__
#define __NLWEAVE_NLINET_ERROR_CODES_H__

typedef int INET_ERROR;

#define NLINET_NO_ERROR 0

#define NLINET_ERROR_BASE         1000
#define NLINET_ERROR_MAX          1999

// Macro for defining InetLayer errors
#define _NLINET_ERROR(e) (NLINET_ERROR_BASE + (e))


// InetLayer Error Definitions
//
#define NLINET_ERROR_WRONG_ADDRESS_TYPE                       _NLINET_ERROR(0)
#define NLINET_ERROR_CONNECTION_ABORTED                       _NLINET_ERROR(1)
#define NLINET_ERROR_PEER_DISCONNECTED                        _NLINET_ERROR(2)
#define NLINET_ERROR_INCORRECT_STATE                          _NLINET_ERROR(3)
#define NLINET_ERROR_MESSAGE_TOO_LONG                         _NLINET_ERROR(4)
#define NLINET_ERROR_NO_CONNECTION_HANDLER                    _NLINET_ERROR(5)
#define NLINET_ERROR_NO_MEMORY                                _NLINET_ERROR(6)
#define NLINET_ERROR_OUTBOUND_MESSAGE_TRUNCATED               _NLINET_ERROR(7)
#define NLINET_ERROR_INBOUND_MESSAGE_TOO_BIG                  _NLINET_ERROR(8)
#define NLINET_ERROR_HOST_NOT_FOUND                           _NLINET_ERROR(9)
#define NLINET_ERROR_DNS_TRY_AGAIN                            _NLINET_ERROR(10)
#define NLINET_ERROR_DNS_NO_RECOVERY                          _NLINET_ERROR(11)
#define NLINET_ERROR_BAD_ARGS                                 _NLINET_ERROR(12)
#define NLINET_ERROR_WRONG_PROTOCOL_TYPE                      _NLINET_ERROR(13)
#define NLINET_ERROR_UNKNOWN_INTERFACE                        _NLINET_ERROR(14)
#define NLINET_ERROR_NOT_IMPLEMENTED                          _NLINET_ERROR(15)
#define NLINET_ERROR_ADDRESS_NOT_FOUND                        _NLINET_ERROR(16)
#define NLINET_ERROR_HOST_NAME_TOO_LONG                       _NLINET_ERROR(17)
#define NLINET_ERROR_INVALID_HOST_NAME                        _NLINET_ERROR(18)

#endif // __NLWEAVE_NLINET_ERROR_CODES_H__

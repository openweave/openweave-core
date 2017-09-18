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
 *       Definitions of errors arising from Weave Certificate encoding and decoding.
 *
 */

#import <Foundation/Foundation.h>

#ifndef __NLWEAVE_NLASN1_ERROR_CODES_H__
#define __NLWEAVE_NLASN1_ERROR_CODES_H__

typedef int32_t ASN1_ERROR;

#define NLASN1_NO_ERROR 0

// Macro for defining ASN1 errors
#define NLASN1_ERROR_BASE 5000
#define NLASN1_ERROR_MAX  5999
#define _NLASN1_ERROR(e) (NLASN1_ERROR_BASE + (e))

// ASN1 Error Definitions
//
#define NLASN1_END                        _NLASN1_ERROR(0)
#define NLASN1_ERROR_UNDERRUN             _NLASN1_ERROR(1)
#define NLASN1_ERROR_OVERFLOW             _NLASN1_ERROR(2)
#define NLASN1_ERROR_INVALID_STATE        _NLASN1_ERROR(3)
#define NLASN1_ERROR_MAX_DEPTH_EXCEEDED   _NLASN1_ERROR(4)
#define NLASN1_ERROR_INVALID_ENCODING     _NLASN1_ERROR(5)
#define NLASN1_ERROR_UNSUPPORTED_ENCODING _NLASN1_ERROR(6)
#define NLASN1_ERROR_TAG_OVERFLOW         _NLASN1_ERROR(7)
#define NLASN1_ERROR_LENGTH_OVERFLOW      _NLASN1_ERROR(8)
#define NLASN1_ERROR_VALUE_OVERFLOW       _NLASN1_ERROR(9)

#endif // __NLWEAVE_NLASN1_ERROR_CODES_H__

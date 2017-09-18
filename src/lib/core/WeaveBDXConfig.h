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
 *      for the Nest Bulk Data Transfer (BDX) profile.
 *
 */

#ifndef _WEAVE_BDX_CONFIG_H
#define _WEAVE_BDX_CONFIG_H

/**
 *  @def WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS
 *
 *  @brief
 *    Number of transfers that can exist at once.
 *    Resize to fit application, noting that clients will likely only have 1.
 */
#ifndef WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS
#define WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS 12
#endif // WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS

/**
 *  @def WEAVE_CONFIG_BDX_VERSION
 *
 *  @brief
 *      Version of BDX that we are using for the development version of BDX
 *
 *      1 will compile a version 1 BDX protocol that responds to both v0 and v1
 *      nodes (version == 0 || version == 1 in init message).
 *
 *      0 will compile a version 0 BDX protocol that rejects and init messages
 *      with version != 0, and should only be used if we want a v0 only version
 *      negotiation.
 */
#ifndef WEAVE_CONFIG_BDX_VERSION
#define WEAVE_CONFIG_BDX_VERSION 1
#endif // WEAVE_CONFIG_BDX_VERSION

/**
 *  @def WEAVE_CONFIG_BDX_V0_SUPPORT
 *
 *  @brief
 *      Adds support for BDX V0 functions.  Set to 0 to save code space.
 */
#ifndef WEAVE_CONFIG_BDX_V0_SUPPORT
#define WEAVE_CONFIG_BDX_V0_SUPPORT 1
#endif // WEAVE_CONFIG_BDX_V0_SUPPORT

#if (WEAVE_CONFIG_BDX_VERSION < 1) && (WEAVE_CONFIG_BDX_V0_SUPPORT == 0)
#error "Cannot disable BDX V0 support when the protocol version is set to 0"
#endif //(WEAVE_CONFIG_BDX_VERSION < 1) && (WEAVE_CONFIG_BDX_V0_SUPPORT == 0)

/**
 *  @def WEAVE_CONFIG_BDX_RESPONSE_TIMEOUT_SEC
 *
 *  @brief
 *      Timeout for BDX when waiting for a response.
 *      Kyle Benson said 60s is good, and 10s would be too low for 2nd floor of 900.
 */
#ifndef WEAVE_CONFIG_BDX_RESPONSE_TIMEOUT_SEC
#define WEAVE_CONFIG_BDX_RESPONSE_TIMEOUT_SEC 60
#endif // WEAVE_CONFIG_BDX_RESPONSE_TIMEOUT_SEC

/**
 *  @def WEAVE_CONFIG_BDX_SERVER_SUPPORT
 *
 *  @brief
 *      Compile the BDX server support.
 *
 *  Compile support for BDX server functionality.  Enabled by default.
 *      Set to 0 to save code space on platforms that do not need
 *      server functionality.
 */
#ifndef WEAVE_CONFIG_BDX_SERVER_SUPPORT
#define WEAVE_CONFIG_BDX_SERVER_SUPPORT 1
#endif // WEAVE_CONFIG_BDX_SERVER_SUPPORT

/**
 *  @def WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
 *
 *  @brief
 *      Compile support for client receive-related functions.
 *
 *  Compile support for client receive-related functions.  Enabled by
 *      default.  Set to 0 to save code space when download is not
 *      necessary.
 */
#ifndef WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT
#define WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT 1
#endif // WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT

/**
 *  @def WEAVE_CONFIG_BDX_WRMP_SUPPORT
 *
 *  @brief
 *      Compile support for WRMP when we are connection-less
 *
 *  Compile support for WRMP when we are connection-less. Enabled by
 *      default. Set to 0 to use UDP when connection-less, which just takes
 *      out the RequestAck flag from messages.
 */
#ifndef WEAVE_CONFIG_BDX_WRMP_SUPPORT
#define WEAVE_CONFIG_BDX_WRMP_SUPPORT 1
#endif // WEAVE_CONFIG_BDX_WRMP_SUPPORT

/**
 *  @def WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
 *
 *  @brief
 *      Compile support for client send-related functions.
 *
 *  Compile support for client send-related functions.  Enabled by
 *      default.  Set to 0 to save code space when upload is not
 *      necessary.
 */
#ifndef WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT
#define WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT 1
#endif // WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT

/**
 *  @def WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES
 *
 *  @brief
 *      Max number of bytes for any metadata attached to a BDX SendInit.
 */
#ifndef WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES
#define WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES 64
#endif // WEAVE_CONFIG_BDX_SEND_INIT_MAX_METADATA_BYTES


#if (WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT == 0) && (WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT == 0)
#error "At least one of WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT or WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT must be enabled"
#endif //(WEAVE_CONFIG_BDX_CLIENT_SEND_SUPPORT == 0) && (WEAVE_CONFIG_BDX_CLIENT_RECEIVE_SUPPORT == 0)

#endif // _WEAVE_BDX_CONFIG_H

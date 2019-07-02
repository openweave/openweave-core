/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file defines and implements the Weave Tunnel common elements
 *      between the Border Gateway and the Service. Some of them include
 *      common headers, encode/decode functions for those headers, etc.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include "WeaveTunnelCommon.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <InetLayer/IPAddress.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Weave::Encoding;

/**
 * Encode Tunnel header into the PacketBuffer to encapsulate the IPv6 packet
 * being sent.
 *
 * @param[in] tunHeader       Pointer to the WeaveTunnelHeader to encode.
 *
 * @param[in] message         Pointer to the PacketBuffer on which to encode
 *                            the tunnel header.
 *
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
WEAVE_ERROR WeaveTunnelHeader::EncodeTunnelHeader (WeaveTunnelHeader *tunHeader,
                                                   PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = NULL;
    uint16_t tunHdrLen = 0;
    uint16_t payloadLen = msg->DataLength();

    // Verify the right tunnel version is selected.

    if (tunHeader->Version != kWeaveTunnelVersion_V1)
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_TUNNEL_VERSION);

    p = msg->Start();

    tunHdrLen = TUN_HDR_VERSION_FIELD_SIZE_IN_BYTES; // Version field (1 byte)

    // Set back the pointer by the length of the fields
    p -= tunHdrLen;

    msg->SetStart(p);

    // Encode the fields in the buffer

    Write8(p, tunHeader->Version);

    msg->SetDataLength(tunHdrLen + payloadLen);
exit:

   return err;
}

/**
 * Decode Tunnel header out from the PacketBuffer to decapsulate the IPv6 packet
 * out.
 *
 * @param[out] tunHeader      Pointer to the WeaveTunnelHeader decoded.
 *
 * @param[in]  message        Pointer to the PacketBuffer from which to decode
 *                            the tunnel header.
 *
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
WEAVE_ERROR WeaveTunnelHeader::DecodeTunnelHeader (WeaveTunnelHeader *tunHeader,
                                                   PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = NULL;
    uint16_t msgLen = msg->DataLength();
    uint8_t *msgEnd = msg->Start() + msgLen;

    p = msg->Start();

    tunHeader->Version = Read8(p);
    // Verify the right tunnel version is selected.
    if (tunHeader->Version != kWeaveTunnelVersion_V1)
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_TUNNEL_VERSION);

    if (p > msgEnd)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    }

    msg->SetStart(p);

exit:

    return err;
}

/**
 * Encode Tunnel route containing the set of prefixes into the PacketBuffer containing
 * the Tunnel Control message being sent.
 *
 * @param[in] fabricId        Fabric ID for the routes.
 *
 * @param[in] tunRoutes       Pointer to the WeaveTunnelRoute object containing the
 *                            list of prefixes.
 *
 * @param[in] message         Pointer to the PacketBuffer on which to encode
 *                            the tunnel route prefixes.
 *
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
WEAVE_ERROR WeaveTunnelRoute::EncodeFabricTunnelRoutes(uint64_t fabricId,
                                                       WeaveTunnelRoute *tunRoutes,
                                                       PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = NULL;
    uint16_t payloadLen = 0;

    // FabricId(8 bytes) + numOfPrefixes(1 byte) + (IPv6 prefix(16 bytes) + prefixLen(1 byte) +
    //                                              priority(1 byte)) * numOfPrefixes
    // The TunnelOpen, TunnelRouteUpdate messages would contain the set of routes
    // to send to the Service to program. The TunnelClose message would, however, only contain
    // the fabricId to signal the closing of the Tunnel for that fabric.

    payloadLen = FABRIC_ID_FIELD_SIZE_IN_BYTES;

    if (tunRoutes)
    {
        payloadLen += NUM_OF_PREFIXES_FIELD_SIZE_IN_BYTES +
                      (NL_INET_IPV6_ADDR_LEN_IN_BYTES + NL_IPV6_PREFIX_LEN_FIELD_SIZE_IN_BYTES +
                      NL_IPV6_PREFIX_PRIORITY_FIELD_SIZE_IN_BYTES) * tunRoutes->numOfPrefixes;
    }
    // Error if not enough space after the message payload.

    VerifyOrExit(msg->AvailableDataLength() >= payloadLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    p = msg->Start() + msg->DataLength();

    // Write fabric Id for the routes

    LittleEndian::Write64(p, fabricId);

    // Encode the fields in the buffer

    if (tunRoutes)
    {
        Write8(p, tunRoutes->numOfPrefixes);
        for (int i = 0; i < tunRoutes->numOfPrefixes; i++)
        {
            tunRoutes->tunnelRoutePrefix[i].IPAddr.WriteAddress(p);
            Write8(p, tunRoutes->tunnelRoutePrefix[i].Length);
            Write8(p, tunRoutes->priority[i]);
        }
    }

    msg->SetDataLength(p - msg->Start());

exit:
    return err;
}

/**
 * Decode Tunnel routes containing the set of prefixes from the PacketBuffer containing
 * the Tunnel Control message.
 *
 * @param[out] fabricId       Fabric ID for the routes.
 *
 * @param[out] tunRoutes      Pointer to the WeaveTunnelRoute object containing the
 *                            list of prefixes.
 *
 * @param[in] message         Pointer to the PacketBuffer from which to decode
 *                            the tunnel route prefixes.
 *
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
WEAVE_ERROR WeaveTunnelRoute::DecodeFabricTunnelRoutes(uint64_t *fabricId,
                                                       WeaveTunnelRoute *tunRoutes,
                                                       PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = NULL;
    uint16_t msgLen = msg->DataLength();
    uint8_t *msgEnd = msg->Start() + msgLen;
    uint16_t expectedRouteFieldsLen = 0;

    p = msg->Start();

    // Verify that we can at least read the fabric id.

    VerifyOrExit(msgLen >= (FABRIC_ID_FIELD_SIZE_IN_BYTES),
                 err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    // Read fabric Id for the routes

    *fabricId = LittleEndian::Read64(p);

    if (msgEnd >= (p + 1) && tunRoutes)
    {
        tunRoutes->numOfPrefixes = Read8(p);

        // Now verify if the message data length is exactly the size of the routes to be read.

        expectedRouteFieldsLen = (NL_INET_IPV6_ADDR_LEN_IN_BYTES + NL_IPV6_PREFIX_LEN_FIELD_SIZE_IN_BYTES +
                                  NL_IPV6_PREFIX_PRIORITY_FIELD_SIZE_IN_BYTES) * tunRoutes->numOfPrefixes;

        VerifyOrExit((p + expectedRouteFieldsLen) == msgEnd, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        for (int i = 0; i < tunRoutes->numOfPrefixes; i++)
        {
            IPAddress::ReadAddress(const_cast<const uint8_t *&>(p), tunRoutes->tunnelRoutePrefix[i].IPAddr);
            tunRoutes->tunnelRoutePrefix[i].Length = Read8(p);
            tunRoutes->priority[i] = Read8(p);
        }
    }

    msg->SetStart(p);

exit:
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_TUNNELING

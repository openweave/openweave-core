/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Utility functions for working with OpenThread.
 */


#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR MapOpenThreadError(otError otErr)
{
    // TODO: implement me
    return (otErr == OT_ERROR_NONE) ? WEAVE_NO_ERROR : WEAVE_ERROR_NOT_IMPLEMENTED;
}

/**
 * Log information related to a state change in the OpenThread stack.
 *
 * NB: This function *must* be called with the Thread stack lock held.
 */
void LogOpenThreadStateChange(otInstance * otInst, uint32_t flags)
{
#if WEAVE_DETAIL_LOGGING

        WeaveLogDetail(DeviceLayer, "OpenThread State Changed (Flags: 0x%08x)", flags);
        if ((flags & OT_CHANGED_THREAD_ROLE) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Device Role: %s", OpenThreadRoleToStr(otThreadGetDeviceRole(otInst)));
        }
        if ((flags & OT_CHANGED_THREAD_NETWORK_NAME) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Network Name: %s", otThreadGetNetworkName(otInst));
        }
        if ((flags & OT_CHANGED_THREAD_PANID) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   PAN Id: 0x%04X", otLinkGetPanId(otInst));
        }
        if ((flags & OT_CHANGED_THREAD_EXT_PANID) != 0)
        {
            const otExtendedPanId * exPanId = otThreadGetExtendedPanId(otInst);
            static char exPanIdStr[32];
            snprintf(exPanIdStr, sizeof(exPanIdStr), "0x%02X%02X%02X%02X%02X%02X%02X%02X",
                     exPanId->m8[0], exPanId->m8[1], exPanId->m8[2], exPanId->m8[3],
                     exPanId->m8[4], exPanId->m8[5], exPanId->m8[6], exPanId->m8[7]);
            WeaveLogDetail(DeviceLayer, "   Extended PAN Id: %s", exPanIdStr);
        }
        if ((flags & OT_CHANGED_THREAD_CHANNEL) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Channel: %d", otLinkGetChannel(otInst));
        }
        if ((flags & (OT_CHANGED_IP6_ADDRESS_ADDED|OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
        {
            WeaveLogDetail(DeviceLayer, "   Interface Addresses:");
            for (const otNetifAddress * addr = otIp6GetUnicastAddresses(otInst); addr != NULL; addr = addr->mNext)
            {
                static char ipAddrStr[64];
                nl::Inet::IPAddress ipAddr;

                memcpy(ipAddr.Addr, addr->mAddress.mFields.m32, sizeof(ipAddr.Addr));

                ipAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

                WeaveLogDetail(DeviceLayer, "        %s/%d%s%s%s", ipAddrStr,
                                 addr->mPrefixLength,
                                 addr->mValid ? " valid" : "",
                                 addr->mPreferred ? " preferred" : "",
                                 addr->mRloc ? " rloc" : "");
            }
        }

#endif // WEAVE_DETAIL_LOGGING
}

void LogOpenThreadPacket(const char * titleStr, otMessage * pkt)
{
#if WEAVE_DETAIL_LOGGING

    char srcStr[50], destStr[50], typeBuf[20];
    const char * type = typeBuf;
    IPAddress addr;
    uint8_t headerData[44];
    uint16_t pktLen;

    const uint8_t & IPv6_NextHeader = headerData[6];
    const uint8_t * const IPv6_SrcAddr = headerData + 8;
    const uint8_t * const IPv6_DestAddr = headerData + 24;
    const uint8_t * const IPv6_SrcPort = headerData + 40;
    const uint8_t * const IPv6_DestPort = headerData + 42;
    const uint8_t & ICMPv6_Type = headerData[40];
    const uint8_t & ICMPv6_Code = headerData[41];

    constexpr uint8_t kIPProto_UDP = 17;
    constexpr uint8_t kIPProto_TCP = 6;
    constexpr uint8_t kIPProto_ICMPv6 = 58;

    constexpr uint8_t kICMPType_EchoRequest = 128;
    constexpr uint8_t kICMPType_EchoResponse = 129;

    pktLen = otMessageGetLength(pkt);

    if (pktLen >= sizeof(headerData))
    {
        otMessageRead(pkt, 0, headerData, sizeof(headerData));

        memcpy(addr.Addr, IPv6_SrcAddr, 16);
        addr.ToString(srcStr, sizeof(srcStr));

        memcpy(addr.Addr, IPv6_DestAddr, 16);
        addr.ToString(destStr, sizeof(destStr));

        if (IPv6_NextHeader == kIPProto_UDP)
        {
            type = "UDP";
        }
        else if (IPv6_NextHeader == kIPProto_TCP)
        {
            type = "TCP";
        }
        else if (IPv6_NextHeader == kIPProto_ICMPv6)
        {
            if (ICMPv6_Type == kICMPType_EchoRequest)
            {
                type = "ICMPv6 Echo Request";
            }
            else if (ICMPv6_Type == kICMPType_EchoResponse)
            {
                type = "ICMPv6 Echo Response";
            }
            else
            {
                snprintf(typeBuf, sizeof(typeBuf), "ICMPv6 %" PRIu8 ",%" PRIu8, ICMPv6_Type, ICMPv6_Code);
            }
        }
        else
        {
            snprintf(typeBuf, sizeof(typeBuf), "IP proto %" PRIu8, IPv6_NextHeader);
        }

        if (IPv6_NextHeader == kIPProto_UDP || IPv6_NextHeader == kIPProto_TCP)
        {
            snprintf(srcStr + strlen(srcStr), 13, ", port %" PRIu16, Encoding::BigEndian::Get16(IPv6_SrcPort));
            snprintf(destStr + strlen(destStr), 13, ", port %" PRIu16, Encoding::BigEndian::Get16(IPv6_DestPort));
        }

        WeaveLogDetail(DeviceLayer, "Thread packet %s: %s, len %" PRIu16, titleStr, type, pktLen);
        WeaveLogDetail(DeviceLayer, "    src  %s", srcStr);
        WeaveLogDetail(DeviceLayer, "    dest %s", destStr);
    }
    else
    {
        WeaveLogDetail(DeviceLayer, "%s: %s, len %" PRIu16, titleStr, "(decode error)", pktLen);
    }

#endif // WEAVE_DETAIL_LOGGING
}

bool IsOpenThreadMeshLocalAddress(otInstance * otInst, const IPAddress & addr)
{
    const otMeshLocalPrefix * otMeshPrefix = otThreadGetMeshLocalPrefix(otInst);

    return otMeshPrefix != NULL && memcmp(otMeshPrefix->m8, addr.Addr, OT_MESH_LOCAL_PREFIX_SIZE) == 0;
}

const char * OpenThreadRoleToStr(otDeviceRole role)
{
    switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED:
        return "DISABLED";
    case OT_DEVICE_ROLE_DETACHED:
        return "DETACHED";
    case OT_DEVICE_ROLE_CHILD:
        return "CHILD";
    case OT_DEVICE_ROLE_ROUTER:
        return "ROUTER";
    case OT_DEVICE_ROLE_LEADER:
        return "LEADER";
    default:
        return "(unknown)";
    }
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/*
 *
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
 *      This file contains definitions and headers for helper classes required by the
 *      Weave Tunneling subsystem.
 *
 */
#ifndef _WEAVE_TUNNEL_COMMON_H_
#define _WEAVE_TUNNEL_COMMON_H_

#include <Weave/Core/WeaveCore.h>
#include <InetLayer/IPPrefix.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

#define MAX_NUM_ROUTES                                 (16)
#define MAX_BORDER_GW                                  (8)
#define INVALID_RT_LIFETIME                            (-1)

// Defines for tunnel control message fields
#define FABRIC_ID_FIELD_SIZE_IN_BYTES                  (8)
#define NUM_OF_PREFIXES_FIELD_SIZE_IN_BYTES            (1)
#define NL_IPV6_PREFIX_LEN_FIELD_SIZE_IN_BYTES         (1)
#define NL_IPV6_PREFIX_PRIORITY_FIELD_SIZE_IN_BYTES    (1)
#define TUN_HDR_VERSION_FIELD_SIZE_IN_BYTES            (1)
#define NL_TUNNEL_AGENT_ROLE_SIZE_IN_BYTES             (1)
#define NL_TUNNEL_TYPE_SIZE_IN_BYTES                   (1)
#define NL_TUNNEL_SRC_INTF_TYPE_SIZE_IN_BYTES          (1)
#define NL_TUNNEL_LIVENESS_TYPE_SIZE_IN_BYTES          (1)
#define NL_TUNNEL_LIVENESS_MAX_TIMEOUT_SIZE_IN_BYTES   (2)
#define TUN_HDR_SIZE_IN_BYTES                          (TUN_HDR_VERSION_FIELD_SIZE_IN_BYTES)

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveTunnel {

// Tunnel Control Message Types
enum TunnelCtrlMsgType
{
    kMsgType_TunnelOpen                     = 0x01,
    kMsgType_TunnelRouteUpdate              = 0x02,
    kMsgType_TunnelClose                    = 0x03,
    kMsgType_TunnelReconnect                = 0x04,
    kMsgType_TunnelRouterAdvertise          = 0x05,
    kMsgType_TunnelMobileClientAdvertise    = 0x06,
    kMsgType_TunnelOpenV2                   = 0x07,
    kMsgType_TunnelLiveness                 = 0x08
};

/// Type of the Tunnel.
typedef enum TunnelType
{
    kType_TunnelUnknown                        = 0, ///<Used to indicate an unknown tunnel type.
    kType_TunnelPrimary                        = 1, ///<A primary tunnel for transiting traffic between the device/fabric and the Service.
    kType_TunnelBackup                         = 2, ///<A secondary tunnel serving as an alternate route between the device/fabric and Service.
                                                    /// in the event that no primary tunnel is available.
    kType_TunnelShortcut                       = 3, ///<Used to indicate a shortcut tunnel between a local stand-alone node(mobile device) and
                                                    /// a border gateway.
} TunnelType;

/// Direction of packet traversing the tunnel.
typedef enum TunnelPktDirection
{
    kDir_Inbound                               = 1, ///<Indicates packet coming in to the border gateway over the tunnel.
    kDir_Outbound                              = 2, ///<Indicates packet going out of the border gateway over the tunnel.
} TunnelPktDirection;

/// Roles that the Tunnel Agent can assume; i.e., either border gateway or mobile device.
typedef enum Role
{
    kClientRole_BorderGateway     = 1, ///<The device is acting as a border gateway for the purpose of routing traffic to and from itself,
                                       /// as well as other devices in its associated fabric.
    kClientRole_StandaloneDevice  = 2, ///<The device is acting as a stand-alone node which does not route traffic for other devices.
    kClientRole_MobileDevice      = 3, ///<The device is acting as a stand-alone node which does not route traffic for other devices.
                                       /// It can establish a shortcut tunnel between itself and another border gateway.
} Role;

/// The technology type of the network interface on the device over which the Tunnel is established with the Service.
typedef enum SrcInterfaceType
{
    kSrcInterface_WiFi      = 1, ///<Used when the WiFi interface is used as the source of the Tunnel to the Service.
    kSrcInterface_Cellular  = 2, ///<Used when the Cellular interface is used as the source of the Tunnel to the Service.
} SrcInterfaceType;

/// The liveness strategy employed to maintain the Tunnel connection to theService.
typedef enum LivenessStrategy
{
    kLiveness_TCPKeepAlive  = 1, ///<Used to indicate that the tunnel connection liveness is maintained by TCP KeepAlives.
    kLiveness_TunnelControl = 2, ///<Used to indicate that the tunnel connection liveness is maintained by Tunnel Control Liveness messages.
} LivenessStrategy;

// Weave Tunnel Header
class WeaveTunnelHeader
{
public:
    uint8_t  Version;
    //...

/**
 * Encode Tunnel header into the PacketBuffer to encapsulate the IPv6 packet
 * being sent.
 */
    static WEAVE_ERROR EncodeTunnelHeader(WeaveTunnelHeader *tunHeader,
                                          PacketBuffer *message);

/**
 * Decode Tunnel header out from the PacketBuffer to decapsulate the IPv6 packet
 * out.
 */
    static WEAVE_ERROR DecodeTunnelHeader(WeaveTunnelHeader *tunHeader,
                                          PacketBuffer *message);
};

// Weave Tunnel Route
class WeaveTunnelRoute
{
public:

    /**
     *  Weave Tunnel Route priority values.
     *
     *  @note
     *    By default, the primary tunnel would be set to the priority value of Medium
     *    and the backup tunnel to Low. The priority value of High is defined here
     *    should the need arise to elevate the priority of a particular tunnel from
     *    Low to High to allow for an immediate switch from the other tunnel
     *    path (with Medium priority).
     *
     */

    typedef enum RoutePriority
    {
        kRoutePriority_High                     = 1, ///< The route priority value for high
        kRoutePriority_Medium                   = 2, ///< The route priority value for medium
        kRoutePriority_Low                      = 3, ///< The route priority value for low
    } RoutePriority;

    // Set of prefix routes to pass to the Service
    IPPrefix  tunnelRoutePrefix[MAX_NUM_ROUTES];

    // Route priority values
    uint8_t priority[MAX_NUM_ROUTES];

    uint8_t   numOfPrefixes;

/**
 * Encode Tunnel routes containing the set of prefixes into the PacketBuffer containing
 * the Tunnel Control message being sent.
 */
    static WEAVE_ERROR EncodeFabricTunnelRoutes(uint64_t fabricId,
                                                WeaveTunnelRoute *tunRoute,
                                                PacketBuffer *message);

/**
 * Decode Tunnel routes containing the set of prefixes from the PacketBuffer containing
 * the Tunnel Control message.
 */
    static WEAVE_ERROR DecodeFabricTunnelRoutes(uint64_t *fabricId,
                                                WeaveTunnelRoute *tunRoute,
                                                PacketBuffer *message);

};

// Version of the Weave Tunnel Subsystem
typedef enum WeaveTunnelVersion
{
    kWeaveTunnelVersion_V1 = 1
} WeaveTunnelVersion;

}
}
}
}

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif //_WEAVE_TUNNEL_COMMON_H_

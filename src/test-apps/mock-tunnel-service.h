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
 *      This file implements the Weave Mock Tunnel Service.
 *
 *      This instantiates a Server that accepts connections from
 *      a border gateway and may perform routing functions
 *      between different border gateways or respond to ping6
 *      over the tunnel.
 *
 */

#ifndef WEAVE_TUNNEL_SERVICE_H_
#define WEAVE_TUNNEL_SERVICE_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/ip6.h"
#include "lwip/icmp6.h"
#include "lwip/ip6_addr.h"
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP

#if WEAVE_CONFIG_ENABLE_TUNNELING

#define SERVICE_ROUTE_TABLE_SIZE  (64)

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace nl::Weave::Profiles::WeaveTunnel;

class WeaveTunnelServer;

//Virtual Route Table used by the Service to route IPv6 packets between various
//border gateways and mobile devices.
class VirtualRouteTable
{
    friend class WeaveTunnelServer;

public:
    //State of route entry in the virtual route table
    enum RouteEntryState
    {
        kRouteEntryState_Invalid  = 0,
        kRouteEntryState_Valid    = 1,
    };

    VirtualRouteTable(void);

/**
 * Lookup an IP prefix in the route table to locate route table entry.
 *
 * @param[in] ip6Prefix       IPPrefix to lookup in the route table.
 *
 * @return index              Index of entry if found, else -1;
 */
    int FindRouteEntry(IPPrefix &ip6Prefix);

/**
 * Remove all route table entries for the connection.
 *
 * @param[in] con             Pointer to a WeaveConnection object
 *
 * @return void
 */
    void RemoveRouteEntryByConnection(WeaveConnection *con);

/**
 * Create a new route entry in the route table.
 *
 * @return index              Index of entry if successful, else -1;
 */
    int NewRouteEntry(void);

/**
 * Free route entry at the given index.
 *
 * @param[in] index           Index of route entry to free in route table.
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
    WEAVE_ERROR FreeRouteEntry(int index);

    //Route Entry in the Route table
    class RouteEntry
    {
      public:
       IPPrefix             prefix;
       uint8_t              priority[2]; // 2 priority entries with 3 possible levels of priority : high(1), medium(2), and low(3)
       uint64_t             fabricId;
       uint64_t             BorderGwList[MAX_BORDER_GW];
       WeaveConnection      *outgoingCon[2]; // 2 connections corresponding to each priority entry
       int16_t              routeLifetime;
       uint16_t             keyId;
       uint8_t              encryptionType;
       RouteEntryState      routeState;
    };
    RouteEntry RouteTable[SERVICE_ROUTE_TABLE_SIZE];

};

class NL_DLL_EXPORT WeaveTunnelServer : public WeaveServerBase
{
public:
    WeaveTunnelServer();

/**
 * Initialize the Weave Tunnel Server. Set handlers for Message Layer and
 * Exchange Manager.
 *
 * @param[in] exchangeMgr     Pointer to exchangeManager object.
 *
 * @return WEAVE_ERROR        WEAVE_NO_ERROR on success, else error;
 */
    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);

/**
 * Close all connections in route table.
 *
 * @return void;
 */
    WEAVE_ERROR Shutdown();

private:
    WeaveTunnelServer(const WeaveTunnelServer&);   // not defined

    WEAVE_ERROR ProcessIPv6Message(WeaveConnection *con, const WeaveMessageInfo *recvMsgInfo, PacketBuffer *msg);

    //TunEndPoint management functions
    WEAVE_ERROR CreateServiceTunEndPoint();
    WEAVE_ERROR SetupServiceTunEndPoint(void);
    WEAVE_ERROR TeardownServiceTunEndPoint(void);

/**
 * Handler for a tunneled IPv6 data message.
 */
    static void HandleTunnelDataMessage(WeaveConnection *con, const WeaveMessageInfo *msgInfo,
                                        PacketBuffer *msg);
/**
 * Handler for a Weave Tunnel control message.
 */
    static void HandleTunnelControlMsg(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                       const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                       uint8_t msgType, PacketBuffer *payload);

    static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);


    static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

    static void RecvdFromServiceTunEndPoint(TunEndPoint *tunEP, PacketBuffer *message);
    static void HandleEchoRequestReceived(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);

    static void HandleSecureSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con,
                                               void *reqState, uint16_t sessionKeyId, uint64_t peerNodeId,
                                               uint8_t encType);
    static void HandleSecureSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState,
                                         WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);

    void StoreGatewayInfoForPriority(WeaveConnection *conn, uint8_t rtIndex, uint8_t priorityIndex,
                                     uint8_t priorityVal, const IPPacketInfo *pktInfo,
                                     const WeaveMessageInfo *msgInfo);

    WeaveConnection * GetOutgoingConn(uint8_t index);

    WEAVE_ERROR SendStatusReport(ExchangeContext *ec, uint32_t profileId, uint32_t tunStatusCode);

    TunEndPoint *mTunEP;

    InetLayer *mInet;                       // [READ ONLY] Associated InetLayer object

    VirtualRouteTable vRouteDB;
};

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif // WEAVE_TUNNEL_SERVICE_H

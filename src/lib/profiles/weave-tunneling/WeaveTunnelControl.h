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
 *      This file contains headers for the Weave Tunnel Control Protocol, its state management
 *      and protocol operation functions.
 *
 *
 */
#ifndef _WEAVE_TUNNEL_CONTROL_H_
#define _WEAVE_TUNNEL_CONTROL_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveTunnel {

class WeaveTunnelAgent;
class WeaveTunnelConnectionMgr;

/// Weave Tunnel Status Codes

enum
{
    kStatusCode_TunnelOpenFail                  = 1,  ///< The Service has encountered an error while opening the tunnel.
    kStatusCode_TunnelCloseFail                 = 2,  ///< The Service has encountered an error while closing the tunnel.
    kStatusCode_TunnelRouteUpdateFail           = 3,  ///< The Service has encountered an error while updating the routes.
    kStatusCode_TunnelReconnectFail             = 4,  ///< The Border gateway has encountered an error while reconnecting to the Service.
};

class NL_DLL_EXPORT WeaveTunnelControl
{
  friend class WeaveTunnelConnectionMgr;
public:

/// The timeout(in seconds) for responses to control messages
    uint16_t mCtrlResponseTimeout;

/// Interval in seconds for periodic shortcut tunnel advertisements.
    uint16_t mShortcutTunnelAdvInterval;

    WeaveTunnelControl(void);

/**
 * Function pointer to handler set by a higher layer to act upon receipt of a StatusReport
 * message in response to a Tunnel control message sent.
 *
 * @param[in] tType           The tunnel type, i.e., Primary or Backup.
 *
 * @param[in] tunStatus       A reference to the Tunnel control StatusReport message.
 *
 * @return void.
 */
    typedef void (*TunnelStatusRcvdFunct)(uint8_t tType, StatusReport &tunStatus);
    TunnelStatusRcvdFunct OnTunStatusRcvd;

/**
 * Initialize WeaveTunnelControl to set relevant members like the Weave Tunnel Agent and callbacks.
 */
    WEAVE_ERROR Init(WeaveTunnelAgent *tunAgent,
                     TunnelStatusRcvdFunct statusRcvd = NULL);

/**
 * Close WeaveTunnelControl by closing any outstanding exchange contexts and resetting members.
 */
    WEAVE_ERROR Close(void);


/**
 * Send a Tunnel Open control message to the peer node with a set of tunnel routes.
 */
    WEAVE_ERROR SendTunnelOpen(WeaveTunnelConnectionMgr *conMgr, WeaveTunnelRoute *tunRoute);

/**
 * Send a Tunnel Close control message to the peer node with a set of tunnel routes.
 */
    WEAVE_ERROR SendTunnelClose(WeaveTunnelConnectionMgr *conMgr);

/**
 * Send a Tunnel Route Update control message to the peer node with a set of tunnel routes.
 */
    WEAVE_ERROR SendTunnelRouteUpdate(WeaveTunnelConnectionMgr *conMgr, WeaveTunnelRoute *tunRoute);

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Send a Tunnel Close control message to the peer node with a set of tunnel routes.
 */
    WEAVE_ERROR SendTunnelLiveness(WeaveTunnelConnectionMgr *conMgr);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Reconnect with the peer node.
 */
    WEAVE_ERROR Reconnect(WeaveTunnelConnectionMgr *conMgr);

/**
 * Send a border router advertise message advertising its fabric Id.
 */
    WEAVE_ERROR SendBorderRouterAdvertise (void);

/**
 * Send a mobile client advertise message advertising its Node Id.
 */
    WEAVE_ERROR SendMobileClientAdvertise (void);

/**
 * Function registered with WeaveMessageLayer for listening to Shortcut
 * tunnel advertisments and updating cache.
 */
    static void HandleShortcutTunnelAdvertiseMessage(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                     const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                     uint8_t msgType, PacketBuffer *payload);

/**
 * Verify if the peer is present in the tunnel shortcut cache for sending locally.
 */
    bool IsPeerInShortcutTunnelCache(uint64_t peerId);

/**
 * Enable shortcut tunneling by sending advertisments from either the Border gateway or Mobile client and
 * also listening to advertisements from shortcut tunnel counterparts.
 */
    void EnableShortcutTunneling(void);

/**
 * Disable shortcut tunneling of sending advertisments from either the Border gateway or Mobile client and
 * also listening to advertisements from shortcut tunnel counterparts.
 */
    void DisableShortcutTunneling(void);

/**
 * Send message over the tunnel shortcut.
 */
    WEAVE_ERROR SendMessageOverTunnelShortcut(uint64_t peerId, WeaveMessageInfo *msgHdr, PacketBuffer *msg);

private:

    WEAVE_ERROR CreateContext(WeaveConnection *aConnection,
                              ExchangeContext::MessageReceiveFunct onMsgRcvd);
    void Free(void);
    WEAVE_ERROR SendTunnelMessage(TunnelCtrlMsgType msgType,
                                  WeaveTunnelConnectionMgr *con,
                                  uint64_t fabricId,
                                  WeaveTunnelRoute *tunRoute,
                                  ExchangeContext::MessageReceiveFunct onMsgRcvd);


    static void HandleTunnelOpenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                         const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                         uint8_t msgType, PacketBuffer *payload);

    static void HandleTunnelCloseResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                          const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                          uint8_t msgType, PacketBuffer *payload);

    static void HandleTunnelRouteUpdateResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                uint8_t msgType, PacketBuffer *payload);

    static void HandleTunnelLivenessResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                             const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                             uint8_t msgType, PacketBuffer *payload);

    static void HandleTunnelReconnect(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                      const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                      uint8_t msgType, PacketBuffer *payload);

    static WEAVE_ERROR SendStatusReport(ExchangeContext *ec, uint32_t profileId,
                                        uint16_t tunStatusCode);

    // Timeout handler for tunnel control response.

    static void TunCtrlRespExpiryHandler(ExchangeContext *ec);

    // Helper function to close the tunnel connection and report error based on a status report

    void TunnelCloseAndReportErrorStatus(WeaveTunnelConnectionMgr *conMgr, WEAVE_ERROR err, StatusReport report);

    // Helper function to free buffer and close the tunnel control ExchangeContext

    void FreeBufferAndCloseExchange(PacketBuffer *buf, ExchangeContext *&ec);

    static WEAVE_ERROR VerifyAndParseStatusResponse(uint32_t profileId,
                                                    uint8_t msgType, PacketBuffer *payload,
                                                    StatusReport &outReport);

    WEAVE_ERROR DecodeTunnelReconnect(uint16_t &hostPort,
                                      char *hostName,
                                      uint8_t &hostNameLen,
                                      PacketBuffer *msg);

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    WEAVE_ERROR GetSendInterfaceIdForBroadcast(InterfaceId &sendIntfId);
    WEAVE_ERROR SendShortcutTunnelAdvertiseMessage(TunnelCtrlMsgType shortcutTunAdvMsgType,
                                                   InterfaceId sendIntfId, uint64_t localAddrIdentifier);

    // Timer based periodic advertisement messaging functions.

    void StartShortcutTunnelAdvertisementsFromBorderRouter(void);
    void StopShortcutTunnelAdvertisementsFromBorderRouter(void);

    void StartShortcutTunnelAdvertisementsFromMobileClient(void);
    void StopShortcutTunnelAdvertisementsFromMobileClient(void);

    void RegisterShortcutTunnelAdvHandlers(void);
    void UnregisterShortcutTunnelAdvHandlers(void);

    // Timeout functions performing timer actions.

    static void BorderRouterAdvTimeout(InetLayer *inetLayer, void *appState, INET_ERROR err);
    static void MobileClientAdvTimeout(InetLayer *inetLayer, void *appState, INET_ERROR err);
    static void PurgeStaleNextHopEntries(InetLayer *inetLayer, void *appState, INET_ERROR err);

    // Timer based Nexthop cache monitor

    void StartNextHopTableMonitor(void);
    void StopNextHopTableMonitor(void);

    // Nexthop cache management functions

    int FindTunnelPeerEntry (uint64_t peerId);
    int NewNextHopEntry(void);
    WEAVE_ERROR FreeNextHopEntry(int index);
    WEAVE_ERROR UpdateOrAddTunnelPeerEntry(uint64_t peerId, IPAddress peerAddr, uint64_t peerNodeId);
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED

    // Weave Tunnel Agent handle

    WeaveTunnelAgent        *mTunnelAgent;

    // Exchange Context to use when sending Weave control messages to Service.

    ExchangeContext         *mServiceExchangeCtxt;

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    // Exchange Context to use when sending Weave shortcut tunnel advertise messages.

    ExchangeContext         *mShortcutTunExchangeCtxt;

    class ShortcutTunnelPeerEntry
    {
      public:
        uint64_t  peerIdentifier; // NodeId for MobileClient or FabricId for Border Gateway
        uint64_t  peerNodeId;     // Peer NodeId
        IPAddress peerAddr;       // Shortcut tunnel peer address
        bool      staleFlag;      // true if stale; false if fresh.
    };

    ShortcutTunnelPeerEntry ShortcutTunnelPeerCache[WEAVE_CONFIG_TUNNELING_MAX_NUM_SHORTCUT_TUNNEL_PEERS];
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
};

} // namespace WeaveTunnel
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif //_WEAVE_TUNNEL_CONTROL_H_

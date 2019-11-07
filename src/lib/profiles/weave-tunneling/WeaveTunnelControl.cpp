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
 *      This file contains header for the Weave Tunnel Control Protocol, its state management
 *      and protocol operation functions.
 *
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include "WeaveTunnelControl.h"
#include "WeaveTunnelAgent.h"
#include "WeaveTunnelConnectionMgr.h"
#include <InetLayer/InetInterface.h>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveTLV.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

using namespace nl::Inet;
using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Weave::Profiles::StatusReporting;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

WeaveTunnelControl::WeaveTunnelControl(void)
{
    mTunnelAgent               = NULL;
#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mShortcutTunExchangeCtxt   = NULL;
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mServiceExchangeCtxt       = NULL;
    mCtrlResponseTimeout       = 0;
    OnTunStatusRcvd            = NULL;
}

/**
 * Initialize WeaveTunnelControl to set relevant members like the Weave Tunnel Agent and callbacks.
 *
 * @param[in] tunAgent          A pointer to the WeaveTunnelAgent object.
 *
 * @param[in] statusRcvd        A pointer to a callback for the StatusRcvd handler.
 *
 * @return WEAVE_NO_ERROR
 */
WEAVE_ERROR WeaveTunnelControl::Init(WeaveTunnelAgent *tunAgent,
                                     TunnelStatusRcvdFunct statusRcvd)
{
    WEAVE_ERROR err            = WEAVE_NO_ERROR;
    mTunnelAgent               = tunAgent;
#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mShortcutTunExchangeCtxt   = NULL;
    mShortcutTunnelAdvInterval = WEAVE_CONFIG_TUNNELING_SHORTCUT_TUNNEL_ADV_INTERVAL_SECS;
    memset(ShortcutTunnelPeerCache, 0, sizeof(ShortcutTunnelPeerCache));
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mServiceExchangeCtxt       = NULL;
    mCtrlResponseTimeout       = WEAVE_CONFIG_TUNNELING_CTRL_RESPONSE_TIMEOUT_SECS;
    OnTunStatusRcvd            = statusRcvd;

    // Check if the WeaveTunnelAgent object is valid.

    VerifyOrExit(mTunnelAgent != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = mTunnelAgent->mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                                        kMsgType_TunnelReconnect,
                                                                        HandleTunnelReconnect, this);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Close WeaveTunnelControl by closing any outstanding exchange contexts and resetting members.
 *
 * @return WEAVE_NO_ERROR.
 */
WEAVE_ERROR WeaveTunnelControl::Close(void)
{
    // Close all the exchanegContexts

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    if (mShortcutTunExchangeCtxt != NULL)
    {
        mShortcutTunExchangeCtxt->Close();
    }
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED

    if (mServiceExchangeCtxt != NULL)
    {
        mServiceExchangeCtxt->Close();
    }

    // Unregister the Tunnel reconnect handler

    mTunnelAgent->mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling,
                                                                    kMsgType_TunnelReconnect);

    Free();

    return WEAVE_NO_ERROR;
}

/**
 * Send a Tunnel Open control message to the peer node with a set of tunnel routes.
 *
 * @param[in] conMgr            A pointer to the WeaveTunnelConnectionMgr object.
 *
 * @param[in] tunRoutes         List of prefix routes to add to route table.
 *
 * @return WEAVE_ERROR          WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendTunnelOpen(WeaveTunnelConnectionMgr *conMgr,
                                               WeaveTunnelRoute *tunRoutes)
{
    return SendTunnelMessage(kMsgType_TunnelOpenV2, conMgr,
                             mTunnelAgent->mExchangeMgr->FabricState->FabricId,
                             tunRoutes, HandleTunnelOpenResponse);
}

/**
 * Send a Tunnel Close control message to the peer node.
 *
 * @param[in] conMgr            A pointer to the WeaveTunnelConnectionMgr object.
 *
 * @return WEAVE_ERROR          WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendTunnelClose (WeaveTunnelConnectionMgr *conMgr)
{
    return SendTunnelMessage(kMsgType_TunnelClose, conMgr,
                             mTunnelAgent->mExchangeMgr->FabricState->FabricId,
                             NULL, HandleTunnelCloseResponse);
}

/**
 * Send a Tunnel Route Update control message to the peer node with a set of tunnel routes.
 *
 * @param[in] conMgr            A pointer to the WeaveTunnelConnectionMgr object.
 *
 * @param[in] tunRoutes         List of prefix routes to add to route table.
 *
 * @return WEAVE_ERROR          WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendTunnelRouteUpdate(WeaveTunnelConnectionMgr *conMgr,
                                                      WeaveTunnelRoute *tunRoutes)
{
    return SendTunnelMessage(kMsgType_TunnelRouteUpdate, conMgr,
                             mTunnelAgent->mExchangeMgr->FabricState->FabricId,
                             tunRoutes, HandleTunnelRouteUpdateResponse);
}

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Send a Tunnel Liveness control message to the peer node.
 *
 * @param[in] conMgr            A pointer to the WeaveTunnelConnectionMgr object.
 *
 * @return WEAVE_ERROR          WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendTunnelLiveness(WeaveTunnelConnectionMgr *conMgr)
{
    return SendTunnelMessage(kMsgType_TunnelLiveness, conMgr,
                             0,
                             NULL, HandleTunnelLivenessResponse);
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
/**
 * Fetch the interface for sending the broadcast advertisements
 *
 * @param[out] sendIntfId      The send interface id for broadcasting advertisements.
 *
 * @return WEAVE_ERROR         WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::GetSendInterfaceIdForBroadcast (InterfaceId &sendIntfId)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;
    uint64_t globalId = 0;
    IPAddress sendIntfAddr;
    globalId = WeaveFabricIdToIPv6GlobalId(mTunnelAgent->mExchangeMgr->FabricState->FabricId);
    if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_BorderGateway)
    {
        sendIntfAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi,
                                WeaveNodeIdToIPv6InterfaceId(mTunnelAgent->mExchangeMgr->FabricState->LocalNodeId));
    }
    else if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_MobileDevice)
    {
        sendIntfAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_MobileDevice,
                                WeaveNodeIdToIPv6InterfaceId(mTunnelAgent->mExchangeMgr->FabricState->LocalNodeId));
    }
    res = mTunnelAgent->mInet->GetInterfaceFromAddr(sendIntfAddr, sendIntfId);

    return res;
}

/* Send the Shortcut Tunnel Advertise message of specified type */
WEAVE_ERROR WeaveTunnelControl::SendShortcutTunnelAdvertiseMessage (TunnelCtrlMsgType shortcutTunAdvMsgType,
                                                                    InterfaceId sendIntfId,
                                                                    uint64_t localAddrIdentifier)
{
    WEAVE_ERROR         err         = WEAVE_NO_ERROR;
    PacketBuffer*       msgBuf      = NULL;
    ExchangeContext*    exchangeCtx = NULL;
    uint8_t*            p           = NULL;

    // allocate buffer and send tunnel message
    msgBuf = PacketBuffer::New();

    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(mTunnelAgent->mExchangeMgr != NULL,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    exchangeCtx = mTunnelAgent->mExchangeMgr->NewContext(nl::Weave::kAnyNodeId, this);

    VerifyOrExit(exchangeCtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mShortcutTunExchangeCtxt = exchangeCtx;

    //Encode the Fabric Id for Border Gateway and the Node Id for Mobile Client.
    p = msgBuf->Start();
    LittleEndian::Write64(p, localAddrIdentifier);
    msgBuf->SetDataLength(8);
    mShortcutTunExchangeCtxt->PeerIntf = sendIntfId;
    err = mShortcutTunExchangeCtxt->SendMessage(kWeaveProfile_Tunneling, shortcutTunAdvMsgType, msgBuf,
                                                nl::Weave::ExchangeContext::kSendFlag_DefaultMulticastSourceAddress);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (mShortcutTunExchangeCtxt != NULL)
    {
        mShortcutTunExchangeCtxt->Close();
        mShortcutTunExchangeCtxt = NULL;
    }

    return err;
}

/**
 * Send a border router advertise message advertising its fabric Id.
 *
 * @return WEAVE_ERROR         WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendBorderRouterAdvertise (void)
{
    WEAVE_ERROR res        = WEAVE_NO_ERROR;
    InterfaceId sendIntfId = INET_NULL_INTERFACEID;

    // Get the sendIntfId for the broadcast

    res = GetSendInterfaceIdForBroadcast(sendIntfId);

    if (res == WEAVE_NO_ERROR)
    {
        res = SendShortcutTunnelAdvertiseMessage(kMsgType_TunnelRouterAdvertise, sendIntfId,
                                                 mTunnelAgent->mExchangeMgr->FabricState->FabricId);
    }

    return res;
}

/**
 * Send a mobile client advertise message advertising its Node Id.
 *
 * @return WEAVE_ERROR         WEAVE_NO_ERROR on success, else error.
 */
WEAVE_ERROR WeaveTunnelControl::SendMobileClientAdvertise (void)
{
    WEAVE_ERROR res        = WEAVE_NO_ERROR;
    InterfaceId sendIntfId = INET_NULL_INTERFACEID;

    //Get the sendIntfId for the broadcast
    res = GetSendInterfaceIdForBroadcast(sendIntfId);

    if (res == WEAVE_NO_ERROR)
    {
        res = SendShortcutTunnelAdvertiseMessage(kMsgType_TunnelMobileClientAdvertise, sendIntfId,
                                                 mTunnelAgent->mExchangeMgr->FabricState->LocalNodeId);
    }

    return res;
}

/**
 * Function registered with WeaveMessageLayer for listening to Shortcut
 * tunnel advertisments and updating cache.
 *
 * @param[in] ec               A pointer to the ExchangeContext object.
 *
 * @param[in] pktInfo          A pointer to the IPPacketInfo object.
 *
 * @param[in] msgInfo          A pointer to the WeaveMessageInfo object.
 *
 * @param[in] profileId        The profileId for which this handler was called.
 *
 * @param[in] msgType          The msgType within the above profile.
 *
 * @param[in] payload          A pointer to the PacketBuffer object holding the
 *                             advertisement message.
 *
 */
void WeaveTunnelControl::HandleShortcutTunnelAdvertiseMessage (ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                               const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                               uint8_t msgType, PacketBuffer *payload)
{
    uint8_t     *p  = NULL;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    uint64_t peerId = 0;
    // Verify that the message is a Tunnel Control Message for added safety.
    if (profileId != kWeaveProfile_Tunneling)
    {
        ExitNow();
    }

    p = payload->Start();
    peerId = LittleEndian::Read64(p);

    //Go through the message type and update the local nextHop Table
    switch (msgType)
    {
        case kMsgType_TunnelRouterAdvertise:
          //TunnelRouter advertisements are meant for mobile client devices.
          if (tunControl->mTunnelAgent->mRole != WeaveTunnelAgent::kRole_MobileDevice)
          {
              ExitNow();
          }

          tunControl->UpdateOrAddTunnelPeerEntry(peerId, pktInfo->SrcAddress,
                                                 msgInfo->SourceNodeId);

          break;
        case kMsgType_TunnelMobileClientAdvertise:
          //Mobile Client advertisements are meant for tunnel border gateways.
          if (tunControl->mTunnelAgent->mRole != WeaveTunnelAgent::kRole_BorderGateway)
          {
              ExitNow();
          }

          tunControl->UpdateOrAddTunnelPeerEntry(peerId, pktInfo->SrcAddress,
                                                 msgInfo->SourceNodeId);

          break;
        default:
          break;
    }

exit:
    // Free the payload buffer.
    PacketBuffer::Free(payload);

    // Discard the exchange context.
    ec->Close();
    ec = NULL;

    return;
}

/**
 * Verify if the peer is present in the shortcut tunnel cache for sending locally.
 *
 * @param[in] peerId          The identifier for the peer for the tunnel shortcut.
 *                            It is the FabricId if the peer is a border gateway and
 *                            the nodeId if the peer is a mobile device.
 *
 * @return true if present and false otherwise.
 */
bool WeaveTunnelControl::IsPeerInShortcutTunnelCache(uint64_t peerId)
{
    int nextHopTableIndex  = -1;
    bool retVal            = false;
    nextHopTableIndex = FindTunnelPeerEntry(peerId);

    if (nextHopTableIndex >= 0)
    {
       retVal = true;
    }

    return retVal;
}

/**
 * Enable shortcut tunneling by sending advertisments from either the Border
 * gateway or Mobile client and also listening to advertisements from shortcut
 * tunnel counterparts.
 *
 */
void WeaveTunnelControl::EnableShortcutTunneling (void)
{
    RegisterShortcutTunnelAdvHandlers();

    if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_BorderGateway)
    {
        StartShortcutTunnelAdvertisementsFromBorderRouter();
    }
    else if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_MobileDevice)
    {
        StartShortcutTunnelAdvertisementsFromMobileClient();
    }

    StartNextHopTableMonitor();
}

/**
 * Disable shortcut tunneling of sending advertisments from either the Border gateway or Mobile client and
 * also listening to advertisements from shortcut tunnel counterparts.
 *
 */
void WeaveTunnelControl::DisableShortcutTunneling (void)
{
    UnregisterShortcutTunnelAdvHandlers();

    if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_BorderGateway)
    {
        StopShortcutTunnelAdvertisementsFromBorderRouter();
    }
    else if (mTunnelAgent->mRole == WeaveTunnelAgent::kRole_MobileDevice)
    {
        StopShortcutTunnelAdvertisementsFromMobileClient();
    }

    StopNextHopTableMonitor();
}

/**
 * Send message over the tunnel shortcut.
 */
WEAVE_ERROR WeaveTunnelControl::SendMessageOverTunnelShortcut(uint64_t peerId,
                                                              WeaveMessageInfo *msgInfo,
                                                              PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int nextHopTableIndex  = -1;

    nextHopTableIndex = FindTunnelPeerEntry(peerId);

    VerifyOrExit(nextHopTableIndex >= 0, err = WEAVE_ERROR_TUNNEL_PEER_ENTRY_NOT_FOUND);

    // For shortcut tunneling explicitly set the destination node id from neighbor cache

    msgInfo->DestNodeId = ShortcutTunnelPeerCache[nextHopTableIndex].peerNodeId;

    err = mTunnelAgent->mExchangeMgr->MessageLayer->SendUDPTunneledMessage(
                                 ShortcutTunnelPeerCache[nextHopTableIndex].peerAddr,
                                 msgInfo, msg);
    msg = NULL;

exit:
    if (msg != NULL)
    {
        PacketBuffer::Free(msg);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        // Update tunnel statistics
        mTunnelAgent->mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    }

    return err;
}

/* Timer expiry function for sending periodic border router advertisements for shortcut tunneling */
void WeaveTunnelControl::BorderRouterAdvTimeout (InetLayer *inetLayer, void *appState, INET_ERROR err)
{
    WEAVE_ERROR      res     = WEAVE_NO_ERROR;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(appState);

    // Send the Border Router Advertise message.

    tunControl->SendBorderRouterAdvertise();

    // Restart the timer.

    res = tunControl->mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(tunControl->mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                                                 BorderRouterAdvTimeout, tunControl);
    VerifyOrDieWithMsg(res == WEAVE_NO_ERROR, WeaveTunnel, "Cannot start BorderRouterAdvTimeout\n");
}

/* Timer expiry function for sending periodic mobile client advertisements for shortcut tunneling */
void WeaveTunnelControl::MobileClientAdvTimeout (InetLayer *inetLayer, void *appState, INET_ERROR err)
{
    WEAVE_ERROR      res     = WEAVE_NO_ERROR;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(appState);

    // Send the Mobile Client Advertise message.

    tunControl->SendMobileClientAdvertise();

    // Restart the timer.

    res = tunControl->mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(tunControl->mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                                                 MobileClientAdvTimeout, tunControl);
    VerifyOrDieWithMsg(res == WEAVE_NO_ERROR, WeaveTunnel, "Cannot start MobileClientAdvTimeout\n");
}

/* Timer expiry functions to mark and purge stale entries from previous advertisements  */
void WeaveTunnelControl::PurgeStaleNextHopEntries (InetLayer *inetLayer, void *appState, INET_ERROR err)
{
    WEAVE_ERROR      res     = WEAVE_NO_ERROR;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(appState);

    // Go through the shortcut tunnel nexthop table and purge stale entries.

    for (int i = 0; i < WEAVE_CONFIG_TUNNELING_MAX_NUM_SHORTCUT_TUNNEL_PEERS; i++)
    {
        if (tunControl->ShortcutTunnelPeerCache[i].peerIdentifier)
        {
            if (tunControl->ShortcutTunnelPeerCache[i].staleFlag)
            {
                // Remove stale entry

                tunControl->FreeNextHopEntry(i);
            }
            else
            {
                // Mark as stale

                tunControl->ShortcutTunnelPeerCache[i].staleFlag = true;
            }
        }
    }

    // Restart the timer.

    res = tunControl->mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(tunControl->mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                                                 PurgeStaleNextHopEntries, tunControl);
    VerifyOrDieWithMsg(res == WEAVE_NO_ERROR, WeaveTunnel, "Cannot start PurgeStaleNextHopEntries\n");
}

/* Start the nexthop cache monitor */
void WeaveTunnelControl::StartNextHopTableMonitor (void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                               PurgeStaleNextHopEntries, this);
}

/* Stop the nexthop cache monitor */
void WeaveTunnelControl::StopNextHopTableMonitor (void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->CancelTimer(PurgeStaleNextHopEntries, this);
}

/* Register the handlers for receving the shortcut tunnel advertisements for refreshing the nexthop cache */
void WeaveTunnelControl::RegisterShortcutTunnelAdvHandlers (void)
{
    mTunnelAgent->mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling, kMsgType_TunnelRouterAdvertise,
                                                                  HandleShortcutTunnelAdvertiseMessage, this);

    mTunnelAgent->mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling, kMsgType_TunnelMobileClientAdvertise,
                                                                  HandleShortcutTunnelAdvertiseMessage, this);
}

/* Unregister the handlers for receving the shortcut tunnel advertisements for refreshing the nexthop cache */
void WeaveTunnelControl::UnregisterShortcutTunnelAdvHandlers (void)
{
    mTunnelAgent->mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling, kMsgType_TunnelRouterAdvertise);

    mTunnelAgent->mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Tunneling, kMsgType_TunnelMobileClientAdvertise);
}

/* Start sending the periodic border router advertisement messages */
void WeaveTunnelControl::StartShortcutTunnelAdvertisementsFromBorderRouter(void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                               BorderRouterAdvTimeout, this);
}

/* Stop sending the periodic border router advertisement messages */
void WeaveTunnelControl::StopShortcutTunnelAdvertisementsFromBorderRouter(void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->CancelTimer(BorderRouterAdvTimeout, this);
}

/* Start sending the periodic mobile client advertisement messages */
void WeaveTunnelControl::StartShortcutTunnelAdvertisementsFromMobileClient(void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->StartTimer(mShortcutTunnelAdvInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                               MobileClientAdvTimeout, this);
}

/* Stop sending the periodic mobile client advertisement messages */
void WeaveTunnelControl::StopShortcutTunnelAdvertisementsFromMobileClient(void)
{
    mTunnelAgent->mExchangeMgr->MessageLayer->Inet->CancelTimer(MobileClientAdvTimeout, this);
}

/* Lookup NextHop table entry */
int WeaveTunnelControl::FindTunnelPeerEntry (uint64_t peerId)
{
    int index = -1;
    for (int i = 0; i < WEAVE_CONFIG_TUNNELING_MAX_NUM_SHORTCUT_TUNNEL_PEERS; i++)
    {
        if (peerId == ShortcutTunnelPeerCache[i].peerIdentifier)
        {
            index = i;
            break;
        }
    }

    return index;
}

/* Create a new route entry */
int WeaveTunnelControl::NewNextHopEntry (void)
{
    int retIndex = -1;

    for (int i = 0; i < WEAVE_CONFIG_TUNNELING_MAX_NUM_SHORTCUT_TUNNEL_PEERS; i++)
    {
        if (!ShortcutTunnelPeerCache[i].peerIdentifier)
        {
            retIndex = i;
            break;
        }
    }

    return retIndex;
}

/* Free a nexthop entry at a particular index */
WEAVE_ERROR WeaveTunnelControl::FreeNextHopEntry (int index)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((index < 0) || (index >= WEAVE_CONFIG_TUNNELING_MAX_NUM_SHORTCUT_TUNNEL_PEERS))
    {
        ExitNow();
    }

    memset(&ShortcutTunnelPeerCache[index], 0, sizeof(ShortcutTunnelPeerEntry));

exit:

    return err;
}

/* Lookup and update a nexthop entry or add a new entry */
WEAVE_ERROR WeaveTunnelControl::UpdateOrAddTunnelPeerEntry (uint64_t peerId, IPAddress peerAddress, uint64_t peerNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int8_t index = -1;
    // Check and update the ShortcutTunnelPeerCache

    index = FindTunnelPeerEntry(peerId);
    if (index < 0)
    {
        // Not found; Create a new entry

        index = NewNextHopEntry();
        if (index < 0)
        {
            ExitNow(err = WEAVE_ERROR_TUNNEL_NEXTHOP_TABLE_FULL);
        }
    }

    // Update the fields in the cache.

    ShortcutTunnelPeerCache[index].peerIdentifier = peerId;
    ShortcutTunnelPeerCache[index].peerAddr       = peerAddress;
    ShortcutTunnelPeerCache[index].peerNodeId     = peerNodeId;
    ShortcutTunnelPeerCache[index].staleFlag      = false;

exit:

    return err;
}
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED

/* Reset the members */
void WeaveTunnelControl::Free (void)
{
    mTunnelAgent               = NULL;
#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mShortcutTunExchangeCtxt   = NULL;
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    mServiceExchangeCtxt       = NULL;
    OnTunStatusRcvd            = NULL;
}

/* Create an Exchange Context for exchanging Weave Control messages */
WEAVE_ERROR WeaveTunnelControl::CreateContext (WeaveConnection *aConnection,
                                               ExchangeContext::MessageReceiveFunct onMsgRcvd)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ExchangeContext *exchangeCtx = NULL;

    VerifyOrExit(mTunnelAgent->mExchangeMgr != NULL,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    exchangeCtx = mTunnelAgent->mExchangeMgr->NewContext(aConnection, this);

    VerifyOrExit(exchangeCtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Assign the appropriate message receipt handler to the callback.

    exchangeCtx->OnMessageReceived = onMsgRcvd;

    exchangeCtx->ResponseTimeout   = mCtrlResponseTimeout * nl::Weave::System::kTimerFactor_milli_per_unit;
    exchangeCtx->OnResponseTimeout = TunCtrlRespTimeoutHandler;
    mServiceExchangeCtxt           = exchangeCtx;

exit:
    return err;
}

WEAVE_ERROR WeaveTunnelControl::VerifyAndParseStatusResponse(uint32_t profileId,
                                                             uint8_t msgType, PacketBuffer *payload,
                                                             StatusReport & outReport,
                                                             bool & outIsRoutingRestricted)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that message is a StatusReport

    VerifyOrExit(profileId == kWeaveProfile_Common, err = WEAVE_ERROR_INVALID_PROFILE_ID);

    VerifyOrExit(msgType == Common::kMsgType_StatusReport, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // Parse the StatusReport.

    err = StatusReport::parse(payload, outReport);
    SuccessOrExit(err);

    VerifyOrExit(outReport.mProfileId == kWeaveProfile_Common && outReport.mStatusCode == Common::kStatus_Success,
                 err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);

    // Check if there is TLV data in the StatusReport.

    if (outReport.mAdditionalInfo.theLength > 0)
    {
        err = ParseTunnelTLVData(outReport, outIsRoutingRestricted);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveTunnelControl::ParseTunnelTLVData(StatusReport & report, bool & outIsRoutingRestricted)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader tunReader;
    nl::Weave::TLV::TLVType OuterContainerType;
    const uint8_t *tlvData = NULL;
    uint16_t tlvDataLen;
    uint64_t tag = 0;

    tlvData = report.mAdditionalInfo.theData;
    tlvDataLen = report.mAdditionalInfo.theLength;

    // Verify that TLV data supplied by the Service is encapsulated in an anonymous
    // TLV structure. All anonymous TLV structures begin with an anonymous structure control
    // byte (0x15) and end with an end-of-container control byte (0x18).

    VerifyOrExit(tlvDataLen > 2, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(tlvData[0] == kTLVElementType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(tlvData[tlvDataLen - 1] == kTLVElementType_EndOfContainer,
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    tunReader.Init(tlvData, tlvDataLen);

    err = tunReader.Next();
    SuccessOrExit(err);

    err = tunReader.EnterContainer(OuterContainerType);
    SuccessOrExit(err);

    err = tunReader.Next();
    SuccessOrExit(err);

    tag = tunReader.GetTag();
    VerifyOrExit(nl::Weave::TLV::IsProfileTag(tag),
                 err = WEAVE_ERROR_INVALID_TLV_TAG);

    VerifyOrExit(nl::Weave::TLV::ProfileIdFromTag(tag) == kWeaveProfile_Tunneling,
                 err = WEAVE_ERROR_INVALID_TLV_TAG);

    VerifyOrExit(nl::Weave::TLV::TagNumFromTag(tag) == kTag_TunnelRoutingRestricted,
                 err = WEAVE_ERROR_INVALID_TLV_TAG);

    err = tunReader.Get(outIsRoutingRestricted);
    SuccessOrExit(err);

exit:
    return err;
}

/* Handle received control messages */
void WeaveTunnelControl::HandleTunnelOpenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                  const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                  uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    bool isRoutingRestricted = false;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    WeaveTunnelConnectionMgr *connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);

    err = VerifyAndParseStatusResponse(profileId, msgType, payload, report, isRoutingRestricted);

    // Free the payload buffer and close the ExchangeContext early to free up resources.

    tunControl->FreeBufferAndCloseExchange(payload, tunControl->mServiceExchangeCtxt);

    SuccessOrExit(err);

    // Received a Tunnel Open Ack; Set the connection state

    connMgr->mConnectionState = WeaveTunnelConnectionMgr::kState_TunnelOpen;

    // Reset the failed connection attempts after a successful TunnelOpen.

    connMgr->mTunFailedConnAttemptsInRow = 0;

    connMgr->mTunReconnectFibonacciIndex = 0;
#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    // Tunnel is up. Start the Tunnel Liveness timer

    connMgr->StartLivenessTimer();
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    // Call the TunnelOpen post processing function.

    tunControl->mTunnelAgent->WeaveTunnelConnectionUp(msgInfo, connMgr, isRoutingRestricted);

    // Stop the online check if it is running since tunnel is established.

    connMgr->StopOnlineCheck();

    // Set the current online state to True.

    connMgr->mIsNetworkOnline = true;


exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "HandleTunnelOpenResponse FAILED with error: %ld\n", (long)err);

        tunControl->TunnelCloseAndReportErrorStatus(connMgr, err, report);
    }

    return;
}

void WeaveTunnelControl::HandleTunnelCloseResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                   const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                   uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    bool isRoutingRestricted = false;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    WeaveTunnelConnectionMgr *connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);

    err = VerifyAndParseStatusResponse(profileId, msgType, payload, report, isRoutingRestricted);

    // Free the payload buffer and close the ExchangeContext early to free up resources.

    tunControl->FreeBufferAndCloseExchange(payload, tunControl->mServiceExchangeCtxt);

    SuccessOrExit(err);

    // Received successful response to a Tunnel control message


#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    // Tunnel is closed. Stop the Tunnel Liveness timer

    connMgr->StopLivenessTimer();
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    // Close the connection; Do not restart the connection as we had proactively issued a
    // Tunnel Close to the peer. The application would need to start the tunnel explicitly.

    connMgr->StopServiceTunnelConn(WEAVE_NO_ERROR);

    // Received a Tunnel Close Ack: Call the TunnelClose post processing function.

    tunControl->mTunnelAgent->WeaveTunnelConnectionDown(connMgr, WEAVE_NO_ERROR);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "HandleTunnelCloseResponse FAILED with error: %ld\n", (long)err);

        tunControl->TunnelCloseAndReportErrorStatus(connMgr, err, report);
    }

    return;
}

void WeaveTunnelControl::HandleTunnelRouteUpdateResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                         const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                         uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    bool isRoutingRestricted = false;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    WeaveTunnelConnectionMgr *connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);

    err = VerifyAndParseStatusResponse(profileId, msgType, payload, report, isRoutingRestricted);

    // Free the payload buffer and close the ExchangeContext early to free up resources.

    tunControl->FreeBufferAndCloseExchange(payload, tunControl->mServiceExchangeCtxt);

    SuccessOrExit(err);

    // Received successful response to a Tunnel control message

    // The connection state is kState_TunnelOpen

    connMgr->mConnectionState = WeaveTunnelConnectionMgr::kState_TunnelOpen;

    if (isRoutingRestricted)
    {
        // Although tunnel is restricted, it is still open but can only be
        // usable by the border gateway for itself to access a limited set of
        // Service endpoints. The device is put in this mode, typically, when
        // it is removed from the account.
        connMgr->mTunAgent->DisableBorderRouting();

        WeaveLogDetail(WeaveTunnel, "Tunnel in restricted mode; Not operating as a Border Router\n");
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "HandleTunnelRouteUpdateResponse FAILED with error: %ld\n", (long)err);

        tunControl->TunnelCloseAndReportErrorStatus(connMgr, err, report);
    }

    return;
}

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
void WeaveTunnelControl::HandleTunnelLivenessResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                                      const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                                      uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    bool isRoutingRestricted = false;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    WeaveTunnelConnectionMgr *connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);

    err = VerifyAndParseStatusResponse(profileId, msgType, payload, report, isRoutingRestricted);

    // Free the payload buffer and close the ExchangeContext early to free up resources.

    tunControl->FreeBufferAndCloseExchange(payload, tunControl->mServiceExchangeCtxt);

    SuccessOrExit(err);

    // Received successful response to a Tunnel control message

    // Tunnel is alive. Schedule the next Tunnel Liveness timer.

    connMgr->StartLivenessTimer();

    // Notify the application of the successful Liveness probe response.

    connMgr->mTunAgent->NotifyTunnelLiveness(connMgr->mTunType, WEAVE_NO_ERROR);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "HandleTunnelLivenessResponse FAILED with error: %ld\n", (long)err);

        tunControl->TunnelCloseAndReportErrorStatus(connMgr, err, report);
    }

    return;
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Handler to reconnect with the peer node.
 *
 */
void WeaveTunnelControl::HandleTunnelReconnect(ExchangeContext *ec, const IPPacketInfo *pktInfo,
                                               const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                               uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveTunnelConnectionMgr *connMgr = NULL;
    WeaveTunnelControl *tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);
    uint16_t hostPort = 0;
    char hostName[255]; // Per spec, max DNS name length is 253.
    uint8_t hostLen = sizeof(hostName);
    uint16_t payloadLen = 0;
    ReconnectParam reconnParam;

    VerifyOrExit(ec->Con != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);

    payloadLen = payload->DataLength();

    // Set the connection state

    connMgr->mConnectionState = WeaveTunnelConnectionMgr::kState_ReconnectRecvd;

    // Send a status report

    err = SendStatusReport(ec, kWeaveProfile_Common, Common::kStatus_Success);
    SuccessOrExit(err);

    // Set the hostname to NULL string

    hostName[0] = '\0';

    if (payloadLen)
    {
        // Fetch the hostname and port

        err = tunControl->DecodeTunnelReconnect(hostPort, hostName, hostLen, payload);
        SuccessOrExit(err);
    }

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (tunControl->mTunnelAgent->mServiceMgr)
    {
        if (0 == payloadLen)
        {
            // No hostname specified. Clear cache and force trip to directory server
            // during reconnect.

            tunControl->mTunnelAgent->mServiceMgr->clearCache();

        }
        else
        {
            // Received a new hostname and port. Add a new Service directory entry

            err = tunControl->mTunnelAgent->mServiceMgr->replaceOrAddCacheEntry(hostPort, hostName, hostLen,
                                                                                kServiceEndpoint_WeaveTunneling);
            if (err != WEAVE_NO_ERROR)
            {
                tunControl->mTunnelAgent->mServiceMgr->clearCache();

                err = WEAVE_NO_ERROR;

                ExitNow();
            }
        }
    }
    else
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    {
        // Store the Service address for connecting directly if the hostname is
        // a DNS resolved IP address in string format.

        if (IPAddress::FromString(hostName, tunControl->mTunnelAgent->mServiceAddress))
        {
            // store the port also if the hostName is a resolved IP address string.

            tunControl->mTunnelAgent->mServicePort = hostPort;
        }
    }

exit:

    tunControl->FreeBufferAndCloseExchange(payload, ec);

    if (connMgr)
    {
        // Reset the failed connection attempts counter as this is a fresh reconnect.

        connMgr->mTunFailedConnAttemptsInRow = 0;

        connMgr->mTunReconnectFibonacciIndex = 0;

        reconnParam.PopulateReconnectParam(err);

        connMgr->StopAndReconnectTunnelConn(reconnParam);

        tunControl->mTunnelAgent->WeaveTunnelServiceReconnectRequested(connMgr, hostName, hostPort);
    }

    return;
}

void WeaveTunnelControl::TunnelCloseAndReportErrorStatus(WeaveTunnelConnectionMgr *connMgr,
                                                         WEAVE_ERROR err,
                                                         StatusReport report)
{

    ReconnectParam reconnParam;
#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
     // Tunnel is being closed. Stop the Tunnel Liveness timer.

     connMgr->StopLivenessTimer();
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    // Report the status

    if (OnTunStatusRcvd != NULL)
    {
        OnTunStatusRcvd(connMgr->mTunType, report);
    }

    if (connMgr->mConnectionState == WeaveTunnelConnectionMgr::kState_TunnelClosing)
    {
        // Close the connection

        connMgr->StopServiceTunnelConn(err);

        // Callback to WeaveTunnelAgent to notify application if it has already closed the tunnel.

        mTunnelAgent->WeaveTunnelConnectionDown(connMgr, err);
    }
    else
    {
        // Close the connection and reconnect

        reconnParam.PopulateReconnectParam(err,
                                           report.mProfileId,
                                           report.mStatusCode);

        connMgr->StopAndReconnectTunnelConn(reconnParam);
    }

    return;
}

void WeaveTunnelControl::FreeBufferAndCloseExchange(PacketBuffer *buf, ExchangeContext *&ec)
{
    // Free the payload buffer.

    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    // Discard the exchange context.
    if (ec)
    {
        ec->Close();
        ec = NULL;
    }

    return;
}

/* Send a tunnel control status report message */
WEAVE_ERROR WeaveTunnelControl::SendStatusReport(ExchangeContext *ec, uint32_t profileId,
                                                 uint16_t tunStatusCode)
{
    WEAVE_ERROR     err;

    err = nl::Weave::WeaveServerBase::SendStatusReport(ec, profileId, tunStatusCode, WEAVE_NO_ERROR, 0);

    return err;
}


/* Send the Tunnel Route Control message of specified type */
WEAVE_ERROR WeaveTunnelControl::SendTunnelMessage(TunnelCtrlMsgType msgType,
                                                  WeaveTunnelConnectionMgr *conMgr,
                                                  uint64_t fabricId,
                                                  WeaveTunnelRoute *tunRoutes,
                                                  ExchangeContext::MessageReceiveFunct onMsgRcvd)
{
    WEAVE_ERROR     err        = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf     = NULL;
    uint8_t         *p         = NULL;

    // allocate buffer and send tunnel message

    msgBuf = PacketBuffer::New();

    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Create an ExchangeContext

    err = CreateContext(conMgr->mServiceCon, onMsgRcvd);
    SuccessOrExit(err);

    // A Tunnel Liveness is an empty tunnel control message.

    if (msgType != kMsgType_TunnelLiveness)
    {

        if (msgType == kMsgType_TunnelOpenV2)
        {
            p = msgBuf->Start();

            VerifyOrExit(msgBuf->AvailableDataLength() >= NL_TUNNEL_AGENT_ROLE_SIZE_IN_BYTES +
                         NL_TUNNEL_TYPE_SIZE_IN_BYTES + NL_TUNNEL_SRC_INTF_TYPE_SIZE_IN_BYTES +
                         NL_TUNNEL_LIVENESS_TYPE_SIZE_IN_BYTES + NL_TUNNEL_LIVENESS_MAX_TIMEOUT_SIZE_IN_BYTES,
                         err = WEAVE_ERROR_BUFFER_TOO_SMALL);

            // Encode the tunnel device role, tunnel type, and source interface type in the TunnelOpen message.

            Write8(p, conMgr->mTunAgent->mRole);

            Write8(p, conMgr->mTunType);

            Write8(p, conMgr->mSrcInterfaceType);

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
            Write8(p, kLiveness_TunnelControl);

            LittleEndian::Write16(p, conMgr->mTunnelLivenessInterval);
#elif WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
            Write8(p, kLiveness_TCPKeepAlive);

            LittleEndian::Write16(p, conMgr->mKeepAliveIntervalSecs * (conMgr->mMaxNumProbes + 1));
#endif

            // Tunnel device role(1 byte) + Tunnel type(1 byte) + Tunnel source interface type(1 byte) +
            // Tunnel liveness strategy(1 byte) + Tunnel Liveness Max timeout(2 bytes)

            msgBuf->SetDataLength(1 + 1 + 1 + 1 + 2);
        }

        err = WeaveTunnelRoute::EncodeFabricTunnelRoutes(fabricId,
                                                         tunRoutes, msgBuf);
        SuccessOrExit(err);
    }

    err = mServiceExchangeCtxt->SendMessage(kWeaveProfile_Tunneling, msgType, msgBuf,
                                            ExchangeContext::kSendFlag_ExpectResponse);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (err != WEAVE_NO_ERROR && mServiceExchangeCtxt != NULL)
    {
        mServiceExchangeCtxt->Close();
        mServiceExchangeCtxt = NULL;
    }

    return err;

}

/**
 * Tunnel control message response timeout handler.
 */
void WeaveTunnelControl::TunCtrlRespTimeoutHandler(ExchangeContext *ec)
{
    WeaveTunnelConnectionMgr *connMgr = NULL;
    WeaveTunnelControl *tunControl    = NULL;
    ReconnectParam reconnParam;

    tunControl = static_cast<WeaveTunnelControl *>(ec->AppState);

    if (ec->Con)
    {
        connMgr = static_cast<WeaveTunnelConnectionMgr *>(ec->Con->AppState);
    }

    // Close the ExchangeContext

    ec->Close();

    // Set the ExchangeContext for this connMgr to NULL;
    ec = NULL;
    tunControl->mServiceExchangeCtxt = NULL;

    if (connMgr)
    {
        if (connMgr->mConnectionState == WeaveTunnelConnectionMgr::kState_TunnelClosing)
        {
            // Stop the connection.

            connMgr->StopServiceTunnelConn(WEAVE_ERROR_TIMEOUT);

            // Callback to WeaveTunnelAgent to notify application if it has already closed the tunnel.

            tunControl->mTunnelAgent->WeaveTunnelConnectionDown(connMgr, WEAVE_ERROR_TIMEOUT);
        }
        else
        {
            // Stop the connection and attempt to reconnect.

            reconnParam.PopulateReconnectParam(WEAVE_ERROR_TIMEOUT);

            connMgr->StopAndReconnectTunnelConn(reconnParam);
        }
    }
}

/**
 * Decode the Tunnel Reconnect message from the Service and update the
 * Service Directory with the new Tunnel Endpoint hostname and port.
 *
 * @note
 *   hostNameLen is passed in with the size of the buffer hostName and
 *   is set to the length of the hostName(decoded from the PacketBuffer)
 *   by this function as output.
 *
 */
WEAVE_ERROR WeaveTunnelControl::DecodeTunnelReconnect(uint16_t &hostPort,
                                                      char *hostName,
                                                      uint8_t &hostNameLen,
                                                      PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t msgLen = msg->DataLength();
    uint8_t *p = NULL;

    p = msg->Start();

    // Verify that we can read the port, which is 2 bytes.

    VerifyOrExit(msgLen > 2, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    hostPort = LittleEndian::Read16(p);

    // Verify that the buffer has enough space for the hostname

    VerifyOrExit(hostNameLen > msgLen - 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Assign the actual length of the hostname.

    hostNameLen = msgLen - 2;

    // Read the hostname string into the buffer provided

    memcpy(hostName, p, hostNameLen);
    hostName[hostNameLen] = '\0';

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_TUNNELING

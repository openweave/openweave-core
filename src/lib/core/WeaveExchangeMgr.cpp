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
 *      This file implements the WeaveExchangeManager class.
 *
 */


#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <SystemLayer/SystemTimer.h>
#include <SystemLayer/SystemStats.h>

namespace nl {
namespace Weave {

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Encoding;

/**
 *  Constructor for the WeaveExchangeManager class.
 *  It sets the state to kState_NotInitialized.
 *
 *  @note
 *    The class must be initialized via WeaveExchangeManager::Init()
 *    prior to use.
 *
 */
WeaveExchangeManager::WeaveExchangeManager()
{
    State = kState_NotInitialized;
}

/**
 *  Initialize the WeaveExchangeManager object. Within the lifetime
 *  of this instance, this method is invoked once after object
 *  construction until a call to Shutdown is made to terminate the
 *  instance.
 *
 *  @param[in]    msgLayer    A pointer to the WeaveMessageLayer object.
 *
 *  @retval #WEAVE_ERROR_INCORRECT_STATE If the state is not equal to
 *          kState_NotInitialized.
 *  @retval #WEAVE_NO_ERROR On success.
 *
 */
WEAVE_ERROR WeaveExchangeManager::Init(WeaveMessageLayer *msgLayer)
{
    if (State != kState_NotInitialized)
        return WEAVE_ERROR_INCORRECT_STATE;

    MessageLayer = msgLayer;
    FabricState = msgLayer->FabricState;

    NextExchangeId = GetRandU16();

    memset(ContextPool, 0, sizeof(ContextPool));
    mContextsInUse = 0;

    InitBindingPool();

    memset(UMHandlerPool, 0, sizeof(UMHandlerPool));
    OnExchangeContextChanged = NULL;

    msgLayer->ExchangeMgr = this;
    msgLayer->OnMessageReceived = HandleMessageReceived;
    msgLayer->OnAcceptError = HandleAcceptError;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mWRMPTimerInterval  = WEAVE_CONFIG_WRMP_TIMER_DEFAULT_PERIOD;       //WRMP Timer tick period

    memset(RetransTable, 0, sizeof(RetransTable));

    mWRMPTimeStampBase = System::Timer::GetCurrentEpoch();

    mWRMPCurrentTimerExpiry = 0;
#endif

    State = kState_Initialized;

    return WEAVE_NO_ERROR;
}

/**
 *  Shutdown the WeaveExchangeManager. This terminates this instance
 *  of the object and releases all held resources.
 *
 *  @note
 *     The application should only call this function after ensuring that
 *     there are no active ExchangeContext objects. Furthermore, it is the
 *     onus of the application to de-allocate the WeaveExchangeManager
 *     object after calling WeaveExchangeManager::Shutdown().
 *
 *  @return #WEAVE_NO_ERROR unconditionally.
 *
 */
WEAVE_ERROR WeaveExchangeManager::Shutdown()
{
    if (MessageLayer != NULL)
    {
        if (MessageLayer->ExchangeMgr == this)
        {
            MessageLayer->ExchangeMgr = NULL;
            MessageLayer->OnMessageReceived = NULL;
            MessageLayer->OnAcceptError = NULL;
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        WRMPStopTimer();

        //Clear the retransmit table
        for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
        {
            ClearRetransmitTable(RetransTable[i]);
        }
#endif
        MessageLayer = NULL;
    }

    OnExchangeContextChanged = NULL;

    FabricState = NULL;

    State = kState_NotInitialized;

    return WEAVE_NO_ERROR;
}

/**
 *  Creates a new ExchangeContext with a given peer Weave node specified by the peer node identifier.
 *
 *  @param[in]    peerNodeId    The node identifier of the peer with which the ExchangeContext is being set up.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @return   A pointer to the created ExchangeContext object On success. Otherwise NULL if no object
 *            can be allocated or is available.
 *
 */
ExchangeContext *WeaveExchangeManager::NewContext(const uint64_t &peerNodeId, void *appState)
{
    return NewContext(peerNodeId, FabricState->SelectNodeAddress(peerNodeId), WEAVE_PORT, INET_NULL_INTERFACEID, appState);
}

/**
 *  Creates a new ExchangeContext with a given peer Weave node specified by the peer node identifier
 *  and peer IP address.
 *
 *  @param[in]    peerNodeId    The node identifier of the peer with which the ExchangeContext is being set up.
 *
 *  @param[in]    peerAddr      The IP address of the peer node.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @return   A pointer to the created ExchangeContext object On success. Otherwise, NULL if no object
 *            can be allocated or is available.
 *
 */
ExchangeContext *WeaveExchangeManager::NewContext(const uint64_t &peerNodeId, const IPAddress &peerAddr, void *appState)
{
    return NewContext(peerNodeId, peerAddr, WEAVE_PORT, INET_NULL_INTERFACEID, appState);
}

/**
 *  Creates a new ExchangeContext with a given peer Weave node specified by the peer node identifier, peer IP address,
 *  and destination port on a specified interface.
 *
 *  @param[in]    peerNodeId    The node identifier of the peer with which the ExchangeContext is being set up.
 *
 *  @param[in]    peerAddr      The IP address of the peer node.
 *
 *  @param[in]    peerPort      The port of the peer node.
 *
 *  @param[in]    sendIntfId    The interface to use for sending Weave messages on this exchange.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @return   A pointer to the created ExchangeContext object On success. Otherwise, NULL if no object
 *            can be allocated or is available.
 *
 */
ExchangeContext *WeaveExchangeManager::NewContext(const uint64_t &peerNodeId, const IPAddress &peerAddr, uint16_t peerPort, InterfaceId sendIntfId, void *appState)
{
    ExchangeContext *ec = AllocContext();
    if (ec != NULL)
    {
        ec->ExchangeId = NextExchangeId++;
        ec->PeerNodeId = peerNodeId;
        ec->PeerAddr = peerAddr;
        ec->PeerPort = (peerPort != 0) ? peerPort : WEAVE_PORT;
        ec->PeerIntf = sendIntfId;
        ec->AppState = appState;
        ec->SetInitiator(true);
        //Initialize WRMP variables
        ec->mMsgProtocolVersion = 0;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // No need to set WRMP timer, this will be done when we add to retrans table
        ec->mWRMPNextAckTime = 0;
        ec->SetAckPending(false);
        ec->SetMsgRcvdFromPeer(false);
        ec->mWRMPConfig = gDefaultWRMPConfig;
        ec->mWRMPThrottleTimeout = 0;
        //Internal and for Debug Only; When set, Exchange Layer does not send Ack.
        ec->SetDropAck(false);
        //Initialize the App callbacks to NULL
        ec->OnThrottleRcvd = NULL;
        ec->OnDDRcvd = NULL;
        ec->OnAckRcvd = NULL;
        ec->OnSendError = NULL;
#endif
#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
        ec->SetUseEphemeralUDPPort(MessageLayer->EphemeralUDPPortEnabled());
#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
        WeaveLogProgress(ExchangeManager, "ec id: %d, AppState: 0x%x", EXCHANGE_CONTEXT_ID(ec - ContextPool), ec->AppState);
    }
    return ec;
}

/**
 *  Creates a new ExchangeContext with a given peer Weave node over a specified WeaveConnection.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object representing the TCP connection
 *                              with the peer.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @return   A pointer to the created ExchangeContext object On success. Otherwise, NULL if no object
 *            can be allocated or is available.
 *
 */
ExchangeContext *WeaveExchangeManager::NewContext(WeaveConnection *con, void *appState)
{
    ExchangeContext *ec = NewContext(con->PeerNodeId, con->PeerAddr, con->PeerPort, INET_NULL_INTERFACEID, appState);
    if (ec != NULL)
    {
        ec->Con = con;
        ec->KeyId = con->DefaultKeyId;
        ec->EncryptionType = con->DefaultEncryptionType;
    }
    return ec;
}

/**
 *  Find the ExchangeContext from a pool matching a given set of parameters.
 *
 *  @param[in]    peerNodeId    The node identifier of the peer with which the ExchangeContext has been set up.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object representing the TCP connection
 *                              with the peer.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @param[in]    isInitiator   Boolean indicator of whether the local node is the initiator of the exchange.
 *
 *  @return   A pointer to the ExchangeContext object matching the provided parameters On success, NULL on no match.
 *
 */
ExchangeContext *WeaveExchangeManager::FindContext(uint64_t peerNodeId, WeaveConnection *con, void *appState, bool isInitiator)
{
    ExchangeContext *ec = (ExchangeContext *) ContextPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
        if (ec->ExchangeMgr != NULL && ec->PeerNodeId == peerNodeId &&
            ec->Con == con && ec->AppState == appState &&
            ec->IsInitiator() == isInitiator)
            return ec;
    return NULL;
}

/**
 *  Register an unsolicited message handler for a given profile identifier. This handler would be
 *  invoked for all messages of the given profile.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId,
        ExchangeContext::MessageReceiveFunct handler, void *appState)
{
    return RegisterUMH(profileId, (int16_t) -1, NULL, false, handler, appState);
}

/**
 *  Register an unsolicited message handler for a given profile identifier. This handler would be invoked for all messages of the given profile.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    allowDups     Boolean indicator of whether duplicate messages are allowed for a given profile.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId,
        ExchangeContext::MessageReceiveFunct handler, bool allowDups, void *appState)
{
    return RegisterUMH(profileId, (int16_t) -1, NULL, allowDups, handler, appState);
}

/**
 *  Register an unsolicited message handler for a given profile identifier and message type.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType,
        ExchangeContext::MessageReceiveFunct handler, void *appState)
{
    return RegisterUMH(profileId, (int16_t) msgType, NULL, false, handler, appState);
}

/**
 *  Register an unsolicited message handler for a given profile identifier and message type.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    allowDups     Boolean indicator of whether duplicate messages are allowed for a given
 *                              profile identifier and message type.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType,
        ExchangeContext::MessageReceiveFunct handler, bool allowDups, void *appState)
{
    return RegisterUMH(profileId, (int16_t) msgType, NULL, allowDups, handler, appState);
}

/**
 *  Register an unsolicited message handler for a given profile identifier, message type on a specified Weave
 *  connection.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object representing the TCP connection
 *                              with the peer.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType, WeaveConnection *con,
        ExchangeContext::MessageReceiveFunct handler, void *appState)
{
    return RegisterUMH(profileId, (int16_t) msgType, con, false, handler, appState);
}

/**
 *  Register an unsolicited message handler for a given profile identifier, message type on a specified Weave
 *  connection.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object representing the TCP connection
 *                              with the peer.
 *
 *  @param[in]    handler       The unsolicited message handler.
 *
 *  @param[in]    allowDups     Boolean indicator of whether duplicate messages are allowed for a given
 *                              profile identifier, message type on a specified Weave connection.
 *
 *  @param[in]    appState      A pointer to a higher layer object that holds context state.
 *
 *  @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS If the unsolicited message handler pool
 *                                                             is full and a new one cannot be allocated.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::RegisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType, WeaveConnection *con,
        ExchangeContext::MessageReceiveFunct handler, bool allowDups, void *appState)
{
    return RegisterUMH(profileId, (int16_t) msgType, con, allowDups, handler, appState);
}

/**
 *  Unregister an unsolicited message handler for a given profile identifier.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @retval #WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER  If the matching unsolicited message handler
 *                                                       is not found.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::UnregisterUnsolicitedMessageHandler(uint32_t profileId)
{
    return UnregisterUMH(profileId, (int16_t) -1, NULL);
}

/**
 *  Unregister an unsolicited message handler for a given profile identifier and message type.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @retval #WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER  If the matching unsolicited message handler
 *                                                       is not found.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::UnregisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType)
{
    return UnregisterUMH(profileId, (int16_t) msgType, NULL);
}

/**
 *  Unregister an unsolicited message handler for a given profile identifier, message type, and Weave connection.
 *
 *  @param[in]    profileId     The profile identifier of the received message.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    con           A pointer to the WeaveConnection object representing the TCP connection
 *                              with the peer.
 *
 *  @retval #WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER  If the matching unsolicited message handler
 *                                                       is not found.
 *  @retval #WEAVE_NO_ERROR On success.
 */
WEAVE_ERROR WeaveExchangeManager::UnregisterUnsolicitedMessageHandler(uint32_t profileId, uint8_t msgType, WeaveConnection *con)
{
    return UnregisterUMH(profileId, (int16_t) msgType, con);
}

void WeaveExchangeManager::HandleAcceptError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err)
{
    WeaveLogError(ExchangeManager, "Accept FAILED, err = %s", ErrorStr(err));
}

void WeaveExchangeManager::HandleConnectionReceived(WeaveConnection *con)
{
    // Hook the OnMessageReceived callback for new inbound connections.
    con->OnMessageReceived = HandleMessageReceived;
}

void WeaveExchangeManager::HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    for (int i = 0; i < WEAVE_CONFIG_MAX_BINDINGS; i++)
    {
        BindingPool[i].OnConnectionClosed(con, conErr);
    }

    ExchangeContext *ec = (ExchangeContext *) ContextPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
        if (ec->ExchangeMgr != NULL && ec->Con == con)
        {
            ec->HandleConnectionClosed(conErr);
        }

    UnsolicitedMessageHandler *umh = (UnsolicitedMessageHandler *) UMHandlerPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS; i++, umh++)
        if (umh->Handler != NULL && umh->Con == con)
        {
            SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kExchangeMgr_NumUMHandlers);
            umh->Handler = NULL;
        }
}

/**
 * Expire the timers started by ExchangeContext instances.
 * This function is not meant to be used in production code.
 *
 * @return Number of timers found running.
 */
#if WEAVE_CONFIG_TEST
size_t WeaveExchangeManager::ExpireExchangeTimers(void)
{
    size_t retval = 0;
    ExchangeContext *ec = (ExchangeContext *) ContextPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL)
        {
            if (ec->ResponseTimeout)
            {
                ec->CancelResponseTimer();
                ec->ResponseTimeout = 1;
                ec->StartResponseTimer();
                retval++;
            }
        }
    }
    return retval;
}
#endif

ExchangeContext *WeaveExchangeManager::AllocContext()
{
    ExchangeContext *ec = (ExchangeContext *) ContextPool;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_AllocExchangeContext,
                       return NULL);

    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
        if (ec->ExchangeMgr == NULL)
        {
            *ec = ExchangeContext();
            ec->ExchangeMgr = this;
            ec->mRefCount = 1;
            mContextsInUse++;
            MessageLayer->SignalMessageLayerActivityChanged();
#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
            WeaveLogProgress(ExchangeManager, "ec++ id: %d, inUse: %d, addr: 0x%x", EXCHANGE_CONTEXT_ID(ec - ContextPool), mContextsInUse, ec);
#endif
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kExchangeMgr_NumContexts);

            return ec;
        }
    WeaveLogError(ExchangeManager, "Alloc ctxt FAILED");
    return NULL;
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
void WeaveExchangeManager::WRMPProcessDDMessage(uint32_t PauseTimeMillis, uint64_t DelayedNodeId)
{
    // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
    WRMPExpireTicks();

    //Go through the retrans table entries for that node and adjust the timer.
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        //Exchcontext is the sentinel object to ascertain validity of the element
        if (RetransTable[i].exchContext)
        {
            //Adjust the retrans timer value if Delayed Node identifier matches Peer in ExchangeContext
            if (DelayedNodeId == RetransTable[i].exchContext->PeerNodeId)
            {

                //Paustime is specified in milliseconds; Update retrans values
                RetransTable[i].nextRetransTime += (PauseTimeMillis / mWRMPTimerInterval);

                //Call the application callback
                if (RetransTable[i].exchContext->OnDDRcvd)
                {
                    RetransTable[i].exchContext->OnDDRcvd(RetransTable[i].exchContext,
                                                          PauseTimeMillis);
                }
                else
                {
                    WeaveLogError(ExchangeManager,
                                  "No App Handler for Delayed Delivery for ExchangeContext with Id %04" PRIX16,
                                  RetransTable[i].exchContext->ExchangeId);

                }
            }//DelayedNodeId == PeerNodeId
        }//exchContext
    }//for loop in table entry

    // Schedule next physical wakeup
    WRMPStartTimer();
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

static void DefaultOnMessageReceived(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload)
{
    WeaveLogError(ExchangeManager,
            "Dropping unexpected message %08" PRIX32 ":%d %04" PRIX16 " MsgId:%08" PRIX32,
            profileId, msgType, ec->ExchangeId, msgInfo->MessageId);

    PacketBuffer::Free(payload);
}

void WeaveExchangeManager::DispatchMessage(WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    WeaveExchangeHeader exchangeHeader;
    UnsolicitedMessageHandler *umh         = NULL;
    UnsolicitedMessageHandler *matchingUMH = NULL;
    ExchangeContext *ec                    = NULL;
    WeaveConnection *msgCon                = NULL;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    const uint8_t      *p                  = NULL;
    uint32_t     PauseTimeMillis           = 0;
    uint64_t     DelayedNodeId             = 0;
    bool dupMsg;
    bool msgNeedsAck;
    bool sendAckAndCloseExchange;
#endif
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    bool isMsgCounterSyncResp;
    bool peerGroupMsgIdNotSynchronized;
#endif
    WEAVE_ERROR  err                       = WEAVE_NO_ERROR;

    // Decode the exchange header.
    err = DecodeHeader(&exchangeHeader, msgInfo, msgBuf);
    SuccessOrExit(err);

    //Check if the version is supported
    if ((msgInfo->MessageVersion != kWeaveMessageVersion_V1) &&
        (msgInfo->MessageVersion != kWeaveMessageVersion_V2))
    {
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_MESSAGE_VERSION);
    }

    // Notify Weave Security Manager that encrypted message has been received.
    if (msgInfo->EncryptionType != kWeaveEncryptionType_None)
    {
        MessageLayer->SecurityMgr->OnEncryptedMsgRcvd(msgInfo->KeyId, msgInfo->SourceNodeId, msgInfo->EncryptionType);
    }

    msgCon = msgInfo->InCon;

    WeaveLogRetain(ExchangeManager, "Msg %s %08" PRIX32 ":%d %d %016" PRIX64 " %04" PRIX16 " %04" PRIX16 " %ld MsgId:%08" PRIX32,
                   "rcvd", exchangeHeader.ProfileId, exchangeHeader.MessageType,
                   (int)msgBuf->DataLength(), msgInfo->SourceNodeId, ((msgCon == NULL) ? -1 : msgCon->LogId()), exchangeHeader.ExchangeId,
                   (long)err, msgInfo->MessageId);

#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    isMsgCounterSyncResp = exchangeHeader.ProfileId == nl::Weave::Profiles::kWeaveProfile_Security &&
                           exchangeHeader.MessageType == nl::Weave::Profiles::Security::kMsgType_MsgCounterSyncResp;
    peerGroupMsgIdNotSynchronized = (msgInfo->Flags & kWeaveMessageFlag_PeerGroupMsgIdNotSynchronized) != 0;

    // If received message is a MsgCounterSyncResp process it first.
    if (isMsgCounterSyncResp)
    {
        MessageLayer->SecurityMgr->HandleMsgCounterSyncRespMsg(msgInfo, msgBuf);
        msgBuf = NULL;
    }

    // If message counter synchronization was requested.
    if ((msgInfo->Flags & kWeaveMessageFlag_MsgCounterSyncReq) != 0)
    {
        MessageLayer->SecurityMgr->SendMsgCounterSyncResp(msgInfo, msgInfo->InPacketInfo);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // Retransmit all pending messages that were encrypted with application group key.
        RetransPendingAppGroupMsgs(msgInfo->SourceNodeId);
#endif
    }
    // Otherwise, if received message is not MsgCounterSyncResp and peer's message counter synchronization is needed.
    else if (!isMsgCounterSyncResp && peerGroupMsgIdNotSynchronized)
    {
        MessageLayer->SecurityMgr->SendSolitaryMsgCounterSyncReq(msgInfo, msgInfo->InPacketInfo);
    }

    // Exit now without error if received MsgCounterSyncResp message.
    if (isMsgCounterSyncResp)
    {
        ExitNow();
    }
#endif // WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    //Received Delayed Delivery Message: Extend time for pending retrans objects
    if (exchangeHeader.ProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
        exchangeHeader.MessageType == nl::Weave::Profiles::Common::kMsgType_WRMP_Delayed_Delivery)
    {
        // Process Delayed Delivery message if it is not a duplicate.
        if ((msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage) == 0)
        {
            p = msgBuf->Start();

            PauseTimeMillis = LittleEndian::Read32(p);
            DelayedNodeId = LittleEndian::Read64(p);

            WRMPProcessDDMessage(PauseTimeMillis, DelayedNodeId);
        }

        //Return after processing Delayed Delivery message
        ExitNow(err = WEAVE_NO_ERROR);
    }//If delayed delivery Msg
#endif

    // Search for an existing exchange that the message applies to. If a match is found...
    ec = (ExchangeContext *) ContextPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL && ec->MatchExchange(msgCon, msgInfo, &exchangeHeader))
        {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            // Found a matching exchange. Set flag for correct subsequent WRM
            // retransmission timeout selection.
            if (!ec->HasRcvdMsgFromPeer())
            {
                ec->SetMsgRcvdFromPeer(true);
            }
#endif

            //Matched ExchangeContext; send to message handler.
            ec->HandleMessage(msgInfo, &exchangeHeader, msgBuf);

            msgBuf = NULL;

            ExitNow(err = WEAVE_NO_ERROR);
        }
    }

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    // Is message a duplicate that needs ack.
    msgNeedsAck = exchangeHeader.Flags & kWeaveExchangeFlag_NeedsAck;
    dupMsg = (msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage);
#endif

    // Search for an unsolicited message handler if it marked as being sent by an initiator. Since we didn't
    // find an existing exchange that matches the message, it must be an unsolicited message. However all
    // unsolicited messages must be marked as being from an initiator.
    if (exchangeHeader.Flags & kWeaveExchangeFlag_Initiator)
    {
        // Search for an unsolicited message handler that can handle the message. Prefer handlers that can explicitly
        // handle the message type over handlers that handle all messages for a profile.
        umh = (UnsolicitedMessageHandler *) UMHandlerPool;

        matchingUMH = NULL;

        for (int i = 0; i < WEAVE_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS; i++, umh++)
            if (umh->Handler != NULL && umh->ProfileId == exchangeHeader.ProfileId && (umh->Con == NULL || umh->Con == msgCon)
                && (!(msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage) || umh->AllowDuplicateMsgs))
            {
                if (umh->MessageType == exchangeHeader.MessageType)
                {
                    matchingUMH = umh;
                    break;
                }

                if (umh->MessageType == -1)
                    matchingUMH = umh;
            }
    }
    // Discard the message if it isn't marked as being sent by an initiator and the message is not a duplicate
    // that needs to send ack to the peer.
    else
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (!msgNeedsAck)
#endif
            ExitNow(err = WEAVE_ERROR_UNSOLICITED_MSG_NO_ORIGINATOR);
    }

    // If no existing exchange that the message applies to was found we need to create
    // a new exchange context (EC) in the following cases:
    //
    //   (Dup.) Msg |  UMH is  |  Allow  | Need Peer |              Action
    //   Needs Ack  |  Found   |   Dup.  | MsgIdSync |
    // ----------------------------------------------------------------------------------------------------------
    //       Y      |     Y    |    Y    |     -     | Create EC, ec->HandleMessage() sends Dup ack and App callback.
    //       Y      |     Y    |    N    |     N     | Create EC; ec->HandleMessage() sends Dup ack; Close EC.
    //       Y      |     N    |    -    |     N     | Create EC, ec->HandleMessage() sends Dup ack; Close EC.
    //       N      |     Y    |    -    |     -     | Create EC, ec->HandleMessage() sends ack (if needed) and App callback.
    //       N      |     N    |    -    |     -     | Do nothing.

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    // Create new exchange to send ack for a duplicate message and then close this exchange.
    sendAckAndCloseExchange = msgNeedsAck && (matchingUMH == NULL || (dupMsg && !matchingUMH->AllowDuplicateMsgs));

#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
    // Don't create new EC only to send an ack if Peer's message counter synchronization is required.
    if (peerGroupMsgIdNotSynchronized)
        sendAckAndCloseExchange = false;
#endif
#endif

    // If we found a handler or we need to open a new exchange to send ack for a duplicate message.
    if (matchingUMH != NULL
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        || sendAckAndCloseExchange
#endif
       )
    {
        ExchangeContext::MessageReceiveFunct umhandler = NULL;

        ec = AllocContext();
        VerifyOrExit(ec != NULL, err = WEAVE_ERROR_NO_MEMORY);

        ec->Con = msgCon;
        ec->ExchangeId = exchangeHeader.ExchangeId;
        ec->PeerNodeId = msgInfo->SourceNodeId;
        if (msgInfo->InPacketInfo != NULL)
        {
            ec->PeerAddr = msgInfo->InPacketInfo->SrcAddress;
            ec->PeerPort = msgInfo->InPacketInfo->SrcPort;

            // If the message was received over UDP, and the peer's address is an
            // IPv6 link-local, capture the interface to be used when sending packets
            // back to the peer.
            //
            // Specifying an outbound interface when sending UDP packets has a subtle
            // effect on routing and source address selection. Thus it is only done when
            // required by the type of destination address.
            //
            if (ec->Con == NULL && ec->PeerAddr.IsIPv6LinkLocal())
            {
                ec->PeerIntf = msgInfo->InPacketInfo->Interface;
            }
        }
        ec->EncryptionType = msgInfo->EncryptionType;
        ec->KeyId = msgInfo->KeyId;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // No need to set WRMP timer, this will be done when we add to retrans table
        ec->mWRMPNextAckTime = 0;
        ec->SetAckPending(false);
        ec->SetMsgRcvdFromPeer(true);
        ec->mWRMPConfig = gDefaultWRMPConfig;
        ec->mWRMPThrottleTimeout = 0;
        //Internal and for Debug Only; When set, Exchange Layer does not send Ack.
        ec->SetDropAck(false);
#endif

        //Set the ExchangeContext version from the Message header version
        ec->mMsgProtocolVersion = msgInfo->MessageVersion;

        // If UMH was found and the exchange is created not just for sending ack.
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (!sendAckAndCloseExchange)
#endif
        {
            umhandler = matchingUMH->Handler;

            ec->SetInitiator(false);
            ec->AppState = matchingUMH->AppState;
            ec->OnMessageReceived = DefaultOnMessageReceived;
            ec->AllowDuplicateMsgs = matchingUMH->AllowDuplicateMsgs;

            WeaveLogProgress(ExchangeManager, "ec id: %d, AppState: 0x%x", EXCHANGE_CONTEXT_ID(ec - ContextPool), ec->AppState);
        }
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // If the exchange is created only to send ack.
        else
        {
            // If rcvd msg is from initiator then this exchange is created as not Initiator (argument to SetInitiator() is false).
            // If rcvd msg is not from initiator then this exchange is created as Initiator (argument to SetInitiator() is true).
            ec->SetInitiator((exchangeHeader.Flags & kWeaveExchangeFlag_Initiator) == 0);
        }
#endif

        // If support for ephemeral UDP ports is enabled, arrange to send outbound messages on this exchange from the
        // local ephemeral UDP port IF the inbound message that initiated the exchange was sent TO the local ephemeral port.
#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
        ec->SetUseEphemeralUDPPort(GetFlag(msgInfo->Flags, kWeaveMessageFlag_ViaEphemeralUDPPort));
#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

        // Add a reservation for the message encryption key.  This will ensure the key is not removed until the exchange is freed.
        MessageLayer->SecurityMgr->ReserveKey(ec->PeerNodeId, ec->KeyId);

        // Arrange to automatically release the encryption key when the exchange is freed.
        ec->SetAutoReleaseKey(true);

        ec->HandleMessage(msgInfo, &exchangeHeader, msgBuf, umhandler);
        msgBuf = NULL;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        // Close exchange if it was created only to send ack for a duplicate message.
        if (sendAckAndCloseExchange)
            ec->Close();
#endif
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(ExchangeManager, "DispatchMessage failed, err = %d", err);
    }

    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    return;
}

WEAVE_ERROR WeaveExchangeManager::RegisterUMH(uint32_t profileId, int16_t msgType, WeaveConnection *con, bool allowDups,
        ExchangeContext::MessageReceiveFunct handler, void *appState)
{
    UnsolicitedMessageHandler *umh = (UnsolicitedMessageHandler *) UMHandlerPool;
    UnsolicitedMessageHandler *selected = NULL;
    for (int i = 0; i < WEAVE_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS; i++, umh++)
    {
        if (umh->Handler == NULL)
        {
            if (selected == NULL)
                selected = umh;
        }
        else if (umh->ProfileId == profileId && umh->MessageType == msgType && umh->Con == con)
        {
            umh->Handler = handler;
            umh->AppState = appState;
            return WEAVE_NO_ERROR;
        }
    }

    if (selected == NULL)
        return WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS;

    selected->Handler = handler;
    selected->AppState = appState;
    selected->ProfileId = profileId;
    selected->Con = con;
    selected->MessageType = msgType;
    selected->AllowDuplicateMsgs = allowDups;

    SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kExchangeMgr_NumUMHandlers);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveExchangeManager::UnregisterUMH(uint32_t profileId, int16_t msgType, WeaveConnection *con)
{
    UnsolicitedMessageHandler *umh = (UnsolicitedMessageHandler *) UMHandlerPool;
    for (int i = 0; i < WEAVE_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS; i++, umh++)
    {
        if (umh->Handler != NULL && umh->ProfileId == profileId && umh->MessageType == msgType && umh->Con == con)
        {
            umh->Handler = NULL;
            SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kExchangeMgr_NumUMHandlers);
            return WEAVE_NO_ERROR;
        }
    }
    return WEAVE_ERROR_NO_UNSOLICITED_MESSAGE_HANDLER;
}

void WeaveExchangeManager::HandleMessageReceived(WeaveMessageLayer *msgLayer, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    msgLayer->ExchangeMgr->DispatchMessage(msgInfo, msgBuf);
}

void WeaveExchangeManager::HandleMessageReceived(WeaveConnection *con, WeaveMessageInfo *msgInfo, PacketBuffer *msgBuf)
{
    con->MessageLayer->ExchangeMgr->DispatchMessage(msgInfo, msgBuf);
}

WEAVE_ERROR WeaveExchangeManager::PrependHeader(WeaveExchangeHeader *exchangeHeader, PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t headLen = 8; //Constant part: Version/Flags + Msg Type + Exch Id + Profile Id
    uint8_t *p = NULL;

    // Make sure the buffer has a reserved size big enough to hold the full Weave header.
    if (!buf->EnsureReservedSize(WEAVE_HEADER_RESERVE_SIZE))
        ExitNow(err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Verify the right application version is selected.
    if (exchangeHeader->Version != kWeaveExchangeVersion_V1)
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    //Compute the Header Len
    if (exchangeHeader->Flags & kWeaveExchangeFlag_AckId)
    {
        headLen += 4;
    }
#endif

    p = buf->Start();

    //Move the buffer start pointer back by the size of the app header.
    p -= headLen;

    // Adjust the buffer so that the start points to the start of the encoded message.
    buf->SetStart(p);

    // Encode the Weave application header
    Write8(p, ((exchangeHeader->Version << 4) | (exchangeHeader->Flags & 0xF)));
    Write8(p, exchangeHeader->MessageType);
    LittleEndian::Write16(p, exchangeHeader->ExchangeId);
    LittleEndian::Write32(p, exchangeHeader->ProfileId);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (exchangeHeader->Flags & kWeaveExchangeFlag_AckId)
    {
        LittleEndian::Write32(p, exchangeHeader->AckMsgId);
    }
#endif

    WEAVE_FAULT_INJECT_MAX_ARG(FaultInjection::kFault_FuzzExchangeHeaderTx,
            // The FuzzExchangeHeader function takes as argument an index (0 to n-1) into a
            // (logical) array of fuzzing cases, because every field of the header can be fuzzed in 3
            // different ways. Therefore, the max index that can be used for the
            // message being sent depends on the number of fields in the header.
            // There are 4 fields, unless the AckMsgId field is present as
            // well, for a total of 5.
                ((exchangeHeader->Flags & kWeaveExchangeFlag_AckId ?
                   WEAVE_FAULT_INJECTION_EXCH_HEADER_NUM_FIELDS :
                   WEAVE_FAULT_INJECTION_EXCH_HEADER_NUM_FIELDS_WRMP) * WEAVE_FAULT_INJECTION_NUM_FUZZ_VALUES) -1,
                int32_t arg = 0;
                if (numFaultArgs > 0)
                {
                    arg = faultArgs[0];
                }
            ,
            // Code executed without the Manager's lock:
                nl::Weave::FaultInjection::FuzzExchangeHeader(buf->Start(), arg);
            );

exit:
    return err;
}

WEAVE_ERROR WeaveExchangeManager::DecodeHeader(WeaveExchangeHeader *exchangeHeader, WeaveMessageInfo *msgInfo, PacketBuffer *buf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t *p = NULL;
    uint8_t versionFlags;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    uint16_t msgLen = buf->DataLength();
    uint8_t *msgEnd = buf->Start() + msgLen;
#endif

    if (buf->DataLength() < 8)
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    p = buf->Start();

    versionFlags = Read8(p);
    exchangeHeader->Version = versionFlags >> 4;
    exchangeHeader->Flags = versionFlags & 0xF;

    if (exchangeHeader->Version != kWeaveExchangeVersion_V1)
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_EXCHANGE_VERSION);

    exchangeHeader->MessageType = Read8(p);

    exchangeHeader->ExchangeId = LittleEndian::Read16(p);

    exchangeHeader->ProfileId = LittleEndian::Read32(p);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if ((exchangeHeader->Flags & kWeaveExchangeFlag_AckId))
    {
        if ((p + 4) > msgEnd)
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
        exchangeHeader->AckMsgId = LittleEndian::Read32(p);
    }
#endif

    buf->SetStart(p);

    SetFlag(msgInfo->Flags, kWeaveMessageFlag_FromInitiator, GetFlag(exchangeHeader->Flags, kWeaveExchangeFlag_Initiator));

exit:
    return err;
}

/**
 *  Allow unsolicited messages to be received on the specified connection. This
 *  method sets the message reception handler on the given Weave connection.
 *
 *  @param[in]    con           A pointer to the Weave connection object.
 *
 */
void WeaveExchangeManager::AllowUnsolicitedMessages(WeaveConnection *con)
{
    // Hook the OnMessageReceived callback.
    con->OnMessageReceived = HandleMessageReceived;
}

/**
 *  Invoked when a message encryption key has been rejected by a peer (via a KeyError), or a key has
 *  otherwise become invalid (e.g. by ending a session).
 *
 *  @param[in] peerNodeId  The ID of the peer node with which the key is associated.
 *  @param[in] keyId       The ID of the key that has failed.
 *  @param[in] keyErr      A WEAVE_ERROR representing the reason the key is no longer valid.
 *
 */
void WeaveExchangeManager::NotifyKeyFailed(uint64_t peerNodeId, uint16_t keyId, WEAVE_ERROR keyErr)
{
    ExchangeContext *ec = (ExchangeContext *) ContextPool;

    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL && ec->KeyId == keyId && ec->PeerNodeId == peerNodeId)
        {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            // Ensure the exchange context stays around until we're done with it.
            ec->AddRef();

            // Fail entries matching ec.
            FailRetransmitTableEntries(ec, keyErr);
#endif

            // Application callback function in key error case.
            if (ec->OnKeyError)
                ec->OnKeyError(ec, keyErr);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            // Release reference to the exchange context.
            ec->Release();
#endif
        }
    }

    for (int i = 0; i < WEAVE_CONFIG_MAX_BINDINGS; i++)
    {
        BindingPool[i].OnKeyFailed(peerNodeId, keyId, keyErr);
    }
}

/**
 *  Invoked when the security manager becomes available for initiating new secure sessions.
 */
void WeaveExchangeManager::NotifySecurityManagerAvailable()
{
    // Notify each binding that the security manager is now available.
    //
    // Note that this algorithm is unfair to bindings that are positioned later in the pool.
    // In practice, however, this is unlikely to cause any problems.
    for (int i = 0; i < WEAVE_CONFIG_MAX_BINDINGS; i++)
    {
        BindingPool[i].OnSecurityManagerAvailable();
    }
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
/**
 *  Clear MsgCounterSyncReq flag for all pending messages to that peer.
 *
 *  @param[in] peerNodeId    Node ID of the destination node.
 *
 */
void WeaveExchangeManager::ClearMsgCounterSyncReq(uint64_t peerNodeId)
{
    RetransTableEntry *re = (RetransTableEntry *) RetransTable;

    // Find all retransmit entries (re) matching peerNodeId and using application group key.
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++, re++)
    {
        if (re->exchContext != NULL && re->exchContext->PeerNodeId == peerNodeId && WeaveKeyId::IsAppGroupKey(re->exchContext->KeyId))
        {
            // Clear MsgCounterSyncReq flag.
            uint16_t headerField = LittleEndian::Get16(re->msgBuf->Start());
            headerField &= ~kWeaveMessageFlag_MsgCounterSyncReq;
            LittleEndian::Put16(re->msgBuf->Start(), headerField);
        }
    }
}

/**
 *  Retransmit all pending messages that were encrypted with application
 *  group key and were addressed to the specified node.
 *
 *  @param[in] peerNodeId    Node ID of the destination node.
 *
 */
void WeaveExchangeManager::RetransPendingAppGroupMsgs(uint64_t peerNodeId)
{
    RetransTableEntry *re = (RetransTableEntry *) RetransTable;

    // Find all retransmit entries (re) matching peerNodeId and using application group key.
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++, re++)
    {
        if (re->exchContext != NULL && re->exchContext->PeerNodeId == peerNodeId && WeaveKeyId::IsAppGroupKey(re->exchContext->KeyId))
        {
            // Decrement counter to discount the first sent message, which
            // was ignored by receiver due to un-synchronized message counter.
            re->sendCount--;

            // Retramsmit message.
            SendFromRetransTable(re);
        }
    }
}

/**
* Return a tick counter value given a time difference and a tick interval.
* The difference in time is not expected to exceed (2^32 - 1) within the
* scope of two timestamp comparisons in WRMP and, thus, it makes sense to cast
* the time delta to uint32_t. This also avoids invocation of 64 bit divisions
* in constrained platforms that do not support them.
*
* @param[in]  newTime        Timestamp value of in milliseconds.
* @param[in]  oldTime        Timestamp value of in milliseconds.
* @param[in]  tickInterval   Timer tick interval in milliseconds.
*
* @return Tick count for the time delta.
*/
uint32_t WeaveExchangeManager::GetTickCounterFromTimeDelta (uint64_t newTime,
                                                            uint64_t oldTime)
{
    // Note on math: we have a utility function that will compute U64 var / U32
    // compile-time const => U32.  At the moment, we are leaving
    // mWRMPTimerInterval as a member variable in WeaveExchangeManager, however,
    // given its current usage, it could be replaced by a compile time const.
    // Should we make that change, I would recommend making the timeDelta a u64,
    // and replacing the plain 32-bit division below with the utility function.
    // Note that the 32bit timeDelta overflows at around 46 days; pursuing the
    // above code strategy would extend that overflow by a factor if 200 given
    // the default mWRMPPTimerInterval.  If and when such change is made, please
    // update the comment around line 1426.
    uint32_t timeDelta = static_cast<uint32_t>(newTime - oldTime);

    return (timeDelta / mWRMPTimerInterval);
}

#if defined(WRMP_TICKLESS_DEBUG)
void WeaveExchangeManager::TicklessDebugDumpRetransTable(const char *log)
{
     WeaveLogProgress(ExchangeManager, log);

     for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
     {
         if (RetransTable[i].exchContext)
         {
             WeaveLogProgress(ExchangeManager, "EC:%04" PRIX16 " MsgId:%08" PRIX32 " NextRetransTimeCtr:%04" PRIX16,
                              RetransTable[i].exchContext,
                              RetransTable[i].msgId,
                              RetransTable[i].nextRetransTime);
         }
     }
}
#else
void WeaveExchangeManager::TicklessDebugDumpRetransTable(const char *log) { return; }
#endif // WRMP_TICKLESS_DEBUG

/**
* Iterate through active exchange contexts and retrans table entries.
* If an action needs to be triggered by WRMP time facilities, execute
* that action.
*
*/
void WeaveExchangeManager::WRMPExecuteActions(void)
{
    ExchangeContext *ec               = NULL;

    //Process Ack Tables for all ExchangeContexts
    ec = (ExchangeContext *)ContextPool;

#if defined(WRMP_TICKLESS_DEBUG)
    WeaveLogProgress(ExchangeManager, "WRMPExecuteActions");
#endif

    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL && ec->IsAckPending())
        {
            if (0 == ec->mWRMPNextAckTime)
            {
#if defined(WRMP_TICKLESS_DEBUG)
                WeaveLogProgress(ExchangeManager, "WRMPExecuteActions sending ACK");
#endif
                //Send the Ack in a Common::Null message
                ec->SendCommonNullMessage();
                ec->SetAckPending(false);
            }
        }
    }

    TicklessDebugDumpRetransTable("WRMPExecuteActions Dumping RetransTable entries before processing");

    // Retransmit / cancel anything in the retrans table whose retrans timeout
    // has expired
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        ec = RetransTable[i].exchContext;
        if (ec)
        {
            WEAVE_ERROR err = WEAVE_NO_ERROR;

            if (0 == RetransTable[i].nextRetransTime)
            {
                uint8_t sendCount = RetransTable[i].sendCount;
                void * msgCtxt = RetransTable[i].msgCtxt;

                if (sendCount > ec->mWRMPConfig.mMaxRetrans)
                {
                    err = WEAVE_ERROR_MESSAGE_NOT_ACKNOWLEDGED;

                    WeaveLogError(ExchangeManager, "Failed to Send Weave MsgId:%08" PRIX32 " sendCount: %" PRIu8 " max retries: %" PRIu8,
                                  RetransTable[i].msgId, sendCount, ec->mWRMPConfig.mMaxRetrans);

                    // Remove from Table
                    ClearRetransmitTable(RetransTable[i]);
                }

                if (err == WEAVE_NO_ERROR)
                {
                    // Resend from Table (if the operation fails, the entry is cleared)
                    err = SendFromRetransTable(&(RetransTable[i]));
                }

                if (err == WEAVE_NO_ERROR)
                {
                    // If the retransmission was successful, update the passive timer
                    RetransTable[i].nextRetransTime = ec->GetCurrentRetransmitTimeout() / mWRMPTimerInterval;
#if defined(DEBUG)
                    WeaveLogProgress(ExchangeManager, "Retransmit MsgId:%08" PRIX32 " Send Cnt %d",
                            RetransTable[i].msgId, RetransTable[i].sendCount);
#endif
                }

                if (err != WEAVE_NO_ERROR)
                {
                    if (ec->OnSendError)
                    {
                        ec->OnSendError(ec, err, msgCtxt);
                    }
                }
            } //nextRetransTime = 0
        }
    }

    TicklessDebugDumpRetransTable("WRMPExecuteActions Dumping RetransTable entries after processing");
}

/**
* Calculate number of virtual WRMP ticks that have expired since we last
* called this function. Iterate through active exchange contexts and
* retrans table entries, subtracting expired virtual ticks to synchronize
* wakeup times with the current system time. Do not perform any actions
* beyond updating tick counts, actions will be performed by the physical
* WRMP timer tick expiry.
*
*/
void WeaveExchangeManager::WRMPExpireTicks(void)
{
    uint64_t            now         = 0;
    ExchangeContext*    ec          = NULL;
    uint32_t            deltaTicks;

    //Process Ack Tables for all ExchangeContexts
    ec = (ExchangeContext *)ContextPool;

    now = System::Timer::GetCurrentEpoch();

    // Number of full ticks elapsed since last timer processing.  We always round down
    // to the previous tick.  If we are between tick boundaries, the extra time since the
    // last virtual tick is not accounted for here (it will be accounted for when resetting
    // the WRMP timer)

    deltaTicks = GetTickCounterFromTimeDelta(now, mWRMPTimeStampBase);

    // Note on math involving deltaTicks: in the code below, deltaTicks, a
    // 32-bit value, is being subtracted from 16-bit expiration times.  In each
    // case, we compare the expiration time prior to subtraction to guard
    // against underflow.

#if defined(WRMP_TICKLESS_DEBUG)
    WeaveLogProgress(ExchangeManager, "WRMPExpireTicks at %" PRIu64 ", %" PRIu64 ", %u", now, mWRMPTimeStampBase, deltaTicks);
#endif

    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL && ec->IsAckPending())
        {
            //Decrement counter of Ack timestamp by the elapsed timer ticks
            if (ec->mWRMPNextAckTime >= deltaTicks)
            {
                ec->mWRMPNextAckTime -= deltaTicks;
            }
            else
            {
                ec->mWRMPNextAckTime = 0;
            }
#if defined(WRMP_TICKLESS_DEBUG)
            WeaveLogProgress(ExchangeManager, "WRMPExpireTicks set mWRMPNextAckTime to %u", ec->mWRMPNextAckTime);
#endif
        }
    }

    //Process Throttle Time
    //Check Throttle timeout stored in EC to set/unset Throttle flag
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        ec = RetransTable[i].exchContext;
        if (ec)
        {
            //Process Retransmit Table
            //Decrement Throttle timeout by elapsed timeticks
            if (ec->mWRMPThrottleTimeout >= deltaTicks)
            {
                ec->mWRMPThrottleTimeout -= deltaTicks;
            }
            else
            {
                ec->mWRMPThrottleTimeout = 0;
            }
#if defined(WRMP_TICKLESS_DEBUG)
            WeaveLogProgress(ExchangeManager, "WRMPExpireTicks set mWRMPThrottleTimeout to %u", RetransTable[i].nextRetransTime);
#endif

            //Decrement Retransmit timeout by elapsed timeticks
            if (RetransTable[i].nextRetransTime >= deltaTicks)
            {
                RetransTable[i].nextRetransTime -= deltaTicks;
            }
            else
            {
                RetransTable[i].nextRetransTime = 0;
            }
#if defined(WRMP_TICKLESS_DEBUG)
            WeaveLogProgress(ExchangeManager, "WRMPExpireTicks set nextRetransTime to %u", RetransTable[i].nextRetransTime);
#endif
        } //ec entry is allocated
    }

    // Re-Adjust the base time stamp to the most recent tick boundary

    // Note on math: we cast deltaTicks to a 64bit value to ensure that that we
    // produce a full 64 bit product.  At the moment this is a bit of a moot
    // conversion: right now, the math in GetTickCounterFromTimeDelta ensures
    // that the deltaTicks * mWRMPTimerTick fits in 32bits.  However, I'm
    // leaving the math in this form, because I'm leaving the door open to
    // refactoring the division in GetTickCounterFromTimeDelta to use our
    // specialized utility function that computes U64 var/ U32 compile-time
    // const ==> U32
    mWRMPTimeStampBase += static_cast<uint64_t>(deltaTicks) * mWRMPTimerInterval;
#if defined(WRMP_TICKLESS_DEBUG)
    WeaveLogProgress(ExchangeManager, "WRMPExpireTicks mWRMPTimeStampBase to %" PRIu64, mWRMPTimeStampBase);
#endif
}

/**
 * Handle physical wakeup of system due to WRMP wakeup.
 *
 */
void WeaveExchangeManager::WRMPTimeout(System::Layer* aSystemLayer, void* aAppState,  System::Error aError)
{
    WeaveExchangeManager*   exchangeMgr             = reinterpret_cast<WeaveExchangeManager*>(aAppState);

    VerifyOrDie((aSystemLayer != NULL) && (exchangeMgr != NULL));

#if defined(WRMP_TICKLESS_DEBUG)
    WeaveLogProgress(ExchangeManager, "WRMPTimeout\n");
#endif

    // Make sure all tick counts are sync'd to the current time
    exchangeMgr->WRMPExpireTicks();

    // Execute any actions that are due this tick
    exchangeMgr->WRMPExecuteActions();

    // Calculate next physical wakeup
    exchangeMgr->WRMPStartTimer();
}

/**
 *  Add a Weave message into the retransmission table to be subsequently resent if a corresponding acknowledgment
 *  is not received within the retransmission timeout.
 *
 *  @param[in]    ec        A pointer to the ExchangeContext object.
 *
 *  @param[in]    msgBuf    A pointer to the message buffer holding the Weave message to be retransmitted.
 *
 *  @param[in]    messageId The message identifier of the stored Weave message.
 *
 *  @param[in]    msgCtxt   A pointer to some application specific context object pertaining to this message.
 *
 *  @param[out]   rEntry    A pointer to a pointer of a retransmission table entry added into the table.
 *
 *  @retval  #WEAVE_ERROR_RETRANS_TABLE_FULL If there is no empty slot left in the table for addition.
 *  @retval  #WEAVE_NO_ERROR On success.
 *
 */
WEAVE_ERROR WeaveExchangeManager::AddToRetransTable(ExchangeContext *ec, PacketBuffer *msgBuf, uint32_t messageId, void *msgCtxt, RetransTableEntry **rEntry)
{
    bool added      = false;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        //Check the exchContext pointer for finding an empty slot in Table
        if (!RetransTable[i].exchContext)
        {
            // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
            WRMPExpireTicks();

            RetransTable[i].exchContext = ec;
            RetransTable[i].msgId = messageId;
            RetransTable[i].msgBuf = msgBuf;
            RetransTable[i].sendCount = 0;
            RetransTable[i].nextRetransTime = GetTickCounterFromTimeDelta(ec->GetCurrentRetransmitTimeout() + System::Timer::GetCurrentEpoch(), mWRMPTimeStampBase);

            RetransTable[i].msgCtxt = msgCtxt;
            *rEntry = &RetransTable[i];
            //Increment the reference count
            ec->AddRef();
            added = true;

            //Check if the timer needs to be started and start it.
            WRMPStartTimer();
            break;
        }
    }

    if (!added)
    {
        WeaveLogError(ExchangeManager, "RetransTable Already Full");
        err = WEAVE_ERROR_RETRANS_TABLE_FULL;
    }

    return err;
}

/**
 *  Send the specified entry from the retransmission table.
 *
 *  @param[in]    entry                A pointer to a retransmission table entry object that needs to be sent.
 *
 *  @return  #WEAVE_NO_ERROR On success, else corresponding WEAVE_ERROR returned from SendMessage.
 *
 */
WEAVE_ERROR WeaveExchangeManager::SendFromRetransTable(RetransTableEntry *entry)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgSendFlags = 0;
    uint8_t     *p = NULL;
    uint32_t    len = 0;
    ExchangeContext *ec = entry->exchContext;

    // To trigger a call to OnSendError, set the number of transmissions so
    // that the next call to WRMPExecuteActions will abort this entry,
    // restart the timer immediately, and ExitNow.

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WRMSendError,
                       entry->sendCount = (ec->mWRMPConfig.mMaxRetrans + 1);
                       entry->nextRetransTime = 0;
                       WRMPStartTimer();
                       ExitNow());

    if (ec)
    {
#if WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
        if (ec->ShouldCaptureSentMessage())
        {
            SetFlag(msgSendFlags, kWeaveMessageFlag_CaptureTxMessage);
        }
#endif // WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
        SetFlag(msgSendFlags, kWeaveMessageFlag_RetainBuffer);

#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
        SetFlag(msgSendFlags, kWeaveMessageFlag_ViaEphemeralUDPPort, ec->UseEphemeralUDPPort());
#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

        //Locally store the start and length;
        p = entry->msgBuf->Start();
        len = entry->msgBuf->DataLength();

        //Send the message through
        err = MessageLayer->SendMessage(ec->PeerAddr, ec->PeerPort, ec->PeerIntf,
                                        entry->msgBuf,
                                        msgSendFlags);
        //Reset the msgBuf start pointer and data length after sending
        entry->msgBuf->SetStart(p);
        entry->msgBuf->SetDataLength(len);

        //Update the counters
        entry->sendCount++;
    }
    else
    {
        WeaveLogError(ExchangeManager, "Table entry invalid");
    }

    VerifyOrExit(err != WEAVE_NO_ERROR, err = WEAVE_NO_ERROR);

    //Any error generated during initial sending is evaluated for criticality which would
    //qualify it to be reportable back to the caller. If it is non-critical then
    //err is set to WEAVE_NO_ERROR.
    if (WeaveMessageLayer::IsSendErrorNonCritical(err))
    {
        WeaveLogError(ExchangeManager, "Non-crit err %ld sending Weave MsgId:%08" PRIX32 " from retrans table",
                      long(err), entry->msgId);
        err = WEAVE_NO_ERROR;
    }
    else
    {
        //Remove from table
        WeaveLogError(ExchangeManager, "Crit-err %ld when sending Weave MsgId:%08" PRIX32 ", send tries: %d",
                long(err), entry->msgId, entry->sendCount);

        ClearRetransmitTable(*entry);
    }

exit:
    return err;
}

/**
 *  Clear entries matching a specified ExchangeContext.
 *
 *  @param[in]    ec    A pointer to the ExchangeContext object.
 *
 */
void WeaveExchangeManager::ClearRetransmitTable(ExchangeContext *ec)
{
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        if (RetransTable[i].exchContext == ec)
        {
            //Clear the retransmit table entry.
            ClearRetransmitTable(RetransTable[i]);
        }
    }
}

/**
 *  Clear an entry in the retransmission table.
 *
 *  @param[in]    rEntry   A reference to the RetransTableEntry object.
 *
 */
void WeaveExchangeManager::ClearRetransmitTable(RetransTableEntry &rEntry)
{
    if (rEntry.exchContext)
    {
        // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
        WRMPExpireTicks();

        rEntry.exchContext->Release();
        rEntry.exchContext = NULL;

        if (rEntry.msgBuf)
        {
            PacketBuffer::Free(rEntry.msgBuf);
            rEntry.msgBuf = NULL;
        }

        // Clear all other fields
        memset(&rEntry, 0, sizeof(rEntry));

        // Schedule next physical wakeup
        WRMPStartTimer();

    }
}

/**
 *  Fail entries matching a specified ExchangeContext.
 *
 *  @param[in]    ec    A pointer to the ExchangeContext object.
 *
 *  @param[in]    err   The error for failing table entries.
 *
 */
void WeaveExchangeManager::FailRetransmitTableEntries(ExchangeContext *ec, WEAVE_ERROR err)
{
    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        if (RetransTable[i].exchContext == ec)
        {
            void *msgCtxt = RetransTable[i].msgCtxt;

            // Remove the entry from the retransmission table.
            ClearRetransmitTable(RetransTable[i]);

            // Application callback OnSendError.
            if (ec->OnSendError)
                ec->OnSendError(ec, err, msgCtxt);
        }
    }
}

/**
* Iterate through active exchange contexts and retrans table entries.
* Determine how many WRMP ticks we need to sleep before we need to physically
* wake the CPU to perform an action.  Set a timer to go off when we
* next need to wake the system.
*
*/
void WeaveExchangeManager::WRMPStartTimer()
{
    WEAVE_ERROR res                   = WEAVE_NO_ERROR;
    uint32_t nextWakeTime             = UINT32_MAX;
    bool foundWake                    = false;
    ExchangeContext *ec               = NULL;

    // When do we need to next wake up to send an ACK?
    ec = (ExchangeContext *)ContextPool;

    for (int i = 0; i < WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS; i++, ec++)
    {
        if (ec->ExchangeMgr != NULL && ec->IsAckPending() && ec->mWRMPNextAckTime < nextWakeTime) {
            nextWakeTime = ec->mWRMPNextAckTime;
            foundWake = true;
#if defined(WRMP_TICKLESS_DEBUG)
            WeaveLogProgress(ExchangeManager, "WRMPStartTimer next ACK time %u", nextWakeTime);
#endif
        }
    }

    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        ec = RetransTable[i].exchContext;
        if (ec)
        {
            // When do we need to next wake up for throttle retransmission?
            if (ec->mWRMPThrottleTimeout != 0 && ec->mWRMPThrottleTimeout < nextWakeTime) {
                nextWakeTime = ec->mWRMPThrottleTimeout;
                foundWake = true;
#if defined(WRMP_TICKLESS_DEBUG)
                WeaveLogProgress(ExchangeManager, "WRMPStartTimer throttle timeout %u", nextWakeTime);
#endif
            }

            // When do we need to next wake up for WRMP retransmit?
            if (RetransTable[i].nextRetransTime < nextWakeTime) {
                nextWakeTime = RetransTable[i].nextRetransTime;
                foundWake = true;
#if defined(WRMP_TICKLESS_DEBUG)
                WeaveLogProgress(ExchangeManager, "WRMPStartTimer RetransTime %u", nextWakeTime);
#endif
            }
        }
    }

    if (foundWake) {
        // Set timer for next tick boundary - subtract the elapsed time from the current tick
        System::Timer::Epoch currentTime = System::Timer::GetCurrentEpoch();
        int32_t timerArmValue = nextWakeTime * mWRMPTimerInterval - (currentTime - mWRMPTimeStampBase);
        System::Timer::Epoch timerExpiryEpoch = currentTime + timerArmValue;

#if defined(WRMP_TICKLESS_DEBUG)
        WeaveLogProgress(ExchangeManager, "WRMPStartTimer wake in %d ms (%" PRIu64" %u %" PRIu64 " %" PRIu64 ")",
                timerArmValue,
                timerExpiryEpoch, nextWakeTime, currentTime, mWRMPTimeStampBase);
#endif
        if (timerExpiryEpoch != mWRMPCurrentTimerExpiry)
        {
            // If the tick boundary has expired in the past (delayed processing of event due to other system activity),
            // expire the timer immediately
            if (timerArmValue < 0) {
                timerArmValue = 0;
            }

#if defined(WRMP_TICKLESS_DEBUG)
            WeaveLogProgress(ExchangeManager, "WRMPStartTimer set timer for %d %" PRIu64, timerArmValue, timerExpiryEpoch);
#endif
            WRMPStopTimer();
            res = MessageLayer->SystemLayer->StartTimer((uint32_t)timerArmValue, WRMPTimeout, this);

            VerifyOrDieWithMsg(res == WEAVE_NO_ERROR, ExchangeManager, "Cannot start WRMPTimeout\n");
            mWRMPCurrentTimerExpiry = timerExpiryEpoch;
#if defined(WRMP_TICKLESS_DEBUG)
        } else {
            WeaveLogProgress(ExchangeManager, "WRMPStartTimer timer already set for %" PRIu64, timerExpiryEpoch);
#endif
        }
    } else {
#if defined(WRMP_TICKLESS_DEBUG)
        WeaveLogProgress(ExchangeManager, "Not setting WRMP timeout at %" PRIu64, System::Timer::GetCurrentEpoch());
#endif
        WRMPStopTimer();
    }

    TicklessDebugDumpRetransTable("WRMPStartTimer Dumping RetransTable entries after setting wakeup times");
}

void WeaveExchangeManager::WRMPStopTimer()
{
    MessageLayer->SystemLayer->CancelTimer(WRMPTimeout, this);
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 *  Initialize the shared pool of Bindings.
 *
 */
void WeaveExchangeManager::InitBindingPool(void)
{
    memset(BindingPool, 0, sizeof(BindingPool));
    for (size_t i = 0; i < WEAVE_CONFIG_MAX_BINDINGS; ++i)
    {
        BindingPool[i].mState = Binding::kState_NotAllocated;
        BindingPool[i].mExchangeManager = this;
    }
    mBindingsInUse = 0;
}

/**
 *  Allocate a new Binding
 *
 *  @return  A pointer to the newly allocated Binding, or NULL if the pool has been exhausted
 *
 */
Binding * WeaveExchangeManager::AllocBinding(void)
{
    Binding * pResult = NULL;

    WEAVE_FAULT_INJECT(FaultInjection::kFault_AllocBinding,
                           return NULL);

    for (size_t i = 0; i < WEAVE_CONFIG_MAX_BINDINGS; ++i)
    {
        if (Binding::kState_NotAllocated == BindingPool[i].mState)
        {
            pResult = &BindingPool[i];
            ++mBindingsInUse;
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kExchangeMgr_NumBindings);
            break;
        }
    }

    return pResult;
}

/**
 *  Deallocate the binding object so it could be reused later
 *
 *  @param[in]  binding         A pointer to the binding object to be deallocated. The object
 *                              must be previously allocated from this #WeaveExchangeManager.
 *
 */
void WeaveExchangeManager::FreeBinding(Binding * binding)
{
    binding->mState = Binding::kState_NotAllocated;
    --mBindingsInUse;
    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kExchangeMgr_NumBindings);
}

/**
 *  Allocate a new Binding with the arguments supplied
 *
 *  @param[in]  eventCallback   A function pointer to be used for event callback
 *  @param[in]  appState        A pointer to some context which would be carried in event callback later
 *
 *  @return  A pointer to the newly allocated Binding, or NULL if the pool has been exhausted
 *
 */
Binding * WeaveExchangeManager::NewBinding(Binding::EventCallback eventCallback, void *appState)
{
    Binding * pResult = AllocBinding();
    if (NULL != pResult)
    {
        pResult->Init(appState, eventCallback);
    }
    return pResult;
}

/**
 *  Get an ID suitable for identifying a binding in log messages.
 *
 *  @param[in]  binding         A pointer to a Binding object
 *
 *  @return                     An unsigned integer identifying the binding
 *
 */
uint16_t WeaveExchangeManager::GetBindingLogId(const Binding * const binding) const
{
    // note that the result of pointer subtraction should be ptrdiff_t
    return static_cast<uint16_t>(binding - BindingPool);
}

} // namespace nl
} // namespace Weave

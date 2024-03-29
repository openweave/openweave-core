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
 *      This file implements the ExchangeContext class.
 *
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <stdlib.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/FlagUtils.hpp>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <SystemLayer/SystemStats.h>
//#include <nestlabs/log/nllog.hpp>
#undef nlLogError
#define nlLogError(MSG, ...)

namespace nl {
namespace Weave {

using namespace nl::Weave::Encoding;

enum {
    kFlagInitiator              = 0x0001, /// This context is the initiator of the exchange.
    kFlagConnectionClosed       = 0x0002, /// This context was associated with a WeaveConnection.
    kFlagAutoRequestAck         = 0x0004, /// When set, automatically request an acknowledgment whenever a message is sent via UDP.
    kFlagDropAck                = 0x0008, /// Internal and debug only: when set, the exchange layer does not send an acknowledgment.
    kFlagResponseExpected       = 0x0010, /// If a response is expected for a message that is being sent.
    kFlagAckPending             = 0x0020, /// When set, signifies that there is an acknowledgment pending to be sent back.
    kFlagPeerRequestedAck       = 0x0040, /// When set, signifies that at least one message received on this exchange requested an acknowledgment.
                                          /// This flag is read by the application to decide if it needs to request an acknowledgment for the
                                          /// response message it is about to send. This flag can also indicate whether peer is using WRMP.
    kFlagMsgRcvdFromPeer        = 0x0080, /// When set, signifies that at least one message has been received from peer on this exchange context.
    kFlagAutoReleaseKey         = 0x0100, /// Automatically release the message encryption key when the exchange context is freed.
    kFlagAutoReleaseConnection  = 0x0200, /// Automatically release the associated WeaveConnection when the exchange context is freed.
    kFlagUseEphemeralUDPPort    = 0x0400, /// When set, use the local ephemeral UDP port as the source port for outbound messages.
    kFlagCaptureSentMessage     = 0x0800, /// Capture the sent message after encoded with Weave headers.
};

/**
 *  Determine whether the context is the initiator of the exchange.
 *
 *  @return Returns 'true' if it is the initiator, else 'false'.
 *
 */
bool ExchangeContext::IsInitiator(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagInitiator));
}

/**
 *  Determine whether the ExchangeContext has an associated active WeaveConnection.
 *
 *  @return Returns 'true' if connection is closed, else 'false'.
 */
bool ExchangeContext::IsConnectionClosed(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagConnectionClosed));
}

/**
 *  Determine whether a response is expected for messages sent over
 *  this exchange.
 *
 *  @return Returns 'true' if response expected, else 'false'.
 */
bool ExchangeContext::IsResponseExpected(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagResponseExpected));
}

/**
 *  Set the kFlagInitiator flag bit. This flag is set by the node that
 *  initiates an exchange.
 *
 *  @param[in]  inInitiator  A Boolean indicating whether (true) or not
 *                           (false) the context is the initiator of
 *                           the exchange.
 *
 */
void ExchangeContext::SetInitiator(bool inInitiator)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagInitiator), inInitiator);
}

/**
 *  Set the kFlagConnectionClosed flag bit. This flag is set
 *  when a WeaveConnection associated with an ExchangeContext
 *  is closed.
 *
 *  @param[in]  inConnectionClosed  A Boolean indicating whether
 *                                  (true) or not (false) the context
 *                                  was associated with a connection.
 *
 */
void ExchangeContext::SetConnectionClosed(bool inConnectionClosed)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagConnectionClosed), inConnectionClosed);
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
/**
 *  Determine whether there is already an acknowledgment pending to be sent
 *  to the peer on this exchange.
 *
 */
bool ExchangeContext::IsAckPending(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagAckPending));
}

/**
 *  Determine whether peer requested acknowledgment for at least one message
 *  on this exchange.
 *
 *  @return Returns 'true' if acknowledgment requested, else 'false'.
 */
bool ExchangeContext::HasPeerRequestedAck(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagPeerRequestedAck));
}

/**
 *  Determine whether at least one message has been received
 *  on this exchange from peer.
 *
 *  @return Returns 'true' if message received, else 'false'.
 */
bool ExchangeContext::HasRcvdMsgFromPeer(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagMsgRcvdFromPeer));
}

/**
 *  Set if a message has been received from the peer
 *  on this exchange.
 *
 *  @param[in]  inMsgRcvdFromPeer  A Boolean indicating whether (true) or not
 *                                 (false) a message has been received
 *                                 from the peer on this exchange context.
 *
 */
void ExchangeContext::SetMsgRcvdFromPeer(bool inMsgRcvdFromPeer)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagMsgRcvdFromPeer), inMsgRcvdFromPeer);
}

/**
 *  Set if an acknowledgment needs to be sent back to the peer on this exchange.
 *
 *  @param[in]  inAckPending A Boolean indicating whether (true) or not
 *                          (false) an acknowledgment should be sent back
 *                          in response to a received message.
 *
 */
void ExchangeContext::SetAckPending(bool inAckPending)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagAckPending), inAckPending);
}

/**
 *  Set if an acknowledgment was requested in the last message received
 *  on this exchange.
 *
 *  @param[in]  inPeerRequestedAck A Boolean indicating whether (true) or not
 *                                 (false) an acknowledgment was requested
 *                                 in the last received message.
 *
 */
void ExchangeContext::SetPeerRequestedAck(bool inPeerRequestedAck)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagPeerRequestedAck), inPeerRequestedAck);
}

/**
 *  Set whether the WeaveExchangeManager should not send acknowledgements
 *  for this context.
 *
 *  For internal, debug use only.
 *
 *  @param[in]  inDropAck  A Boolean indicating whether (true) or not
 *                         (false) the acknowledgements should be not
 *                         sent for the exchange.
 *
 */
void ExchangeContext::SetDropAck(bool inDropAck)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagDropAck), inDropAck);
}

/**
 *  Determine whether the WeaveExchangeManager should not send an
 *  acknowledgement.
 *
 *  For internal, debug use only.
 *
 */
bool ExchangeContext::ShouldDropAck(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagDropAck));
}

static inline bool IsWRMPControlMessage(uint32_t profileId, uint8_t msgType)
{
    return (profileId == nl::Weave::Profiles::kWeaveProfile_Common &&
            (msgType == nl::Weave::Profiles::Common::kMsgType_WRMP_Throttle_Flow ||
             msgType == nl::Weave::Profiles::Common::kMsgType_WRMP_Delayed_Delivery));
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 *  Set whether a response is expected on this exchange.
 *
 *  @param[in]  inResponseExpected  A Boolean indicating whether (true) or not
 *                                  (false) a response is expected on this
 *                                  exchange.
 *
 */
void ExchangeContext::SetResponseExpected(bool inResponseExpected)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagResponseExpected), inResponseExpected);
}

/**
 * Returns whether an acknowledgment will be requested whenever a message is sent.
 */
bool ExchangeContext::AutoRequestAck() const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagAutoRequestAck));
}

/**
 * Set whether an acknowledgment should be requested whenever a message is sent.
 *
 * @param[in] autoReqAck            A Boolean indicating whether or not an
 *                                  acknowledgment should be requested whenever a
 *                                  message is sent.
 */
void ExchangeContext::SetAutoRequestAck(bool autoReqAck)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagAutoRequestAck), autoReqAck);
}

/**
 * Return whether the encryption key associated with the exchange should be
 * released when the exchange is freed.
 */
bool ExchangeContext::GetAutoReleaseKey() const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagAutoReleaseKey));
}

/**
 * Set whether the encryption key associated with the exchange should be
 * released when the exchange is freed.
 *
 * @param[in] autoReleaseKey        True if the message encryption key should be
 *                                  automatically released.
 */
void ExchangeContext::SetAutoReleaseKey(bool autoReleaseKey)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagAutoReleaseKey), autoReleaseKey);
}

/**
 * Return whether the Weave connection associated with the exchange should be
 * released when the exchange is freed.
 */
bool ExchangeContext::ShouldAutoReleaseConnection() const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagAutoReleaseConnection));
}

/**
 * Set whether the Weave connection associated with the exchange should be
 * released when the exchange is freed.
 *
 * @param[in] autoReleaseCon        True if the Weave connection should be
 *                                  automatically released.
 */
void ExchangeContext::SetShouldAutoReleaseConnection(bool autoReleaseCon)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagAutoReleaseConnection), autoReleaseCon);
}

#if WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
void ExchangeContext::SetCaptureSentMessage(bool inCaptureSentMessage)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagCaptureSentMessage), inCaptureSentMessage);
}

bool ExchangeContext::ShouldCaptureSentMessage() const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagCaptureSentMessage));
}
#endif // WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE

/**
 * @fn  bool ExchangeContext::UseEphemeralUDPPort(void) const
 *
 * Return whether outbound messages sent via the exchange should be sent from
 * the local ephemeral UDP port.
 */

#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

bool ExchangeContext::UseEphemeralUDPPort(void) const
{
    return GetFlag(mFlags, static_cast<uint16_t>(kFlagUseEphemeralUDPPort));
}

#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

/**
 * Set whether outbound messages sent via the exchange should be sent from
 * the local ephemeral UDP port.
 */
void ExchangeContext::SetUseEphemeralUDPPort(bool val)
{
    SetFlag(mFlags, static_cast<uint16_t>(kFlagUseEphemeralUDPPort), val);
}

#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

/**
 *  Send a Weave message on this exchange.
 *
 *  @param[in]    profileId     The profile identifier of the Weave message to be sent.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer object holding the Weave message.
 *
 *  @param[in]    sendFlags     Flags set by the application for the Weave message being sent.
 *
 *  @param[in]    msgCtxt       A pointer to an application-specific context object to be associated
 *                              with the message being sent.

 *  @retval  #WEAVE_ERROR_INVALID_ARGUMENT               if an invalid argument was passed to this SendMessage API.
 *  @retval  #WEAVE_ERROR_SEND_THROTTLED                 if this exchange context has been throttled when using the
 *                                                       Weave reliable messaging protocol.
 *  @retval  #WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE if there is a mismatch in the specific send operation and the
 *                                                       Weave message protocol version that is supported. For example,
 *                                                       this error would be generated if Weave Reliable Messaging
 *                                                       semantics are being attempted when the Weave message protocol
 *                                                       version is V1.
 *  @retval  #WEAVE_ERROR_NOT_CONNECTED                  if the context was associated with a connection that is now
 *                                                       closed.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE                if the state of the exchange context is incorrect.
 *  @retval  #WEAVE_NO_ERROR                             if the Weave layer successfully sent the message down to the
 *                                                       network layer.
 */
WEAVE_ERROR ExchangeContext::SendMessage(uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf, uint16_t sendFlags,
    void *msgCtxt)
{
    // Setup the message info structure.
    WeaveMessageInfo msgInfo;
    msgInfo.Clear();
    msgInfo.SourceNodeId = ExchangeMgr->FabricState->LocalNodeId;
    msgInfo.DestNodeId = PeerNodeId;
    msgInfo.EncryptionType = EncryptionType;
    msgInfo.KeyId = KeyId;

    return SendMessage(profileId, msgType, msgBuf, sendFlags, &msgInfo, msgCtxt);
}

/**
 *  Send a Weave message on this exchange.
 *
 *  @param[in]    profileId     The profile identifier of the Weave message to be sent.
 *
 *  @param[in]    msgType       The message type of the corresponding profile.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer object holding the Weave message.
 *
 *  @param[in]    sendFlags     Flags set by the application for the Weave message being sent.
 *
 *  @param[in]    msgInfo       A pointer to the WeaveMessageInfo object.
 *
 *  @param[in]    msgCtxt       A pointer to an application-specific context object to be
 *                              associated with the message being sent.
 *
 *  @retval  #WEAVE_ERROR_INVALID_ARGUMENT               if an invalid argument was passed to this SendMessage API.
 *  @retval  #WEAVE_ERROR_SEND_THROTTLED                 if this exchange context has been throttled when using the
 *                                                       Weave reliable messaging protocol.
 *  @retval  #WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE if there is a mismatch in the specific send operation and the
 *                                                       Weave message protocol version that is supported. For example,
 *                                                       this error would be generated if Weave Reliable Messaging
 *                                                       semantics are being attempted when the Weave message protocol
 *                                                       version is V1.
 *  @retval  #WEAVE_ERROR_NOT_CONNECTED                  if the context was associated with a connection that is now
 *                                                       closed.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE                if the state of the exchange context is incorrect.
 *  @retval  #WEAVE_NO_ERROR                             if the Weave layer successfully sent the message down to the
 *                                                       network layer.
 */
WEAVE_ERROR ExchangeContext::SendMessage(uint32_t profileId, uint8_t msgType, PacketBuffer *msgBuf, uint16_t sendFlags,
    WeaveMessageInfo * msgInfo, void *msgCtxt)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool sendCalled = false;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    WeaveExchangeManager::RetransTableEntry *entry = NULL;
#endif

#if WEAVE_RETAIN_LOGGING
    uint16_t payloadLen = msgBuf->DataLength();
#endif

    // Don't let method get called on a freed object.
    VerifyOrDie(ExchangeMgr != NULL && mRefCount != 0);

    // we hold the exchange context here in case the entity that
    // originally generated it tries to close it as a result of
    // an error arising below. at the end, we have to close it.
    AddRef();

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // If sending via UDP and the auto-request ACK feature is enabled, automatically
    // request an acknowledgment, UNLESS the NoAutoRequestAck send flag has been specified.
    if (Con == NULL && (mFlags & kFlagAutoRequestAck) != 0 && (sendFlags & kSendFlag_NoAutoRequestAck) == 0)
    {
        sendFlags |= kSendFlag_RequestAck;
    }

    // Do not allow WRM to be used over a TCP connection
    if ((sendFlags & kSendFlag_RequestAck) && Con != NULL)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Abort early if Throttle is already set;
    VerifyOrExit(mWRMPThrottleTimeout == 0, err = WEAVE_ERROR_SEND_THROTTLED);

#else // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // If WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING == 0, then
    // kSendFlag_RequestAck should not be set.
    if (sendFlags & kSendFlag_RequestAck)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Set the Message Protocol Version
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (sendFlags & kSendFlag_RequestAck || IsWRMPControlMessage(profileId, msgType) ||
        (profileId == nl::Weave::Profiles::kWeaveProfile_Common &&
         msgType == nl::Weave::Profiles::Common::kMsgType_Null))
    {
        if (kWeaveMessageVersion_Unspecified == mMsgProtocolVersion)
        {
            mMsgProtocolVersion = msgInfo->MessageVersion = kWeaveMessageVersion_V2;
        }
        else
        {
            VerifyOrExit(mMsgProtocolVersion == kWeaveMessageVersion_V2, err = WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE);
        }
    }
#endif
    if (kWeaveMessageVersion_Unspecified == mMsgProtocolVersion)
    {
        mMsgProtocolVersion = msgInfo->MessageVersion = kWeaveMessageVersion_V1;
    }
    else
    {
        msgInfo->MessageVersion = mMsgProtocolVersion;
    }

    // Prevent sending if the context was associated with a connection that is now closed.
    VerifyOrExit(!IsConnectionClosed(), err = WEAVE_ERROR_NOT_CONNECTED);

    // TODO: implement support for retransmissions.

    // flag validation
    if (sendFlags & kSendFlag_RetransmissionTrickle)
    {
        //We do not allow WRM to be used when Trickle retransmission is requested
        if (sendFlags & kSendFlag_RequestAck)
        {
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        // We do not support trickle retrasnmissions over
        // connection-oriented exchanges
        VerifyOrExit(Con == NULL, err=WEAVE_ERROR_INVALID_ARGUMENT);

        if (0 == RetransInterval)
        { // we're not retransmitting, do not hold onto the buffer
            sendFlags &= ~kSendFlag_RetainBuffer;
        }
        else
        {
            sendFlags |= kSendFlag_RetainBuffer;
            msg = msgBuf;
        }
    }

    // Add the exchange header to the message buffer.
    WeaveExchangeHeader exchangeHeader;
    memset(&exchangeHeader, 0, sizeof(exchangeHeader));

    err = EncodeExchHeader(&exchangeHeader, profileId, msgType, msgBuf, sendFlags);
    SuccessOrExit(err);

    // If a response message is expected...
    if ((sendFlags & kSendFlag_ExpectResponse) != 0)
    {
        // Only one 'response expected' message can be outstanding at a time.
        VerifyOrExit(!IsResponseExpected(), err = WEAVE_ERROR_INCORRECT_STATE);

        SetResponseExpected(true);

        // Arm the response timer if a timeout has been specified.
        if (ResponseTimeout > 0)
        {
            err = StartResponseTimer();
            SuccessOrExit(err);
        }
    }

    //Fill in appropriate message header flags
    if (sendFlags & kSendFlag_DelaySend)
        msgInfo->Flags |= kWeaveMessageFlag_DelaySend;
    //FIXME: RS: possibly unnecessary, should addref instead
    if (sendFlags & kSendFlag_RetainBuffer)
        msgInfo->Flags |= kWeaveMessageFlag_RetainBuffer;
    if (sendFlags & kSendFlag_AlreadyEncoded)
        msgInfo->Flags |= kWeaveMessageFlag_MessageEncoded;
    if (sendFlags & kSendFlag_ReuseMessageId)
        msgInfo->Flags |= kWeaveMessageFlag_ReuseMessageId;
    if (sendFlags & kSendFlag_ReuseSourceId)
        msgInfo->Flags |= kWeaveMessageFlag_ReuseSourceId;
    if (sendFlags & kSendFlag_DefaultMulticastSourceAddress)
        msgInfo->Flags |= kWeaveMessageFlag_DefaultMulticastSourceAddress;

    SetFlag(msgInfo->Flags, kWeaveMessageFlag_FromInitiator, IsInitiator());

#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
    SetFlag(msgInfo->Flags, kWeaveMessageFlag_ViaEphemeralUDPPort, UseEphemeralUDPPort());
#endif // WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT

#if WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
    // Set flag for capture in MessageInfo if ExchangeContext is marked for
    // capture.
    if (ShouldCaptureSentMessage())
    {
        msgInfo->Flags |= kWeaveMessageFlag_CaptureTxMessage;
    }
#endif // WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE

    // Send the message via UDP or TCP/BLE based on the presence of a connection.
    if (Con != NULL)
    {
        // Hook the message received callback on the connection so that the WeaveExchangeManager gets
        // called when messages arrive.
        Con->OnMessageReceived = WeaveExchangeManager::HandleMessageReceived;

        err = Con->SendMessage(msgInfo, msgBuf);
        msgBuf = NULL;
        sendCalled = true;
    }
    else
    {

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (sendFlags & kSendFlag_RequestAck)
        {
            err = ExchangeMgr->MessageLayer->SelectDestNodeIdAndAddress(msgInfo->DestNodeId, PeerAddr);
            SuccessOrExit(err);
            err = ExchangeMgr->MessageLayer->EncodeMessage(PeerAddr, PeerPort, PeerIntf,
                                                           msgInfo, msgBuf);
            SuccessOrExit(err);

            // Copy msg to a right-sized buffer if applicable
            msgBuf = PacketBuffer::RightSize(msgBuf);

            //Add to Table for subsequent sending
            err = ExchangeMgr->AddToRetransTable(this, msgBuf, msgInfo->MessageId, msgCtxt, &entry);
            SuccessOrExit(err);
            msgBuf = NULL;

            err = ExchangeMgr->SendFromRetransTable(entry);
            sendCalled = true;
            SuccessOrExit(err);

            WEAVE_FAULT_INJECT(FaultInjection::kFault_WRMDoubleTx,
                               entry->nextRetransTime = 0;
                               ExchangeMgr->WRMPStartTimer()
                               );

        }
        else
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        {
            err = ExchangeMgr->MessageLayer->SendMessage(PeerAddr, PeerPort, PeerIntf,
                                                         msgInfo, msgBuf);
            msgBuf = NULL;
            sendCalled = true;
            SuccessOrExit(err);
        }

        if (sendFlags & kSendFlag_RetransmissionTrickle)
        {
            currentBcastMsgID = msgInfo->MessageId;
            if (RetransInterval != 0)
            {
                if (StartTimerT() != WEAVE_NO_ERROR)
                {
                    nlLogError("EC: cant start T\n");
                }
            }
        }
    }


exit:
    if (sendCalled)
    {
        WeaveLogRetain(ExchangeManager, "Msg %s %08" PRIX32 ":%d %d %016" PRIX64 " %04" PRIX16 " %04" PRIX16 " %ld MsgId:%08" PRIX32,
                       "sent", profileId, msgType, (int)payloadLen, msgInfo->DestNodeId,
                       (Con ? Con->LogId() : 0), ExchangeId, (long)err, msgInfo->MessageId);
    }
    if (err != WEAVE_NO_ERROR && IsResponseExpected())
    {
        CancelResponseTimer();
        SetResponseExpected(false);
    }
    if (msgBuf != NULL && (sendFlags & kSendFlag_RetainBuffer) == 0)
    {
        PacketBuffer::Free(msgBuf);
        if (msg == msgBuf)
            msg = NULL;
    }

    //Release the reference to the exchange context acquired above. Under normal circumstances
    //this will merely decrement the reference count, without actually freeing the exchange context.
    //However if one of the function calls in this method resulted in a callback to the application,
    //the application may have released its reference, resulting in the exchange context actually
    //being freed here.
    Release();

    return err;
}

/**
 *  Send a Common::Null message.
 *
 *  @note  When sent via UDP, the null message is sent *without* requesting an acknowledgment,
 *  even in the case where the auto-request acknowledgment feature has been enabled on the
 *  exchange.
 *
 *  @retval  #WEAVE_ERROR_NO_MEMORY   If no available PacketBuffers.
 *  @retval  #WEAVE_NO_ERROR          If the method succeeded or the error wasn't critical.
 *  @retval  other                    Another critical error returned by SendMessage().
 *
 */
WEAVE_ERROR ExchangeContext::SendCommonNullMessage(void)
{
    WEAVE_ERROR  err     = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;

    // Allocate a buffer for the null message
    msgBuf = PacketBuffer::NewWithAvailableSize(0);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Send the null message
    err = SendMessage(nl::Weave::Profiles::kWeaveProfile_Common,
                      nl::Weave::Profiles::Common::kMsgType_Null, msgBuf,
                      kSendFlag_NoAutoRequestAck);
    msgBuf = NULL;

exit:
    if (WeaveMessageLayer::IsSendErrorNonCritical(err))
    {
        WeaveLogError(ExchangeManager, "Non-crit err %ld sending solitary ack",
                      long(err));
        err = WEAVE_NO_ERROR;
    }
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(ExchangeManager, "Failed to send Solitary ack for MsgId:%08" PRIX32 " to Peer %016" PRIX64 ":%ld",
                      mPendingPeerAckId, PeerNodeId, (long)err);
    }

    return err;
}

/**
 *  Encode the exchange header into a message buffer.
 *
 *  @param[in]    exchangeHeader     A pointer to the Weave Exchange header object.
 *
 *  @param[in]    profileId          The profile identifier of the Weave message to be sent.
 *
 *  @param[in]    msgType            The message type of the corresponding profile.
 *
 *  @param[in]    msgBuf             A pointer to the PacketBuffer needed for the Weave message.
 *
 *  @param[in]    sendFlags          Flags set by the application for the Weave message being sent.
 *
 *
 *  @retval  #WEAVE_ERROR_BUFFER_TOO_SMALL  If the message buffer does not have sufficient space
 *                                          for encoding the exchange header.
 *  @retval  #WEAVE_NO_ERROR                If encoding of the message was successful.
 */
WEAVE_ERROR ExchangeContext::EncodeExchHeader(WeaveExchangeHeader *exchangeHeader, uint32_t profileId, uint8_t msgType,
    PacketBuffer *msgBuf, uint16_t sendFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Fill the exchange header to the message buffer.
    exchangeHeader->Version = kWeaveExchangeVersion_V1;
    exchangeHeader->ExchangeId = ExchangeId;
    exchangeHeader->ProfileId = profileId;
    exchangeHeader->MessageType = msgType;
    // sendFlags under special circumstances (such as a retransmission
    // of the remote alarm) can override the initiator flag in the
    // exchange header.  The semantics here really is: use the
    // ExchangeId in the namespace of the SourceNodeId
    exchangeHeader->Flags = (IsInitiator() || (sendFlags & kSendFlag_FromInitiator)) ? kWeaveExchangeFlag_Initiator : 0;

    // WRMP PreProcess Checks and Flag setting
    if (mMsgProtocolVersion == kWeaveMessageVersion_V2)
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        //If there is a pending acknowledgment piggyback it on this message.
        //If there is no pending acknowledgment piggyback the last Ack that was sent.
        //   - HasPeerRequestedAck() is used to verify that AckId field is valid
        //     to avoid piggybacking uninitialized AckId.
        if (HasPeerRequestedAck())
        {
            // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
            ExchangeMgr->WRMPExpireTicks();

            exchangeHeader->Flags |= kWeaveExchangeFlag_AckId;
            exchangeHeader->AckMsgId = mPendingPeerAckId;

            //Set AckPending flag to false after setting the Ack flag;
            SetAckPending(false);

            // Schedule next physical wakeup
            ExchangeMgr->WRMPStartTimer();

#if defined(DEBUG)
            WeaveLogProgress(ExchangeManager, "Piggybacking Ack for MsgId:%08" PRIX32 " with msg",
                             mPendingPeerAckId);
#endif
        }

        //Assert the flag if message requires an Ack back;
        if ((sendFlags & kSendFlag_RequestAck) && !IsWRMPControlMessage(profileId, msgType))
        {
            exchangeHeader->Flags |= kWeaveExchangeFlag_NeedsAck;
        }
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    }

    err = WeaveExchangeManager::PrependHeader(exchangeHeader, msgBuf);

    return err;
}

/**
 *  Cancel the Trickle retransmission mechanism.
 *
 */
void ExchangeContext::CancelRetrans()
{
    // NOTE: modify for other retransmission schemes
    TeardownTrickleRetransmit();
}

/**
 *  Increment the reference counter for the exchange context by one.
 *
 */
void ExchangeContext::AddRef()
{
    mRefCount++;
#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
    WeaveLogProgress(ExchangeManager, "ec id: %d [%04" PRIX16 "], refCount++: %d", EXCHANGE_CONTEXT_ID(this - ExchangeMgr->ContextPool), ExchangeId, mRefCount);
#endif
}

void ExchangeContext::DoClose(bool clearRetransTable)
{
    // Clear app callbacks
    OnMessageReceived = NULL;
    OnResponseTimeout = NULL;
    OnRetransmissionTimeout = NULL;
    OnConnectionClosed = NULL;
    OnKeyError = NULL;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
    ExchangeMgr->WRMPExpireTicks();

    OnThrottleRcvd = NULL;
    OnDDRcvd = NULL;
    OnSendError = NULL;
    OnAckRcvd = NULL;

    // Flush any pending WRM acks
    WRMPFlushAcks();

    // Clear the WRM retransmission table
    if (clearRetransTable)
    {
        ExchangeMgr->ClearRetransmitTable(this);
    }

    // Schedule next physical wakeup
    ExchangeMgr->WRMPStartTimer();
#endif

    // Cancel the trickle retransmission timer.
    CancelRetrans();
    // Cancel the response timer.
    CancelResponseTimer();
}

/**
 *  Gracefully close an exchange context. This call decrements the
 *  reference count and releases the exchange when the reference
 *  count goes to zero.
 *
 */
void ExchangeContext::Close()
{
    VerifyOrDie(ExchangeMgr != NULL && mRefCount != 0);

#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
    WeaveLogProgress(ExchangeManager, "ec id: %d [%04" PRIX16 "], %s", EXCHANGE_CONTEXT_ID(this - ExchangeMgr->ContextPool), ExchangeId, __func__);
#endif

    DoClose(false);
    Release();
}
/**
 *  Abort the Exchange context immediately and release all
 *  references to it.
 *
 */
void ExchangeContext::Abort()
{
    VerifyOrDie(ExchangeMgr != NULL && mRefCount != 0);

#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
    WeaveLogProgress(ExchangeManager, "ec id: %d [%04" PRIX16 "], %s", EXCHANGE_CONTEXT_ID(this - ExchangeMgr->ContextPool), ExchangeId, __func__);
#endif

    DoClose(true);
    Release();
}

/**
 *  Release reference to this exchange context. If count is down
 *  to one then close the context, reset all application callbacks,
 *  and stop all timers.
 *
 */
void ExchangeContext::Release(void)
{
    VerifyOrDie(ExchangeMgr != NULL && mRefCount != 0);

    if (mRefCount == 1)
    {
        //Ideally, in this scenario, the retransmit table should
        //be clear of any outstanding messages for this context and
        //the boolean parameter passed to DoClose() should not matter.
        WeaveExchangeManager *em = ExchangeMgr;
#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
        uint16_t tmpid = ExchangeId;
#endif

        // If so configured, automatically release any reservation held on
        // the message encryption key.
        if (GetAutoReleaseKey())
        {
            em->MessageLayer->SecurityMgr->ReleaseKey(PeerNodeId, KeyId);
        }

        // If configured, automatically release a reference to the WeaveConnection object.
        if (ShouldAutoReleaseConnection() && Con != NULL)
        {
            SetShouldAutoReleaseConnection(false);
            Con->Release();
        }

        DoClose(false);
        mRefCount = 0;
        ExchangeMgr = NULL;

        em->mContextsInUse--;
        em->MessageLayer->SignalMessageLayerActivityChanged();
#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
        WeaveLogProgress(ExchangeManager, "ec-- id: %d [%04" PRIX16 "], inUse: %d, addr: 0x%x", EXCHANGE_CONTEXT_ID(this - em->ContextPool), tmpid,  em->mContextsInUse, this);
#endif
        SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kExchangeMgr_NumContexts);
    }
    else
    {
        mRefCount--;
#if defined(WEAVE_EXCHANGE_CONTEXT_DETAIL_LOGGING)
        WeaveLogProgress(ExchangeManager, "ec id: %d [%04" PRIX16 "], refCount--: %d", EXCHANGE_CONTEXT_ID(this - ExchangeMgr->ContextPool), ExchangeId, mRefCount);
#endif
    }
}

WEAVE_ERROR ExchangeContext::ResendMessage()
{
    WeaveMessageInfo msgInfo;
    WEAVE_ERROR res;
    uint8_t * payload;

    if (msg == NULL)
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    msgInfo.Clear();
    msgInfo.MessageVersion = mMsgProtocolVersion;
    msgInfo.SourceNodeId = ExchangeMgr->FabricState->LocalNodeId;
    msgInfo.EncryptionType = EncryptionType;
    msgInfo.KeyId = KeyId;
    msgInfo.DestNodeId = PeerNodeId;
    res = ExchangeMgr->MessageLayer->DecodeHeader(msg, &msgInfo, &payload);
    if (res != WEAVE_NO_ERROR)
        return WEAVE_ERROR_INCORRECT_STATE;

    msgInfo.Flags |=
        kWeaveMessageFlag_RetainBuffer |
        kWeaveMessageFlag_MessageEncoded |
        kWeaveMessageFlag_ReuseMessageId |
        kWeaveMessageFlag_ReuseSourceId;

    return ExchangeMgr->MessageLayer->ResendMessage(PeerAddr, PeerPort, PeerIntf, &msgInfo, msg);
}

/**
 *  Start the Trickle rebroadcast algorithm's periodic retransmission timer mechanism.
 *
 *  @return  #WEAVE_NO_ERROR if successful, else an INET_ERROR mapped into a WEAVE_ERROR.
 *
 */
WEAVE_ERROR ExchangeContext::StartTimerT()
{
    if (RetransInterval == 0)
    {
        return WEAVE_NO_ERROR;
    }

    // range from 1 to RetransInterval
    backoff = 1 + (GetRandU32() % (RetransInterval-1));
    msgsReceived = 0;
    WeaveLogDetail(ExchangeManager, "Trickle new interval");
    return ExchangeMgr->MessageLayer->SystemLayer->StartTimer(backoff, TimerTau, this);
}

void ExchangeContext::TimerT(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ExchangeContext* client = reinterpret_cast<ExchangeContext*>(aAppState);

    if ( (aSystemLayer == NULL) || (aAppState == NULL) || (aError != WEAVE_SYSTEM_NO_ERROR))
    {
        return;
    }
    if (client->StartTimerT() != WEAVE_NO_ERROR)
    {
        nlLogError("EC: cant start T\n");
    }
}

void ExchangeContext::TimerTau(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ExchangeContext* ec = reinterpret_cast<ExchangeContext*>(aAppState);

    if ( (aSystemLayer == NULL) || (aAppState == NULL) || (aError != WEAVE_SYSTEM_NO_ERROR))
    {
        return;
    }
    if (ec->msgsReceived < ec->rebroadcastThreshold)
    {
        WeaveLogDetail(ExchangeManager, "Trickle re-send with duplicate message counter: %u", ec->msgsReceived);
        ec->ResendMessage();
    }
    else
    {
        WeaveLogDetail(ExchangeManager, "Trickle skipping this interval");
    }
    if ((ec->RetransInterval == 0) || (ec->RetransInterval <= ec->backoff))
    {
        return;
    }
    if (aSystemLayer->StartTimer(ec->RetransInterval - ec->backoff, TimerT, ec) != WEAVE_NO_ERROR)
    {
        nlLogError("EC: cant start Tau\n");
    }
}


bool ExchangeContext::MatchExchange(WeaveConnection *msgCon, const WeaveMessageInfo *msgInfo,
        const WeaveExchangeHeader *exchangeHeader)
{
    // A given message is part of a particular exchange if...
    return

        // The exchange identifier of the message matches the exchange identifier of the context.
        (ExchangeId == exchangeHeader->ExchangeId)

        // AND The message was received over the connection bound to the context, or the
        // message was received over UDP (msgCon == NULL) and the context is not bound
        // to a connection (Con == NULL).
        && (Con == msgCon)

        // AND The message was received from the peer node associated with the exchange, or the peer node identifier is 'any'.
            && (((PeerNodeId == kAnyNodeId) && (msgInfo->DestNodeId == ExchangeMgr->FabricState->LocalNodeId))
                || (PeerNodeId == msgInfo->SourceNodeId))


        // AND The message was sent by an initiator and the exchange context is a responder (IsInitiator==false)
        //    OR The message was sent by a responder and the exchange context is an initiator (IsInitiator==true) (for the broadcast case, the initiator is ill defined)

        && (((exchangeHeader->Flags & kWeaveExchangeFlag_Initiator) != 0) != IsInitiator());
}

/**
 *  Handle trickle message within the exchange context.
 *
 *  @param[in]    pktInfo    A pointer to the IPPacketInfo object.
 *
 *  @param[in]    msgInfo    A pointer to the Weave message info structure.
 *
 */
void ExchangeContext::HandleTrickleMessage(const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo)
{
    // check if we're at all interested in this message
    const bool isMessageIdMatching = currentBcastMsgID == msgInfo->MessageId;
    const bool isNodeIdMatching = (PeerNodeId == kAnyNodeId) || (PeerNodeId == msgInfo->SourceNodeId);
    if (isMessageIdMatching &&
        isNodeIdMatching)
    {
        msgsReceived++;
        WeaveLogDetail(ExchangeManager, "Increasing trickle duplicate message counter: %u", msgsReceived);
    }
    else
    {
        WeaveLogDetail(ExchangeManager, "Not counted as duplicate message, for MsgId:%08" PRIX32 " NodeId:%d",
            isMessageIdMatching, isNodeIdMatching);
    }

}

/**
 *  Setup the trickle retransmission mechanism by setting the corresponding retransmission interval
 *  and rebroadcast threshold.
 *
 *  @param[in]    retransInterval    The retransmit interval of the Trickle rebroadcast algorithm.
 *
 *  @param[in]    threshold          The maximum number of times a message is rebroadcast.
 *
 *  @param[in]    timeout            The maximum time to wait before canceling the Trickle retransmission timer.
 *
 *  @return  #WEAVE_NO_ERROR if Trickle setup was successful, else an INET_ERROR mapped into a WEAVE_ERROR.
 *
 */
WEAVE_ERROR ExchangeContext::SetupTrickleRetransmit(uint32_t retransInterval, uint8_t threshold, uint32_t timeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CancelRetrans();
    RetransInterval = retransInterval;
    rebroadcastThreshold = threshold;
    if (timeout != 0)
    {
        err = ExchangeMgr->MessageLayer->SystemLayer->StartTimer(timeout, CancelRetransmissionTimer, this);
        if (err != WEAVE_NO_ERROR)
        {
            nlLogError("EC: cant setup timeout\n");
            return err;
        }
    }
    WeaveLogDetail(ExchangeManager, "Trickle interval %u ms, threshold %u, timeout %u ms", retransInterval, threshold, timeout);
    return WEAVE_NO_ERROR;
}

/**
 *  Tear down the Trickle retransmission mechanism by canceling the periodic timers
 *  within Trickle and freeing the message buffer holding the Weave
 *  message.
 *
 */
void ExchangeContext::TeardownTrickleRetransmit()
{
    System::Layer* lSystemLayer = ExchangeMgr->MessageLayer->SystemLayer;
    if (lSystemLayer == NULL)
    {
        // this is an assertion error, which shall never happen
        return;
    }
    lSystemLayer->CancelTimer(TimerT, this);
    lSystemLayer->CancelTimer(TimerTau, this);
    lSystemLayer->CancelTimer(CancelRetransmissionTimer, this);
    if (msg != NULL)
    {
        PacketBuffer::Free(msg);
    }

    msg = NULL;
    backoff = 0;
    RetransInterval = 0;
}

void ExchangeContext::CancelRetransmissionTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ExchangeContext* ec = reinterpret_cast<ExchangeContext*>(aAppState);

    if (ec == NULL)
        return;
    ec->CancelRetrans();
    if (ec->OnRetransmissionTimeout != NULL)
    {
        ec->OnRetransmissionTimeout(ec);
    }
}

WEAVE_ERROR ExchangeContext::StartResponseTimer()
{
    return ExchangeMgr->MessageLayer->SystemLayer->StartTimer(ResponseTimeout, HandleResponseTimeout, this);
}

void ExchangeContext::CancelResponseTimer()
{
    ExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleResponseTimeout, this);
}

void ExchangeContext::HandleResponseTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ExchangeContext* ec = reinterpret_cast<ExchangeContext*>(aAppState);

    // NOTE: we don't set mResponseExpected to false here because the response could still arrive. If the user
    // wants to never receive the response, they must close the exchange context.

    // Call the user's timeout handler.
    if (ec->OnResponseTimeout != NULL)
        ec->OnResponseTimeout(ec);
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
bool ExchangeContext::WRMPCheckAndRemRetransTable(uint32_t ackMsgId, void **rCtxt)
{
    bool res = false;

    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        if ((ExchangeMgr->RetransTable[i].exchContext == this) &&
            (ExchangeMgr->RetransTable[i].msgId == ackMsgId))
        {
            //Return context value
            *rCtxt = ExchangeMgr->RetransTable[i].msgCtxt;

            //Clear the entry from the retransmision table.
            ExchangeMgr->ClearRetransmitTable(ExchangeMgr->RetransTable[i]);

#if defined(DEBUG)
            WeaveLogProgress(ExchangeManager, "Rxd Ack; Removing MsgId:%08" PRIX32 " from Retrans Table",
                             ackMsgId);
#endif
            res = true;
            break;
        }
        else
        {
            continue;
        }
    }

    return res;
}

//Flush the pending Ack
WEAVE_ERROR ExchangeContext::WRMPFlushAcks(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (IsAckPending())
    {
        //Send the acknowledgment as a Common::Null message
        err = SendCommonNullMessage();

        if (err == WEAVE_NO_ERROR)
        {
#if defined(DEBUG)
            WeaveLogProgress(ExchangeManager, "Flushed pending ack for MsgId:%08" PRIX32 " to Peer %016" PRIX64,
                             mPendingPeerAckId, PeerNodeId);
#endif
        }
    }

    return err;
}

/**
 *  Get the current retransmit timeout. It would be either the initial or
 *  the active retransmit timeout based on whether the ExchangeContext has
 *  an active message exchange going with its peer.
 *
 *  @return the current retransmit time.
 */
uint32_t ExchangeContext::GetCurrentRetransmitTimeout(void)
{
  return (HasRcvdMsgFromPeer() ? mWRMPConfig.mActiveRetransTimeout :
                                 mWRMPConfig.mInitialRetransTimeout);
}

/**
 *  Send a Throttle Flow message to the peer node requesting it to throttle its sending of messages.
 *
 *  @note
 *    This message is part of the Weave Reliable Messaging protocol.
 *
 *  @param[in]    pauseTimeMillis    The time (in milliseconds) that the recipient is expected
 *                                   to throttle its sending.
 *  @retval  #WEAVE_ERROR_INVALID_ARGUMENT               If an invalid argument was passed to this SendMessage API.
 *  @retval  #WEAVE_ERROR_SEND_THROTTLED                 If this exchange context has been throttled when using the
 *                                                       Weave reliable messaging protocol.
 *  @retval  #WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE If there is a mismatch in the specific send operation and the
 *                                                       Weave message protocol version that is supported. For example,
 *                                                       this error would be generated if Weave Reliable Messaging
 *                                                       semantics are being attempted when the Weave message protocol
 *                                                       version is V1.
 *  @retval  #WEAVE_ERROR_NOT_CONNECTED                  If the context was associated with a connection that is now
 *                                                       closed.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE                If the state of the exchange context is incorrect.
 *  @retval  #WEAVE_NO_ERROR                             If the Weave layer successfully sent the message down to the
 *                                                       network layer.
 *
 */
WEAVE_ERROR ExchangeContext::WRMPSendThrottleFlow(uint32_t pauseTimeMillis)
{
    WEAVE_ERROR  err     = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    uint8_t      *p      = NULL;
    uint8_t      msgLen  = sizeof(pauseTimeMillis);

    msgBuf = PacketBuffer::NewWithAvailableSize(msgLen);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();

    //Encode the fields in the buffer
    LittleEndian::Write32(p, pauseTimeMillis);
    msgBuf->SetDataLength(msgLen);

    // Send a Throttle Flow message to the peer.  Throttle Flow messages must never request
    // acknowledgment, so suppress the auto-request ACK feature on the exchange in case it has been
    // enabled by the application.
    err = SendMessage(Profiles::kWeaveProfile_Common, Profiles::Common::kMsgType_WRMP_Throttle_Flow,
                      msgBuf, kSendFlag_NoAutoRequestAck);

exit:
    msgBuf = NULL;

    return err;
}

/**
 *  Send a Delayed Delivery message to notify a sender node that its previously sent message would experience an expected
 *  delay before being delivered to the recipient. One of the possible causes for messages to be delayed before being
 *  delivered is when the recipient end node is sleepy. This message is potentially generated by a suitable intermediate
 *  node in the send path who has enough knowledge of the recipient to infer about the delayed delivery. Upon receiving
 *  this message, the sender would re-adjust its retransmission timers for messages that seek acknowledgments back.
 *
 *  @note
 *    This message is part of the Weave Reliable Messaging protocol.
 *
 *  @param[in]    pauseTimeMillis    The time (in milliseconds) that the previously sent message is expected
 *                                   to be delayed before being delivered.
 *
 *  @param[in]    delayedNodeId      The node identifier of the peer node to whom the mesage delivery would be delayed.
 *
 *  @retval  #WEAVE_ERROR_INVALID_ARGUMENT               if an invalid argument was passed to this SendMessage API.
 *  @retval  #WEAVE_ERROR_WRONG_MSG_VERSION_FOR_EXCHANGE if there is a mismatch in the specific send operation and the
 *                                                       Weave message protocol version that is supported. For example,
 *                                                       this error would be generated if Weave Reliable Messaging
 *                                                       semantics are being attempted when the Weave message protocol
 *                                                       version is V1.
 *  @retval  #WEAVE_ERROR_NOT_CONNECTED                  if the context was associated with a connection that is now
 *                                                       closed.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE                if the state of the exchange context is incorrect.
 *  @retval  #WEAVE_NO_ERROR                             if the Weave layer successfully sent the message down to the
 *                                                       network layer.
 *
 */
WEAVE_ERROR ExchangeContext::WRMPSendDelayedDelivery(uint32_t pauseTimeMillis, uint64_t delayedNodeId)
{
    WEAVE_ERROR  err     = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;
    uint8_t      *p      = NULL;
    uint8_t      msgLen  = sizeof(pauseTimeMillis) + sizeof(delayedNodeId);

    msgBuf = PacketBuffer::NewWithAvailableSize(msgLen);
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    //Set back the pointer by the length of the fields

    //Encode the fields in the buffer
    LittleEndian::Write32(p, pauseTimeMillis);
    LittleEndian::Write64(p, delayedNodeId);
    msgBuf->SetDataLength(msgLen);

    // Send a Delayed Delivery message to the peer.  Delayed Delivery messages must never request
    // acknowledgment, so suppress the auto-request ACK feature on the exchange in case it has been
    // enabled by the application.
    err = SendMessage(Profiles::kWeaveProfile_Common, Profiles::Common::kMsgType_WRMP_Delayed_Delivery,
                      msgBuf, kSendFlag_NoAutoRequestAck);

exit:
    msgBuf = NULL;

    return err;
}

/**
 *  Process received Ack. Remove the corresponding message context from the RetransTable and execute the application
 *  callback
 *
 *  @note
 *    This message is part of the Weave Reliable Messaging protocol.
 *
 *  @param[in]    exchHeader         Weave exchange information for incoming Ack message.
 *
 *  @param[in]    msgInfo            General Weave message information for the incoming Ack message.
 *
 *  @retval  #WEAVE_ERROR_INVALID_ACK_ID                 if the msgId of received Ack is not in the RetransTable.
 *  @retval  #WEAVE_NO_ERROR                             if the context was removed.
 *
 */
WEAVE_ERROR ExchangeContext::WRMPHandleRcvdAck(const WeaveExchangeHeader *exchHeader, const WeaveMessageInfo *msgInfo)
{
    void         *msgCtxt  = NULL;
    WEAVE_ERROR  err     = WEAVE_NO_ERROR;

    //Msg is an Ack; Check Retrans Table and remove message context
    if (!WRMPCheckAndRemRetransTable(exchHeader->AckMsgId, &msgCtxt))
    {
#if defined(DEBUG)
        WeaveLogError(ExchangeManager, "Weave MsgId:%08" PRIX32" not in RetransTable",
                      exchHeader->AckMsgId);
#endif
        err = WEAVE_ERROR_INVALID_ACK_ID;
        //Optionally call an application callback with this error.
    }
    else
    {
        //Call OnAckRcvd Here
        if (OnAckRcvd)
        {
            OnAckRcvd(this, msgCtxt);
        }
        else
        {
            WeaveLogDetail(ExchangeManager,
                           "No App Handler for Ack");
        }
#if defined(DEBUG)
        WeaveLogProgress(ExchangeManager, "Removed Weave MsgId:%08" PRIX32 " from RetransTable",
                         exchHeader->AckMsgId);
#endif
    }

    return err;
}

WEAVE_ERROR ExchangeContext::WRMPHandleNeedsAck(const WeaveMessageInfo *msgInfo)
{
    WEAVE_ERROR  err = WEAVE_NO_ERROR;

    // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
    ExchangeMgr->WRMPExpireTicks();

    // If the message IS a duplicate.
    if (msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage)
    {
#if defined(DEBUG)
            WeaveLogProgress(ExchangeManager, "Forcing tx of solitary ack for duplicate MsgId:%08" PRIX32,
                             msgInfo->MessageId);
#endif
        // Is there pending ack for a different message id.
        bool wasAckPending = IsAckPending() && mPendingPeerAckId != msgInfo->MessageId;

        // Temporary store currently pending ack id (even if there is none).
        uint32_t tempAckId = mPendingPeerAckId;

        // Set the pending ack id.
        mPendingPeerAckId = msgInfo->MessageId;

        // Send the Ack for the duplication message in a Common::Null message.
        err = SendCommonNullMessage();

        // If there was pending ack for a different message id.
        if (wasAckPending)
        {
            // Restore previously pending ack id.
            mPendingPeerAckId = tempAckId;
            SetAckPending(true);
        }

        SuccessOrExit(err);
    }
    // Otherwise, the message IS NOT a duplicate.
    else
    {
        if (IsAckPending())
        {
#if defined(DEBUG)
            WeaveLogProgress(ExchangeManager, "Pending ack queue full; forcing tx of solitary ack for MsgId:%08" PRIX32,
                             mPendingPeerAckId);
#endif
            // Send the Ack for the currently pending Ack in a Common::Null message.
            err = SendCommonNullMessage();
            SuccessOrExit(err);
        }

        // Replace the Pending ack id.
        mPendingPeerAckId = msgInfo->MessageId;
        mWRMPNextAckTime = ExchangeMgr->GetTickCounterFromTimeDelta(mWRMPConfig.mAckPiggybackTimeout + System::Timer::GetCurrentEpoch(), ExchangeMgr->mWRMPTimeStampBase);
        SetAckPending(true);
    }

exit:
    // Schedule next physical wakeup
    ExchangeMgr->WRMPStartTimer();
    return err;
}

WEAVE_ERROR ExchangeContext::HandleThrottleFlow(uint32_t PauseTimeMillis)
{
    // Expire any virtual ticks that have expired so all wakeup sources reflect the current time
    ExchangeMgr->WRMPExpireTicks();

    // Flow Control Message Received; Adjust Throttle timeout accordingly.
    // A PauseTimeMillis of zero indicates that peer is unthrottling this Exchange.

    if (0 != PauseTimeMillis)
    {
        mWRMPThrottleTimeout = ExchangeMgr->GetTickCounterFromTimeDelta((System::Timer::GetCurrentEpoch() + PauseTimeMillis),
                                                                        ExchangeMgr->mWRMPTimeStampBase);
    }
    else
    {
        mWRMPThrottleTimeout = 0;
    }

    // Go through the retrans table entries for that node and adjust the timer.

    for (int i = 0; i < WEAVE_CONFIG_WRMP_RETRANS_TABLE_SIZE; i++)
    {
        // Check if ExchangeContext matches

        if (ExchangeMgr->RetransTable[i].exchContext == this)
        {
            // Adjust the retrans timer value to account for throttling.
            if (0 != PauseTimeMillis)
            {
                ExchangeMgr->RetransTable[i].nextRetransTime += PauseTimeMillis / ExchangeMgr->mWRMPTimerInterval;
            }
            // UnThrottle when PauseTimeMillis is set to 0
            else
            {
                ExchangeMgr->RetransTable[i].nextRetransTime = 0;
            }
            break;
        }
    }
    // Call OnThrottleRcvd application callback

    if (OnThrottleRcvd)
    {
        OnThrottleRcvd(this, PauseTimeMillis);
    }
    else
    {
        WeaveLogDetail(ExchangeManager,
                       "No App Handler for Throttle Message");
    }

    // Schedule next physical wakeup
    ExchangeMgr->WRMPStartTimer();
    return WEAVE_NO_ERROR;
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * @overload
 */
WEAVE_ERROR ExchangeContext::HandleMessage(WeaveMessageInfo *msgInfo, const WeaveExchangeHeader *exchHeader, PacketBuffer *msgBuf)
{
    return HandleMessage(msgInfo, exchHeader, msgBuf, NULL);
}

/**
 * Handle a message in the context of an Exchange. This method processes ACKs and duplicate messages
 * and then invokes the application handler.
 *
 * Note on OnMessageReceived and the umhandler argument:
 * When the ExchangeManager creates a new EC for an inbound message,
 * OnMessageReceived is set by default to a handler that drops the message, so
 * any future message on the exchange is discarded unless the application
 * installs an OnMessageReceived handler.
 * The unsolicited message that triggers the creation of the EC is
 * handled by an UMH, which is passed to this method via the umhandler param.
 *
 * @param[in] msgInfo       General Weave message information for the incoming message.
 * @param[in] exchHeader    Weave exchange information for the incoming message.
 * @param[in] msgBuf        PacketBuffer containing the payload of the incoming message.
 * @param[in] umhandler     Pointer to a message receive callback function; if this function
 *                          is not NULL it will be used in place of the OnMessageReceived function
 *                          installed in the ExchangeContext.
 *
 */
WEAVE_ERROR ExchangeContext::HandleMessage(WeaveMessageInfo *msgInfo, const WeaveExchangeHeader *exchHeader, PacketBuffer *msgBuf,
                                           ExchangeContext::MessageReceiveFunct umhandler)
{
    WEAVE_ERROR   err = WEAVE_NO_ERROR;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    const uint8_t *p  = NULL;
    uint32_t      PauseTimeMillis = 0;
#endif

    // We hold a reference to the ExchangeContext here to
    // guard against Close() calls(decrementing the reference
    // count) by the application before the Weave Exchange
    // layer has completed its work on the ExchangeContext.
    AddRef();

    if (msgInfo->MessageVersion == kWeaveMessageVersion_V2)
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (exchHeader->Flags & kWeaveExchangeFlag_AckId)
        {
            err = WRMPHandleRcvdAck(exchHeader, msgInfo);
        }
        if (exchHeader->Flags & kWeaveExchangeFlag_NeedsAck)
        {
            //Set the flag in message header indicating an ack requested by peer;
            msgInfo->Flags |= kWeaveMessageFlag_PeerRequestedAck;

            //Set the flag in the exchange context indicating an ack requested;
            SetPeerRequestedAck(true);

            if (!ShouldDropAck())
            {
                err = WRMPHandleNeedsAck(msgInfo);
            }
        }
#endif
    }

    // If the message is a duplicate and duplicates are not allowed for this exchange.
    if ((msgInfo->Flags & kWeaveMessageFlag_DuplicateMessage) && !AllowDuplicateMsgs)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }

    //Received Flow Throttle
    if (exchHeader->ProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
        exchHeader->MessageType == nl::Weave::Profiles::Common::kMsgType_WRMP_Throttle_Flow)
    {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        //Extract PauseTimeMillis from msgBuf
        p = msgBuf->Start();
        PauseTimeMillis = LittleEndian::Read32(p);
        HandleThrottleFlow(PauseTimeMillis);
        ExitNow(err = WEAVE_NO_ERROR);
#endif
    }
    //Return and not pass this to Application if Common::Null Msg Type
    else if ((exchHeader->ProfileId == nl::Weave::Profiles::kWeaveProfile_Common) &&
        (exchHeader->MessageType == nl::Weave::Profiles::Common::kMsgType_Null))
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
    else
    {
        // Since we got the response, cancel the response timer.
        CancelResponseTimer();

        // If the context was expecting a response to a previously sent message, this message
        // is implicitly that response.
        SetResponseExpected(false);

        // Deliver the message to the app via its callback.
        if (umhandler)
        {
            umhandler(this, msgInfo->InPacketInfo, const_cast<WeaveMessageInfo *>(msgInfo), exchHeader->ProfileId,
                    exchHeader->MessageType, msgBuf);
            msgBuf = NULL;
        }
        else if (OnMessageReceived != NULL)
        {
            OnMessageReceived(this, msgInfo->InPacketInfo, const_cast<WeaveMessageInfo *>(msgInfo), exchHeader->ProfileId,
                              exchHeader->MessageType, msgBuf);
            msgBuf = NULL;
        }
        else
        {
            WeaveLogError(ExchangeManager,
                          "No App Handler for Msg(MsgId:%08" PRIX32 ")",
                          msgInfo->MessageId);
        }
    }

exit:

    // Release the reference to the ExchangeContext that was held at the beginning of this function.
    // This call should also do the needful of closing the ExchangeContext if the application has
    // already made a prior call to Close().
    Release();

    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    return err;
}

void ExchangeContext::HandleConnectionClosed(WEAVE_ERROR conErr)
{
    // Record that the EC had a connection that is now closed.
    SetConnectionClosed(true);

    // If configured, automatically release the EC's reference to the WeaveConnection object.
    if (ShouldAutoReleaseConnection() && Con != NULL)
    {
        SetShouldAutoReleaseConnection(false);
        Con->Release();
    }

    // Discard the EC's pointer to the connection, preventing further use.
    WeaveConnection * con = Con;
    Con = NULL;

    // Call the application's OnConnectionClosed handler, if set.
    if (OnConnectionClosed != NULL)
        OnConnectionClosed(this, con, conErr);
}

/**
 * Constructs a string describing the peer node and its associated address / connection information.
 *
 * @param[in] buf                       A pointer to a buffer into which the string should be written. The supplied
 *                                      buffer should be at least as big as kGetPeerDescription_MaxLength. If a
 *                                      smaller buffer is given the string will be truncated to fit. The output
 *                                      will include a NUL termination character in all cases.
 * @param[in] bufSize                   The size of the buffer pointed at by buf.
 */
void ExchangeContext::GetPeerDescription(char * buf, uint32_t bufSize) const
{
    WeaveMessageLayer::GetPeerDescription(buf, bufSize, PeerNodeId,
        (PeerAddr != IPAddress::Any) ? &PeerAddr : NULL, PeerPort, PeerIntf, Con);
}

} // namespace nl
} // namespace Weave

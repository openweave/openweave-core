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
 *      This file implements an object for a Weave Echo unsolicitied
 *      initiator (client).
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveEcho.h"

namespace nl {
namespace Weave {
namespace Profiles {

WeaveEchoClient::WeaveEchoClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    EncryptionType = kWeaveEncryptionType_None;
    KeyId = WeaveKeyId::kNone;
    OnEchoResponseReceived = NULL;
    ExchangeCtx = NULL;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    OnAckRcvdReceived = NULL;
    RequestAck = false;
    AckReceived = false;
    ResponseReceived = false;
    WRMPACKDelay = WEAVE_CONFIG_WRMP_DEFAULT_ACK_TIMEOUT;
    WRMPRetransInterval = WEAVE_CONFIG_WRMP_DEFAULT_ACTIVE_RETRANS_TIMEOUT;
    WRMPRetransCount = WEAVE_CONFIG_WRMP_DEFAULT_MAX_RETRANS;
    appContext = 0xcafebabe;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
}

WEAVE_ERROR WeaveEchoClient::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    EncryptionType = kWeaveEncryptionType_None;
    KeyId = WeaveKeyId::kNone;
    OnEchoResponseReceived = NULL;
    ExchangeCtx = NULL;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveEchoClient::Shutdown()
{
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Abort();
        ExchangeCtx = NULL;
    }

    ExchangeMgr = NULL;
    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Send an echo request over a WeaveConnection
 *
 * @param con     The connection
 * @param payload A PacketBuffer with the payload. This function takes ownership of the PacketBuffer
 *
 * @return WEAVE_ERROR_NO_MEMORY if no ExchangeContext is available.
 *         Other WEAVE_ERROR codes as returned by the lower layers.
 */
WEAVE_ERROR WeaveEchoClient::SendEchoRequest(WeaveConnection *con, PacketBuffer *payload)
{
    // Discard any existing exchange context. Effectively we can only have one Echo exchange with
    // a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Abort();
        ExchangeCtx = NULL;
    }

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(con, this);
    if (ExchangeCtx == NULL)
    {
        PacketBuffer::Free(payload);
        return WEAVE_ERROR_NO_MEMORY;
    }

    ExchangeCtx->OnConnectionClosed = HandleConnectionClosed;

    return SendEchoRequest(payload);
}

/**
 * Send an echo request to a Weave node using the default Weave port and the
 * letting the system's routing table choose the output interface.
 *
 * @param nodeId        The destination's nodeId
 * @param nodeAddr      The destination's ip address
 * @param payload       A PacketBuffer with the payload. This function takes ownership of the PacketBuffer
 *
 * @return WEAVE_ERROR_NO_MEMORY if no ExchangeContext is available.
 *         Other WEAVE_ERROR codes as returned by the lower layers.
 */
WEAVE_ERROR WeaveEchoClient::SendEchoRequest(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    return SendEchoRequest(nodeId, nodeAddr, WEAVE_PORT, INET_NULL_INTERFACEID, payload);
}

/**
 * Send an echo request to a Weave node.
 *
 * @param nodeId        The destination's nodeId
 * @param nodeAddr      The destination's ip address
 * @param port          The destination's UDP port (WEAVE_PORT by default)
 * @param sendIntfId    A specific interface to use
 * @param payload       A PacketBuffer with the payload. This function takes ownership of the PacketBuffer
 *
 * @return WEAVE_ERROR_NO_MEMORY if no ExchangeContext is available.
 *         Other WEAVE_ERROR codes as returned by the lower layers.
 */
WEAVE_ERROR WeaveEchoClient::SendEchoRequest(uint64_t nodeId, IPAddress nodeAddr, uint16_t port, InterfaceId sendIntfId, PacketBuffer *payload)
{
    // Discard any existing exchange context. Effectively we can only have one Echo exchange with
    // a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Abort();
        ExchangeCtx = NULL;
    }
    if (nodeAddr == IPAddress::Any)
        nodeAddr = FabricState->SelectNodeAddress(nodeId);

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, port, sendIntfId, this);
    if (ExchangeCtx == NULL)
    {
        PacketBuffer::Free(payload);
        return WEAVE_ERROR_NO_MEMORY;
    }

    return SendEchoRequest(payload);
}

WEAVE_ERROR WeaveEchoClient::SendEchoRequest(PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Configure the encryption and signature types to be used to send the request.
    ExchangeCtx->EncryptionType = EncryptionType;
    ExchangeCtx->KeyId = KeyId;

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleResponse;

    ExchangeCtx->OnKeyError = HandleKeyError;

    // Send an Echo Request message.  Discard the exchange context if the send fails.
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    AckReceived = false;
    ResponseReceived = false;

    if (RequestAck)
    {
        ExchangeCtx->OnAckRcvd = HandleAckRcvd;
        ExchangeCtx->OnSendError = HandleSendError;
        ExchangeCtx->mWRMPConfig.mAckPiggybackTimeout = WRMPACKDelay;
        ExchangeCtx->mWRMPConfig.mInitialRetransTimeout = WRMPRetransInterval;
        ExchangeCtx->mWRMPConfig.mActiveRetransTimeout = WRMPRetransInterval;
        ExchangeCtx->mWRMPConfig.mMaxRetrans = WRMPRetransCount;
        err = ExchangeCtx->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payload, ExchangeContext::kSendFlag_RequestAck, &appContext);
    }
    else
        err = ExchangeCtx->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payload);
#else // !WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    err = ExchangeCtx->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payload);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    payload = NULL;
    if (err != WEAVE_NO_ERROR && ExchangeCtx)
    {
        ExchangeCtx->Abort();
        ExchangeCtx = NULL;
    }

    return err;
}

void WeaveEchoClient::HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveEchoClient *echoApp = (WeaveEchoClient *)ec->AppState;

    // Assert that the exchange context matches the client's current context.
    // This should never fail because even if SendEchoRequest is called
    // back-to-back, the second call will call Close() on the first exchange,
    // which clears the OnMessageReceived callback.
    VerifyOrDie(echoApp && ec == echoApp->ExchangeCtx);

    // Verify that the message is an Echo Response.
    // If not, close the exchange and free the payload.
    if (profileId != kWeaveProfile_Echo || msgType != kEchoMessageType_EchoResponse)
    {
        ec->Close();
        echoApp->ExchangeCtx = NULL;
        ExitNow();
    }

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    echoApp->ResponseReceived = true;

    if (!echoApp->RequestAck || echoApp->AckReceived || (!echoApp->OnAckRcvdReceived))
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    {
        // Remove the EC from the app state now. OnEchoResponseReceived can call
        // SendEchoRequest and install a new one. We abort rather than close
        // because we no longer care whether the echo request message has been
        // acknowledged at the transport layer.
        echoApp->ExchangeCtx->Abort();
        echoApp->ExchangeCtx = NULL;
    }

    // Call the registered OnEchoResponseReceived handler, if any.
    if (echoApp->OnEchoResponseReceived != NULL)
        echoApp->OnEchoResponseReceived(msgInfo->SourceNodeId, (pktInfo != NULL) ? pktInfo->SrcAddress : IPAddress::Any, payload);

exit:
    // Free the payload buffer.
    PacketBuffer::Free(payload);
}

void WeaveEchoClient::HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    HandleError(ec, conErr);
}

void WeaveEchoClient::HandleSendError(ExchangeContext *ec, WEAVE_ERROR sendErr, void *msgCtxt)
{
    HandleError(ec, sendErr);
}

void WeaveEchoClient::HandleKeyError(ExchangeContext *ec, WEAVE_ERROR keyErr)
{
    HandleError(ec, keyErr);
}

void WeaveEchoClient::HandleError(ExchangeContext *ec, WEAVE_ERROR err)
{
    WeaveEchoClient *echoApp = (WeaveEchoClient *)ec->AppState;

    VerifyOrExit(echoApp && echoApp->ExchangeCtx, /* no action */);

    VerifyOrDie(ec == echoApp->ExchangeCtx);

    if (err != WEAVE_NO_ERROR)
    {
        echoApp->ExchangeCtx->Abort();
    }
    else
    {
        echoApp->ExchangeCtx->Close();
    }
exit:
    echoApp->ExchangeCtx = NULL;
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
void WeaveEchoClient::HandleAckRcvd(ExchangeContext *ec, void *msgCtxt)
{
    WeaveEchoClient *echoApp = (WeaveEchoClient *)ec->AppState;

    // Assert that the exchange context matches the client's current context.
    // This should never fail because even if SendEchoRequest is called
    // back-to-back, the second call will call Close() on the first exchange,
    // which clears the OnAckReceived callback.
    VerifyOrDie(echoApp && ec == echoApp->ExchangeCtx);

    echoApp->AckReceived = true;

    if (echoApp->ResponseReceived)
    {
        // Remove the EC from the app state now. OnAckRcvdReceived can call
        // SendEchoRequest and install a new one.
        echoApp->ExchangeCtx->Close();
        echoApp->ExchangeCtx = NULL;
    }

    // Call the registered OnEchoResponseReceived handler, if any.
    if (echoApp->OnAckRcvdReceived != NULL)
        echoApp->OnAckRcvdReceived(msgCtxt);
}

void WeaveEchoClient::SetRequestAck(bool requestAck)
{
    RequestAck = requestAck;
}

void WeaveEchoClient::SetWRMPACKDelay(uint16_t aWRMPACKDelay)
{
    WRMPACKDelay = aWRMPACKDelay;
}

void WeaveEchoClient::SetWRMPRetransInterval(uint32_t aWRMPRetransInterval)
{
    WRMPRetransInterval = aWRMPRetransInterval;
}

void WeaveEchoClient::SetWRMPRetransCount(uint8_t aWRMPRetransCount)
{
    WRMPRetransCount = aWRMPRetransCount;
}
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

} // namespace Profiles
} // namespace Weave
} // namespace nl

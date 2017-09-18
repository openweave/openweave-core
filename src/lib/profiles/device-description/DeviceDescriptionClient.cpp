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
 *      This file implements the Device Description Client, which
 *      generates and transmits IdentifyRequest messages and processes
 *      responses to discover Weave devices.
 *
 */

#include <string.h>
#include <ctype.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace DeviceDescription {

using namespace nl::Weave::Encoding;

DeviceDescriptionClient::DeviceDescriptionClient()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnIdentifyResponseReceived = NULL;
    ExchangeCtx = NULL;
}

/**
 * Initialize the Device Description client state.
 *
 * param[in]    exchangeMgr     A pointer to the Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     When a remote passive rendezvous server
 *                                          has already been registered.
 * @retval #WEAVE_NO_ERROR                  On success.
 */
WEAVE_ERROR DeviceDescriptionClient::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    OnIdentifyResponseReceived = NULL;
    ExchangeCtx = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Shutdown the Device Description Client.
 *
 * This function closes any active exchange context and resets pointers.  The object
 * can be reused by calling the #Init method again.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
WEAVE_ERROR DeviceDescriptionClient::Shutdown()
{
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    ExchangeMgr = NULL;
    FabricState = NULL;
    OnIdentifyResponseReceived = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Send a broadcast IdentifyRequest message to discover Weave nodes.
 *
 * @param[in] msg   A reference to the IdentifyRequest message to send.
 *
 * @retval
 */
WEAVE_ERROR DeviceDescriptionClient::SendIdentifyRequest(const IdentifyRequestMessage& msg)
{
    return SendIdentifyRequest(IPAddress::Any, msg);
}

/**
 * Send an IdentifyRequest message to a particular IP address.
 *
 * @param[in] nodeAddr  A reference to the IP address of the Weave node to query.
 * @param[in] msg       A reference to the IdentifyRequest message to send.
 *
 * @retval #WEAVE_ERROR_NO_MEMORY  If allocating the exchange context of packet buffer fails.
 * @retval #WEAVE_NO_ERROR         On success.
 * @retval other                   Other Weave or platform-specific error codes indicating that an error
 *                                 occurred preventing the sending of the IdentifyRequest.
 */
WEAVE_ERROR DeviceDescriptionClient::SendIdentifyRequest(const IPAddress& nodeAddr, const IdentifyRequestMessage& msg)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    // Discard any existing exchange context. Effectively we can only have one exchange with
    // a single node at any one time.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    // Create a new exchange context.
    ExchangeCtx = ExchangeMgr->NewContext(msg.TargetDeviceId, nodeAddr, this);
    VerifyOrExit(ExchangeCtx != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Arrange for messages in this exchange to go to our response handler.
    ExchangeCtx->OnMessageReceived = HandleResponse;

    // Encode the message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    // Send the Request message.  Discard the exchange context if the send fails.
    err = ExchangeCtx->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, msgBuf);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR && ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }
    return err;
}

/**
 * Cancel an in-progress IdentifyRequest exchange awaiting a response.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
WEAVE_ERROR DeviceDescriptionClient::CancelExchange()
{
    // Discard any existing exchange context.
    if (ExchangeCtx != NULL)
    {
        ExchangeCtx->Close();
        ExchangeCtx = NULL;
    }

    return WEAVE_NO_ERROR;
}

void DeviceDescriptionClient::HandleResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err;
    DeviceDescriptionClient *client = (DeviceDescriptionClient *)ec->AppState;
    IdentifyResponseMessage respMsg;

    // Verify that the message is an Identify Response.
    if (profileId != kWeaveProfile_DeviceDescription || msgType !=  kMessageType_IdentifyResponse)
    {
        // TODO: handle unexpected response
        ExitNow();
    }

    // Verify that the exchange context matches our current context. Bail if not.
    if (ec != client->ExchangeCtx)
        ExitNow();

    // Verify the user has registered a OnEchoResponseReceived handler.
    if (client->OnIdentifyResponseReceived == NULL)
        ExitNow();

    // Decode the response, fail if it is invalid.
    err = IdentifyResponseMessage::Decode(payload, respMsg);
    SuccessOrExit(err);

    // Call the registered OnEchoResponseReceived handler.
    client->OnIdentifyResponseReceived(client->AppState, msgInfo->SourceNodeId,
        (pktInfo != NULL) ? pktInfo->SrcAddress : IPAddress::Any, respMsg);

exit:
    // Free the payload buffer.
    PacketBuffer::Free(payload);
}


} // namespace DeviceDescription
} // namespace Profiles
} // namespace Weave
} // namespace nl

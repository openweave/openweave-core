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
 *      This file implements the Device Description Server, which receives
 *      and processes IndentifyRequest messages, responding as appropriate
 *      with additional details of the server device.
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

DeviceDescriptionServer::DeviceDescriptionServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    AppState = NULL;
}

/**
 * Initialize the Device Description Server state and register to receive
 * Device Description messages.
 *
 * param[in]    exchangeMgr     A pointer to the Weave Exchange Manager.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE                         When a remote passive rendezvous server
 *                                                              has already been registered.
 * @retval #WEAVE_ERROR_TOO_MANY_UNSOLICITED_MESSAGE_HANDLERS   When too many unsolicited message
 *                                                              handlers are registered.
 * @retval #WEAVE_NO_ERROR                                      On success.
 */
WEAVE_ERROR DeviceDescriptionServer::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;

    // Register to receive unsolicited Echo Request messages from the exchange manager.
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, HandleRequest, this);

    return WEAVE_NO_ERROR;
}

/**
 * Shutdown the Device Description Server.
 *
 * @retval #WEAVE_NO_ERROR unconditionally.
 */
// TODO: Additional documentation detail required (i.e. how this function impacts object lifecycle).
WEAVE_ERROR DeviceDescriptionServer::Shutdown()
{
    if (ExchangeMgr != NULL)
    {
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest);
        ExchangeMgr = NULL;
    }

    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

void DeviceDescriptionServer::HandleRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err;
    DeviceDescriptionServer *server = (DeviceDescriptionServer *) ec->AppState;
    IdentifyRequestMessage reqMsg;
    IdentifyResponseMessage respMsg;
    bool sendResp;

    // NOTE: we already know this is an Identify Request message because we explicitly registered with the
    // Exchange Manager for unsolicited Identify requests.

    // If no callback has been registered, simply return without sending a response.
    if (server->OnIdentifyRequestReceived == NULL)
        ExitNow();

    // Decode the request message.
    err = IdentifyRequestMessage::Decode(payload, msgInfo->DestNodeId, reqMsg);
    SuccessOrExit(err);

    // Call the callback to handle the request and generate the full response.
    server->OnIdentifyRequestReceived(server->AppState, msgInfo->SourceNodeId, (pktInfo != NULL) ? pktInfo->SrcAddress : IPAddress::Any,
            reqMsg, sendResp, respMsg);

    // If the user has opted NOT to send a response, return now.
    if (!sendResp)
        ExitNow();

    // Release the request buffer and allocate a new one to hold the response.
    PacketBuffer::Free(payload);
    payload = PacketBuffer::New();
    VerifyOrExit(payload != NULL, );

    // Encode the response message.
    err = respMsg.Encode(payload);
    SuccessOrExit(err);

    // Send the response back to the requestor.
    ec->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyResponse, payload);
    payload = NULL;

exit:

    // Discard the exchange context.
    ec->Close();

    // Free the payload buffer if it hasn't been reused.
    if (payload != NULL)
        PacketBuffer::Free(payload);
}


} // namespace DeviceDescription
} // namespace Profiles
} // namespace Weave
} // namespace nl

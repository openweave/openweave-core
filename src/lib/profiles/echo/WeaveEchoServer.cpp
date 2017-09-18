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
 *      responder (server).
 *
 */

#include "WeaveEcho.h"

namespace nl {
namespace Weave {
namespace Profiles {

WeaveEchoServer::WeaveEchoServer()
{
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnEchoRequestReceived = NULL;
}

WEAVE_ERROR WeaveEchoServer::Init(WeaveExchangeManager *exchangeMgr)
{
    // Error if already initialized.
    if (ExchangeMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    OnEchoRequestReceived = NULL;

    // Register to receive unsolicited Echo Request messages from the exchange manager.
    ExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, HandleEchoRequest,
            this);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveEchoServer::Shutdown()
{
    if (ExchangeMgr != NULL)
    {
        ExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Echo, kEchoMessageType_EchoRequest);
        ExchangeMgr = NULL;
    }

    FabricState = NULL;

    return WEAVE_NO_ERROR;
}

void WeaveEchoServer::HandleEchoRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveEchoServer *echoApp = (WeaveEchoServer *) ec->AppState;

    // NOTE: we already know this is an Echo Request message because we explicitly registered with the
    // Exchange Manager for unsolicited Echo Requests.

    // Call the registered OnEchoRequestReceived handler, if any.
    if (echoApp->OnEchoRequestReceived != NULL)
        echoApp->OnEchoRequestReceived(ec->PeerNodeId, ec->PeerAddr, payload);

    // Send an Echo Response back to the sender.
    ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload);

    // TODO: fix bug that results in payload not being freed.

    // Discard the exchange context.
    ec->Close();
}

} // namespace Profiles
} // namespace Weave
} // namespace nl

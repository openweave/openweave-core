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
 *      Implementation for the WeaveEchoServer object.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveEcho.h"
#include "WeaveEchoServer.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace Echo_Next {

// ==================== Public Members ====================

/**
 * Default constructor for WeaveEchoServer.
 */
WeaveEchoServer::WeaveEchoServer()
{
    AppState = NULL;
    FabricState = NULL;
    ExchangeMgr = NULL;
    OnEchoRequestReceived = NULL;
    mEventCallback = NULL;
}

/**
 * Initialize a WeaveEchoServer object.
 *
 * Initialize a WeaveEchoServer object to respond to echo messages from a peer.
 *
 *  @param[in]  exchangeMgr     A pointer to the WeaveExchangeManager object.
 *  @param[in]  eventCallback   A pointer to a function that will be called by the WeaveEchoServer object
 *                              to deliver API events to the application.
 *  @param[in]  appState        A pointer to an application-defined object which will be passed back
 *                              to the application whenever an API event occurs.
 */
WEAVE_ERROR WeaveEchoServer::Init(WeaveExchangeManager * exchangeMgr, EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Error if already initialized.
    VerifyOrExit(ExchangeMgr == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

#if DEBUG
    // Verify that the application's event callback function correctly calls the default handler.
    //
    // NOTE: If your code receives WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED it means that the event hander
    // function you supplied for the echo client does not properly call WeaveEchoClient::DefaultEventHandler
    // for unrecognized/unhandled events.
    //
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();
        eventCallback(appState, kEvent_DefaultCheck, inParam, outParam);
        VerifyOrExit(outParam.DefaultHandlerCalled, err = WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED);
    }
#endif

    // Register to receive unsolicited Echo Request messages from the exchange manager.
    err = exchangeMgr->RegisterUnsolicitedMessageHandler(
            kWeaveProfile_Echo, kEchoMessageType_EchoRequest, HandleEchoRequest, this);
    SuccessOrExit(err);

    ExchangeMgr = exchangeMgr;
    FabricState = exchangeMgr->FabricState;
    AppState = appState;
    OnEchoRequestReceived = NULL;
    mEventCallback = eventCallback;

exit:
    return err;
}

/**
 * Shutdown a previously initialized WeaveEchoServer object.
 *
 * Note that this method can only be called if the Init() method has been called previously.
 */
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

/**
 *  Default handler for WeaveEchoServer API events.
 *
 *  Applications are required to call this method for any API events that they don't recognize or handle.
 *  Supplied parameters must be the same as those passed by the server object to the application's event handler
 *  function.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the server object.
 *  @param[in]  eventType   Event ID passed by the event callback
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback
 *
 */
void WeaveEchoServer::DefaultEventHandler(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam)
{
    outParam.DefaultHandlerCalled = true;
}

/**
 * Initialize a WeaveEchoServer object.
 *
 * Initialize a WeaveEchoServer object to respond to echo messages from a peer.
 *
 * DEPRECATED: Please use Init(WeaveExchangeManager * exchangeMgr, EventCallback eventCallback, void * appState).
 *
 *  @param[in]  exchangeMgr     A pointer to the WeaveExchangeManager object.
 *  @param[in]  eventCallback   A pointer to a function that will be called by the WeaveEchoServer object
 *                              to deliver API events to the application.
 *  @param[in]  appState        A pointer to an application-defined object which will be passed back
 *                              to the application whenever an API event occurs.
 */
WEAVE_ERROR WeaveEchoServer::Init(WeaveExchangeManager *exchangeMgr)
{
    return Init(exchangeMgr, DefaultEventHandler, NULL);
}

// ==================== Private Members ====================

/**
 * Handle an incoming EchoRequest message from a peer.
 */
void WeaveEchoServer::HandleEchoRequest(ExchangeContext * ec, const IPPacketInfo * pktInfo,
        const WeaveMessageInfo * msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer * payload)
{
    WeaveEchoServer * server = (WeaveEchoServer *) ec->AppState;
    System::Layer *systemLayer = server->ExchangeMgr->MessageLayer->SystemLayer;
    InEventParam inParam;
    OutEventParam outParam;

    // NOTE: we already know this is an Echo Request message because we explicitly registered with the
    // Exchange Manager for unsolicited Echo Requests.

    inParam.Clear();
    outParam.Clear();

    // If the application is using the new event-based API, deliver an EchoRequestReceived event.
    if (server->mEventCallback != DefaultEventHandler)
    {
        inParam.Source = server;
        inParam.EchoRequestReceived.MessageInfo = msgInfo;
        inParam.EchoRequestReceived.EC = ec;
        inParam.EchoRequestReceived.Payload = payload;

        server->mEventCallback(server->AppState, kEvent_EchoRequestReceived, inParam, outParam);
    }

    // Otherwise, call the legacy OnEchoRequestReceived handler, if set.
    else if (server->OnEchoRequestReceived != NULL)
    {
        server->OnEchoRequestReceived(ec->PeerNodeId, ec->PeerAddr, payload);
    }

    // If the application has requested to suppress the response, simply abort the exchange context
    // and free the payload buffer.
    if (outParam.EchoRequestReceived.SuppressResponse)
    {
        ec->Abort();
        PacketBuffer::Free(payload);
    }

    // Otherwise arrange to send a response...
    else
    {
        // Save a pointer to the exchange context in the reserved area of the payload buffer.
        // This makes it so the payload buffer captures the entire state of a pending request,
        // eliminating the need for the server object to manage per-request state.
        payload->EnsureReservedSize(sizeof(ec));
        memcpy(payload->Start() - sizeof(ec), &ec, sizeof(ec));

        // If no response delay has been requested, send the response immediately.  Otherwise
        // arm a timer to send the response after an appropriate delay.
        if (outParam.EchoRequestReceived.ResponseDelay == 0)
        {
            SendEchoResponse(systemLayer, payload, WEAVE_NO_ERROR);
        }
        else
        {
            systemLayer->StartTimer(outParam.EchoRequestReceived.ResponseDelay, SendEchoResponse, payload);
        }
    }
}

/**
 * Send an EchoResponse message to the peer.
 */
void WeaveEchoServer::SendEchoResponse(System::Layer * systemLayer, void * appState, System::Error /* ignored */)
{
    PacketBuffer * payload = (PacketBuffer *)appState;
    InEventParam inParam;
    OutEventParam outParam;
    ExchangeContext *& ec = inParam.EchoResponseSent.EC;
    WeaveEchoServer *& server = inParam.Source;
    WEAVE_ERROR & err = inParam.EchoResponseSent.Error;

    inParam.Clear();
    outParam.Clear();

    // Recover the pointer to exchange context from the reserved area of payload buffer.
    memcpy(&ec, payload->Start() - sizeof(ec), sizeof(ec));

    // Get a pointer to the server object.
    server = (WeaveEchoServer *) ec->AppState;

    // Exit immediately if the server object has been shutdown.
    VerifyOrExit(server->ExchangeMgr != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Since we may be re-using the inbound EchoRequest buffer to send the EchoResponse, if necessary,
    // adjust the position of the payload within the buffer to ensure there is enough room for the
    // outgoing network headers.  This is necessary because in some network stack configurations, the
    // incoming header size may be smaller than the outgoing size.
    payload->EnsureReservedSize(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE);

    // Send an Echo Response message back to the sender containing the given payload.
    err = ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload);
    payload = NULL;

    // Deliver a EchoResponseSent API event to the application.
    server->mEventCallback(server->AppState, kEvent_EchoResponseSent, inParam, outParam);

exit:

    // If successful, close the exchange context gracefully; otherwise abort it.
    if (err == WEAVE_NO_ERROR)
    {
        ec->Close();
    }
    else
    {
        ec->Abort();
    }

    // Release the packet buffer if necessary.
    PacketBuffer::Free(payload);
}

// ==================== Documentation for Inline Public Members ====================

/**
 * @fn EventCallback WeaveEchoServer::GetEventCallback(void) const
 *
 * Returns a pointer to the API event callback function currently configured on the WeaveEchoServer object.
 */

/**
 * @fn void WeaveEchoServer::SetEventCallback(EventCallback eventCallback)
 *
 * Sets the API event callback function on the WeaveEchoServer object.
 */


} // namespace Echo_Next
} // namespace Profiles
} // namespace Weave
} // namespace nl

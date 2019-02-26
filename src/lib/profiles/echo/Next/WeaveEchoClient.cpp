/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *      Implementation for the WeaveEchoClient object.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include "WeaveEcho.h"
#include "WeaveEchoClient.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace Echo_Next {

// ==================== Public Members ====================

/**
 * Initialize a WeaveEchoClient object.
 *
 * Initialize a WeaveEchoClient object in preparation for sending echo messages to a peer.
 *
 *  @param[in]  binding         A Binding object that will be used to establish communication with the
 *                              peer node.
 *  @param[in]  eventCallback   A pointer to a function that will be called by the WeaveEchoClient object
 *                              to deliver API events to the application.
 *  @param[in]  appState        A pointer to an application-defined object which will be passed back
 *                              to the application whenever an API event occurs.
 */
WEAVE_ERROR WeaveEchoClient::Init(Binding * binding, EventCallback eventCallback, void * appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(binding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(eventCallback != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    AppState = appState;
    mBinding = binding;
    mEventCallback = eventCallback;
    mSendIntervalMS = 0;
    mEC = NULL;
    mQueuedPayload = NULL;
    mState = kState_NotInitialized;
    mFlags = 0;

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

    // Retain a reference to the binding object.
    binding->AddRef();

    mState = kState_Idle;

exit:
    return err;
}

/**
 * Shutdown a previously initialized WeaveEchoClient object.
 *
 * Note that this method can only be called if the Init() method has been called previously.
 */
void WeaveEchoClient::Shutdown(void)
{
    // Stop any request that is currently in progress.
    Stop();

    // Release the reference to the binding.
    if (mBinding != NULL)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    mEventCallback = NULL;
    mState = kState_NotInitialized;
}

/**
 * Send an EchoRequest message to the peer.
 *
 * This method initiates the process of sending an EchoRequest message to the peer node.
 * If and when a corresponding EchoResponse message is received it will be delivered to
 * the application via the ResponseReceived API event.
 *
 * When forming the EchoRequest message, the WeaveEchoClient makes a request to the
 * application, via the PreparePayload API event, to prepare the payload of the message.
 *
 * If the Binding object is not in the Ready state when this method is called, a request
 * will be made to Binding::RequestPrepare() method to initiate on-demand preparation.
 * The send operation will then be queued until this process completes.  The maximum
 * depth of this queue is one.  Thus any call to Send() while there is a previous send
 * in the queue will result in only a single EchoRequest being sent.
 *
 * Calling Send() while the WeaveEchoClient is in send-repeating mode (i.e. because of
 * a previous call to SendRepeating()) has the effect of accelerating and resetting the
 * send cycle but does not take the WeaveEchoClient out of send-repeating mode.
 */
WEAVE_ERROR WeaveEchoClient::Send(void)
{
    return Send(NULL);
}

/**
 * Send an EchoRequest message to the peer with a specific payload.
 *
 * This method initiates the process of sending an EchoRequest message to the peer node.
 * The contents of the supplied payload buffer will be sent to the peer as the payload
 * of the EchoRequest message.  If and when a corresponding EchoResponse message is
 * received it will be delivered to the application via the ResponseReceived API event.
 *
 * Upon calling this method, ownership of the supplied payload buffer passes to the
 * WeaveEchoClient object, which has the responsibility to free it. This is true regardless
 * of whether method completes successfully or with an error.
 *
 * If the Binding object is not in the Ready state when this method is called, a request
 * will be made to Binding::RequestPrepare() method to initiate on-demand preparation.
 * The send operation will then be queued until this process is complete.  The maximum
 * depth of this queue is one.  Thus any call to Send() while there is a previous send
 * in the queue will result in only a single EchoRequest being sent.
 *
 * Calling Send() while the WeaveEchoClient is in send-repeating mode (i.e. because of
 * a previous call to SendRepeating()) has the effect of accelerating and resetting the
 * send cycle but does not take the WeaveEchoClient out of send-repeating mode.
 *
 *  @param[in]  payloadBuf      A PacketBuffer object containing payload data to be sent
 *                              to the peer.  Ownership of this buffer passes to the
 *                              WeaveEchoClient object in all cases.
 */
WEAVE_ERROR WeaveEchoClient::Send(PacketBuffer * payloadBuf)
{
    // Queue the supplied payload buffer, to be sent on the next EchoRequest.  Note that
    // this queue has a max depth of 1.
    PacketBuffer::Free(mQueuedPayload);
    mQueuedPayload = payloadBuf;

    // Initiate sending.
    return DoSend(false);
}

/**
 * Initiate sending a repeating sequence of EchoRequest messages to the peer.
 *
 * This method initiates a repeating process of sending EchoRequest messages to the peer.
 * As EchoResponse messages are received from the peer they are delivered to the application
 * via the ResponseReceived API event.
 *
 * When SendRepeating() is called, the WeaveEchoClient enters send-repeating mode in which
 * it stays until Stop() is called or a Binding error occurs.  Calling SendRepeating()
 * multiple times has the effect of resetting the send cycle and updating the interval.
 *
 * The initial send of a sequence occurs at the time SendRepeating() is called, or whenever
 * the Binding becomes ready after SendRepeating() is called (see below).  Subsequent sends
 * occur thereafter at the specified interval.
 *
 * Each time a send occurs, the WeaveEchoClient makes a request to the application, via the
 * PreparePayload API event, to prepare the payload of the message.
 *
 * If the Binding object is not in the Ready state when it comes time to send a message,
 * a request will be made to the Binding::RequestPrepare() method to initiate on-demand
 * preparation.  Further repeated message sends will be paused until this process completes.
 * A failure during on-demand Binding preparation will cause the WeaveEchoClient to leave
 * send-repeating mode.
 *
 */
WEAVE_ERROR WeaveEchoClient::SendRepeating(uint32_t sendIntervalMS)
{
    // Enable SendRepeating mode.
    SetFlag(kFlag_SendRepeating);

    // Set the requested send interval.
    mSendIntervalMS = sendIntervalMS;

    // Initiate sending.
    return DoSend(false);
}

/**
 * Stops any Echo exchange in progress and cancels send-repeating mode.
 */
void WeaveEchoClient::Stop(void)
{
    // Abort any request currently in progress.
    ClearRequestState();

    // Free any queued payload buffer.
    PacketBuffer::Free(mQueuedPayload);
    mQueuedPayload = NULL;

    // Cancel the send timer.
    CancelSendTimer();

    // Clear the SendRepeating mode flag.
    ClearFlag(kFlag_SendRepeating);

    // Enter the Idle state.
    mState = kState_Idle;
}

/**
 *  Default handler for WeaveEchoClient API events.
 *
 *  Applications are required to call this method for any API events that they don't recognize or handle.
 *  Supplied parameters must be the same as those passed by the client object to the application's event handler
 *  function.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the client object.
 *  @param[in]  eventType   Event ID passed by the event callback
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback
 *
 */
void WeaveEchoClient::DefaultEventHandler(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam)
{
    outParam.DefaultHandlerCalled = true;

    // If the application didn't handle the PreparePayload event, arrange to send a zero-length payload.
    if (eventType == kEvent_PreparePayload)
    {
        outParam.PreparePayload.Payload = PacketBuffer::NewWithAvailableSize(0);
        outParam.PreparePayload.PrepareError = (outParam.PreparePayload.Payload != NULL)
                ? WEAVE_NO_ERROR : WEAVE_ERROR_NO_MEMORY;
    }
}

// ==================== Private Members ====================

/**
 * Performs the work of initiating Binding preparation and sending an EchoRequest message.
 */
WEAVE_ERROR WeaveEchoClient::DoSend(bool callbackOnError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer * payloadBuf = NULL;
    bool reqSent = false;

    // Set the protocol callback on the binding object.  NOTE: This needs to happen after the
    // app has explicitly started the sending process by calling either Send() or SendRepeating().
    // Otherwise the client object might receive callbacks from the binding before its ready.
    // Even though it happens redundantly, we do it here to ensure it happens at the right point.
    mBinding->SetProtocolLayerCallback(HandleBindingEvent, this);

    // If there's a request already in progress, abort it.
    if (mState == kState_RequestInProgress)
    {
        bool respReceived = GetFlag(kFlag_ResponseReceived);

        // Clear the request state.
        ClearRequestState();

        // If no EchoResponse message was received...
        if (!respReceived)
        {
            // Deliver a RequestAborted API event to the application.
            // Note that the application MAY change the state of the client object during this
            // callback by calling one of the API methods.  For example, it may call Stop() to
            // signal that no further requests should be sent.
            {
                InEventParam inParam;
                OutEventParam outParam;
                inParam.Clear();
                inParam.Source = this;
                outParam.Clear();
                mEventCallback(AppState, kEvent_RequestAborted, inParam, outParam);
            }

            // If the application reset the state of the client by calling Stop(), return without
            // sending anything.
            if (mState == kState_Idle)
            {
                ExitNow();
            }
        }
    }

    // If the binding is ready ...
    if (mBinding->IsReady())
    {
        // "Dequeue" any queued payload buffer.
        payloadBuf = mQueuedPayload;
        mQueuedPayload = NULL;

        // If a payload hasn't been supplied, call back to the application to prepare one.
        // Note that the application is PROHIBITED from changing the state of the client
        // object during this callback.
        if (payloadBuf == NULL)
        {
            InEventParam inParam;
            OutEventParam outParam;
            inParam.Clear();
            inParam.Source = this;
            outParam.Clear();
            mEventCallback(AppState, kEvent_PreparePayload, inParam, outParam);
            err = outParam.PreparePayload.PrepareError;
            SuccessOrExit(err);
            payloadBuf = outParam.PreparePayload.Payload;
        }

        // Allocate and initialize a new exchange context.
        err = mBinding->NewExchangeContext(mEC);
        SuccessOrExit(err);
        mEC->AppState = this;
        mEC->OnMessageReceived = HandleResponse;
        mEC->OnResponseTimeout = HandleResponseTimeout;
        mEC->OnKeyError = HandleKeyError;
        mEC->OnConnectionClosed = HandleConnectionClosed;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        mEC->OnAckRcvd = HandleAckRcvd;
        mEC->OnSendError = HandleSendError;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

        // Enter MultiResponse mode if the peer address may result in multiple response messages.
        SetFlag(kFlag_MultiResponse, IsMultiResponseAddress(mEC->PeerAddr));

        // Enter the RequestInProgress state.
        mState = kState_RequestInProgress;

        // Send an EchoRequest message to the peer containing the specified payload.
        err = mEC->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, payloadBuf, ExchangeContext::kSendFlag_ExpectResponse);
        payloadBuf = NULL;
        SuccessOrExit(err);

        reqSent = true;
    }

    // Otherwise, if the binding needs to be prepared...
    else
    {
        // Enter the PreparingBinding state.  Once binding preparation is complete, the binding will call back to
        // the echo client, which will result in DoSend() being called again.
        mState = kState_PreparingBinding;

        // If the binding is in a state where it can be prepared, ask the application to prepare it by delivering a
        // PrepareRequested API event via the binding's callback.  At some point the binding will call back to
        // WeaveEchoClient::HandleBindingEvent() signaling that preparation has completed (successfully or otherwise).
        // Note that this callback *may* happen synchronously within the RequestPrepare() method, in which case this
        // method (DoSend) will recurse.
        if (mBinding->CanBePrepared())
        {
            err = mBinding->RequestPrepare();
            SuccessOrExit(err);
        }

        // Fail if the binding is in some state, other than Ready, that doesn't allow it to be prepared.
        else
        {
            VerifyOrExit(mBinding->IsPreparing(), err = WEAVE_ERROR_INCORRECT_STATE);
        }
    }

exit:

    // If something failed, enter either the WaitingToSend state or the Idle state, based on whether
    // SendRepeating mode is enabled.
    if (err != WEAVE_NO_ERROR)
    {
        mState = (GetFlag(kFlag_SendRepeating)) ? kState_WaitingToSend : kState_Idle;
    }

    // Free the payload buffer if it hasn't been sent or queued.
    PacketBuffer::Free(payloadBuf);

    // Arm the send timer if needed based on the current state.
    WEAVE_ERROR armTimerErr = ArmSendTimer();
    if (err == WEAVE_NO_ERROR)
    {
        err = armTimerErr;
    }

    // If needed, deliver a CommuncationError API event to the application.  Note that this may result
    // in the state of the client object changing (e.g. as a result of the application calling Stop()).
    // So the code here shouldn't presume anything about the current state after the callback.
    if (err != WEAVE_NO_ERROR)
    {
        if (callbackOnError)
        {
            DeliverCommunicationError(err);
        }
    }

    // Otherwise, if a request was sent, deliver a RequestSent API event to the application.  As above,
    // this code shouldn't presume anything about the current state after the callback.
    else if (reqSent)
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();
        mEventCallback(AppState, kEvent_RequestSent, inParam, outParam);
    }

    return err;
}

/**
 * Determines whether a Echo exchange has completed.
 */
bool WeaveEchoClient::IsRequestDone() const
{
    // The EchoRequest is considered done when we receive the EchoResponse and, if using WRMP,
    // we receive the WRMP ACK for the request.
    // HOWEVER, if multiple responses are expected, this method always returns false, and the
    // request is not considered done until: the application calls Stop() or Send(), 2) the send
    // timer fires (in SendRepeating mode) or 3) no response is received and the receive timeout
    // fires.
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    bool usingWRMP = mEC->AutoRequestAck();
#else
    bool usingWRMP = false;
#endif
    return (!GetFlag(kFlag_MultiResponse) && GetFlag(kFlag_ResponseReceived) && (!usingWRMP || GetFlag(kFlag_AckReceived)));
}

/**
 * Called when a Echo exchange completes.
 */
void WeaveEchoClient::HandleRequestDone()
{
    // Clear the request state.
    ClearRequestState();

    // Enter either WaitingToSend or Idle depending on whether SendRepeating mode is enabled.
    mState = (GetFlag(kFlag_SendRepeating)) ? kState_WaitingToSend : kState_Idle;
}

/**
 * Aborts any in-progress exchange and resets the receive state.
 */
void WeaveEchoClient::ClearRequestState()
{
    // Abort any in-progress exchange and release the exchange context.
    if (mEC != NULL)
    {
        mEC->Abort();
        mEC = NULL;
    }

    // Clear the received flags.
    ClearFlag(kFlag_ResponseReceived|kFlag_AckReceived|kFlag_MultiResponse);
}

/**
 * Arms a timer for the next send time if SendRepeating mode is enabled
 */
WEAVE_ERROR WeaveEchoClient::ArmSendTimer()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Cancel any existing timer.
    CancelSendTimer();

    // If the current state is WaitingToSend or RequestInProgress, and SendRepeating mode is
    // enabled, then arm the send timer for the specified interval.
    if ((mState == kState_WaitingToSend || mState == kState_RequestInProgress) &&
        GetFlag(kFlag_SendRepeating))
    {
        err = mBinding->GetExchangeManager()->MessageLayer->SystemLayer->StartTimer(mSendIntervalMS, HandleSendTimerExpired, this);
    }

    return err;
}

/**
 * Cancels any outstanding SendRepeating mode timer.
 */
void WeaveEchoClient::CancelSendTimer()
{
    mBinding->GetExchangeManager()->MessageLayer->SystemLayer->CancelTimer(HandleSendTimerExpired, this);
}

/**
 * Deliver a CommuncationError API event to the application.
 */
void WeaveEchoClient::DeliverCommunicationError(WEAVE_ERROR err)
{
    InEventParam inParam;
    OutEventParam outParam;

    inParam.Clear();
    inParam.Source = this;
    inParam.CommuncationError.Reason = err;

    outParam.Clear();

    mEventCallback(AppState, kEvent_CommuncationError, inParam, outParam);
}

/**
 * Process an event delivered from the binding associated with a WeaveEchoClient.
 */
void WeaveEchoClient::HandleBindingEvent(void * const appState, const Binding::EventType eventType, const Binding::InEventParam & inParam,
                                         Binding::OutEventParam & outParam)
{
    WeaveEchoClient * client = (WeaveEchoClient *)appState;

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:

        // When the binding is ready, if the client is still in the PreparingBinding state,
        // initiate sending of an EchoRequest.
        if (client->mState == kState_PreparingBinding)
        {
            client->DoSend(true);
        }

        break;

    case Binding::kEvent_PrepareFailed:

        // If binding preparation fails and the client is still in the PreparingBinding state...
        if (client->mState == kState_PreparingBinding)
        {
            // Clean-up the request state and enter either WaitingToSend or Idle depending on
            // whether SendRepeating mode is enabled
            client->HandleRequestDone();

            // If SendRepeating mode is enabled arm the send timer.  When the timer fires another
            // attempt will be made to prepare the binding.
            // Otherwise, if SendRepeating mode is NOT enabled, deliver a CommuncationError API
            // event to the application.
            if (client->GetFlag(kFlag_SendRepeating))
            {
                client->ArmSendTimer();
            }
            else
            {
                client->DeliverCommunicationError(inParam.PrepareFailed.Reason);
            }
        }

        break;

    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

/**
 * Called by the Weave messaging layer when a response is received on an Echo exchange.
 */
void WeaveEchoClient::HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer * payload)
{
    WeaveEchoClient * client = (WeaveEchoClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Ignore any messages other than EchoResponse.
    if (profileId != kWeaveProfile_Echo || msgType != kEchoMessageType_EchoResponse)
    {
        PacketBuffer::Free(payload);
        return;
    }

    // Record that the response has been received.
    client->SetFlag(kFlag_ResponseReceived);

    // If the request is now complete, clear the request state and enter either WaitingToSend or
    // Idle depending on whether SendRepeating mode is enabled.
    if (client->IsRequestDone())
    {
        client->HandleRequestDone();
    }

    // Deliver a ResponseReceived API event to the application.
    //
    // NOTE that ownership of the payload buffer transfers to the application during this call,
    // meaning the application is responsible for freeing it.
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = client;
        inParam.ResponseReceived.MsgInfo = msgInfo;
        inParam.ResponseReceived.Payload = payload;
        outParam.Clear();
        client->mEventCallback(client->AppState, kEvent_ResponseReceived, inParam, outParam);
    }
}

/**
 * Called by the Weave messaging layer when a timeout occurs while waiting for an EchoResponse message.
 */
void WeaveEchoClient::HandleResponseTimeout(ExchangeContext * ec)
{
    WeaveEchoClient * client = (WeaveEchoClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Clear the request state and enter either WaitingToSend or Idle depending on whether SendRepeating mode is enabled.
    client->HandleRequestDone();

    // Deliver a ResponseTimeout API event to the application.
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = client;
        outParam.Clear();
        client->mEventCallback(client->AppState, kEvent_ResponseTimeout, inParam, outParam);
    }
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when a WRM ACK is received for an EchoRequest message.
 */
void WeaveEchoClient::HandleAckRcvd(ExchangeContext * ec, void * msgCtxt)
{
    WeaveEchoClient * client = (WeaveEchoClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Record that the ACK has been received.
    client->SetFlag(kFlag_AckReceived);

    // If the request is now complete, clear the request state and enter either WaitingToSend or
    // Idle state depending on whether SendRepeating mode is enabled.
    if (client->IsRequestDone())
    {
        client->HandleRequestDone();
    }
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/**
 * Called by the Weave messaging layer when an error occurs while sending an EchoRequest message.
 */
void WeaveEchoClient::HandleSendError(ExchangeContext * ec, WEAVE_ERROR sendErr, void * msgCtxt)
{
    WeaveEchoClient * client = (WeaveEchoClient *)ec->AppState;

    VerifyOrDie(client->mState == kState_RequestInProgress);
    VerifyOrDie(client->mEC == ec);

    // Clear the request state and enter either WaitingToSend or Idle depending on whether
    // SendRepeating mode is enabled.
    client->HandleRequestDone();

    // Deliver a CommunicationError API event to the application.
    client->DeliverCommunicationError(sendErr);
}

/**
 * Called by the Weave messaging layer when the peer response to an EchoRequest message with a key error.
 */
void WeaveEchoClient::HandleKeyError(ExchangeContext * ec, WEAVE_ERROR keyErr)
{
    // Treat this the same as if a send error had occurred.
    HandleSendError(ec, keyErr, NULL);
}

/**
 * Called by the Weave messaging layer if an underlying Weave connection closes while waiting for an EchoResponse message.
 */
void WeaveEchoClient::HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    // If the other side simply closed the connection without responding, deliver a "closed unexpectedly"
    // error to the application.
    if (conErr == WEAVE_NO_ERROR)
    {
        conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    }

    // Treat this the same as if a send error had occurred.
    HandleSendError(ec, conErr, NULL);
}

/**
 * Called by send timer while in SendRepeating mode.
 */
void WeaveEchoClient::HandleSendTimerExpired(System::Layer * systemLayer, void * appState, System::Error err)
{
    WeaveEchoClient * client = (WeaveEchoClient *)appState;

    VerifyOrDie(client->mState != kState_NotInitialized);

    // Attempt to send another EchoRequest message.
    if (client->mState == kState_WaitingToSend || client->mState == kState_RequestInProgress)
    {
        client->DoSend(true);
    }
}

/**
 * Returns true if the given peer address may result in multiple responses.
 */
bool WeaveEchoClient::IsMultiResponseAddress(const IPAddress & addr)
{
#if INET_CONFIG_ENABLE_IPV4
   return addr.IsMulticast() ||  addr.IsIPv4Broadcast();
#else
   return addr.IsMulticast();
#endif
}



// ==================== Documentation for Inline Public Members ====================

/**
 * @fn State WeaveEchoClient::GetState(void) const
 *
 * Retrieve the current state of the WeaveEchoClient object.
 */

/**
 * @fn bool WeaveEchoClient::RequestInProgress() const
 *
 * Returns true if an EchoRequest has been sent and the WeaveEchoClient object is awaiting a response.
 */

/**
 * @fn bool WeaveEchoClient::IsSendRrepeating() const
 *
 * Returns true if the WeaveEchoClient object is currently in send-repeating mode.
 */

/**
 * @fn Binding * WeaveEchoClient::GetBinding(void) const
 *
 * Returns a pointer to the Binding object associated with the WeaveEchoClient.
 */

/**
 * @fn EventCallback WeaveEchoClient::GetEventCallback(void) const
 *
 * Returns a pointer to the API event callback function currently configured on the WeaveEchoClient object.
 */

/**
 * @fn void WeaveEchoClient::SetEventCallback(EventCallback eventCallback)
 *
 * Sets the API event callback function on the WeaveEchoClient object.
 */


} // namespace Echo_Next
} // namespace Profiles
} // namespace Weave
} // namespace nl

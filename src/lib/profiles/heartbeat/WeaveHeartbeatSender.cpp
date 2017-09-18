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
 *      This file implements Weave Heartbeat Sender.
 *
 */

#include "WeaveHeartbeat.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Profiles/time/WeaveTime.h>

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Heartbeat {

WeaveHeartbeatSender::WeaveHeartbeatSender()
{
    mHeartbeatBase = 0;
    mFabricState = NULL;
    mExchangeMgr = NULL;
    mBinding = NULL;
    mExchangeCtx = NULL;
    mEventCallback = NULL;
    mHeartbeatInterval_msec = 0;
    mHeartbeatPhase_msec = 0;
    mHeartbeatWindow_msec = 0;
    mSubscriptionState = 0;
    mRequestAck = false;
}

/**
 * Initialize the Weave Heartbeat Sender.
 *
 * @param[in] exchangeMgr   A pointer to the system Weave Exchange Manager.
 * @param[in] binding       A pointer to a Weave binding object which will be used to address the peer node.
 * @param[in] eventCallback A pointer to a function that will be called to notify the application of events or
 *                          state changes that occur in the sender.
 * @param[in] appState      A pointer to application-specific data.  This pointer will be returned in callbacks
 *                          to the application.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     If the WeaveHeartbeatSender object has already been initialized.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT    If any of the supplied arguments is null.
 * @retval #WEAVE_NO_ERROR                  On success.
 */
WEAVE_ERROR WeaveHeartbeatSender::Init(WeaveExchangeManager *exchangeMgr, Binding *binding, EventCallback eventCallback, void *appState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mExchangeMgr == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(exchangeMgr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(binding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(eventCallback != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    AppState = appState;
    mHeartbeatBase = 0;
    mFabricState = exchangeMgr->FabricState;
    mExchangeMgr = exchangeMgr;
    mBinding = binding;
    binding->AddRef();
    mExchangeCtx = NULL;
    mEventCallback = eventCallback;
    mHeartbeatInterval_msec = 0;
    mHeartbeatPhase_msec = 0;
    mHeartbeatWindow_msec = 0;
    mSubscriptionState = 0;
    mRequestAck = false;

    // Set the protocol callback on the binding object.  NOTE: This should only happen once the
    // app has explicitly started the subscription process by calling either InitiateSubscription() or
    // InitiateCounterSubscription().  Otherwise the client object might receive callbacks from
    // the binding before its ready.
    mBinding->SetProtocolLayerCallback(BindingEventCallback, this);

#if DEBUG
    // Verify that the application's event callback function correctly calls the default handler.
    //
    // NOTE: If your code receives WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED it means that the event hander
    // function you supplied does not properly call WeaveHeartbeatSender::DefaultEventHandler for unrecognized/
    // unhandled events.
    //
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();
        mEventCallback(appState, kEvent_DefaultCheck, inParam, outParam);
        VerifyOrExit(outParam.DefaultHandlerCalled, err = WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED);
    }
#endif

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Shutdown();
    }
    return err;
}

/**
 * Shutdown the Weave Heartbeat Sender.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 */
WEAVE_ERROR WeaveHeartbeatSender::Shutdown()
{
    if (mExchangeMgr != NULL)
    {
        StopHeartbeat();
    }

    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Abort();
        mExchangeCtx = NULL;
    }

    if (mBinding != NULL)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    mExchangeMgr = NULL;
    mFabricState = NULL;
    mEventCallback = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Get heartbeat timing configuration
 *
 * @param[out] interval     A reference to an integer to receive the heartbeat interval.
 * @param[out] phase        A reference to an integer to receive the heartbeat phase.
 * @param[out] window       A reference to an integer to receive the heartbeat randomization window.
 *
 */
void WeaveHeartbeatSender::GetConfiguration(uint32_t& interval, uint32_t& phase, uint32_t& window) const
{
    interval = mHeartbeatInterval_msec;
    phase = mHeartbeatPhase_msec;
    window = mHeartbeatWindow_msec;
}

/**
 * Set heartbeat timing configuration
 *
 * @param[in] interval      Interval to use when sending Weave Heartbeat messages.
 * @param[in] phase         Phase to use when sending Weave Heartbeat messages.
 * @param[in] window        Window range to use for choosing random interval
 *
 */
void WeaveHeartbeatSender::SetConfiguration(uint32_t interval, uint32_t phase, uint32_t window)
{
    mHeartbeatInterval_msec = interval;
    mHeartbeatPhase_msec = phase;
    mHeartbeatWindow_msec = window;
}

/**
 * Start sending Weave Heartbeat messages.
 *
 * @retval #INET_ERROR_NO_MEMORY            if StartTimer() failed
 * @retval #WEAVE_NO_ERROR                  on success
 *
 */
WEAVE_ERROR WeaveHeartbeatSender::StartHeartbeat()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mHeartbeatInterval_msec > 0, err = WEAVE_ERROR_INCORRECT_STATE);

    mHeartbeatBase = GetHeartbeatBase();

    err = ScheduleHeartbeat();

exit:
    return err;
}

/**
 * Schedule sending Weave Heartbeat messages.
 *
 * @retval #WEAVE_SYSTEM_ERROR_NO_MEMORY    if StartTimer() failed
 * @retval #WEAVE_NO_ERROR                  on success
 *
 */
WEAVE_ERROR WeaveHeartbeatSender::ScheduleHeartbeat()
{
    // deltaTicks is mostly +ve and less than mHeartbeatInterval_msec since heartbeatBase is one interval ahead
    int32_t deltaTicks = (int32_t)(mHeartbeatBase - GetPlatformTimeMs());
    int32_t interval = deltaTicks + mHeartbeatPhase_msec + GetRandomInterval(0, mHeartbeatWindow_msec);

    // Update the mHeartBeatBase after the interval has been calculated so as to not
    // add the heartbeat interval twice to the base (causing heartbeat to be missed at the
    // first interval).
    mHeartbeatBase += mHeartbeatInterval_msec;

    // Bounds check for interval
    if (interval < 0)
        interval = 0;

    return mExchangeMgr->MessageLayer->SystemLayer->StartTimer(static_cast<uint32_t>(interval), HandleHeartbeatTimer, this);
}

/**
 * Stop sending Weave Heartbeat messages.
 *
 * @retval #WEAVE_NO_ERROR                  unconditionally
 *
 */
WEAVE_ERROR WeaveHeartbeatSender::StopHeartbeat()
{
    mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(HandleHeartbeatTimer, this);

    return WEAVE_NO_ERROR;
}

/**
 * Send a Weave Heartbeat message now.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE     if WeaveHeartbeatSender is not initialized
 * @retval #WEAVE_NO_ERROR                  on success
 *
 */
WEAVE_ERROR WeaveHeartbeatSender::SendHeartbeatNow()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const bool scheduleNextHeartbeat = true;

    VerifyOrExit(mExchangeMgr != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    SendHeartbeat(!scheduleNextHeartbeat);

exit:
    return err;
}

/**
 * Get epoch time base for Weave Heartbeat messages.
 *
 */
uint64_t WeaveHeartbeatSender::GetHeartbeatBase()
{
    uint64_t now = GetPlatformTimeMs();

    // Divide 64-bit now in msec by 1000 and return 32-bit now in sec
    uint32_t now_sec = Platform::DivideBy1000(now);

    // Convert heartbeat interval to sec
    uint32_t interval_sec = mHeartbeatInterval_msec / 1000;

    // Aligned to the next heartbeat interval
    return 1000ULL * (now_sec / interval_sec + 1) * interval_sec;
}

/**
 * Get UTC time or time since boot in ms if UTC time not available
 */
uint64_t WeaveHeartbeatSender::GetPlatformTimeMs(void)
{
    WEAVE_ERROR err;
    uint64_t now_ms;

    err = Platform::Time::GetSystemTimeMs((Profiles::Time::timesync_t *)&now_ms);

    if (err || (!now_ms))
    {
        now_ms = static_cast<uint64_t>(System::Timer::GetCurrentEpoch());
    }
    return now_ms;
}

/**
 * Get random interval within a target range for a Weave Heartbeat
 *
 * @param[in] minVal            Min value of the target range
 * @param[in] maxVal            Max value of the target range
 *
 * @retval #WEAVE_NO_ERROR      A random value within the target range
 *
 */
uint32_t WeaveHeartbeatSender::GetRandomInterval(uint32_t minVal, uint32_t maxVal)
{
    const uint32_t range = maxVal - minVal + 1;
    const uint32_t buckets = RAND_MAX / range;
    const uint32_t limit = buckets * range;

    uint32_t r;
    do
    {
        r = rand();
    } while (r >= limit);

    return minVal + (r / buckets);
}

/**
 * Send a Weave Heartbeat message when the timer fires.
 *
 */
void WeaveHeartbeatSender::HandleHeartbeatTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    const bool scheduleNextHeartbeat = true;
    WeaveHeartbeatSender* sender = reinterpret_cast<WeaveHeartbeatSender*>(aAppState);

    sender->SendHeartbeat(scheduleNextHeartbeat);
}

/**
 * Send Weave Heartbeat messages to the peer.
 */
void WeaveHeartbeatSender::SendHeartbeat(bool scheduleNextHeartbeat)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *payload = NULL;
    bool heartbeatSentWithoutACK = false;

    // Abort any existing exchange that is still in progress.  In practice this should never be necessary.
    // However if the application configures the total WRM retry time longer than the heartbeat interval
    // we don't want to allow exchanges to pile up.
    if (mExchangeCtx != NULL)
    {
        mExchangeCtx->Abort();
        mExchangeCtx = NULL;
    }

    // Schedule the next heartbeat if requested.
    if (scheduleNextHeartbeat)
    {
        err = ScheduleHeartbeat();
        SuccessOrExit(err);
    }

    // If the binding is NOT ready, but is in a state where it can be prepared...
    if (mBinding->CanBePrepared())
    {
        // Ask the application to prepare the binding by delivering a PrepareRequested API event to it via the
        // binding's callback.  At some point the binding will call back to the WeaveHeartbeatSender object
        // signaling that preparation has completed (successfully or otherwise).  In the success case, the
        // callback will cause SendHeartbeat() to be called again, at which point the heartbeat message will
        // be sent.
        //
        // Note that the callback from the binding can happen synchronously within the RequestPrepare() method,
        // implying that SendHeartbeat() will recurse.
        //
        err = mBinding->RequestPrepare();
        SuccessOrExit(err);

        // Wait for the binding to call back.
        ExitNow();
    }

    // If the binding is in the process of being prepared, wait for it to call us back.
    if (mBinding->IsPreparing())
    {
        ExitNow();
    }

    // Verify that the binding is ready to be used.  Based on the above checks, if the binding is NOT in the
    // ready state then it is not possible to proceed.
    VerifyOrExit(mBinding->IsReady(), err = WEAVE_ERROR_INCORRECT_STATE);

    // Call back to the application to update the subscription state value.  If the application
    // chooses not to handle this event the current value will be used.
    {
        InEventParam inParam;
        OutEventParam outParam;

        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();

        mEventCallback(AppState, kEvent_UpdateSubscriptionState, inParam, outParam);
    }

    // Allocate a packet buffer to hold the heartbeat message.
    payload = PacketBuffer::NewWithAvailableSize(kHeartbeatMessageLength);
    VerifyOrExit(payload != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Encode the heartbeat message.
    nl::Weave::Encoding::Put8(payload->Start(), mSubscriptionState);
    payload->SetDataLength(kHeartbeatMessageLength);

    // Allocate and initialize a new exchange context for sending the heartbeat message.
    err = mBinding->NewExchangeContext(mExchangeCtx);
    SuccessOrExit(err);
    mExchangeCtx->AppState = this;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // If the application requested reliable transmission, arrange to request an ACK for the heartbeat message.
    // Note that if the application configured the binding to use WRM, then an ACK will always be requested
    // regardless of the state of the SendReliably flag.
    if (mRequestAck)
        mExchangeCtx->SetAutoRequestAck(true);

    // Setup callbacks for ACK received and WRM send errors.
    mExchangeCtx->OnAckRcvd = HandleAckReceived;
    mExchangeCtx->OnSendError = HandleSendError;

#endif

    // Send the heartbeat message to the peer.
    err = mExchangeCtx->SendMessage(kWeaveProfile_Heartbeat, kHeartbeatMessageType_Heartbeat, payload);
    payload = NULL;
    SuccessOrExit(err);

    heartbeatSentWithoutACK = !mExchangeCtx->AutoRequestAck();

exit:

    // Discard the packet buffer if necessary.
    if (payload != NULL)
    {
        PacketBuffer::Free(payload);
    }

    // If a heartbeat message was successfully sent WITHOUT requesting an ACK
    // OR if an error occurred while trying to send a heartbeat...
    if (heartbeatSentWithoutACK || err != WEAVE_NO_ERROR)
    {
        InEventParam inParam;
        OutEventParam outParam;

        // Discard the exchange context, as it is no longer needed.
        if (mExchangeCtx != NULL)
        {
            mExchangeCtx->Abort();
            mExchangeCtx = NULL;
        }

        // Deliver either a HeartbeatSent or HeartbeatFailed event to the application based on what happened.
        inParam.Clear();
        inParam.Source = this;
        inParam.HeartbeatFailed.Reason = err;
        outParam.Clear();
        EventType eventType = (err == WEAVE_NO_ERROR) ? kEvent_HeartbeatSent : kEvent_HeartbeatFailed;
        mEventCallback(AppState, eventType, inParam, outParam);
    }
}

/**
 * Default handler function for WeaveHeartbeatSender API events.
 *
 * Applications must call this function for any API events that they don't handle.
 */
void WeaveHeartbeatSender::DefaultEventHandler(void *appState, EventType eventType, const InEventParam& inParam, OutEventParam& outParam)
{
    // No specific behavior currently required.
    outParam.DefaultHandlerCalled = true;
}

/**
 * Handle events from the binding object associated with the WeaveHeartbeatSender.
 */
void WeaveHeartbeatSender::BindingEventCallback(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam)
{
    const bool scheduleNextHeartbeat = true;
    WeaveHeartbeatSender *sender = (WeaveHeartbeatSender *)appState;

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:
        // Binding is ready. So send the heartbeat now.
        sender->SendHeartbeat(!scheduleNextHeartbeat);
        break;

    case Binding::kEvent_PrepareFailed:
    {
        InEventParam senderInParam;
        OutEventParam senderOutParam;

        senderInParam.Clear();
        senderInParam.Source = sender;
        senderInParam.HeartbeatFailed.Reason = inParam.PrepareFailed.Reason;

        senderOutParam.Clear();

        // Deliver a HeartBeat failed event to the application.
        sender->mEventCallback(sender->AppState, kEvent_HeartbeatFailed, senderInParam, senderOutParam);
        break;
    }

    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

/**
 * Handle the reception of a WRM acknowledgment for a heartbeat message that was sent reliably.
 */
void WeaveHeartbeatSender::HandleAckReceived(ExchangeContext *ec, void *msgCtxt)
{
    WeaveHeartbeatSender *sender = (WeaveHeartbeatSender *)ec->AppState;
    InEventParam inParam;
    OutEventParam outParam;

    VerifyOrDie(sender->mExchangeCtx == ec);

    sender->mExchangeCtx->Abort();
    sender->mExchangeCtx = NULL;

    inParam.Clear();
    inParam.Source = sender;

    outParam.Clear();

    sender->mEventCallback(sender->AppState, kEvent_HeartbeatSent, inParam, outParam);
}

/**
 * Handle a failure to transmit a heartbeat message that was sent reliably.
 */
void WeaveHeartbeatSender::HandleSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt)
{
    WeaveHeartbeatSender *sender = (WeaveHeartbeatSender *)ec->AppState;
    InEventParam inParam;
    OutEventParam outParam;

    VerifyOrDie(sender->mExchangeCtx == ec);

    sender->mExchangeCtx->Abort();
    sender->mExchangeCtx = NULL;

    inParam.Clear();
    inParam.Source = sender;
    inParam.HeartbeatFailed.Reason = err;

    outParam.Clear();

    sender->mEventCallback(sender->AppState, kEvent_HeartbeatFailed, inParam, outParam);
}

/**
 * @fn Binding *WeaveHeartbeatSender::GetBinding() const
 *
 * Get the binding object associated with heartbeat sender.
 */

/**
 * @fn bool WeaveHeartbeatSender::GetRequestAck() const
 *
 * Returns a flag indicating whether heartbeat messages will be sent reliably using Weave Reliable Messaging.
 */

/**
 * @fn void WeaveHeartbeatSender::SetRequestAck(bool val)
 *
 * Sets a flag indicating whether heartbeat messages should be sent reliably using Weave Reliable Messaging.
 *
 * Note that this flag is only meaningful when using UDP as a transport.
 *
 * @param[in] val               True if heartbeat messages should be sent reliably.
 */

/**
 * @fn uint8_t WeaveHeartbeatSender::GetSubscriptionState() const
 *
 * Get the current subscription state value.
 */

/**
 * @fn void WeaveHeartbeatSender::SetSubscriptionState(uint8_t val)
 *
 * Set the current subscription state.
 *
 * @param[in] subscriptionId    An 8-bit subscription state value to be conveyed with the heartbeat message.
 */

/**
 * @fn WeaveHeartbeatSender::EventCallback WeaveHeartbeatSender::GetEventCallback() const
 *
 * Returns the function that will be called to notify the application of events or changes that occur in the
 * WeaveHeartbeatSender.
 */

/**
 * @fn void WeaveHeartbeatSender::SetEventCallback(EventCallback eventCallback)
 *
 * Sets the function that will be called to notify the application of events or changes that occur in the
 * WeaveHeartbeatSender.
 */


} // namespace Heartbeat
} // namespace Profiles
} // namespace Weave
} // namespace nl

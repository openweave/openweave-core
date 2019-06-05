/*
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
 *      Definitions for the WeaveEchoClient object.
 *
 */

#ifndef WEAVE_ECHO_CLIENT_NEXT_H_
#define WEAVE_ECHO_CLIENT_NEXT_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveBinding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/echo/Next/WeaveEcho.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Echo_Next {

using namespace ::nl::Weave;

/**
 * @class WeaveEchoClient
 *
 * @brief
 *     Provides the ability to send Weave EchoRequest messages to a peer node and receive
 *     the corresponding EchoResponse messages.
 *
 * The WeaveEchoClient class implements the initiator side of the Weave Echo protocol. Similar
 * to the ICMP ping protocol, the Weave Echo protocol can be used to test the liveness and
 * reachability of a Weave node.
 *
 * Applications can use the WeaveEchoClient class to send one-off or repeating EchoRequest
 * messages to a peer node identified by a Binding object.  A corresponding class exists for
 * responding to echo requests (see WeaveEchoServer).
 *
 * ## Client Binding
 *
 * The WeaveEchoClient takes a Weave Binding object which is used to identify and establish
 * communication with the intended recipient of the echo requests.  The Binding can be
 * configured and prepared by the application prior to the initialization of the WeaveEchoClient
 * object, or it can be left unprepared, in which case the WeaveEchoClient will request on-demand
 * preparation of the binding (see Binding::RequestPrepare() for details).
 *
 * On-demand preparation of the Binding will also be requested should it fail after having
 * entered the Ready state.
 *
 * ## SendRepeating Mode
 *
 * The SendRepeating() method can be used to put the WeaveEchoClient into SendRepeating mode.
 * In this mode the client object sends a repeating sequence of EchoRequest messages to the peer
 * at a configured interval.  SendRepeating mode can be canceled by calling the Stop() method.
 *
 * ## Multicast and Broadcast
 *
 * A WeaveEchoClient object can be used to send EchoRequests to multiple recipients simultaneously
 * by configuring the Binding object with an appropriate IPv6 multicast address or IPv4 local
 * network broadcast address (255.255.255.255).  When the WeaveEchoClient object detects a multicast
 * or broadcast peer address, it automatically enters MultiResponse mode upon send of the EchoRequest.
 *
 * In this mode, the object continues to listen for and deliver all incoming EchoResponse messages
 * that arrive on the same exchange.  The object remains in MultiResponse mode until: 1) the
 * application calls Stop() or Send(), 2) in SendRepeating mode, the time comes to send another
 * request, or 3) no response is received and the receive timeout expires.
 *
 * ## API Events
 *
 * During the course of its operation, the WeaveEchoClient object will call up to the application
 * to request specific actions or deliver notifications of important events.  These API event
 * calls are made to the currently configured callback function on the client object.  Except where
 * noted, applications are free to alter the state of the client during an event callback.  One
 * overall exception is the object's Shutdown() method, which may never be called during a callback.
 *
 * The following API events are defined:
 *
 * ### PreparePayload
 *
 * The WeaveEchoClient is about to form an EchoRequest message and is requesting the application
 * to supply a payload.  If an application desires, it may return a new PacketBuffer containing
 * the payload data.  If the application does not handle this event, a EchoRequest with a zero-length
 * payload will be sent automatically.  *The application MAY NOT alter the state of the WeaveEchoClient
 * during this callback.*
 *
 * ### RequestSent
 *
 * An EchoRequest message was sent to the peer.
 *
 * ### ResponseReceived
 *
 * An EchoResponse message was received from the peer.  Arguments to the event contain the response
 * payload and meta-information about the response message.
 *
 * ### CommunicationError
 *
 * An error occurred while forming or sending an EchoRequest, or while waiting for a response.
 * Examples of errors that can occur while waiting for a response are key errors or unexpected close
 * of a connection.  Arguments to the event contain the error reason.
 *
 * ### ResponseTimeout
 *
 * An EchoResponse was not received in the alloted time. The response timeout is controlled by the
 * DefaultResponseTimeout property on the Binding object.
 *
 * ### RequestAborted
 *
 * An in-progress Echo exchange was aborted because a request was made to send another EchoRequest
 * before a response was received to the previous message.  This can arise in SendRepeating mode when
 * the time arrives to send the next EchoRequest.  This can also happen if the application calls
 * Send() after an EchoRequest has been sent but before any response is received.
 *
 * When the object is in MultiResponse mode, the event is suppressed if at least one EchoResponse
 * message has been received.
 */
class NL_DLL_EXPORT WeaveEchoClient
{
    // DOCUMENTATION FOR METHODS IS LOCATED IN THE CORRESPONDING SOURCE FILE

public:

    struct InEventParam;
    struct OutEventParam;

    enum State
    {
        kState_NotInitialized                       = 0,    /**< The client object is not initialized. */
        kState_Idle                                 = 1,    /**< The client object is idle. */
        kState_PreparingBinding                     = 2,    /**< The client object is waiting for the binding to become ready. */
        kState_RequestInProgress                    = 3,    /**< An EchoRequest message has been sent and the client object is awaiting a response. */
        kState_WaitingToSend                        = 4,    /**< SendRepeating() has been called and the client object is waiting for the
                                                                 next time to send an EchoRequest. */
    };

    enum EventType
    {
        kEvent_PreparePayload                       = 1,    /**< The application is requested to prepare the payload for the Echo request. */
        kEvent_RequestSent                          = 2,    /**< An EchoRequest message was sent to the peer. */
        kEvent_ResponseReceived                     = 3,    /**< An EchoResponse message was received from the peer. */
        kEvent_CommunicationError                   = 4,    /**< A communication error occurred while sending an EchoRequest or waiting for a response. */
        kEvent_ResponseTimeout                      = 5,    /**< An EchoResponse was not received in the alloted time. */
        kEvent_RequestAborted                       = 6,    /**< An in-progress Echo exchange was aborted because a request was made to start another exchange. */

        kEvent_DefaultCheck                         = 100,  /**< Used to verify correct default event handling in the application. */
    };

    typedef void (* EventCallback)(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

    void * AppState;                                        /**< A pointer to application-specific data. */

    WeaveEchoClient(void);

    WEAVE_ERROR Init(Binding * binding, EventCallback eventCallback, void * appState = NULL);
    void Shutdown(void);

    WEAVE_ERROR Send(void);
    WEAVE_ERROR Send(PacketBuffer * payloadBuf);
    WEAVE_ERROR SendRepeating(uint32_t sendIntervalMS);
    void Stop(void);

    State GetState(void) const;

    bool RequestInProgress() const;
    bool IsSendRrepeating() const;

    Binding * GetBinding(void) const;

    EventCallback GetEventCallback(void) const;
    void SetEventCallback(EventCallback eventCallback);

    static void DefaultEventHandler(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

private:

    enum Flags
    {
        kFlag_SendRepeating                         = 0x1,  /**< SendRepeating mode is enabled. */
        kFlag_MultiResponse                         = 0x2,  /**< EchoResponse messages may be received from multiple responders.  */
        kFlag_ResponseReceived                      = 0x4,  /**< An EchoResponse message has been received from the peer. */
        kFlag_AckReceived                           = 0x8,  /**< A WRM ACK has been received for the EchoRequest message. */
    };

    Binding * mBinding;
    EventCallback mEventCallback;
    uint32_t mSendIntervalMS;
    ExchangeContext * mEC;
    PacketBuffer * mQueuedPayload;
    State mState;
    uint8_t mFlags;

    WEAVE_ERROR DoSend(bool callbackOnError);
    bool IsRequestDone() const;
    void HandleRequestDone();
    void ClearRequestState();
    WEAVE_ERROR ArmSendTimer();
    void CancelSendTimer();
    void DeliverCommunicationError(WEAVE_ERROR err);

    bool GetFlag(uint8_t flag) const;
    void SetFlag(uint8_t flag);
    void SetFlag(uint8_t flag, const bool value);
    void ClearFlag(uint8_t flag);

    static void HandleBindingEvent(void * const appState, const Binding::EventType eventType, const Binding::InEventParam & inParam,
                                   Binding::OutEventParam & outParam);
    static void HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo,
                               uint32_t profileId, uint8_t msgType, PacketBuffer * payload);
    static void HandleResponseTimeout(ExchangeContext * ec);
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    static void HandleAckRcvd(ExchangeContext * ec, void * msgCtxt);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    static void HandleSendError(ExchangeContext * ec, WEAVE_ERROR sendErr, void * msgCtxt);
    static void HandleKeyError(ExchangeContext * ec, WEAVE_ERROR keyErr);
    static void HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleSendTimerExpired(System::Layer * systemLayer, void * appState, System::Error err);

    static bool IsMultiResponseAddress(const IPAddress & addr);

    WeaveEchoClient(const WeaveEchoClient&);   // not defined
};

/**
 * Input parameters to WeaveEchoClient API event.
 */
struct WeaveEchoClient::InEventParam
{
    WeaveEchoClient * Source;                               /**< The WeaveEchoClient from which the API event originated. */
    union
    {
        struct
        {
            WEAVE_ERROR Reason;                             /**< The error code associated with the communication failure. */
        } CommunicationError;

        struct
        {
            const WeaveMessageInfo * MsgInfo;               /**< An object containing meta-information about the received EchoResponse message. */
            PacketBuffer * Payload;                         /**< A PacketBuffer containing the payload of the received EchoResponse message. */
        } ResponseReceived;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to WeaveEchoClient API event.
 */
struct WeaveEchoClient::OutEventParam
{
    bool DefaultHandlerCalled;                              /**< Set to true by the DefaultEventHandler; should NOT be set by
                                                                 the application. */
    union
    {
        struct
        {
            PacketBuffer * Payload;                         /**< A PacketBuffer, allocated by the application and given to the
                                                                 WeaveEchoClient, containing the EchoRequest payload. */
            WEAVE_ERROR PrepareError;                       /**< An error set by the application indicating that a payload
                                                                 couldn't be prepared (e.g. WEAVE_ERROR_NO_MEMORY). */
        } PreparePayload;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};


/*
 * Inline Functions
 *
 * Documentation for these functions can be found at the end of the .cpp file.
 */

inline WeaveEchoClient::WeaveEchoClient(void)
{
}

inline WeaveEchoClient::State WeaveEchoClient::GetState(void) const
{
    return mState;
}

inline bool WeaveEchoClient::RequestInProgress() const
{
    return mState == kState_RequestInProgress;
}

inline bool WeaveEchoClient::IsSendRrepeating() const
{
    return GetFlag(kFlag_SendRepeating);
}

inline Binding *WeaveEchoClient::GetBinding(void) const
{
    return mBinding;
}

inline WeaveEchoClient::EventCallback WeaveEchoClient::GetEventCallback(void) const
{
    return mEventCallback;
}

inline void WeaveEchoClient::SetEventCallback(WeaveEchoClient::EventCallback eventCallback)
{
    mEventCallback = eventCallback;
}

inline bool WeaveEchoClient::GetFlag(uint8_t flag) const
{
    return ::nl::GetFlag(mFlags, flag);
}

inline void WeaveEchoClient::SetFlag(uint8_t flag)
{
    return ::nl::SetFlag(mFlags, flag);
}

inline void WeaveEchoClient::SetFlag(uint8_t flag, const bool value)
{
    return ::nl::SetFlag(mFlags, flag, value);
}

inline void WeaveEchoClient::ClearFlag(uint8_t flag)
{
    return ::nl::ClearFlag(mFlags, flag);
}

} // namespace Echo_Next
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_ECHO_CLIENT_NEXT_H_

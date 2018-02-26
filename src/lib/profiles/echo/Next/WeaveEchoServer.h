/*
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Definitions for the WeaveEchoServer object.
 *
 */

#ifndef WEAVE_ECHO_SERVER_NEXT_H_
#define WEAVE_ECHO_SERVER_NEXT_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/echo/Next/WeaveEcho.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Echo_Next {

/**
 * @class WeaveEchoServer
 *
 * @brief
 *     Accepts EchoRequest messages from a peer node and responds with an EchoResponse message.
 *
 * The WeaveEchoServer class implements the responder side of the Weave Echo protocol. Similar to
 * the ICMP ping protocol, the Weave Echo protocol can be used to test the liveness and reachability
 * of a Weave node.
 *
 * Applications can use the WeaveEchoServer class to enable automatic response to incoming
 * EchoRequest messages. A corresponding class exists for initiating echo requests (see
 * WeaveEchoClient).
 *
 * By default the WeaveEchoServer responds immediately to an EchoRequest with response containing
 * the same payload as the request.  However this behavior can be altered by the application during
 * processing of the EchoRequestReceived API event.
 *
 * ## API Events
 *
 * During the course of its operation, the WeaveEchoServer object will call up to the application
 * to request specific actions or deliver notifications of important events.  These API event
 * calls are made to the currently configured callback function on the server object.  Except where
 * noted, applications are free to alter the state of the server during an event callback.  One
 * overall exception is the object's Shutdown() method, which may never be called during a callback.
 *
 * The following API events are defined:
 *
 * ### EchoRequestReceived
 *
 * An EchoRequest message was received from a peer.  Arguments to the event contain the request
 * payload, the exchange context over which the message was received and meta-information about
 * the request message.
 *
 * If the application chooses, it may alter the output arguments to the event to force a delay
 * in responding or to suppress the response altogether.  Additionally, it may alter the contents
 * of the payload buffer, which will become the payload for the response message.
 *
 * ### EchoResponseSent
 *
 * An EchoResponse message was sent, or failed to be sent.  Arguments to the event contain the
 * error that resulted from sending the message (if any) and the exchange context over which the
 * message was sent.
 */
class NL_DLL_EXPORT WeaveEchoServer : public WeaveServerBase
{
public:

    struct InEventParam;
    struct OutEventParam;

    enum EventType
    {
        kEvent_EchoRequestReceived                  = 1,    //< An EchoRequest message was received from a peer.
        kEvent_EchoResponseSent                     = 2,    //< An EchoResponse message was sent, or failed to be sent.

        kEvent_DefaultCheck                         = 100,  //< Used to verify correct default event handling in the application.
    };

    typedef void (* EventCallback)(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

    void * AppState;                                        //< A pointer to application-specific data.

    WeaveEchoServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager * exchangeMgr, EventCallback eventCallback, void * appState = NULL);
    WEAVE_ERROR Shutdown(void);

    EventCallback GetEventCallback(void) const;
    void SetEventCallback(EventCallback eventCallback);

    static void DefaultEventHandler(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

    // DEPRECATED APIs -- These APIs provide backwards compatibility with previous versions of WeaveEchoServer.
    // DO NOT USE IN NEW CODE.
    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    typedef void (*EchoFunct)(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
    EchoFunct OnEchoRequestReceived;

private:
    EventCallback mEventCallback;

    static void HandleEchoRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void SendEchoResponse(System::Layer * systemLayer, void * appState, System::Error err);

    WeaveEchoServer(const WeaveEchoServer&);   // not defined
};

/**
 * Input parameters to WeaveEchoServer API event.
 */
struct WeaveEchoServer::InEventParam
{
    WeaveEchoServer * Source;                               //< The WeaveEchoServer that is the source of the API event.
    union
    {
        struct
        {
            const WeaveMessageInfo *MessageInfo;            //< Information about the received Echo Request message.
            ExchangeContext *EC;                            //< The exchange context over which the Echo Request message was received.
            PacketBuffer *Payload;                          //< A buffer containing the payload of the Echo Request message.
        } EchoRequestReceived;

        struct
        {
            ExchangeContext *EC;                            //< The exchange context over which the Echo Response was sent.
            WEAVE_ERROR Error;                              //< The error code returned when the Echo Response was sent.
        } EchoResponseSent;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to WeaveEchoServer API event.
 */
struct WeaveEchoServer::OutEventParam
{
    bool DefaultHandlerCalled;
    union
    {
        struct
        {
            uint32_t ResponseDelay;                         //< The amount of time (in milliseconds) to delay sending the response.  Defaults to 0.
            bool SuppressResponse;                          //< If true, suppress sending a response.  Defaults to false.
        } EchoRequestReceived;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};


/*
 * Inline Functions
 *
 * Documentation for these functions can be found at the end of .cpp file.
 */

inline WeaveEchoServer::EventCallback WeaveEchoServer::GetEventCallback(void) const
{
    return mEventCallback;
}

inline void WeaveEchoServer::SetEventCallback(WeaveEchoServer::EventCallback eventCallback)
{
    mEventCallback = eventCallback;
}


} // namespace Echo_Next
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_ECHO_SERVER_NEXT_H_

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
 *      This file defines Weave Heartbeat Profile.
 *
 */

#ifndef WEAVE_HEARTBEAT_H_
#define WEAVE_HEARTBEAT_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <SystemLayer/SystemTimer.h>

/**
 *   @namespace nl::Weave::Profiles::Heartbeat
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Heartbeat profile.
 *
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Heartbeat {

using ::nl::Inet::IPAddress;
using ::nl::Weave::System::PacketBuffer;

/**
 * Weave Heartbeat Message Types
 */
enum
{
    kHeartbeatMessageType_Heartbeat             = 1,
};


enum
{
    kHeartbeatMessageLength                     = 1
};


/**
 * Weave Heartbeat Sender class
 */
class NL_DLL_EXPORT WeaveHeartbeatSender
{
public:
    struct InEventParam;
    struct OutEventParam;

    enum EventType
    {
        kEvent_UpdateSubscriptionState          = 1,    //< The application is requested to update the subscription state.

        kEvent_HeartbeatSent                    = 2,    //< A heartbeat message was successfully sent to the peer. If reliable transmission
                                                        //  is enabled, this event indicates that the message was acknowledged.
        kEvent_HeartbeatFailed                  = 3,    //< A heartbeat message failed to be sent to the peer.

        kEvent_DefaultCheck                     = 100,  //< Used to verify correct default event handling in the application.
                                                        //  Applications should NOT expressly handle this event.
    };

    typedef void (*EventCallback)(void *appState, EventType eventType, const InEventParam& inParam, OutEventParam& outParam);

    void *AppState;

    WeaveHeartbeatSender(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr, Binding *binding, EventCallback eventCallback, void *appState);
    WEAVE_ERROR Shutdown(void);

    WEAVE_ERROR StartHeartbeat(void);
    WEAVE_ERROR StopHeartbeat(void);
    WEAVE_ERROR ScheduleHeartbeat(void);
    WEAVE_ERROR SendHeartbeatNow(void);

    uint8_t GetSubscriptionState() const;
    void SetSubscriptionState(uint8_t val);

    bool GetRequestAck() const;
    void SetRequestAck(bool val);

    void GetConfiguration(uint32_t& interval, uint32_t& phase, uint32_t& window) const;
    void SetConfiguration(uint32_t interval, uint32_t phase, uint32_t window);

    Binding *GetBinding() const;

    EventCallback GetEventCallback() const;
    void SetEventCallback(EventCallback eventCallback);

    static void DefaultEventHandler(void *appState, EventType eventType, const InEventParam& inParam, OutEventParam& outParam);

private:

    void SendHeartbeat(bool scheduleNextHeartbeat);
    uint32_t GetRandomInterval(uint32_t minVal, uint32_t maxVal);
    uint64_t GetHeartbeatBase(void);
    static void HandleHeartbeatTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static uint64_t GetPlatformTimeMs(void);
    static void BindingEventCallback(void *appState, Binding::EventType eventType, const Binding::InEventParam& inParam, Binding::OutEventParam& outParam);
    static void HandleAckReceived(ExchangeContext *ec, void *msgCtxt);
    static void HandleSendError(ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt);
    WeaveHeartbeatSender(const WeaveHeartbeatSender&); // Not defined.

    uint64_t                    mHeartbeatBase;
    const WeaveFabricState *    mFabricState;
    WeaveExchangeManager *      mExchangeMgr;
    Binding *                   mBinding;
    ExchangeContext *           mExchangeCtx;
    EventCallback               mEventCallback;
    uint32_t                    mHeartbeatInterval_msec;
    uint32_t                    mHeartbeatPhase_msec;
    uint32_t                    mHeartbeatWindow_msec;
    uint8_t                     mSubscriptionState;
    bool                        mRequestAck;
};

/**
 * Input parameters to WeaveHeartbeatSender API event.
 */
struct WeaveHeartbeatSender::InEventParam
{
    WeaveHeartbeatSender *Source;
    union
    {
        struct
        {
            WEAVE_ERROR Reason;                         //< An error describing why the heartbeat message couldn't be sent.
        } HeartbeatFailed;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to WeaveHeartbeatSender API event.
 */
struct WeaveHeartbeatSender::OutEventParam
{
    bool DefaultHandlerCalled;

    void Clear() { memset(this, 0, sizeof(*this)); }
};



/**
 * Weave Heartbeat Receiver class
 */
class NL_DLL_EXPORT WeaveHeartbeatReceiver : public WeaveServerBase
{
public:
    WeaveHeartbeatReceiver(void);
    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    typedef void (*OnHeartbeatReceivedHandler)(const WeaveMessageInfo *aMsgInfo, uint8_t nodeState, WEAVE_ERROR err);
    OnHeartbeatReceivedHandler OnHeartbeatReceived;

private:
    static void HandleHeartbeat(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    WeaveHeartbeatReceiver(const WeaveHeartbeatReceiver&);
};


/*
 * Inline Functions
 *
 * Documentation for these functions can be found in the .cpp file.
 */

inline Binding *WeaveHeartbeatSender::GetBinding() const
{
    return mBinding;
}

inline bool WeaveHeartbeatSender::GetRequestAck() const
{
    return mRequestAck;
}

inline void WeaveHeartbeatSender::SetRequestAck(bool val)
{
    mRequestAck = val;
}

inline uint8_t WeaveHeartbeatSender::GetSubscriptionState() const
{
    return mSubscriptionState;
}

inline void WeaveHeartbeatSender::SetSubscriptionState(uint8_t val)
{
    mSubscriptionState = val;
}

inline WeaveHeartbeatSender::EventCallback WeaveHeartbeatSender::GetEventCallback() const
{
    return mEventCallback;
}

inline void WeaveHeartbeatSender::SetEventCallback(EventCallback eventCallback)
{
    mEventCallback = eventCallback;
}


} // namespace Heartbeat
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif // WEAVE_HEARTBEAT_H_

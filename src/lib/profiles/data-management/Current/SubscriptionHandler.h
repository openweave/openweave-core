/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file defines subscription handler for Weave
 *      Data Management (WDM) profile.
 *
 */
#ifndef _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_HANDLER_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_HANDLER_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Profiles/data-management/EventLogging.h>
#include <Weave/Profiles/data-management/LoggingManagement.h>
#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

#include <Weave/Profiles/status-report/StatusReportProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class SubscriptionHandler
{
public:
    typedef uint8_t HandlerId;

    enum
    {
        // Note the WDM spec says 0x7FFFFFFF, but Weave implementation can only hold timeout of much shorter
        // 32-bit in milliseconds, which is about 1200 hours
        kMaxTimeoutSec = 3600000,

        kNoTimeout = 0,
    };

    struct TraitInstanceInfo
    {
        void Init(void) { this->ClearDirty(); }
        bool IsDirty(void) { return mDirty; }
        void SetDirty(void) { mDirty = true; }
        void ClearDirty(void) { mDirty = false; }

        TraitDataHandle mTraitDataHandle;
        uint16_t mRequestedVersion;
        bool mDirty;
    };

    enum EventID
    {
        kEvent_OnSubscribeRequestParsed = 0,

        // Last chance to adjust EC, mEC is valid and can be tuned for timeout settings
        // Don't change anything on the handler and don't close the EC
        kEvent_OnExchangeStart = 1,

        kEvent_OnSubscriptionEstablished = 2,

        kEvent_OnSubscriptionTerminated = 3,
    };

    union InEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

        struct
        {
            TraitInstanceInfo * mTraitInstanceList;
            uint16_t mNumTraitInstances;

            bool mSubscribeToAllEvents;

            nl::Weave::ExchangeContext * mEC;
            const nl::Inet::IPPacketInfo * mPktInfo; //< A pointer to the packet information of the request
            const nl::Weave::WeaveMessageInfo *
                mMsgInfo; //< A pointer to a WeaveMessageInfo structure containing information about the Subscribe Request message.

            uint32_t mTimeoutSecMin;
            uint32_t mTimeoutSecMax;

            bool mIsSubscriptionIdValid;
            uint64_t mSubscriptionId;

            event_id_t mNextVendedEvents[kImportanceType_Last - kImportanceType_First + 1];

            SubscriptionHandler * mHandler;
        } mSubscribeRequestParsed;

        struct
        {
            // Do not close the EC
            nl::Weave::ExchangeContext * mEC;
            SubscriptionHandler * mHandler;
        } mExchangeStart;

        struct
        {
            uint64_t mSubscriptionId;
            SubscriptionHandler * mHandler;
        } mSubscriptionEstablished;

        struct
        {
            SubscriptionHandler * mHandler;
            WEAVE_ERROR mReason;

            uint32_t mStatusProfileId;
            uint16_t mStatusCode;
            ReferencedTLVData * mAdditionalInfoPtr;
        } mSubscriptionTerminated;
    };

    union OutEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }
    };

    typedef void (*EventCallback)(void * const aAppState, EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    static void DefaultEventHandler(EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    uint64_t GetPeerNodeId(void) const;

    Binding * GetBinding(void) const;

    WEAVE_ERROR GetSubscriptionId(uint64_t * const apSubscriptionId);

    WEAVE_ERROR AcceptSubscribeRequest(const uint32_t aLivenessTimeoutSec = kNoTimeout);

    /**
     * @brief This function initiates a graceful shutdown of the subscription and clean-up of the handler object. This is an
     * asynchronous call and will notify a client of the impending shutdown through a SubscribeCancel/StatusReport message where
     * relevant.
     *
     * Notably, this relinquishes the application's involvement in this subscription. After this call, the application will not be
     * notified of any further activity on this object. Additionally, the application is not allowed to interact with this object
     * thereafter through any of its methods.
     *
     * @param[in] aReasonProfileId      ProfileId of the StatusCode that indicates the reason behind the termination
     * @param[in] aReasonStatusCode     StatusCode that indicates the reason behind the termination
     *
     * @retval Returns a Weave error code for informational purposes only. On any error, the object will be terminated synchronously
     * (i.e aborted).
     *
     */
    WEAVE_ERROR EndSubscription(const uint32_t aReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_Common,
                                const uint16_t aReasonStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest);

    /**
     * @brief This function terminates a subscription immediately - this is a synchronous call. No attempt is made to notify the
     * client of the termination, and the underlying exchange context if present is aborted immediately. After this call, the
     * application will not be notified of any further activity on this object. Additionally, the application is not allowed to
     * interact with this object thereafter through any of its methods.
     */
    void AbortSubscription(void);

    bool IsEstablishedIdle() { return (mCurrentState == kState_SubscriptionEstablished_Idle); }
    bool IsActive(void)
    {
        return (mCurrentState >= kState_Subscribing_Evaluating && mCurrentState <= kState_SubscriptionEstablished_Notifying);
    }
    bool IsAborted() { return (mCurrentState == kState_Aborted); }
    bool IsFree() { return (mCurrentState == kState_Free); }

    uint32_t GetMaxNotificationSize(void) const { return mMaxNotificationSize == 0 ? UINT16_MAX : mMaxNotificationSize; }

    void SetMaxNotificationSize(const uint32_t aMaxPayload);

private:
    friend class SubscriptionEngine;
    friend class NotificationEngine;
    friend class TestSubscriptionHandler;
    friend class TestTdm;
    friend class TestWdm;

    struct LastVendedEvent
    {
        uint64_t mSourceId;
        uint8_t mImportance;
        uint64_t mEventId;
    };

    enum HandlerState
    {
        kState_Free                              = 0,
        kState_Subscribing_Evaluating            = 1,
        kState_Subscribing                       = 2,
        kState_Subscribing_Notifying             = 3,
        kState_Subscribing_Responding            = 4,
        kState_SubscriptionEstablished_Idle      = 5,
        kState_SubscriptionEstablished_Notifying = 6,
        kState_Canceling                         = 7,

        kState_SubscriptionInfoValid_Begin = kState_Subscribing,
        kState_SubscriptionInfoValid_End   = kState_Canceling,

        kState_Aborting = 9,
        kState_Aborted  = 10,
    };

    HandlerState mCurrentState;

    bool IsNotifiable(void)
    {
        return (mCurrentState == kState_Subscribing || mCurrentState == kState_SubscriptionEstablished_Idle);
    }
    bool IsSubscribing(void)
    {
        return (mCurrentState >= kState_Subscribing_Evaluating && mCurrentState <= kState_Subscribing_Responding);
    }
    bool IsNotifying(void)
    {
        return (mCurrentState == kState_Subscribing_Notifying || mCurrentState == kState_SubscriptionEstablished_Notifying);
    }

    // initialize once at boot up
    void * mAppState;
    EventCallback mEventCallback;

    // initialize at incoming subscribe request
    bool mIsInitiator;
    int8_t mRefCount;
    nl::Weave::ExchangeContext * mEC;
    uint32_t mLivenessTimeoutMsec;
    uint64_t mPeerNodeId;
    uint64_t mSubscriptionId;
    Binding * mBinding;

    TraitInstanceInfo * mTraitInstanceList;
    uint16_t mNumTraitInstances;
    uint16_t mMaxNotificationSize;
    uint32_t mCurProcessingTraitInstanceIdx;

    TraitInstanceInfo * GetTraitInstanceInfoList(void) { return mTraitInstanceList; }
    uint32_t GetNumTraitInstances(void) { return mNumTraitInstances; }

    void OnNotifyProcessingComplete(const bool aPossibleLossOfEvent, const LastVendedEvent aLastVendedEventList[],
                                    const size_t aLastVendedEventListSize);

    bool mSubscribeToAllEvents;
    // TODO: WEAV-1426 in this incarnation, we do not account for event aggregation.
    event_id_t mSelfVendedEvents[kImportanceType_Last - kImportanceType_First + 1];
    event_id_t mLastScheduledEventId[kImportanceType_Last - kImportanceType_First + 1];
    ImportanceType mCurrentImportance;

    // The counter here tracks the number of event bytes offloaded to
    // the subscriber.  The variable is updated by the
    // NotificationEngine during building the event list and it is
    // read by LoggingManagement to determine if the we had
    // accumulated enough bytes in events to trigger an event offload
    // by triggering the NotificationEngine.
    size_t mBytesOffloaded;

    // Do nothing
    SubscriptionHandler(void);

    void _AddRef(void);
    void _Release(void);
    void HandleSubscriptionTerminated(WEAVE_ERROR aReason, nl::Weave::Profiles::StatusReporting::StatusReport * aStatusReportPtr);
    void MoveToState(const HandlerState aTargetState);
    const char * GetStateStr() const;

    WEAVE_ERROR ReplaceExchangeContext(void);
    void InitExchangeContext(void);
    void FlushExistingExchangeContext(const bool aAbortNow = false);

    void InitWithIncomingRequest(Binding * const aBinding, const uint64_t aRandomNumber, nl::Weave::ExchangeContext * aEC,
                                 const nl::Inet::IPPacketInfo * aPktInfo, const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                 PacketBuffer * aPayload);

    WEAVE_ERROR SendNotificationRequest(PacketBuffer * aMsgBuf);
    WEAVE_ERROR SendSubscribeResponse(const bool aPossibleLossOfEvent, const LastVendedEvent aLastVendedEventList[],
                                      const size_t aLastVendedEventListSize);

    void TimerEventHandler(void);
    WEAVE_ERROR RefreshTimer(void);

    bool CheckEventUpToDate(LoggingManagement & logger);
    ImportanceType FindNextImportanceForTransfer(void);
    WEAVE_ERROR SetEventLogEndpoint(LoggingManagement & logger);

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
    WEAVE_ERROR Cancel(void);
    void CancelRequestHandler(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                              const nl::Weave::WeaveMessageInfo * aMsgInfo, PacketBuffer * aPayload);
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

    void InitAsFree(void);

    WEAVE_ERROR ParsePathVersionEventLists(SubscribeRequest::Parser & aRequest, uint32_t & aRejectReasonProfileId,
                                           uint16_t & aRejectReasonStatusCode);

    inline WEAVE_ERROR ParseSubscriptionId(SubscribeRequest::Parser & aRequest, uint32_t & aRejectReasonProfileId,
                                           uint16_t & aRejectReasonStatusCode, const uint64_t aRandomNumber);

    static void OnTimerCallback(System::Layer * aSystemLayer, void * aAppState, System::Error aErrorCode);
    static void OnAckReceived(ExchangeContext * aEC, void * aMsgSpecificContext);
    static void OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext);
    static void OnResponseTimeout(nl::Weave::ExchangeContext * aEC);
    static void OnMessageReceivedFromLocallyHeldExchange(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                                         const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId,
                                                         uint8_t aMsgType, PacketBuffer * aPayload);
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_HANDLER_CURRENT_H

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
 *      This file defines subscription engine for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_ENGINE_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_ENGINE_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>

#include <Weave/Profiles/data-management/SubscriptionClient.h>
#include <Weave/Profiles/data-management/SubscriptionHandler.h>
#include <Weave/Profiles/data-management/NotificationEngine.h>
#include <Weave/Profiles/data-management/Command.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * @class IWeavePublisherLock
 *
 * @brief Interface that is to be implemented by app to serialize access to key WDM data structures.
 *        This should be backed by a recursive lock implementation.
 */
class IWeavePublisherLock
{
public:
    virtual WEAVE_ERROR Lock(void)   = 0;
    virtual WEAVE_ERROR Unlock(void) = 0;
};

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
class IUpdateRequestDataElementAccessControlDelegate
{
public:
    virtual WEAVE_ERROR DataElementAccessCheck(const TraitPath & aTraitPath, const TraitCatalogBase<TraitDataSource> & aCatalog) = 0;
};
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

/**
 * @class SubscriptionEngine
 *
 * @brief This is a singleton hosting all WDM Next subscriptions, both client and publisher sides.
 *
 */
class SubscriptionEngine
{
public:
    /**
     * @brief Retrieve the singleton Subscription Engine. Note this function should be implemented by the
     *  adoption layer.
     *
     *  @return  A pointer to the shared Subscription Engine
     *
     */
    static SubscriptionEngine * GetInstance(void);

    /**
     * Events generated directly from this component
     *
     */
    enum EventID
    {
#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        kEvent_OnIncomingSubscribeRequest =
            0, ///< Called when an incoming subscribe request has arrived, before any parsing is done.
#endif         // #if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        kEvent_OnIncomingSubscriptionlessNotification =
            1, ///< Called when an incoming subscriptionless notification has arrived before updating the data element.
        kEvent_DataElementAccessControlCheck =
            2, ///< Called when an incoming subscriptionless notification is being processed for access control of each data element.
        kEvent_SubscriptionlessNotificationProcessingComplete =
            3, ///< Called upon completion of processing of all trait data in the subscriptionless notify.
#endif         // #if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        kEvent_OnIncomingUpdateRequest =
            4, ///< Called when an incoming update has arrived before updating the data element.
        kEvent_UpdateRequestDataElementAccessControlCheck =
            5, ///< Called when an incoming update is being processed for access control of each data element.
        kEvent_UpdateRequestProcessingComplete =
            6, ///< Called upon completion of processing of all trait data in the update.
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    };

    /**
     * Incoming parameters sent with events generated directly from this component
     *
     */
    union InEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        /**
         * @brief Incoming parameters for kEvent_OnIncomingSubscribeRequest
         */
        struct
        {
            nl::Weave::ExchangeContext * mEC;             ///< A pointer to the exchange context object this request comes from
            PacketBuffer * mPayload;                      ///< A pointer to the packet buffer containing the request
            const nl::Inet::IPPacketInfo * mPktInfo;      ///< A pointer to the packet information of the request
            const nl::Weave::WeaveMessageInfo * mMsgInfo; ///< A pointer to the message information for the request
            Binding * mBinding; ///< A pointer to the Binding object created based on the exchange context object
        } mIncomingSubscribeRequest;
#endif // #if WDM_ENABLE_SUBSCRIPTION_PUBLISHER

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        struct
        {
            WEAVE_ERROR processingError;                  ///< The WEAVE_ERROR encountered in processing the
                                                          ///< subscriptionless notification.
            const nl::Weave::WeaveMessageInfo * mMsgInfo; ///< A pointer to the message information for the request
        } mIncomingSubscriptionlessNotification;

        struct
        {
            const TraitCatalogBase<TraitDataSink> * mCatalog; ///< A pointer to the TraitCatalog for the data sinks.
            const TraitPath *mPath;                           ///< A pointer to the TraitPath being accessed by the
                                                          ///< subscriptionless notification.
            const nl::Weave::WeaveMessageInfo * mMsgInfo; ///< A pointer to the message information for the request
        } mDataElementAccessControlForNotification;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        struct
        {
            WEAVE_ERROR processingError;                  ///< The WEAVE_ERROR encountered in processing the
                                                          ///< Update Request.
            const nl::Weave::WeaveMessageInfo * mMsgInfo; ///< A pointer to the message information for the request
        } mIncomingUpdateRequest;

        struct
        {
            const TraitCatalogBase<TraitDataSource> * mCatalog; ///< A pointer to the TraitCatalog for the data sources.
            const TraitPath *mPath;                             ///< A pointer to the TraitPath being accessed by the
                                                                ///< update request.
            const nl::Weave::WeaveMessageInfo * mMsgInfo; ///< A pointer to the message information for the request
        } mDataElementAccessControlForUpdateRequest;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    };

    /**
     * Outgoing parameters sent with events generated directly from this component
     *
     */
    union OutEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }
#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        /**
         * @brief Outgoing parameters for kEvent_OnIncomingSubscribeRequest
         */
        struct
        {
            bool mAutoClosePriorSubscription; ///< Set to true if subscription engine must close existing subscription with the same
                                              ///< peer node id
            bool mRejectRequest; ///< Set to true if subscription engine must reject this request with the reason and status code
            uint32_t * mpReasonProfileId;  ///< A pointer to the profile ID of reason for rejection
            uint16_t * mpReasonStatusCode; ///< A pointer to the status code of reason for rejection

            void * mHandlerAppState;                                  ///< A pointer to application layer supplied state object
            SubscriptionHandler::EventCallback mHandlerEventCallback; ///< A function pointer for event call back
        } mIncomingSubscribeRequest;
#endif // #if WDM_ENABLE_SUBSCRIPTION_PUBLISHER

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
        struct
        {
            bool mShouldContinueProcessing;      ///< Set to true if subscriptionless notification is allowed.
        } mIncomingSubscriptionlessNotification;

        struct
        {
            bool mRejectNotification;      ///< Set to true if subscriptionless notification is rejected.
            WEAVE_ERROR mReason;           ///< The reason for the rejection, if any.
        } mDataElementAccessControlForNotification;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
        struct
        {
            bool mShouldContinueProcessing;      ///< Set to true if update is allowed.
        } mIncomingUpdateRequest;

        struct
        {
            bool mRejectUpdateRequest;      ///< Set to true if update is rejected.
            WEAVE_ERROR mReason;     ///< The reason for the rejection, if any.
        } mDataElementAccessControlForUpdateRequest;
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    };

    /**
     * @brief Set the event back function and pointer to associated state object for SubscriptionEngine specific call backs
     *
     * @param[in]  aAppState    A pointer to application layer supplied state object
     * @param[in]  aEvent       A function pointer for event call back
     * @param[in]  aInParam     A const reference to the input parameter for this event
     * @param[out] aOutParam    A reference to the output parameter for this event
     */
    typedef void (*EventCallback)(void * const aAppState, EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    /**
     * @brief Set the event back function and pointer to associated state object for SubscriptionEngine specific call backs
     *
     * @param[in]  aAppState  		A pointer to application layer supplied state object
     * @param[in]  aEventCallback  	A function pointer for event call back
     */
    void SetEventCallback(void * const aAppState, const EventCallback aEventCallback);

    /**
     * @brief This is the default event handler to be called by application layer for any ignored or unrecognized event
     *
     * @param[in]  aEvent       A function pointer for event call back
     * @param[in]  aInParam     A const reference to the input parameter for this event
     * @param[out] aOutParam    A reference to the output parameter for this event
     */
    static void DefaultEventHandler(EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    /**
     * @brief Retrieve the minimum relative position of the event offload point from all active subscription handlers.
     *
     * Retrieves the minimum relative (to the boottime) position of
     * the event offload point from all active subscription handlers.
     *
     * @param[inout] outLogPosition Minimum log offload point for all
     *                              active subscription handlers.  If
     *                              no subscription handlers are
     *                              active, the value remains
     *                              unchanged.  The log position is
     *                              set to 0 upon initializing the
     *                              subscription handler.
     *
     * @retval #WEAVE_NO_ERROR      unconditionally
     */
    WEAVE_ERROR GetMinEventLogPosition(size_t & outLogPosition) const;

#if WDM_ENABLE_SUBSCRIPTION_CLIENT
    enum
    {
        kMaxNumSubscriptionClients =
            (WDM_MAX_NUM_SUBSCRIPTION_CLIENTS), ///< Max number of subscription clients this engine can accommodate
    };

    /**
     * @brief This is the default event handler to be called by application layer for any ignored or unrecognized event
     *
     * @param[in]  appClient        A pointer to pointer for the new subscription client object
     * @param[in]  apBinding        A pointer to Binding to be used for this subscription client
     * @param[in]  apAppState       A pointer to application layer supplied state object
     * @param[in]  aEventCallback   A function pointer for event call back
     * @param[in]  apCatalog        A pointer to data sink catalog object
     * @param[in]  aTimeoutMsecBeforeSubscribeResponse    Max number of milliseconds before subscribe
     *                                                    response must be received after subscribe request is sent
     */
    WEAVE_ERROR NewClient(SubscriptionClient ** const appClient, Binding * const apBinding, void * const apAppState,
                          SubscriptionClient::EventCallback const aEventCallback,
                          const TraitCatalogBase<TraitDataSink> * const apCatalog,
                          const uint32_t aInactivityTimeoutDuringSubscribingMsec);

    /**
     * @brief This is the default event handler to be called by application layer for any ignored or unrecognized event
     *
     * @param[in]  appClient        A pointer to pointer for the new subscription client object
     * @param[in]  apBinding        A pointer to Binding to be used for this subscription client
     * @param[in]  apAppState       A pointer to application layer supplied state object
     * @param[in]  aEventCallback   A function pointer for event call back
     * @param[in]  apCatalog        A pointer to data sink catalog object
     * @param[in]  aTimeoutMsecBeforeSubscribeResponse    Max number of milliseconds before subscribe
     *                                                    response must be received after subscribe request is sent
     * @param[in]  aUpdateMutex     A mutex to protect the internal data structures used in WDM updates; NULL by default,
     *                              it must be provided if the application will call WDM Update methods from multiple threads.
     */
    WEAVE_ERROR NewClient(SubscriptionClient ** const appClient, Binding * const apBinding, void * const apAppState,
                          SubscriptionClient::EventCallback const aEventCallback,
                          const TraitCatalogBase<TraitDataSink> * const apCatalog,
                          const uint32_t aInactivityTimeoutDuringSubscribingMsec,
                          IWeaveWDMMutex * aUpdateMutex);

    WEAVE_ERROR NewSubscriptionHandler(SubscriptionHandler ** const subHandler);

    uint16_t GetClientId(const SubscriptionClient * const apClient) const;

    SubscriptionClient * FindClient(const uint64_t aPeerNodeId, const uint64_t aSubscriptionId);

    bool UpdateClientLiveness(const uint64_t aPeerNodeId, const uint64_t aSubscriptionId, const bool aKill = false);

#endif // WDM_ENABLE_SUBSCRIPTION_CLIENT

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
    WEAVE_ERROR EnablePublisher(IWeavePublisherLock * aLock, TraitCatalogBase<TraitDataSource> * const aPublisherCatalog);

    // The lock methods here guard access to a couple of data structures:
    //      - mPublisherCatalog
    //      - mHandlers
    //      - mNotificationEngine
    //      - mTraintInfoPool
    //      - mNumTraitInfosPool
    //
    // The implementation is not complete in ensuring all of the above structures are guarded.
    WEAVE_ERROR Lock(void);
    WEAVE_ERROR Unlock(void);

    NotificationEngine * GetNotificationEngine(void) { return &mNotificationEngine; }

    // After this call returns, it's free to tear down the current publisher catalog
    void DisablePublisher(void);

    SubscriptionHandler * FindHandler(const uint64_t aPeerNodeId, const uint64_t aSubscriptionId);

    bool UpdateHandlerLiveness(const uint64_t aPeerNodeId, const uint64_t aSubscriptionId, const bool aKill = false);

    uint16_t GetHandlerId(const SubscriptionHandler * const apHandler) const;

    uint16_t GetCommandObjId(const Command * const apHandle) const;

#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER

    SubscriptionEngine(void);

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager * const apExchangeMgr, void * const aAppState = NULL,
                     const EventCallback aEventCallback = NULL);

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    WEAVE_ERROR RegisterForSubscriptionlessNotifications(
                     const TraitCatalogBase<TraitDataSink> * const apCatalog);
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    nl::Weave::WeaveExchangeManager * GetExchangeManager(void) const { return mExchangeMgr; };

private:
    friend class SubscriptionHandler;
    friend class SubscriptionClient;
    friend class NotificationEngine;
    friend class TestTdm;
    friend class TestWdm;

    nl::Weave::WeaveExchangeManager * mExchangeMgr;
    void * mAppState;
    EventCallback mEventCallback;

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    const TraitCatalogBase<TraitDataSink> * mSubscriptionlessNotifySinkCatalog;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    static WEAVE_ERROR SendStatusReport(nl::Weave::ExchangeContext * aEC, uint32_t aProfileId, uint16_t aStatusCode);

    static void UnsolicitedMessageHandler(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                          const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                          PacketBuffer * aPayload);

    static void OnUnknownMsgType(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                 const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                 PacketBuffer * aPayload);

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
    static void OnCancelRequest(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                PacketBuffer * aPayload);
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    class SubscriptionlessNotifyDataElementAccessControlDelegate : public IDataElementAccessControlDelegate
    {
    public:
         SubscriptionlessNotifyDataElementAccessControlDelegate(const WeaveMessageInfo * aMsgInfo)
         {
             mMsgInfo = aMsgInfo;
         }

         WEAVE_ERROR DataElementAccessCheck(const TraitPath & aTraitPath,
                                            const TraitCatalogBase<TraitDataSink> & aCatalog);

    private:
         const WeaveMessageInfo * mMsgInfo;
    };

    static void OnSubscriptionlessNotification(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                               const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                               PacketBuffer * aPayload);
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    class UpdateRequestDataElementAccessControlDelegate : public IUpdateRequestDataElementAccessControlDelegate
        {
        public:
             UpdateRequestDataElementAccessControlDelegate(const WeaveMessageInfo * aMsgInfo)
             {
                 mMsgInfo = aMsgInfo;
             }

             WEAVE_ERROR DataElementAccessCheck(const TraitPath & aTraitPath,
                                                const TraitCatalogBase<TraitDataSource> & aCatalog);

        private:
             const WeaveMessageInfo * mMsgInfo;
        };

    static void OnUpdateRequest(nl::Weave::ExchangeContext * apEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                               const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                               PacketBuffer * aPayload);
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

#if WDM_ENABLE_SUBSCRIPTION_CLIENT
    // Client-specific features

    SubscriptionClient mClients[kMaxNumSubscriptionClients];

    static void OnNotificationRequest(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                      const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                      PacketBuffer * aPayload);

#endif // WDM_ENABLE_SUBSCRIPTION_CLIENT

    static WEAVE_ERROR ProcessDataList(nl::Weave::TLV::TLVReader & aReader,
                                       const TraitCatalogBase<TraitDataSink> * aCatalog,
                                       bool & aOutIsPartialChange,
                                       TraitDataHandle & aOutTraitDataHandle,
                                       IDataElementAccessControlDelegate & acDelegate);

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    struct StatusDataHandleElement
    {
        uint32_t        mProfileId;
        uint16_t        mStatusCode;
        TraitDataHandle mTraitDataHandle;
    };

    struct UpdateResponseWriterContext
    {
        void * mpFirstStatusDataHandleElement;
        const TraitCatalogBase<TraitDataSource> * mpCatalog;
        uint32_t mNumDataElements;
    };

    static WEAVE_ERROR GetPreviousFirstSameTraitVersion(TLV::TLVReader & aDataListReader, uint32_t &aProfileId, uint64_t &aInstanceId, uint64_t &aPreviousFirstSameTraitVersion, uint32_t &aNumDataElements);
    static WEAVE_ERROR GetProfileAndInstanceIds(TLV::TLVReader & aPathReader, uint32_t &aProfileId, uint64_t &aInstanceId);
    static WEAVE_ERROR AllocateRightSizedBuffer(PacketBuffer *& buf,
                                              const uint32_t desiredSize,
                                              const uint32_t minSize,
                                              uint32_t & outMaxPayloadSize);
    static void StatusListVersionListWriter(nl::Weave::TLV::TLVWriter &aWriter, void  *apContext);
    static void BuildStatusDataHandleElement(PacketBuffer* pBuf, TraitDataHandle aTraitDataHandle,uint32_t aProfileId, uint16_t aStatusCode, uint32_t aNumDataElement);

    static WEAVE_ERROR StoreUpdateDE(Weave::TLV::TLVReader & aReader, uint64_t & aPreviousFirstSameTraitVersion, TraitDataHandle & aHandle, PropertyPathHandle & aPathHandle,  TraitDataSource * apDataSource);

    static WEAVE_ERROR GenerateUpdateResponse(nl::Weave::ExchangeContext * apEC, uint32_t aNumDataElements, const TraitCatalogBase<TraitDataSource> * apCatalog, PacketBuffer* pBuf, bool existSuccess, bool existFailure, uint32_t aMaxPayloadSize);

    static WEAVE_ERROR CheckAndStoreDataList(Weave::TLV::TLVReader & aDataListReader, Weave::TLV::TLVReader & aReader, uint64_t & aPreviousFirstSameTraitVersion, uint32_t & aNumDataElements, TraitDataHandle & aHandle, PropertyPathHandle & aPathHandle, bool & aOutIsPartialChange, const TraitCatalogBase<TraitDataSource> * apCatalog, IUpdateRequestDataElementAccessControlDelegate & acDelegate);

    static WEAVE_ERROR ProcessDataListAndGenerateUpdateResponse(nl::Weave::ExchangeContext * apEC, nl::Weave::TLV::TLVReader & aReader,
                                       const TraitCatalogBase<TraitDataSource> * apCatalog,
                                       bool & aOutIsPartialChange,
                                       TraitDataHandle & aOutTraitDataHandle,
                                       IUpdateRequestDataElementAccessControlDelegate & acDelegate);

#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
#if WEAVE_DETAIL_LOGGING
    void LogSubscriptionFreed(void) const;
#endif // #if WEAVE_DETAIL_LOGGING

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
    // Publisher-specific features

    enum
    {
        kMaxNumSubscriptionHandlers = (WDM_MAX_NUM_SUBSCRIPTION_HANDLERS),
        kMaxNumPathGroups           = (WDM_PUBLISHER_MAX_NUM_PATH_GROUPS),
        kMaxNumPropertyPathHandles  = (WDM_PUBLISHER_MAX_NUM_PROPERTY_PATH_HANDLES),
        kMaxNumCommandObjs          = (WDM_MAX_NUM_COMMAND_OBJECTS), ///< Max number of command objects this engine can accommodate
    };

    Command mCommandObjs[kMaxNumCommandObjs];

    // Lock
    IWeavePublisherLock * mLock;

    // ******************* begin protected by lock **************************
    bool mIsPublisherEnabled;
    SubscriptionHandler mHandlers[kMaxNumSubscriptionHandlers];
    TraitCatalogBase<TraitDataSource> * mPublisherCatalog;
    NotificationEngine mNotificationEngine;

    // used for fairness
    uint16_t mNextHandlerToNotify;

    uint16_t mNumTraitInfosInPool;
    SubscriptionHandler::TraitInstanceInfo mTraitInfoPool[kMaxNumPathGroups];

    uint16_t mNumOfPropertyPathHandlesAllocated;
    // PropertyPathHandle mPropertyPathHandlePool[kMaxNumPropertyPathHandles];
    // ******************* end protected by lock   **************************

    void ReclaimTraitInfo(SubscriptionHandler * const aHandlerToBeReclaimed);

    static void OnSubscribeRequest(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                   const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                   PacketBuffer * aPayload);

    static void OnSubscribeConfirmRequest(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                          const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                          PacketBuffer * aPayload);

#if WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS
    static void OnCustomCommand(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                PacketBuffer * aPayload);

#endif // WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS

#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_SUBSCRIPTION_ENGINE_CURRENT_H

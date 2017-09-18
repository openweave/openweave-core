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
 *  @file
 *
 *  @brief
 *    Definitions for the abstract DMPublisher base class.
 *
 *  This file contains definitions for the abstract DMPublisher base
 *  class, which should be used as the basis for application-specific
 *  publishers base on WDM. See the document, "Nest Weave-Data
 *  Management Protocol" document for a complete description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_DM_PUBLISHER_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_DM_PUBLISHER_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/ProtocolEngine.h>
#include <Weave/Profiles/data-management/PublisherDataManager.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @class DMPublisher
     *
     *  @brief
     *    The abstract base class for application-specific WDM publishers.
     *
     *  DMPublisher is the standard WDM publisher base class. It is a
     *  mix of the DM ProtocolEngine class, which handles the comms
     *  crank-turning, and the wholly abstract PublisherDataManager
     *  class. Support for subscription and notification are optional
     *  and may be suppressed simply by configuring a subscription table
     *  with no entries.
     */

    class DMPublisher :

        public ProtocolEngine,

        public PublisherDataManager
    {
    public:

        DMPublisher(void);

        virtual ~DMPublisher(void);

        virtual WEAVE_ERROR Init(WeaveExchangeManager *aExchangeMgr, uint32_t aResponseTimeout);

        inline WEAVE_ERROR Init(WeaveExchangeManager *aExchangeMgr)
        {
            return Init(aExchangeMgr, kResponseTimeoutNotSpecified);
        };

        virtual void Clear(void);

        virtual void Finalize(void);

        virtual void IncompleteIndication(Binding *aBinding, StatusReport &aReport);

        WEAVE_ERROR ViewResponse(ExchangeContext *aResponseCtx, StatusReport &aStatus, ReferencedTLVData *aDataList);

        WEAVE_ERROR UpdateResponse(ExchangeContext *aResponseCtx, StatusReport &aStatus);

        WEAVE_ERROR CancelTransactionRequest(uint16_t aTxnId, WEAVE_ERROR aError);

        void OnMsgReceived(ExchangeContext *aResponseCtx, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aMsg);

        /*
         * everything from here down is related to subscription and
         * notification and, in cases where the publisher can just get
         * by with servicing view and update requests, can be omitted.
         */

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

        WEAVE_ERROR BeginSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId);

        WEAVE_ERROR EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId);

        void ClearSubscriptionTable(void);

        bool SubscriptionTableEmpty(void) const;

        WEAVE_ERROR SubscribeResponse(ExchangeContext *aResponseCtx,
                                      StatusReport &aStatus,
                                      const TopicIdentifier &aTopicId,
                                      ReferencedTLVData *aDataList);

        virtual WEAVE_ERROR CancelSubscriptionIndication(ExchangeContext *aResponseCtxconst, const TopicIdentifier &aTopicId);

        virtual WEAVE_ERROR NotifyRequest(const uint64_t &aDestinationId,
                                          const TopicIdentifier &aTopicId,
                                          ReferencedTLVData &aDataList,
                                          uint16_t aTxnId,
                                          uint32_t aTimeout);

        virtual WEAVE_ERROR NotifyRequest(const TopicIdentifier &aTopicId,
                                          ReferencedTLVData &aDataList,
                                          uint16_t aTxnId,
                                          uint32_t aTimeout);

        virtual WEAVE_ERROR NotifyRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR NotifyConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId);

        /*
         * this inner Subscription class contains the information that
         * the publisher requires to maintain a map of topics onto
         * clients wishing to receive the data of interest. even
         * though it is, in principal, not part of the published
         * interface it needs to be pubic so that the various handlers
         * can get at it.
         */

        class Subscription
        {
        public:
            /*
             * subscriptions on the publisher may be allocated and
             * activated separately. the following bit flags are used
             * in this regard.
             */

            enum
            {
                kSubscriptionFlags_Active =      2,
                kSubscriptionFlags_Allocated =   1,
                kSubscriptionFlags_Free =        0
            };

            Subscription(void);

            ~Subscription(void);

            WEAVE_ERROR Init(const TopicIdentifier &aTopicId, const TopicIdentifier &aRequestedTopicId, const uint64_t &aClientId);

            void Free(void);

            inline bool IsFree(void)
            {
                return !(mFlags & kSubscriptionFlags_Allocated);
            }

            inline bool IsActive(void)
            {
                return (mFlags & kSubscriptionFlags_Allocated) && (mFlags & kSubscriptionFlags_Active);
            }

            inline void Activate(void)
            {
                mFlags |= kSubscriptionFlags_Active;
            }

            inline void Deactivate(void)
            {
                mFlags &= !kSubscriptionFlags_Active;
            }

            inline bool MatchSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId) const
            {
                return ((aTopicId == kTopicIdNotSpecified || mAssignedId == aTopicId || mRequestedId == aTopicId) &&
                        (aClientId == kAnyNodeId || mClientId == aClientId));
            }

            TopicIdentifier mAssignedId;
            TopicIdentifier mRequestedId;
            uint64_t        mClientId;
            ExchangeContext *mSubscriptionCtx;
            uint8_t         mFlags;
        };

    protected:
        /*
         * this method, as distinct from BeginSubscription above,
         * simply adds a subscription to the table without activating
         * it and returns a pointer to the subscription object.
         */

        Subscription *AddSubscription(const TopicIdentifier &aTopicId, const TopicIdentifier &aRequestedTopicId, const uint64_t &aClientId);

        /*
         * and this one removes a subscription (or possibly many
         * subscriptions).
         */

        void RemoveSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId);

        /*
         * this one does the same but also calls the failure indicatio
         * on the data manager object.
         */

        void FailSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId, StatusReport &aReport);

        /*
         * the inner transaction class for the Notify transactions is
         * derived from the ProtocolEngine::Transaction object and
         * contains whatever additional information is required to
         * make the notification work.
         */

        class Notify :
            public DMTransaction
        {

        public:
            WEAVE_ERROR Init(DMPublisher *aPublisher,
                             const TopicIdentifier &aTopicId,
                             ReferencedTLVData &aDataList,
                             uint16_t aTxnId,
                             uint32_t aTimeout);
            void Free(void);

            WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags);

            // transaction-specific handlers

            WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus);

            ReferencedTLVData mDataList;
            TopicIdentifier mTopicId;
        };

        Notify *NewNotify(void);

        // data members

        Subscription mSubscriptionTable[kSubscriptionMgrTableSize];

        Notify       mNotifyPool[kNotifyPoolSize];

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_DM_PUBLISHER_LEGACY_H

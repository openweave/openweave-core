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
 *    Definitions for the abstract DMClient base class.
 *
 *  This file contains definitions for the abstract DMClient base
 *  class, which serves as the basis for application-specific clients
 *  based on WDM. See the document, "Nest Weave-Data Management
 *  Protocol" document for a complete(ish) description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_DM_CLIENT_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_DM_CLIENT_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <Weave/Profiles/data-management/ClientDataManager.h>
#include <Weave/Profiles/data-management/ProtocolEngine.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @class DMClient
     *
     *  @brief
     *    The abstract base class for application-specific WDM clients
     *
     *  DMClient is the standard WDM client. The implementation
     *  optionally includes subscription/notification. It is a mix of
     *  the DM ProtocolEngine class, which handles the comms
     *  crank-turning, and the wholly abstract ClientDataManager class,
     *  with some of the subscription-related methods implemented so
     *  that higher layers don't have to worry about them.
     *
     *  The handlers for subscription-related tasks mainly act as a
     *  thin adapter over the ClientNotifier object. Subclass
     *  implementers should take care to call the relevant super-class
     *  methods in order to turn the subscription manager crank.
     *
     *  DMClient request methods generally have 2 signatures, one with
     *  an explicit destination node ID and the other with a specified
     *  destination. In either case, the ability to send a message to
     *  a publisher depends on a pre-existing binding in the client
     *  but, in the first case, the destination ID is intended to
     *  select between multiple bound destination and, in the second
     *  case, the first item in the binding table is selected as a
     *  default. This is useful, for example, if the client will only
     *  ever be bound to a single publisher.
     */

    class DMClient :

        public ProtocolEngine,

        public ClientDataManager
    {
    public:

        DMClient(void);

        virtual ~DMClient(void);

        virtual void Clear(void);

        virtual void Finalize(void);

        virtual void IncompleteIndication(Binding *aBinding, StatusReport &aReport);

        virtual WEAVE_ERROR ViewRequest(const uint64_t &aDestinationId, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR ViewRequest(ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        /*
         * this whole block can be turned on and off at compile time
         * in order to enable or disable subscription for the whole
         * device. there's not much point in enabling or disabling it
         * for individual clients since there will, in any case, be
         * only one client notifier, defined statically below.
         */

        bool HasSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId);

        inline bool HasSubscription(const TopicIdentifier &aTopicId)
        {
            return HasSubscription(aTopicId, kNodeIdNotSpecified);
        };

        WEAVE_ERROR BeginSubscription(const TopicIdentifier &aAssignedId, const TopicIdentifier &aRequestedId, const uint64_t &aPublisherId);

        WEAVE_ERROR EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId);

        virtual WEAVE_ERROR SubscribeRequest(const uint64_t &aDestinationId, const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR SubscribeRequest(const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR SubscribeRequest(const uint64_t &aDestinationId, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR SubscribeRequest(ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR CancelSubscriptionRequest(const uint64_t &aDestinationId,
                                                      const TopicIdentifier &aTopicId,
                                                      uint16_t aTxnId,
                                                      uint32_t aTimeout);

        virtual WEAVE_ERROR CancelSubscriptionRequest(const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout);

        /*
         * There are multiple clients but only one notifier and, if more than
         * one client tries to install a notifier, subscriptions that have
         * been placed with the first one will be lost mysteriously. So, we
         * need to put a single notifier in place.
         */

        static ClientNotifier sNotifier;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        virtual WEAVE_ERROR UpdateRequest(const uint64_t &aDestinationId, ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout);

        virtual WEAVE_ERROR UpdateRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

        /*
         * this method is provided as a way of allowing access to the
         * "legacy" message types. at this point only UpdateRequest()
         * - and only this particular form of UpdateReuest() - actually
         * needs it.
         */

        virtual WEAVE_ERROR UpdateRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout, bool aUseLegacyMsgType);

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

        /*
         * cancel transaction just returns a Weave error and doesn't
         * itself require any saved state.
         */

        WEAVE_ERROR CancelTransactionRequest(uint16_t aTxnId, WEAVE_ERROR aError);

        // public data members

        /*
         * when we're checking resource usage we want to track
         * these. they are here for testing and diagnostic purposes
         * and may be turned off in production builds.
         */

    protected:
        /*
         * transaction classes for the View, Subscribe,
         * CancelSubscription amd and Update transactions, all derived
         * from the ProtocolEngine::Transaction object and containing
         * whatever additional information is required for the
         * operation of interest.
         */

        class View :
            public DMTransaction
        {
        public:
            WEAVE_ERROR Init(DMClient *aClient,
                             ReferencedTLVData &aPathList,
                             uint16_t aTxnId,
                             uint32_t aTimeout);

            void Free(void);

            WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags);

            // transaction-specific handlers

            WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus);

            WEAVE_ERROR OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg);

            ReferencedTLVData mPathList;
        };

        View *NewView(void);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        /*
         * again, we want to be able to enable or disable subscription
         * at compile time for a particular device.
         */

        class Subscribe :
            public DMTransaction
        {
        public:
            WEAVE_ERROR Init(DMClient *aClient,
                             const TopicIdentifier &aTopicId,
                             uint16_t aTxnId,
                             uint32_t aTimeout);
            WEAVE_ERROR Init(DMClient *aClient,
                             ReferencedTLVData &aPathList,
                             uint16_t aTxnId,
                             uint32_t aTimeout);
            void Free(void);

            WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags);

            // transaction-specific handlers

            WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus);

            WEAVE_ERROR OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg);

            ReferencedTLVData  mPathList;
            TopicIdentifier    mTopicId;
        };

        Subscribe *NewSubscribe(void);

        Subscribe           mSubscribePool[kSubscribePoolSize];

        class CancelSubscription :
            public DMTransaction
        {
        public:
            WEAVE_ERROR Init(DMClient *aClient,
                             const TopicIdentifier &aTopicId,
                             uint16_t aTxnId,
                             uint32_t aTimeout);

            void Free(void);

            WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags);

            // transaction-specific handlers

            WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus);

            TopicIdentifier mTopicId;
        };

        CancelSubscription *NewCancelSubscription(void);

        CancelSubscription  mCancelSubscriptionPool[kCancelSubscriptionPoolSize];

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        class Update :
            public DMTransaction
        {
        public:
            WEAVE_ERROR Init(DMClient *aClient,
                             ReferencedTLVData &aDataList,
                             uint16_t aTxnId,
                             uint32_t aTimeout);

            void Free(void);

            WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags);

            // transaction-specific handlers

            WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus);

            // data members

            ReferencedTLVData mDataList;
        };

        Update *NewUpdate(void);

        // protected data members

        View                mViewPool[kViewPoolSize];
        Update              mUpdatePool[kUpdatePoolSize];
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_DM_CLIENT_LEGACY_H

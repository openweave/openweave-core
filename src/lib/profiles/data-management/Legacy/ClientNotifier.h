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
 *    Definitions for the ClientNotifier auxiliary class.
 *
 *  This file provides definitions for the ClientNotifier auxiliary
 *  class, which is employed when a WDM client wishes to support
 *  subscription and notification. See the document, "Nest Weave-Data
 *  Management Protocol" document for a complete description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_CLIENT_NOTIFIER_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_CLIENT_NOTIFIER_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    class DMClient;

    /**
     *  @class ClientNotifier
     *
     *  @brief
     *    An auxiliary class employed when subscription and
     *    notification support are desired on a WDM client.
     *
     *  The ClientNotifier is a class that performs dispatching of
     *  incoming notifications to the interested client based on the
     *  topic ID provided by the publisher at subscription time or else
     *  based on a well-known topic ID. The ClientNotifier also returns
     *  a status to the publisher.
     *
     *  Subscription and notification are optional in WDM but if they
     *  are supported the client must be provided with a notifier at
     *  initialization time.
     */

    class ClientNotifier
    {
        friend class DMClient;
        friend class DMClientTester;

    public:

        /**
         *  @brief
         *    A client-side subscription.
         *
         *  A subscription on the client side just maps a pair:
         *
         *    [<topic id>, <publisher id>]
         *
         *  onto a client data manager object. both topic ID and
         *  publisher ID may be supplied as wild cards. A subscription
         *  may be requested under a known topic ID as well in which
         *  case the requested ID is kept around for reference.
         *
         *  @note
         *    Generally, implementers need not concern themselves with
         *    subscriptions since they are manged by the
         *    ClientNotifier class and even this management function
         *    is further concealed by the DMClient class. These
         *    interfaces are public largely to provide future
         *    flexibility and expansion.
         */

        class Subscription
        {
            friend class ClientNotifier;

        public:
            Subscription(void);

            virtual ~Subscription(void);

            WEAVE_ERROR Init(const TopicIdentifier &aAssignedId,
                             const TopicIdentifier &aRequestedId,
                             const uint64_t &aPublisherId,
                             DMClient *aClient);

            void Free(void);

            inline bool IsFree(void)
            {
                return (mAssignedId == kTopicIdNotSpecified &&
                        mRequestedId == kTopicIdNotSpecified &&
                        mPublisherId == kNodeIdNotSpecified &&
                        mClient == NULL);
            }

            /**
             *  @brief
             *    Check the target of a subscription.
             *
             *  This test is used to check incoming messages against
             *  the notifier table. In addition to checking an exact
             *  match in either of the parameters of interest, it also
             *  checks whether the table contains "wildcards" that
             *  match.
             *
             *  @param [in]     aTopicId        A reference to the
             *                                  publisher-assigned
             *                                  "working" topic ID
             *                                  under which the
             *                                  subscription is
             *                                  stored.
             *
             *  @param [in]     aPublisherId    A reference to the 64-
             *                                  bit node ID or service
             *                                  endpoint of the
             *                                  publisher servicing
             *                                  the subscription.
             *
             *  @return true if the subscription matches, false
             *  otherwise.
             */

            inline bool CheckSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId)
            {
                return ((mAssignedId == kAnyTopicId || mAssignedId == aTopicId) &&
                        (aPublisherId == kAnyNodeId || mPublisherId == kAnyNodeId || mPublisherId == aPublisherId));
            }

            /**
             *  @brief
             *    Check the contents of a subscription.
             *
             *  This test, is used in order to figure out whether the
             *  notifier table contains a particular subscription and
             *  is assumed to be called "from above". As such, the
             *  parameters may have "wildcard" values but, if they do
             *  not, then an exact match is required. This version
             *  assumes that the data manager component of the
             *  subscription is the current client.
             *
             *  @param [in]     aTopicId        A reference to the
             *                                  publisher-assigned
             *                                  "working" topic ID
             *                                  under which the
             *                                  subscription is
             *                                  stored.
             *
             *  @param [in]     aPublisherId    A reference to the 64-
             *                                  bit node ID or service
             *                                  endpoint of the
             *                                  publisher servicing
             *                                  the subscription.
             *
             *  @return true if the subscription matches, false
             *  otherwise.
             */

            inline bool MatchSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId) const
            {
                return ((aTopicId == kTopicIdNotSpecified || mAssignedId == aTopicId || mRequestedId == aTopicId) &&
                        (aPublisherId == kNodeIdNotSpecified || mPublisherId == aPublisherId));
            }

            /**
             *  @brief
             *    Check the contents of a subscription.
             *
             *  This test, is used in order to figure out whether the
             *  notifier table contains a particular subscription and
             *  is assumed to be called "from above". As such, some
             *  parameters may have "wildcard" values but, if they do
             *  not, then an exact match is required.
             *
             *  @param [in]     aTopicId        A reference to the
             *                                  publisher-assigned
             *                                  "working" topic ID
             *                                  under which the
             *                                  subscription is
             *                                  stored.
             *
             *  @param [in]     aPublisherId    A reference to the 64-
             *                                  bit node ID or service
             *                                  endpoint of the
             *                                  publisher servicing
             *                                  the subscription.
             *
             *  @param [in]     aClient         A pointer to the
             *                                  DMClient present in
             *                                  the subscription.
             *
             *  @return true if the subscription matches, false
             *  otherwise.
             */

            inline bool MatchSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient) const
            {
                return MatchSubscription(aTopicId, aPublisherId) && mClient == aClient;
            }

            /**
             *  @brief
             *    The client to which this subscription relates.
             *
             *  This member variable is public because users of the
             *  subscription class need to be able to get at it in
             *  order to invoke indications.
             */

            DMClient *mClient;

        protected:

            TopicIdentifier    mAssignedId;
            TopicIdentifier    mRequestedId;
            uint64_t           mPublisherId;
        };

        ClientNotifier(void);

        virtual ~ClientNotifier(void);

        WEAVE_ERROR DispatchNotifyIndication(ExchangeContext *aResponseCtx, PacketBuffer *payload);

        inline bool SubscriptionIsEnabled(void)
        {
            return mSubscriptionCount != 0;
        }

        bool HasSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient) const;

        WEAVE_ERROR InstallSubscription(const TopicIdentifier &aTopicId,
                                        const TopicIdentifier &aRequestedId,
                                        const uint64_t &aPublisherId,
                                        DMClient *aClient);

        void RemoveSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient);

        void FailSubscription(const TopicIdentifier &aTopicId,
                              const uint64_t &aPublisherId,
                              DMClient *aClient,
                              StatusReport &aReport);

        void Clear(void);

    protected:

        WeaveExchangeManager  *mExchangeMgr;
        uint16_t               mSubscriptionCount;
        Subscription           mNotifierTable[kNotifierTableSize];
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_CLIENT_NOTIFIER_LEGACY_H

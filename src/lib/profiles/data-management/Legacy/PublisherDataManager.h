/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *    Definitions for the abstract PublisherDataManager class.
 *
 *  This file contains definitions, all pure virtual, for the confirm
 *  and indication methods required for data management on a WDM
 *  publisher.  See the document, "Nest Weave-Data Management Protocol"
 *  document for a complete description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_PUBLISHER_DATA_MANAGER_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_PUBLISHER_DATA_MANAGER_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @class PublisherDataManager
     *
     *  @brief
     *    An abstract class containing confirm and notification method
     *    definitions required by the WDM publisher.
     *
     *  Class PublisherDataManager is an abstract class that spells out
     *  the methods an application implementer must provide in order to
     *  handle the data and status delivered by the publisher in WDM
     *  protocol exchanges. It is one of the two primary components of
     *  the DMPublisher abstract base class.
     *
     *  Note that all of the indication methods below take an exchange
     *  context argument and that it is the responsibility of
     *  implementers of these methods to manage this context. In
     *  particular, to dispose of it when it is no longer needed.
     */

    class PublisherDataManager
    {
    public:

        /**
         *  @brief
         *    Indicate receipt of a view request.
         *
         *  Indicate that a view request frame has been received and
         *  the sender awaits processing and response.
         *
         *  @param [in]     aResponseCtx    A pointer to the Weave
         *                                  exchange context in which
         *                                  the message was delivered.
         *                                  See above note about
         *                                  exchange contexts.
         *
         *  @param [in]     aPathList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  path list detailing
         *                                  requested data.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR ViewIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList) =  0;

        /**
         *  @brief
         *    Indicate the receipt of an update request.
         *
         *  Indicate that an update request frame has been received and
         *  the sender awaits processing and response.
         *
         *  @param [in]     aResponseCtx    A pointer to the Weave
         *                                  exchange context in which
         *                                  the message was delivered.
         *                                  See above note about
         *                                  exchange contexts.
         *
         *  @param [in]     aDataList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  data list detailing new
         *                                  data values and including
         *                                  the identifier of the
         *                                  version against which the
         *                                  update was made.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR UpdateIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aDataList) = 0;

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

        /**
         *  @brief
         *    Indicate the receipt of a subscribe request for a topic.
         *
         *  Indicate that an subscribe request frame has been received
         *  and the sender awaits processing and response. In this
         *  version, the request contained a well-known topic ID.
         *
         *  @note
         *    This interface is only available if #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
         *    has been asserted.
         *
         *  @param [in]     aResponseCtx    A pointer to the Weave
         *                                  exchange context in which
         *                                  the message was delivered.
         *                                  See above note about
         *                                  exchange contexts.
         *
         *  @param [in]     aTopicId        A reference to the
         *                                  well-known, 64-bit
         *                                  identifier of the topic to
         *                                  which a subscription is
         *                                  being requested.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR SubscribeIndication(ExchangeContext *aResponseCtx, const TopicIdentifier &aTopicId) = 0;

        /**
         *  @brief
         *    Indicate the receipt of a subscribe request for a path
         *    list.
         *
         *  Indicate that an subscribe request frame has been received
         *  and the sender awaits processing and response. In this
         *  case, the request contained a path list to specify the data
         *  of interest.
         *
         *  @note
         *    This interface is only available if #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
         *    has been asserted.
         *
         *  @param [in]     aResponseCtx    A pointer to the Weave
         *                                  exchange context in which
         *                                  the message was delivered.
         *                                  See above note about
         *                                  exchange contexts.
         *
         *  @param [in]     aPathList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  path list detailing the
         *                                  data of interest the
         *                                  client that sent the
         *                                  request.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR SubscribeIndication(ExchangeContext *aResponseCtx, ReferencedTLVData &aPathList) = 0;

        /**
         *  @brief
         *    Indicate the failure of a subscription.
         *
         *  Handle an indication that a previously installed
         *  subscription has failed for some reason or has been
         *  canceled.
         *
         *  @note
         *    This interface is only available if #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
         *    has been asserted.
         *
         *  @param [in]     aClientId       A reference to the 64-bit
         *                                  node ID of the client.
         *
         *  @param [in]     aTopicId        A reference to the topic
         *                                  ID assigned by this
         *                                  publisher.
         *
         *  @param [in]     aReport         A reference to a
         *                                  StatusReport object giving
         *                                  information about how the
         *                                  subscription failed.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR UnsubscribeIndication(const uint64_t &aClientId, const TopicIdentifier &aTopicId, StatusReport &aReport) = 0;

        /**
         *  @brief
         *    Indicate the receipt of a cancel subscribe request.
         *
         *  Indicate that a cancel subscription request frame has been
         *  received, processed and responded to. The underlying code
         *  both processes and responds on behalf of the next higher
         *  layer.
         *
         *  @note
         *    This interface is only available if #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
         *    has been asserted.
         *
         *  @param [in]     aResponseCtx    A pointer to the Weave
         *                                  exchange context in which
         *                                  the message was delivered.
         *                                  See above note about
         *                                  exchange contexts.
         *
         *  @param [in]     aTopicId        A reference to the 64-bit
         *                                  "working" topic identifier
         *                                  supplied by the publisher
         *                                  (the receiver in this
         *                                  case) to cover the
         *                                  subscription that the
         *                                  client now wishes to
         *                                  cancel.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR CancelSubscriptionIndication(ExchangeContext *aResponseCtx, const TopicIdentifier &aTopicId) = 0;

        /**
         *  @brief
         *    Confirm the receipt of a notification.
         *
         *  Confirm that a notify frame was received by a subscriber
         *  and a status response was returned.
         *
         *  Note that under most circumstances, there is little or no
         *  reason for a publisher to provided an implementation for
         *  this method beyond the one provided by the DMClient base
         *  class.
         *
         *  @note
         *    This interface is only available if #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
         *    has been asserted.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  client.
         *
         *  @param [in]     aTopicId        A reference to the 64-bit
         *                                  topic ID under which the
         *                                  notification was issued.
         *
         *  @param [in]     aStatus         A reference to a
         *                                  StatusReport object
         *                                  containing the status of
         *                                  the notification. The two
         *                                  possibilities here are
         *                                  "success" and "unknown
         *                                  topic" in which case the
         *                                  publisher assumes that the
         *                                  client has somehow lost
         *                                  the subscription.
         *
         *  @param [in]     aTxnId          The transaction ID that
         *                                  refers to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR NotifyConfirm(const uint64_t &aResponderId,
                                          const TopicIdentifier &aTopicId,
                                          StatusReport &aStatus,
                                          uint16_t aTxnId) = 0;

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_PUBLISHER_DATA_MANAGER_LEGACY_H

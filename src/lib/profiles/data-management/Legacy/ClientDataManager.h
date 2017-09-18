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
 *    Definitions for the abstract ClientDataManager class.
 *
 *  This file contains definitions, all pure virtual, for the confirm
 *  and indication methods required for data management on a WDM
 *  client.  See the document, "Nest Weave-Data Management Protocol"
 *  document for a complete description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_CLIENT_DATA_MANAGER_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_CLIENT_DATA_MANAGER_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/TopicIdentifier.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @class ClientDataManager
     *
     *  @brief
     *    An abstract class containing confirm and indication
     *    method definitions required by the WDM client.
     *
     *  Class ClientDataManager is an abstract class that spells out
     *  the methods an application implementer must provide in order to
     *  handle the data and status delivered by the publisher in WDM
     *  protocol exchanges. These methods are, for the most part,
     *  confirmations invoked as a result of the receipt of a response
     *  to a client request and indications of the receipt of a
     *  request from a remote peer. ClientDataManager is one of the
     *  two primary components of the DMClient abstract base class.
     */

    class ClientDataManager
    {
    public:

        /**
         *  @brief
         *    Confirm a failed view request.
         *
         *  Confirm that a view request failed in some way and a status
         *  report has been submitted describing the failure.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aStatus         A reference to a
         *                                  StatusReport object
         *                                  detailing what went wrong.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Confirm a successful view request.
         *
         *  Confirm that a view request was received, a response was
         *  returned and that the operation was successful, delivering
         *  a data list.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aDataList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  data list with the
         *                                  requested data.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, ReferencedTLVData &aDataList, uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Confirm the status of an update request.
         *
         *  In the case of update requests, there is no distinguished
         *  "success" response. In either case, the responder sends a
         *  status report and this is how it is delivered to the next
         *  higher layer.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aStatus         A reference to a
         *                                  StatusReport object
         *                                  detailing the status of
         *                                  the request.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR UpdateConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Confirm a failed subscribe request.
         *
         *  Confirm that a subscribe request failed in some way and a
         *  status report has been submitted describing the failure.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aStatus         A reference to a
         *                                  StatusReport object
         *                                  detailing what went wrong.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        virtual WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Confirm a successful subscribe request.
         *
         *  Confirm that a subscribe request was received, the
         *  subscription was successfully installed and a response was
         *  generated and received.
         *
         *  In this version of the response, the publisher sends back a
         *  topic ID but NOT a data list. This will be the case if the
         *  publisher wishes to respond before it is finished
         *  marshaling the relevant data, which shall be returned in a
         *  separate notify request.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aTopicId        A reference to the topic
         *                                  ID that applies to the
         *                                  subscription. This may be
         *                                  a well-known topic ID
         *                                  under which the
         *                                  subscription was requested
         *                                  or else the topic ID
         *                                  assigned by the publisher
         *                                  in the case where no topic
         *                                  ID was given.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Confirm a successful subscribe request.
         *
         *  Confirm that a subscribe request was received, the
         *  subscription was successfully installed and a response was
         *  generated and received.
         *
         *  In this version of the response a data list is included,
         *  which constitutes the state of the data of interest at the
         *  time of receipt of the subscribe request.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aTopicId        A reference to the topic
         *                                  ID that applies to the
         *                                  subscription. This may be
         *                                  a well-known topic ID
         *                                  under which the
         *                                  subscription was requested
         *                                  or else the topic ID
         *                                  assigned by the publisher
         *                                  in the case where no topic
         *                                  ID was given.
         *
         *  @param [in]     aDataList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  data list with the
         *                                  requested data or else, at
         *                                  a minimum in the case of
         *                                  the renewal of an existing
         *                                  subscription, version
         *                                  information regarding the
         *                                  data of interest.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId,
                                             const TopicIdentifier &aTopicId,
                                             ReferencedTLVData &aDataList,
                                             uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Handle an indication of subscription failure.
         *
         *  Handle an indication that a previously installed
         *  subscription has failed for some reason or has been
         *  canceled.
         *
         *  @param [in]     aPublisherId    A reference to the 64-bit
         *                                  node ID of the publisher.
         *
         *  @param [in]     aTopicId        A reference to the topic
         *                                  ID assigned by the
         *                                  publisher.
         *
         *  @param [in]     aReport         A reference to a
         *                                  StatusReport object
         *                                  detailing how the
         *                                  subscription failed.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR UnsubscribeIndication(const uint64_t &aPublisherId, const TopicIdentifier &aTopicId, StatusReport &aReport) = 0;

        /**
         *  @brief
         *    Confirm the status of a cancel subscription request.
         *
         *  Confirm the status - success or failure - of a request to
         *  cancel a subscription requested of a particular publisher.
         *
         *  @param [in]     aResponderId    A reference to the 64-bit
         *                                  node ID of the responding
         *                                  publisher.
         *
         *  @param [in]     aTopicId        A reference to the topic
         *                                  ID applying to the
         *                                  subscription being
         *                                  canceled. Note that this
         *                                  is always the ID given in
         *                                  the request and may be a
         *                                  well-known topic if that
         *                                  was the method by which
         *                                  the subscription was
         *                                  originally requested.
         *
         *  @param [in]     aStatus         A reference to a
         *                                  StatusReport object
         *                                  detailing the status of
         *                                  the request. In some
         *                                  cases, e.g. when there is
         *                                  an error on the publisher,
         *                                  the status report may
         *                                  contain meta-data
         *                                  including, but not
         *                                  necessarily limited to,
         *                                  the TLV-encoded error
         *                                  code.
         *
         *  @param [in]     aTxnId          The client-assigned
         *                                  transaction ID that refers
         *                                  to this particular
         *                                  exchange.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
        */

        virtual WEAVE_ERROR CancelSubscriptionConfirm(const uint64_t &aResponderId,
                                                      const TopicIdentifier &aTopicId,
                                                      StatusReport &aStatus,
                                                      uint16_t aTxnId) = 0;

        /**
         *  @brief
         *    Handle a notification.
         *
         *  Handle an indication that a notification has been received
         *  with respect to an existing subscription.
         *
         *  @param [in]     aTopicId        A reference to the 64-bit
         *                                  topic identifier that
         *                                  applies to this
         *                                  subscription. Note that
         *                                  this may be a well-known
         *                                  topic ID or a
         *                                  publisher-assigned ID.
         *
         *  @param [in]     aDataList       A reference to a
         *                                  ReferencedTLVData object
         *                                  containing a TLV-encoded
         *                                  data list with the changed
         *                                  data from the publisher.
         *
         *  @return #WEAVE_NO_ERROR to communicate success. Otherwise
         *  the value is at the discretion of the implementer.
         */

        virtual WEAVE_ERROR NotifyIndication(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList) = 0;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_CLIENT_DATA_MANAGER_LEGACY_H

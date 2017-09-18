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
 *    Implementations for the abstract DMClient base class.
 *
 *  This file contains implementations for the abstract DMClient base
 *  class, which serves as the basis for application-specific clients
 *  based on WDM. See the document, "Nest Weave-Data Management
 *  Protocol" document for a complete(ish) description.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <SystemLayer/SystemStats.h>
#include <Weave/Support/CodeUtils.h>

#include <Weave/Profiles/data-management/DataManagement.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::nl::Weave::Profiles::StatusReporting;

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

// C++ requires this be declared here.

ClientNotifier DMClient::sNotifier;

#endif

/**
 *  @brief
 *    The default constructor for DMClient objects.
 *
 *  Clears all internal state. A DMClient requires further
 *  initialization with Init() before use.
 */

DMClient::DMClient(void)
{
    Clear();
}

/**
 *  @brief
 *    The destructor for DMClient objects.
 *
 *  Clears all internal state and, if necessary cancels pending
 *  subscriptions.
 */

DMClient::~DMClient(void)
{
    Finalize();
}

/**
 *  @brief
 *    Clear the internal state associated with a DMClient object.
 *
 *  In particular, this method clears all the client transaction
 *  pools. For clients that have been in use the Finalize() method is
 *  preferable since it also cancels subscriptions and cleans up the
 *  transaction and binding tables.
 *
 * @sa Finalize()
 */

void DMClient::Clear(void)
{
    int i;

    for (i = 0; i<kViewPoolSize; i++)
        mViewPool[i].Free();
    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumViews);

    for (i = 0; i<kUpdatePoolSize; i++)
        mUpdatePool[i].Free();
    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumUpdates);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    for (i = 0; i<kSubscribePoolSize; i++)
        mSubscribePool[i].Free();
    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumSubscribes);

    for (i = 0; i<kCancelSubscriptionPoolSize; i++)
        mCancelSubscriptionPool[i].Free();
    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumCancels);

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION
}

/**
 *  @brief
 *    Shut down an operating DMClient.
 *
 *  Clears all the operating state associated with the client and
 *  removes all related subscriptions from the notifier. After a call
 *  to Finalize() a DMClient may be reinitialized simply by calling
 *  Init(). Finalize() is invoked by the DMClient destructor but may
 *  be called in the case where a DMClient requires cleanup, e.g. in
 *  case of failure or temporary shutdown, but may need to be
 *  reconstituted at some later time.
 */

void DMClient::Finalize(void)
{
#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    sNotifier.RemoveSubscription(kTopicIdNotSpecified, kNodeIdNotSpecified, this);

#endif

    Clear();

    ProtocolEngine::Finalize();
}

/**
 *  @brief
 *    Handle the "incompletion" of a binding in use by the client.
 *
 *  When a binding fails unexpectedly, e.g. if the connection involved
 *  in the binding is closed, then this method is called.
 *
 *  @param [in]     aBinding        A pointer to the Binding that has
 *                                  become incomplete.
 *
 *  @param [in]     aReport         A reference to a status report
 *                                  giving a reason for the failure.
 */

void DMClient::IncompleteIndication(Binding *aBinding, StatusReport &aReport)
{
    ProtocolEngine::IncompleteIndication(aBinding, aReport);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    sNotifier.FailSubscription(kTopicIdNotSpecified, aBinding->mPeerNodeId, this, aReport);

#endif
}

/**
 *  @name
 *    View methods.
 *
 *  @brief
 *    View, under WDM, is used by a client to request a snapshot of
 *    specified data managed by a publisher.
 *
 * See Detailed Description for discussion of WDM request method
 * signatures.
 *
 *  @{
 */

/**
 *  @brief
 *    Request a view of published data.
 *
 *  Request a view of data residing on and managed by a specified
 *  remote publisher.
 *
 *  @param [in]     aDestinationId      A reference to the 64-bit
 *                                      node ID of the remote
 *                                      publisher.
 *
 *  @param [in]     aPathList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded path
 *                                      list indicating the requested
 *                                      data.
 *
 *  @param [in]     aTxnId,             An identifier for the WDM
 *                                      transaction set up to manage
 *                                      this view operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the view response.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If a transaction couldn't be allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::ViewRequest(const uint64_t &aDestinationId, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_INCORRECT_STATE;
    Binding *binding = GetBinding(aDestinationId);
    View *view;

    if (binding)
    {
        view = NewView();

        if (view)
        {
            err = view->Init(this, aPathList, aTxnId, aTimeout);
            SuccessOrExit(err);

            err = StartTransaction(view, binding);
        }

        else
        {
            err = WEAVE_ERROR_NO_MEMORY;
        }
    }

exit:
    return err;
}

/**
 *  @brief
 *    Request a view of data on default publisher.
 *
 *  Request a view of data residing on and managed by the "default"
 *  publisher, i.e. the first (or only) publisher in the client's
 *  binding table.
 *
 *  @param [in]     aPathList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded path
 *                                      list indicating the requested
 *                                      data.
 *
 *  @param [in]     aTxnId              An identifier for the WDM
 *                                      transaction set up to manage
 *                                      this view operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the view response.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If a transaction couldn't be
 *  allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::ViewRequest(ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    View *view = NewView();

    if (view)
    {
        err = view->Init(this, aPathList, aTxnId, aTimeout);
        SuccessOrExit(err);

        err = StartTransaction(view);
    }

exit:
    return err;
}

/**
 * @}
 */

/**
 *  @name
 *    Subscribe methods.
 *
 *  @brief
 *    Subscription, under WDM, is used by a client to request a
 *    snapshot of specified data managed by a publisher as with a
 *    view, but in addition requests notification when the data of
 *    interest changes,
 *
 * See Detailed Description for discussion of WDM request method
 * signatures.
 *
 *  @{
 */

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

/**
 *  @brief
 *    Check if this client has a particular subscription with a
 *    specified publisher.
 *
 *  @param [in]     aTopicId            A reference to a topic ID to
 *                                      search or. This may be a
 *                                      well-known ID or the "working"
 *                                      publisher-assigned ID and it
 *                                      may be the value kAnyTopicId
 *                                      in the case where the caller
 *                                      doesn't care.
 *
 *  @param [in]     aPublisherId        A reference to the 64-bit node
 *                                      ID of a publisher to search
 *                                      for. This may have a value of
 *                                      kAnyNodeId in the case where
 *                                      the caller doesn't care.
 *
 *  @return true if a match is found, false otherwise.
 */

bool DMClient::HasSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId)
{
    bool retVal = false;

    retVal =  sNotifier.HasSubscription(aTopicId, aPublisherId, this);

    return retVal;
}

/**
 *  @brief
 *    Start a subscription.
 *
 *  This method installs a subscription to a particular assigned
 *  ID/requested ID pair in the notifier's subscription table with the
 *  current client as the client requiring notification.
 *
 *  @param [in] aAssignedId A reference to the "working" topic ID
 *                                      assigned by the publisher. In
 *                                      the case where the
 *                                      subscription is unilateral
 *                                      this parameter shall have a
 *                                      value of
 *                                      kTopicIdNotSpecified. In the
 *                                      case where the a subscribe
 *                                      request has been sent to the
 *                                      publisher and no well-known ID
 *                                      has been supplied, the
 *                                      "working" topic ID is
 *                                      delivered as a parameter to
 *                                      the SubscribeConfirm() method
 *                                      that results form the receipt
 *                                      of a successful response.
 *
 *  @param [in]     aRequestedId        A reference to a well-known
 *                                      topic ID given in the initial
 *                                      request. This parameter may
 *                                      take a value of
 *                                      kTopicIdNotSpecified in the
 *                                      case where there was no
 *                                      requested topic.
 *
 *  @param [in]     aPublisherId        A reference to the 64-bit node
 *                                      ID of the publisher.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting a failure to install the subscription.
 */

WEAVE_ERROR DMClient::BeginSubscription(const TopicIdentifier &aAssignedId, const TopicIdentifier &aRequestedId, const uint64_t &aPublisherId)
{
    WEAVE_ERROR err;

    err =  sNotifier.InstallSubscription(aAssignedId, aRequestedId, aPublisherId, this);

    return err;
}

/**
 *  @brief
 *    Stop, and remove, a subscription.
 *
 *  Remove a subscription from the notifier's subscription table
 *  thereby stopping any future notifications from being delivered to
 *  this client.
 *
 *  @note
 *    This method simply removes the subscription locally. To cancel a
 *    subscription that has been established using SubscribeRequest(),
 *    use CancelSubscriptionRequest().
 *
 *  @sa CancelSubscriptionRequest(const uint64_t &aDestinationId, const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
 *
 *  @param [in]     aTopicId            A reference to a topic ID
 *                                      associated with the
 *                                      subscription. This may be the
 *                                      "working" ID or the requested
 *                                      ID.
 *
 *  @param [in]     aPublisherId        A reference to the 64-bit node
 *                                      ID of the publisher.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting a failure to remove the subscription.
 */

WEAVE_ERROR DMClient::EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    sNotifier.RemoveSubscription(aTopicId, aPublisherId, this);

    return err;

}

/**
 *  @brief
 *    Request a subscription to a published topic from a specified
 *    publisher.
 *
 *  Request a view of data residing on and managed by a specified
 *  remote publisher and request subsequent updates, at the discretion
 *  of the publisher in the case that the data changes. This version
 *  uses a known topic ID.
 *
 *  @param [in]     aDestinationId      A reference to the 64-bit node
 *                                      ID of the remote publisher.
 *
 *  @param [in]     aTopicId            A reference to a well-known
 *                                      topic identifier representing
 *                                      the requested data.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INCORRECT_STATE If a binding to the
 *  destination could not be found.
 *
 *  @return #WEAVE_ERROR_NO_MEMORY If a transaction couldn't be allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::SubscribeRequest(const uint64_t &aDestinationId, const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Binding *binding = GetBinding(aDestinationId);
    Subscribe *subscribe;

    if (binding)
    {
        subscribe = NewSubscribe();

        if (subscribe)
        {
            err = subscribe->Init(this, aTopicId, aTxnId, aTimeout);
            SuccessOrExit(err);

            err = StartTransaction(subscribe, binding);
        }

        else
        {
            err = WEAVE_ERROR_NO_MEMORY;
        }
    }

    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

exit:
    return err;
}

/**
 *  @brief
 *    Request a subscription to a published topic on the default
 *    publisher.
 *
 *  Request a view of data residing on and managed by the "default"
 *  publisher, i.e. the first (or only) publisher in the client's
 *  binding table request subsequent updates, at the discretion of the
 *  publisher in the case that the data changes. This version uses a
 *  known topic ID.
 *
 *  @param [in]     aTopicId            A reference to a well-known
 *                                      topic identifier representing
 *                                      the requested data.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If a transaction couldn't be allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::SubscribeRequest(const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Subscribe *subscribe = NewSubscribe();

    if (subscribe)
    {
        err = subscribe->Init(this, aTopicId, aTxnId, aTimeout);
        SuccessOrExit(err);

        err = StartTransaction(subscribe);
    }

    else
    {
        err = WEAVE_ERROR_NO_MEMORY;
    }

exit:
    return err;
}

/**
 *  @brief
 *    Request a subscrption to published data from a specified
 *    publisher.
 *
 *  Request a view of data residing on and managed by a specified
 *  remote publisher and request subsequent updates, at the discretion
 *  of the publisher in the case that the data changes. This version
 *  uses a path list to specify the data of interest.
 *
 *  @param [in]     aDestinationId      A reference to the 64-bit node ID
 *                                      of the remote publisher.
 *
 *  @param [in]     aPathList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded path
 *                                      list representing the
 *                                      requested data.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INCORRECT_STATE If a binding to the
 *  destination could not be found.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If a transaction couldn't be allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::SubscribeRequest(const uint64_t &aDestinationId, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Binding *binding = GetBinding(aDestinationId);
    Subscribe *subscribe;

    if (binding)
    {
        subscribe = NewSubscribe();

        if (subscribe)
        {
            err = subscribe->Init(this, aPathList, aTxnId, aTimeout);
            SuccessOrExit(err);

            err = StartTransaction(subscribe, binding);
        }

        else
        {
            err = WEAVE_ERROR_NO_MEMORY;
        }
    }

    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

exit:
    return err;
}

/**
 *  @brief
 *    Request a subscription to data on the default publisher.
 *
 *  Request a view of data residing on and managed by the "default"
 *  publisher, i.e. the first (or only) publisher in the client's
 *  binding table and request subsequent updates, at the discretion of
 *  the publisher in the case that the data changes. This version uses
 *  a path list to specify the data of interest.
 *
 *  @param [in]     aPathList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded path
 *                                      list representing the
 *                                      requested data.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @retval #WEAVE_NO_ERROR on success,
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY if a transaction couldn't be
 *  allocated.
 *
 *  @return Otherwise, a #WEAVE_ERROR reflecting the failure to
 *  initialize or start the transaction.
 */

WEAVE_ERROR DMClient::SubscribeRequest(ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Subscribe *subscribe = NewSubscribe();

    if (subscribe)
    {
        err = subscribe->Init(this, aPathList, aTxnId, aTimeout);
        SuccessOrExit(err);

        err = StartTransaction(subscribe);
    }

    else
    {
        err = WEAVE_ERROR_NO_MEMORY;
    }

exit:
    return err;
}

/**
 *  @brief
 *    Cancel a subscription.
 *
 *  Request the cancellation of a subscription from a given publisher,
 *  and remove the corresponding subscription from the local notifier
 *  table.
 *
 *  @note
 *    This method should be used to cancel a subscription that has
 *    been establish using SubscribeRequest(). To simply remove a
 *    subscription locally, use EndSubscription().
 *
 *  @sa EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId)
 *
 *  @param [in]     aDestinationId      A reference to the 64-bit node
 *                                      ID of the remote publisher to
 *                                      which the request is being
 *                                      sent.
 *
 *  @param [in]     aTopicId            A reference to a topic ID for
 *                                      this subscription. Note this
 *                                      this may be either the
 *                                      publisher-assigned "working"
 *                                      ID or the well-known topic ID
 *                                      associated with the original
 *                                      request.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @return #WEAVE_NO_ERROR on success or a #WEAVE_ERROR reflecting
 *  a failure to cancel the subscription.
 */

WEAVE_ERROR DMClient::CancelSubscriptionRequest(const uint64_t &aDestinationId, const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Binding *binding;

    if (HasSubscription(aTopicId, aDestinationId))
    {
        /*
         * we should get rid of the subscription here and not rely
         * on the publisher to respond before we get rid of it. this
         * may result in a status message with "unknown topic" also
         * being sent to the publisher, but it saves us from keeping
         * the subscription around indefinitely if the publisher
         * fails to respond or the packet gets lost.
         */

        sNotifier.RemoveSubscription(aTopicId, aDestinationId, this);

        binding = GetBinding(aDestinationId);

        if (binding)
        {
            CancelSubscription *cancel = NewCancelSubscription();

            if (cancel)
            {
                err = cancel->Init(this, aTopicId, aTxnId, aTimeout);

                if (err == WEAVE_NO_ERROR)
                    err = StartTransaction(cancel, binding);
            }

            else

            {
                err = WEAVE_ERROR_NO_MEMORY;
            }
        }

        else
        {
            err = WEAVE_ERROR_INCORRECT_STATE;
        }
    }

    /*
     * if the subscription didn't exist, the NHL may
     * still want a confirmation.
     */

    else
    {
        StatusReport s;

        s.init(kWeaveProfile_Common, kStatus_Success);

        err = CancelSubscriptionConfirm(aDestinationId, aTopicId, s, aTxnId);
    }

    return err;
}

/**
 *  @brief
 *    Cancel a subscription on the default publisher.
 *
 *  Request the cancellation of a subscription on the "default"
 *  publisher, i.e. the first (or only) publisher in the client's
 *  binding table and remove the corresponding subscription from the
 *  local notifier table.
 *
 *  @note
 *    This method should be used to cancel a subscription that has
 *    been establish using SubscribeRequest(). To simply remove a
 *    subscription locally, use EndSubscription().
 *
 *  @sa EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId)
 *
 *  @param [in]     aTopicId            A reference to a topic ID for
 *                                      this subscription. Note this
 *                                      this may be either the
 *                                      publisher-assigned "working"
 *                                      ID or the well-known topic ID
 *                                      associated with the original
 *                                      request.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      this subscribe operation.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the subscribe
 *                                      response.
 *
 *  @return #WEAVE_NO_ERROR on success or a #WEAVE_ERROR reflecting
 *  a failure to cancel the subscription.
 */

WEAVE_ERROR DMClient::CancelSubscriptionRequest(const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_INCORRECT_STATE;
    uint64_t &defaultPeer = mBindingTable[kDefaultBindingTableIndex].mPeerNodeId;

    if (defaultPeer != kNodeIdNotSpecified)
        err =  CancelSubscriptionRequest(defaultPeer, aTopicId, aTxnId, aTimeout);

    return err;
}

/**
 * @}
 */

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

/**
 *  @name
 *    Update methods.
 *
 *  @brief
 *    Update, under WDM, is used by a client to request a
 *    change to specified data managed by a publisher.
 *
 * See Detailed Description for discussion of WDM request method
 * signatures.
 *
 *  @{
 */

/**
 *  @brief
 *    Request an update to published data.
 *
 *  Request that a remote publisher update data under management.
 *
 *  @param [in]     aDestinationId      A reference to the 64-bit node
 *                                      ID of the remote publisher to
 *                                      which the request is being
 *                                      sent.
 *
 *  @param [in]     aDataList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded data
 *                                      list containing a
 *                                      representation of the update
 *                                      including the paths to which
 *                                      the update is to be applied.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      the update request.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the corresponding
 *                                      status report.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  an update transaction could not be allocated. Otherwise, return a
 *  #WEAVE_ERROR reflecting an update failure.
 */

WEAVE_ERROR DMClient::UpdateRequest(const uint64_t &aDestinationId, ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_INCORRECT_STATE;
    Binding *binding = GetBinding(aDestinationId);
    Update *update;

    if (binding)
    {
        update = NewUpdate();

        if (update)
        {
            err = update->Init(this, aDataList, aTxnId, aTimeout);
            SuccessOrExit(err);

            err = StartTransaction(update, binding);
        }

        else
        {
            err = WEAVE_ERROR_NO_MEMORY;
        }
    }

    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

exit:
    return err;
}

/*
 * if we're allowing old-style updates with the legacy message types,
 * e.g. for Amber, then this is how it works. the "real" method just
 * calls the compatibility method with the "use legacy msg type" flag
 * set to false. in order to use the compatibility mode, use the call
 * with the flag set to true.
 */

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

/**
 *  @brief
 *    Request an update to data on the default publisher.
 *
 *  Request that a remote publisher update data under management. This
 *  version directs the request to the publisher that is the target of
 *  the client's default binding.
 *
 *  @param [in]     aDataList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded data
 *                                      list containing a
 *                                      representation of the update
 *                                      including the paths to which
 *                                      the update is to be applied.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      the update request.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the corresponding
 *                                      status report.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  an update transaction could not be allocated. Otherwise, return a
 *  #WEAVE_ERROR reflecting an update failure.
 */

WEAVE_ERROR DMClient::UpdateRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout)
{
    return UpdateRequest(aDataList, aTxnId, aTimeout, false);
}


/**
 *  @brief
 *    Request an update to published data.
 *
 *  Request that a remote publisher update data under management. This
 *  version takes a boolean selector for legacy message types - see
 *  DMConstants.h
 *
 *  @param [in]     aDataList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded data
 *                                      list containing a
 *                                      representation of the update
 *                                      including the paths to which
 *                                      the update is to be applied.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      the update request.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the corresponding
 *                                      status report.
 *
 *  @param [in]     aUseLegacyMsgType   A control flag that should be
 *                                      true if the message is to be
 *                                      sent with the message type
 *                                      defined in the WDM spec. up
 *                                      through v1.2. Otherwise the
 *                                      message should be sent using
 *                                      the type defined in the WDM2.0
 *                                      spec and beyond.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  an update transaction could not be allocated. Otherwise, return a
 *  #WEAVE_ERROR reflecting an update failure.
 */

WEAVE_ERROR DMClient::UpdateRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout, bool aUseLegacyMsgType)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    Update *update = NewUpdate();

    if (update)
    {
        err = update->Init(this, aDataList, aTxnId, aTimeout);
        SuccessOrExit(err);

        update->mUseLegacyMsgType = aUseLegacyMsgType;

        err = StartTransaction(update);
    }

exit:
    return err;
}

/*
 * otherwise, there's no compatiblity method defined.
 */

#else // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

/**
 *  @brief
 *    Request an update to data on the default publisher.
 *
 *  Request that a remote publisher update data under management. This
 *  version directs the request to the publisher that is the target of
 *  the client's default binding.
 *
 *  @param [in]     aDataList           A reference to a
 *                                      ReferencedTLVData object
 *                                      containing a TLV-encoded data
 *                                      list containing a
 *                                      representation of the update
 *                                      including the paths to which
 *                                      the update is to be applied.
 *
 *  @param [in]     aTxnId              An identifier for the
 *                                      transaction set up to manage
 *                                      the update request.
 *
 *  @param [in]     aTimeout            A maximum time in milliseconds
 *                                      to wait for the corresponding
 *                                      status report.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  an update transaction could not be allocated. Otherwise, return a
 *  #WEAVE_ERROR reflecting an update failure.
 */

WEAVE_ERROR DMClient::UpdateRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    Update *update = NewUpdate();

    if (update)
    {
        err = update->Init(this, aDataList, aTxnId, aTimeout);
        SuccessOrExit(err);

        err = StartTransaction(update);
    }

    else
    {
        err = WEAVE_ERROR_NO_MEMORY;
    }

exit:
    return err;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

/**
 * @}
 */

/**
 *  @brief
 *    Request that an executing transaction be canceled.
 *    This method doesn't generate network traffic, but just release resources allocated for
 *    the specified transaction(s)
 *
 *  @param [in]     aTxnId              The number of the transaction
 *                                      to be canceled. If kTransactionIdNotSpecified is provided,
 *                                      all transactions would be canceled.
 *
 *  @return #WEAVE_NO_ERROR on success or a #WEAVE_ERROR reflecting
 *  a failure to cancel the transaction.
 */

WEAVE_ERROR DMClient::CancelTransactionRequest(uint16_t aTxnId, WEAVE_ERROR aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int i;

    if (aTxnId == kTransactionIdNotSpecified)
    {
        for (i = 0; i<kViewPoolSize; i++)
            mViewPool[i].Finalize();

        for (i = 0; i<kUpdatePoolSize; i++)
            mUpdatePool[i].Finalize();

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        for (i = 0; i<kSubscribePoolSize; i++)
            mSubscribePool[i].Finalize();

        for (i = 0; i<kCancelSubscriptionPoolSize; i++)
            mCancelSubscriptionPool[i].Finalize();

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    }

    else
    {
        for (i = 0; i<kViewPoolSize; i++)
        {
            View &v = mViewPool[i];

            if (!v.IsFree())
                VerifyOrExit(v.mTxnId != aTxnId, err = v.Finalize());
        }

        for (i = 0; i<kUpdatePoolSize; i++)
        {
            Update &u = mUpdatePool[i];

            if (!u.IsFree())
                VerifyOrExit(u.mTxnId != aTxnId, err = u.Finalize());
        }

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

        for (i = 0; i<kSubscribePoolSize; i++)
        {
            Subscribe &s = mSubscribePool[i];

            if (!s.IsFree())
                VerifyOrExit(s.mTxnId != aTxnId, err = s.Finalize());
        }

        for (i = 0; i<kCancelSubscriptionPoolSize; i++)
        {
            CancelSubscription &c = mCancelSubscriptionPool[i];

            if (!c.IsFree())
                VerifyOrExit(c.mTxnId != aTxnId, err = c.Finalize());
        }

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

    }

exit:
    return err;
}

/*
 * the following transaction-related methods are not a part of the
 * public interface to WDM and so are declared protected in DMClient.h
 */

WEAVE_ERROR DMClient::View::Init(DMClient *aClient, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    DMTransaction::Init(aClient, aTxnId, aTimeout);

    mPathList = aPathList;

    return WEAVE_NO_ERROR;
}

void DMClient::View::Free(void)
{
    DMTransaction::Free();

    mPathList.free();

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumViews);
}

WEAVE_ERROR DMClient::View::SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mExchangeCtx, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mPathList.pack(aBuffer);
    SuccessOrExit(err);

    err = mExchangeCtx->SendMessage(kWeaveProfile_WDM, kMsgType_ViewRequest, aBuffer, aSendFlags);
    aBuffer = NULL;

exit:
    if (aBuffer != NULL)
    {
        PacketBuffer::Free(aBuffer);
    }

    /*
     * and free the path list since we're done with it. note
     * that this ONLY does something substantive if the path
     * list has an PacketBuffer associated with it.
     */

    mPathList.free();

    return err;
}

WEAVE_ERROR DMClient::View::OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus)
{
    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    Finalize();

    return client->ViewConfirm(aResponderId, aStatus, txnId);
}

WEAVE_ERROR DMClient::View::OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WEAVE_ERROR appErr = WEAVE_NO_ERROR;

    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    ReferencedTLVData dataList;

    VerifyOrExit(aMsgType == kMsgType_ViewResponse, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    err = ReferencedTLVData::parse(aMsg, dataList);
    SuccessOrExit(err);

    Finalize();

    appErr = client->ViewConfirm(aResponderId, dataList, txnId);

    if (appErr != WEAVE_NO_ERROR)
        WeaveLogError(DataManagement, "DMClient::ViewConfirm => %s", ErrorStr(appErr));

exit:
    if (err != WEAVE_NO_ERROR)
        OnError(aResponderId, err);

    return err;
}

DMClient::View *DMClient::NewView(void)
{
    int i;
    View *result = NULL;

    for (i = 0; i < kViewPoolSize; i++)
    {
        if (mViewPool[i].IsFree())
        {
            result = &mViewPool[i];
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumViews);

            break;
        }
    }

    return result;
}

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

WEAVE_ERROR DMClient::Subscribe::Init(DMClient *aClient, const TopicIdentifier &aTopicId, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aTopicId == kTopicIdNotSpecified)
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    else
    {
        DMTransaction::Init(aClient, aTxnId, aTimeout);

        mPathList.free();
        mTopicId = aTopicId;
    }

    return err;
}

WEAVE_ERROR DMClient::Subscribe::Init(DMClient *aClient, ReferencedTLVData &aPathList, uint16_t aTxnId, uint32_t aTimeout)
{
    DMTransaction::Init(aClient, aTxnId, aTimeout);

    mTopicId = kTopicIdNotSpecified;
    mPathList = aPathList;

    return WEAVE_NO_ERROR;
}

void DMClient::Subscribe::Free(void)
{
    DMTransaction::Free();

    mTopicId = kTopicIdNotSpecified;
    mPathList.free();

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumSubscribes);
}

WEAVE_ERROR DMClient::Subscribe::SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    WEAVE_ERROR err;

    VerifyOrExit(mExchangeCtx, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        MessageIterator i(aBuffer);
        i.append();

        err = i.write64(mTopicId);
        SuccessOrExit(err);

        if (!mPathList.isEmpty())
        {
            err = mPathList.pack(i);
            SuccessOrExit(err);
        }
    }

    err = mExchangeCtx->SendMessage(kWeaveProfile_WDM, kMsgType_SubscribeRequest, aBuffer, aSendFlags);
    aBuffer = NULL;

exit:
    if (aBuffer != NULL)
    {
        PacketBuffer::Free(aBuffer);
    }

    /*
     * and free the path list since we're done with it. note that this
     * ONLY does something substantive if the path list has an
     * PacketBuffer associated with it.
     */

    mPathList.free();

    return err;
}

WEAVE_ERROR DMClient::Subscribe::OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus)
{
    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    Finalize();

    return client->SubscribeConfirm(aResponderId, aStatus, txnId);
}

WEAVE_ERROR DMClient::Subscribe::OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WEAVE_ERROR appErr = WEAVE_NO_ERROR;
    ReferencedTLVData dataList;
    TopicIdentifier topicId;

    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    {
        MessageIterator i(aMsg);

        VerifyOrExit(aMsgType == kMsgType_SubscribeResponse, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        err = i.read64(&topicId);
        SuccessOrExit(err);

        err = ReferencedTLVData::parse(i, dataList);
        SuccessOrExit(err);
    }

    /*
     * install the subscription here rather than burdening higher layers with it.
     */

    err = client->BeginSubscription(topicId, mTopicId, aResponderId);
    SuccessOrExit(err);

    /*
     * if the subscription was requested under a well-known topic
     * ID then pass that up to the client code rather than the
     * publisher-assigned one.
     */

    if (mTopicId != kTopicIdNotSpecified)
        topicId = mTopicId;

    Finalize();

    if (dataList.isEmpty())
        appErr = client->SubscribeConfirm(aResponderId, topicId, txnId);

    else
        appErr = client->SubscribeConfirm(aResponderId, topicId, dataList, txnId);

    if (appErr != WEAVE_NO_ERROR)
        WeaveLogError(DataManagement, "DMClient::SubscribeConfirm => %s", ErrorStr(appErr));

exit:
    if (err != WEAVE_NO_ERROR)
        OnError(aResponderId, err);

    return err;
}

DMClient::Subscribe *DMClient::NewSubscribe(void)
{
    int i;
    Subscribe *result = NULL;

    for (i = 0; i < kSubscribePoolSize; i++)
    {
        if (mSubscribePool[i].IsFree())
        {
            result = &mSubscribePool[i];
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumSubscribes);

            break;
        }
    }

    return result;
}

WEAVE_ERROR DMClient::CancelSubscription::Init(DMClient *aClient,
                                               const TopicIdentifier &aTopicId,
                                               uint16_t aTxnId,
                                               uint32_t aTimeout)
{
    WEAVE_ERROR err;

    err = DMTransaction::Init(aClient, aTxnId, aTimeout);
    SuccessOrExit(err);

    mTopicId = aTopicId;

exit:
    return err;
}

void DMClient::CancelSubscription::Free(void)
{
    DMTransaction::Free();

    mTopicId = kTopicIdNotSpecified;

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumCancels);
}

WEAVE_ERROR DMClient::CancelSubscription::SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    WEAVE_ERROR err;

    VerifyOrExit(mExchangeCtx, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        MessageIterator i(aBuffer);
        i.append();

        err = i.write64(mTopicId);
        SuccessOrExit(err);
    }

    err = mExchangeCtx->SendMessage(kWeaveProfile_WDM, kMsgType_CancelSubscriptionRequest, aBuffer, aSendFlags);
    aBuffer = NULL;

exit:
    if (aBuffer != NULL)
    {
        PacketBuffer::Free(aBuffer);
    }

    return err;
}

WEAVE_ERROR DMClient::CancelSubscription::OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus)
{
    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    Finalize();

    return client->CancelSubscriptionConfirm(aResponderId, mTopicId, aStatus, txnId);
}

DMClient::CancelSubscription *DMClient::NewCancelSubscription(void)
{
    int i;
    CancelSubscription *result = NULL;

    for (i = 0; i < kCancelSubscriptionPoolSize; i++)
    {
        if (mCancelSubscriptionPool[i].IsFree())
        {
            result = &mCancelSubscriptionPool[i];
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumCancels);

            break;
        }
    }

    return result;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

WEAVE_ERROR DMClient::Update::Init(DMClient *aClient,
                                   ReferencedTLVData &aDataList,
                                   uint16_t aTxnId,
                                   uint32_t aTimeout)
{
    DMTransaction::Init(aClient, aTxnId, aTimeout);

    mDataList = aDataList;

    return WEAVE_NO_ERROR;
}

void DMClient::Update::Free(void)
{
    DMTransaction::Free();

    mDataList.free();

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumUpdates);
}

WEAVE_ERROR DMClient::Update::SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    WEAVE_ERROR err;
    uint8_t msgType = kMsgType_UpdateRequest;

    VerifyOrExit(mExchangeCtx, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDataList.pack(aBuffer);
    SuccessOrExit(err);

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

    if (mUseLegacyMsgType)
        msgType = kMsgType_UpdateRequest_Deprecated;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

    err = mExchangeCtx->SendMessage(kWeaveProfile_WDM, msgType, aBuffer, aSendFlags);
    aBuffer = NULL;

exit:
    if (aBuffer != NULL)
    {
        PacketBuffer::Free(aBuffer);
    }

    /*
     * and free the data list since we're done with it. note
     * that this ONLY does something substantive if the data
     * list has an PacketBuffer associated with it.
     */

    mDataList.free();

    return err;
}

WEAVE_ERROR DMClient::Update::OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus)
{
    DMClient *client = static_cast<DMClient *>(mEngine);
    uint16_t txnId = mTxnId;

    Finalize();

    return client->UpdateConfirm(aResponderId, aStatus, txnId);
}

DMClient::Update *DMClient::NewUpdate(void)
{
    int i;
    Update *result = NULL;

    for (i = 0; i < kUpdatePoolSize; i++)
    {
        if (mUpdatePool[i].IsFree())
        {
            result = &mUpdatePool[i];
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumUpdates);

            break;
        }
    }

    return result;
}

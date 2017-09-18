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
 *    Implementations for the abstract DMPublisher base class.
 *
 *  This file contains implementations for the abstract DMPublisher
 *  base class, which should be used as the basis for
 *  application-specific publishers base on WDM. See the document,
 *  "Nest Weave-Data Management Protocol" document for a complete
 *  description.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

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

/*
 * This is the listener that's put in place when the publisher starts
 * up and which handles unsolicited View, Subscribe,
 * CancelSubscription and Update requests.
 */

static void PublisherListener(ExchangeContext *ec,
                              const IPPacketInfo *pktInfo,
                              const WeaveMessageInfo *msgInfo,
                              uint32_t profileId,
                              uint8_t msgType,
                              PacketBuffer *payload)
{
    DMPublisher *scribners = static_cast<DMPublisher*>(ec->AppState);

    scribners->OnMsgReceived(ec, profileId, msgType, payload);
}

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

/*
 *  This low-level handler is called on receipt of the ACK to a
 *  subscribe response frame and is used to activate the subscription
 *  in question since. This protects the client from receiving
 *  notifications before it is ready.
 */

static void SubscriptionSuccess(ExchangeContext *ec, void *aSubscription)
{
    DMPublisher::Subscription *s = static_cast<DMPublisher::Subscription*>(aSubscription);

    s->Activate();

    // Only close out the context once we get the ACK back for the subscription response.
    s->mSubscriptionCtx->Close();
    s->mSubscriptionCtx = NULL;
}

/*
 *  This low-level handler is called on failure to receive an ACK to a
 *  subscribe response frame. It cancels the subscription and logs an
 *  error
 */

static void SubscriptionFailure(ExchangeContext *ec, WEAVE_ERROR aError, void *aSubscription)
{
    DMPublisher::Subscription *s = static_cast<DMPublisher::Subscription*>(aSubscription);

    WeaveLogError(DataManagement,
                  "Subscription [0x%" PRIx64 ", 0x%" PRIx64 ", 0x%" PRIx64 "] failed - %s",
                  s->mAssignedId,
                  s->mRequestedId,
                  s->mClientId,
                  ErrorStr(aError));

    s->mSubscriptionCtx->Abort();
    s->Free();
}

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

/**
 *  @brief
 *    The default constructor for DMPublisher objects.
 *
 *  Clears all internal state.
 */

DMPublisher::DMPublisher(void) { Clear(); }

/**
 * @brief
 *   The destructor for DMPublisher objects.
 *
 *  Clears all internal state and removes the listener from the
 *  exchange manager if one is in place.
 */

DMPublisher::~DMPublisher(void) { Finalize(); }

/**
 *  @brief
 *    Initialize a DMPublisher object.
 *
 *  This method has the side effect of installing a listener in the
 *  exchange manager for the full range of client requests possibly
 *  including those for subscription.
 *
 *  @param [in]     aExchangeMgr     A pointer to the
 *                                   WeaveExchangeManager object to
 *                                   use for all exchanges in which
 *                                   the publisher wishes to
 *                                   participate.
 *
 *  @param [in]     aResponseTimeout A response timeout in
 *                                   milliseconds, i.e. the maximum
 *                                   time to wait for a response.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise, return a
 *  #WEAVE_ERROR reflecting a failure to properly set up the
 *  publisher.
 */

WEAVE_ERROR DMPublisher::Init(WeaveExchangeManager *aExchangeMgr, uint32_t aResponseTimeout)
{
    WEAVE_ERROR err;

    err = ProtocolEngine::Init(aExchangeMgr, aResponseTimeout);
    SuccessOrExit(err);

    err = aExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_ViewRequest, PublisherListener, this);
    SuccessOrExit(err);

    err = aExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_UpdateRequest, PublisherListener, this);
    SuccessOrExit(err);

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES

    err = aExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM,
                                                          kMsgType_UpdateRequest_Deprecated,
                                                          PublisherListener,
                                                          this);
    SuccessOrExit(err);

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    ClearSubscriptionTable();

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    err = aExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_SubscribeRequest, PublisherListener, this);
    SuccessOrExit(err);

    err = aExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_CancelSubscriptionRequest, PublisherListener, this);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 *  @brief
 *    Clear the internal state of DMPublisher object.
 *
 *  Clears the notify transaction pool and the subscription table.
 */

void DMPublisher::Clear(void)
{

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    int i;

    ClearSubscriptionTable();

    for (i = 0; i<kNotifyPoolSize; i++)
        mNotifyPool[i].Free();

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

}

/**
 *  @brief
 *    Shut down an operating DMPublisher.
 *
 *  Clears all the operating state and shuts down the listener if one
 *  is running.
 */

void DMPublisher::Finalize(void)
{
    if (mExchangeMgr)
    {
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_ViewRequest);
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_UpdateRequest);
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_SubscribeRequest);
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_CancelSubscriptionRequest);
    }

    Clear();

    ProtocolEngine::Finalize();
}

void DMPublisher::IncompleteIndication(Binding *aBinding, StatusReport &aReport)
{

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    FailSubscription(kTopicIdNotSpecified, aBinding->mPeerNodeId, aReport);

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

}

/**
 *  @brief
 *    Respond to a view request.
 *
 *  Send the response to a view request after processing, using the
 *  exchange context that was given in the indication.
 *
 *  @param [in]     aResponseCtx    A pointer to the exchange context
 *                                  under which the request was
 *                                  received.
 *
 *  @param [in]     aStatus         A reference to a StatusReport
 *                                  object containing information
 *                                  about the status of the
 *                                  request. In the case where this is
 *                                  success, the requestor will be
 *                                  expecting a data list containing
 *                                  the data of interest.
 *
 *  @param [in]     aDataList       A pointer to an optional
 *                                  ReferencedTLVData object
 *                                  containing a TLV-encoded data list
 *                                  containing the data of interest
 *                                  and the paths indicating the
 *                                  disposition of that data. Note
 *                                  that this parameter shall be NULL
 *                                  in the case where the status given
 *                                  in the previous parameter is not
 *                                  success.
 *
 *  @retval         #WEAVE_NO_ERROR On success. Otherwise, return a
 *                  #WEAVE_ERROR reflecting a failure to send the response message.
 *  @retval         #WEAVE_ERROR_INVALID_ARGUMENT If the given parameters are
 *                  inconsistent
 *  @retval         #WEAVE_ERROR_NO_MEMORY If an Inet buffer could not be
 *                  allocated.
 */

WEAVE_ERROR DMPublisher::ViewResponse(ExchangeContext *aResponseCtx, StatusReport &aStatus, ReferencedTLVData *aDataList)
{
    WEAVE_ERROR     err = WEAVE_NO_ERROR;
    PacketBuffer*   buf = NULL;

    VerifyOrExit((aDataList || !aStatus.success()), err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (aStatus.success())
    {
        buf = PacketBuffer::New();
        VerifyOrExit(buf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        err = aDataList->pack(buf);
        SuccessOrExit(err);

        err = aResponseCtx->SendMessage(kWeaveProfile_WDM, kMsgType_ViewResponse, buf);
        buf = NULL;
    }
    else
    {
        err = StatusResponse(aResponseCtx, aStatus);
    }

exit:
    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

/**
 *  @brief
 *    Respond to an update request.
 *
 *  Send the response to an update request after processing, using the
 *  exchange context that was given in the indication.
 *
 *  @param [in]     aResponseCtx    A pointer to the exchange context
 *                                  under which the request was
 *                                  received.
 *
 *  @param [in]     aStatus         A reference to a StatusReport
 *                                  object containing information
 *                                  about the status of the request.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting a failure to send the response message.
 */

WEAVE_ERROR DMPublisher::UpdateResponse(ExchangeContext *aResponseCtx, StatusReport &aStatus)
{
    return StatusResponse(aResponseCtx, aStatus);
}

/*
 * note that an exchange context is passed to the "request received"
 * method and is assumed to be passed through to the "send response"
 * methods. the NHL IS responsible for managing this exchange context.
 */

void DMPublisher::OnMsgReceived(ExchangeContext *aExchangeCtx, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MessageIterator i(aMsg);
    StatusReport status;

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    TopicIdentifier topicId;

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

    ReferencedTLVData pathList;
    ReferencedTLVData dataList;

    /*
     * in the case where NONE of the indication methods get called we
     * still have to close the context AND we should still send a
     * response of some sort.
     */

    bool sendRsp = false;

    if (aProfileId == kWeaveProfile_WDM)
    {
        switch (aMsgType)
        {
        case kMsgType_ViewRequest:

            err = ReferencedTLVData::parse(i, pathList);
            SuccessOrExit(err);

            err = ViewIndication(aExchangeCtx, pathList);

            break;

        case kMsgType_UpdateRequest:

            err = ReferencedTLVData::parse(i, dataList);
            SuccessOrExit(err);

            err = UpdateIndication(aExchangeCtx, dataList);

            break;

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES

        case kMsgType_UpdateRequest_Deprecated:

            err = ReferencedTLVData::parse(i, dataList);
            SuccessOrExit(err);

            err = UpdateIndication(aExchangeCtx, dataList);

            break;

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_LEGACY_MESSAGE_TYPES

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

        case kMsgType_SubscribeRequest:

            err = i.read64(&topicId);
            SuccessOrExit(err);

            if (topicId == kTopicIdNotSpecified)
            {
                err = ReferencedTLVData::parse(i, pathList);
                SuccessOrExit(err);

                err = SubscribeIndication(aExchangeCtx, pathList);
            }

            else
            {
                err = SubscribeIndication(aExchangeCtx, topicId);
            }

            break;

        case kMsgType_CancelSubscriptionRequest:

            err = i.read64(&topicId);
            SuccessOrExit(err);

            err = CancelSubscriptionIndication(aExchangeCtx, topicId);

            if (err == WEAVE_NO_ERROR)
            {
                status.init(kWeaveProfile_Common, kStatus_Success, NULL);
            }
            else
            {
                status.init(err);
            }

            sendRsp = true;

            break;

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

        default:

            err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

            status.init(kWeaveProfile_WDM, kStatus_UnsupportedMessage);

            sendRsp = true;

            break;
        }
    }
    else
    {
        err = WEAVE_ERROR_INVALID_PROFILE_ID;

        status.init(kWeaveProfile_WDM, kStatus_UnsupportedMessage);

        sendRsp = true;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        WeaveLogError(DataManagement, "OnMsgReceived() - %s", ErrorStr(err));

    if (sendRsp)
    {
        err = StatusResponse(aExchangeCtx, status);

        aExchangeCtx->Close();
    }

    PacketBuffer::Free(aMsg);
}

#if WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

/**
 *  @brief
 *    Start a subscription.
 *
 *  This method installs a subscription to a particular assigned
 *  ID/requested ID pair in the publisher's subscription table with
 *  the given client as the client requiring notification.
 *
 *  @param [in]     aTopicId        A reference to a well-known topic
 *                                  ID.
 *
 *  @param [in]     aClientId       A reference to the 64-bit node ID
 *                                  of the client.
 *
 *  @retval         #WEAVE_NO_ERROR On success.
 *  @retval         #WEAVE_ERROR_NO_MEMORY If a subscription could not be added.
 */

WEAVE_ERROR DMPublisher::BeginSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Subscription *s;

    s = AddSubscription(aTopicId, aTopicId, aClientId);
    VerifyOrExit(s, err = WEAVE_ERROR_NO_MEMORY);

    s->Activate();

exit:
    return err;
}

/**
 *  @brief
 *    Stop, and remove, a subscription.
 *
 *  Remove a subscription from the publisher's subscription table
 *  thereby stopping any future notifications from being delivered to
 *  the remote client.
 *
 *  @param [in]     aTopicId        A reference to a topic ID
 *                                  associated with the subscription.
 *                                  This may be the "working" ID or
 *                                  the requested ID.
 *
 *  @param [in]     aClientId       A reference to the 64-bit node ID
 *                                  of the client.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise returns a
 *  #WEAVE_ERROR reflecting a failure to remove the subscription.
 */

WEAVE_ERROR DMPublisher::EndSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    RemoveSubscription(aTopicId, aClientId);

    return err;
}

/**
 *  @brief
 *    Free all items in the subscription table.
 */

void DMPublisher::ClearSubscriptionTable(void)
{
    for (int i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        Subscription &s = mSubscriptionTable[i];

        s.Free();
    }
}

/**
 *  @brief
 *    Check that the subscription table is empty.
 *
 *  In particular, this means, check that all entries are free.
 *
 *  @return true if all entries are free, false otherwise.
 */

bool DMPublisher::SubscriptionTableEmpty(void) const
{
    bool result = true;

    for (int i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        const Subscription &s = mSubscriptionTable[i];

        if (s.mAssignedId != kTopicIdNotSpecified && s.mClientId != kNodeIdNotSpecified)
        {
            result = false;

            break;
        }
    }

    return result;

}

/**
 *  @brief
 *    Respond to a subscribe request.
 *
 *  Send the response to a subscribe request after processing, using
 *  the exchange context that was given in the indication. Invoking
 *  this method has the side effect of actually installing the
 *  subscription.
 *
 *  @param [in]     aResponseCtx    A pointer to the exchange context
 *                                  under which the request was
 *                                  received.
 *
 *  @param [in]     aStatus         A reference to a StatusReport
 *                                  object containing information
 *                                  about the status of the request.
 *                                  In the case where this is success,
 *                                  the requestor will be expecting a
 *                                  data list containing the data of
 *                                  interest.
 *
 *  @param [in]     aTopicId        A reference to the topic ID, if
 *                                  any, associated with the subscribe
 *                                  request. This parameter may have a
 *                                  value of kTopicIdNotSpecified in
 *                                  the case where the subscribe
 *                                  request did not contain a
 *                                  well-known topic ID.
 *
 *  @param [in]     aDataList       A pointer to an optional
 *                                  ReferencedTLVData object
 *                                  containing a TLV-encoded data list
 *                                  containing the data of interest
 *                                  and the paths indicating the
 *                                  disposition of that data. Note
 *                                  that this parameter shall be NULL
 *                                  in the case where the status given
 *                                  above is not success.
 *
 *  @retval         #WEAVE_NO_ERROR On success. Otherwise, return a
 *                  #WEAVE_ERROR reflecting a failure to send the response message.
 *  @retval         #WEAVE_ERROR_NO_MEMORY If an Inet buffer could not be allocated.
 *  @retval         #WEAVE_ERROR_INVALID_ARGUMENT If the given parameters are inconsistent.
 */

WEAVE_ERROR DMPublisher::SubscribeResponse(ExchangeContext *aResponseCtx,
                                           StatusReport &aStatus,
                                           const TopicIdentifier &aTopicId,
                                           ReferencedTLVData *aDataList)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   buf     = NULL;
    TopicIdentifier topic   = PublisherSpecificTopicId();
    Binding*        binding;
    Subscription*   subscription;

    VerifyOrExit((aDataList || !aStatus.success()), err = WEAVE_ERROR_INVALID_ARGUMENT);

    /*
     * generate a binding and add the subscription here but don't
     * activate it yet.
     */

    binding = FromExchangeCtx(aResponseCtx);
    VerifyOrExit(binding, err = WEAVE_ERROR_NO_MEMORY);

    subscription = AddSubscription(topic, aTopicId, binding->mPeerNodeId);
    VerifyOrExit(subscription, err = WEAVE_ERROR_NO_MEMORY);

    if (aStatus.success())
    {
        buf = PacketBuffer::New();
        VerifyOrExit(buf != NULL, err = WEAVE_ERROR_NO_MEMORY);
        {
            MessageIterator i(buf);
            i.append();

            err = i.write64(topic);
            SuccessOrExit(err);

            err = aDataList->pack(i);
            SuccessOrExit(err);
        }

        /*
         * what we do here depends on what transport we're
         * using. if it's raw UDP or, sadly, TCP we have no choice
         * but to just activate the subscription after we send the
         * response. if it's WRMP, on the other hand, we really
         * want to install the subscription here but only activate
         * it when ACK is received on the response.
         */
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        if (binding->mTransport == kTransport_WRMP)
        {
            aResponseCtx->OnAckRcvd = SubscriptionSuccess;
            aResponseCtx->OnSendError = SubscriptionFailure;

            subscription->mSubscriptionCtx = aResponseCtx;

            err = aResponseCtx->SendMessage(kWeaveProfile_WDM,
                                            kMsgType_SubscribeResponse,
                                            buf, ExchangeContext::kSendFlag_RequestAck,
                                            subscription);
            buf = NULL;
        }
        else
#endif
        {
            err = aResponseCtx->SendMessage(kWeaveProfile_WDM, kMsgType_SubscribeResponse, buf);
            buf = NULL;

            if (err == WEAVE_NO_ERROR)
            {
                subscription->Activate();
            }
            else
            {
                subscription->Free();
            }
        }
    }
    else
    {
        err = StatusResponse(aResponseCtx, aStatus);
    }

exit:
    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

/**
 *  @brief
 *    Cancel a subscription.
 *
 *  Cancel a subscription in response to the receipt of a cancel
 *  subscription request. This method doesn't generate further network
 *  traffic but simply remove the subscription record.
 *
 *  @note
 *    This virtual method simply handles the subscription
 *    cancellation itself and must be called by derived sub-classes that
 *    implement their own method.
 *
 *  @param [in]     aResponseCtx        A pointer to the exchange
 *                                      context under which the request
 *                                      was received.
 *
 *  @param [in]     aTopicId            A reference to the topic ID,
 *                                      if any, associated with the
 *                                      subscribe request.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting a failure to end the subscription.
 */

WEAVE_ERROR DMPublisher::CancelSubscriptionIndication(ExchangeContext *aResponseCtx, const TopicIdentifier &aTopicId)
{
    return EndSubscription(aTopicId, aResponseCtx->PeerNodeId);
}

/**
 *  @brief
 *    Request a notification.
 *
 *  Notify a specific remote client of changes to data of interest
 *  managed by this publisher.
 *
 *  @param [in]     aDestinationId  A reference to the 64-bit node ID
 *                                  of the remote client.
 *
 *  @param [in]     aTopicId        A reference to the topic ID
 *                                  associated with this subscription.
 *
 *  @param [in]     aDataList       A reference to a ReferencedTLVData
 *                                  object containing a TLV_encoded
 *                                  data list with the changed data as
 *                                  well as paths describing its
 *                                  disposition.
 *
 *  @param [in]     aTxnId          An identifier for the transaction
 *                                  set up to manage this notification.
 *
 *  @param [in]     aTimeout        A maximum time in milliseconds to
 *                                  wait for the notify response.
 *
 *  @retval         #WEAVE_NO_ERROR On success. Otherwise, return
 *                  a #WEAVE_ERROR describing a failure to initialize or start the
 *                  transaction.
 *  @retval         #WEAVE_ERROR_INVALID_ARGUMENT If the destination ID is not specified.
 *  @retval         #WEAVE_ERROR_NO_MEMORY If a new notify transaction cannot be allocated.
 */

WEAVE_ERROR DMPublisher::NotifyRequest(const uint64_t &aDestinationId,
                                       const TopicIdentifier &aTopicId,
                                       ReferencedTLVData &aDataList,
                                       uint16_t aTxnId,
                                       uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_ERROR_INCORRECT_STATE;
    Binding *binding = GetBinding(aDestinationId);
    Notify *notify = NULL;

    if (binding)
    {
        notify = NewNotify();

        if (notify)
        {
            err = notify->Init(this, aTopicId, aDataList, aTxnId, aTimeout);
            SuccessOrExit(err);

            err = StartTransaction(notify, binding);
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
 *    Request notifications based on topic.
 *
 *  Notify interested clients of changes to data of interest managed by
 *  this publisher. This version includes a specific topic identifier
 *  as a parameter.
 *
 *  @param [in]     aTopicId        A reference to the topic ID
 *                                  associated with this subscription.
 *
 *  @param [in]     aDataList       A reference to a ReferencedTLVData
 *                                  object containing a TLV_encoded
 *                                  data list containing the changed
 *                                  data as well as paths describing
 *                                  its disposition.
 *
 *  @param [in]     aTxnId          An identifier for the transaction
 *                                  set up to manage this notification.
 *
 *  @param [in]     aTimeout        A maximum time in milliseconds to
 *                                  wait for the notify response.
 *
 *  @retval         #WEAVE_NO_ERROR On success. Otherwise, return
 *                  a #WEAVE_ERROR describing a failure to initialize or start the
 *                  transaction.
 *  @retval         #WEAVE_ERROR_INVALID_ARGUMENT is the destination ID is not specified.
 *  @retval         #WEAVE_ERROR_NO_MEMORY If a new notify transaction cannot be allocated.
 */

WEAVE_ERROR DMPublisher::NotifyRequest(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    for (int i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        Subscription &s = mSubscriptionTable[i];

        if (s.IsActive())
        {
            if (s.mAssignedId == aTopicId || s.mRequestedId == aTopicId)
            {
                if (s.mClientId == kNodeIdNotSpecified)
                {
                    /*
                     * we leave open the possibility that someone has put in
                     * an entry with an unspecified client and intends to use
                     * a default binding.
                     */

                    Notify *notify = NewNotify();

                    if (notify)
                    {
                        err = notify->Init(this, aTopicId, aDataList, aTxnId, aTimeout);

                        if (err == WEAVE_NO_ERROR)
                            err = StartTransaction(notify);
                    }

                    else

                    {
                        err = WEAVE_ERROR_NO_MEMORY;
                    }
                }

                else
                {
                    err = NotifyRequest(s.mClientId, s.mAssignedId, aDataList, aTxnId, aTimeout);
                }
            }
        }

        if (err != WEAVE_NO_ERROR)
            break;
    }

    return err;
}

/**
 *  @brief
 *    Request notifications for changed data.
 *
 *  Notify clients of changes to data of interest managed by this
 *  publisher.
 *
 *  @warning Not implemented.
 *
 *  @param [in]     aDataList       A reference to a ReferencedTLVData
 *                                  object containing a TLV_encoded
 *                                  data list with the changed data as
 *                                  well as paths describing its
 *                                  disposition.
 *
 *  @param [in]     aTxnId          An identifier for the transaction
 *                                  set up to manage this notification.
 *
 *  @param [in]     aTimeout        A maximum time in milliseconds to
 *                                  wait for the notify response.
 *
 *  @retval         #WEAVE_NO_ERROR On success. Otherwise, return a
 *                  #WEAVE_ERROR describing a failure to initialize or start the
 *                  transaction.
 *  @retval         #WEAVE_ERROR_INVALID_ARGUMENT is the destination ID is not specified.
 *  @retval         #WEAVE_ERROR_NO_MEMORY if a new notify transaction cannot be allocated.
 *
 */

WEAVE_ERROR DMPublisher::NotifyRequest(ReferencedTLVData &aDataList, uint16_t aTxnId, uint32_t aTimeout)
{
    // jira://WEAV-265 has been created to track this
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

/**
 *  @brief
 *    Handle a notify confirm.
 *
 *  @note
 *    In the case of an unknown topic, which indicates that the
 *    notification has been successfully delivered but did not match any
 *    topic of interest on the client, the subscription is automatically
 *    revoked. IN ANY other error case, the subscription remains in place
 *    and it is the responsibility of the NHL to revoke it if so desired.
 *
 *    This virtual method does the subscription-based crank-turning
 *    on behalf of higher layers and must be invoked by higher-layer
 *    handlers in order to maintain the subscription table.
 *
 *  @param [in]     aResponderId    A reference to the 64-bit node ID
 *                                  of the remote client that sent the
 *                                  confirmation.
 *
 *  @param [in]     aAssignedId     A reference to the publisher-
 *                                  assigned "working" topic ID for
 *                                  this subscription.
 *
 *  @param [in]     aStatus         A reference to a StatusReport
 *                                  object containing the reported
 *                                  status of the
 *                                  notification. Specifically, there
 *                                  are two possible values here:
 *
 *                                     1) kStatus_Success, or
 *
 *                                     2) kStatus_UnknownTopic.
 *
 *                                  In the latter case, the
 *                                  subscription should be removed.
 *
 *  @param [in]     aTxnId          An identifier for the transaction
 *                                  set up to manage the original
 *                                  notification.
 *
 *  @return         #WEAVE_NO_ERROR On success. Otherwise, a #WEAVE_ERROR reflecting the
 *                  failure to end the subscription in the case where this is called for.
 */

WEAVE_ERROR DMPublisher::NotifyConfirm(const uint64_t &aResponderId,
                                       const TopicIdentifier &aAssignedId,
                                       StatusReport &aStatus,
                                       uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aStatus.mProfileId == kWeaveProfile_WDM && aStatus.mStatusCode == kStatus_UnknownTopic)
        FailSubscription(aAssignedId, aResponderId, aStatus);

    /*
     * we could get an error status here, e.g. in the case of a
     * timeout. in this case, return the given error.
     */

    else if (aStatus.mProfileId == kWeaveProfile_Common && aStatus.mStatusCode == kStatus_InternalError)
        err = aStatus.mError;

    return err;
}

/*
 * these are methods for the inner Subscription class and, as such,
 * are not part of the DMPublisher public interface.
 */

DMPublisher::Subscription::Subscription(void)
{
    Free();
}

DMPublisher::Subscription::~Subscription(void)
{
    Free();
}

WEAVE_ERROR DMPublisher::Subscription::Init(const TopicIdentifier &aAssignedId,
                                            const TopicIdentifier &aRequestedId,
                                            const uint64_t &aClientId)
{
    mFlags = kSubscriptionFlags_Allocated;
    mAssignedId = aAssignedId;
    mRequestedId = aRequestedId;
    mClientId = aClientId;

    return WEAVE_NO_ERROR;
}

void DMPublisher::Subscription::Free(void)
{
    mFlags = kSubscriptionFlags_Free;
    mAssignedId = kTopicIdNotSpecified;
    mRequestedId = kTopicIdNotSpecified;
    mClientId = kNodeIdNotSpecified;
    mSubscriptionCtx = NULL;
}

DMPublisher::Subscription *DMPublisher::AddSubscription(const TopicIdentifier &aAssignedId,
                                                        const TopicIdentifier &aRequestedId,
                                                        const uint64_t &aClientId)
{
    int i;
    Subscription *result = NULL;

    // look to see if it's already there.

    for (i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        Subscription &s = mSubscriptionTable[i];

        if (s.mAssignedId == aAssignedId && s.mRequestedId == aRequestedId &&s.mClientId == aClientId)
        {
            result = &s;
            result->mSubscriptionCtx = NULL;
            break;
        }
    }

    // and if not, add it.

    if (!result)
    {
        for (i = 0; i < kSubscriptionMgrTableSize; i++)
        {
            Subscription &s = mSubscriptionTable[i];

            if (s.IsFree())
            {
                s.Init(aAssignedId, aRequestedId, aClientId);

                result = &s;
                result->mSubscriptionCtx = NULL;
                break;
            }
        }
    }

    return result;
}

void DMPublisher::RemoveSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId)
{
    for (int i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        Subscription &s = mSubscriptionTable[i];

        if (s.MatchSubscription(aTopicId, aClientId))
            s.Free();
    }
}

void DMPublisher::FailSubscription(const TopicIdentifier &aTopicId, const uint64_t &aClientId, StatusReport &aReport)
{
    for (int i = 0; i < kSubscriptionMgrTableSize; i++)
    {
        Subscription &s = mSubscriptionTable[i];

        if (s.MatchSubscription(aTopicId, aClientId))
        {
            UnsubscribeIndication(s.mClientId, s.mAssignedId, aReport);

            s.Free();
        }
    }
}

/*
 * transaction-related methods are not part of the DMPublisher public
 * interface.
 */

WEAVE_ERROR DMPublisher::Notify::Init(DMPublisher *aPublisher,
                                      const TopicIdentifier &aTopicId,
                                      ReferencedTLVData &aDataList,
                                      uint16_t aTxnId,
                                      uint32_t aTimeout)
{
    DMTransaction::Init(aPublisher, aTxnId, aTimeout);

    mDataList = aDataList;
    mTopicId = aTopicId;

    return WEAVE_NO_ERROR;
}

void DMPublisher::Notify::Free(void)
{
    DMTransaction::Free();

    mDataList.free();
    mTopicId = kTopicIdNotSpecified;
}

WEAVE_ERROR DMPublisher::Notify::SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags)
{
    WEAVE_ERROR err;
    StatusReport report;

    VerifyOrExit(mExchangeCtx, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        MessageIterator i(aBuffer);
        i.append();

        err = i.write64(mTopicId);
        SuccessOrExit(err);

        err = mDataList.pack(i);
        SuccessOrExit(err);
    }

    err = mExchangeCtx->SendMessage(kWeaveProfile_WDM, kMsgType_NotifyRequest, aBuffer, aSendFlags);
    aBuffer = NULL;

exit:
    if (aBuffer != NULL)
    {
        PacketBuffer::Free(aBuffer);
    }

    /*
     * free the data list since we're done with it. note
     * that this ONLY does something substantive if the
     * list has an PacketBuffer associated with it.
     */

    mDataList.free();

    return err;
}

WEAVE_ERROR DMPublisher::Notify::OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus)
{
    DMPublisher *publisher = static_cast<DMPublisher *>(mEngine);
    uint16_t txnId = mTxnId;
    TopicIdentifier topicId = mTopicId;

    Finalize();

    return publisher->NotifyConfirm(aResponderId, topicId, aStatus, txnId);
}

DMPublisher::Notify *DMPublisher::NewNotify(void)
{
    int i;
    Notify *result = NULL;

    for (i = 0; i < kNotifyPoolSize; i++)
    {
        if (mNotifyPool[i].IsFree())
        {
            result = &mNotifyPool[i];

            break;
        }
    }

    return result;
}

/**
 *  @brief
 *    Request that an executing transaction be canceled.
 *
 *  @note
 *    The only transactions of interest in a publisher at this
 *    point are subscription/notification related so this code only
 *    compiles if subscriptions are allowed.
 *
 *  @sa #WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION
 *
 *  @param [in]     aTxnId          The number of the transaction to
 *                                  be canceled.
 *
 *  @parame [in]    aError          The #WEAVE_ERROR to report when
 *                                  canceling the transaction.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise a #WEAVE_ERROR reflecting
 *  a failure to cancel the transaction.
 */

WEAVE_ERROR DMPublisher::CancelTransactionRequest(uint16_t aTxnId, WEAVE_ERROR aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int i;

    for (i = 0; i < kNotifyPoolSize; i++)
    {
        Notify &n = mNotifyPool[i];

        VerifyOrExit(n.mTxnId != aTxnId, err = n.Finalize());
    }

exit:
    return err;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_PUBLISHER_SUBSCRIPTION

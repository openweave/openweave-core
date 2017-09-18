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
 *    Implementations for the ClientNotifier auxiliary class.
 *
 *  This file provides implementations for the ClientNotifier auxiliary
 *  class, which is a employed when support for subscription and
 *  notification is desired for a WDM client. See the document, "Nest
 *  Weave-Data Management Protocol" document for a complete
 *  description.
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveConfig.h>

#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
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
 *  This is the listener or unsolicited message handler installed in
 *  the Weave exchange manager when the client wishes to receive
 *  notifications. Once installed, it is called whenever a WDM
 *  notification is received.
 */

static void ClientListener(ExchangeContext *ec,
                           const IPPacketInfo *pktInfo,
                           const WeaveMessageInfo *msgInfo,
                           uint32_t profileId,
                           uint8_t msgType,
                           PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ClientNotifier *n = static_cast<ClientNotifier*>(ec->AppState);

    if (msgType == kMsgType_NotifyRequest)
        err = n->DispatchNotifyIndication(ec, payload);

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

    if (err != WEAVE_NO_ERROR)
        WeaveLogError(DataManagement, "ClientListener() - %s", ErrorStr(err));

    PacketBuffer::Free(payload);
}

/**
 *  @brief
 *    The default constructor for the Subscription object used by the
 *    ClientNotifier.
 *
 *  @note
 *    Subscriptions must be initialize using Init() before use.
 */

ClientNotifier::Subscription::Subscription(void)
{
    Free();
}

/**
 *  @brief
 *    The destructor for the Subscription object used by the
 *    ClientNotifier.
 */

ClientNotifier::Subscription::~Subscription(void) { Free(); }

/**
 *  @brief
 *    Initialize a Subscription object.
 *
 *  This basically sets up the state required for a subscription to
 *  operate. In particular, a subscription must have a client to refer
 *  back to, at least one specified topic ID and a concrete publisher.
 *
 *  @note
 *    Subscriptions in WDM have a number of forms. They may be
 *    unilateral subscriptions to broadcast notifications from a
 *    publisher and identified with a well-known topic identifier, or
 *    they may be subscriptions to unicast notifications, which
 *    require a request/response protocol to establish. The latter
 *    form may again be established using a well-know topic ID or by
 *    including an arbitrary list of paths in the subscribe
 *    request. See the WDM specification for further details.
 *
 *  @param [in]     aAssignedId     A reference to the publisher-
 *                                  assigned "working" topic ID for
 *                                  this subscription. This may be
 *                                  unspecified if the subscription is
 *                                  being made unilaterally.
 *
 *  @param [in]     aRequestedId    A reference to the well-known
 *                                  topic ID, if any, under which the
 *                                  subscription was requested
 *
 *  @param [in]     aPublisherId    A reference to the 64-bit node ID
 *                                  of the publisher. In the case
 *                                  where the subscription is to the
 *                                  Weave service, the WDM service
 *                                  endpoint ID may appear here.
 *
 *  @param [in]     aClient         A pointer to the DMClient on
 *                                  behalf of which the subscription
 *                                  is being made. In particular this
 *                                  object should be a concrete
 *                                  subclass of DMClient containing
 *                                  implementations of the relevant
 *                                  indication methods.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If one or more of the
 *  arguments has an invalid value.
 */

WEAVE_ERROR ClientNotifier::Subscription::Init(const TopicIdentifier &aAssignedId,
                                               const TopicIdentifier &aRequestedId,
                                               const uint64_t &aPublisherId,
                                               DMClient *aClient)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((aAssignedId != kTopicIdNotSpecified || aRequestedId != kTopicIdNotSpecified) &&
        aPublisherId != kNodeIdNotSpecified &&
        aClient != NULL)
    {
        mAssignedId = aAssignedId;
        mRequestedId = aRequestedId;
        mPublisherId = aPublisherId;
        mClient = aClient;
    }

    else
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    return err;
}

/**
 *  @brief
 *    Free a client-side subscription.
 *
 *  This method simply blows away the subscription-related state.
 */

void ClientNotifier::Subscription::Free(void)
{
    mAssignedId = kTopicIdNotSpecified;
    mRequestedId = kTopicIdNotSpecified;
    mPublisherId = kNodeIdNotSpecified;
    mClient = NULL;
}

/**
 *  @brief
 *    The default constructor for a client notifier.
 *
 *  @note
 *    No further initialization is required before use.
 */

ClientNotifier::ClientNotifier(void)
{
    Clear();
}

/**
 *  @brief
 *    The destructor for a client notifier.
 *
 *  The destructor attempts to disable subscription on the theory that,
 *  if the notifier is going away, we don't want a listener running
 *  either.
 */

ClientNotifier::~ClientNotifier(void)
{
    if (mExchangeMgr)
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_NotifyRequest);

    Clear();
}

/**
 *  @brief
 *    Deliver a WDM notification message to the appropriate client.
 *
 *  When a notification arrives, this method checks the notifier
 *  subscription table, looks up the client for which the message is
 *  destined, and delivers the message by calling the relevant
 *  indication method. If no relevant subscription is found, it sends
 *  back a status report to that effect.
 *
 *  @param [in]     aResponseCtx    A pointer to the exchange context
 *                                  under which the message was
 *                                  delivered and which may be used to
 *                                  respond.
 *
 *  @parm [in]      payload         A pointer to an PacketBuffer
 *                                  containing the notification message
 *                                  itself.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR associated with a failure to process or deliver the
 *  message.
 */

WEAVE_ERROR ClientNotifier::DispatchNotifyIndication(ExchangeContext *aResponseCtx, PacketBuffer *payload)
{
    WEAVE_ERROR err;
    MessageIterator it(payload);
    bool indicated = false;

    TopicIdentifier topicId;
    ReferencedTLVData dataList;

    uint64_t peerId = aResponseCtx->PeerNodeId;

    StatusReport report;

    WeaveLogProgress(DataManagement, "ClientNotifier::DispatchNotifyIndication()");

    err = it.read64(&topicId);
    SuccessOrExit(err);

    WeaveLogProgress(DataManagement, " - topicId = 0x%" PRIx64, topicId);

    err = ReferencedTLVData::parse(it, dataList);
    SuccessOrExit(err);

    for (int i = 0; i < kNotifierTableSize; i++)
    {
        Subscription &s = mNotifierTable[i];

        if (s.CheckSubscription(topicId, peerId))
        {
            indicated = true;

            WeaveLogProgress(DataManagement, " - informing client");

            err = s.mClient->NotifyIndication(topicId, dataList);
        }
    }

    /*
     * if we found a subscription here AND the subscription was
     * explicit (we can tell because the publisher supplied and is
     * using a working topic ID), we should tell the subscription
     * manager on the other side that SOMETHING is still live over
     * here. otherwise we should inform them that the subscription has
     * magically vanished.
     */

    if (IsPublisherSpecific(topicId))
    {
        if (indicated)
        {
            report.init(kWeaveProfile_Common, kStatus_Success);
        }

        else
        {
            err = WEAVE_ERROR_UNKNOWN_TOPIC;

            report.init(kWeaveProfile_WDM, kStatus_UnknownTopic);
        }

        SendStatusReport(aResponseCtx, report);
    }

exit:
    /*
     * in this case, since we're not passing the context on to the
     * NHL, we close it.
     */

    aResponseCtx->Close();

    WeaveLogProgress(DataManagement, "ClientNotifier::DispatchNotifyIndication() => %s", ErrorStr(err));

    return err;
}

/**
 *  @brief
 *    Check if a notifier has a particular subscription.
 *
 *  This method scans the subscription table looking for a subscription
 *  that matches the given parameters (see MatchSubscription()).
 *
 *  @param [in]     aTopicId        A reference to a topic ID to look
 *                                  for - may have the wild-card value
 *                                  kTopicIdNotSpecified.
 *
 *  @param [in]     aPublisherId    A reference to a 64-bit publisher
 *                                  ID or service endpoint to look for
 *                                  - may have the wild-card value
 *                                  kNodeIdNotSpecified or the
 *                                  broadcast value kAnyNodeId.
 *
 *  @param [in]     aClient         A pointer to the DMClient on behalf
 *                                  of which the subscription is in use.
 *
 *  @return True if a match is found. Otherwise return false.
 */

bool ClientNotifier::HasSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient) const
{
    bool result = false;

    for (int i = 0; i < kNotifierTableSize; i++)
    {
        const Subscription &s = mNotifierTable[i];

        if (s.MatchSubscription(aTopicId, aPublisherId, aClient))
        {
            result = true;

            break;
        }
    }

    return result;
}

/**
 *  @brief
 *    Install a subscription in the table.
 *
 *  This method checks whether a subscription with the given parameters
 *  is already available in the subscription table and, if not,
 *  installs one.
 *
 *  @param [in]     aTopicId            A reference to the
 *                                      publisher-assigned "working"
 *                                      topic ID for this
 *                                      subscription.
 *
 *  @param [in]     aRequestedId        A reference to the well-known
 *                                      topic ID, if any, under which
 *                                      the subscription was
 *                                      requested. If the subscription
 *                                      was not requested in this way
 *                                      then the value may be
 *                                      kTopicIdNotSpecified.
 *
 *  @param [in]     aPublisherId        A reference to the 64-bit node
 *                                      ID or service endpoint ID of
 *                                      the publisher to which the
 *                                      subscription applies.
 *
 *  @param [in]     aClient             A pointer to the DMClient that
 *                                      requested the subscription.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If no subscription could be allocated.
 *
 *  @return Otherwise, an error reflecting the inability to install a
 *  subscription.
 */

WEAVE_ERROR ClientNotifier::InstallSubscription(const TopicIdentifier &aTopicId,
                                                const TopicIdentifier &aRequestedId,
                                                const uint64_t &aPublisherId,
                                                DMClient *aClient)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;

    /*
     * if the subscription already exists then there's nothing else to
     * do.
     */

    if (HasSubscription(aTopicId, aPublisherId, aClient))
    {
        err = WEAVE_NO_ERROR;
    }

    /*
     * otherwise, there exists the possiblity that a subscription with
     * the correct requested topic ID has been installed but the
     * working topic ID has not been assigned. if so, assign it.
     */

    else if (HasSubscription(aRequestedId, aPublisherId, aClient))
    {
        for (int i = 0; i < kNotifierTableSize; i++)
        {
            Subscription &s = mNotifierTable[i];

            if (s.MatchSubscription(aRequestedId, aPublisherId))
            {
                err = s.Init(aTopicId, aRequestedId, aPublisherId, aClient);

                break;
            }
        }
    }

    /*
     * otherwise just look for a free slot.
     */

    else
    {
        for (int i = 0; i < kNotifierTableSize; i++)
        {
            Subscription &s = mNotifierTable[i];

            if (s.IsFree())
            {
                /*
                 * deal with counting and start up the listener if
                 * this is the first installation.
                 */

                mSubscriptionCount++;

                if (mSubscriptionCount == 1)
                {

                    /*
                     * this is basically a hack pput here to try and
                     * get everything to build. NONE of this code
                     * should be lying around if subscription isn't
                     * allowed.
                     */

                    mExchangeMgr = aClient->mExchangeMgr;

                    err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_NotifyRequest, ClientListener, this);
                    SuccessOrExit(err);
                }

                err = s.Init(aTopicId, aRequestedId, aPublisherId, aClient);

                break;
            }
        }
    }

exit:

    return err;
}

/**
 *  @brief
 *    Remove a subscription for the table.
 *
 *  If a subscription with the given parameters exists in the
 *  subscription table, it is removed.
 *
 *  @param [in]     aTopicId        A reference to a topic ID that
 *                                  applies to this subscription. This
 *                                  may be a publisher-assigned
 *                                  "working" ID or the well-known ID
 *                                  under which the subscription was
 *                                  requested.
 *
 *  @param [in]     aPublisherId    A reference to the 64-bit node ID
 *                                  or service endpoint ID of the
 *                                  publisher to which the
 *                                  subscription applies.
 *
 *  @param [in]     aClient         A pointer to the client that requested
 *                                  the subscription.
 */

void ClientNotifier::RemoveSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient)
{
    for (int i = 0; i < kNotifierTableSize; i++)
    {
        Subscription &s = mNotifierTable[i];

        if (s.MatchSubscription(aTopicId, aPublisherId, aClient))
        {
            /*
             * deal with counting and stop the listener if this is the
             * last subscription.
             */

            if (mSubscriptionCount > 0)
                mSubscriptionCount--;

            if (mSubscriptionCount == 0)
                mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_NotifyRequest);

            s.Free();
        }
    }
}

/**
 *  @brief
 *    Remove a subscription for the table and tell the next higher layer.
 *
 *  If a subscription with the given parameters exists in the
 *  subscription table, it is removed, and the UnsubscribeIndication()
 *  method on the client is called.
 *
 *  @param [in]     aTopicId        A reference to a topic ID that
 *                                  applies to this subscription. This
 *                                  may be a publisher-assigned
 *                                  "working" ID or the well-known ID
 *                                  under which the subscription was
 *                                  requested.
 *
 *  @param [in]     aPublisherId    A reference to the 64-bit node ID
 *                                  or service endpoint ID of the
 *                                  publisher to which the
 *                                  subscription applies.
 *
 *  @param [in]     aClient         A pointer to the DMClient that
 *                                  requested the subscription.
 *
 *  @param [in]     aReport         A reference to a status report
 *                                  describing the reason for failure.
 */

void ClientNotifier::FailSubscription(const TopicIdentifier &aTopicId, const uint64_t &aPublisherId, DMClient *aClient, StatusReport &aReport)
{
    for (int i = 0; i < kNotifierTableSize; i++)
    {
        Subscription &s = mNotifierTable[i];

        if (s.MatchSubscription(aTopicId, aPublisherId, aClient))
        {
            /*
             * deal with counting and stop the listener if this is the
             * last subscription.
             */

            if (mSubscriptionCount > 0)
                mSubscriptionCount--;

            if (mSubscriptionCount == 0)
                mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_WDM, kMsgType_NotifyRequest);

            aClient->UnsubscribeIndication(s.mPublisherId, s.mAssignedId, aReport);

            s.Free();
        }
    }
}

/**
 *  @brief
 *    Clear client notifier state.
 *
 *  This method just clears the notifier working state without call
 *  any indications to higher layers.
 */

void ClientNotifier::Clear(void)
{
    mSubscriptionCount = 0;

    for (int i = 0; i < kNotifierTableSize; i++)
    {
        Subscription &s = mNotifierTable[i];

        s.Free();
    }

    mExchangeMgr = NULL;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file contains implementation of the TimeSyncClient class used in Time Services
 *      WEAVE_CONFIG_TIME must be defined if Time Services are needed
 *
 */

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX and INT32_MAX to be defined for pre-C++11 clib
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

// __STDC_CONSTANT_MACROS must be defined for INT64_C and UINT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

// it is important for this first inclusion of stdint.h to have all the right switches turned ON
#include <stdint.h>

// __STDC_FORMAT_MACROS must be defined for PRIX64 to be defined for pre-C++11 clib
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

// it is important for this first inclusion of inttypes.h to have all the right switches turned ON
#include <inttypes.h>

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#if WEAVE_CONFIG_TIME
#if WEAVE_CONFIG_TIME_ENABLE_CLIENT

using namespace nl::Weave::Profiles::Time;
using nl::Weave::ExchangeContext;
using nl::Weave::Binding;

WEAVE_ERROR TimeSyncNode::InitClient(void * const aApp, WeaveExchangeManager *aExchangeMgr,
    const uint8_t aEncryptionType, const uint16_t aKeyId
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    , const int8_t aInitialLikelyhood
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // initialize general data
    err = InitState(kTimeSyncRole_Client, aApp, aExchangeMgr);
    SuccessOrExit(err);

    // initialize Client-specific data
    err = _InitClient(aEncryptionType, aKeyId
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        , aInitialLikelyhood
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncNode::_InitClient(const uint8_t aEncryptionType,
    const uint16_t aKeyId
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    , const int8_t aInitialLikelyhood
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    InvalidateAllContacts();

    mEncryptionType = aEncryptionType;
    mKeyId = aKeyId;

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    if ((aInitialLikelyhood < TimeSyncRequest::kLikelihoodForResponse_Min)
        || (aInitialLikelyhood > TimeSyncRequest::kLikelihoodForResponse_Max))
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    else
    {
        mLastLikelihoodSent = aInitialLikelyhood;
    }
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    // Register to receive unsolicited time sync request advisory messages from the exchange manager.
    err = GetExchangeMgr()->RegisterUnsolicitedMessageHandler(kWeaveProfile_Time,
        kTimeMessageType_TimeSyncTimeChangeNotification,
        HandleTimeChangeNotification, this);
    SuccessOrExit(err);

    mActiveContact = NULL;
    mExchageContext = NULL;
    mUnadjTimestampLastSent_usec = 0;

exit:
    WeaveLogFunctError(err);
    SetClientState((WEAVE_NO_ERROR == err) ? kClientState_Idle : kClientState_InitializationFailed);

    return err;
}

WEAVE_ERROR TimeSyncNode::_ShutdownClient()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    (void)Abort();

    // unregister message handler
    err = GetExchangeMgr()->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Time,
        kTimeMessageType_TimeSyncTimeChangeNotification);

exit:
    WeaveLogFunctError(err);
    SetClientState((WEAVE_NO_ERROR == err) ? kClientState_ShutdownCompleted : kClientState_ShutdownFailed);

    return err;
}

void TimeSyncNode::AbortOnError(const WEAVE_ERROR aCode)
{
    if (WEAVE_NO_ERROR == aCode)
    {
        // do nothing
    }
    else
    {
        if (NULL != OnSyncFailed)
        {
            mIsInCallback = true;
            OnSyncFailed(mApp, aCode);
            mIsInCallback = false;
        }

        Abort();
    }
}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
int8_t TimeSyncNode::GetNextLikelihood(void) const

{
    return mLastLikelihoodSent;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

const char * const TimeSyncNode::GetClientStateName() const
{
    const char * stateName = NULL;

    switch (mClientState)
    {
    case kClientState_Uninitialized:
        stateName = "Uninitialized";
        break;
    case kClientState_ContructionFailed:
        stateName = "ContructionFailed";
        break;
    case kClientState_Constructed:
        stateName = "Constructed";
        break;
    case kClientState_InitializationFailed:
        stateName = "InitializationFailed";
        break;
    case kClientState_Idle:
        stateName = "Idle";
        break;
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    case kClientState_Sync_Discovery:
        stateName = "Sync_Discovery";
        break;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    case kClientState_Sync_1:
        stateName = "Sync_1";
        break;
    case kClientState_Sync_2:
        stateName = "Sync_2";
        break;

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    case kClientState_ServiceSync_1:
        stateName = "ServiceSync_1";
        break;
    case kClientState_ServiceSync_2:
        stateName = "ServiceSync_2";
        break;
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    case kClientState_ShutdownNeeded:
        stateName = "ShutdownNeeded";
        break;
    case kClientState_ShutdownCompleted:
        stateName = "ShutdownCompleted";
        break;
    case kClientState_ShutdownFailed:
        stateName = "ShutdownFailed";
        break;
    default:
        stateName = "unnamed";
        break;
    }

    return stateName;
}

void TimeSyncNode::SetClientState(const TimeSyncNode::ClientState state)
{
    mClientState = state;

    WeaveLogDetail(TimeService, "Client entering state %d (%s)", mClientState,
        GetClientStateName());
}

TimeSyncNode::ClientState TimeSyncNode::GetClientState(void) const
{
    return mClientState;
}

int TimeSyncNode::GetCapacityOfContactList(void) const
{
    return int(WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS);
}

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
void TimeSyncNode::InvalidateServiceContact(void)
{
    mServiceContact.mCommState = uint8_t(kCommState_Invalid);
    mServiceContact.mResponseStatus = uint8_t(kResponseStatus_Invalid);
    mConnectionToService = NULL;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

void TimeSyncNode::InvalidateAllContacts(void)
{
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        mContacts[i].mCommState = uint8_t(kCommState_Invalid);
        mContacts[i].mResponseStatus = uint8_t(kResponseStatus_Invalid);
    }

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    InvalidateServiceContact();
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
}

int16_t TimeSyncNode::SetAllValidContactsToIdleAndInvalidateResponse(void)
{
    int16_t rCountIdleContact = 0;

    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if (uint8_t(kCommState_Invalid) != mContacts[i].mCommState)
        {
            mContacts[i].mCommState = uint8_t(kCommState_Idle);
            mContacts[i].mResponseStatus = uint8_t(kResponseStatus_Invalid);
            ++rCountIdleContact;
        }
    }

    return rCountIdleContact;
}

int16_t TimeSyncNode::SetAllCompletedContactsToIdle(void)
{
    int16_t rCountIdleContact = 0;

    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if (uint8_t(kCommState_Completed) == mContacts[i].mCommState)
        {
            mContacts[i].mCommState = uint8_t(kCommState_Idle);
            ++rCountIdleContact;
        }
    }

    return rCountIdleContact;
}

int16_t TimeSyncNode::GetNumNotYetCompletedContacts(void)
{
    int16_t rCountCompletedOrInvalidContact = 0;

    // count the number of invalid or completed contacts
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if ((uint8_t(kCommState_Completed) == mContacts[i].mCommState)
            || (uint8_t(kCommState_Invalid) == mContacts[i].mCommState))
        {
            ++rCountCompletedOrInvalidContact;
        }
    }

    // the result is somewhat valid but not yet completed
    return (WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS - rCountCompletedOrInvalidContact);
}

int16_t TimeSyncNode::GetNumReliableResponses(void)
{
    int16_t rCountReliableResponses = 0;

    // count the number of invalid or completed contacts
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if ((uint8_t(kCommState_Invalid) == mContacts[i].mCommState)
            && (uint8_t(kResponseStatus_ReliableResponse) == mContacts[i].mResponseStatus))
        {
            ++rCountReliableResponses;
        }
    }

    return rCountReliableResponses;
}

Contact * TimeSyncNode::GetNextIdleContact(void)
{
    Contact * rContact = NULL;

    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if (uint8_t(kCommState_Idle) == mContacts[i].mCommState)
        {
            rContact = &mContacts[i];

            break;
        }
    }

    return rContact;
}

WEAVE_ERROR TimeSyncNode::SetupUnicastCommContext(Contact * const aContact)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (NULL != mExchageContext)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }
    else
    {
        // Create a new exchange context, targeting this state instance

#if WEAVE_DETAIL_LOGGING
        {
            char buffer[128] = { 0 };
            aContact->mNodeAddr.ToString(buffer, sizeof(buffer));
            WeaveLogDetail(TimeService, "Preparing exchange context for %" PRIX64 " at %s",
                aContact->mNodeId, buffer);
        }
#endif // WEAVE_DETAIL_LOGGING

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
        // check if we're syncing with cloud service with a dedicated connection or not
        if (aContact != &mServiceContact)
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
        {
            // we're not using connection to sync
            mExchageContext = GetExchangeMgr()->NewContext(aContact->mNodeId, aContact->mNodeAddr, this);

            // Configure the encryption and key used to send the request
            mExchageContext->EncryptionType = mEncryptionType;
            mExchageContext->KeyId = mKeyId;
        }
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
        else
        {
            // we're syncing with the cloud service
            // use the security settings of the connection
            mExchageContext = GetExchangeMgr()->NewContext(mConnectionToService, this);
        }
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

        if (NULL == mExchageContext)
        {
            ExitNow(err = WEAVE_ERROR_NO_MEMORY);
        }

        mExchageContext->OnMessageReceived = HandleUnicastSyncResponse;

        mExchageContext->ResponseTimeout = WEAVE_CONFIG_TIME_CLIENT_TIMER_UNICAST_MSEC;
        mExchageContext->OnResponseTimeout = HandleUnicastResponseTimeout;

        mActiveContact = aContact;

        // acquire unadjusted timestamp
        err = Platform::Time::GetMonotonicRawTime(&mUnadjTimestampLastSent_usec);
        SuccessOrExit(err);
    }

exit:
    WeaveLogFunctError(err);
    if ((WEAVE_NO_ERROR != err) && (NULL != mExchageContext))
    {
        mExchageContext->Close();
        mExchageContext = NULL;
    }

    return err;
}

bool TimeSyncNode::DestroyCommContext(void)
{
    bool HaveToClose = false;

    if (NULL != mExchageContext)
    {
        mExchageContext->Close();
        mExchageContext = NULL;
        HaveToClose = true;
    }
    mActiveContact = NULL;
    mUnadjTimestampLastSent_usec = TIMESYNC_INVALID;

    return HaveToClose;
}

WEAVE_ERROR TimeSyncNode::SyncWithNodes(const int16_t aNumNode, const ServingNode aNodes[])
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS < aNumNode)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (kClientState_Idle != GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    InvalidateAllContacts();

    for (int i = 0; i < aNumNode; ++i)
    {
        mContacts[i].mCommState = uint8_t(kCommState_Idle);
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        mContacts[i].mIsTimeChangeNotification = false;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        mContacts[i].mNodeId = aNodes[i].mNodeId;
        mContacts[i].mNodeAddr = aNodes[i].mNodeAddr;
        mContacts[i].mCountCommError = 0;
    }

    EnterState_Sync_1();

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncNode::Sync(
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    const bool aForceDiscoverAgain
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (kClientState_Idle != GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    if (aForceDiscoverAgain)
    {
        // mark all known contacts to be invalid, forcing a re-discovery
        InvalidateAllContacts();
        EnterState_Discover();
    }
    else
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    {
        const int countNumContacts = SetAllValidContactsToIdleAndInvalidateResponse();
        if (countNumContacts <= 0)
        {
            WEAVE_TIME_PROGRESS_LOG(TimeService, "No contact to sync to. Discovery is needed to proceed");
            RegisterCommError();
        }
        EnterState_Sync_1();
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncNode::Abort(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    const TimeSyncNode::ClientState state = GetClientState();

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (kClientState_Idle == state)
    {
        // no operation
    }
    else
    {
        WEAVE_TIME_PROGRESS_LOG(TimeService, "Time sync aborted in state %d (%s)", int(state), GetClientStateName());

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        // unregister timer handler
        // note this function doesn't complain even if the timer has not been registered, and there is no return value
        GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleMulticastResponseTimeout, this);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

        DestroyCommContext();

        if ((kClientState_BeginNormal > state) || (kClientState_EndNormal < state))
        {
            // don't touch the state
        }
        else
        {
            SetClientState(kClientState_Idle);
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
void TimeSyncNode::StoreNotifyingContact(const uint64_t aNodeId, const IPAddress & aNodeAddr)
{
    // TODO: not implemented yet
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // find a slot to store contact info for the sender of this time change notification
    // note we set the last parameter to true, which means we shall overwrite the previous
    // time change notification, if any
    Contact * const contact = FindReplaceableContact(aNodeId, aNodeAddr, true);

    if (NULL == contact)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // initialize the contact as if this is a unicast case
    contact->mCommState = uint8_t(kCommState_Idle);
    contact->mIsTimeChangeNotification = true;
    contact->mCountCommError = 0;
    contact->mNodeId = aNodeId;
    contact->mNodeAddr = aNodeAddr;
    contact->mResponseStatus = uint8_t(kResponseStatus_Invalid);

exit:
    WeaveLogFunctError(err);

    return;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

void TimeSyncNode::RegisterCommError(Contact * const aContact)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IgnoreUnusedVariable(err);

    if (NULL != aContact)
    {
        if (aContact->mCountCommError < UINT8_MAX)
        {
            ++aContact->mCountCommError;
        }

        WeaveLogDetail(TimeService, "Node %" PRIX64 ": communication error count %d",
            aContact->mNodeId, aContact->mCountCommError);

        aContact->mCommState = uint8_t(kCommState_Completed);
    }
    else
    {
        // we have not any contact!
    }

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    if (mIsAutoSyncEnabled)
    {
        timesync_t boottime_usec;

        err = Platform::Time::GetSleepCompensatedMonotonicTime(&boottime_usec);
        SuccessOrExit(err);

        if ((mBootTimeForNextAutoDiscovery_usec - boottime_usec) > timesync_t(mShortestDiscoveryPeriod_msec) * 1000)
        {
            // schedule discovery to happen at urgent rate
            err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(mShortestDiscoveryPeriod_msec, HandleAutoDiscoveryTimeout,
                this);
            SuccessOrExit(err);

            // calculate timestamp for the next discovery
            mBootTimeForNextAutoDiscovery_usec = boottime_usec + timesync_t(mShortestDiscoveryPeriod_msec) * 1000;

            WEAVE_TIME_PROGRESS_LOG(TimeService, "Communication error changed schedule for auto discovery");
        }
        else
        {
            // we're about to re-discover the environment soon, so there is nothing to do here
            WEAVE_TIME_PROGRESS_LOG(TimeService, "Communication error overlooked as auto discovery is going to happen soon");
        }
    }

exit:
    WeaveLogFunctError(err);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
}

WEAVE_ERROR TimeSyncNode::SendSyncRequest(bool * const rIsMessageSent, Contact * const aContact)
{
    WEAVE_ERROR     err         = WEAVE_NO_ERROR;
    TimeSyncRequest request;
    PacketBuffer*   msgBuf      = NULL;

    *rIsMessageSent = false;

    // we're sending request to this node
    aContact->mCommState = uint8_t(kCommState_Active);

    // allocate buffer and then encode the response into it
    msgBuf = PacketBuffer::NewWithAvailableSize(TimeSyncRequest::kPayloadLen);
    if (NULL == msgBuf)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    // encode request into the buffer
    // since this is unicast, we're using the maximum likelihood here
    request.Init(TimeSyncRequest::kLikelihoodForResponse_Max, (kTimeSyncRole_Coordinator == mRole) ? true : false);

    err = SetupUnicastCommContext(aContact);
    SuccessOrExit(err);

    err = request.Encode(msgBuf);
    SuccessOrExit(err);

    // send out the request
    err = mExchageContext->SendMessage(kWeaveProfile_Time, kTimeMessageType_TimeSyncRequest, msgBuf,
        ExchangeContext::kSendFlag_ExpectResponse);
    msgBuf = NULL;
    if (WEAVE_NO_ERROR == err)
    {
        // if nothing goes wrong, we should see either a response message or a timeout event
        *rIsMessageSent = true;
    }
    else
    {
        // failure at this stage is special, as we might fail to contact any node because of
        // any kind of network issues, and we won't hear from the response timeout
        // let's clear the error, mark the comm state as completed, and try the next contact
        WeaveLogFunctError(err);
        err = WEAVE_NO_ERROR;
        RegisterCommError(aContact);
        DestroyCommContext();
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (WEAVE_NO_ERROR != err)
    {
        if (NULL != aContact)
        {
            // marking this contact as invalid is weird, but we're just trying to avoid any problem next time
            aContact->mCommState = uint8_t(kCommState_Invalid);
        }
        DestroyCommContext();
    }

    return err;
}

void TimeSyncNode::EnterState_Sync_1(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Contact * contact = NULL;
    bool isMessageSent = false;

    switch (GetClientState())
    {
    case kClientState_Sync_1:
        // do nothing. note we'd keep entering this same state until we get enough responses from our contacts
        break;
    case kClientState_Idle:
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        // fall through
    case kClientState_Sync_Discovery:
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        SetClientState(kClientState_Sync_1);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    do
    {
        contact = GetNextIdleContact();
        if (NULL == contact)
        {
            // no one left for us to contact to, move to Sync_2 anyways
            SetAllCompletedContactsToIdle();
            EnterState_Sync_2();
            break;
        }

        err = SendSyncRequest(&isMessageSent, contact);
        contact = NULL;
        SuccessOrExit(err);
    }
    while (!isMessageSent);

exit:
    WeaveLogFunctError(err);

    // abort, and let the application layer know, if we encounter any error that we cannot handle
    AbortOnError(err);
}

void TimeSyncNode::EnterState_Sync_2(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    Contact * contact = NULL;
    bool isMessageSent = false;

    switch (GetClientState())
    {
    case kClientState_Sync_2:
        // do nothing. note we'd keep entering this same context until we get enough responses from our contacts
        break;
    case kClientState_Sync_1:
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        // fall through
    case kClientState_Sync_Discovery:
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        SetClientState(kClientState_Sync_2);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    do
    {
        // try to get the next contact to reach
        contact = GetNextIdleContact();
        if (NULL == contact)
        {
            // we have no more nodes to contact to, try to calculate a time fix or fail
            EndLocalSyncAndTryCalculateTimeFix();
            break;
        }

        err = SendSyncRequest(&isMessageSent, contact);
        contact = NULL;
        SuccessOrExit(err);

    } while (!isMessageSent);

exit:
    WeaveLogFunctError(err);

    // abort, and let the application layer know, if we encounter any error that we cannot handle
    AbortOnError(err);
}

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
void TimeSyncNode::EnterState_ServiceSync_1(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    bool isMessageSent = false;

    switch (GetClientState())
    {
    case kClientState_Idle:
        SetClientState(kClientState_ServiceSync_1);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (uint8_t(kCommState_Idle) != mServiceContact.mCommState)
    {
        // we should only enter this state with the comm state is idle
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    err = SendSyncRequest(&isMessageSent, &mServiceContact);
    SuccessOrExit(err);

    if (!isMessageSent)
    {
        // if we cannot send the message to service over this TCP connection,
        // it's very unlikely that a retry would work
        ExitNow(err = WEAVE_ERROR_CONNECTION_ABORTED);
    }

exit:
    WeaveLogFunctError(err);

    // abort, and let the application layer know, if we encounter any error that we cannot handle
    AbortOnError(err);
}

void TimeSyncNode::EnterState_ServiceSync_2(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    bool isMessageSent = false;

    switch (GetClientState())
    {
    case kClientState_ServiceSync_1:
        SetClientState(kClientState_ServiceSync_2);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (uint8_t(kCommState_Idle) != mServiceContact.mCommState)
    {
        // we should only enter this state with the comm state of idle
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    err = SendSyncRequest(&isMessageSent, &mServiceContact);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    // abort, and let the application layer know, if we encounter any error that we cannot handle
    AbortOnError(err);
}
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
void TimeSyncNode::EnterState_Discover(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    TimeSyncRequest request;

    switch (GetClientState())
    {
    case kClientState_Sync_Discovery:
        // do nothing. we could re-enter from timeout
        break;
    case kClientState_Idle:
        // fall through
    case kClientState_Sync_1:
        SetClientState(kClientState_Sync_Discovery);
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // every time we enter this state, all contacts are flushed
    // pros: simplify the code to find a slot to store the responses
    // cons: we lose a few contacts that has responded in previous inquires
    // the penalty is insignificant, as very probably those contacts would respond to our next inquiry, anyway
    InvalidateAllContacts();

    // we do not expect to see the exchange context still open, as it
    // shall have been closed last time when we completed or aborted
    if (false != DestroyCommContext())
    {
        WeaveLogError(TimeService, "previous exchange context is still open");
    }

    // setup timer
    // note that we cannot actually recover all by ourselves if this timer setup shall fail
    // note that we cannot rely on the response timer used for unicasts, as multicasts could generate
    // multiple responses
    err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(WEAVE_CONFIG_TIME_CLIENT_TIMER_MULTICAST_MSEC,
        HandleMulticastResponseTimeout, this);
    SuccessOrExit(err);

    // Create a new exchange context, targeting all nodes
    mExchageContext = GetExchangeMgr()->NewContext(nl::Weave::kAnyNodeId, this);
    if (NULL == mExchageContext)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    // Configure the encryption and key used to send the request
    mExchageContext->EncryptionType = mEncryptionType;
    mExchageContext->KeyId = mKeyId;

    mExchageContext->OnMessageReceived = HandleMulticastSyncResponse;

    mActiveContact = NULL;

    // acquire unadjusted timestamp
    err = Platform::Time::GetMonotonicRawTime(&mUnadjTimestampLastSent_usec);
    SuccessOrExit(err);

    request.Init(mLastLikelihoodSent, (kTimeSyncRole_Coordinator == mRole) ? true : false);

    // allocate buffer and then encode the response into it
    msgBuf = PacketBuffer::NewWithAvailableSize(TimeSyncRequest::kPayloadLen);
    if (NULL == msgBuf)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    err = request.Encode(msgBuf);
    SuccessOrExit(err);

    WEAVE_TIME_PROGRESS_LOG(TimeService, "Sending out multicast request with likelihood %d / %d", mLastLikelihoodSent,
        TimeSyncRequest::kLikelihoodForResponse_Max);

    // send out the request
    err = mExchageContext->SendMessage(kWeaveProfile_Time, kTimeMessageType_TimeSyncRequest, msgBuf);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (WEAVE_NO_ERROR != err)
    {
        if (NULL != mExchageContext)
        {
            mExchageContext->Close();
        }

        Abort();
    }
    // abort, and let the application layer know, if we encounter any error that we cannot handle
    AbortOnError(err);
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
WEAVE_ERROR TimeSyncNode::SyncWithService(WeaveConnection * const aConnection)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (kClientState_Idle != GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // initialize the contact as if this is a normal unicast case
    mServiceContact.mCommState = uint8_t(kCommState_Idle);

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    mServiceContact.mIsTimeChangeNotification = false;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    mServiceContact.mCountCommError = 0;
    mServiceContact.mNodeId = aConnection->PeerNodeId;
    mServiceContact.mNodeAddr = aConnection->PeerAddr;
    mServiceContact.mResponseStatus = uint8_t(kResponseStatus_Invalid);

    mConnectionToService = aConnection;

    // enter ServiceSync 1
    EnterState_ServiceSync_1();

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
Contact * TimeSyncNode::FindReplaceableContact(const uint64_t aNodeId,
    const IPAddress & aNodeAddr, bool aIsTimeChangeNotification)
{
    Contact * contact = NULL;

    // we only keep one time change notification at any moment
    if (aIsTimeChangeNotification)
    {
        for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
        {
            if (mContacts[i].mIsTimeChangeNotification)
            {
                WeaveLogDetail(TimeService, "Node %" PRIX64 " is taking space from a prior notification",
                    aNodeId);
                contact = &mContacts[i];

                ExitNow();
            }
        }
    }

    // find a slot for this response
    // 1. try to reuse the same contact if we already know this node
    // whether this entry is considered valid or a notification is irrelevant,
    // as we're overwriting the information, anyway
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if ((aNodeId == mContacts[i].mNodeId)
            && (aNodeAddr == mContacts[i].mNodeAddr))
        {
            WeaveLogDetail(TimeService, "Node %" PRIX64 " is already known to us", aNodeId);
            contact = &mContacts[i];

            ExitNow();
        }
    }

    // 2. find any invalid contact to use
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if (uint8_t(kCommState_Invalid) == mContacts[i].mCommState)
        {
            WeaveLogDetail(TimeService, "Node %" PRIX64 " took a previously invalid contact entry", aNodeId);
            contact = &mContacts[i];

            ExitNow();
        }
    }

    // we have confirmed that the contact information of all entries are valid after step 2

    // 3. find any low-quality response to use
    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if ((uint8_t(kResponseStatus_ReliableResponse) != mContacts[i].mResponseStatus))
        {
            WeaveLogDetail(TimeService, "Node %" PRIX64 " replaced a contact entry with bad response", aNodeId);
            contact = &mContacts[i];

            ExitNow();
        }
    }

    // we have confirmed that all responses are valid and reliable to some degree

    // 4. find the oldest one
    // we deliberately want to replace the oldest one, as they responded too quickly
    // we want stable consensus across the fabric, instead of fragmented timing groups
    {
        timesync_t earliest_timestamp = TIMESYNC_MAX;
        int oldest_response = -1;
        for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
        {
            if (mContacts[i].mUnadjTimestampLastContact_usec < earliest_timestamp)
            {
                oldest_response = i;
                earliest_timestamp = mContacts[i].mUnadjTimestampLastContact_usec;
            }
        }

        if (oldest_response >= 0)
        {
            WeaveLogDetail(TimeService, "Node %" PRIX64 " replaced the oldest contact entry", aNodeId);
            contact = &mContacts[oldest_response];

            ExitNow();
        }
        else
        {
            // something is wrong and we failed to find a slot for this response
        }
    }

exit:

    return contact;
}

void TimeSyncNode::UpdateMulticastSyncResponse(const uint64_t aNodeId,
    const IPAddress & aNodeAddr, const TimeSyncResponse & aResponse)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mActiveContact = FindReplaceableContact(aNodeId, aNodeAddr);

    if (NULL == mActiveContact)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // initialize the contact as if this is a unicast case
    mActiveContact->mCommState = uint8_t(kCommState_Active);
    mActiveContact->mIsTimeChangeNotification = false;
    mActiveContact->mCountCommError = 0;
    mActiveContact->mNodeId = aNodeId;
    mActiveContact->mNodeAddr = aNodeAddr;
    mActiveContact->mResponseStatus = uint8_t(kResponseStatus_Invalid);
    // update the contact with response, reusing the unicast code
    UpdateUnicastSyncResponse(aResponse);

exit:
    WeaveLogFunctError(err);

    // flush Contact, for it's a multicast context which shouldn't have any particular context
    mActiveContact = NULL;

    return;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

void TimeSyncNode::UpdateUnicastSyncResponse(const TimeSyncResponse & aResponse)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    timesync_t timestamp_now_usec;
    int32_t rtt_usec;

    // acquire unadjusted timestamp
    err = Platform::Time::GetMonotonicRawTime(&timestamp_now_usec);
    SuccessOrExit(err);

    {
        const timesync_t rtt64_usec = timestamp_now_usec - mUnadjTimestampLastSent_usec;
        if (rtt64_usec < INT32_MAX)
        {
            rtt_usec = int32_t(rtt64_usec);
        }
        else
        {
            // something is wrong, as we shall never see a response coming in after 2^31 seconds!
            ExitNow(err = WEAVE_ERROR_TIMEOUT);
        }
    }

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    // we have received a response from it, so a time change notification is 'normal' contact now
    mActiveContact->mIsTimeChangeNotification = false;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    if (uint8_t(kCommState_Active) != mActiveContact->mCommState)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (uint8_t(kResponseStatus_Invalid) == mActiveContact->mResponseStatus)
    {
        // this is the first response we receive from this node
        // Preserve all data, but mark response status to reflect the qualification

        if ((rtt_usec > WEAVE_CONFIG_TIME_CLIENT_MAX_RTT_USEC)
            || ((rtt_usec / 2) > aResponse.mTimeOfResponse))
        {
            // the response comes back after WEAVE_CONFIG_TIME_CLIENT_MAX_RTT_USEC, which is just too long
            // or
            // the timestamp of the responding node is so low that we cannot compensate for flight time
            // this is not right, as the epoch is 1970/1/1, and no one should have that low timestamp
            mActiveContact->mResponseStatus = uint8_t(kResponseStatus_UnusableResponse);
        }
        else if (aResponse.mTimeSinceLastSyncWithServer_min
            > WEAVE_CONFIG_TIME_CLIENT_REASONABLE_TIME_SINCE_LAST_SYNC_MIN)
        {
            mActiveContact->mResponseStatus = uint8_t(kResponseStatus_LessReliableResponse);
        }
        else
        {
            mActiveContact->mResponseStatus = uint8_t(kResponseStatus_ReliableResponse);
        }

        mActiveContact->mRemoteTimestamp_usec = aResponse.mTimeOfResponse;
        mActiveContact->mRole = uint8_t((aResponse.mIsTimeCoordinator) ? kTimeSyncRole_Coordinator : kTimeSyncRole_Server);
        mActiveContact->mFlightTime_usec = rtt_usec / 2;
        mActiveContact->mNumberOfContactUsedInLastLocalSync = aResponse.mNumContributorInLastLocalSync;
        mActiveContact->mTimeSinceLastSuccessfulSync_min = aResponse.mTimeSinceLastSyncWithServer_min;
        mActiveContact->mUnadjTimestampLastContact_usec = timestamp_now_usec;

        // state moved to completed
        mActiveContact->mCommState = uint8_t(kCommState_Completed);

        WeaveLogDetail(TimeService,
            "Received 1st response from node %" PRIX64 ", with RTT/2 %d usec",
            mActiveContact->mNodeId, mActiveContact->mFlightTime_usec);

        WeaveLogDetail(TimeService, "Role:%d, #Error:%d, #Contributor:%d, LastSync:%d", mActiveContact->mRole,
            mActiveContact->mCountCommError, mActiveContact->mNumberOfContactUsedInLastLocalSync,
            mActiveContact->mTimeSinceLastSuccessfulSync_min);
    }
    else
    {
        // this is a second, or even more, response from the same node
        // we only keep the fastest response

        if ((rtt_usec > WEAVE_CONFIG_TIME_CLIENT_MAX_RTT_USEC)
            || ((rtt_usec / 2) > aResponse.mTimeOfResponse))
        {
            // the response comes back after WEAVE_CONFIG_TIME_CLIENT_MAX_RTT_USEC, which is just too long
            // or
            // the timestamp of the responding node is so low that we cannot compensate for flight time
            // this is not right, as the epoch is 1970/1/1, and no one should have that low timestamp

            // do nothing, keep the previous result
        }
        else if ((aResponse.mTimeSinceLastSyncWithServer_min >= mActiveContact->mTimeSinceLastSuccessfulSync_min)
            && ((rtt_usec / 2) > mActiveContact->mFlightTime_usec))
        {
            // the second response is not based on some newer sync, and the flight time is longer
            // note we probably should use 'age' respective to each response here, but the 2 responses
            // are just a few seconds a part. comparing their age respective to the 2nd response shouldn't
            // bring too much error.

            // do nothing, keep the previous results
        }
        else
        {
            if (aResponse.mTimeSinceLastSyncWithServer_min
                > WEAVE_CONFIG_TIME_CLIENT_REASONABLE_TIME_SINCE_LAST_SYNC_MIN)
            {
                mActiveContact->mResponseStatus = uint8_t(kResponseStatus_LessReliableResponse);
            }
            else
            {
                // set it to be a reliable response
                mActiveContact->mResponseStatus = uint8_t(kResponseStatus_ReliableResponse);
            }

            // all response related data is updated to match with the current round

            mActiveContact->mRemoteTimestamp_usec = aResponse.mTimeOfResponse;
            mActiveContact->mRole = uint8_t((aResponse.mIsTimeCoordinator) ? kTimeSyncRole_Coordinator : kTimeSyncRole_Server);
            mActiveContact->mFlightTime_usec = rtt_usec / 2;
            mActiveContact->mNumberOfContactUsedInLastLocalSync = aResponse.mNumContributorInLastLocalSync;
            mActiveContact->mTimeSinceLastSuccessfulSync_min = aResponse.mTimeSinceLastSyncWithServer_min;
            mActiveContact->mUnadjTimestampLastContact_usec = timestamp_now_usec;
        }

        // state moved to completed
        mActiveContact->mCommState = uint8_t(kCommState_Completed);

        WeaveLogDetail(TimeService,
            "Received 2nd round from node %" PRIX64 ", with RTT/2 %d usec",
            mActiveContact->mNodeId, mActiveContact->mFlightTime_usec);

        WeaveLogDetail(TimeService, "Role:%d, #Error:%d, #Contributor:%d, LastSync:%d", mActiveContact->mRole,
            mActiveContact->mCountCommError, mActiveContact->mNumberOfContactUsedInLastLocalSync,
            mActiveContact->mTimeSinceLastSuccessfulSync_min);
    }

exit:
    WeaveLogFunctError(err);

    return;
}

WEAVE_ERROR TimeSyncNode::CallbackForSyncCompletion(const bool aIsSuccessful, bool aShouldUpdate,
    const bool aIsCorrectionReliable, const bool aIsFromServer, const uint8_t aNumContributor,
    const timesync_t aSystemTimestamp_usec, const timesync_t aDiffTime_usec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (!aIsSuccessful)
    {
        WEAVE_TIME_PROGRESS_LOG(TimeService, "Time sync operation failed");

        if (NULL != OnSyncFailed)
        {
            mIsInCallback = true;
            OnSyncFailed(mApp, WEAVE_END_OF_INPUT);
            mIsInCallback = false;
        }
    }
    else
    {
        WEAVE_TIME_PROGRESS_LOG(TimeService, "Time sync operation succeeded");

        if (NULL != OnSyncSucceeded)
        {
            mIsInCallback = true;
            aShouldUpdate = OnSyncSucceeded(mApp, aDiffTime_usec, aIsCorrectionReliable, aIsFromServer,
                aNumContributor);

            // if this is just a notification for 'no result'
            // ignore the return value of the callback
            if (0 == aNumContributor)
            {
                aShouldUpdate = false;
            }
            mIsInCallback = false;
        }

        if (aShouldUpdate)
        {
            if (0 != aDiffTime_usec)
            {
                WeaveLogDetail(TimeService, "Applying update");

                // acquire unadjusted timestamp
                err = Platform::Time::SetSystemTime(aSystemTimestamp_usec + aDiffTime_usec);
                SuccessOrExit(err);
            }
            else
            {
                WeaveLogDetail(TimeService, "Skipping time update, for the correction is zero");
            }
        }
        else
        {
            // ignore update
             WeaveLogDetail(TimeService, "Time sync correction has been rejected");
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
void TimeSyncNode::EndServiceSyncAndTryCalculateTimeFix(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // time sync operation is considered completed when we reach this function

    timesync_t unadjTimestamp_usec;
    timesync_t systemTimestamp_usec;

    if (kClientState_ServiceSync_2 != GetClientState())
    {
        // we shall only get into this function from ServiceSync_2
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // acquire unadjusted timestamp
    err = Platform::Time::GetMonotonicRawTime(&unadjTimestamp_usec);
    SuccessOrExit(err);

    // acquire System Time
    err = Platform::Time::GetSystemTime(&systemTimestamp_usec);
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogFunctError(err);

        WeaveLogDetail(TimeService, "System time not available, skip");

        err = WEAVE_NO_ERROR;
        ExitNow();
    }

    if ((uint8_t(kCommState_Completed) == mServiceContact.mCommState)
        && ((uint8_t(kResponseStatus_ReliableResponse) == mServiceContact.mResponseStatus)
            || (uint8_t(kResponseStatus_LessReliableResponse) == mServiceContact.mResponseStatus)))
    {
        const timesync_t correctedRemoteSystemTime_usec = (mServiceContact.mRemoteTimestamp_usec
            + mServiceContact.mFlightTime_usec)
            + (unadjTimestamp_usec - mServiceContact.mUnadjTimestampLastContact_usec);

        const timesync_t DiffTime_usec = correctedRemoteSystemTime_usec - systemTimestamp_usec;

        WeaveLogDetail(TimeService, "Update from service");

        err = CallbackForSyncCompletion(
            true, // is sync successful,
            true, // if we should update,
            true, // is the correction from reliable sources
            true, // is the correction from server nodes
            1, // number of contributors
            systemTimestamp_usec, // current system time
            DiffTime_usec /* correction for the system time */);
    }
    else
    {
        // sync failed
        WeaveLogDetail(TimeService, "Sync with service failed");

        err = CallbackForSyncCompletion(
            false, // is sync successful,
            false, // if we should update,
            false, // is the correction from reliable sources
            false, // is the correction from server nodes
            0, // number of contributors
            systemTimestamp_usec, // current system time
            0 /* correction for the system time */);
    }

exit:
    WeaveLogFunctError(err);
    // close all exchange contexts no matter what
    DestroyCommContext();
    // this is one of the final states. we move to either IDLE or ShutdownNeeded
    SetClientState((WEAVE_NO_ERROR == err) ? kClientState_Idle : kClientState_ShutdownNeeded);

    return;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

void TimeSyncNode::EndLocalSyncAndTryCalculateTimeFix(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // time sync operation is considered completed when we reach this function

    // the time correction used for update on successful time sync
    timesync_t DiffTime_usec = 0;

    timesync_t unadjTimestamp_usec;
    timesync_t systemTimestamp_usec;

    uint8_t numAdvisor = 0;
    IgnoreUnusedVariable(numAdvisor);
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    timesync_t sumAdvisorTimestamp_usec = 0;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    uint8_t numCoordinator = 0;
    timesync_t sumCoordinatorTimestamp_usec = 0;

    uint8_t numServer = 0;
    timesync_t sumServerTimestamp_usec = 0;

    uint8_t numUnreliableResponses = 0;
    timesync_t sumUnreliableTimestamp_usec = 0;

    if (kClientState_Sync_2 != GetClientState())
    {
        // we shall only get into this function from Sync_2
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (NULL != FilterTimeCorrectionContributor)
    {
        mIsInCallback = true;
        FilterTimeCorrectionContributor(mApp, mContacts, WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS);
        mIsInCallback = false;
    }

    // acquire unadjusted timestamp
    err = Platform::Time::GetMonotonicRawTime(&unadjTimestamp_usec);
    SuccessOrExit(err);

    for (int i = 0; i < WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS; ++i)
    {
        if ((uint8_t(kCommState_Completed) == mContacts[i].mCommState)
            && ((uint8_t(kResponseStatus_ReliableResponse) == mContacts[i].mResponseStatus)
                || (uint8_t(kResponseStatus_LessReliableResponse) == mContacts[i].mResponseStatus)))
        {
            const timesync_t correctedRemoteSystemTime_usec = (mContacts[i].mRemoteTimestamp_usec
                + mContacts[i].mFlightTime_usec)
                + (unadjTimestamp_usec - mContacts[i].mUnadjTimestampLastContact_usec);

            if (uint8_t(kResponseStatus_ReliableResponse) == mContacts[i].mResponseStatus)
            {
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
                if (mContacts[i].mIsTimeChangeNotification)
                {
                    ++numAdvisor;
                    sumAdvisorTimestamp_usec += correctedRemoteSystemTime_usec;
                }
                else
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
                if (uint8_t(kTimeSyncRole_Coordinator) == mContacts[i].mRole)
                {
                    ++numCoordinator;
                    sumCoordinatorTimestamp_usec += correctedRemoteSystemTime_usec;
                }
                else if (uint8_t(kTimeSyncRole_Server) == mContacts[i].mRole)
                {
                    ++numServer;
                    sumServerTimestamp_usec += correctedRemoteSystemTime_usec;
                }
                else
                {
                    // this shall not happen
                    ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
                }
            }
            else if (uint8_t(kResponseStatus_LessReliableResponse) == mContacts[i].mResponseStatus)
            {
                ++numUnreliableResponses;
                sumUnreliableTimestamp_usec += correctedRemoteSystemTime_usec;
            }
        }
        else
        {
            // skip this contact. it's either invalid or the response unusable
        }
    }

    WeaveLogDetail(TimeService,
        "Number of responses: A:%d C:%d S:%d U:%d",
        numAdvisor, numCoordinator, numServer,
        numUnreliableResponses);

    // acquire System Time
    err = Platform::Time::GetSystemTime(&systemTimestamp_usec);
    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogFunctError(err);

        WeaveLogDetail(TimeService, "System time not available, skip");

        err = WEAVE_NO_ERROR;
        ExitNow();
    }

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    // 1. check if we're getting result from an advisor, which sent us time change notification earlier
    if (numAdvisor)
    {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        DiffTime_usec = sumAdvisorTimestamp_usec - systemTimestamp_usec;
#else
        DiffTime_usec = Platform::Divide(sumAdvisorTimestamp_usec, numAdvisor) - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1

        WeaveLogDetail(TimeService, "Update from %d advisor(s)", numAdvisor);

        err = CallbackForSyncCompletion(
            true, // is sync successful,
            true, // if we should update,
            true, // is the correction from reliable sources
            false, // is the correction from server nodes
            numAdvisor, // number of contributors
            systemTimestamp_usec, // current system time
            DiffTime_usec /* correction for the system time */);

        ExitNow();
    }
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    // 2. check if server time correction is large enough
    if (numServer)
    {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        DiffTime_usec = sumServerTimestamp_usec - systemTimestamp_usec;
#else
        DiffTime_usec = Platform::Divide(sumServerTimestamp_usec, numServer) - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1

        if ((WEAVE_CONFIG_TIME_CLIENT_MIN_OFFSET_FROM_SERVER_USEC < DiffTime_usec)
            || (-WEAVE_CONFIG_TIME_CLIENT_MIN_OFFSET_FROM_SERVER_USEC > DiffTime_usec))
        {
            // offset from server is too big and we cannot ignore
            WeaveLogDetail(TimeService, "Update from %d server(s)", numServer);

            err = CallbackForSyncCompletion(
                true, // is sync successful,
                true, // if we should update,
                true, // is the correction from reliable sources
                true, // is the correction from server nodes
                numServer, // number of contributors
                systemTimestamp_usec, // current system time
                DiffTime_usec /* correction for the system time */);

            ExitNow();
        }
        else
        {
            if (0 == numCoordinator)
            {
                // update from server is too small
                // correction is reliable, but we don't want to apply it
                WeaveLogDetail(TimeService, "Update from %d server(s) too small, rejection suggested", numServer);

                err = CallbackForSyncCompletion(
                    true, // is sync successful,
                    false, // if we should update,
                    true, // is the correction from reliable sources
                    true, // is the correction from server nodes
                    numServer, // number of contributors
                    systemTimestamp_usec, // current system time
                    DiffTime_usec /* correction for the system time */);

                ExitNow();
            }
            else
            {
                WeaveLogDetail(TimeService,
                    "Update from %d server(s) too small, skip", numServer);

                ExitNow();
            }
        }
    }

    // 3. check if we are using time correction from coordinator
    if (numCoordinator)
    {
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        if (kTimeSyncRole_Coordinator == mRole)
        {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
            // We take the average of two timestamps.
            DiffTime_usec = ((sumCoordinatorTimestamp_usec + systemTimestamp_usec) >> 1)
                            - systemTimestamp_usec;
#else
            DiffTime_usec = Platform::Divide(sumCoordinatorTimestamp_usec + systemTimestamp_usec, numCoordinator + 1)
                - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        }
        else
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
            DiffTime_usec = sumCoordinatorTimestamp_usec - systemTimestamp_usec;
#else
            DiffTime_usec = Platform::Divide(sumCoordinatorTimestamp_usec, numCoordinator) - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        }
        WeaveLogDetail(TimeService, "Update from %d coordinator(s)", numCoordinator);

        err = CallbackForSyncCompletion(
            true, // is sync successful,
            true, // if we should update,
            true, // is the correction from reliable sources
            false, // is the correction from server nodes
            numCoordinator, // number of contributors
            systemTimestamp_usec, // current system time
            DiffTime_usec /* correction for the system time */);

        ExitNow();
    }

    // 4. last hope is any unreliable node
    if (numUnreliableResponses)
    {
#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        if (kTimeSyncRole_Coordinator == mRole)
        {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
            DiffTime_usec = ((sumUnreliableTimestamp_usec + systemTimestamp_usec) >> 1)
                            - systemTimestamp_usec;
#else
            DiffTime_usec = Platform::Divide(sumUnreliableTimestamp_usec + systemTimestamp_usec, numUnreliableResponses + 1)
                - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        }
        else
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
        {
#if WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
            DiffTime_usec = sumUnreliableTimestamp_usec - systemTimestamp_usec;
#else
            DiffTime_usec = Platform::Divide(sumUnreliableTimestamp_usec, numUnreliableResponses) - systemTimestamp_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS == 1
        }

        WeaveLogDetail(TimeService, "Update from %d unreliable source(s)", numUnreliableResponses);

        err = CallbackForSyncCompletion(
            true, // is sync successful,
            true, // if we should update,
            false, // is the correction from reliable sources
            false, // is the correction from server nodes
            numUnreliableResponses, // number of contributors
            systemTimestamp_usec, // current system time
            DiffTime_usec /* correction for the system time */);

        ExitNow();
    }

    err = CallbackForSyncCompletion(
        true, // is sync successful,
        false, // if we should update,
        false, // is the correction from reliable sources
        false, // is the correction from server nodes
        0, // number of contributors
        systemTimestamp_usec, // current system time
        0 /* correction for the system time */);

exit:
    WeaveLogFunctError(err);
    // close all exchange contexts no matter what
    DestroyCommContext();
    // this is one of the final states. we move to either IDLE or ShutdownNeeded
    SetClientState((WEAVE_NO_ERROR == err) ? kClientState_Idle : kClientState_ShutdownNeeded);

    return;
}

void TimeSyncNode::HandleUnicastSyncResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo,
    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(ec->AppState);
    TimeSyncResponse response;
    const TimeSyncNode::ClientState ClientStateAtEntry(client->GetClientState());

    if (kTimeMessageType_TimeSyncResponse != msgType)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
    }

    err = TimeSyncResponse::Decode(&response, payload);
    SuccessOrExit(err);

    if ((kClientState_Sync_1 == ClientStateAtEntry) || (kClientState_Sync_2 == ClientStateAtEntry))
    {
        // Verify the response was received via an authenticated session
        // note that under this error, we just throw the whole message away, so communication with
        // this node will be treated as timeout
        if ((ec->KeyId != client->mKeyId) || (ec->EncryptionType != client->mEncryptionType))
        {
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_AUTH_MODE);
        }

        // now we believe we have received a response from the node we intend to hear from
        // update the record now
        client->UpdateUnicastSyncResponse(response);

        // Close this exchange context
        // note we need to close it before we enter any of Sync_1 or Sync_2 states,
        // which might need that exchange context to talk to someone else
        client->DestroyCommContext();
        ec = NULL;

        if (kClientState_Sync_1 == ClientStateAtEntry)
        {
            // check number of contacts again and decide our next state
            if (0 == client->GetNumNotYetCompletedContacts())
            {
                // we have no more nodes to contact, move the Sync_2
                client->SetAllCompletedContactsToIdle();
                client->EnterState_Sync_2();
            }
            else
            {
                // re-enter Sync_1 to continue evaluating the contact list
                client->EnterState_Sync_1();
            }
        }
        else
        {
            // check number of contacts again and decide our next state
            if (0 == client->GetNumNotYetCompletedContacts())
            {
                // we have no more nodes to contact, try to calculate a time fix
                client->EndLocalSyncAndTryCalculateTimeFix();
            }
            else
            {
                // re-enter Sync_2 to continue evaluating the contact list
                client->EnterState_Sync_2();
            }
        }
    }
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    else if ((kClientState_ServiceSync_1 == ClientStateAtEntry) || (kClientState_ServiceSync_2 == ClientStateAtEntry))
    {
        // Verify the response was received via an authenticated session
        // note that under this error, we just throw the whole message away, so communication with
        // this node will be treated as timeout
        if ((ec->KeyId != client->mConnectionToService->DefaultKeyId)
            || (ec->EncryptionType != client->mConnectionToService->DefaultEncryptionType))
        {
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_AUTH_MODE);
        }

        // now we believe we have received a response from the node we intend to hear from
        // update the record now
        client->UpdateUnicastSyncResponse(response);

        // Close this exchange context
        // note we need to close it before we enter any other states,
        // which might need that exchange context to talk to someone else
        client->DestroyCommContext();
        ec = NULL;

        if (kClientState_ServiceSync_1 == ClientStateAtEntry)
        {
            // ServiceSync_1 => ServiceSync_2
            client->mServiceContact.mCommState = uint8_t(kCommState_Idle);
            client->EnterState_ServiceSync_2();
        }
        else
        {
            // Complete the statemachine and go back to Idle
            client->EndServiceSyncAndTryCalculateTimeFix();
            client->InvalidateServiceContact();
        }
    }
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        client->DestroyCommContext();
        client->AbortOnError(err);
    }

exit:
    // note we have to be very careful about what we do at here
    // as the state of 'client' might have changed due to transition
    WeaveLogFunctError(err);
    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
    }
}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
void TimeSyncNode::HandleMulticastSyncResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo,
    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // note the 'contact' pointer is NULL for multicasts, as we're not sending
    // the request to any particular contact
    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(ec->AppState);
    TimeSyncResponse response;

    if (kTimeMessageType_TimeSyncResponse != msgType)
    {
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
    }

#if WEAVE_DETAIL_LOGGING
    {
        char msgSourceStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];
        WeaveMessageSourceToStr(msgSourceStr, sizeof(msgSourceStr), msgInfo);
        WeaveLogDetail(TimeService, "Received response from %s", msgSourceStr);
    }
#endif // WEAVE_DETAIL_LOGGING

    // Verify the response was received via an authenticated session
    // note that under this error, we just throw the whole message away
    if ((ec->KeyId != client->mKeyId) || (ec->EncryptionType != client->mEncryptionType))
    {
        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_AUTH_MODE);
    }

    if (ec != client->mExchageContext)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (kClientState_Sync_Discovery != client->GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    err = TimeSyncResponse::Decode(&response, payload);
    SuccessOrExit(err);

    // try to find a slot to store the response
    client->UpdateMulticastSyncResponse(msgInfo->SourceNodeId, pktInfo->SrcAddress, response);

    // keep waiting for the next response
    // note we don't leave discovery phase because of responses we receive, for we want to hear from
    // as many nodes as possible after each multicast, and choose the furtherest from this node

exit:
    WeaveLogFunctError(err);
    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
    }

    if (NULL != client)
    {
        // abort, and let the application layer know, if we encounter any error that we cannot handle
        client->AbortOnError(err);
    }
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

void TimeSyncNode::HandleTimeChangeNotification(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo,
    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool ShouldHandle = false;

    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(ec->AppState);

    // TODO: Note that authentication for Time Change Notification is not available yet

    WeaveLogDetail(TimeService, "Time Change Notification: local node ID: %" PRIX64 ", peer node ID: %" PRIX64,
      client->GetFabricState()->LocalNodeId, ec->PeerNodeId);

    // ignore notifications coming from our own node ID
    // this is because some network stacks would be looped back multicasts
    if (client->GetFabricState()->LocalNodeId == ec->PeerNodeId)
    {
      // ignore notification
    }
    else
    {
        TimeChangeNotification notification;
        const TimeSyncNode::ClientState ClientStateAtEntry(client->GetClientState());

        // check internal state
        // only try to decode if we're in any of these normal states
        if ((kClientState_BeginNormal >= ClientStateAtEntry)
            && (kClientState_EndNormal <= ClientStateAtEntry))
        {
            ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        }

        if (kClientState_Idle != ClientStateAtEntry)
        {
            // ignore notification if we're not in IDLE state
            WeaveLogDetail(TimeService, "Time change notification ignored, for we're not in idle state");
            err = WEAVE_NO_ERROR;
            ExitNow();
        }

        ShouldHandle = true;

        err = TimeChangeNotification::Decode(&notification, payload);
        SuccessOrExit(err);

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        // make a copy of the notification, so we can contact this node in the next sync
        client->StoreNotifyingContact(ec->PeerNodeId, ec->PeerAddr);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

        if (client->mIsAutoSyncEnabled)
        {
            // schedule sync to happen after short delay
            // note this actually could push the next sync further away if we're very close to the next schedule sync
            // however, the chance is not very high, for the push is just one second
            const uint32_t randomDelayMsec = rand() % 1000;

            WeaveLogDetail(TimeService, "AutoSync: arrange next time sync in %u sec.", randomDelayMsec / 1000);

            err = client->GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(randomDelayMsec, HandleAutoSyncTimeout, client);
            SuccessOrExit(err);
        }
    }

exit:
    WeaveLogFunctError(err);
    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
    }

    if ((NULL != client) && (NULL != ec) && ShouldHandle)
    {
        if (NULL != client->OnTimeChangeNotificationReceived)
        {
            // make a copy and then close the context
            uint64_t nodeId(ec->PeerNodeId);
            IPAddress peerAddr(ec->PeerAddr);
            ec->Close();
            // set ec to NULL so it doesn't get closed again
            ec = NULL;

            // this is a special callback
            // note we don't have the mIsInCallback protection around it
            // the reason is to enable calling Sync family functions
            // within this callback from the app layer, which might be
            // easier for the application layer to use
            client->OnTimeChangeNotificationReceived(
                client->mApp, nodeId, peerAddr);
        }
        else
        {
            // silently ignore this notification, for a handler is not provided
        }
    }
    else
    {
        // silently ignore this notification, for state is not right
    }

    // close the exchange context if non-NULL
    if (NULL != ec)
    {
        ec->Close();
    }
}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
void TimeSyncNode::HandleMulticastResponseTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(aAppState);

    WeaveLogDetail(TimeService, "Multicast just timed out at client state: %d (%s)", client->GetClientState(),
        client->GetClientStateName());

    if (kClientState_Sync_Discovery != client->GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    client->DestroyCommContext();

    if ((TimeSyncRequest::kLikelihoodForResponse_Max > client->mLastLikelihoodSent)
        && (WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS > client->GetNumReliableResponses()))
    {
        client->mLastLikelihoodSent += 8;
        if (client->mLastLikelihoodSent > TimeSyncRequest::kLikelihoodForResponse_Max)
        {
            client->mLastLikelihoodSent = TimeSyncRequest::kLikelihoodForResponse_Max;
        }

        // there is still room to raise the likelihood, continue to discover
        client->EnterState_Discover();
    }
    else
    {
        // we're already using the maximum likelihood in this round
        // move to sync_2 and hope we have someone to talk to for the second round

        if (client->mLastLikelihoodSent > TimeSyncRequest::kLikelihoodForResponse_Min)
        {
            --(client->mLastLikelihoodSent);
        }

        client->SetAllCompletedContactsToIdle();
        client->EnterState_Sync_2();
    }

exit:
    WeaveLogFunctError(err);
    if (NULL != client)
    {
        // abort, and let the application layer know, if we encounter any error that we cannot handle
        client->AbortOnError(err);
    }

    return;
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

void TimeSyncNode::HandleUnicastResponseTimeout(ExchangeContext * const ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the client and contact pointer, as the context will be closed later
    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(ec->AppState);
    Contact * contact = client->mActiveContact;

    const TimeSyncNode::ClientState ClientStateAtEntry(client->GetClientState());

    WeaveLogDetail(TimeService, "Unicast just timed out at client state: %d (%s)", client->GetClientState(),
        client->GetClientStateName());

    // close this context as timeout
    client->DestroyCommContext();

    // register communication error
    // note we don't invalidated the contact easily
    client->RegisterCommError(contact);

    if (kClientState_Sync_1 == ClientStateAtEntry)
    {
        if (0 == client->GetNumNotYetCompletedContacts())
        {
            // we've run out of contacts
            // move to Sync_2
            client->SetAllCompletedContactsToIdle();
            client->EnterState_Sync_2();
        }
        else
        {
            // we haven't exhausted all contacts and haven't collected enough number of responses
            // re-enter Sync_1 to continue evaluating the contact list
            client->EnterState_Sync_1();
        }
    }
    else if (kClientState_Sync_2 == ClientStateAtEntry)
    {
        if (0 == client->GetNumNotYetCompletedContacts())
        {
            // we have no more nodes to contact, try to calculate a time fix
            client->EndLocalSyncAndTryCalculateTimeFix();
        }
        else
        {
            // we haven't exhausted all contacts and haven't collected enough number of responses
            // re-enter Sync_2 to continue evaluating the contact list
            client->EnterState_Sync_2();
        }
    }
#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    else if (kClientState_ServiceSync_1 == ClientStateAtEntry)
    {
        // ServiceSync_1 => ServiceSync_2
        client->mServiceContact.mCommState = uint8_t(kCommState_Idle);
        client->EnterState_ServiceSync_2();
    }
    else if (kClientState_ServiceSync_2 == ClientStateAtEntry)
    {
        // Complete the statemachine and go back to Idle
        client->EndServiceSyncAndTryCalculateTimeFix();
        client->InvalidateServiceContact();
    }
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        client->AbortOnError(err);
    }

    // Note that we have to be careful what to do at here, as
    // the state of 'client' might have been changed in those state transitions
    WeaveLogFunctError(err);

    return;
}

void TimeSyncNode::DisableAutoSync(void)
{
    GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleAutoSyncTimeout, this);
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleAutoDiscoveryTimeout, this);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    mIsAutoSyncEnabled = false;
}

WEAVE_ERROR TimeSyncNode::EnableAutoSync(const int32_t aSyncPeriod_msec
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    ,
    const int32_t aNominalDiscoveryPeriod_msec,
    const int32_t aShortestDiscoveryPeriod_msec
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    )
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (kClientState_Idle != GetClientState())
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    mIsAutoSyncEnabled = true;
    mSyncPeriod_msec = aSyncPeriod_msec;

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    mIsUrgentDiscoveryPending = false;
    mNominalDiscoveryPeriod_msec = aNominalDiscoveryPeriod_msec;
    mShortestDiscoveryPeriod_msec = aShortestDiscoveryPeriod_msec;

    // schedule discovery immediately
    err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(0, HandleAutoDiscoveryTimeout, this);
    SuccessOrExit(err);

    // calculate the timestamp for the next discovery
    // this is needed for handling of communication errors
    err = Platform::Time::GetSleepCompensatedMonotonicTime(&mBootTimeForNextAutoDiscovery_usec);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    // schedule sync to happen at nominal rate
    err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(mSyncPeriod_msec, HandleAutoSyncTimeout, this);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

void TimeSyncNode::AutoSyncNow(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleAutoSyncTimeout, this);

    if (mIsAutoSyncEnabled)
    {
        // schedule sync to happen at nominal rate
        err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(mSyncPeriod_msec, HandleAutoSyncTimeout, this);
        SuccessOrExit(err);

        if (kClientState_Idle == GetClientState())
        {
            err = Sync();
        }
        else
        {
            // skip this chance
            WeaveLogDetail(TimeService, "Auto sync operation skipped");
        }
    }
    else
    {
        // ignore
    }

exit:
    WeaveLogFunctError(err);

    AbortOnError(err);
}

void TimeSyncNode::HandleAutoSyncTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(aAppState);

    WEAVE_TIME_PROGRESS_LOG(TimeService, "Auto Sync timer just fired at client state: %d (%s)", client->GetClientState(),
        client->GetClientStateName());

    client->AutoSyncNow();
}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
void TimeSyncNode::HandleAutoDiscoveryTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TimeSyncNode * const client = reinterpret_cast<TimeSyncNode *>(aAppState);

    WEAVE_TIME_PROGRESS_LOG(TimeService, "Auto Discovery timer just fired at client state: %d (%s)", client->GetClientState(),
        client->GetClientStateName());

    client->mIsUrgentDiscoveryPending = false;

    if (client->mIsAutoSyncEnabled)
    {
        if (kClientState_Idle != client->GetClientState())
        {
            // Silently abort what we're doing, without notifying the application layer
            client->Abort();
        }

        // reset the sync timer to be aligned with this discovery
        err = client->GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(client->mSyncPeriod_msec,
            HandleAutoSyncTimeout, client);
        SuccessOrExit(err);

        // schedule discovery to happen at nominal rate
        err = client->GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(client->mNominalDiscoveryPeriod_msec,
            HandleAutoDiscoveryTimeout, client);
        SuccessOrExit(err);

        // calculate the timestamp for the next discovery
        // this is needed for handling of communication errors
        err = Platform::Time::GetSleepCompensatedMonotonicTime(&(client->mBootTimeForNextAutoDiscovery_usec));
        SuccessOrExit(err);
        (client->mBootTimeForNextAutoDiscovery_usec) += timesync_t(client->mNominalDiscoveryPeriod_msec) * 1000;

        err = client->Sync(true);
        SuccessOrExit(err);
    }
    else
    {
        // ignore
    }

exit:
    WeaveLogFunctError(err);

    client->AbortOnError(err);
}
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

void SingleSourceTimeSyncClient::SetClientState(const SingleSourceTimeSyncClient::ClientState state)
{
    mClientState = state;

    WeaveLogDetail(TimeService, "Client entering state %d (%s)", mClientState,
        GetClientStateName());
}

WEAVE_ERROR SingleSourceTimeSyncClient::Init(void * const aApp, WeaveExchangeManager * const aExchangeMgr)
{
    mApp = aApp;
    mExchangeMgr = aExchangeMgr;
    mBinding = NULL;
    SetClientState(kClientState_Idle);
    mIsInCallback = false;
    mExchageContext = NULL;
    mFlightTime_usec = kFlightTimeInvalid;
    mUnadjTimestampLastSent_usec = TIMESYNC_INVALID;
    mRemoteTimestamp_usec = TIMESYNC_INVALID;
    mRegisterSyncResult_usec = TIMESYNC_INVALID;

    OnTimeChangeNotificationReceived = NULL;
    mOnSyncCompleted = NULL;

    // Register to receive unsolicited time sync request advisory messages from the exchange manager.
    WEAVE_ERROR err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Time,
        kTimeMessageType_TimeSyncTimeChangeNotification,
        HandleTimeChangeNotification, this);

    return err;
}

void SingleSourceTimeSyncClient::Abort(void)
{
    if (NULL != mBinding)
    {
        mBinding->Release();
        mBinding = NULL;
    }

    if (NULL != mExchageContext)
    {
        mExchageContext->Abort();
        mExchageContext = NULL;
    }

    SetClientState(kClientState_Idle);
}

WEAVE_ERROR SingleSourceTimeSyncClient::Sync(Binding * const aBinding, SyncCompletionHandler OnSyncCompleted)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(false == mIsInCallback, err = WEAVE_ERROR_INCORRECT_STATE);

    if (kClientState_Idle != mClientState)
    {
        Abort();
    }

    SetClientState(kClientState_Sync_1);

    InvalidateRegisteredResult();

    mOnSyncCompleted = OnSyncCompleted;

    mBinding = aBinding;
    mBinding->AddRef();

    // failure at here would prevent the state machine form continuing,
    // so we simply return an error code
    err = SendSyncRequest();
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    // This saves the app layer from calling Abort when there is an error
    if (WEAVE_NO_ERROR != err)
    {
        Abort();
    }

    return err;
}

WEAVE_ERROR SingleSourceTimeSyncClient::SendSyncRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TimeSyncRequest request;
    PacketBuffer* msgBuf = NULL;

    // allocate buffer and then encode the response into it
    msgBuf = PacketBuffer::NewWithAvailableSize(TimeSyncRequest::kPayloadLen);
    if (NULL == msgBuf)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    // encode request into the buffer
    // since this is unicast, we're using the maximum likelihood here
    request.Init(TimeSyncRequest::kLikelihoodForResponse_Max, false);

    err = request.Encode(msgBuf);
    SuccessOrExit(err);

    if (NULL != mExchageContext)
    {
        mExchageContext->Close();
        mExchageContext = NULL;
    }

    err = mBinding->NewExchangeContext(mExchageContext);
    SuccessOrExit(err);

    if (0 == mExchageContext->ResponseTimeout)
    {
        mExchageContext->ResponseTimeout = WEAVE_CONFIG_TIME_CLIENT_TIMER_UNICAST_MSEC;
    }
    mExchageContext->OnResponseTimeout = HandleResponseTimeout;
    mExchageContext->OnMessageReceived = HandleSyncResponse;
    mExchageContext->AppState = this;

    // acquire unadjusted timestamp
    err = Platform::Time::GetMonotonicRawTime(&mUnadjTimestampLastSent_usec);

    // send out the request
    err = mExchageContext->SendMessage(kWeaveProfile_Time, kTimeMessageType_TimeSyncRequest, msgBuf,
        ExchangeContext::kSendFlag_ExpectResponse);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    // There is no need to release mBinding nor mExchageContext,
    // as the caller for this routine would needs its own error handling

    return err;
}

void SingleSourceTimeSyncClient::HandleSyncResponse(ExchangeContext *aEC, const IPPacketInfo *aPktInfo,
    const WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload)
{
    SingleSourceTimeSyncClient * const client = reinterpret_cast<SingleSourceTimeSyncClient *>(aEC->AppState);
    client->OnSyncResponse(aProfileId, aMsgType, aPayload);
}

void SingleSourceTimeSyncClient::RegisterSyncResultIfNewOrBetter(const timesync_t aNow_usec, const timesync_t aRemoteTimestamp_usec, const int32_t aFlightTime_usec)
{
    WeaveLogDetail(TimeService, "[%4.4s] Flight time: %" PRId32 ", server utc time: %" PRId64,
            GetClientStateName(), aFlightTime_usec, mRemoteTimestamp_usec);

    if ((!IsRegisteredResultValid()) || (aFlightTime_usec < mFlightTime_usec))
    {
        if (!IsRegisteredResultValid())
        {
            WeaveLogDetail(TimeService, "[%4.4s] Registering new result", GetClientStateName());
        }
        else if (aFlightTime_usec < mFlightTime_usec)
        {
            WeaveLogDetail(TimeService, "[%4.4s] Replacing with better result", GetClientStateName());
        }

        mRemoteTimestamp_usec = aRemoteTimestamp_usec;
        mFlightTime_usec = aFlightTime_usec;
        mRegisterSyncResult_usec = aNow_usec;
    }
}

void SingleSourceTimeSyncClient::FinalProcessing()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    timesync_t now_usec;

    // If we have a valid flight time, we have some valid result (from either of the attempts).
    // This is because mFlightTime_usec is only set to valid, non-negative value in RegisterSyncResultIfNewOrBetter.
    // We also need the current time in case we're applying result from the first attempt.
    // Note that mRegisterSyncResult_usec would be very close to now_usec if we're applying result from the current attempt,
    // but special casing it would require more logic/code.
    if (IsRegisteredResultValid())
    {
        timesync_t correctedSystemTime_usec;

        err = Platform::Time::GetMonotonicRawTime(&now_usec);
        SuccessOrExit(err);

        WeaveLogDetail(TimeService, "Now (monotonic raw): %" PRId64 " usec", now_usec);

        // Let's calculate the fix and call back to app layer
        correctedSystemTime_usec = mRemoteTimestamp_usec + mFlightTime_usec + (now_usec - mRegisterSyncResult_usec);

        WeaveLogDetail(TimeService, "(Best result) Remote time: %" PRId64 " usec", mRemoteTimestamp_usec);
        WeaveLogDetail(TimeService, "(Best result) Avg flight time: %" PRId64 " usec", mFlightTime_usec);
        WeaveLogDetail(TimeService, "(Best result) Registered at: %" PRId64 " usec", mRegisterSyncResult_usec);
        WeaveLogDetail(TimeService, "(Best result) Was taken at: %" PRId64 " usec ago", now_usec - mRegisterSyncResult_usec);

        mIsInCallback = true;
        mOnSyncCompleted(mApp, WEAVE_NO_ERROR, correctedSystemTime_usec);
        mIsInCallback = false;
        // After the callback, clean up resources
        Abort();
    }
    else
    {
        // inform the app layer that we do not have any valid result to report
        _AbortWithCallback(WEAVE_ERROR_INVALID_TIME);
    }

exit:
    WeaveLogFunctError(err);

    if (WEAVE_NO_ERROR != err)
    {
        // inform the app layer that we just completed with error
        _AbortWithCallback(err);
    }
}

void SingleSourceTimeSyncClient::EnterSync2()
{
    // Let's try again. Any error would trigger a short cut to conclusion
    SetClientState(kClientState_Sync_2);

    if (WEAVE_NO_ERROR != SendSyncRequest())
    {
        WeaveLogDetail(TimeService, "Failed sending out 2nd request. Proceed with final processing");

        FinalProcessing();
    }
}

void SingleSourceTimeSyncClient::OnSyncResponse(uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TimeSyncResponse response;
    timesync_t now_usec;
    timesync_t serverProcessingTime_usec;
    timesync_t roundTripTime_usec;
    timesync_t sumFlightTime64_usec;
    int32_t sumFlightTime32_usec;
    int32_t averageFlightTime_usec;

    err = Platform::Time::GetMonotonicRawTime(&now_usec);
    SuccessOrExit(err);

    VerifyOrExit((kWeaveProfile_Time == aProfileId)
            && (kTimeMessageType_TimeSyncResponse == aMsgType),
            err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    err = TimeSyncResponse::Decode(&response, aPayload);
    SuccessOrExit(err);

    serverProcessingTime_usec = response.mTimeOfResponse - response.mTimeOfRequest;
    roundTripTime_usec = now_usec - mUnadjTimestampLastSent_usec;
    sumFlightTime64_usec = roundTripTime_usec - serverProcessingTime_usec;

    WeaveLogDetail(TimeService, "Now (monotonic raw): %" PRId64 " usec", now_usec);
    WeaveLogDetail(TimeService, "Time of request:  %" PRId64 " usec", response.mTimeOfRequest);
    WeaveLogDetail(TimeService, "Time of response: %" PRId64 " usec", response.mTimeOfResponse);
    WeaveLogDetail(TimeService, "Server processing time: %" PRId64 " usec", serverProcessingTime_usec);
    WeaveLogDetail(TimeService, "Round trip time: %" PRId64 " usec", roundTripTime_usec);
    WeaveLogDetail(TimeService, "Sum flight time: %" PRId64 " usec", sumFlightTime64_usec);

    VerifyOrExit((serverProcessingTime_usec >= 0)
            && (roundTripTime_usec >= 0)
            && (roundTripTime_usec <= WEAVE_CONFIG_TIME_CLIENT_MAX_RTT_USEC)
            && (sumFlightTime64_usec >= 0)
            && (sumFlightTime64_usec <= INT32_MAX),
            err = WEAVE_ERROR_INVALID_TIME);

    // note that these values shall never be negative
    sumFlightTime32_usec = static_cast<int32_t>(sumFlightTime64_usec);
    averageFlightTime_usec = sumFlightTime32_usec / 2;

    WeaveLogDetail(TimeService, "Average flight time: %" PRId32 " usec", averageFlightTime_usec);

    // remember this result only if it is the first valid result or better than the existing one
    RegisterSyncResultIfNewOrBetter(now_usec, response.mTimeOfResponse, averageFlightTime_usec);

exit:
    WeaveLogFunctError(err);

    // release the payload no matter what
    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    // close the incoming exchange context no matter what after the first response we get
    if (NULL != mExchageContext)
    {
        mExchageContext->Close();
        mExchageContext = NULL;
    }

    ProceedToNextState();
}

void SingleSourceTimeSyncClient::OnResponseTimeout(void)
{
    WeaveLogDetail(TimeService, "Timed out at client state: %d (%s)", GetClientState(),
        GetClientStateName());

    if (NULL != mExchageContext)
    {
        mExchageContext->Abort();
        mExchageContext = NULL;
    }

    ProceedToNextState();
}

void SingleSourceTimeSyncClient::ProceedToNextState(void)
{
    if (kClientState_Sync_1 == mClientState)
    {
        // Note that we are entering Sync 2 no matter what error was encountered
        EnterSync2();
    }
    else if (kClientState_Sync_2 == mClientState)
    {
        // Note that we are performing final processing no matter what error was encountered
        FinalProcessing();
    }
    else
    {
        // make a callback to app
        _AbortWithCallback(WEAVE_ERROR_INCORRECT_STATE);
    }
}

void SingleSourceTimeSyncClient::HandleResponseTimeout(ExchangeContext *aEC)
{
    // assume aEC == mExchageContext
    SingleSourceTimeSyncClient * const client = reinterpret_cast<SingleSourceTimeSyncClient *>(aEC->AppState);
    client->OnResponseTimeout();
}

void SingleSourceTimeSyncClient::_AbortWithCallback(const WEAVE_ERROR aErrorCode)
{
    WeaveLogDetail(TimeService, "Abort at client state: %d (%s)", GetClientState(),
        GetClientStateName());

    if (NULL != mOnSyncCompleted)
    {
        mIsInCallback = true;
        mOnSyncCompleted(mApp, aErrorCode, TIMESYNC_INVALID);
        mIsInCallback = false;
    }

    Abort();
}

void SingleSourceTimeSyncClient::HandleTimeChangeNotification(ExchangeContext *aEC, const IPPacketInfo *aPktInfo,
    const WeaveMessageInfo *aMsgInfo,
    uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload)
{
    SingleSourceTimeSyncClient * const client = reinterpret_cast<SingleSourceTimeSyncClient *>(aEC->AppState);

    WeaveLogDetail(TimeService, "Time Change Notification: peer node ID: 0x%" PRIX64, aEC->PeerNodeId);

    PacketBuffer::Free(aPayload);
    aPayload = NULL;

    aEC->Close();
    aEC = NULL;

    if (NULL != client->OnTimeChangeNotificationReceived)
    {
        // this is a special callback
        // note we don't have the mIsInCallback protection around it
        // the reason is to enable calling Sync family functions
        // within this callback from the app layer, which might be
        // easier for the application layer to use
        client->OnTimeChangeNotificationReceived(client->mApp, aEC);
    }
}

const char * const SingleSourceTimeSyncClient::GetClientStateName(void) const
{
    const char * stateName = NULL;

    switch (mClientState)
    {
    case kClientState_Idle:
        stateName = "Idle";
        break;
    case kClientState_Sync_1:
        stateName = "Syn1";
        break;
    case kClientState_Sync_2:
        stateName = "Syn2";
        break;
    default:
        stateName = "N/A";
        break;
    }

    return stateName;
}

#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
#endif // WEAVE_CONFIG_TIME

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
 *      This file contains implementation of the TimeSyncServer class used in Time Services
 *      WEAVE_CONFIG_TIME must be defined if Time Services are needed
 *
 */

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX to be defined for pre-C++11 clib
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

using namespace nl::Weave::Profiles::Time;

TimeSyncNode::TimeSyncNode() :

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    // Server data section
    OnSyncRequestReceived(NULL),
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    // Client data section
    OnTimeChangeNotificationReceived(NULL),
    FilterTimeCorrectionContributor(NULL),
    OnSyncSucceeded(NULL),
    OnSyncFailed(NULL),
    mEncryptionType(kWeaveEncryptionType_None),
    mKeyId(WeaveKeyId::kNone),
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

    // General data section
    mApp(NULL),
    mRole(kTimeSyncRole_Unknown),
    mIsInCallback(false)

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    // Server data section
    ,
    mServerState(kServerState_Uninitialized),
    mIsAlwaysFresh(false),
    mNumContributorInLastLocalSync(0),
    mTimestampLastCorrectionFromServerOrNtp_usec(TIMESYNC_INVALID),
    mTimestampLastLocalSync_usec(TIMESYNC_INVALID)
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    // Client data section
    ,
    mClientState(kClientState_Uninitialized),
    mIsAutoSyncEnabled(false),
    mSyncPeriod_msec(0)

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    ,
    mIsUrgentDiscoveryPending(false),
    mNominalDiscoveryPeriod_msec(0),
    mShortestDiscoveryPeriod_msec(0),
    mBootTimeForNextAutoDiscovery_usec(TIMESYNC_INVALID)
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    ,
    mConnectionToService(NULL)
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    ,
    mActiveContact(NULL),
    mExchageContext(NULL),
    mUnadjTimestampLastSent_usec(TIMESYNC_INVALID)

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    ,
    mLastLikelihoodSent(TimeSyncRequest::kLikelihoodForResponse_Min)
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

{
    // It seems constructor could be skipped on some products
    // for static objects, and only memory zeroed-out
    // so it's important that the initialization list
    // has similar results as InitState
}

WEAVE_ERROR TimeSyncNode::InitState(const TimeSyncRole aRole,
    void * const aApp, WeaveExchangeManager * const aExchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    if (kServerState_Uninitialized != mServerState)
    {
        // this function can only be called once
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    if (kClientState_Uninitialized != mClientState)
    {
        // this function can only be called once
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

    // now we know we haven't been initialized before, initialize all members
    // as if they are properly initialized through constructor
    // the reason is on some platforms, constructors for global static objects could be
    // skipped
    ClearState();

    // Base class
    _TimeSyncNodeBase::Init(aExchangeMgr->FabricState, aExchangeMgr);

    // General data section
    mApp = aApp;
    mRole = aRole;
    mIsInCallback = false;

exit:
    WeaveLogFunctError(err);

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    mServerState = (WEAVE_NO_ERROR == err) ? kServerState_Constructed : kServerState_InitializationFailed;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    if (WEAVE_NO_ERROR == err)
    {
        SetClientState(kClientState_Constructed);
    }
    else
    {
        SetClientState(kClientState_InitializationFailed);
    }
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

    return err;
}

void TimeSyncNode::ClearState(void)
{
#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    // Server data section
    OnSyncRequestReceived = NULL;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    // Client data section
    OnTimeChangeNotificationReceived = NULL;
    FilterTimeCorrectionContributor = NULL;
    OnSyncSucceeded = NULL;
    OnSyncFailed = NULL;
    mEncryptionType = kWeaveEncryptionType_None;
    mKeyId = WeaveKeyId::kNone;
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

    // General data section
    mApp = NULL;
    mRole = kTimeSyncRole_Unknown;
    mIsInCallback = false;

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    // Server data section
    mServerState = kServerState_Uninitialized;
    mIsAlwaysFresh = false;
    mNumContributorInLastLocalSync = 0;
    mTimestampLastCorrectionFromServerOrNtp_usec = TIMESYNC_INVALID;
    mTimestampLastLocalSync_usec = TIMESYNC_INVALID;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    // Client data section
    mClientState = kClientState_Uninitialized;
    mIsAutoSyncEnabled = false;
    mSyncPeriod_msec = 0;
    mActiveContact = NULL;
    mExchageContext = NULL;
    mUnadjTimestampLastSent_usec = TIMESYNC_INVALID;

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    mIsUrgentDiscoveryPending = false;
    mNominalDiscoveryPeriod_msec = 0;
    mShortestDiscoveryPeriod_msec = 0;
    mBootTimeForNextAutoDiscovery_usec = TIMESYNC_INVALID;
    mLastLikelihoodSent = TimeSyncRequest::kLikelihoodForResponse_Min;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    mConnectionToService = NULL;
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT
}

WEAVE_ERROR TimeSyncNode::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    switch (mRole)
    {
#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    case kTimeSyncRole_Server:
        err = _ShutdownServer();
        break;
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT
    case kTimeSyncRole_Client:
        err = _ShutdownClient();
        break;
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
    case kTimeSyncRole_Coordinator:
        err = _ShutdownCoordinator();
        break;
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

    default:
        err = WEAVE_ERROR_INCORRECT_STATE;
        break;
    }

    ClearState();

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
WEAVE_ERROR TimeSyncNode::InitServer(void * const aApp, WeaveExchangeManager * const aExchangeMgr,
    const bool aIsAlwaysFresh)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // initialize general data
    err = InitState(kTimeSyncRole_Server, aApp, aExchangeMgr);
    SuccessOrExit(err);

    // initialize Server-specific data
    err = _InitServer(aIsAlwaysFresh);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR TimeSyncNode::_InitServer(const bool aIsAlwaysFresh)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mIsAlwaysFresh = aIsAlwaysFresh;
    mNumContributorInLastLocalSync = 0;
    mTimestampLastCorrectionFromServerOrNtp_usec = TIMESYNC_INVALID;
    mTimestampLastLocalSync_usec = TIMESYNC_INVALID;

    // Register to receive unsolicited time sync request messages from the exchange manager.
    err = GetExchangeMgr()->RegisterUnsolicitedMessageHandler(kWeaveProfile_Time, kTimeMessageType_TimeSyncRequest,
        HandleSyncRequest, this);
    SuccessOrExit(err);

    err = GetExchangeMgr()->MessageLayer->SystemLayer->StartTimer(WEAVE_CONFIG_TIME_SERVER_TIMER_UNRELIABLE_AFTER_BOOT_MSEC,
        HandleUnreliableAfterBootTimer, this);
    SuccessOrExit(err);

    if (aIsAlwaysFresh)
    {
        // only "always fresh" servers need this timer
        // so they are not fresh right after boot, and then become always fresh
        mServerState = kServerState_UnreliableAfterBoot;

        WEAVE_TIME_PROGRESS_LOG(TimeService, "Unreliable-After-Boot timer armed for %u msec",
            WEAVE_CONFIG_TIME_SERVER_TIMER_UNRELIABLE_AFTER_BOOT_MSEC);
    }
    else
    {
        // "not always fresh" servers don't need a timer to indicate that their time is not fresh
        mServerState = kServerState_Idle;

        WEAVE_TIME_PROGRESS_LOG(TimeService, "Server entered IDLE state, reason 1");
    }

exit:
    WeaveLogFunctError(err);
    if (WEAVE_NO_ERROR != err)
    {
        mServerState = kServerState_InitializationFailed;
    }

    return err;
}

WEAVE_ERROR TimeSyncNode::_ShutdownServer(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // unregister message handler
    err = GetExchangeMgr()->UnregisterUnsolicitedMessageHandler(kWeaveProfile_Time, kTimeMessageType_TimeSyncRequest);
    WeaveLogFunctError(err);

    // unregister timer handler
    // note this function doesn't complain even if the timer has not been registered, and there is no return value
    GetExchangeMgr()->MessageLayer->SystemLayer->CancelTimer(HandleUnreliableAfterBootTimer, this);

exit:
    WeaveLogFunctError(err);
    mServerState = (WEAVE_NO_ERROR == err) ? kServerState_ShutdownCompleted : kServerState_ShutdownFailed;

    return err;
}

void TimeSyncNode::HandleUnreliableAfterBootTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TimeSyncNode * const server = reinterpret_cast<TimeSyncNode *>(aAppState);

    if (kServerState_UnreliableAfterBoot != server->mServerState)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    server->mServerState = kServerState_Idle;

    WEAVE_TIME_PROGRESS_LOG(TimeService, "Server entered IDLE state, reason 2");

exit:
    WeaveLogFunctError(err);

    return;
}

void TimeSyncNode::HandleSyncRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
    const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR             err         = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf      = NULL;
    bool                    shouldReply = false;
    TimeSyncNode* const     server      = reinterpret_cast<TimeSyncNode*>(ec->AppState);

    // note that a Server doesn't check the encryption/auth type of the request,
    // but just sends back the response using the same context

    WeaveLogDetail(TimeService, "Time Sync Request: local node ID: %" PRIX64 ", peer node ID: %" PRIX64,
        server->GetFabricState()->LocalNodeId, ec->PeerNodeId);

    // ignore requests coming from our own node ID
    // this is because some network stacks would be looped back multicasts
    if (server->GetFabricState()->LocalNodeId == ec->PeerNodeId)
    {
        // ignore request
    }
    // decode request and determine if we should reply
    else
    {
        TimeSyncRequest request;

        // check internal state
        // only try to decode and then respond if we're in any of these two states
        if ((kServerState_UnreliableAfterBoot != server->mServerState)
            && (kServerState_Idle != server->mServerState))
        {
            ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        }

        err = TimeSyncRequest::Decode(&request, payload);
        SuccessOrExit(err);

        if (NULL != server->OnSyncRequestReceived)
        {
            shouldReply = server->OnSyncRequestReceived(server->mApp, msgInfo,
                request.mLikelihoodForResponse, request.mIsTimeCoordinator);
        }
        else if (request.mLikelihoodForResponse == TimeSyncRequest::kLikelihoodForResponse_Max)
        {
            shouldReply = true;
        }
        else
        {
            // get a random number uniformly distributed among [0, kLikelihoodForResponse_Max]
            // note this method is simple and common but the result is not uniformly distributed!
            // also we are assuming that srand() has been called somewhere using proper seed!
            const int8_t dice = int8_t(rand() % (TimeSyncRequest::kLikelihoodForResponse_Max + 1));

            shouldReply = request.mLikelihoodForResponse >= dice;
        }
    }

    if (shouldReply)
    {
        TimeSyncResponse response;
        uint16_t timeSinceLastSyncWithServer_min = TimeSyncResponse::kTimeSinceLastSyncWithServer_Invalid;
        timesync_t timeSinceLastLocaSync_usec;

        // obtain unadjusted timestamp (note zero-initializer is skipped to save code space)
        // note it has to be boot time as we need compensation for sleep time
        timesync_t unadjTimestamp_usec;
        err = Platform::Time::GetSleepCompensatedMonotonicTime(&unadjTimestamp_usec);
        SuccessOrExit(err);

        timeSinceLastLocaSync_usec = unadjTimestamp_usec - server->mTimestampLastLocalSync_usec;
        if ((TIMESYNC_INVALID == server->mTimestampLastLocalSync_usec) ||
            (timeSinceLastLocaSync_usec >= (3600 * 1000000LL)))
        {
            server->mNumContributorInLastLocalSync = 0;
        }

        if (server->mIsAlwaysFresh)
        {
            if (kServerState_UnreliableAfterBoot != server->mServerState)
            {
                timeSinceLastSyncWithServer_min = 0;
                WeaveLogDetail(TimeService, "Server is always fresh and has passed initial phase");
            }
            else
            {
                // invalid age
                WeaveLogDetail(TimeService, "Server is still unreliable after boot");
            }
        }
        else
        {
            if (TIMESYNC_INVALID != server->mTimestampLastCorrectionFromServerOrNtp_usec)
            {
                const timesync_t age_min = Platform::Divide(
                    (unadjTimestamp_usec - server->mTimestampLastCorrectionFromServerOrNtp_usec), (60 * 1000000));

                if (age_min < TimeSyncResponse::kTimeSinceLastSyncWithServer_Max)
                {
                    timeSinceLastSyncWithServer_min = uint16_t(age_min);

                    WeaveLogDetail(TimeService, "Returning age %d min", timeSinceLastSyncWithServer_min);
                }
                else
                {
                    // invalid age
                    WeaveLogDetail(TimeService, "Server synced with reliable source too long ago");
                }
            }
            else
            {
                // invalid age;
                WeaveLogDetail(TimeService, "Server hasn't synced with reliable source");
            }
        }

        // obtain system time (note zero-initializer is skipped to save code space)
        timesync_t systemTimestamp_usec;
        err = Platform::Time::GetSystemTime(&systemTimestamp_usec);
        SuccessOrExit(err);

        // create sync response based on system time
        response.Init(server->mRole, systemTimestamp_usec, systemTimestamp_usec,
            server->mNumContributorInLastLocalSync, timeSinceLastSyncWithServer_min);

        // allocate buffer and then encode the response into it
        msgBuf = PacketBuffer::New();
        if (NULL == msgBuf)
        {
            ExitNow(err = WEAVE_ERROR_NO_MEMORY);
        }

        err = response.Encode(msgBuf);
        SuccessOrExit(err);

        // send out the response
        err = ec->SendMessage(kWeaveProfile_Time, kTimeMessageType_TimeSyncResponse, msgBuf);
        msgBuf = NULL;
        SuccessOrExit(err);
    }
    else
    {
        // ignore the request
        WeaveLogDetail(TimeService, "Time sync request ignored");
    }

exit:
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
    }
    // close the exchange context no matter what
    if (NULL != ec)
    {
        ec->Close();
    }
    WeaveLogFunctError(err);
}

TimeSyncNode::ServerState TimeSyncNode::GetServerState(void) const
    {
    return mServerState;
}

void TimeSyncNode::RegisterCorrectionFromServerOrNtp(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    (void) Platform::Time::GetSleepCompensatedMonotonicTime(&mTimestampLastCorrectionFromServerOrNtp_usec);

exit:
    WeaveLogFunctError(err);
}

void TimeSyncNode::RegisterLocalSyncOperation(const uint8_t aNumContributor)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    (void) Platform::Time::GetSleepCompensatedMonotonicTime(&mTimestampLastLocalSync_usec);
    mNumContributorInLastLocalSync = aNumContributor;

exit:
    WeaveLogFunctError(err);
}

void TimeSyncNode::MulticastTimeChangeNotification(const uint8_t aEncryptionType, const uint16_t aKeyId) const
    {
    WEAVE_ERROR             err             = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf          = NULL;
    ExchangeContext*        ec              = NULL;
    TimeChangeNotification  notification;

    if (mIsInCallback)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // Create a new exchange context, targeting all nodes
    ec = GetExchangeMgr()->NewContext(nl::Weave::kAnyNodeId);
    if (NULL == ec)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    // Configure the encryption and key used to send the request
    ec->EncryptionType = aEncryptionType;
    ec->KeyId = aKeyId;

    // allocate buffer and then encode the response into it
    msgBuf = PacketBuffer::New();
    if (NULL == msgBuf)
    {
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);
    }

    err = notification.Encode(msgBuf);
    SuccessOrExit(err);

    // send out the request
    err = ec->SendMessage(kWeaveProfile_Time, kTimeMessageType_TimeSyncTimeChangeNotification, msgBuf);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);
    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
    }
    if (NULL != ec)
    {
        ec->Close();
    }
}
#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER

#endif // WEAVE_CONFIG_TIME

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
 *    @file
 *      This file implements subscription client for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Support/WeaveFaultInjection.h>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/FibonacciUtils.h>
#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

// Do nothing
SubscriptionClient::SubscriptionClient ()
{
}

void SubscriptionClient::InitAsFree ()
{
    mCurrentState = kState_Free;
    mRefCount = 0;
    Reset();
}

void SubscriptionClient::Reset(void)
{
    mBinding = NULL;
    mEC = NULL;
    mAppState = NULL;
    mEventCallback = NULL;
    mResubscribePolicyCallback = NULL;
    mDataSinkCatalog = NULL;
    mInactivityTimeoutDuringSubscribingMsec = kNoTimeOut;
    mLivenessTimeoutMsec = kNoTimeOut;
    mSubscriptionId = 0;
    mIsInitiator = false;
    mRetryCounter = 0;

#if WDM_ENABLE_PROTOCOL_CHECKS
    mPrevTraitDataHandle = -1;
#endif

    mPrevIsPartialChange = false;
}

// AddRef to Binding
// store pointers to binding and delegate
// null out EC
WEAVE_ERROR SubscriptionClient::Init (Binding * const apBinding,
    void * const apAppState,
    EventCallback const aEventCallback,
    const TraitCatalogBase<TraitDataSink>* const apCatalog,
    const uint32_t aInactivityTimeoutDuringSubscribingMsec)
{
    WeaveLogIfFalse(0 == mRefCount);

    // add reference to the binding
    apBinding->AddRef();

    // make a copy of the pointers
    mBinding = apBinding;
    mAppState = apAppState;
    mEventCallback = aEventCallback;
    mDataSinkCatalog = apCatalog;
    mInactivityTimeoutDuringSubscribingMsec = aInactivityTimeoutDuringSubscribingMsec;

    MoveToState(kState_Initialized);

    _AddRef ();

    return WEAVE_NO_ERROR;
}

#if WEAVE_DETAIL_LOGGING
const char * SubscriptionClient::GetStateStr() const
{
    switch (mCurrentState)
    {
    case kState_Free:
        return "FREE";

    case kState_Initialized:
        return "INIT";

    case kState_Subscribing:
        return "SReq1";

    case kState_Subscribing_IdAssigned:
        return "SReq2";

    case kState_SubscriptionEstablished_Idle:
        return "ALIVE";

    case kState_SubscriptionEstablished_Confirming:
        return "CONFM";

    case kState_Canceling:
        return "CANCL";

    case kState_Resubscribe_Holdoff:
        return "RETRY";

    case kState_Aborting:
        return "ABTNG";

    case kState_Aborted:
        return "ABORT";
    }
    return "N/A";
}
#else // WEAVE_DETAIL_LOGGING
const char * SubscriptionClient::GetStateStr() const
{
    return "N/A";
}
#endif // WEAVE_DETAIL_LOGGING

void SubscriptionClient::MoveToState(const ClientState aTargetState)
{
    mCurrentState = aTargetState;
    WeaveLogDetail(DataManagement, "Client[%u] moving to [%5.5s] Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), mRefCount);

#if WEAVE_DETAIL_LOGGING
    if (kState_Free == mCurrentState)
    {
        SubscriptionEngine::GetInstance()->LogSubscriptionFreed();
    }
#endif // #if WEAVE_DETAIL_LOGGING
}

/**
 * @brief Enable automatic resubscribes. Attach a callback to specify
 * the next retry time on failure.
 *
 * @param aCallback[in]     Optional callback to fetch the amount of time to
 *                          wait before retrying after a failure. If NULL use
 *                          a default policy.
 */
void SubscriptionClient::EnableResubscribe(ResubscribePolicyCallback aCallback)
{
    if (aCallback)
    {
        mResubscribePolicyCallback = aCallback;
    }
    else
    {
        mResubscribePolicyCallback = DefaultResubscribePolicyCallback;
    }
}

/**
 * @brief Disable the resubscribe mechanism. This will abort if a resubscribe
 * was pending.
 */
void SubscriptionClient::DisableResubscribe(void)
{
    mResubscribePolicyCallback = NULL;

    if (mCurrentState == kState_Resubscribe_Holdoff)
    {
        // cancel timer
        SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->
                SystemLayer->CancelTimer(OnTimerCallback, this);

        // app doesn't need to know since it triggered this
        AbortSubscription();
    }
}

/**
 * @brief Kick the resubscribe mechanism. This will initiate an immediate retry
 */
void SubscriptionClient::ResetResubscribe(void)
{
    if (mCurrentState == kState_Resubscribe_Holdoff)
    {
        // cancel timer
        SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->
                SystemLayer->CancelTimer(OnTimerCallback, this);
        MoveToState(kState_Initialized);
    }

    mRetryCounter = 0;

    if (mCurrentState == kState_Initialized || mCurrentState == kState_Resubscribe_Holdoff)
    {
        SetRetryTimer(WEAVE_NO_ERROR);
    }
}


WEAVE_ERROR SubscriptionClient::GetSubscriptionId (uint64_t * const apSubscriptionId)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;

    *apSubscriptionId = 0;

    switch (mCurrentState)
    {
    case kState_Subscribing_IdAssigned:
    case kState_SubscriptionEstablished_Idle:
    case kState_SubscriptionEstablished_Confirming:
    case kState_Canceling:
        *apSubscriptionId = mSubscriptionId;
        ExitNow();
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void SubscriptionClient::DefaultEventHandler(EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam)
{
    IgnoreUnusedVariable(aInParam);
    IgnoreUnusedVariable(aOutParam);

    WeaveLogDetail(DataManagement, "%s event: %d", __func__, aEvent);
}

/**
 * @brief The default policy implementation will pick a random timeslot
 * with millisecond resolution over an ever increasing window,
 * following a fibonacci sequence upto WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX.
 * Average of the randomized wait time past the WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX
 * will be around one hour.
 * When the retry count resets to 0, the sequence starts from the beginning again.
 */

void SubscriptionClient::DefaultResubscribePolicyCallback(void * const aAppState,
                                                    ResubscribeParam & aInParam,
                                                    uint32_t & aOutIntervalMsec)
{
    IgnoreUnusedVariable(aAppState);

    uint32_t fibonacciNum = 0;
    uint32_t maxWaitTimeInMsec = 0;
    uint32_t waitTimeInMsec = 0;
    uint32_t minWaitTimeInMsec = 0;

    if (aInParam.mNumRetries <= WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX)
    {
        fibonacciNum = GetFibonacciForIndex(aInParam.mNumRetries);
        maxWaitTimeInMsec = fibonacciNum * WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS;
    }
    else
    {
        maxWaitTimeInMsec = WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS;
    }

    if (maxWaitTimeInMsec != 0)
    {
        minWaitTimeInMsec = (WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP * maxWaitTimeInMsec) / 100;
        waitTimeInMsec = minWaitTimeInMsec + (GetRandU32() % (maxWaitTimeInMsec - minWaitTimeInMsec));
    }

    aOutIntervalMsec = waitTimeInMsec;

    WeaveLogDetail(DataManagement, "Computing resubscribe policy: attempts %" PRIu32 ", max wait time %" PRIu32 " ms, selected wait time %" PRIu32 " ms",
        aInParam.mNumRetries, maxWaitTimeInMsec, waitTimeInMsec);

    return;
}

void SubscriptionClient::_InitiateSubscription(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    // Make sure the client object is not freed during the callback to the application.
    _AddRef();

    VerifyOrExit(kState_Subscribing != mCurrentState && kState_Subscribing_IdAssigned != mCurrentState, /* no-op */);

    VerifyOrExit(kState_Initialized == mCurrentState, err = WEAVE_ERROR_INCORRECT_STATE);

    // Set the protocol callback on the binding object.  NOTE: This should only happen once the
    // app has explicitly started the subscription process by calling either InitiateSubscription() or
    // InitiateCounterSubscription().  Otherwise the client object might receive callbacks from
    // the binding before its ready.
    mBinding->SetProtocolLayerCallback(BindingEventCallback, this);

#if WDM_ENABLE_PROTOCOL_CHECKS
    mPrevTraitDataHandle = -1;
#endif

    mPrevIsPartialChange = false;

    // If the binding is ready...
    if (mBinding->IsReady())
    {
        // Using the binding, form and send a SubscribeRequest to the publisher.
        err = SendSubscribeRequest();
        SuccessOrExit(err);

        // Enter the Subscribing state.
        if (mIsInitiator)
        {
            MoveToState(kState_Subscribing);
        }
        else
        {
            MoveToState(kState_Subscribing_IdAssigned);
        }

        err = RefreshTimer();
        SuccessOrExit(err);
    }

    // Otherwise, if the binding needs to be prepared...
    else if (mBinding->CanBePrepared())
    {
        // Ask the application prepare the binding by delivering a PrepareRequested API event to it via the
        // binding's callback.  At some point the binding will callback into the SubscriptionClient signaling
        // that preparation has completed (successfully or otherwise).  Note that this callback can happen
        // synchronously within the RequestPrepare() method, implying that _InitiateSubscription() will recurse.
        err = mBinding->RequestPrepare();
        SuccessOrExit(err);
    }

    // Otherwise, verify that the binding is in one of the preparing states.  Once preparation completes, the
    // binding will call back, at which point, if preparation was successful, _InitiateSubscription() will be
    // called again.
    else
    {
        VerifyOrExit(mBinding->IsPreparing(), err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    if (WEAVE_NO_ERROR != err)
    {
        HandleSubscriptionTerminated(IsRetryEnabled(), err, NULL);
    }

    _Release();
}

WEAVE_ERROR SubscriptionClient::SendSubscribeRequest(void)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t         msgType = kMsgType_SubscribeRequest;
    InEventParam inSubscribeParam;
    OutEventParam outSubscribeParam;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    inSubscribeParam.Clear();
    outSubscribeParam.Clear();

    outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList = NULL;
    outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList = NULL;

    inSubscribeParam.mSubscribeRequestPrepareNeeded.mClient = this;
    mEventCallback(mAppState, kEvent_OnSubscribeRequestPrepareNeeded,
            inSubscribeParam, outSubscribeParam);

    if (!mIsInitiator)
    {
        mSubscriptionId = outSubscribeParam.mSubscribeRequestPrepareNeeded.mSubscriptionId;
    }

    VerifyOrExit(kState_Initialized == mCurrentState,
                 err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit((outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin <= kMaxTimeoutSec) ||
                 (kNoTimeOut == outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin),
                 err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit((outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax <= kMaxTimeoutSec) ||
                 (kNoTimeOut == outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax),
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    {
        nl::Weave::TLV::TLVWriter writer;
        SubscribeRequest::Builder request;
        writer.Init(msgBuf);

        err = request.Init(&writer);
        SuccessOrExit(err);

        if (kNoTimeOut != outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin)
        {
            request.SubscribeTimeoutMin(outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin);
        }
        if (kNoTimeOut != outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax)
        {
            request.SubscribeTimeoutMax(outSubscribeParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax);
        }
        if (!mIsInitiator)
        {
            request.SubscriptionID(mSubscriptionId);
        }

        // It's safe to bail out after a series of operation, for
        // SubscriptionRequest::Builder would internally turn to NOP after error is logged
        SuccessOrExit(err = request.GetError());

        {
            PathList::Builder & pathList = request.CreatePathListBuilder();

            for (size_t i = 0; i < outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathListSize; ++i)
            {
                TraitDataSink *pDataSink;
                nl::Weave::TLV::TLVType dummyContainerType;
                SchemaVersionRange versionIntersection;
                VersionedTraitPath versionedTraitPath;

                // Applications can set either the versioned or non versioned path lists for now. We pick either
                // depending on which is non-NULL. If both are non-NULL, we then select the versioned list.
                if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList) {
                    versionedTraitPath = outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList[i];
                }
                else {
                    versionedTraitPath.mTraitDataHandle = outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mTraitDataHandle;
                    versionedTraitPath.mPropertyPathHandle = outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mPropertyPathHandle;
                }

                err = mDataSinkCatalog->Locate(versionedTraitPath.mTraitDataHandle, &pDataSink);
                SuccessOrExit(err);

                // Start the TLV Path
                err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Path, dummyContainerType);
                SuccessOrExit(err);

                // Start, fill, and close the TLV Structure that contains ResourceID, ProfileID, and InstanceID
                err = mDataSinkCatalog->HandleToAddress(versionedTraitPath.mTraitDataHandle, writer, versionedTraitPath.mRequestedVersionRange);
                SuccessOrExit(err);

                // Append zero or more TLV tags based on the Path Handle
                err = pDataSink->GetSchemaEngine()->MapHandleToPath(versionedTraitPath.mPropertyPathHandle, writer);
                SuccessOrExit(err);

                // Close the TLV Path
                err = writer.EndContainer(dummyContainerType);
                SuccessOrExit(err);
            }

            pathList.EndOfPathList();
            SuccessOrExit(err = pathList.GetError());
        }

        {
            VersionList::Builder & versionList = request.CreateVersionListBuilder();

            for (size_t i = 0; i < outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathListSize; ++i)
            {
                TraitDataSink *pDataSink;
                VersionedTraitPath versionedTraitPath;

                if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList) {
                    versionedTraitPath = outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList[i];
                }
                else {
                    versionedTraitPath.mTraitDataHandle = outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mTraitDataHandle;
                    versionedTraitPath.mPropertyPathHandle = outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mPropertyPathHandle;
                }

                err = mDataSinkCatalog->Locate(versionedTraitPath.mTraitDataHandle, &pDataSink);
                SuccessOrExit(err);

                if (pDataSink->IsVersionValid()) {
                    versionList.AddVersion(pDataSink->GetVersion());
                }
                else
                {
                    versionList.AddNull();
                }
            }

            versionList.EndOfVersionList();
            SuccessOrExit(err = versionList.GetError());
        }

        if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents)
        {
            request.SubscribeToAllEvents(true);

            if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize > 0)
            {

                EventList::Builder & eventList = request.CreateLastObservedEventIdListBuilder();

                for (size_t n = 0; n < outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize; ++n)
                {
                    Event::Builder & event = eventList.CreateEventBuilder();
                    event.SourceId(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mSourceId)
                        .Importance(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mImportance)
                        .EventId(outSubscribeParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList[n].mEventId)
                        .EndOfEvent ();
                    SuccessOrExit(err = event.GetError());
                }
                eventList.EndOfEventList ();
                SuccessOrExit(err = eventList.GetError());
            }
        }

        request.EndOfRequest();
        SuccessOrExit(err = request.GetError());

        err = writer.Finalize();
        SuccessOrExit(err);
    }

    err = ReplaceExchangeContext ();
    SuccessOrExit(err);

    // NOTE: State could be changed in sync error callback by message layer
    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_SendUnsupportedReqMsgType, msgType += 50);

    err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, msgType, msgBuf,
            nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    return err;
}

void SubscriptionClient::InitiateSubscription(void)
{
    mIsInitiator = true;

    if (IsRetryEnabled())
    {
        SetRetryTimer(WEAVE_NO_ERROR);
    }
    else
    {
        _InitiateSubscription();
    }
}

void SubscriptionClient::InitiateCounterSubscription(const uint32_t aLivenessTimeoutSec)
{
    mIsInitiator = false;

    // the liveness timeout spec is given and not part of the subscription setup
    mLivenessTimeoutMsec = aLivenessTimeoutSec * 1000;

    _InitiateSubscription();
}

void SubscriptionClient::_AddRef ()
{
    WeaveLogIfFalse(mRefCount < INT8_MAX);

    ++mRefCount;

    // 0: free
    // 1: in some phase of subscription
    // increase: in downcall to message layer, some callback might come from message layer (send error/connection broken)
    // increase: in callback to app layer
}

void SubscriptionClient::_Release ()
{
    WeaveLogIfFalse(mRefCount > 0);

    --mRefCount;

    if (0 == mRefCount)
    {
        AbortSubscription();

        SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMNext_NumSubscriptionClients);
    }
}

Binding * SubscriptionClient::GetBinding() const
{
    return mBinding;
}

uint64_t SubscriptionClient::GetPeerNodeId(void) const
{
    return (mBinding != NULL) ? mBinding->GetPeerNodeId() : kNodeIdNotSpecified;
}

WEAVE_ERROR SubscriptionClient::ReplaceExchangeContext ()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;

    // Make sure we're not freed by accident.
    _AddRef();

    FlushExistingExchangeContext();

    err = mBinding->NewExchangeContext(mEC);
    SuccessOrExit(err);

    mEC->AppState = this;
    mEC->OnMessageReceived = OnMessageReceivedFromLocallyInitiatedExchange;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError = OnSendError;
    mEC->OnAckRcvd = NULL;

    inParam.mExchangeStart.mEC = mEC;
    inParam.mExchangeStart.mClient = this;

    // NOTE: app layer is not supposed to change state/ref count in this callback
    mEventCallback(mAppState, kEvent_OnExchangeStart, inParam, outParam);

exit:
    WeaveLogFunctError(err);

    _Release ();

    return err;
}

void SubscriptionClient::FlushExistingExchangeContext (const bool aAbortNow)
{
    if (NULL != mEC)
    {
        mEC->AppState = NULL;
        mEC->OnMessageReceived = NULL;
        mEC->OnResponseTimeout = NULL;
        mEC->OnSendError = NULL;
        mEC->OnAckRcvd = NULL;
        if (aAbortNow)
        {
            mEC->Abort();
        }
        else
        {
            mEC->Close();
        }
        mEC = NULL;
    }
}

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
WEAVE_ERROR SubscriptionClient::EndSubscription ()
{
    WEAVE_ERROR                     err         = WEAVE_NO_ERROR;
    PacketBuffer*                   msgBuf      = NULL;
    nl::Weave::TLV::TLVWriter       writer;
    SubscribeCancelRequest::Builder request;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident.
    _AddRef();

    switch (mCurrentState)
    {
    case kState_Subscribing:
        // fall through
    case kState_Subscribing_IdAssigned:

        WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s: subscription not established yet, abort",
                SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__);

        AbortSubscription();

        ExitNow();
        break;

    case kState_SubscriptionEstablished_Confirming:
        // forget we're in the middle of confirmation, as the outcome
        // has become irrelevant
        FlushExistingExchangeContext();
        // fall through
    case kState_SubscriptionEstablished_Idle:
        msgBuf = PacketBuffer::NewWithAvailableSize(request.kBaseMessageSubscribeId_PayloadLen);
        VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

        err = ReplaceExchangeContext ();
        SuccessOrExit(err);

        writer.Init(msgBuf);
        request.Init(&writer);
        err = request.SubscriptionID(mSubscriptionId)
                     .EndOfRequest()
                     .GetError();
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        // NOTE: State could be changed if there is a sync error callback from message layer
        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeCancelRequest,
            msgBuf, nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        msgBuf = NULL;
        SuccessOrExit(err);

        MoveToState(kState_Canceling);
        break;

    // Cancel is not supported in any other state
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    _Release();

    return err;
}

#else // WDM_ENABLE_SUBSCRIPTION_CANCEL

WEAVE_ERROR SubscriptionClient::EndSubscription()
{
    AbortSubscription();
    return WEAVE_NO_ERROR;
}

#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

void SubscriptionClient::AbortSubscription(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const bool nullReference = (0 == mRefCount);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    if (!nullReference)
    {
        // Make sure we're not freed by accident.
        // NOTE: In the last Abort call from _Release, mRefCount is already 0.
        // In that case, we do not need this AddRef/Release pair, and we move to FREE state.
        _AddRef();
    }

    if (kState_Free == mCurrentState)
    {
        // This must not happen
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }
    else if (kState_Aborted == mCurrentState || kState_Aborting == mCurrentState)
    {
        // we're already aborted, so there is nothing else to flush
        ExitNow();
    }
    else
    {
        // This is an intermediate state for external calls during the abort process
        uint64_t peerNodeId = mBinding->GetPeerNodeId();
        uint64_t subscriptionId = mSubscriptionId;
        bool deliverSubTerminatedToCatalog =
            ((NULL != mDataSinkCatalog) &&
             (mCurrentState >= kState_NotifyDataSinkOnAbort_Begin) && (mCurrentState <= kState_NotifyDataSinkOnAbort_End));

        MoveToState(kState_Aborting);

        if (deliverSubTerminatedToCatalog)
        {
            // iterate through the whole catalog and deliver kEventSubscriptionTerminated event
            mDataSinkCatalog->DispatchEvent(TraitDataSink::kEventSubscriptionTerminated, NULL);
        }

        mBinding->SetProtocolLayerCallback(NULL, NULL);

        mBinding->Release();
        mBinding = NULL;

        // Note that ref count is not touched at here, as _Abort doesn't change the ownership
        FlushExistingExchangeContext(true);
        (void)RefreshTimer();

        Reset();

        MoveToState(kState_Aborted);

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        if (!mIsInitiator)
        {
            SubscriptionEngine::GetInstance()->UpdateHandlerLiveness(peerNodeId, subscriptionId, true);
        }
#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER
    }

exit:
    WeaveLogFunctError(err);

    if (nullReference)
    {
        // No one is referencing us, move to FREE
        MoveToState(kState_Free);
    }
    else
    {
        _Release();
    }
}

void SubscriptionClient::HandleSubscriptionTerminated(bool aWillRetry, WEAVE_ERROR aReason,
        StatusReporting::StatusReport *aStatusReportPtr)
{
    void * const pAppState = mAppState;
    EventCallback callbackFunc = mEventCallback;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    _AddRef();

    if (!aWillRetry)
    {
        // Flush most internal states, except for mRefCount and mCurrentState
        // move to kState_Aborted
        AbortSubscription();
    }
    else
    {
        // We do not need to perform a full-fledged subscription
        // abort.  On the other hand, we can safely flush existing
        // exchange context as any communication on that exchange
        // context should be considered an error.
        const bool abortExchangeContext = true;
        FlushExistingExchangeContext(abortExchangeContext);
    }

    if (NULL != callbackFunc)
    {
        InEventParam inParam;
        OutEventParam outParam;

        inParam.Clear();
        outParam.Clear();

        inParam.mSubscriptionTerminated.mReason = aReason;
        inParam.mSubscriptionTerminated.mClient = this;
        inParam.mSubscriptionTerminated.mWillRetry = aWillRetry;
        inParam.mSubscriptionTerminated.mIsStatusCodeValid = (aStatusReportPtr != NULL);
        if (aStatusReportPtr != NULL)
        {
            inParam.mSubscriptionTerminated.mStatusProfileId = aStatusReportPtr->mProfileId;
            inParam.mSubscriptionTerminated.mStatusCode = aStatusReportPtr->mStatusCode;
            inParam.mSubscriptionTerminated.mAdditionalInfoPtr = &(aStatusReportPtr->mAdditionalInfo);
        }

        callbackFunc(pAppState, kEvent_OnSubscriptionTerminated, inParam, outParam);
    }
    else
    {
        WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) app layer callback skipped",
            SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);
    }

    // only set this timer if the app cb hasn't changed our state.
    if (aWillRetry && !IsAborted())
    {
        SetRetryTimer(aReason);
    }

    _Release();
}

void SubscriptionClient::SetRetryTimer(WEAVE_ERROR aReason)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ClientState entryState = mCurrentState;
    ResubscribePolicyCallback entryCb = mResubscribePolicyCallback;

    // this check serves to see whether we already have a timer set
    // and if resubscribes are enabled
    if (entryCb && entryState < kState_Resubscribe_Holdoff)
    {
        uint32_t timeoutMsec = 0;

        _AddRef();

        MoveToState(kState_Resubscribe_Holdoff);

        ResubscribeParam param;
        param.mNumRetries = mRetryCounter;
        param.mReason = aReason;

        mResubscribePolicyCallback(mAppState, param, timeoutMsec);
        VerifyOrExit(mCurrentState != kState_Aborted, /* exit without error */);

        err = SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->
                SystemLayer->StartTimer(timeoutMsec, OnTimerCallback, this);
        SuccessOrExit(err);

        WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) timeout: %u",
            SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount, timeoutMsec);
    }

exit:
    // all errors are considered fatal in this function
    if (err != WEAVE_NO_ERROR)
    {
        HandleSubscriptionTerminated(false, err, NULL);
    }

    if (entryCb && (entryState < kState_Resubscribe_Holdoff))
    {
        _Release();
    }

}

void SubscriptionClient::Free ()
{
    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    WeaveLogIfFalse(kState_Free != mCurrentState);
    WeaveLogIfFalse(mRefCount > 0);

    // Abort the subscription if we're not already aborted
    if (kState_Aborted != mCurrentState)
    {
        AbortSubscription();
    }

    // If mRefCount == 1, _Release would decrement it to 0, call Abort again and move us to FREE state
    _Release();
}

void SubscriptionClient::BindingEventCallback(void * const aAppState,
                                              const Binding::EventType aEvent,
                                              const Binding::InEventParam & aInParam,
                                              Binding::OutEventParam & aOutParam)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient*>(aAppState);

    pClient->_AddRef();

    switch (aEvent)
    {
        case Binding::kEvent_BindingReady:
            // Binding is ready. We can send the subscription req now.
            pClient->_InitiateSubscription();
            break;

        case Binding::kEvent_BindingFailed:
            pClient->SetRetryTimer(aInParam.BindingFailed.Reason);
            break;

        case Binding::kEvent_PrepareFailed:
            // need to prepare again.
            pClient->SetRetryTimer(aInParam.PrepareFailed.Reason);
            break;

        default:
            Binding::DefaultEventHandler(aAppState, aEvent, aInParam, aOutParam);
    }

    pClient->_Release();
}

void SubscriptionClient::OnTimerCallback(System::Layer* aSystemLayer, void *aAppState, System::Error)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient*>(aAppState);

    pClient->TimerEventHandler();
}

WEAVE_ERROR SubscriptionClient::RefreshTimer (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool isTimerNeeded = false;
    uint32_t timeoutMsec = 0;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    // Cancel timer first
    SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->
            SystemLayer->CancelTimer(OnTimerCallback, this);

    // Arm timer according to current state
    switch (mCurrentState)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        if (kNoTimeOut != mInactivityTimeoutDuringSubscribingMsec)
        {
            // Note that loss of range is not expected, as ExchangeManager::Timeout is indeed uint32_t
            timeoutMsec = mInactivityTimeoutDuringSubscribingMsec;
            isTimerNeeded = true;

            WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Set inactivity time limit during subscribing to %" PRIu32 " msec",
                SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount, timeoutMsec);
        }
        break;
    case kState_SubscriptionEstablished_Idle:
        if (kNoTimeOut != mLivenessTimeoutMsec)
        {
            if (mIsInitiator)
            {
                // Calculate margin to reserve for WRM activity, so we send out SubscribeConfirm earlier
                // Note that wrap around could happen, if the system is configured with excessive delays and number of retries
                const nl::Weave::WRMPConfig& defaultWRMPConfig = mBinding->GetDefaultWRMPConfig();
                const uint32_t marginMsec = (defaultWRMPConfig.mMaxRetrans + 1) * defaultWRMPConfig.mInitialRetransTimeout;

                // If the margin is smaller than the desired liveness timeout, set a timer for the difference.
                // Otherwise, set the timer to 0 (which will fire immediately)
                if (marginMsec < mLivenessTimeoutMsec)
                {
                    timeoutMsec = mLivenessTimeoutMsec - marginMsec;
                }
                else
                {
                    // This is a system configuration problem
                    WeaveLogError(DataManagement, "Client[%u] Liveness period (%" PRIu32 " msec) <= margin reserved for WRM (%" PRIu32 " msec)",
                                    SubscriptionEngine::GetInstance()->GetClientId(this), mLivenessTimeoutMsec, marginMsec);

                    ExitNow(err = WEAVE_ERROR_TIMEOUT);
                }
            }
            else
            {
                timeoutMsec = mLivenessTimeoutMsec;
            }
            isTimerNeeded = true;

            WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Set timer for liveness confirmation to %" PRIu32 " msec",
                SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount, timeoutMsec);
        }
        break;
    case kState_SubscriptionEstablished_Confirming:
        // Do nothing
        break;
    case kState_Aborting:
        // Do nothing
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (isTimerNeeded)
    {
        err = SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->
                SystemLayer->StartTimer(timeoutMsec, OnTimerCallback, this);

        VerifyOrExit(WEAVE_SYSTEM_NO_ERROR == err, /* no-op */);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void SubscriptionClient::TimerEventHandler (void)
{
    WEAVE_ERROR     err                         = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf                      = NULL;
    bool            skipTimerCheck              = false;

    if ((0 == mRefCount) ||
        (mCurrentState < kState_TimerTick_Begin) ||
        (mCurrentState > kState_TimerTick_End))
    {
        skipTimerCheck = true;
        ExitNow();
    }

    // Make sure we're not freed by accident
    _AddRef();

    switch (mCurrentState)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Timeout for subscribing phase, abort",
            SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

        ExitNow(err = WEAVE_ERROR_TIMEOUT);
        break;

    case kState_SubscriptionEstablished_Idle:
        if (mIsInitiator)
        {
            WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Confirming liveness",
                SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

            // timeout for liveness check
            nl::Weave::TLV::TLVWriter writer;
            SubscribeConfirmRequest::Builder request;
            msgBuf = PacketBuffer::NewWithAvailableSize(request.kBaseMessageSubscribeId_PayloadLen);
            VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

            err = ReplaceExchangeContext ();
            SuccessOrExit(err);

            writer.Init(msgBuf);
            request.Init(&writer);
            err = request.SubscriptionID(mSubscriptionId)
                         .EndOfRequest()
                         .GetError();
            SuccessOrExit(err);

            err = writer.Finalize();
            SuccessOrExit(err);

            // NOTE: State could be changed if there is a send error callback from message layer
            err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeConfirmRequest,
                msgBuf, nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
            msgBuf = NULL;
            SuccessOrExit(err);

            if (kState_SubscriptionEstablished_Idle == mCurrentState)
            {
                MoveToState(kState_SubscriptionEstablished_Confirming);
            }
            else
            {
                // state has changed, probably because some callback from message layer
                ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
            }
        }
        else
        {
            // We are not the initiator, so we cannot send out the subscribe confirm
            WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Timeout",
                SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

            // abort the subscription as we've timed out
            ExitNow(err = WEAVE_ERROR_TIMEOUT);
        }
        break;

    case kState_Resubscribe_Holdoff:
        mRetryCounter++;

        MoveToState(kState_Initialized);

        _InitiateSubscription();
        break;


    default:
        WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d) Timer event fired at wrong state, ignore",
            SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);
        break;
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (err != WEAVE_NO_ERROR)
    {
        HandleSubscriptionTerminated(IsRetryEnabled(), err, NULL);
    }

    if (!skipTimerCheck)
    {
        _Release();
    }
}

WEAVE_ERROR SubscriptionClient::ProcessDataList(nl::Weave::TLV::TLVReader &aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    // TODO: We currently don't support changes that span multiple notifies, nor changes
    // that get aborted and restarted within the same notify. See WEAV-1586 for more details.
    bool isPartialChange = false;
    uint8_t flags;

    while (WEAVE_NO_ERROR == (err = aReader.Next()))
    {
        nl::Weave::TLV::TLVReader pathReader;

        {
            DataElement::Parser element;

            err = element.Init(aReader);
            SuccessOrExit(err);

            err = element.GetReaderOnPath(&pathReader);
            SuccessOrExit(err);

            isPartialChange = false;
            err = element.GetPartialChangeFlag(&isPartialChange);
            VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );
        }

        TraitDataSink *DataSink;
        TraitDataHandle handle;
        PropertyPathHandle pathHandle;
        SchemaVersionRange versionRange;

        err = mDataSinkCatalog->AddressToHandle(pathReader, handle, versionRange);
        SuccessOrExit(err);

        err = mDataSinkCatalog->Locate(handle, &DataSink);
        SuccessOrExit(err);

        err = DataSink->GetSchemaEngine()->MapPathToHandle(pathReader, pathHandle);
#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
        // if we're not in strict compliance mode, we can ignore data elements that refer to paths we can't map due to mismatching schema.
        // The eventual call to StoreDataElement will correctly deal with the presence of a null property path handle that
        // has been returned by the above call. It's necessary to call into StoreDataElement with this null handle to ensure
        // the requisite OnEvent calls are made to the application despite the presence of an unknown tag. It's also necessary to ensure
        // that we update the internal version tracked by the sink.
        if (err == WEAVE_ERROR_TLV_TAG_NOT_FOUND) {
            WeaveLogDetail(DataManagement, "Ignoring un-mappable path!");
            err = WEAVE_NO_ERROR;
        }
#endif
        SuccessOrExit(err);

        pathReader = aReader;
        flags = 0;

#if WDM_ENABLE_PROTOCOL_CHECKS
        // If we previously had a partial change, the current handle should match the previous one.
        // If they don't, we have a partial change violation.
        if (mPrevIsPartialChange && (mPrevTraitDataHandle != handle)) {
            WeaveLogError(DataManagement, "Encountered partial change flag violation (%u, %x, %x)", mPrevIsPartialChange, mPrevTraitDataHandle, handle);
            err = WEAVE_ERROR_INVALID_DATA_LIST;
            goto exit;
        }
#endif

        if (!mPrevIsPartialChange) {
            flags = TraitDataSink::kFirstElementInChange;
        }

        if (!isPartialChange) {
            flags |= TraitDataSink::kLastElementInChange;
        }

        err = DataSink->StoreDataElement(pathHandle, pathReader, flags, NULL, NULL);
        SuccessOrExit(err);

        mPrevIsPartialChange = isPartialChange;

#if WDM_ENABLE_PROTOCOL_CHECKS
        mPrevTraitDataHandle = handle;
#endif
    }

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    return err;
}

void SubscriptionClient::NotificationRequestHandler (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
        const nl::Weave::WeaveMessageInfo *aMsgInfo, PacketBuffer *aPayload)
{
    WEAVE_ERROR                 err                     = WEAVE_NO_ERROR;
    InEventParam                inParam;
    OutEventParam               outParam;
    NotificationRequest::Parser notify;
    const ClientState           StateWhenEntered        = mCurrentState;
    nl::Weave::TLV::TLVReader   reader;
    bool                        isDataListPresent       = false;
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool                        isEventListPresent      = false;
#endif
    uint8_t                     statusReportLen         = 6;
    PacketBuffer*               msgBuf                  = PacketBuffer::NewWithAvailableSize(statusReportLen);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident
    _AddRef();

    if (mEC != aEC)
    {
        // only re-configure if this is an incoming EC
        mBinding->AdjustResponseTimeout(aEC);
    }

    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    // skip the first 6 bytes in status report, as they are reserved for the profile ID and status code
    msgBuf->SetDataLength(statusReportLen);

    switch (StateWhenEntered)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        // In these two states, all notifications must come in the same exchange context
        VerifyOrExit(aEC == mEC, err = WEAVE_ERROR_INCORRECT_STATE);

        // refresh inactivity monitor every time we receive a notification request
        err = RefreshTimer();
        SuccessOrExit(err);
        break;

    case kState_SubscriptionEstablished_Idle:
    case kState_SubscriptionEstablished_Confirming:

        // refresh inactivity monitor every time we receive a notification request
        err = RefreshTimer();
        SuccessOrExit(err);

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        SubscriptionEngine::GetInstance()->UpdateHandlerLiveness(mBinding->GetPeerNodeId(), mSubscriptionId);
#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER

        break;

    // we are going to ignore any notification requests in other states
    default:
        ExitNow();
    }

    inParam.mNotificationRequest.mEC = aEC;
    inParam.mNotificationRequest.mMessage = aPayload;
    inParam.mNotificationRequest.mClient = this;

    // NOTE: state could be changed in the callback to app layer
    mEventCallback(mAppState, kEvent_OnNotificationRequest, inParam, outParam);

    mDataSinkCatalog->DispatchEvent(TraitDataSink::kEventNotifyRequestBegin, NULL);

    // jump to Exit if the state has been changed in the callback to app layer
    VerifyOrExit(StateWhenEntered == mCurrentState, /* no-op */);

    reader.Init(aPayload);
    reader.Next();

    err = notify.Init(reader);
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    // simple schema checking
    err = notify.CheckSchemaValidity();
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    // TODO: use the new GetReaderOnXYZ pattern to locate the data list, instead creating a data list parser object
    {
        DataList::Parser dataList;

        err = notify.GetDataList(&dataList);
        if (WEAVE_NO_ERROR == err)
        {
            isDataListPresent = true;
        }
        else if (WEAVE_END_OF_TLV == err)
        {
            isDataListPresent = false;
            err = WEAVE_NO_ERROR;
        }
        SuccessOrExit(err);

        // re-initialize the reader to point to individual date element (reuse to save stack depth).
        dataList.GetReader(&reader);
    }

    if (isDataListPresent)
    {
        err = ProcessDataList(reader);
        SuccessOrExit(err);
    }

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    {
        EventList::Parser eventList;

        err = notify.GetEventList(&eventList);
        if (WEAVE_NO_ERROR == err)
        {
            isEventListPresent = true;
        }
        else if (WEAVE_END_OF_TLV == err)
        {
            isEventListPresent = false;
            err = WEAVE_NO_ERROR;
        }
        SuccessOrExit(err);

        // re-initialize the reader (reuse to save stack depth).
        eventList.GetReader(&reader);
    }

    if (isEventListPresent)
    {
        inParam.mEventStreamReceived.mReader = &reader;
        inParam.mEventStreamReceived.mClient = this;

        // Invoke our callback.
        mEventCallback(mAppState, kEvent_OnEventStreamReceived, inParam, outParam);
    }
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

    // TODO: As I've commented in Weave PR#614, there is no support for event sink

    inParam.mNotificationProcessed.mClient = this;

    // NOTE: state could be changed in the callback to app layer
    mEventCallback(mAppState, kEvent_OnNotificationProcessed, inParam, outParam);

    mDataSinkCatalog->DispatchEvent(TraitDataSink::kEventNotifyRequestEnd, NULL);

    // jump to Exit if the state has been changed in the callback to app layer
    VerifyOrExit(StateWhenEntered == mCurrentState, /* no-op */);

    {
        uint8_t * p = msgBuf->Start();
        nl::Weave::Encoding::LittleEndian::Write32(p, nl::Weave::Profiles::kWeaveProfile_Common);
        nl::Weave::Encoding::LittleEndian::Write16(p, nl::Weave::Profiles::Common::kStatus_Success);

        err = aEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common,
            nl::Weave::Profiles::Common::kMsgType_StatusReport,
            msgBuf,
            aEC->HasPeerRequestedAck() ? nl::Weave::ExchangeContext::kSendFlag_RequestAck : 0);
        msgBuf = NULL;
        SuccessOrExit(err);
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (NULL != aPayload) {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    // If this is not a locally initiated exchange, always close the exchange
    if (aEC != mEC)
    {
        aEC->Close();
        aEC = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        // If we're not aborted yet, make a callback to app layer
        HandleSubscriptionTerminated(IsRetryEnabled(), err, NULL);
    }

    _Release();
}

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
void SubscriptionClient::CancelRequestHandler (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
    const nl::Weave::WeaveMessageInfo *aMsgInfo, PacketBuffer *aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t statusReportLen = 6;
    PacketBuffer *msgBuf = PacketBuffer::NewWithAvailableSize(statusReportLen);
    uint8_t * p;
    bool canceled = true;
    uint32_t statusProfile = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t statusCode = nl::Weave::Profiles::Common::kStatus_Success;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(this), GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident
    _AddRef();

    mBinding->AdjustResponseTimeout(aEC);

    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    // Verify the cancel request is truly from the publisher.  If not, reject the request with
    // "invalid subscription id" to avoid revealing the existence of the subscription.
    if (!mBinding->IsAuthenticMessageFromPeer(aMsgInfo))
    {
        WeaveLogDetail(DataManagement, "Rejecting SubscribeCancelRequest from unauthorized source");
        canceled = false;
        statusProfile = nl::Weave::Profiles::kWeaveProfile_WDM;
        statusCode = kStatus_InvalidSubscriptionID;
    }

    p = msgBuf->Start();
    nl::Weave::Encoding::LittleEndian::Write32(p, statusProfile);
    nl::Weave::Encoding::LittleEndian::Write16(p, statusCode);
    msgBuf->SetDataLength(statusReportLen);

    err = aEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common,
        nl::Weave::Profiles::Common::kMsgType_StatusReport,
        msgBuf,
        aEC->HasPeerRequestedAck() ? nl::Weave::ExchangeContext::kSendFlag_RequestAck : 0);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    // In either case, the subscription is already canceled, move to ABORTED
    if ((WEAVE_NO_ERROR != err) || canceled)
    {
        HandleSubscriptionTerminated(false, err, NULL);
    }

    _Release();
}
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

void SubscriptionClient::OnSendError (ExchangeContext *aEC, WEAVE_ERROR aErrorCode, void *aMsgSpecificContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    IgnoreUnusedVariable(aMsgSpecificContext);
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);
    bool subscribeRequestFailed = false;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(pClient), pClient->GetStateStr(), __func__, pClient->mRefCount);

    // Make sure we're not freed by accident
    pClient->_AddRef();

    switch (pClient->mCurrentState)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        // Subscribe request failed, deliver mSubscriptionRequestFailedEventParam
        subscribeRequestFailed = true;
        ExitNow(err = aErrorCode);
        break;

    case kState_SubscriptionEstablished_Confirming:
        // Subscribe Confirm request failed, so no point trying to send a cancel.
        // Go ahead and terminate it.
        ExitNow(err = aErrorCode);
        break;

    case kState_Resubscribe_Holdoff:
        // OnResponseTimeout posts an error to OnSendError (this function).
        // That can happen after we've already received a cb for OnSendError.
        // So if we've already set a timeout, then we can ignore this error.
        if (aErrorCode == WEAVE_ERROR_TIMEOUT)
        {
            err = WEAVE_NO_ERROR;
        }
        break;

    case kState_Canceling:
        ExitNow(err = aErrorCode);
        break;

    // In any of these states, we must not see this callback
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    if ((subscribeRequestFailed) || (WEAVE_NO_ERROR != err))
    {
        pClient->HandleSubscriptionTerminated(pClient->IsRetryEnabled(), err, NULL);
    }

    pClient->_Release();
}

void SubscriptionClient::OnResponseTimeout (nl::Weave::ExchangeContext *aEC)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);

    IgnoreUnusedVariable(pClient);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(pClient), pClient->GetStateStr(), __func__, pClient->mRefCount);

    OnSendError(aEC, WEAVE_ERROR_TIMEOUT, NULL);
}

void SubscriptionClient::OnMessageReceivedFromLocallyInitiatedExchange (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
    const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId,
    uint8_t aMsgType, PacketBuffer *aPayload)
{
    // Notification Requests during initial setup
    // Subscribe response
    // Status Report for Subscribe request
    // Status Report for Subscribe Cancel request
    // Status Report for Subscribe Confirm request

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SubscriptionClient * pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);

    InEventParam inParam;
    OutEventParam outParam;
    bool retainExchangeContext = false;
    bool isStatusReportValid = false;
    nl::Weave::Profiles::StatusReporting::StatusReport status;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)",
        SubscriptionEngine::GetInstance()->GetClientId(pClient), pClient->GetStateStr(), __func__, pClient->mRefCount);

    // Make sure we're not freed by accident
    pClient->_AddRef();

    WeaveLogIfFalse(aEC == pClient->mEC);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) && (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        // Note that payload is not freed in this call to parse
        err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);
        isStatusReportValid = true;
        WeaveLogDetail(DataManagement, "Received Status Report 0x%" PRIX32 " : 0x%" PRIX16, status.mProfileId, status.mStatusCode);
    }

    switch (pClient->mCurrentState)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        if (isStatusReportValid)
        {
            ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
        }
        else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_NotificationRequest == aMsgType))
        {
            // notification request, don't close the exchange context, for more notification requests might arrive
            // through this same exchange context
            retainExchangeContext = true;
            pClient->NotificationRequestHandler(aEC, aPktInfo, aMsgInfo, aPayload);
            aPayload = NULL;
        }
        else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_SubscribeResponse == aMsgType))
        {
            // capture subscription ID and liveness timeout
            nl::Weave::TLV::TLVReader reader;
            reader.Init(aPayload);
            err = reader.Next();
            SuccessOrExit(err);

            SubscribeResponse::Parser response;
            err = response.Init(reader);
            SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
            // simple schema checking
            err = response.CheckSchemaValidity();
            SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

            {
                uint64_t subscriptionId;
                err = response.GetSubscriptionID(&subscriptionId);
                SuccessOrExit(err);
                if (kState_Subscribing == pClient->mCurrentState)
                {
                    // capture subscription ID
                    pClient->mSubscriptionId = subscriptionId;
                }
                else
                {
                    // verify they are the same
                    VerifyOrExit(pClient->mSubscriptionId == subscriptionId, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
                }
            }

            if (kState_Subscribing == pClient->mCurrentState)
            {
                uint32_t livenessTimeoutSec;
                err = response.GetSubscribeTimeout(&livenessTimeoutSec);

                if (WEAVE_NO_ERROR == err)
                {
                    VerifyOrExit(livenessTimeoutSec <= kMaxTimeoutSec, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);

                    // capture liveness timeout
                    pClient->mLivenessTimeoutMsec = livenessTimeoutSec * 1000;
                }
                else if (WEAVE_END_OF_TLV == err)
                {
                    err = WEAVE_NO_ERROR;
                }
                else
                {
                    ExitNow();
                }
            }

            // Subscribe response, move to alive-idle state (and close the exchange context)
            pClient->MoveToState(kState_SubscriptionEstablished_Idle);

            err = pClient->RefreshTimer();
            SuccessOrExit(err);

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
            SubscriptionEngine::GetInstance()->UpdateHandlerLiveness(pClient->mBinding->GetPeerNodeId(), pClient->mSubscriptionId);
#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER

            pClient->mRetryCounter = 0;

            inParam.mSubscriptionEstablished.mSubscriptionId = pClient->mSubscriptionId;
            inParam.mSubscriptionEstablished.mClient = pClient;

            // it's allowed to cancel or even abandon this subscription right inside this callback
            pClient->mEventCallback(pClient->mAppState, kEvent_OnSubscriptionEstablished, inParam, outParam);
            // since the state could have been changed, we must not assume anything
            ExitNow();
        }
        else
        {
            // protocol error
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        }
        break;

    case kState_SubscriptionEstablished_Confirming:
        if (isStatusReportValid && status.success())
        {
            // Status Report (success) for Subscribe Confirm request
            // confirmed, move back to idle state
            pClient->FlushExistingExchangeContext();
            pClient->MoveToState(kState_SubscriptionEstablished_Idle);

            WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] liveness confirmed",
                SubscriptionEngine::GetInstance()->GetClientId(pClient), pClient->GetStateStr());

            err = pClient->RefreshTimer();
            SuccessOrExit(err);

#if WDM_ENABLE_SUBSCRIPTION_PUBLISHER
            SubscriptionEngine::GetInstance()->UpdateHandlerLiveness(pClient->mBinding->GetPeerNodeId(), pClient->mSubscriptionId);
#endif // WDM_ENABLE_SUBSCRIPTION_PUBLISHER
        }
        else
        {
            // anything else is a failure, tear down the subscription
            ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        }
        break;

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
    case kState_Canceling:
        // It doesn't really matter what we receive from the other end, as we're heading out
        // call abort silently without callback to upper layer, for this subscription was canceled by the upper layer
        pClient->AbortSubscription();
        ExitNow();
        break;
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

    // We must not receive this callback in any other states
    default:
        WeaveLogDetail(DataManagement, "Received message in some wrong state, ignore");
        ExitNow();
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    if (!retainExchangeContext)
    {
        pClient->FlushExistingExchangeContext();
    }

    if (err != WEAVE_NO_ERROR)
    {
        // if we're already aborted, this call becomes a no-op
        pClient->HandleSubscriptionTerminated(pClient->IsRetryEnabled(), err, isStatusReportValid ? &status : NULL);
    }

    pClient->_Release();
}

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

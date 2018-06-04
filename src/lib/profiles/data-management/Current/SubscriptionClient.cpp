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

class AlwaysAcceptDataElementAccessControlDelegate : public IDataElementAccessControlDelegate
{
public:

     // TODO : This Access Check function needs to be more sophisticated in
     // allowing access to subscription-based notifications.

     WEAVE_ERROR DataElementAccessCheck(const TraitPath & aTraitPath,
                                        const TraitCatalogBase<TraitDataSink> & aCatalog)
     {
         return WEAVE_NO_ERROR;
     }
};

// Do nothing
SubscriptionClient::SubscriptionClient() { }

void SubscriptionClient::InitAsFree()
{
    mCurrentState = kState_Free;
    mRefCount     = 0;
    Reset();
}

void SubscriptionClient::Reset(void)
{
    mBinding                                = NULL;
    mEC                                     = NULL;
    mAppState                               = NULL;
    mEventCallback                          = NULL;
    mResubscribePolicyCallback              = NULL;
    mDataSinkCatalog                        = NULL;
    mLock                                   = NULL;
    mInactivityTimeoutDuringSubscribingMsec = kNoTimeOut;
    mLivenessTimeoutMsec                    = kNoTimeOut;
    mSubscriptionId                         = 0;
    mIsInitiator                            = false;
    mRetryCounter                           = 0;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    mUpdateInFlight                         = false;
    mNumUpdatableTraitInstances             = 0;
    mMaxUpdateSize                          = 0;
    mUpdateRequestContext.mItemInProgress = 0;
    mUpdateRequestContext.mNextDictionaryElementPathHandle = kNullPropertyPathHandle;
    mPendingSetState = kPendingSetEmpty;
    mPendingUpdateSet.Init(mPendingStore, ArraySize(mPendingStore));
    mInProgressUpdateList.Init(mInProgressStore, ArraySize(mInProgressStore));
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

#if WDM_ENABLE_PROTOCOL_CHECKS
    mPrevTraitDataHandle = -1;
#endif

    mPrevIsPartialChange = false;
}

// AddRef to Binding
// store pointers to binding and delegate
// null out EC
WEAVE_ERROR SubscriptionClient::Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback,
                                     const TraitCatalogBase<TraitDataSink> * const apCatalog,
                                     const uint32_t aInactivityTimeoutDuringSubscribingMsec, IWeaveClientLock * aLock)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveLogIfFalse(0 == mRefCount);

    // add reference to the binding
    apBinding->AddRef();

    // make a copy of the pointers
    mBinding                                = apBinding;
    mAppState                               = apAppState;
    mEventCallback                          = aEventCallback;

    if (NULL == apCatalog)
    {
        mDataSinkCatalog = NULL;
    }
    else
    {
        mDataSinkCatalog = const_cast <TraitCatalogBase<TraitDataSink>*>(apCatalog);
    }

    mInactivityTimeoutDuringSubscribingMsec = aInactivityTimeoutDuringSubscribingMsec;

    mLock                                   = aLock;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    mUpdateInFlight                         = false;
    mNumUpdatableTraitInstances             = 0;
    mMaxUpdateSize                          = 0;

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
    MoveToState(kState_Initialized);

    _AddRef();

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE

    err = mUpdateClient.Init(mBinding, this, UpdateEventCallback);
    SuccessOrExit(err);

    if (NULL != mDataSinkCatalog)
    {
        mDataSinkCatalog->Iterate(InitUpdatableSinkTrait, this);
    }

exit:
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
    return err;
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
#else  // WEAVE_DETAIL_LOGGING
const char * SubscriptionClient::GetStateStr() const
{
    return "N/A";
}
#endif // WEAVE_DETAIL_LOGGING

void SubscriptionClient::MoveToState(const ClientState aTargetState)
{
    mCurrentState = aTargetState;
    WeaveLogDetail(DataManagement, "Client[%u] moving to [%5.5s] Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), mRefCount);

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
 * @param[in] aCallback     Optional callback to fetch the amount of time to
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
        SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->CancelTimer(OnTimerCallback, this);

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
        SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->CancelTimer(OnTimerCallback, this);
        MoveToState(kState_Initialized);
    }

    mRetryCounter = 0;

    if (mCurrentState == kState_Initialized || mCurrentState == kState_Resubscribe_Holdoff)
    {
        SetRetryTimer(WEAVE_NO_ERROR);
    }
}

void SubscriptionClient::IndicateActivity(void)
{
    // emit an OnSubscriptionActivity event
    InEventParam inParam;
    OutEventParam outParam;

    inParam.mSubscriptionActivity.mClient = this;
    mEventCallback(mAppState, kEvent_OnSubscriptionActivity, inParam, outParam);
}

WEAVE_ERROR SubscriptionClient::GetSubscriptionId(uint64_t * const apSubscriptionId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

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

void SubscriptionClient::DefaultResubscribePolicyCallback(void * const aAppState, ResubscribeParam & aInParam,
                                                          uint32_t & aOutIntervalMsec)
{
    IgnoreUnusedVariable(aAppState);

    uint32_t fibonacciNum      = 0;
    uint32_t maxWaitTimeInMsec = 0;
    uint32_t waitTimeInMsec    = 0;
    uint32_t minWaitTimeInMsec = 0;

    if (aInParam.mNumRetries <= WDM_RESUBSCRIBE_MAX_FIBONACCI_STEP_INDEX)
    {
        fibonacciNum      = GetFibonacciForIndex(aInParam.mNumRetries);
        maxWaitTimeInMsec = fibonacciNum * WDM_RESUBSCRIBE_WAIT_TIME_MULTIPLIER_MS;
    }
    else
    {
        maxWaitTimeInMsec = WDM_RESUBSCRIBE_MAX_RETRY_WAIT_INTERVAL_MS;
    }

    if (maxWaitTimeInMsec != 0)
    {
        minWaitTimeInMsec = (WDM_RESUBSCRIBE_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP * maxWaitTimeInMsec) / 100;
        waitTimeInMsec    = minWaitTimeInMsec + (GetRandU32() % (maxWaitTimeInMsec - minWaitTimeInMsec));
    }

    aOutIntervalMsec = waitTimeInMsec;

    WeaveLogDetail(DataManagement,
                   "Computing resubscribe policy: attempts %" PRIu32 ", max wait time %" PRIu32 " ms, selected wait time %" PRIu32
                   " ms",
                   aInParam.mNumRetries, maxWaitTimeInMsec, waitTimeInMsec);

    return;
}

void SubscriptionClient::_InitiateSubscription(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure the client object is not freed during the callback to the application.
    _AddRef();

    VerifyOrExit(kState_Subscribing != mCurrentState && kState_Subscribing_IdAssigned != mCurrentState, /* no-op */);

    VerifyOrExit(kState_Initialized == mCurrentState, err = WEAVE_ERROR_INCORRECT_STATE);

    // Set the protocol callback on the binding object.  NOTE: This should only happen once the
    // app has explicitly started the subscription process by calling either InitiateSubscription() or
    // InitiateCounterSubscription().  Otherwise the client object might receive callbacks from
    // the binding before it's ready.
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
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    uint8_t msgType       = kMsgType_SubscribeRequest;
    InEventParam inSubscribeParam;
    OutEventParam outSubscribeParam;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

    inSubscribeParam.Clear();
    outSubscribeParam.Clear();

    outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList = NULL;
    outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList          = NULL;

    inSubscribeParam.mSubscribeRequestPrepareNeeded.mClient = this;
    mEventCallback(mAppState, kEvent_OnSubscribeRequestPrepareNeeded, inSubscribeParam, outSubscribeParam);

    if (!mIsInitiator)
    {
        mSubscriptionId = outSubscribeParam.mSubscribeRequestPrepareNeeded.mSubscriptionId;
    }

    VerifyOrExit(kState_Initialized == mCurrentState, err = WEAVE_ERROR_INCORRECT_STATE);
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
                TraitDataSink * pDataSink;
                nl::Weave::TLV::TLVType dummyContainerType;
                SchemaVersionRange versionIntersection;
                VersionedTraitPath versionedTraitPath;

                // Applications can set either the versioned or non versioned path lists for now. We pick either
                // depending on which is non-NULL. If both are non-NULL, we then select the versioned list.
                if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList)
                {
                    versionedTraitPath = outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList[i];
                }
                else
                {
                    versionedTraitPath.mTraitDataHandle =
                        outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mTraitDataHandle;
                    versionedTraitPath.mPropertyPathHandle =
                        outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mPropertyPathHandle;
                }

                err = mDataSinkCatalog->Locate(versionedTraitPath.mTraitDataHandle, &pDataSink);
                SuccessOrExit(err);

                // Start the TLV Path
                err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Path, dummyContainerType);
                SuccessOrExit(err);

                // Start, fill, and close the TLV Structure that contains ResourceID, ProfileID, and InstanceID
                err = mDataSinkCatalog->HandleToAddress(versionedTraitPath.mTraitDataHandle, writer,
                                                        versionedTraitPath.mRequestedVersionRange);
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
                TraitDataSink * pDataSink;
                VersionedTraitPath versionedTraitPath;

                if (outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList)
                {
                    versionedTraitPath = outSubscribeParam.mSubscribeRequestPrepareNeeded.mVersionedPathList[i];
                }
                else
                {
                    versionedTraitPath.mTraitDataHandle =
                        outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mTraitDataHandle;
                    versionedTraitPath.mPropertyPathHandle =
                        outSubscribeParam.mSubscribeRequestPrepareNeeded.mPathList[i].mPropertyPathHandle;
                }

                err = mDataSinkCatalog->Locate(versionedTraitPath.mTraitDataHandle, &pDataSink);
                SuccessOrExit(err);

                if (pDataSink->IsVersionValid())
                {
                    versionList.AddVersion(pDataSink->GetVersion());
                }
                else
                {
                    versionList.AddNull();
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
                    if (pDataSink->IsUpdatableDataSink() && ( ! pDataSink->IsVersionValid()))
                    {
                        ClearPotentialDataLoss(versionedTraitPath.mTraitDataHandle);
                    }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

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
                        .EndOfEvent();
                    SuccessOrExit(err = event.GetError());
                }
                eventList.EndOfEventList();
                SuccessOrExit(err = eventList.GetError());
            }
        }

        request.EndOfRequest();
        SuccessOrExit(err = request.GetError());

        err = writer.Finalize();
        SuccessOrExit(err);
    }

    err = ReplaceExchangeContext();
    SuccessOrExit(err);

    // NOTE: State could be changed in sync error callback by message layer
    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_SendUnsupportedReqMsgType, msgType += 50);

    err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, msgType, msgBuf,
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

void SubscriptionClient::_AddRef()
{
    WeaveLogIfFalse(mRefCount < INT8_MAX);

    ++mRefCount;

    // 0: free
    // 1: in some phase of subscription
    // increase: in downcall to message layer, some callback might come from message layer (send error/connection broken)
    // increase: in callback to app layer
}

void SubscriptionClient::_Release()
{
    WeaveLogIfFalse(mRefCount > 0);

    --mRefCount;

    if (0 == mRefCount)
    {
        AbortSubscription();

        SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDM_NumSubscriptionClients);
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

WEAVE_ERROR SubscriptionClient::ReplaceExchangeContext()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;

    // Make sure we're not freed by accident.
    _AddRef();

    FlushExistingExchangeContext();

    err = mBinding->NewExchangeContext(mEC);
    SuccessOrExit(err);

    mEC->AppState          = this;
    mEC->OnMessageReceived = OnMessageReceivedFromLocallyInitiatedExchange;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError       = OnSendError;
    mEC->OnAckRcvd         = NULL;

    inParam.mExchangeStart.mEC     = mEC;
    inParam.mExchangeStart.mClient = this;

    // NOTE: app layer is not supposed to change state/ref count in this callback
    mEventCallback(mAppState, kEvent_OnExchangeStart, inParam, outParam);

exit:
    WeaveLogFunctError(err);

    _Release();

    return err;
}

void SubscriptionClient::FlushExistingExchangeContext(const bool aAbortNow)
{
    if (NULL != mEC)
    {
        mEC->AppState          = NULL;
        mEC->OnMessageReceived = NULL;
        mEC->OnResponseTimeout = NULL;
        mEC->OnSendError       = NULL;
        mEC->OnAckRcvd         = NULL;
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
WEAVE_ERROR SubscriptionClient::EndSubscription()
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    nl::Weave::TLV::TLVWriter writer;
    SubscribeCancelRequest::Builder request;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

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

        err = ReplaceExchangeContext();
        SuccessOrExit(err);

        writer.Init(msgBuf);
        request.Init(&writer);
        err = request.SubscriptionID(mSubscriptionId).EndOfRequest().GetError();
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        // NOTE: State could be changed if there is a sync error callback from message layer
        err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeCancelRequest, msgBuf,
                               nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
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
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    const bool nullReference = (0 == mRefCount);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

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
        uint64_t peerNodeId                = mBinding->GetPeerNodeId();
        uint64_t subscriptionId            = mSubscriptionId;
        bool deliverSubTerminatedToCatalog = ((NULL != mDataSinkCatalog) && (mCurrentState >= kState_NotifyDataSinkOnAbort_Begin) &&
                                              (mCurrentState <= kState_NotifyDataSinkOnAbort_End));

        MoveToState(kState_Aborting);

        if (deliverSubTerminatedToCatalog)
        {
            // iterate through the whole catalog and deliver kEventSubscriptionTerminated event
            mDataSinkCatalog->DispatchEvent(TraitDataSink::kEventSubscriptionTerminated, NULL);
        }

        mBinding->SetProtocolLayerCallback(NULL, NULL);

        mBinding->Release();
        mBinding = NULL;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
        // TODO: aborting the subscription should not impact the "udpate client"
        ClearPathStore(mPendingUpdateSet, WEAVE_ERROR_CONNECTION_ABORTED);
        // TODO: what's the right error code for this?
        ClearPathStore(mInProgressUpdateList, WEAVE_ERROR_CONNECTION_ABORTED);
        ShutdownUpdateClient();
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
        // Note that ref count is not touched at here, as _Abort doesn't change the ownership
        FlushExistingExchangeContext(true);
        (void) RefreshTimer();

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
                                                      StatusReporting::StatusReport * aStatusReportPtr)
{
    void * const pAppState     = mAppState;
    EventCallback callbackFunc = mEventCallback;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

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

        inParam.mSubscriptionTerminated.mReason            = aReason;
        inParam.mSubscriptionTerminated.mClient            = this;
        inParam.mSubscriptionTerminated.mWillRetry         = aWillRetry;
        inParam.mSubscriptionTerminated.mIsStatusCodeValid = (aStatusReportPtr != NULL);
        if (aStatusReportPtr != NULL)
        {
            inParam.mSubscriptionTerminated.mStatusProfileId   = aStatusReportPtr->mProfileId;
            inParam.mSubscriptionTerminated.mStatusCode        = aStatusReportPtr->mStatusCode;
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
    WEAVE_ERROR err                   = WEAVE_NO_ERROR;
    ClientState entryState            = mCurrentState;
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
        param.mReason     = aReason;

        mResubscribePolicyCallback(mAppState, param, timeoutMsec);
        VerifyOrExit(mCurrentState != kState_Aborted, /* exit without error */);

        err = SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->StartTimer(timeoutMsec,
                                                                                                             OnTimerCallback, this);
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

void SubscriptionClient::Free()
{
    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

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

void SubscriptionClient::BindingEventCallback(void * const aAppState, const Binding::EventType aEvent,
                                              const Binding::InEventParam & aInParam, Binding::OutEventParam & aOutParam)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aAppState);

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

void SubscriptionClient::OnTimerCallback(System::Layer * aSystemLayer, void * aAppState, System::Error)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aAppState);

    pClient->TimerEventHandler();
}

WEAVE_ERROR SubscriptionClient::RefreshTimer(void)
{
    WEAVE_ERROR err      = WEAVE_NO_ERROR;
    bool isTimerNeeded   = false;
    uint32_t timeoutMsec = 0;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

    // Cancel timer first
    SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->CancelTimer(OnTimerCallback, this);

    // Arm timer according to current state
    switch (mCurrentState)
    {
    case kState_Subscribing:
    case kState_Subscribing_IdAssigned:
        if (kNoTimeOut != mInactivityTimeoutDuringSubscribingMsec)
        {
            // Note that loss of range is not expected, as ExchangeManager::Timeout is indeed uint32_t
            timeoutMsec   = mInactivityTimeoutDuringSubscribingMsec;
            isTimerNeeded = true;

            WeaveLogDetail(DataManagement,
                           "Client[%u] [%5.5s] %s Ref(%d) Set inactivity time limit during subscribing to %" PRIu32 " msec",
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
                const nl::Weave::WRMPConfig & defaultWRMPConfig = mBinding->GetDefaultWRMPConfig();
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
                    WeaveLogError(DataManagement,
                                  "Client[%u] Liveness period (%" PRIu32 " msec) <= margin reserved for WRM (%" PRIu32 " msec)",
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
        err = SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->StartTimer(timeoutMsec,
                                                                                                             OnTimerCallback, this);

        VerifyOrExit(WEAVE_SYSTEM_NO_ERROR == err, /* no-op */);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void SubscriptionClient::TimerEventHandler(void)
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    bool skipTimerCheck   = false;

    if ((0 == mRefCount) || (mCurrentState < kState_TimerTick_Begin) || (mCurrentState > kState_TimerTick_End))
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

            err = ReplaceExchangeContext();
            SuccessOrExit(err);

            writer.Init(msgBuf);
            request.Init(&writer);
            err = request.SubscriptionID(mSubscriptionId).EndOfRequest().GetError();
            SuccessOrExit(err);

            err = writer.Finalize();
            SuccessOrExit(err);

            // NOTE: State could be changed if there is a send error callback from message layer
            err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeConfirmRequest, msgBuf,
                                   nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
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

WEAVE_ERROR SubscriptionClient::ProcessDataList(nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    AlwaysAcceptDataElementAccessControlDelegate acDelegate;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    bool isLocked = false;

    err = Lock();
    SuccessOrExit(err);

    isLocked = true;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    err = SubscriptionEngine::ProcessDataList(aReader, mDataSinkCatalog, mPrevIsPartialChange, mPrevTraitDataHandle, acDelegate);
    SuccessOrExit(err);

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    if (false == IsUpdateInFlight())
    {
        PurgePendingUpdate();
    }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

exit:
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    if (isLocked)
    {
        Unlock();
    }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    return err;
}

void SubscriptionClient::NotificationRequestHandler(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                                    const nl::Weave::WeaveMessageInfo * aMsgInfo, PacketBuffer * aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;
    NotificationRequest::Parser notify;
    const ClientState StateWhenEntered = mCurrentState;
    nl::Weave::TLV::TLVReader reader;
    bool isDataListPresent = false;
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool isEventListPresent = false;
#endif
    uint8_t statusReportLen = 6;
    PacketBuffer * msgBuf   = PacketBuffer::NewWithAvailableSize(statusReportLen);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

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

    // Emit an OnSubscriptionActivity event
    inParam.Clear();
    inParam.mSubscriptionActivity.mClient = this;
    mEventCallback(mAppState, kEvent_OnSubscriptionActivity, inParam, outParam);

    inParam.Clear();
    outParam.Clear();
    inParam.mNotificationRequest.mEC      = aEC;
    inParam.mNotificationRequest.mMessage = aPayload;
    inParam.mNotificationRequest.mClient  = this;

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
            err               = WEAVE_NO_ERROR;
        }
        SuccessOrExit(err);

        // re-initialize the reader to point to individual data element (reuse to save stack depth).
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
            err                = WEAVE_NO_ERROR;
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

        err    = aEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kMsgType_StatusReport,
                               msgBuf, aEC->HasPeerRequestedAck() ? nl::Weave::ExchangeContext::kSendFlag_RequestAck : 0);
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

    if (NULL != aPayload)
    {
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
void SubscriptionClient::CancelRequestHandler(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                              const nl::Weave::WeaveMessageInfo * aMsgInfo, PacketBuffer * aPayload)
{
    WEAVE_ERROR err         = WEAVE_NO_ERROR;
    uint8_t statusReportLen = 6;
    PacketBuffer * msgBuf   = PacketBuffer::NewWithAvailableSize(statusReportLen);
    uint8_t * p;
    bool canceled          = true;
    uint32_t statusProfile = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t statusCode    = nl::Weave::Profiles::Common::kStatus_Success;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident
    _AddRef();

    mBinding->AdjustResponseTimeout(aEC);

    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    // Verify the cancel request is truly from the publisher.  If not, reject the request with
    // "invalid subscription id" to avoid revealing the existence of the subscription.
    if (!mBinding->IsAuthenticMessageFromPeer(aMsgInfo))
    {
        WeaveLogDetail(DataManagement, "Rejecting SubscribeCancelRequest from unauthorized source");
        canceled      = false;
        statusProfile = nl::Weave::Profiles::kWeaveProfile_WDM;
        statusCode    = kStatus_InvalidSubscriptionID;
    }

    p = msgBuf->Start();
    nl::Weave::Encoding::LittleEndian::Write32(p, statusProfile);
    nl::Weave::Encoding::LittleEndian::Write16(p, statusCode);
    msgBuf->SetDataLength(statusReportLen);

    err    = aEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kMsgType_StatusReport, msgBuf,
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

void SubscriptionClient::OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    IgnoreUnusedVariable(aMsgSpecificContext);
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);
    bool subscribeRequestFailed        = false;

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(pClient),
                   pClient->GetStateStr(), __func__, pClient->mRefCount);

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

void SubscriptionClient::OnResponseTimeout(nl::Weave::ExchangeContext * aEC)
{
    SubscriptionClient * const pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);

    IgnoreUnusedVariable(pClient);

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(pClient),
                   pClient->GetStateStr(), __func__, pClient->mRefCount);

    OnSendError(aEC, WEAVE_ERROR_TIMEOUT, NULL);
}

void SubscriptionClient::OnMessageReceivedFromLocallyInitiatedExchange(nl::Weave::ExchangeContext * aEC,
                                                                       const nl::Inet::IPPacketInfo * aPktInfo,
                                                                       const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                                                       uint32_t aProfileId, uint8_t aMsgType,
                                                                       PacketBuffer * aPayload)
{
    // Notification Requests during initial setup
    // Subscribe response
    // Status Report for Subscribe request
    // Status Report for Subscribe Cancel request
    // Status Report for Subscribe Confirm request

    WEAVE_ERROR err              = WEAVE_NO_ERROR;
    SubscriptionClient * pClient = reinterpret_cast<SubscriptionClient *>(aEC->AppState);
    InEventParam inParam;
    OutEventParam outParam;
    bool retainExchangeContext = false;
    bool isStatusReportValid   = false;
    nl::Weave::Profiles::StatusReporting::StatusReport status;

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    bool isLocked = false;
    err = pClient->Lock();
    SuccessOrExit(err);

    isLocked = true;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    WeaveLogDetail(DataManagement, "Client[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetClientId(pClient),
                   pClient->GetStateStr(), __func__, pClient->mRefCount);

    // Make sure we're not freed by accident
    pClient->_AddRef();

    WeaveLogIfFalse(aEC == pClient->mEC);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) &&
        (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
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

            inParam.mSubscriptionActivity.mClient = pClient;
            pClient->mEventCallback(pClient->mAppState, kEvent_OnSubscriptionActivity, inParam, outParam);

            inParam.Clear();
            inParam.mSubscriptionEstablished.mSubscriptionId = pClient->mSubscriptionId;
            inParam.mSubscriptionEstablished.mClient         = pClient;

            // it's allowed to cancel or even abandon this subscription right inside this callback
            pClient->mEventCallback(pClient->mAppState, kEvent_OnSubscriptionEstablished, inParam, outParam);
            // since the state could have been changed, we must not assume anything

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
            if (pClient->mPendingSetState == kPendingSetReady &&
                    pClient->mInProgressUpdateList.IsEmpty())
            {
                // TODO: test failing here..
                err = pClient->MovePendingToInProgress();
                SuccessOrExit(err);
                err = pClient->FormAndSendUpdate(true);
                SuccessOrExit(err);
            }
            else
            {
                if (pClient->CheckForSinksWithDataLoss())
                {
                    ExitNow(err = WEAVE_ERROR_WDM_POTENTIAL_DATA_LOSS);
                }
            }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

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

            // Emit an OnSubscriptionActivity event
            inParam.mSubscriptionActivity.mClient = pClient;
            pClient->mEventCallback(pClient->mAppState, kEvent_OnSubscriptionActivity, inParam, outParam);

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

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    if (isLocked)
    {
        pClient->Unlock();
    }
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
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

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
void SubscriptionClient::SetMaxUpdateSize(const uint32_t aMaxSize)
{
    if (aMaxSize > UINT16_MAX)
        mMaxUpdateSize = 0;
    else
        mMaxUpdateSize = aMaxSize;
}

/**
 * Move paths from the dispatched store back to the pending one.
 * Skip the private ones, as they will be re-added during the recursion.
 */
WEAVE_ERROR SubscriptionClient::MoveInProgressToPending(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t numSourceItems = mInProgressUpdateList.GetNumItems();
    TraitDataSink *dataSink;
    TraitPath traitPath;

    for (size_t i = 0; i < numSourceItems; i++)
    {
        if (mInProgressUpdateList.IsItemInUse(i))
        {
            mInProgressUpdateList.GetItemAt(i, traitPath);

            if ( ! mInProgressUpdateList.AreFlagsSet(i, kFlag_Private))
            {
                err = mDataSinkCatalog->Locate(traitPath.mTraitDataHandle, &dataSink);
                SuccessOrExit(err);
                err = AddItemPendingUpdateSet(traitPath, dataSink->GetSchemaEngine());
                SuccessOrExit(err);
            }
        }
    }

    mInProgressUpdateList.Clear();

    if (mPendingSetState == kPendingSetEmpty)
    {
        SetPendingSetState(kPendingSetReady);
    }

exit:
    return err;
}

// Move the pending set to the in-progress list, grouping the
// paths by trait instance
WEAVE_ERROR SubscriptionClient::MovePendingToInProgress(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath traitPath;
    UpdatableTIContext * traitInfo;
    int count = 0;

    VerifyOrDie(mInProgressUpdateList.IsEmpty());

    // TODO: if we send too many DataElements in the same UpdateRequest, the response
    // is never received. Untill the problem is rootcaused and fixed, the loop below
    // limits the number of items transferred to mInProgressUpdateList.
    // 94 items triggers the problem; 75 does not. Using a value of 50 to be safe (more
    // DataElements are generated during the encoding).

    for (size_t traitInstance = 0; traitInstance < mNumUpdatableTraitInstances; traitInstance++)
    {
        traitInfo = mClientTraitInfoPool + traitInstance;

        for (size_t i = mPendingUpdateSet.GetFirstValidItem(traitInfo->mTraitDataHandle);
                i < mPendingUpdateSet.GetPathStoreSize();
                i = mPendingUpdateSet.GetNextValidItem(i, traitInfo->mTraitDataHandle))
        {
            mPendingUpdateSet.GetItemAt(i, traitPath);

            err = mInProgressUpdateList.AddItem(traitPath);
            SuccessOrExit(err);

            mPendingUpdateSet.RemoveItemAt(i); // Temp hack: remove this line

            count++;
        }
    }

    // Temp hack: uncomment this line
    // mPendingUpdateSet.Clear();

    if (mPendingUpdateSet.IsEmpty())
    {
        SetPendingSetState(kPendingSetEmpty);
    }

exit:
    WeaveLogDetail(DataManagement, "Moved %d items from Pending to InProgress; err %" PRId32 "", count, err);

    return err;
}

/**
 * Clears the list of paths, giving a callback
 * to the application with an internal error for each
 * path still in the list.
 */
void SubscriptionClient::ClearPathStore(TraitPathStore &aPathStore, WEAVE_ERROR aErr)
{
    TraitPath traitPath;

    for (size_t j = 0; j < aPathStore.GetPathStoreSize(); j++)
    {
        if (! aPathStore.IsItemInUse(j))
        {
            continue;
        }
        aPathStore.GetItemAt(j, traitPath);
        if (! aPathStore.AreFlagsSet(j, kFlag_Private))
        {
            UpdateCompleteEventCbHelper(traitPath,
                                        nl::Weave::Profiles::kWeaveProfile_Common,
                                        nl::Weave::Profiles::Common::kStatus_InternalError,
                                        aErr);
        }
    }

    aPathStore.Clear();
}

/**
 * Notify the application for each failed pending path and
 * remove it from the pending set.
 * Return the number of paths removed.
 */
size_t SubscriptionClient::PurgeFailedPendingPaths(WEAVE_ERROR aErr)
{
    TraitPath traitPath;
    TraitUpdatableDataSink *updatableDataSink = NULL;
    size_t count = 0;


    for (size_t j = 0; j < mPendingUpdateSet.GetPathStoreSize(); j++)
    {
        if (! mPendingUpdateSet.IsItemInUse(j))
        {
            continue;
        }
        if (mPendingUpdateSet.IsItemFailed(j))
        {
            mPendingUpdateSet.GetItemAt(j, traitPath);
            updatableDataSink = Locate(traitPath.mTraitDataHandle);
            updatableDataSink->ClearVersion();
            updatableDataSink->ClearUpdateRequiredVersion();
            updatableDataSink->SetConditionalUpdate(false);

            if (! mPendingUpdateSet.AreFlagsSet(j, kFlag_Private))
            {
                UpdateCompleteEventCbHelper(traitPath,
                        nl::Weave::Profiles::kWeaveProfile_Common,
                        nl::Weave::Profiles::Common::kStatus_InternalError,
                        aErr);
            }
            mPendingUpdateSet.RemoveItemAt(j);
            count++;
        }
    }

    if (mPendingUpdateSet.IsEmpty())
    {
        SetPendingSetState(kPendingSetEmpty);
    }

    return count;
}

WEAVE_ERROR SubscriptionClient::AddItemPendingUpdateSet(const TraitPath &aItem, const TraitSchemaEngine * const aSchemaEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mPendingUpdateSet.AddItemDedup(aItem, aSchemaEngine);

    WeaveLogDetail(DataManagement, "%s t%u, p%u, err %d", __func__, aItem.mTraitDataHandle, aItem.mPropertyPathHandle, err);
    return err;
}

/**
 * Add a private path in the list of paths in progress,
 * inserting it after the one being encoded right now.
 */
WEAVE_ERROR SubscriptionClient::InsertInProgressUpdateItem(const TraitPath &aItem,
                                                           const TraitSchemaEngine * const aSchemaEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitPath traitPath;
    TraitPathStore::Flags flags = (kFlag_Private | kFlag_ForceMerge);

    err = mInProgressUpdateList.InsertItemAfter(mUpdateRequestContext.mItemInProgress, aItem, flags);
    SuccessOrExit(err);

exit:
    WeaveLogDetail(DataManagement, "%s %u t%u, p%u  numItems: %u, err %d", __func__,
            mUpdateRequestContext.mItemInProgress,
            aItem.mTraitDataHandle, aItem.mPropertyPathHandle,
            mInProgressUpdateList.GetNumItems(), err);

    return err;
}

void SubscriptionClient::RemoveInProgressPrivateItemsAfter(uint16_t aItemInProgress)
{
    int count = 0;

    for (size_t i = mInProgressUpdateList.GetNextValidItem(aItemInProgress);
            i < mInProgressUpdateList.GetPathStoreSize();
            i = mInProgressUpdateList.GetNextValidItem(i))
    {
        if (mInProgressUpdateList.AreFlagsSet(i, kFlag_Private))
        {
            mInProgressUpdateList.RemoveItemAt(i);
            count++;
        }
    }

    if (count > 0)
    {
        mInProgressUpdateList.Compact();
    }

    WeaveLogDetail(DataManagement, "Removed %d private InProgress items after %u; numItems: %u",
            count, aItemInProgress, mInProgressUpdateList.GetNumItems());
}

void SubscriptionClient::ClearPotentialDataLoss(TraitDataHandle aTraitDataHandle)
{
    TraitUpdatableDataSink *updatableDataSink = Locate(aTraitDataHandle);

    if (updatableDataSink->IsPotentialDataLoss())
    {
        WeaveLogDetail(DataManagement, "Potential data loss cleared for traitDataHandle: %" PRIu16 ", trait %08x",
                aTraitDataHandle, updatableDataSink->GetSchemaEngine()->GetProfileId());
    }

    updatableDataSink->SetPotentialDataLoss(false);
}

void SubscriptionClient::MarkFailedPendingPaths(TraitDataHandle aTraitDataHandle, const DataVersion &aLatestVersion)
{
    if (! IsUpdateInFlight())
    {
        TraitUpdatableDataSink *updatableDataSink = Locate(aTraitDataHandle);

        if (updatableDataSink->IsConditionalUpdate() &&
                IsVersionNewer(aLatestVersion, updatableDataSink->GetUpdateRequiredVersion()))
        {
            WeaveLogDetail(DataManagement, "<MarkFailedPendingPaths> current version 0x%" PRIx64
                    ", valid: %d"
                    ", updateRequiredVersion: 0x%" PRIx64
                    ", latest known version: 0x%" PRIx64 "",
                    updatableDataSink->GetVersion(),
                    updatableDataSink->IsVersionValid(),
                    updatableDataSink->GetUpdateRequiredVersion(),
                    aLatestVersion);

            mPendingUpdateSet.SetFailedTrait(aTraitDataHandle);
        }
    }

    return;
}

bool SubscriptionClient::FilterNotifiedPath(TraitDataHandle aTraitDataHandle,
                                            PropertyPathHandle aLeafPathHandle,
                                            const TraitSchemaEngine * const aSchemaEngine)
{
    bool retval = false;

    retval = mInProgressUpdateList.Includes(TraitPath(aTraitDataHandle, aLeafPathHandle), aSchemaEngine) ||
             mPendingUpdateSet.Includes(TraitPath(aTraitDataHandle, aLeafPathHandle), aSchemaEngine);

    if (retval)
    {
        TraitUpdatableDataSink *updatableDataSink = Locate(aTraitDataHandle);

        if (false == updatableDataSink->IsPotentialDataLoss())
        {
            updatableDataSink->SetPotentialDataLoss(true);

            WeaveLogDetail(DataManagement, "Potential data loss set for traitDataHandle: %" PRIu16 ", trait %08x pathHandle: %" PRIu32 "",
                    aTraitDataHandle, aSchemaEngine->GetProfileId(), aLeafPathHandle);
        }
    }

    return retval;
}

WEAVE_ERROR SubscriptionClient::Lock()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mLock)
    {
        err = mLock->Lock();
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(DataManagement, "Lock failed with %d", err);
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR SubscriptionClient::Unlock()
{
    if (mLock)
    {
        return mLock->Unlock();
    }

    return WEAVE_NO_ERROR;
}

void SubscriptionClient::UpdateCompleteEventCbHelper(const TraitPath &aTraitPath, uint32_t aStatusProfileId, uint16_t aStatusCode, WEAVE_ERROR aReason)
{
    InEventParam inParam;
    OutEventParam outParam;

    if (aReason == WEAVE_NO_ERROR &&
            false == (aStatusProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
                      aStatusCode == nl::Weave::Profiles::Common::kStatus_Success))
    {
        aReason = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
    }

    inParam.Clear();
    outParam.Clear();
    inParam.mUpdateComplete.mClient = this;
    inParam.mUpdateComplete.mStatusProfileId = aStatusProfileId;
    inParam.mUpdateComplete.mStatusCode = aStatusCode;
    inParam.mUpdateComplete.mReason = aReason;
    inParam.mUpdateComplete.mTraitDataHandle = aTraitPath.mTraitDataHandle;
    inParam.mUpdateComplete.mPropertyPathHandle = aTraitPath.mPropertyPathHandle;

    mEventCallback(mAppState, kEvent_OnUpdateComplete, inParam, outParam);
}

void SubscriptionClient::NoMorePendingEventCbHelper()
{
    InEventParam inParam;
    OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mEventCallback(mAppState, kEvent_OnNoMorePendingUpdates, inParam, outParam);
}

void SubscriptionClient::SetPendingSetState(PendingSetState aState)
{
    if (aState != mPendingSetState)
    {
        WeaveLogDetail(DataManagement, "PendingSetState %d -> %d", mPendingSetState, aState);
    }
    mPendingSetState = aState;
}

// TODO: Break this method down into smaller methods.
void SubscriptionClient::OnUpdateConfirm(WEAVE_ERROR aReason, nl::Weave::Profiles::StatusReporting::StatusReport * apStatus)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool isLocked = false;
    TraitPath traitPath;
    TraitUpdatableDataSink * updatableDataSink = NULL;
    UpdateResponse::Parser response;
    ReferencedTLVData additionalInfo;
    uint32_t numDispatchedHandles;
    StatusList::Parser statusList;
    VersionList::Parser versionList;
    uint64_t versionCreated = 0;
    bool IsVersionListPresent = false;
    bool IsStatusListPresent = false;
    nl::Weave::TLV::TLVReader reader;
    uint32_t profileID;
    uint16_t statusCode;
    bool wholeRequestSucceeded = false;
    bool needToResubscribe = false;

    err = Lock();
    SuccessOrExit(err);

    isLocked = true;

    numDispatchedHandles = mInProgressUpdateList.GetNumItems();
    additionalInfo = apStatus->mAdditionalInfo;
    ClearUpdateInFlight();

    if (mUpdateRequestContext.mIsPartialUpdate)
    {
        WeaveLogDetail(DataManagement, "Got StatusReport in the middle of a long update");

        // TODO: implement a simple FSM to handle long updates

        mUpdateRequestContext.mIsPartialUpdate = false;
        mUpdateRequestContext.mPathToEncode.mPropertyPathHandle = kNullPropertyPathHandle;
        mUpdateRequestContext.mNextDictionaryElementPathHandle = kNullPropertyPathHandle;
    }

    WeaveLogDetail(DataManagement, "Received Status Report 0x%" PRIX32 " : 0x%" PRIX16,
                   apStatus->mProfileId, apStatus->mStatusCode);
    WeaveLogDetail(DataManagement, "Received Status Report additional info %u",
                   additionalInfo.theLength);

    if (apStatus->mProfileId == nl::Weave::Profiles::kWeaveProfile_Common &&
            apStatus->mStatusCode == nl::Weave::Profiles::Common::kStatus_Success)
    {
        // If the whole udpate has succeeded, the status list
        // is allowed to be empty.
        wholeRequestSucceeded = true;
    }

    profileID = apStatus->mProfileId;
    statusCode = apStatus->mStatusCode;

    if (additionalInfo.theLength != 0)
    {
        reader.Init(additionalInfo.theData, additionalInfo.theLength);

        err = reader.Next();
        SuccessOrExit(err);

        err = response.Init(reader);
        SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        err = response.CheckSchemaValidity();
        SuccessOrExit(err);
#endif

        err = response.GetVersionList(&versionList);
        switch (err)
        {
        case WEAVE_NO_ERROR:
            IsVersionListPresent = true;
            break;
        case WEAVE_END_OF_TLV:
            err = WEAVE_NO_ERROR;
            break;
        default:
            ExitNow();
            break;
        }

        err = response.GetStatusList(&statusList);
        switch (err)
        {
        case WEAVE_NO_ERROR:
            IsStatusListPresent = true;
            break;
        case WEAVE_END_OF_TLV:
            err = WEAVE_NO_ERROR;
            break;
        default:
            ExitNow();
            break;
        }
    }

    if ((wholeRequestSucceeded) && !(IsStatusListPresent && IsVersionListPresent))
    {
        WeaveLogDetail(DataManagement, "<OnUpdateConfirm> version/status list missing");
        ExitNow(err = WEAVE_ERROR_WDM_MALFORMED_UPDATE_RESPONSE);
    }

    // TODO: validate that the version and status lists are either empty or contain
    // the same number of items as the dispatched list

    for (size_t j = 0; j < numDispatchedHandles; j++)
    {
        VerifyOrDie(mInProgressUpdateList.IsItemValid(j));

        if (IsVersionListPresent)
        {
            err = versionList.Next();
            SuccessOrExit(err);

            err = versionList.GetVersion(&versionCreated);
            SuccessOrExit(err);
        }

        if ((! wholeRequestSucceeded) && IsStatusListPresent)
        {
            err = statusList.Next();

            err = statusList.GetStatusAndProfileID(&profileID, &statusCode);
            SuccessOrExit(err);
        }

        err = WEAVE_NO_ERROR;

        mInProgressUpdateList.GetItemAt(j, traitPath);

        updatableDataSink = Locate(traitPath.mTraitDataHandle);

        if (! mInProgressUpdateList.AreFlagsSet(j, kFlag_Private))
        {
            UpdateCompleteEventCbHelper(traitPath, profileID, statusCode, aReason);
        }

        mInProgressUpdateList.RemoveItemAt(j);

        WeaveLogDetail(DataManagement, "item: %zu, profile: %" PRIu32 ", statusCode: 0x% " PRIx16 ", version 0x%" PRIx64 "",
                j, profileID, statusCode, versionCreated);
        WeaveLogDetail(DataManagement, "item: %zu, traitDataHandle: %" PRIu16 ", pathHandle: %" PRIu32 "",
                j, traitPath.mTraitDataHandle, traitPath.mPropertyPathHandle);

        if (profileID == nl::Weave::Profiles::kWeaveProfile_Common &&
                statusCode == nl::Weave::Profiles::Common::kStatus_Success)
        {
            if (updatableDataSink->IsConditionalUpdate())
            {
                if (updatableDataSink->IsVersionValid() &&
                        versionCreated > updatableDataSink->GetVersion() &&
                        updatableDataSink->GetVersion() >= updatableDataSink->GetUpdateStartVersion())
                {
                    updatableDataSink->SetVersion(versionCreated);
                }
                if (mPendingUpdateSet.IsPresent(traitPath))
                {
                    updatableDataSink->SetUpdateRequiredVersion(versionCreated);
                }
                else
                {
                    updatableDataSink->ClearUpdateRequiredVersion();
                    updatableDataSink->SetConditionalUpdate(false);
                }
            }

            if (updatableDataSink->IsPotentialDataLoss() &&
                    updatableDataSink->IsVersionValid() &&
                    versionCreated >= updatableDataSink->GetVersion() &&
                    updatableDataSink->GetVersion() >= updatableDataSink->GetUpdateStartVersion())
            {
                ClearPotentialDataLoss(traitPath.mTraitDataHandle);
            }
        } // Success
        else
        {
            if (profileID == nl::Weave::Profiles::kWeaveProfile_WDM &&
                    statusCode == nl::Weave::Profiles::DataManagement::kStatus_VersionMismatch)
            {
                // Fail all pending ones as well for VersionMismatch and force resubscribe
                if (mPendingUpdateSet.IsTraitPresent(traitPath.mTraitDataHandle))
                {
                    mPendingUpdateSet.SetFailedTrait(traitPath.mTraitDataHandle);
                }
                updatableDataSink->ClearVersion();
                updatableDataSink->ClearUpdateRequiredVersion();
                updatableDataSink->SetConditionalUpdate(false);
                needToResubscribe = true;
            }
            else
            {
                if (updatableDataSink->IsConditionalUpdate() &&
                        mPendingUpdateSet.IsTraitPresent(traitPath.mTraitDataHandle))
                {
                    mPendingUpdateSet.SetFailedTrait(traitPath.mTraitDataHandle);
                    updatableDataSink->ClearUpdateRequiredVersion();
                    updatableDataSink->SetConditionalUpdate(false);
                }

                if (updatableDataSink->IsVersionValid())
                {
                    WeaveLogDetail(DataManagement, "Clearing version for tdh %d, trait %08x",
                            traitPath.mTraitDataHandle,
                            updatableDataSink->GetSchemaEngine()->GetProfileId());

                    updatableDataSink->ClearVersion();
                    needToResubscribe = true;
                }
            }
        } // Not success
    } // for numDispatchedHandles

exit:

    // If the loop above exited early for an error, the application
    // is notified for any remaining path by the following method.
    ClearPathStore(mInProgressUpdateList, err);

    mUpdateRequestContext.mItemInProgress = 0;

    if (needToResubscribe)
    {
        WeaveLogDetail(DataManagement, "UpdateResponse: triggering resubscription");
    }

    // TODO: should the purge happen only if the pending set is ready?
    PurgePendingUpdate();

    if (mPendingSetState == kPendingSetReady)
    {
        // TODO: handle error!
        FormAndSendUpdate(true);
    }
    else if (mPendingSetState == kPendingSetEmpty)
    {
        NoMorePendingEventCbHelper();

        if (CheckForSinksWithDataLoss())
        {
            needToResubscribe = true;
        }
    }

    if (needToResubscribe && IsEstablishedIdle())
    {
        HandleSubscriptionTerminated(IsRetryEnabled(), err, NULL);
    }

    if (isLocked)
    {
        Unlock();
    }

    WeaveLogFunctError(err);
}

/**
 * This handler is optimized for the case that the request never reached the
 * responder: the dispatched paths are put back in the pending queue and retried.
 */
void SubscriptionClient::OnUpdateNoResponse(WEAVE_ERROR aError)
{
    // TODO: no test for this yet

    TraitPath traitPath;
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool isLocked = false;
    uint32_t numDispatchedHandles = mInProgressUpdateList.GetPathStoreSize();

    err = Lock();
    SuccessOrExit(err);

    isLocked = true;

    ClearUpdateInFlight();

    // Notify the app for all dispatched paths.
    // TODO: this implementation is incomplete...
    for (size_t j = 0; j < numDispatchedHandles; j++)
    {
        if (! mInProgressUpdateList.IsItemValid(j))
        {
            continue;
        }

        if (! mInProgressUpdateList.AreFlagsSet(j, kFlag_Private))
        {
            // TODO: does it make sense to put a profile and status when we never received a StatusReport?
            UpdateCompleteEventCbHelper(traitPath, nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kStatus_Timeout, aError);
        }
    }

    //Move paths from DispatchedUpdates to PendingUpdates for all TIs.
    err = MoveInProgressToPending();
    mUpdateRequestContext.mItemInProgress = 0;
    if (err != WEAVE_NO_ERROR)
    {
        // Fail everything; think about dictionaries spread over
        // more than one DataElement
        ClearPathStore(mInProgressUpdateList, WEAVE_ERROR_NO_MEMORY);
        ClearPathStore(mPendingUpdateSet, WEAVE_ERROR_NO_MEMORY);
    }
    else
    {
        PurgePendingUpdate();
    }

    if ((false == mPendingUpdateSet.IsEmpty()) && IsEstablishedIdle())
    {
        HandleSubscriptionTerminated(IsRetryEnabled(), aError, NULL);
    }

exit:

    if (isLocked)
    {
        Unlock();
    }
}

void SubscriptionClient::UpdateEventCallback (void * const aAppState,
                                              UpdateClient::EventType aEvent,
                                              const UpdateClient::InEventParam & aInParam,
                                              UpdateClient::OutEventParam & aOutParam)
{
    SubscriptionClient * const pSubClient = reinterpret_cast<SubscriptionClient *>(aAppState);

    VerifyOrExit(!(pSubClient->IsAborting() ||pSubClient->IsAborted()),
            WeaveLogDetail(DataManagement, "<UpdateEventCallback> subscription has been aborted"));

    switch (aEvent)
    {
    case UpdateClient::kEvent_UpdateComplete:
        WeaveLogDetail(DataManagement, "UpdateComplete event: %d", aEvent);

        if (aInParam.UpdateComplete.Reason == WEAVE_NO_ERROR)
        {
            pSubClient->OnUpdateConfirm(aInParam.UpdateComplete.Reason, aInParam.UpdateComplete.StatusReportPtr);
        }
        else
        {
            pSubClient->OnUpdateNoResponse(aInParam.UpdateComplete.Reason);
        }

        break;
    case UpdateClient::kEvent_UpdateContinue:
        WeaveLogDetail(DataManagement, "UpdateContinue event: %d", aEvent);
        pSubClient->ClearUpdateInFlight();
        // TODO: handle error!
        pSubClient->FormAndSendUpdate(true);
        break;
    default:
        WeaveLogDetail(DataManagement, "Unknown UpdateClient event: %d", aEvent);
        break;
    }

exit:
    return;
}

WEAVE_ERROR SubscriptionClient::SetUpdated(TraitUpdatableDataSink * aDataSink, PropertyPathHandle aPropertyHandle, bool aIsConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataHandle dataHandle;
    bool isLocked = false;
    const TraitSchemaEngine * schemaEngine;
    bool needToSetUpdateRequiredVersion = false;
    bool isTraitInstanceInUpdate = false;

    err = Lock();
    SuccessOrExit(err);

    isLocked = true;

    if (aIsConditional)
    {
        if (!aDataSink->IsVersionValid())
        {
            err = WEAVE_ERROR_INCORRECT_STATE;
            WeaveLogDetail(DataManagement, "Rejected mutation with error %d", err);
            ExitNow();
        }
    }

    schemaEngine = aDataSink->GetSchemaEngine();

    err = mDataSinkCatalog->Locate(aDataSink, dataHandle);
    SuccessOrExit(err);

    isTraitInstanceInUpdate = mPendingUpdateSet.IsTraitPresent(dataHandle) ||
                              mInProgressUpdateList.IsTraitPresent(dataHandle);

    // It is not supported to mix conditional and non-conditional updates
    // in the same trait.
    if (isTraitInstanceInUpdate)
    {
        VerifyOrExit(aIsConditional == aDataSink->IsConditionalUpdate(), err = WEAVE_ERROR_INCORRECT_STATE);
    }
    else
    {
        if (aIsConditional)
        {
            needToSetUpdateRequiredVersion = true;
        }
    }

    err = AddItemPendingUpdateSet(TraitPath(dataHandle, aPropertyHandle), schemaEngine);
    SuccessOrExit(err);

    if (aIsConditional)
    {
        if (needToSetUpdateRequiredVersion)
        {
            uint64_t requiredDataVersion = aDataSink->GetVersion();
            aDataSink->SetUpdateRequiredVersion(requiredDataVersion);
            WeaveLogDetail(DataManagement, "<SetUpdated> Set update required version to 0x%" PRIx64 "", aDataSink->GetUpdateRequiredVersion());
        }

    }
    aDataSink->SetConditionalUpdate(aIsConditional);

exit:
    if (err == WEAVE_NO_ERROR)
    {
        SetPendingSetState(kPendingSetOpen);
    }

    if (isLocked)
    {
        Unlock();
    }

    return err;
}

/**
 * Fail all conditional pending paths that have become obsolete and
 * notify the application.
 */
WEAVE_ERROR SubscriptionClient::PurgePendingUpdate()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitUpdatableDataSink * updatableDataSink;
    UpdatableTIContext * traitInfo;
    bool isLocked = false;
    size_t numUpdatableTraitInstances = GetNumUpdatableTraitInstances();
    size_t numPendingPathsDeleted = 0;

    // Lock before attempting to modify any of the shared data structures.
    err = Lock();
    SuccessOrExit(err);

    isLocked = true;

    WeaveLogDetail(DataManagement, "PurgePendingUpdate: numItems before: %d", mPendingUpdateSet.GetNumItems());

    VerifyOrExit(mPendingUpdateSet.GetNumItems() > 0, );

    for (size_t i = 0; i < numUpdatableTraitInstances; i++)
    {
        traitInfo = mClientTraitInfoPool + i;
        updatableDataSink = traitInfo->mUpdatableDataSink;

        if (updatableDataSink->IsVersionValid())
        {
            MarkFailedPendingPaths(traitInfo->mTraitDataHandle, updatableDataSink->GetVersion());
        }
    }

    numPendingPathsDeleted = PurgeFailedPendingPaths(WEAVE_ERROR_WDM_VERSION_MISMATCH);

    if (numPendingPathsDeleted > 0 && IsEstablishedIdle())
    {
        HandleSubscriptionTerminated(IsRetryEnabled(), WEAVE_ERROR_WDM_VERSION_MISMATCH, NULL);
    }

exit:

    WeaveLogDetail(DataManagement, "PurgePendingUpdate: numItems after: %d", mPendingUpdateSet.GetNumItems());

    if (isLocked)
    {
        Unlock();
    }

    return err;
}

void SubscriptionClient::CancelUpdateClient(void)
{
    WeaveLogDetail(DataManagement, "SubscriptionClient::CancelUpdateClient");
    ClearUpdateInFlight();
    mUpdateClient.CancelUpdate();
}

void SubscriptionClient::ShutdownUpdateClient(void)
{
    mNumUpdatableTraitInstances        = 0;
    mUpdateRequestContext.mItemInProgress = 0;
    mUpdateRequestContext.mNextDictionaryElementPathHandle   = kNullPropertyPathHandle;
    mPendingUpdateSet.Clear();
    mInProgressUpdateList.Clear();
    mMaxUpdateSize                     = 0;
    mUpdateInFlight                    = false;
    mPendingSetState                   = kPendingSetEmpty;

    mUpdateClient.Shutdown();
}

WEAVE_ERROR SubscriptionClient::AddElementFunc(UpdateClient * apClient, void * apCallState, TLV::TLVWriter & aWriter)
{
    WEAVE_ERROR err;
    TraitUpdatableDataSink * updatableDataSink;
    const TraitSchemaEngine * schemaEngine;
    bool isDictionaryReplace = false;
    nl::Weave::TLV::TLVType dataContainerType;
    uint64_t tag = nl::Weave::TLV::ContextTag(DataElement::kCsTag_Data);

    UpdateRequestContext * updateRequestContext = static_cast<UpdateRequestContext *>(apCallState);
    SubscriptionClient * pSubClient = updateRequestContext->mSubClient;

    updatableDataSink = pSubClient->Locate(updateRequestContext->mPathToEncode.mTraitDataHandle);
    schemaEngine = updatableDataSink->GetSchemaEngine();

    WeaveLogDetail(DataManagement, "<AddElementFunc> with property path handle 0x%08x",
            updateRequestContext->mPathToEncode.mPropertyPathHandle);

    if (schemaEngine->IsDictionary(updateRequestContext->mPathToEncode.mPropertyPathHandle) &&
        !updateRequestContext->mForceMerge)
    {
        isDictionaryReplace = true;
    }

    if (isDictionaryReplace)
    {
        // If the element is a whole dictionary, use the "replace" scheme.
        // The path of the DataElement points to the parent of the dictionary.
        // The data has to be a structure with one element, which is the dictionary itself.
        WeaveLogDetail(DataManagement, "<AddElementFunc> replace dictionary");
        err = aWriter.StartContainer(tag, nl::Weave::TLV::kTLVType_Structure, dataContainerType);
        SuccessOrExit(err);

        tag = schemaEngine->GetTag(updateRequestContext->mPathToEncode.mPropertyPathHandle);
    }

    err = updatableDataSink->ReadData(updateRequestContext->mPathToEncode.mTraitDataHandle,
                                      updateRequestContext->mPathToEncode.mPropertyPathHandle,
                                      tag,
                                      aWriter,
                                      updateRequestContext->mNextDictionaryElementPathHandle);
    SuccessOrExit(err);

    if (isDictionaryReplace)
    {
        err = aWriter.EndContainer(dataContainerType);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR SubscriptionClient::Lookup(TraitDataHandle aTraitDataHandle,
                                       TraitUpdatableDataSink * &updatableDataSink,
                                       const TraitSchemaEngine * &schemaEngine,
                                       ResourceIdentifier &resourceId,
                                       uint64_t &instanceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    updatableDataSink = Locate(aTraitDataHandle);

    schemaEngine = updatableDataSink->GetSchemaEngine();
    VerifyOrDie(schemaEngine != NULL);

    err = mDataSinkCatalog->GetResourceId(aTraitDataHandle, resourceId);
    SuccessOrExit(err);

    err = mDataSinkCatalog->GetInstanceId(aTraitDataHandle, instanceId);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR SubscriptionClient::DirtyPathToDataElement(UpdateRequestContext &aContext)
{
    WEAVE_ERROR err;
    uint32_t numTags = 0;
    ResourceIdentifier resourceId;
    uint64_t instanceId;
    const TraitSchemaEngine * schemaEngine;
    TraitUpdatableDataSink * updatableDataSink  = NULL;

    err = Lookup(aContext.mPathToEncode.mTraitDataHandle,
                 updatableDataSink, schemaEngine, resourceId, instanceId);
    SuccessOrExit(err);

    {
        uint64_t tags[schemaEngine->mSchema.mTreeDepth];

        err = schemaEngine->GetRelativePathTags(aContext.mPathToEncode.mPropertyPathHandle, tags,
                //ArraySize(tags),
                schemaEngine->mSchema.mTreeDepth,
                numTags);
        SuccessOrExit(err);

        if (schemaEngine->IsDictionary(aContext.mPathToEncode.mPropertyPathHandle) &&
                ! aContext.mForceMerge)
        {
            // If the property being updated is a dictionary, we need to use the "replace"
            // scheme explicitly so that the whole property is replaced on the responder.
            // So, the path has to point to the parent of the dictionary.
            VerifyOrExit(numTags > 0, err = WEAVE_ERROR_WDM_SCHEMA_MISMATCH);
            numTags--;
        }

        err = mUpdateClient.AddElement(schemaEngine->GetProfileId(),
                instanceId,
                resourceId,
                updatableDataSink->GetUpdateRequiredVersion(),
                NULL, tags, numTags,
                AddElementFunc, &aContext);
        SuccessOrExit(err);

        aContext.mForceMerge = false;
        aContext.mNumDataElementsAddedToPayload++;
    }

exit:
    return err;
}

WEAVE_ERROR SubscriptionClient::BuildSingleUpdateRequestDataList(UpdateRequestContext &context)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ResourceIdentifier resourceId;
    const TraitSchemaEngine * schemaEngine;
    TraitUpdatableDataSink * updatableDataSink = NULL;
    bool dictionaryOverflowed = false;
    TraitPath traitPath;

    WeaveLogDetail(DataManagement, "Num items in progress = %u/%u; current: %u",
            mInProgressUpdateList.GetNumItems(),
            mInProgressUpdateList.GetPathStoreSize(),
            context.mItemInProgress);

    while (context.mItemInProgress < mInProgressUpdateList.GetPathStoreSize())
    {
        size_t &i = context.mItemInProgress;

        if (!(mInProgressUpdateList.IsItemValid(i)))
        {
            i++;
            continue;
        }

        WeaveLogDetail(DataManagement, "Encoding item %u, ForceMerge: %d, Private: %d", i, mInProgressUpdateList.AreFlagsSet(i, kFlag_ForceMerge),
                mInProgressUpdateList.AreFlagsSet(i, kFlag_Private));

        mInProgressUpdateList.GetItemAt(i, traitPath);

        updatableDataSink = Locate(traitPath.mTraitDataHandle);
        schemaEngine = updatableDataSink->GetSchemaEngine();
        context.mPathToEncode = traitPath;
        context.mForceMerge = mInProgressUpdateList.AreFlagsSet(i, kFlag_ForceMerge);

        if (context.mNextDictionaryElementPathHandle != kNullPropertyPathHandle)
        {
            WeaveLogDetail(DataManagement, "Resume encoding a dictionary");
        }

        err = DirtyPathToDataElement(context);
        SuccessOrExit(err);

        dictionaryOverflowed = (context.mNextDictionaryElementPathHandle != kNullPropertyPathHandle);
        if (dictionaryOverflowed)
        {
            InsertInProgressUpdateItem(traitPath, schemaEngine);
        }

        i++;

        VerifyOrExit(!dictionaryOverflowed, /* no error */);
    }

exit:

    if (mUpdateRequestContext.mNumDataElementsAddedToPayload > 0 &&
            (err == WEAVE_ERROR_BUFFER_TOO_SMALL))
    {
        WeaveLogDetail(DataManagement, "Suppressing error %" PRId32 "; will try again later", err);
        RemoveInProgressPrivateItemsAfter(context.mItemInProgress);
        err = WEAVE_NO_ERROR;
    }

    if (err == WEAVE_NO_ERROR)
    {
        mUpdateRequestContext.mIsPartialUpdate =
            (context.mItemInProgress < mInProgressUpdateList.GetPathStoreSize());
    }
    else
    {
        traitPath.mPropertyPathHandle = kRootPropertyPathHandle;

        // TODO: there is no coverage for this yet
        WeaveLogDetail(DataManagement, "%s failed: %d", __func__, err);

        if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
        {
            WeaveLogDetail(DataManagement, "illegal oversized trait property is too big to fit in the packet");
        }

        UpdateCompleteEventCbHelper(traitPath,
                                    nl::Weave::Profiles::kWeaveProfile_Common,
                                    nl::Weave::Profiles::Common::kStatus_InternalError,
                                    err);

        mInProgressUpdateList.RemoveTrait(traitPath.mTraitDataHandle);
        mPendingUpdateSet.RemoveTrait(traitPath.mTraitDataHandle);

        updatableDataSink->ClearVersion();
        updatableDataSink->ClearUpdateRequiredVersion();
        updatableDataSink->SetConditionalUpdate(false);

        context.mNextDictionaryElementPathHandle = kNullPropertyPathHandle;

        if (IsEstablishedIdle())
        {
            HandleSubscriptionTerminated(IsRetryEnabled(), err, NULL);
        }
    }

    return err;
}

void SubscriptionClient::SetUpdateStartVersions(void)
{
    TraitPath traitPath;
    TraitUpdatableDataSink *updatableSink;

    for (size_t i = mInProgressUpdateList.GetFirstValidItem();
            i < mInProgressUpdateList.GetPathStoreSize();
            i = mInProgressUpdateList.GetNextValidItem(i))
    {
        mInProgressUpdateList.GetItemAt(i, traitPath);

        updatableSink = Locate(traitPath.mTraitDataHandle);

        updatableSink->SetUpdateStartVersion();
    }
}

WEAVE_ERROR SubscriptionClient::SendSingleUpdateRequest(void)
{
    WEAVE_ERROR err   = WEAVE_NO_ERROR;
    uint32_t maxUpdateSize;

    maxUpdateSize = GetMaxUpdateSize();

    mUpdateRequestContext.mSubClient = this;
    mUpdateRequestContext.mNumDataElementsAddedToPayload = 0;
    mUpdateRequestContext.mIsPartialUpdate = false;

    err = mUpdateClient.StartUpdate(0, NULL, maxUpdateSize);
    SuccessOrExit(err);

    err = BuildSingleUpdateRequestDataList(mUpdateRequestContext);
    SuccessOrExit(err);

    if (mUpdateRequestContext.mNumDataElementsAddedToPayload)
    {
        if (false == mUpdateRequestContext.mIsPartialUpdate)
        {
            SetUpdateStartVersions();
        }

        WeaveLogDetail(DataManagement, "Sending update");
        // TODO: SetUpdateInFlight is here instead of after SendUpdate
        // to be able to inject timeouts; must improve this..
        SetUpdateInFlight();

        WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_UpdateRequestSendError,
                           nl::Weave::FaultInjection::GetManager().FailAtFault(
                               nl::Weave::FaultInjection::kFault_WRMSendError,
                               0, 1));

        err = mUpdateClient.SendUpdate(mUpdateRequestContext.mIsPartialUpdate);
        SuccessOrExit(err);

        WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_DelayUpdateResponse,
                           nl::Weave::FaultInjection::GetManager().FailAtFault(
                               nl::Weave::FaultInjection::kFault_DropIncomingUDPMsg,
                               0, 1));

    }
    else
    {
        mUpdateClient.CancelUpdate();
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        ClearUpdateInFlight();
        mUpdateClient.CancelUpdate();
    }

    WeaveLogFunctError(err);
    return err;
}

WEAVE_ERROR SubscriptionClient::FormAndSendUpdate(bool aNotifyOnError)
{
    WEAVE_ERROR err                  = WEAVE_NO_ERROR;
    bool isLocked                    = false;
    InEventParam inParam;
    OutEventParam outParam;

    // Lock before attempting to modify any of the shared data structures.
    err = Lock();
    SuccessOrExit(err);

    isLocked = true;

    // TODO: doesn't this prevent an unconditional update before the
    // subscription is established?
    VerifyOrExit(IsEstablishedIdle(), WeaveLogDetail(DataManagement, "client is not active"));

    VerifyOrExit(!IsUpdateInFlight(), WeaveLogDetail(DataManagement, "updating is ongoing"));

    if (mInProgressUpdateList.IsEmpty() && mPendingSetState == kPendingSetReady)
    {
        MovePendingToInProgress();
    }

    WeaveLogDetail(DataManagement, "Eval Subscription: (state = %s, num-updatableTraits = %u)!",
            GetStateStr(), mNumUpdatableTraitInstances);
    // This is needed because some error could trigger abort on subscription, which leads to destroy of the handler

    err = SendSingleUpdateRequest();
    SuccessOrExit(err);

    WeaveLogDetail(DataManagement, "Done update processing!");

exit:

    if (isLocked)
    {
        Unlock();
    }

    if (aNotifyOnError && WEAVE_NO_ERROR != err)
    {
        inParam.Clear();
        outParam.Clear();
        inParam.mUpdateComplete.mClient = this;
        inParam.mUpdateComplete.mReason = err;
        mEventCallback(mAppState, kEvent_OnUpdateComplete, inParam, outParam);
    }

    WeaveLogFunctError(err);
    return err;
}

/**
 * Signals that the application has finished mutating all TraitUpdatableDataSinks.
 * Unless a previous update exchange is in progress, the client will
 * take all data marked as updated and send it to the responder in one update request.
 *
 * @return WEAVE_NO_ERROR in case of success; other WEAVE_ERROR codes in case of failure.
 */
WEAVE_ERROR SubscriptionClient::FlushUpdate()
{
    WEAVE_ERROR err                  = WEAVE_NO_ERROR;
    bool isLocked                    = false;

    // Lock before attempting to modify any of the shared data structures.
    err = Lock();
    SuccessOrExit(err);

    VerifyOrExit(mPendingSetState == kPendingSetOpen,
            WeaveLogDetail(DataManagement, "%s: PendingSetState: %d", __func__, mPendingSetState));

    SetPendingSetState(kPendingSetReady);

    VerifyOrExit(mUpdateInFlight == false,
            WeaveLogDetail(DataManagement, "%s: update in flight", __func__, mPendingSetState));

    err = FormAndSendUpdate(false);
    SuccessOrExit(err);

exit:

    if (isLocked)
    {
        Unlock();
    }

    return err;
}

bool SubscriptionClient::CheckForSinksWithDataLoss()
{
    bool needToResubscribe = false;

    mDataSinkCatalog->Iterate(CheckForSinksWithDataLossIteratorCb, &needToResubscribe);

    return needToResubscribe;
}

void SubscriptionClient::CheckForSinksWithDataLossIteratorCb(void * aDataSink, TraitDataHandle aDataHandle, void * aContext)
{
    TraitDataSink * dataSink = static_cast<TraitDataSink *>(aDataSink);
    TraitUpdatableDataSink * updatableDataSink = NULL;
    bool *needToResubscribe = static_cast<bool *>(aContext);

    VerifyOrExit(dataSink->IsUpdatableDataSink() == true, /* no error */);

    updatableDataSink = static_cast<TraitUpdatableDataSink *>(dataSink);

    if (updatableDataSink->IsPotentialDataLoss())
    {
        WeaveLogDetail(DataManagement, "Need to resubscribe for potential data loss in TDH %d, trait %08x",
                aDataHandle,
                updatableDataSink->GetSchemaEngine()->GetProfileId());

        updatableDataSink->ClearVersion();
        updatableDataSink->ClearUpdateRequiredVersion();
        updatableDataSink->SetConditionalUpdate(false);
        *needToResubscribe = true;
    }

exit:
    return;
}

void SubscriptionClient::InitUpdatableSinkTrait(void * aDataSink, TraitDataHandle aDataHandle, void * aContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitUpdatableDataSink * updatableDataSink = NULL;
    SubscriptionClient * subClient = NULL;
    UpdatableTIContext * traitInstance = NULL;
    TraitDataSink * dataSink = static_cast<TraitDataSink *>(aDataSink);

    VerifyOrExit(dataSink->IsUpdatableDataSink() == true, /* no error */);

    updatableDataSink = static_cast<TraitUpdatableDataSink *>(dataSink);

    subClient = static_cast<SubscriptionClient *>(aContext);
    updatableDataSink->SetSubscriptionClient(subClient);
    updatableDataSink->ClearUpdateRequiredVersion();
    updatableDataSink->SetConditionalUpdate(false);

    VerifyOrExit(subClient->mNumUpdatableTraitInstances < WDM_CLIENT_MAX_NUM_UPDATABLE_TRAITS,
                 err = WEAVE_ERROR_NO_MEMORY);

    traitInstance = subClient->mClientTraitInfoPool + subClient->mNumUpdatableTraitInstances;
    ++(subClient->mNumUpdatableTraitInstances);
    traitInstance->Init(updatableDataSink, aDataHandle);

exit:

    if (WEAVE_NO_ERROR != err)
    {
        InEventParam inParam;
        OutEventParam outParam;

        inParam.Clear();
        outParam.Clear();
        inParam.mUpdateComplete.mClient = subClient;
        inParam.mUpdateComplete.mReason = err;
        subClient->mEventCallback(subClient->mAppState, kEvent_OnUpdateComplete, inParam, outParam);

        WeaveLogDetail(DataManagement, "run out of updatable trait instances");

        /* TODO: this iteration is invoked by SubscriptionClient::Init(); the event
         * given to the application in case of error is not right.
         * We should store an error in the context, so that Init can return error.
         * Assert for now.
         */
        WeaveDie();
    }

    return;
}

TraitUpdatableDataSink *SubscriptionClient::Locate(TraitDataHandle aTraitDataHandle) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataSink *dataSink = NULL;
    TraitUpdatableDataSink *updatableDataSink = NULL;

    err = mDataSinkCatalog->Locate(aTraitDataHandle, &dataSink);
    SuccessOrExit(err);

    VerifyOrExit(dataSink->IsUpdatableDataSink(), );

    updatableDataSink = static_cast<TraitUpdatableDataSink *>(dataSink);

exit:
    VerifyOrDie(NULL != updatableDataSink);

    return updatableDataSink;
}

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE
}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

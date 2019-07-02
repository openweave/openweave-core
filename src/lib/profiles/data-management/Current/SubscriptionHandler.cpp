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
 *      This file implements notification handler for Weave
 *      Data Management (WDM) profile.
 *
 */

// __STDC_FORMAT_MACROS must be defined for PRIX64 to be defined for pre-C++11 clib
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX and INT32_MAX to be defined for pre-C++11 clib
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

// __STDC_CONSTANT_MACROS must be defined for INT64_C and UINT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/WeaveFaultInjection.h>

#include <Weave/Profiles/status-report/StatusReportProfile.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

SubscriptionHandler::SubscriptionHandler() { }

void SubscriptionHandler::InitAsFree()
{
    // These variables are going to be changed and reset along with subscription state machine
    // Must update SubscriptionHandler::Abort for anything added here
    mAppState                      = NULL;
    mEventCallback                 = NULL;
    mCurrentState                  = kState_Free;
    mEC                            = NULL;
    mLivenessTimeoutMsec           = kNoTimeout;
    mPeerNodeId                    = 0;
    mSubscriptionId                = 0;
    mBinding                       = NULL;
    mRefCount                      = 0;
    mIsInitiator                   = false;
    mTraitInstanceList             = NULL;
    mNumTraitInstances             = 0;
    mMaxNotificationSize           = 0;
    mSubscribeToAllEvents          = false;
    mCurProcessingTraitInstanceIdx = 0;
    mCurrentImportance             = kImportanceType_Invalid;
    mBytesOffloaded                = 0;

    memset(mSelfVendedEvents, 0, sizeof(mSelfVendedEvents));
    memset(mLastScheduledEventId, 0, sizeof(mLastScheduledEventId));
}

WEAVE_ERROR SubscriptionHandler::AcceptSubscribeRequest(const uint32_t aLivenessTimeoutSec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    VerifyOrExit(kState_Subscribing_Evaluating == mCurrentState, err = WEAVE_ERROR_INCORRECT_STATE);

    if (mIsInitiator)
    {
        // Do nothing, as the handler on the initiator side of a mutual subscription doesn't need to timeout
        // The client machinery would kill both if anything happens
    }
    else
    {
        // We can only change the timeout spec if we're a responder
        if (aLivenessTimeoutSec <= kMaxTimeoutSec)
        {
            mLivenessTimeoutMsec = aLivenessTimeoutSec * 1000;
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }

    // make sure the changes in Binding in event callback
    // is reflected onto the active EC before we send out the first notify request

    err = mBinding->AdjustResponseTimeout(mEC);
    SuccessOrExit(err);

    // walk through the path list, prime the client
    MoveToState(kState_Subscribing);

    // Note that the call to NotificationEngine::Run could actually cause this particular handler to be aborted
    SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();

exit:
    WeaveLogFunctError(err);

    if (WEAVE_NO_ERROR != err)
    {
        AbortSubscription();
    }
    return err;
}

void SubscriptionHandler::DefaultEventHandler(EventID aEvent, const InEventParam & aInParam, OutEventParam & aOutParam)
{
    IgnoreUnusedVariable(aInParam);
    IgnoreUnusedVariable(aOutParam);

    WeaveLogDetail(DataManagement, "%s event: %d", __func__, aEvent);

    switch (aEvent)
    {
    case kEvent_OnSubscribeRequestParsed:
        // reject, don't care about current state should be in kState_Subscribing
        (void) aInParam.mSubscribeRequestParsed.mHandler->EndSubscription(nl::Weave::Profiles::kWeaveProfile_Common,
                                                                          nl::Weave::Profiles::Common::kStatus_UnsupportedMessage);
        break;

    default:
        // ignore
        break;
    }
}

WEAVE_ERROR SubscriptionHandler::EndSubscription(const uint32_t aReasonProfileId, const uint16_t aReasonStatusCode)
{
    WEAVE_ERROR err   = WEAVE_NO_ERROR;
    bool abortOnError = true;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    switch (mCurrentState)
    {
    case kState_Subscribing_Evaluating:
    case kState_Subscribing:
        // reject the request with status report without any callback to app layer
        {
            uint8_t * p;
            uint8_t statusReportLen = 6;
            PacketBuffer * msgBuf   = PacketBuffer::NewWithAvailableSize(statusReportLen);
            VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

            p = msgBuf->Start();
            nl::Weave::Encoding::LittleEndian::Write32(p, aReasonProfileId);
            nl::Weave::Encoding::LittleEndian::Write16(p, aReasonStatusCode);
            msgBuf->SetDataLength(statusReportLen);

            err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kMsgType_StatusReport,
                                   msgBuf, mEC->HasPeerRequestedAck() ? nl::Weave::ExchangeContext::kSendFlag_RequestAck : 0);
            msgBuf = NULL;
            SuccessOrExit(err);

            // Close our EC, but not abort it since the status report will still be in flight.
            FlushExistingExchangeContext(false);

            // This will clean-up the handler and reset it to the right state, but leave the EC untouched since we've already closed
            // it out above
            AbortSubscription();

            ExitNow();
        }
        break;

    case kState_Subscribing_Notifying:
    case kState_Subscribing_Responding:
    case kState_SubscriptionEstablished_Notifying:
    case kState_Canceling:
        // message in flight - for now, we're not going to take any action in these cases since we haven't
        // spent enough time plumbing the depths here.
        abortOnError = false;
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        break;

    case kState_SubscriptionEstablished_Idle:
#if WDM_ENABLE_SUBSCRIPTION_CANCEL
        err = Cancel();
#else  // WDM_ENABLE_SUBSCRIPTION_CANCEL
        AbortSubscription();
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL
        break;

    default:
        // nothing we can do
        abortOnError = false;
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
        break;
    }

exit:
    WeaveLogFunctError(err);

    if (abortOnError && (err != WEAVE_NO_ERROR))
    {
        AbortSubscription();
    }

    return err;
}

WEAVE_ERROR SubscriptionHandler::ParsePathVersionEventLists(SubscribeRequest::Parser & aRequest,
                                                            uint32_t & aRejectReasonProfileId,
                                                            uint16_t & aRejectReasonStatusCode)
{
    WEAVE_ERROR err                   = WEAVE_NO_ERROR;
    bool parsingCompletedSuccessfully = false;
    bool IsVersionListPresent         = false;
    nl::Weave::TLV::TLVReader pathListIterator;
    PathList::Parser pathList;
    VersionList::Parser versionList;

    aRejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
    aRejectReasonStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;

    err = aRequest.GetPathList(&pathList);
    SuccessOrExit(err);

    pathList.GetReader(&pathListIterator);

    err = aRequest.GetVersionList(&versionList);
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
    }

    while (WEAVE_NO_ERROR == (err = pathListIterator.Next()))
    {
        TraitDataHandle traitDataHandle;
        PropertyPathHandle propertyPathHandle;
        TraitDataSource * dataSource;
        TraitInstanceInfo * traitInstance = NULL;
        nl::Weave::TLV::TLVReader pathReader;
        SchemaVersionRange requestedSchemaVersionRange, computedVersionIntersection;
        SchemaVersion computedForwardRequestedVersion;

        if (IsVersionListPresent)
        {
            // verify that we still have something to read in version list
            if (WEAVE_NO_ERROR != (err = versionList.Next()))
            {
                // Failed at reading from version list, reject with the default reason of bad request
                ExitNow();
            }
        }

        // Make a copy here
        pathReader.Init(pathListIterator);

        err = SubscriptionEngine::GetInstance()->mPublisherCatalog->AddressToHandle(pathReader, traitDataHandle,
                                                                                    requestedSchemaVersionRange);

        if (WEAVE_ERROR_INVALID_PROFILE_ID == err)
        {
            // note we can safely 'continue' directly from here because 'pathReader' is a copy of 'reader'
            // so the actual reader is advancing irrespective of what happened during parsing

            WeaveLogDetail(DataManagement, "Unknown profile ID in the subscribe request, ignore.");
            continue;
        }

        SuccessOrExit(err);

        if (SubscriptionEngine::GetInstance()->mPublisherCatalog->Locate(traitDataHandle, &dataSource) != WEAVE_NO_ERROR)
        {
            // Ideally, this code will not be reached as Locate() should find the entry in the catalog.
            // Otherwise, the earlier AddressToHandle() call would have continued.
            // However, keeping this check here for consistency and code safety
            continue;
        }

        if (dataSource->GetSchemaEngine()->GetVersionIntersection(requestedSchemaVersionRange, computedVersionIntersection))
        {
            computedForwardRequestedVersion =
                dataSource->GetSchemaEngine()->GetHighestForwardVersion(computedVersionIntersection.mMaxVersion);
        }
        else
        {
            WeaveLogDetail(DataManagement, "Mismatch in requested version on handle %u (requested: %u, %u, provided: %u %u)", traitDataHandle,
                           requestedSchemaVersionRange.mMaxVersion, requestedSchemaVersionRange.mMinVersion, dataSource->GetSchemaEngine()->GetLowestCompatibleVersion(1), dataSource->GetSchemaEngine()->GetHighestForwardVersion(1));
            continue;
        }

        err = dataSource->GetSchemaEngine()->MapPathToHandle(pathReader, propertyPathHandle);
        SuccessOrExit(err);

        if (propertyPathHandle != kRootPropertyPathHandle)
        {
            WeaveLogError(DataManagement, "Device only supports subscriptions to root!\n");
            ExitNow();
        }

        for (size_t i = 0; i < mNumTraitInstances; ++i)
        {
            if (mTraitInstanceList[i].mTraitDataHandle == traitDataHandle)
            {
                // we found a prior trait instance which has the same root (trait instance)
                traitInstance = &mTraitInstanceList[i];
                break;
            }
        }

        if (!traitInstance)
        {
            // allocate a new trait instance
            WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_TraitInstanceNew, ExitNow(err = WEAVE_ERROR_NO_MEMORY));

            if (SubscriptionEngine::GetInstance()->mNumTraitInfosInPool < SubscriptionEngine::kMaxNumPathGroups)
            {
                traitInstance =
                    SubscriptionEngine::GetInstance()->mTraitInfoPool + SubscriptionEngine::GetInstance()->mNumTraitInfosInPool;
                ++mNumTraitInstances;
                ++(SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);
                SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDM_NumTraits);

                traitInstance->Init();
            }
            else
            {
                // we run out of trait instances, abort
                // Note it might help the client understanding what's going on with an error status like
                // "out of memory" or "internal error", but it's pretty common that a server doesn't disclose
                // too much internal status to clients
                SuccessOrExit(err = WEAVE_ERROR_NO_MEMORY);
            }
        }

        traitInstance->mTraitDataHandle  = traitDataHandle;
        traitInstance->mRequestedVersion = computedForwardRequestedVersion;

        if (NULL == mTraitInstanceList)
        {
            // this the first trait instance for this subscription
            // mNumTraitInstanceList has already be incremented
            mTraitInstanceList = traitInstance;
        }

        if (!IsVersionListPresent)
        {
            // no existing version
            WeaveLogDetail(DataManagement, "Handler[%u] Syncing is requested for trait[%u].path[%u]",
                           SubscriptionEngine::GetInstance()->GetHandlerId(this), traitDataHandle, propertyPathHandle);

            traitInstance->SetDirty();
        }
        else
        {
            if (versionList.IsNull())
            {
                // no existing version

                WeaveLogDetail(DataManagement, "Handler[%u] Syncing is requested for trait[%u].path[%u]",
                               SubscriptionEngine::GetInstance()->GetHandlerId(this), traitDataHandle, propertyPathHandle);

                traitInstance->SetDirty();
            }
            else
            {
                uint64_t existingVersion;
                uint64_t datasourceVersion = dataSource->GetVersion();
                err                        = versionList.GetVersion(&existingVersion);
                SuccessOrExit(err);

                if (existingVersion != datasourceVersion)
                {
                    WeaveLogDetail(DataManagement, "Handler[%u] Syncing is necessary for trait[%u].path[%u]",
                                   SubscriptionEngine::GetInstance()->GetHandlerId(this), traitDataHandle, propertyPathHandle);

                    WeaveLogIfFalse(existingVersion < datasourceVersion);
                    traitInstance->SetDirty();
                }
                else
                {
                    WeaveLogDetail(DataManagement, "Handler[%u] Syncing is NOT necessary for trait[%u].path[%u]",
                                   SubscriptionEngine::GetInstance()->GetHandlerId(this), traitDataHandle, propertyPathHandle);
                }
            }

#if WEAVE_CONFIG_WDM_NEXT_SUBSCRIPTION_HANDLER_DEBUG
            // we're going to log some warning if
            // 1. this is not the first encounter with this trait instance
            // 2. we're changing the decision made for this trait instance
            // this warning implies the versions for diff paths provided in subscribe request are inconsistent
            if ((!isFirstPathInGroup) && (previousSyncDecision != pathGroup->mFurtherSyncNeededForThisSubscription))
            {
                WeaveLogDetail(DataManagement, "Handler[%u] Inconsistent version information for trait[%u], reject",
                               SubscriptionEngine::GetInstance()->GetHandlerId(this), root);

                RejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_WDM;
                RejectReasonStatusCode = nl::Weave::Profiles::DataManagement::kStatus_VersionMismatch;
                ExitNow();
            }
#endif // WEAVE_CONFIG_WDM_NEXT_SUBSCRIPTION_HANDLER_DEBUG
        }
    }

    WeaveLogDetail(DataManagement, "Number allocated of trait info instances: %u",
                   SubscriptionEngine::GetInstance()->mNumTraitInfosInPool);

    // Check if we still have anything in version list after we run out of paths
    if ((WEAVE_END_OF_TLV == err) && IsVersionListPresent)
    {
        // everything is fine, but we just ran out of paths
        if (IsVersionListPresent && (WEAVE_END_OF_TLV != (err = versionList.Next())))
        {
            // how come the version list has not been exhausted?
            WeaveLogDetail(DataManagement, "Path has been exhausted unexpectedly, rejecting");
            ExitNow();
        }
    }

    // Setting it to false is not absolutely necessary, as mSubscribeToAllEvents is reset to false in
    // InitAsFree, and Abort, and again in GetSubscribeToAllEvents
    mSubscribeToAllEvents = false;
    err                   = aRequest.GetSubscribeToAllEvents(&mSubscribeToAllEvents);
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }
    VerifyOrExit(WEAVE_NO_ERROR == err, /* no-op */);

    memset(mSelfVendedEvents, 0, sizeof(mSelfVendedEvents));

    if (mSubscribeToAllEvents)
    {
        EventList::Parser eventList;
        uint64_t localNodeId = mEC->ExchangeMgr->FabricState->LocalNodeId;
        uint64_t sourceId, importance, eventId;
        err = aRequest.GetLastObservedEventIdList(&eventList);

        if (WEAVE_NO_ERROR == err)
        {
            while ((err = eventList.Next()) == WEAVE_NO_ERROR)
            {
                nl::Weave::TLV::TLVReader eventReader;
                Event::Parser event;

                eventList.GetReader(&eventReader);

                err = event.Init(eventReader);
                SuccessOrExit(err);

                err = event.GetSourceId(&sourceId);
                SuccessOrExit(err);

                err = event.GetImportance(&importance);
                SuccessOrExit(err);

                err = event.GetEventId(&eventId);
                SuccessOrExit(err);

                // At the moment, we don't support event aggregation in subscription

                if ((sourceId == localNodeId) &&
                    (static_cast<uint32_t>(importance) >= static_cast<uint32_t>(kImportanceType_First)) &&
                    (static_cast<uint32_t>(importance) <= static_cast<uint32_t>(kImportanceType_Last)))
                {
                    // We add one to the observed event ID because
                    // mSelfVendedEvents should point to the next event ID that
                    // we publish. Otherwise, we would publish an event that
                    // the subscriber already received.
                    mSelfVendedEvents[static_cast<uint32_t>(importance) - static_cast<uint32_t>(kImportanceType_First)] =
                        static_cast<event_id_t>(eventId + 1);
                }
                else
                {
                    ExitNow();
                }
            }
            if (WEAVE_END_OF_TLV == err)
            {
                err = WEAVE_NO_ERROR;
            }
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

    // Great! We've successfully processed path, version, and event lists
    parsingCompletedSuccessfully = true;

exit:
    WeaveLogFunctError(err);

    if (!parsingCompletedSuccessfully)
    {
        if (WEAVE_NO_ERROR == err)
        {
            err = WEAVE_ERROR_BAD_REQUEST;
        }
    }

    return err;
}

WEAVE_ERROR SubscriptionHandler::ParseSubscriptionId(SubscribeRequest::Parser & aRequest, uint32_t & aRejectReasonProfileId,
                                                     uint16_t & aRejectReasonStatusCode, const uint64_t aRandomNumber)
{
    WEAVE_ERROR err                   = WEAVE_NO_ERROR;
    bool parsingCompletedSuccessfully = false;

    // From now on, the main reason for rejection is invalid subscription ID
    aRejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_WDM;
    aRejectReasonStatusCode = nl::Weave::Profiles::DataManagement::kStatus_InvalidSubscriptionID;

    err = aRequest.GetSubscriptionID(&mSubscriptionId);
    if (WEAVE_END_OF_TLV == err)
    {
        // good, this is a normal request
        // use the generated subscription id

        mSubscriptionId = aRandomNumber;
        err             = WEAVE_NO_ERROR;
    }
    else if (WEAVE_NO_ERROR == err)
    {
#if WDM_ENABLE_SUBSCRIPTION_CLIENT

        // Note FindHandler is not to find this particular handler, for we're still in evaluating state
        if (NULL == SubscriptionEngine::GetInstance()->FindHandler(mPeerNodeId, mSubscriptionId))
        {
            SubscriptionClient * pClient;
            pClient = SubscriptionEngine::GetInstance()->FindClient(mPeerNodeId, mSubscriptionId);
            if (NULL != pClient)
            {
                // this is the second half of a mutual subscription (and we did find the first half)
                // continue using id in the request
                mIsInitiator = true;

                // as a result, this is also an indication of subscription activity on the client
                // regardless of whether the subscription is accepted or not.
                pClient->IndicateActivity();
            }
            else
            {
                // This incoming request carries some subscription ID, which implies there should be an existing client
                // associated with the remote node already. Reject it if we couldn't find one
                WeaveLogError(DataManagement, "No matching subscription found for incoming mutual subscription");
                ExitNow();
            }
        }
        else
        {
            // This incoming request carries some subscription ID, but we already have an existing
            // subscription with the same client and the same ID. It's not obvious which one we should keep,
            // and we choose to keep the existing one here.
            WeaveLogError(DataManagement, "Mutual subscription with duplicated ID");
            ExitNow();
        }

#else  //#if WDM_ENABLE_SUBSCRIPTION_CLIENT
        WeaveLogError(DataManagement, "Mutual subscription is not supported");
        RejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
        RejectReasonStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;
        ExitNow();
#endif //#if WDM_ENABLE_SUBSCRIPTION_CLIENT
    }
    else
    {
        aRejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
        aRejectReasonStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;
        ExitNow();
    }

    parsingCompletedSuccessfully = true;

exit:
    WeaveLogFunctError(err);

    if (!parsingCompletedSuccessfully)
    {
        if (WEAVE_NO_ERROR == err)
        {
            err = WEAVE_ERROR_BAD_REQUEST;
        }
    }

    return err;
}

void SubscriptionHandler::InitWithIncomingRequest(Binding * const aBinding, const uint64_t aRandomNumber,
                                                  nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                                  const nl::Weave::WeaveMessageInfo * aMsgInfo, PacketBuffer * aPayload)
{
    WEAVE_ERROR err                 = WEAVE_NO_ERROR;
    uint32_t RejectReasonProfileId  = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t RejectReasonStatusCode = nl::Weave::Profiles::Common::kStatus_BadRequest;
    nl::Weave::TLV::TLVReader reader;
    SubscribeRequest::Parser request;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    WeaveLogIfFalse(0 == mRefCount);

    _AddRef();

    aBinding->AddRef();
    mBinding = aBinding;

    mPeerNodeId     = aMsgInfo->SourceNodeId;
    mBytesOffloaded = 0;

    mEC = aEC;
    // The ownership has been transferred to this subscription
    aEC = NULL;

    // Add reference when we enter this initial state
    // This is needed because the app layer doesn't automatically get hold of this instance,
    // but we need this instance to be around until we clear the protocol state machine (by entering aborted)
    _AddRef();
    MoveToState(kState_Subscribing_Evaluating);

    InitExchangeContext();

    // First stage: Initial parsing
    // Convert all path lists to TargetHandles and PathHandles
    // Note that notification rejection is not supported, so version is only validated but not tracked
    // This job doesn't require application level intervention
    // If any conversion fails (which means schema error), just reject this request

    reader.Init(aPayload);

    err = reader.Next();
    SuccessOrExit(err);

    err = request.Init(reader);
    SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
    err = request.CheckSchemaValidity();
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

    // Second stage: Subscription ID
    err = ParseSubscriptionId(request, RejectReasonProfileId, RejectReasonStatusCode, aRandomNumber);
    SuccessOrExit(err);

    // Third stage: path, version, and events
    err = ParsePathVersionEventLists(request, RejectReasonProfileId, RejectReasonStatusCode);
    SuccessOrExit(err);

    // Final stage: app callback
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.mSubscribeRequestParsed.mEC                   = mEC;
        inParam.mSubscribeRequestParsed.mPktInfo              = aPktInfo;
        inParam.mSubscribeRequestParsed.mMsgInfo              = aMsgInfo;
        inParam.mSubscribeRequestParsed.mTraitInstanceList    = mTraitInstanceList;
        inParam.mSubscribeRequestParsed.mNumTraitInstances    = mNumTraitInstances;
        inParam.mSubscribeRequestParsed.mSubscribeToAllEvents = mSubscribeToAllEvents;

        memcpy(inParam.mSubscribeRequestParsed.mNextVendedEvents, mSelfVendedEvents, sizeof(inParam.mSubscribeRequestParsed.mNextVendedEvents));

        err = request.GetSubscribeTimeoutMin(&inParam.mSubscribeRequestParsed.mTimeoutSecMin);
        if (WEAVE_END_OF_TLV == err)
        {
            err                                            = WEAVE_NO_ERROR;
            inParam.mSubscribeRequestParsed.mTimeoutSecMin = kNoTimeout;
        }
        else if (WEAVE_NO_ERROR != err)
        {
            ExitNow();
        }

        err = request.GetSubscribeTimeoutMax(&inParam.mSubscribeRequestParsed.mTimeoutSecMax);
        if (WEAVE_END_OF_TLV == err)
        {
            err                                            = WEAVE_NO_ERROR;
            inParam.mSubscribeRequestParsed.mTimeoutSecMax = kNoTimeout;
        }
        else if (WEAVE_NO_ERROR != err)
        {
            ExitNow();
        }

        // Note that err must be WEAVE_NO_ERROR now, otherwise we should just reject and not call to app layer
        SuccessOrExit(err);

        inParam.mSubscribeRequestParsed.mHandler               = this;
        inParam.mSubscribeRequestParsed.mIsSubscriptionIdValid = mIsInitiator;
        inParam.mSubscribeRequestParsed.mSubscriptionId        = mSubscriptionId;

        // From now on, the app layer has to make the explicit call to accept or reject,
        // and then Free this handler later
        mEventCallback(mAppState, kEvent_OnSubscribeRequestParsed, inParam, outParam);
        // Note that either Abort or EndSubscription could be called.
        // There is no need to explicitly reject the request in this function.
    }

exit:
    WeaveLogFunctError(err);

    // Release the pbuf first, guaranteed to be non-NULL
    PacketBuffer::Free(aPayload);

    if (WEAVE_NO_ERROR != err)
    {
        // reject the request if we encountered any error
        (void) EndSubscription(RejectReasonProfileId, RejectReasonStatusCode);
    }

    _Release();
}

WEAVE_ERROR SubscriptionHandler::SendNotificationRequest(PacketBuffer * aMsgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    WeaveLogIfFalse((kState_Subscribing == mCurrentState) || (kState_SubscriptionEstablished_Idle == mCurrentState));

    // Make sure we're not freed by accident.
    _AddRef();

    // Create new exchange context when we're idle (otherwise we must be using the existing EC)
    if (kState_SubscriptionEstablished_Idle == mCurrentState)
    {
        err = ReplaceExchangeContext();
        SuccessOrExit(err);
    }

    // Note we're sending back a message using an EC initiated by the client
    err     = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_NotificationRequest, aMsgBuf,
                           nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
    aMsgBuf = NULL;
    SuccessOrExit(err);

    mCurrentState = (mCurrentState == kState_Subscribing) ? kState_Subscribing_Notifying : kState_SubscriptionEstablished_Notifying;

exit:
    WeaveLogFunctError(err);

    if (NULL != aMsgBuf)
    {
        PacketBuffer::Free(aMsgBuf);
        aMsgBuf = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        HandleSubscriptionTerminated(err, NULL);
    }

    _Release();

    return err;
}

void SubscriptionHandler::OnNotifyProcessingComplete(const bool aPossibleLossOfEvent, const LastVendedEvent aLastVendedEventList[],
                                                     const size_t aLastVendedEventListSize)
{
    if (mCurrentState == kState_Subscribing)
    {
        SendSubscribeResponse(aPossibleLossOfEvent, aLastVendedEventList, aLastVendedEventListSize);
    }
}

WEAVE_ERROR SubscriptionHandler::SendSubscribeResponse(const bool aPossibleLossOfEvent,
                                                       const LastVendedEvent aLastVendedEventList[],
                                                       const size_t aLastVendedEventListSize)
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    nl::Weave::TLV::TLVWriter writer;
    SubscribeResponse::Builder response;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident.
    _AddRef();

    msgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(msgBuf);

    response.Init(&writer);
    response.SubscriptionID(mSubscriptionId);

    if (!mIsInitiator)
    {
        // If we're the initiator in a mutual subscription,
        // the subscribe response must not carry a timeout spec
        if (kNoTimeout != mLivenessTimeoutMsec)
        {
            response.SubscribeTimeout(mLivenessTimeoutMsec / 1000);
        }
    }

    if (aPossibleLossOfEvent)
    {
        response.PossibleLossOfEvents(aPossibleLossOfEvent);
    }

    if (aLastVendedEventListSize > 0)
    {
        EventList::Builder & eventList = response.CreateLastVendedEventIdListBuilder();

        for (size_t n = 0; n < aLastVendedEventListSize; ++n)
        {
            Event::Builder & event = eventList.CreateEventBuilder();
            event.SourceId(aLastVendedEventList[n].mSourceId)
                .Importance(aLastVendedEventList[n].mImportance)
                .EventId(aLastVendedEventList[n].mEventId)
                .EndOfEvent();
            SuccessOrExit(err = event.GetError());
        }
        eventList.EndOfEventList();
        SuccessOrExit(err = eventList.GetError());
    }

    response.EndOfResponse();

    err = response.GetError();
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    // Note we're sending back a message using an EC initiated by the client
    err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeResponse, msgBuf,
                           nl::Weave::ExchangeContext::kSendFlag_RequestAck);
    msgBuf = NULL;
    SuccessOrExit(err);

    // Wait for Ack to move to alive state
    MoveToState(SubscriptionHandler::kState_Subscribing_Responding);

exit:
    WeaveLogFunctError(err);

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        HandleSubscriptionTerminated(err, NULL);
    }

    _Release();

    return err;
}

Binding * SubscriptionHandler::GetBinding(void) const
{
    return mBinding;
}

uint64_t SubscriptionHandler::GetPeerNodeId(void) const
{
    return mPeerNodeId;
}

void SubscriptionHandler::InitExchangeContext()
{
    mEC->AppState          = this;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError       = OnSendError;
    mEC->OnAckRcvd         = OnAckReceived;
    mEC->OnMessageReceived = OnMessageReceivedFromLocallyHeldExchange;
}

WEAVE_ERROR SubscriptionHandler::ReplaceExchangeContext()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident.
    _AddRef();

    FlushExistingExchangeContext();

    err = mBinding->NewExchangeContext(mEC);
    SuccessOrExit(err);

    InitExchangeContext();

    inParam.mExchangeStart.mEC      = mEC;
    inParam.mExchangeStart.mHandler = this;
    mEventCallback(mAppState, kEvent_OnExchangeStart, inParam, outParam);

exit:
    WeaveLogFunctError(err);

    _Release();

    return err;
}

void SubscriptionHandler::FlushExistingExchangeContext(const bool aAbortNow)
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
WEAVE_ERROR SubscriptionHandler::Cancel()
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * msgBuf = NULL;
    bool cancel           = false;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident.
    _AddRef();

    switch (mCurrentState)
    {
    case kState_SubscriptionEstablished_Notifying:
        // abort whatever we're doing (notification request)
        FlushExistingExchangeContext(true);
        cancel = true;

        break;

    case kState_SubscriptionEstablished_Idle:
        // send a cancel request
        cancel = true;
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    if (cancel)
    {
        nl::Weave::TLV::TLVWriter writer;
        SubscribeCancelRequest::Builder request;

        msgBuf = PacketBuffer::NewWithAvailableSize(request.kBaseMessageSubscribeId_PayloadLen);
        VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);
        writer.Init(msgBuf);

        err = ReplaceExchangeContext();
        SuccessOrExit(err);

        request.Init(&writer);

        request.SubscriptionID(mSubscriptionId).EndOfRequest();

        err = request.GetError();
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        // NOTE: State could be changed if there is a sync error callback from message layer
        err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_SubscribeCancelRequest, msgBuf,
                               nl::Weave::ExchangeContext::kSendFlag_ExpectResponse);
        msgBuf = NULL;
        SuccessOrExit(err);

        MoveToState(SubscriptionHandler::kState_Canceling);
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
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

WEAVE_ERROR SubscriptionHandler::GetSubscriptionId(uint64_t * const apSubscriptionId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    *apSubscriptionId = 0;

    if ((mCurrentState >= kState_SubscriptionInfoValid_Begin) && (mCurrentState <= kState_SubscriptionInfoValid_End))
    {
        *apSubscriptionId = mSubscriptionId;
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void SubscriptionHandler::_AddRef()
{
    WeaveLogIfFalse(mRefCount < INT8_MAX);

    ++mRefCount;
}

void SubscriptionHandler::_Release()
{
    WeaveLogIfFalse(mRefCount > 0);

    --mRefCount;

    if (0 == mRefCount)
    {
        SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDM_NumSubscriptionHandlers);
        AbortSubscription();
    }
}

void SubscriptionHandler::HandleSubscriptionTerminated(WEAVE_ERROR aReason, StatusReporting::StatusReport * aStatusReportPtr)
{
    void * const appState      = mAppState;
    EventCallback callbackFunc = mEventCallback;
    uint64_t peerNodeId;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    _AddRef();

    // Retain a copy of the NodeId before AbortSubscription wipes it. We'll then stuff it back into mNodeId
    // momentarily so that the application can still observe that field on the ensuing up-call.
    // After the up-call returns, we'll nuke it back to 0.
    //
    // NOTE: This is a quick fix that has the least impact to the behaviors in this file. There's a lot that
    // leaves to be desired around AddRef/_Release and its interactions with AbortSubscription. As part of cleaning
    // that up, we'll find a better approach for retaining peer node ID through the up-call (see WEAV-2367).
    peerNodeId = mPeerNodeId;

    // Flush most internal states, except for mRefCount and mCurrentState
    // move to kState_Aborted
    AbortSubscription();

    mPeerNodeId = peerNodeId;

    // Make app layer callback in kState_Aborted
    // Note that mRefCount should still be more than 0 when we make this callback.
    // Since this function does not call _AddRef, it is possible if the app
    // calls Free() that we'll exit the app callback in a free state.
    // In that case, we won't have any cleanup to do.
    // On the other hand, if the caller of this function has AddRef'ed,
    // the state after exiting the app callback could be Aborted.
    // In that case, we still won't have cleanup, since everything will
    // be cleaned up upon _Release by the caller of this func.
    if (NULL != callbackFunc)
    {
        InEventParam inParam;
        OutEventParam outParam;

        inParam.Clear();
        outParam.Clear();

        inParam.mSubscriptionTerminated.mReason  = aReason;
        inParam.mSubscriptionTerminated.mHandler = this;
        if (aStatusReportPtr != NULL)
        {
            inParam.mSubscriptionTerminated.mStatusProfileId   = aStatusReportPtr->mProfileId;
            inParam.mSubscriptionTerminated.mStatusCode        = aStatusReportPtr->mStatusCode;
            inParam.mSubscriptionTerminated.mAdditionalInfoPtr = &(aStatusReportPtr->mAdditionalInfo);
        }

        callbackFunc(appState, kEvent_OnSubscriptionTerminated, inParam, outParam);
    }
    else
    {
        WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d) app layer callback skipped",
                       SubscriptionEngine::GetInstance()->GetHandlerId(this), GetStateStr(), __func__, mRefCount);
    }

    mPeerNodeId = 0;

    _Release();
}

void SubscriptionHandler::AbortSubscription()
{
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    const bool nullReference = (0 == mRefCount);

    if (!nullReference)
    {
        // only do verbose logging if we're still referenced by someone
        WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                       GetStateStr(), __func__, mRefCount);

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
        uint64_t peerNodeId            = mPeerNodeId;
        uint64_t subscriptionId        = mSubscriptionId;
        bool notifyEngineCleanupNeeded = false;

        // If we were previously in a notifying state, we must tell NE so that it can do some clean-up.
        // However, we should wait till the subscription state has moved to aborting before attempting
        // to do so.
        if (IsNotifying())
        {
            notifyEngineCleanupNeeded = true;
        }

        MoveToState(kState_Aborting);

        if (notifyEngineCleanupNeeded)
        {
            const bool notifySuccessfullyDelivered = true;

            SubscriptionEngine::GetInstance()->GetNotificationEngine()->OnNotifyConfirm(this, !notifySuccessfullyDelivered);
        }

        mBinding->SetProtocolLayerCallback(NULL, NULL);

        mBinding->Release();
        mBinding = NULL;

        FlushExistingExchangeContext(true);
        mLivenessTimeoutMsec           = kNoTimeout;
        mPeerNodeId                    = 0;
        mSubscriptionId                = 0;
        mIsInitiator                   = false;
        mAppState                      = NULL;
        mEventCallback                 = NULL;
        mMaxNotificationSize           = 0;
        mSubscribeToAllEvents          = false;
        mCurProcessingTraitInstanceIdx = 0;
        mCurrentImportance             = kImportanceType_Invalid;
        (void) RefreshTimer();

        // release all trait instances back to the shared pool
        SubscriptionEngine::GetInstance()->ReclaimTraitInfo(this);

        mTraitInstanceList = NULL;
        mNumTraitInstances = 0;

        memset(mSelfVendedEvents, 0, sizeof(mSelfVendedEvents));
        memset(mLastScheduledEventId, 0, sizeof(mLastScheduledEventId));

        MoveToState(kState_Aborted);

        // decrease the ref we added when we first started the protocol state machine
        // now we've cleared this state machine (nothing for the protocol anymore),
        // this instance doesn't need to be around any more
        _Release();

#if WDM_ENABLE_SUBSCRIPTION_CLIENT
        (void) SubscriptionEngine::GetInstance()->UpdateClientLiveness(peerNodeId, subscriptionId, true);
#endif // WDM_ENABLE_SUBSCRIPTION_CLIENT
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
        // Note that this call could lower ref count to 0 and cause this instance to be freed
        // this is because we have an extra _Release when we first move into Aborting state
        _Release();
    }
}

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
void SubscriptionHandler::CancelRequestHandler(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                               const nl::Weave::WeaveMessageInfo * aMsgInfo, PacketBuffer * aPayload)
{
    WEAVE_ERROR err         = WEAVE_NO_ERROR;
    uint8_t statusReportLen = 6;
    PacketBuffer * msgBuf   = PacketBuffer::NewWithAvailableSize(statusReportLen);
    uint8_t * p;
    bool canceled          = true;
    uint32_t statusProfile = nl::Weave::Profiles::kWeaveProfile_Common;
    uint16_t statusCode    = nl::Weave::Profiles::Common::kStatus_Success;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    // Make sure we're not freed by accident.
    _AddRef();

    mBinding->AdjustResponseTimeout(aEC);

    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    // Verify the cancel request is truly from the client.  If not, reject the request with
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

    if (canceled)
    {
        HandleSubscriptionTerminated(WEAVE_ERROR_TRANSACTION_CANCELED, NULL);
    }

    _Release();
}
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

void SubscriptionHandler::OnTimerCallback(System::Layer * aSystemLayer, void * aAppState, System::Error)
{
    SubscriptionHandler * const pHandler = reinterpret_cast<SubscriptionHandler *>(aAppState);

    pHandler->TimerEventHandler();
}

WEAVE_ERROR SubscriptionHandler::RefreshTimer(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), __func__, mRefCount);

    // Cancel timer first
    SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->CancelTimer(OnTimerCallback, this);

    // Arm timer according to current state
    switch (mCurrentState)
    {
    case kState_SubscriptionEstablished_Idle:
    case kState_SubscriptionEstablished_Notifying:
        if (mIsInitiator)
        {
            WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d) Ignored for handler on initiator",
                           SubscriptionEngine::GetInstance()->GetHandlerId(this), GetStateStr(), __func__, mRefCount);
        }
        else if (kNoTimeout != mLivenessTimeoutMsec)
        {
            WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d) Set timer for liveness confirmation to %" PRIu32 " msec",
                           SubscriptionEngine::GetInstance()->GetHandlerId(this), GetStateStr(), __func__, mRefCount,
                           mLivenessTimeoutMsec);

            err = SubscriptionEngine::GetInstance()->GetExchangeManager()->MessageLayer->SystemLayer->StartTimer(
                mLivenessTimeoutMsec, OnTimerCallback, this);

            VerifyOrExit(WEAVE_SYSTEM_NO_ERROR == err, /* no-op */);
        }
        break;
    case kState_Aborting:
        // Do nothing
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void SubscriptionHandler::TimerEventHandler(void)
{
    bool ShouldCleanUp  = false;
    bool skipTimerCheck = false;

    if (0 == mRefCount)
    {
        skipTimerCheck = true;
        ExitNow();
    }

    // Make sure we're not freed by accident.
    _AddRef();

    if (kState_SubscriptionEstablished_Idle == mCurrentState)
    {
        ShouldCleanUp = true;

        WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d) Timeout",
                       SubscriptionEngine::GetInstance()->GetHandlerId(this), GetStateStr(), __func__, mRefCount);
    }
    else
    {
        WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d) Timer event fired at wrong state, ignore",
                       SubscriptionEngine::GetInstance()->GetHandlerId(this), GetStateStr(), __func__, mRefCount);
    }

exit:
    if (ShouldCleanUp)
    {
        HandleSubscriptionTerminated(WEAVE_ERROR_TIMEOUT, NULL);
    }

    if (!skipTimerCheck)
    {
        _Release();
    }
}

bool SubscriptionHandler::CheckEventUpToDate(LoggingManagement & inLogger)
{
    bool retval = true;

    if (inLogger.IsValid())
    {
        for (size_t i = 0; i < sizeof(mSelfVendedEvents) / sizeof(event_id_t); i++)
        {
            event_id_t eid = inLogger.GetLastEventID(static_cast<ImportanceType>(i + kImportanceType_First));
            if ((eid != 0) && (eid >= mSelfVendedEvents[i]))
            {
                retval = false;
                break;
            }
        }
    }

    return retval;
}

ImportanceType SubscriptionHandler::FindNextImportanceForTransfer(void)
{
    ImportanceType retval = kImportanceType_Invalid;

    for (size_t i = 0; i < sizeof(mSelfVendedEvents) / sizeof(event_id_t); i++)
    {
        if ((mLastScheduledEventId[i] != 0) && mSelfVendedEvents[i] <= mLastScheduledEventId[i])
        {
            retval = static_cast<ImportanceType>(i + kImportanceType_First);
            break;
        }
    }

    return retval;
}

WEAVE_ERROR SubscriptionHandler::SetEventLogEndpoint(LoggingManagement & inLogger)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(inLogger.IsValid(), err = WEAVE_ERROR_INCORRECT_STATE);

    err =
        inLogger.SetLoggingEndpoint(&(mLastScheduledEventId[0]), kImportanceType_Last - kImportanceType_First + 1, mBytesOffloaded);

exit:
    return err;
}

#if WEAVE_DETAIL_LOGGING
const char * SubscriptionHandler::GetStateStr() const
{
    switch (mCurrentState)
    {
    case kState_Free:
        return "FREE";

    case kState_Subscribing_Evaluating:
        return "EVAL";

    case kState_Subscribing:
        return "PRIME";

    case kState_Subscribing_Notifying:
        return "pNOTF";

    case kState_Subscribing_Responding:
        return "pRESP";

    case kState_SubscriptionEstablished_Idle:
        return "ALIVE";

    case kState_SubscriptionEstablished_Notifying:
        return "NOTIF";

    case kState_Canceling:
        return "CANCL";

    case kState_Aborting:
        return "ABTNG";

    case kState_Aborted:
        return "ABORT";
    }
    return "N/A";
}
#else  // WEAVE_DETAIL_LOGGING
const char * SubscriptionHandler::GetStateStr() const
{
    return "N/A";
}
#endif // WEAVE_DETAIL_LOGGING

void SubscriptionHandler::MoveToState(const HandlerState aTargetState)
{
    mCurrentState = aTargetState;
    WeaveLogDetail(DataManagement, "Handler[%u] Moving to [%5.5s] Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(this),
                   GetStateStr(), mRefCount);

#if WEAVE_DETAIL_LOGGING
    if (kState_Free == mCurrentState)
    {
        SubscriptionEngine::GetInstance()->LogSubscriptionFreed();
    }
#endif // #if WEAVE_DETAIL_LOGGING
}

void SubscriptionHandler::OnAckReceived(ExchangeContext * aEC, void * aMsgSpecificContext)
{
    WEAVE_ERROR err                      = WEAVE_NO_ERROR;
    SubscriptionHandler * const pHandler = reinterpret_cast<SubscriptionHandler *>(aEC->AppState);

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(pHandler),
                   pHandler->GetStateStr(), __func__, pHandler->mRefCount);

    // Make sure we're not freed by accident.
    pHandler->_AddRef();

    switch (pHandler->mCurrentState)
    {
    case kState_Subscribing_Responding:

        pHandler->FlushExistingExchangeContext();

        pHandler->MoveToState(SubscriptionHandler::kState_SubscriptionEstablished_Idle);

        err = pHandler->RefreshTimer();
        SuccessOrExit(err);

        {
            InEventParam inParam;
            OutEventParam outParam;
            inParam.mSubscriptionEstablished.mSubscriptionId = pHandler->mSubscriptionId;
            inParam.mSubscriptionEstablished.mHandler        = pHandler;

            // Note we could be aborted in this callback
            pHandler->mEventCallback(pHandler->mAppState, kEvent_OnSubscriptionEstablished, inParam, outParam);
        }

        // Run NE since things may have changed.
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
        break;

    default:
        break;
    }

exit:

    if (WEAVE_NO_ERROR != err)
    {
        pHandler->HandleSubscriptionTerminated(err, NULL);
    }

    pHandler->_Release();
}

void SubscriptionHandler::OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext)
{
    SubscriptionHandler * const pHandler = reinterpret_cast<SubscriptionHandler *>(aEC->AppState);

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(pHandler),
                   pHandler->GetStateStr(), __func__, pHandler->mRefCount);

    // Make sure we're not freed by accident.
    pHandler->_AddRef();
    pHandler->HandleSubscriptionTerminated(aErrorCode, NULL);
    pHandler->_Release();

    // Run it again to do more useful work.
    // Note that the call to NotificationEngine::Run could actually cause this particular handler to be aborted
    SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
}

void SubscriptionHandler::OnResponseTimeout(nl::Weave::ExchangeContext * aEC)
{
    SubscriptionHandler * const pHandler = reinterpret_cast<SubscriptionHandler *>(aEC->AppState);

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(pHandler),
                   pHandler->GetStateStr(), __func__, pHandler->mRefCount);

    // Make sure we're not freed by accident.
    pHandler->_AddRef();
    pHandler->HandleSubscriptionTerminated(WEAVE_ERROR_TIMEOUT, NULL);
    pHandler->_Release();
}

void SubscriptionHandler::OnMessageReceivedFromLocallyHeldExchange(nl::Weave::ExchangeContext * aEC,
                                                                   const nl::Inet::IPPacketInfo * aPktInfo,
                                                                   const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                                                   uint32_t aProfileId, uint8_t aMsgType, PacketBuffer * aPayload)
{
    // Status Report for Notification request during priming
    // Status Report for Notification request when subscription is alive
    // Status Report for Cancel request

    WEAVE_ERROR err                      = WEAVE_NO_ERROR;
    SubscriptionHandler * const pHandler = reinterpret_cast<SubscriptionHandler *>(aEC->AppState);

    bool RetainExchangeContext                 = false;
    bool isStatusReportValid                   = false;
    bool isNotificationRejectedForInvalidValue = false;
    nl::Weave::Profiles::StatusReporting::StatusReport status;

    WeaveLogDetail(DataManagement, "Handler[%u] [%5.5s] %s Ref(%d)", SubscriptionEngine::GetInstance()->GetHandlerId(pHandler),
                   pHandler->GetStateStr(), __func__, pHandler->mRefCount);

    // Make sure we're not freed by accident.
    pHandler->_AddRef();

    VerifyOrExit(aEC == pHandler->mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) &&
        (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        // Note that payload is not freed in this call to parse
        err = nl::Weave::Profiles::StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);
        isStatusReportValid = true;
        WeaveLogDetail(DataManagement, "Received Status Report 0x%" PRIX32 " : 0x%" PRIX16, status.mProfileId, status.mStatusCode);

        if ((nl::Weave::Profiles::kWeaveProfile_WDM == status.mProfileId) &&
            (kStatus_InvalidValueInNotification == status.mStatusCode))
        {
            isNotificationRejectedForInvalidValue = true;
        }
    }

    switch (pHandler->mCurrentState)
    {
    case kState_Subscribing_Notifying:
        // response for notification request during priming, don't close the exchange context, for more notification requests might
        // need to go through this same exchange context
        VerifyOrExit(isStatusReportValid, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        if (status.success())
        {
            // good
        }
        else if (isNotificationRejectedForInvalidValue)
        {
            // rejected for invalid value
            // we don't really support this, assume it's accepted and continue.
            WeaveLogDetail(DataManagement, "Notification rejected, ignore rejection");
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
        }

        // only retain the EC if we're good to continue processing
        RetainExchangeContext = true;

        // Only prompt the NotificationEngine if we received a status report indicating success. Otherwise, the subscription will
        // get torn down and as part of that clean-up, a similar invocation of OnNotifyConfirm will happen.
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->OnNotifyConfirm(pHandler, status.success());

        // kick back from kState_Subscribing_Notifying to kState_Subscribing and evaluate again
        pHandler->MoveToState(kState_Subscribing);

        // kick notification engine again
        // Note that the call to NotificationEngine::Run could actually cause this particular handler to be aborted
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
        break;

    case kState_SubscriptionEstablished_Notifying:
        // response for notification request while subscription is alive. close the exchange context
        VerifyOrExit(isStatusReportValid, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        if (status.success())
        {
            // good
        }
        else if (isNotificationRejectedForInvalidValue)
        {
            // rejected for invalid value
            // we don't really support this, assume it's accepted and continue.
            WeaveLogDetail(DataManagement, "Notification rejected, ignore rejection");
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_STATUS_REPORT_RECEIVED);
        }

        // don't call flush for us in the end
        RetainExchangeContext = true;

        // Only prompt the NotificationEngine if we received a status report indicating success. Otherwise, the subscription will
        // get torn down and as part of that clean-up, a similar invocation of OnNotifyConfirm will happen.
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->OnNotifyConfirm(pHandler, status.success());

        // flush right here, for NotificationEngine::Run() only flushes when a new notification is needed
        // make it clear that we do not need this EC anymore
        pHandler->FlushExistingExchangeContext();
        aEC = NULL;

        // kick back from kState_SubscriptionEstablished_Notifying to kState_SubscriptionEstablished_Idle and evaluate again
        pHandler->MoveToState(kState_SubscriptionEstablished_Idle);

        err = pHandler->RefreshTimer();
        SuccessOrExit(err);

#if WDM_ENABLE_SUBSCRIPTION_CLIENT
        (void) SubscriptionEngine::GetInstance()->UpdateClientLiveness(pHandler->mPeerNodeId, pHandler->mSubscriptionId);
#endif // WDM_ENABLE_SUBSCRIPTION_CLIENT

        // kick notification engine again
        // Note: we could have a new pHandler->mEC here, for sending out the next notification request.
        // when that happens, we're again in kState_SubscriptionEstablished_Notifying
        // Note that the call to NotificationEngine::Run could actually cause this particular handler to be aborted
        SubscriptionEngine::GetInstance()->GetNotificationEngine()->Run();
        break;

#if WDM_ENABLE_SUBSCRIPTION_CANCEL
    case kState_Canceling:
        // response for cancel request while subscription is alive. close the exchange context
        // call abort silently without callback to upper layer, for this subscription was canceled by the upper layer
        pHandler->AbortSubscription();
        break;
#endif // WDM_ENABLE_SUBSCRIPTION_CANCEL

    // We must not receive any callback in any of these states
    default:
        WeaveLogDetail(DataManagement, "Received message in some wrong state, ignore");
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

exit:
    WeaveLogFunctError(err);

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }

    if (!RetainExchangeContext)
    {
        pHandler->FlushExistingExchangeContext();
    }

    if (WEAVE_NO_ERROR != err)
    {
        pHandler->HandleSubscriptionTerminated(err, isStatusReportValid ? &status : NULL);
    }

    pHandler->_Release();
}

void SubscriptionHandler::SetMaxNotificationSize(const uint32_t aMaxSize)
{
    if (aMaxSize > UINT16_MAX)
        mMaxNotificationSize = 0;
    else
        mMaxNotificationSize = aMaxSize;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

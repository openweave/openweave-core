/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/TraitManager.h>
#include <Weave/Profiles/security/ApplicationKeysTraitDataSink.h>
#include <Weave/DeviceLayer/internal/DeviceIdentityTraitDataSource.h>

#include <new>

using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

using Schema::Weave::Trait::Auth::ApplicationKeysTrait::ApplicationKeysTraitDataSink;

namespace {

template <typename T>
class TraitCatalogImpl : public TraitCatalogBase<T>
{
public:
    enum
    {
        kMaxEntries = 20
    };

    TraitCatalogImpl();
    virtual ~TraitCatalogImpl();

    WEAVE_ERROR Add(const ResourceIdentifier & resId, const uint64_t & instanceId, PropertyPathHandle basePathHandle, T * traitInstance, TraitDataHandle & traitHandle);
    WEAVE_ERROR Remove(T * traitInstance);
    WEAVE_ERROR PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize, uint16_t & pathListLen);

    virtual WEAVE_ERROR AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter & aWriter,
                                        SchemaVersionRange & aSchemaVersionRange) const;
    virtual WEAVE_ERROR Locate(TraitDataHandle aHandle, T ** aTraitInstance) const;
    virtual WEAVE_ERROR Locate(T * aTraitInstance, TraitDataHandle & aHandle) const;
    virtual WEAVE_ERROR DispatchEvent(uint16_t aEvent, void * aContext) const;
    virtual void Iterate(IteratorCallback aCallback, void * aContext);
#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    virtual WEAVE_ERROR GetInstanceId(TraitDataHandle aHandle, uint64_t &aInstanceId) const;
    virtual WEAVE_ERROR GetResourceId(TraitDataHandle aHandle, ResourceIdentifier &aResourceId) const;
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

private:

    struct CatalogEntry
    {
        ResourceIdentifier ResourceId;
        uint64_t InstanceId;
        T * Item;
        PropertyPathHandle BasePathHandle;
        uint8_t EntryRevision;
    };

    CatalogEntry mEntries[kMaxEntries];

    static inline uint8_t HandleIndex(TraitDataHandle handle) { return (uint8_t)handle; }
    static inline uint8_t HandleRevision(TraitDataHandle handle) { return (uint8_t)(handle >> 8); }
    static inline TraitDataHandle MakeTraitDataHandle(uint8_t index, uint8_t revision) { return ((TraitDataHandle)revision) << 8 | index; }
};

typedef TraitCatalogImpl<TraitDataSink> TraitSinkCatalog;
typedef TraitCatalogImpl<TraitDataSource> TraitSourceCatalog;

enum
{
    // TODO: eliminate this when the Weave code defining service endpoints is cleaned-up
    kServiceEndpoint_Data_Management = 0x18B4300200000003ull
};

SubscriptionEngine WdmSubscriptionEngine;
TraitSinkCatalog SubscribedServiceTraits;
TraitSourceCatalog PublishedTraits;
ApplicationKeysTraitDataSink AppKeysTraitDataSink;
DeviceIdentityTraitDataSource DeviceIdTraitDataSource;

} // unnamed namespace

namespace nl {
namespace Weave {
namespace DeviceLayer {

WEAVE_ERROR TraitManager::SetServiceSubscriptionMode(ServiceSubscriptionMode val)
{
    mServiceSubMode = val;
    DriveServiceSubscriptionState(false);
    return WEAVE_NO_ERROR;
}

uint32_t TraitManager::GetServiceSubscribeConfirmIntervalMS(void) const
{
    // TODO: implement me
    return 0;
}

WEAVE_ERROR TraitManager::SetServiceSubscribeConfirmIntervalMS(uint32_t val) const
{
    // TODO: implement me
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

WEAVE_ERROR TraitManager::SubscribeServiceTrait(const ResourceIdentifier & resId, const uint64_t & instanceId,
        PropertyPathHandle basePathHandle, TraitDataSink * dataSink)
{
    TraitDataHandle traitHandle;
    return SubscribedServiceTraits.Add(resId, instanceId, basePathHandle, dataSink, traitHandle);
}

WEAVE_ERROR TraitManager::UnsubscribeServiceTrait(TraitDataSink * dataSink)
{
    return SubscribedServiceTraits.Remove(dataSink);
}

WEAVE_ERROR TraitManager::PublishTrait(const uint64_t & instanceId, TraitDataSource * dataSource)
{
    ResourceIdentifier selfResId(ResourceIdentifier::RESOURCE_TYPE_RESERVED, ResourceIdentifier::SELF_NODE_ID);
    TraitDataHandle traitHandle;
    return PublishedTraits.Add(selfResId, instanceId, kRootPropertyPathHandle, dataSource, traitHandle);
}

WEAVE_ERROR TraitManager::PublishTrait(const ResourceIdentifier & resId, const uint64_t & instanceId, TraitDataSource * dataSource)
{
    TraitDataHandle traitHandle;
    return PublishedTraits.Add(resId, instanceId, kRootPropertyPathHandle, dataSource, traitHandle);
}

WEAVE_ERROR TraitManager::UnpublishTrait(TraitDataSource * dataSource)
{
    return PublishedTraits.Remove(dataSource);
}

WEAVE_ERROR TraitManager::Init(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Binding * serviceBinding = NULL;

    new (&WdmSubscriptionEngine) SubscriptionEngine();
    new (&SubscribedServiceTraits) TraitSinkCatalog();
    new (&PublishedTraits) TraitSourceCatalog();
    new (&AppKeysTraitDataSink) ApplicationKeysTraitDataSink();
    new (&DeviceIdTraitDataSource) DeviceIdentityTraitDataSource();

    err = WdmSubscriptionEngine.Init(&ExchangeMgr, NULL, HandleSubscriptionEngineEvent);
    SuccessOrExit(err);

    serviceBinding = ExchangeMgr.NewBinding(HandleServiceBindingEvent, this);
    VerifyOrExit(NULL != serviceBinding, err = WEAVE_ERROR_NO_MEMORY);

    err = SubscriptionEngine::GetInstance()->NewClient(&mServiceSubClient,
            serviceBinding,
            this,
            HandleOutboundServiceSubscriptionEvent,
            &SubscribedServiceTraits,
            5000); // TODO: set default subscribe response timeout properly this
    SuccessOrExit(err);

    serviceBinding->Release();

    mServiceSubClient->EnableResubscribe(NULL);

    err = SubscriptionEngine::GetInstance()->EnablePublisher(NULL, &PublishedTraits);
    SuccessOrExit(err);

    AppKeysTraitDataSink.SetGroupKeyStore(ConfigurationMgr().GetGroupKeyStore());

    {
        ResourceIdentifier resourceId(ResourceIdentifier::RESOURCE_TYPE_RESERVED, ResourceIdentifier::SELF_NODE_ID);
        TraitDataHandle handle;
        err = SubscribedServiceTraits.Add(resourceId, 0, kRootPropertyPathHandle, &AppKeysTraitDataSink, handle);
        SuccessOrExit(err);
        err = PublishedTraits.Add(resourceId, 0, kRootPropertyPathHandle, &DeviceIdTraitDataSource, handle);
        SuccessOrExit(err);
    }

    mServiceSubMode = kServiceSubscriptionMode_Enabled;
    mServicePathList = NULL;
    mServiceCounterSubHandler = NULL;
    mFlags = 0;

exit:
    return err;
}

void TraitManager::OnPlatformEvent(const WeaveDeviceEvent* event)
{
    // If connectivity to the service has changed...
    if (event->Type == WeaveDeviceEvent::kEventType_ServiceConnectivityChange)
    {
        // Update the service subscription state as needed.
        DriveServiceSubscriptionState(true);
    }
}

void TraitManager::DriveServiceSubscriptionState(bool serviceConnectivityChanged)
{
    bool serviceSubShouldBeActivated =
            (mServiceSubMode == kServiceSubscriptionMode_Enabled &&
             ConnectivityMgr().IsWiFiStationProvisioned() &&
             ConfigurationMgr().IsPairedToAccount());

    // If the service subscription activation state needs to change...
    if (GetFlag(mFlags, kFlag_ServiceSubscriptionActivated) != serviceSubShouldBeActivated)
    {
        // If the system currently has service connectivity...
        if (ConnectivityMgr().HaveServiceConnectivity())
        {
            // Update the activation state.
            SetFlag(mFlags, kFlag_ServiceSubscriptionActivated, serviceSubShouldBeActivated);

            // If service subscription should be activated, schedule an async work item to activate it.
            if (serviceSubShouldBeActivated)
            {
                PlatformMgr.ScheduleWork(ActivateServiceSubscription);
            }

            // If the service subscription should be deactivated...
            else
            {
                // Abort both the outgoing and incoming service subscriptions, if established.
                mServiceSubClient->AbortSubscription();
                if (mServiceCounterSubHandler != NULL)
                {
                    mServiceCounterSubHandler->AbortSubscription();
                    mServiceCounterSubHandler = NULL;
                }

                // If prior to this the service subscription was fully established (including the service's
                // counter subscription) change the state and raise an event announcing the loss of the
                // subscription.
                if (GetFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished))
                {
                    ClearFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished);
                    WeaveDeviceEvent event;
                    event.Type = WeaveDeviceEvent::kEventType_ServiceSubscriptionStateChange;
                    event.ServiceSubscriptionStateChange.Result = kConnectivity_Lost;
                    PlatformMgr.PostEvent(&event);
                }
            }
        }
    }

    // Otherwise, if service connectivity has just been established, and a service subscription should
    // be active, but currently isn't, kick-start the resubscription process.
    else if (serviceConnectivityChanged && ConnectivityMgr().HaveServiceConnectivity() &&
             serviceSubShouldBeActivated && !mServiceSubClient->IsInProgressOrEstablished())
    {
        mServiceSubClient->ResetResubscribe();
    }
}

void TraitManager::ActivateServiceSubscription(intptr_t arg)
{
    // Enable automatic resubscription to the service using the default resubscription back-off policy.
    TraitMgr.mServiceSubClient->EnableResubscribe(NULL);

    // Initial the outbound service subscription.  This will ultimately result in the service
    // setting up an inbound counter-subscription back to the device, at which point the
    // full mutual service subscription is considered established.
    TraitMgr.mServiceSubClient->InitiateSubscription();
}

void TraitManager::HandleSubscriptionEngineEvent(void * appState, SubscriptionEngine::EventID eventType,
        const SubscriptionEngine::InEventParam & inParam, SubscriptionEngine::OutEventParam & outParam)
{
    switch (eventType)
    {
    case SubscriptionEngine::kEvent_OnIncomingSubscribeRequest:
        outParam.mIncomingSubscribeRequest.mHandlerEventCallback = HandleInboundSubscriptionEvent;
        outParam.mIncomingSubscribeRequest.mHandlerAppState = NULL;
        outParam.mIncomingSubscribeRequest.mRejectRequest = false;
        // TODO: Is this necessary?  Why isn't the counter subscription using the same Binding as the client subscription?
        // inParam.mIncomingSubscribeRequest.mBinding->SetDefaultResponseTimeout(kResponseTimeoutMsec);
        // inParam.mIncomingSubscribeRequest.mBinding->SetDefaultWRMPConfig(gWRMPConfig);
        break;

    // TODO: Add support for subscriptionless notifies

    default:
        SubscriptionEngine::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}

void TraitManager::HandleServiceBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
    const ::nl::Weave::Binding::InEventParam & inParam, ::nl::Weave::Binding::OutEventParam & outParam)
{
    Binding * binding = inParam.Source;

    // TODO: fix logging
    switch (eventType)
    {
    case nl::Weave::Binding::kEvent_PrepareRequested:
        // TODO: set response timeout
        // TODO: set WRM config
        outParam.PrepareRequested.PrepareError = binding->BeginConfiguration()
            .Target_ServiceEndpoint(kServiceEndpoint_Data_Management)
            .Transport_UDP_WRM()
            .Security_SharedCASESession()
            .PrepareBinding();
        break;

    case nl::Weave::Binding::kEvent_PrepareFailed:
        WeaveLogProgress(DeviceLayer, "Failed to prepare service subscription binding: %s", ErrorStr(inParam.PrepareFailed.Reason));
        break;

    case nl::Weave::Binding::kEvent_BindingFailed:
        WeaveLogProgress(DeviceLayer, "Service subscription binding failed: %s", ErrorStr(inParam.BindingFailed.Reason));
        break;

    case nl::Weave::Binding::kEvent_BindingReady:
        WeaveLogProgress(DeviceLayer, "Service subscription binding ready");
        break;

    default:
        nl::Weave::Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

void TraitManager::HandleOutboundServiceSubscriptionEvent(void * appState, SubscriptionClient::EventID eventType,
        const SubscriptionClient::InEventParam & inParam, SubscriptionClient::OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (eventType)
    {
    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded:
    {
        uint16_t pathListLen;

        // TODO: Fix *LAME* SubscriptionClient callback API to handle errors!!!!
        // TODO: Fix *LAME* SubscriptionClient callback API to support initiator versions!!!!

        if (TraitMgr.mServicePathList == NULL)
        {
            TraitMgr.mServicePathList = new TraitPath[TraitSinkCatalog::kMaxEntries];
        }

        err = SubscribedServiceTraits.PrepareSubscriptionPathList(TraitMgr.mServicePathList, TraitSinkCatalog::kMaxEntries, pathListLen);
        SuccessOrExit(err);

        outParam.mSubscribeRequestPrepareNeeded.mPathList = TraitMgr.mServicePathList;
        outParam.mSubscribeRequestPrepareNeeded.mPathListSize = pathListLen;
        outParam.mSubscribeRequestPrepareNeeded.mVersionedPathList = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents = false;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin = 30; // TODO: set these properly
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax = 60;

        WeaveLogProgress(DeviceLayer, "Sending outbound service subscribe request (path count %" PRIu16 ")", pathListLen);

        break;
    }
    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogProgress(DeviceLayer, "Outbound service subscription established (sub id %016" PRIX64 ")",
                inParam.mSubscriptionEstablished.mSubscriptionId);
        break;

    case SubscriptionClient::kEvent_OnSubscriptionTerminated:
        WeaveLogProgress(DeviceLayer, "Outbound service subscription terminated: %s",
                (inParam.mSubscriptionTerminated.mIsStatusCodeValid)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason));
        // If prior to this the service subscription was fully established (including the service's counter subscription)
        // change the state and raise an event announcing the loss of the subscription.
        if (GetFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished))
        {
            ClearFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished);
            WeaveDeviceEvent event;
            event.Type = WeaveDeviceEvent::kEventType_ServiceSubscriptionStateChange;
            event.ServiceSubscriptionStateChange.Result = kConnectivity_Lost;
            PlatformMgr.PostEvent(&event);
        }
        break;

    default:
        SubscriptionClient::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }

exit:
    return;
}

void TraitManager::HandleInboundSubscriptionEvent(void * aAppState, SubscriptionHandler::EventID eventType,
        const SubscriptionHandler::InEventParam & inParam, SubscriptionHandler::OutEventParam & outParam)
{
    switch (eventType)
    {
    case SubscriptionHandler::kEvent_OnSubscribeRequestParsed:
        if (inParam.mSubscribeRequestParsed.mIsSubscriptionIdValid &&
            inParam.mSubscribeRequestParsed.mMsgInfo->SourceNodeId == kServiceEndpoint_Data_Management)
        {
            WeaveLogProgress(DeviceLayer, "Inbound service counter-subscription request received (sub id %016" PRIX64 ", path count %" PRId16 ")",
                    inParam.mSubscribeRequestParsed.mSubscriptionId,
                    inParam.mSubscribeRequestParsed.mNumTraitInstances);

            TraitMgr.mServiceCounterSubHandler = inParam.mSubscribeRequestParsed.mHandler;
        }
        else
        {
#if WEAVE_PROGRESS_LOGGING
            char peerDesc[kWeavePeerDescription_MaxLength];
            WeaveMessageLayer::GetPeerDescription(peerDesc, sizeof(peerDesc), inParam.mSubscribeRequestParsed.mMsgInfo);
            WeaveLogProgress(DeviceLayer, "Inbound subscription request received from node %s (path count %" PRId16 ")",
                    peerDesc,
                    inParam.mSubscribeRequestParsed.mNumTraitInstances);
#endif // WEAVE_PROGRESS_LOGGING
        }

        // TODO: dispatch this to the event loop to avoid crazy-deep stack.
        // TODO: is this the right way to set the subscription timeout?
        inParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest(inParam.mSubscribeRequestParsed.mTimeoutSecMin);
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionEstablished:
        if (inParam.mSubscriptionEstablished.mHandler == TraitMgr.mServiceCounterSubHandler)
        {
            WeaveLogProgress(DeviceLayer, "Inbound service counter-subscription established");

            // Note that the service subscription is fully established.
            SetFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished);

            // Raise an event announcing the establishment of the subscription.
            {
                WeaveDeviceEvent event;
                event.Type = WeaveDeviceEvent::kEventType_ServiceSubscriptionStateChange;
                event.ServiceSubscriptionStateChange.Result = kConnectivity_Established;
                PlatformMgr.PostEvent(&event);
            }
        }
        else
        {
#if WEAVE_PROGRESS_LOGGING
            uint64_t peerNodeId = inParam.mSubscriptionEstablished.mHandler->GetPeerNodeId();
            uint64_t subId;
            inParam.mSubscriptionEstablished.mHandler->GetSubscriptionId(&subId);
            WeaveLogProgress(DeviceLayer, "Inbound subscription established with node %016" PRIX64 "(sub id %016" PRIX64 ")",
                    peerNodeId, subId);
#endif // WEAVE_PROGRESS_LOGGING
        }
        break;

    case SubscriptionHandler::kEvent_OnSubscriptionTerminated:
    {
#if WEAVE_PROGRESS_LOGGING
        const char * termDesc =
                (inParam.mSubscriptionTerminated.mReason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason);
#endif // WEAVE_PROGRESS_LOGGING
        if (inParam.mSubscriptionTerminated.mHandler == TraitMgr.mServiceCounterSubHandler)
        {
            WeaveLogProgress(DeviceLayer, "Inbound service counter-subscription terminated: %s", termDesc);

            TraitMgr.mServiceCounterSubHandler = NULL;

            // If prior to this the service subscription was fully established (including the device's outbound subscription)
            // change the state and raise an event announcing the loss of the subscription.
            if (GetFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished))
            {
                ClearFlag(TraitMgr.mFlags, kFlag_ServiceSubscriptionEstablished);
                WeaveDeviceEvent event;
                event.Type = WeaveDeviceEvent::kEventType_ServiceSubscriptionStateChange;
                event.ServiceSubscriptionStateChange.Result = kConnectivity_Lost;
                PlatformMgr.PostEvent(&event);
            }
        }
        else
        {
#if WEAVE_PROGRESS_LOGGING
            uint64_t peerNodeId = inParam.mSubscriptionTerminated.mHandler->GetPeerNodeId();
            uint64_t subId;
            inParam.mSubscriptionTerminated.mHandler->GetSubscriptionId(&subId);
            WeaveLogProgress(DeviceLayer, "Inbound subscription terminated with node %016" PRIX64 "(sub id %016" PRIX64 "): %s",
                    peerNodeId, subId, termDesc);
#endif // WEAVE_PROGRESS_LOGGING
        }
        break;
    }

    default:
        SubscriptionHandler::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}


} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


namespace {

template <typename T>
TraitCatalogImpl<T>::TraitCatalogImpl()
{
    // Nothing to do.
}

template <typename T>
TraitCatalogImpl<T>::~TraitCatalogImpl()
{
    // Nothing to do.
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::Add(const ResourceIdentifier & resId, const uint64_t & instanceId,
        PropertyPathHandle basePathHandle, T * traitInstance, TraitDataHandle & traitHandle)
{
    uint8_t freeIndex = kMaxEntries;

    // Search the catalog...
    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        // Keep track of the first free entry.
        if (mEntries[i].Item == NULL)
        {
            if (freeIndex == kMaxEntries)
            {
                freeIndex = i;
            }
            continue;
        }

        // If the resource, trait id and instance id match an existing entry, replace the
        // existing trait instance with the supplied one, reusing the assigned trait handle.
        if (mEntries[i].ResourceId == resId &&
            mEntries[i].Item->GetSchemaEngine()->GetProfileId() == traitInstance->GetSchemaEngine()->GetProfileId() &&
            mEntries[i].InstanceId == instanceId)
        {
            mEntries[i].Item = traitInstance;
            mEntries[i].BasePathHandle = basePathHandle;
            traitHandle = MakeTraitDataHandle(i, mEntries[i].EntryRevision);
            return WEAVE_NO_ERROR;
        }
    }

    // Fail if the catalog is full.
    if (freeIndex == kMaxEntries)
    {
        return WEAVE_ERROR_NO_MEMORY;
    }

    // Add the new trait instance.
    mEntries[freeIndex].ResourceId = resId;
    mEntries[freeIndex].InstanceId = instanceId;
    mEntries[freeIndex].Item = traitInstance;
    mEntries[freeIndex].BasePathHandle = basePathHandle;
    mEntries[freeIndex].EntryRevision++;

    traitHandle = MakeTraitDataHandle(freeIndex, mEntries[freeIndex].EntryRevision);

    return WEAVE_NO_ERROR;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::Remove(T * traitInstance)
{
    for (TraitDataHandle i = 0; i < kMaxEntries; i++)
    {
        if (mEntries[i].Item == traitInstance)
        {
            mEntries[i].Item = NULL;
            return WEAVE_NO_ERROR;
        }
    }

    return WEAVE_ERROR_INVALID_ARGUMENT;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::PrepareSubscriptionPathList(TraitPath * pathList, uint16_t pathListSize, uint16_t & pathListLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    pathListLen = 0;

    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        if (mEntries[i].Item != NULL)
        {
            VerifyOrExit(pathListLen < pathListSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            *pathList++ = TraitPath(MakeTraitDataHandle(i, mEntries[i].EntryRevision), mEntries[i].BasePathHandle);
            pathListLen++;
        }
    }

exit:
    return err;
}

WEAVE_ERROR DecodeTraitInstancePath(TLV::TLVReader & aReader, ResourceIdentifier & resourceId, uint32_t & profileId,
        SchemaVersionRange & aSchemaVersionRange, uint64_t & instanceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Path::Parser path;

    instanceId = 0;

    err = path.Init(aReader);
    SuccessOrExit(err);

    {
        nl::Weave::TLV::TLVReader reader;

        err = path.GetResourceID(&reader);
        if (err == WEAVE_NO_ERROR)
        {
            err = resourceId.FromTLV(reader, ::nl::Weave::DeviceLayer::FabricState.LocalNodeId);
            SuccessOrExit(err);
        }
        else if (err == WEAVE_END_OF_TLV)
        {
            resourceId = ResourceIdentifier(ResourceIdentifier::RESOURCE_TYPE_RESERVED, ResourceIdentifier::SELF_NODE_ID);
        }
        else
        {
            ExitNow();
        }
    }

    err = path.GetProfileID(&profileId, &aSchemaVersionRange);
    SuccessOrExit(err);

    err = path.GetInstanceID(&instanceId);
    if ((WEAVE_NO_ERROR != err) && (WEAVE_END_OF_TLV != err))
    {
        ExitNow();
    }

    err = path.GetTags(&aReader);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::AddressToHandle(TLV::TLVReader & aReader, TraitDataHandle & aHandle,
        SchemaVersionRange & aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ResourceIdentifier resourceId;
    uint32_t profileId;
    uint64_t instanceId = 0;

    err = DecodeTraitInstancePath(aReader, resourceId, profileId, aSchemaVersionRange, instanceId);
    SuccessOrExit(err);

    err = WEAVE_ERROR_INVALID_PROFILE_ID; // TODO: Use sensible error!
    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        const CatalogEntry * entry = &mEntries[i];

        if (entry->Item != NULL &&
            entry->ResourceId == resourceId &&
            entry->Item->GetSchemaEngine()->GetProfileId() == profileId &&
            entry->InstanceId == instanceId)
        {
            err = WEAVE_NO_ERROR;
            aHandle = MakeTraitDataHandle(i, entry->EntryRevision);
            break;
        }
    }
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR EncodeTraitInstancePath(TLV::TLVWriter & writer, const ResourceIdentifier & resourceId, uint32_t profileId,
        SchemaVersionRange & schemaVersionRange, const uint64_t & instanceId)
{
    WEAVE_ERROR err;
    TLV::TLVType containerType;

    VerifyOrExit(schemaVersionRange.IsValid(), err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = writer.StartContainer(TLV::ContextTag(Path::kCsTag_InstanceLocator), TLV::kTLVType_Structure, containerType);
    SuccessOrExit(err);

    if (schemaVersionRange.mMinVersion != 1 || schemaVersionRange.mMaxVersion != 1)
    {
        TLV::TLVType containerType2;

        err = writer.StartContainer(TLV::ContextTag(Path::kCsTag_TraitProfileID), TLV::kTLVType_Array, containerType2);
        SuccessOrExit(err);

        err = writer.Put(TLV::AnonymousTag, profileId);
        SuccessOrExit(err);

        // Only encode the max version if it isn't 1.
        if (schemaVersionRange.mMaxVersion != 1)
        {
            err = writer.Put(TLV::AnonymousTag, schemaVersionRange.mMaxVersion);
            SuccessOrExit(err);
        }

        // Only encode the min version if it isn't 1.
        if (schemaVersionRange.mMinVersion != 1)
        {
            err = writer.Put(TLV::AnonymousTag, schemaVersionRange.mMinVersion);
            SuccessOrExit(err);
        }

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }
    else
    {
        err = writer.Put(TLV::ContextTag(Path::kCsTag_TraitProfileID), profileId);
        SuccessOrExit(err);
    }

    if (instanceId != 0)
    {
        err = writer.Put(TLV::ContextTag(Path::kCsTag_TraitInstanceID), instanceId);
        SuccessOrExit(err);
    }

    err = resourceId.ToTLV(writer);
    SuccessOrExit(err);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::HandleToAddress(TraitDataHandle aHandle, TLV::TLVWriter& aWriter,
        SchemaVersionRange& aSchemaVersionRange) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const CatalogEntry * entry;
    uint32_t profileId;
    uint8_t handleIndex = HandleIndex(aHandle);
    uint8_t handleRev = HandleRevision(aHandle);

    VerifyOrExit(handleIndex < kMaxEntries, err = WEAVE_ERROR_INVALID_ARGUMENT);

    entry = &mEntries[handleIndex];

    VerifyOrExit(handleRev == entry->EntryRevision, err = WEAVE_ERROR_INVALID_ARGUMENT);

    profileId = entry->Item->GetSchemaEngine()->GetProfileId();

    err = EncodeTraitInstancePath(aWriter, entry->ResourceId, profileId, aSchemaVersionRange, entry->InstanceId);
    SuccessOrExit(err);

exit:
    return err;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::Locate(TraitDataHandle aHandle, T ** aTraitInstance) const
{
    uint8_t handleIndex = HandleIndex(aHandle);
    uint8_t handleRev = HandleRevision(aHandle);

    if (handleIndex < kMaxEntries && mEntries[handleIndex].Item != NULL && mEntries[handleIndex].EntryRevision == handleRev)
    {
        *aTraitInstance = mEntries[handleIndex].Item;
        return WEAVE_NO_ERROR;
    }

    return WEAVE_ERROR_INVALID_ARGUMENT;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::Locate(T * aTraitInstance, TraitDataHandle & aHandle) const
{
    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        if (mEntries[i].Item == aTraitInstance)
        {
            aHandle = MakeTraitDataHandle(i, mEntries[i].EntryRevision);
            return WEAVE_NO_ERROR;
        }
    }

    return WEAVE_ERROR_INVALID_ARGUMENT;
}

template <typename T>
WEAVE_ERROR TraitCatalogImpl<T>::DispatchEvent(uint16_t aEvent, void * aContext) const
{
    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        if (mEntries[i].Item != NULL)
        {
            mEntries[i].Item->OnEvent(aEvent, aContext);
        }
    }

    return WEAVE_NO_ERROR;
}

template <typename T>
void TraitCatalogImpl<T>::Iterate(IteratorCallback aCallback, void * aContext)
{
    for (uint8_t i = 0; i < kMaxEntries; i++)
    {
        if (mEntries[i].Item != NULL)
        {
            aCallback(mEntries[i].Item, MakeTraitDataHandle(i, mEntries[i].EntryRevision), aContext);
        }
    }
}

template class TraitCatalogImpl<TraitDataSink>;
template class TraitCatalogImpl<TraitDataSource>;

} // unnamed namespace

nl::Weave::Profiles::DataManagement::SubscriptionEngine * nl::Weave::Profiles::DataManagement::SubscriptionEngine::GetInstance()
{
    return &WdmSubscriptionEngine;
}

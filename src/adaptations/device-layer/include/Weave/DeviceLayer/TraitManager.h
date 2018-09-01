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

/**
 *    @file
 *      Defines the Weave Device Layer TraitManager object.
 *
 */

#ifndef TRAIT_MANAGER_H
#define TRAIT_MANAGER_H

#include <Weave/Profiles/data-management/Current/DataManagement.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

class PlatformManagerImpl;

/**
 * Manages publication and subscription of Weave Data Management traits for a Weave device.
 */
class TraitManager final
{
    typedef ::nl::Weave::Profiles::DataManagement_Current::SubscriptionClient SubscriptionClient;
    typedef ::nl::Weave::Profiles::DataManagement_Current::SubscriptionEngine SubscriptionEngine;
    typedef ::nl::Weave::Profiles::DataManagement_Current::SubscriptionHandler SubscriptionHandler;
    typedef ::nl::Weave::Profiles::DataManagement_Current::TraitDataSink TraitDataSink;
    typedef ::nl::Weave::Profiles::DataManagement_Current::TraitDataSource TraitDataSource;
    typedef ::nl::Weave::Profiles::DataManagement_Current::TraitPath TraitPath;
    typedef ::nl::Weave::Profiles::DataManagement_Current::ResourceIdentifier ResourceIdentifier;
    typedef ::nl::Weave::Profiles::DataManagement_Current::PropertyPathHandle PropertyPathHandle;

public:

    // ===== Members that define the public interface of the TraitManager

    enum ServiceSubscriptionMode
    {
        kServiceSubscriptionMode_NotSupported               = -1,
        kServiceSubscriptionMode_Disabled                   = 0,
        kServiceSubscriptionMode_Enabled                    = 1,
    };

    ServiceSubscriptionMode GetServiceSubscriptionMode(void);
    WEAVE_ERROR SetServiceSubscriptionMode(ServiceSubscriptionMode val);
    bool IsServiceSubscriptionEstablished(void);
    uint32_t GetServiceSubscribeConfirmIntervalMS(void) const;
    WEAVE_ERROR SetServiceSubscribeConfirmIntervalMS(uint32_t val) const;

    WEAVE_ERROR SubscribeServiceTrait(const ResourceIdentifier & resId, const uint64_t & instanceId,
            PropertyPathHandle basePathHandle, TraitDataSink * dataSink);
    WEAVE_ERROR UnsubscribeServiceTrait(TraitDataSink * dataSink);

    WEAVE_ERROR PublishTrait(const uint64_t & instanceId, TraitDataSource * dataSource);
    WEAVE_ERROR PublishTrait(const ResourceIdentifier & resId, const uint64_t & instanceId, TraitDataSource * dataSource);
    WEAVE_ERROR UnpublishTrait(TraitDataSource * dataSource);

private:

    // ===== Members for internal use by the following friends.

    friend class ::nl::Weave::DeviceLayer::PlatformManagerImpl;
    friend TraitManager & TraitMgr(void);

    static TraitManager sInstance;

    WEAVE_ERROR Init(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

    // ===== Private members for use by this class only.

    enum
    {
        kFlag_ServiceSubscriptionEstablished        = 0x01,
        kFlag_ServiceSubscriptionActivated          = 0x02,
    };

    ServiceSubscriptionMode mServiceSubMode;
    SubscriptionClient * mServiceSubClient;
    SubscriptionHandler * mServiceCounterSubHandler;
    TraitPath * mServicePathList;
    uint8_t mFlags;

    void DriveServiceSubscriptionState(bool serviceConnectivityChanged);

    static void ActivateServiceSubscription(intptr_t arg);
    static void HandleSubscriptionEngineEvent(void * appState, SubscriptionEngine::EventID eventType,
            const SubscriptionEngine::InEventParam & inParam, SubscriptionEngine::OutEventParam & outParam);
    static void HandleServiceBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
        const ::nl::Weave::Binding::InEventParam & inParam, ::nl::Weave::Binding::OutEventParam & outParam);
    static void HandleOutboundServiceSubscriptionEvent(void * appState, SubscriptionClient::EventID eventType,
            const SubscriptionClient::InEventParam & inParam, SubscriptionClient::OutEventParam & outParam);
    static void HandleInboundSubscriptionEvent(void * aAppState, SubscriptionHandler::EventID eventType,
            const SubscriptionHandler::InEventParam & inParam, SubscriptionHandler::OutEventParam & outParam);
};

inline TraitManager::ServiceSubscriptionMode TraitManager::GetServiceSubscriptionMode(void)
{
    return mServiceSubMode;
}

inline bool TraitManager::IsServiceSubscriptionEstablished(void)
{
    return GetFlag(mFlags, kFlag_ServiceSubscriptionEstablished);
}

/**
 * Returns a reference to the TraitManager singleton object.
 */
inline TraitManager & TraitMgr(void)
{
    return TraitManager::sInstance;
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // TRAIT_MANAGER_H

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
 *          Defines the Device Layer ServiceProvisioningServer object.
 */

#ifndef SERVICE_PROVISIONING_SERVER_H
#define SERVICE_PROVISIONING_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace StatusReporting {
class StatusReport;
}
}
}
}

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Implements the Weave Service Provisioning profile for a Weave device.
 */
class ServiceProvisioningServer final
    : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer,
      public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
    using ServerBaseClass = ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer;

public:

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

    // ===== Members that override virtual methods on ServiceProvisioningDelegate

    WEAVE_ERROR HandleRegisterServicePairAccount(::nl::Weave::Profiles::ServiceProvisioning::RegisterServicePairAccountMessage& msg) override;
    WEAVE_ERROR HandleUpdateService(::nl::Weave::Profiles::ServiceProvisioning::UpdateServiceMessage& msg) override;
    WEAVE_ERROR HandleUnregisterService(uint64_t serviceId) override;
    void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode) override;
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
    void HandleIFJServiceFabricJoinResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode) override;
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

    // ===== Members that override virtual methods on ServiceProvisioningServer

    bool IsPairedToAccount(void) const override;

private:

    // ===== Members for internal use by the following friends.

    friend ServiceProvisioningServer & ServiceProvisioningSvr(void);

    static ServiceProvisioningServer sInstance;

    // ===== Members for internal use by this class only.

    ::nl::Weave::Binding * mProvServiceBinding;
    bool mWaitingForServiceTunnel;

    void StartPairDeviceToAccount(void);
    void SendPairDeviceToAccountRequest(void);

    static void AsyncStartPairDeviceToAccount(intptr_t arg);
    static void HandleServiceTunnelTimeout(::nl::Weave::System::Layer * layer, void * appState, ::nl::Weave::System::Error err);
    static void HandleProvServiceBindingEvent(void * appState, nl::Weave::Binding::EventType eventType,
            const nl::Weave::Binding::InEventParam & inParam, nl::Weave::Binding::OutEventParam & outParam);
};

inline ServiceProvisioningServer & ServiceProvisioningSvr()
{
    return ServiceProvisioningServer::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // SERVICE_PROVISIONING_SERVER_H

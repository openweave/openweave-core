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

#ifndef SERVICE_PROVISIONING_SERVER_H
#define SERVICE_PROVISIONING_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
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
namespace Device {
namespace Internal {

class ServiceProvisioningServer
        : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer,
          public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
public:

    typedef ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init(void);
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(::nl::Weave::Profiles::ServiceProvisioning::RegisterServicePairAccountMessage& msg);
    virtual WEAVE_ERROR HandleUpdateService(::nl::Weave::Profiles::ServiceProvisioning::UpdateServiceMessage& msg);
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    virtual bool IsPairedToAccount(void) const;

    void OnPlatformEvent(const WeaveDeviceEvent * event);

private:

    ::nl::Weave::Binding * mProvServiceBinding;
    bool mWaitingForServiceTunnel;

    void StartPairDeviceToAccount(void);
    void SendPairDeviceToAccountRequest(void);
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);

    static void AsyncStartPairDeviceToAccount(intptr_t arg);
    static void HandleServiceTunnelTimeout(::nl::Weave::System::Layer * layer, void * appState, ::nl::Weave::System::Error err);
    static void HandleProvServiceBindingEvent(void * appState, nl::Weave::Binding::EventType eventType,
            const nl::Weave::Binding::InEventParam & inParam, nl::Weave::Binding::OutEventParam & outParam);
};

extern ServiceProvisioningServer ServiceProvisioningSvr;

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // SERVICE_PROVISIONING_SERVER_H

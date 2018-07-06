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

#ifndef FABRIC_PROVISIONING_SERVER_H
#define FABRIC_PROVISIONING_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class FabricProvisioningServer
        : public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer,
          public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();
    virtual WEAVE_ERROR HandleCreateFabric(void);
    virtual WEAVE_ERROR HandleJoinExistingFabric(void);
    virtual WEAVE_ERROR HandleLeaveFabric(void);
    virtual WEAVE_ERROR HandleGetFabricConfig(void);
    WEAVE_ERROR LeaveFabric(void);
    virtual bool IsPairedToAccount() const;

    void OnPlatformEvent(const WeaveDeviceEvent * event);
};

extern FabricProvisioningServer FabricProvisioningSvr;

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // FABRIC_PROVISIONING_SERVER_H

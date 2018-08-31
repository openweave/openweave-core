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
 *          Defines the Device Layer FabricProvisioningServer object.
 */

#ifndef FABRIC_PROVISIONING_SERVER_H
#define FABRIC_PROVISIONING_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Implements the Weave Fabric Provisioning profile for a Weave device.
 */
class FabricProvisioningServer final
    : public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer,
      public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningDelegate
{
    using ServerBaseClass = ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer;

public:

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init();
    void OnPlatformEvent(const WeaveDeviceEvent * event);
    WEAVE_ERROR LeaveFabric(void);

    // ===== Members that override virtual methods on FabricProvisioningDelegate

    WEAVE_ERROR HandleCreateFabric(void) override;
    WEAVE_ERROR HandleJoinExistingFabric(void) override;
    WEAVE_ERROR HandleLeaveFabric(void) override;
    WEAVE_ERROR HandleGetFabricConfig(void) override;

    // ===== Members that override virtual methods on FabricProvisioningServer

    bool IsPairedToAccount() const override;

private:

    // ===== Members for internal use by the following friends.

    friend FabricProvisioningServer & FabricProvisioningSvr(void);

    static FabricProvisioningServer sInstance;
};

/**
 * Returns a reference to the FabricProvisioningServer singleton object.
 */
inline FabricProvisioningServer & FabricProvisioningSvr(void)
{
    return FabricProvisioningServer::sInstance;
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // FABRIC_PROVISIONING_SERVER_H

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
#include <Weave/DeviceLayer/internal/FabricProvisioningServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::FabricProvisioning;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR FabricProvisioningServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::DeviceLayer::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleCreateFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigurationMgr().StoreFabricId(::nl::Weave::DeviceLayer::FabricState.FabricId);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "Weave fabric created; fabric id %016" PRIX64, ::nl::Weave::DeviceLayer::FabricState.FabricId);

    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_FabricMembershipChange;
        event.FabricMembershipChange.IsMemberOfFabric = true;
        PlatformMgr.PostEvent(&event);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleJoinExistingFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigurationMgr().StoreFabricId(::nl::Weave::DeviceLayer::FabricState.FabricId);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "Join existing Weave fabric; fabric id %016" PRIX64, ::nl::Weave::DeviceLayer::FabricState.FabricId);

    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_FabricMembershipChange;
        event.FabricMembershipChange.IsMemberOfFabric = true;
        PlatformMgr.PostEvent(&event);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleLeaveFabric(void)
{
    WEAVE_ERROR err;

    WeaveLogProgress(DeviceLayer, "Leave Weave fabric");

    err = ConfigurationMgr().StoreFabricId(kFabricIdNotSpecified);
    SuccessOrExit(err);

    {
        WeaveDeviceEvent event;
        event.Type = WeaveDeviceEvent::kEventType_FabricMembershipChange;
        event.FabricMembershipChange.IsMemberOfFabric = false;
        PlatformMgr.PostEvent(&event);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::LeaveFabric(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (ConfigurationMgr().IsMemberOfFabric())
    {
        // Clear the fabric state.
        FabricState->ClearFabricState();

        // Clear the persisted fabric id.
        err = ConfigurationMgr().StoreFabricId(kFabricIdNotSpecified);
        SuccessOrExit(err);

        // Post a FabricMembershipChange event.
        {
            WeaveDeviceEvent event;
            event.Type = WeaveDeviceEvent::kEventType_FabricMembershipChange;
            event.FabricMembershipChange.IsMemberOfFabric = false;
            PlatformMgr.PostEvent(&event);
        }
    }

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleGetFabricConfig(void)
{
    // Nothing to do
    return WEAVE_NO_ERROR;
}

bool FabricProvisioningServer::IsPairedToAccount() const
{
    return ConfigurationMgr().IsServiceProvisioned() && ConfigurationMgr().IsPairedToAccount();
}

void FabricProvisioningServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // Nothing to do so far.
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

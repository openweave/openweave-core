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
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/NetworkInfo.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_Common;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

NetworkProvisioningServerImpl NetworkProvisioningServerImpl::sInstance;

WEAVE_ERROR NetworkProvisioningServerImpl::_Init(void)
{
    // TODO: implement this
    return NULL;
}

::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate * NetworkProvisioningServerImpl::_GetDelegate(void)
{
    // TODO: implement this
    return NULL;
}

void NetworkProvisioningServerImpl::_StartPendingScan(void)
{
    // TODO: implement this
}

bool NetworkProvisioningServerImpl::_ScanInProgress(void)
{
    // TODO: implement this
    return false;
}

void NetworkProvisioningServerImpl::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // TODO: implement this
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

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

#include <internal/WeaveDeviceInternal.h>
#include <internal/NetworkProvisioningServer.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_NetworkProvisioning;

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

WEAVE_ERROR NetworkProvisioningServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::Device::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(ConnectivityMgr.GetNetworkProvisioningDelegate());

exit:
    return err;
}

bool NetworkProvisioningServer::IsPairedToAccount() const
{
    return ConfigurationMgr.IsServiceProvisioned() && ConfigurationMgr.IsPairedToAccount();
}

void NetworkProvisioningServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // Nothing to do so far.
}

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

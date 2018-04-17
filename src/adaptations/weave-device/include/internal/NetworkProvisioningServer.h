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

#ifndef NETWORK_PROVISIONING_SERVER_H
#define NETWORK_PROVISIONING_SERVER_H

#include <internal/WeaveDeviceInternal.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class NetworkProvisioningServer
        : public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer
{
public:
    typedef ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();

    int16_t GetCurrentOp() const;

    virtual bool IsPairedToAccount() const;

    void OnPlatformEvent(const WeaveDeviceEvent * event);
};

extern NetworkProvisioningServer NetworkProvisioningSvr;

inline int16_t NetworkProvisioningServer::GetCurrentOp() const
{
    return (mCurOp != NULL) ? mCurOpType : -1;
}

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // NETWORK_PROVISIONING_SERVER_H

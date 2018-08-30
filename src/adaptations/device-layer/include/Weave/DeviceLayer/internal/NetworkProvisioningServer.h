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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

class NetworkProvisioningServerImpl;

/**
 * Provides network provisioning services for a Weave Device.
 */
class NetworkProvisioningServer
{
    using NetworkProvisioningDelegate = ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate;
    using ImplClass = NetworkProvisioningServerImpl;

public:

    // Members for internal use by components within the Weave Device Layer.

    WEAVE_ERROR Init(void);
    NetworkProvisioningDelegate * GetDelegate(void);
    void StartPendingScan(void);
    bool ScanInProgress(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

protected:

    // Access to construction/destruction is limited to subclasses.
    NetworkProvisioningServer(void) = default;
    ~NetworkProvisioningServer(void) = default;
};

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/* Include a header file containing the implementation of the NetworkProvisioningServer
 * object for the selected platform.
 */
#ifdef EXTERNAL_NETWORKPROVISIONINGSERVERIMPL_HEADER
#include EXTERNAL_NETWORKPROVISIONINGSERVERIMPL_HEADER
#else
#define NETWORKPROVISIONINGSERVERIMPL_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/NetworkProvisioningServerImpl.h>
#include NETWORKPROVISIONINGSERVERIMPL_HEADER
#endif

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

inline WEAVE_ERROR NetworkProvisioningServer::Init(void)
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate * NetworkProvisioningServer::GetDelegate(void)
{
    return static_cast<ImplClass*>(this)->_GetDelegate();
}

inline void NetworkProvisioningServer::StartPendingScan(void)
{
    static_cast<ImplClass*>(this)->_StartPendingScan();
}

inline bool NetworkProvisioningServer::ScanInProgress(void)
{
    return static_cast<ImplClass*>(this)->_ScanInProgress();
}

inline void NetworkProvisioningServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    static_cast<ImplClass*>(this)->_OnPlatformEvent(event);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // NETWORK_PROVISIONING_SERVER_H

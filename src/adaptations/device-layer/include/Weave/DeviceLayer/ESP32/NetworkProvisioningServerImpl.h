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

#ifndef NETWORK_PROVISIONING_SERVER_IMPL_H
#define NETWORK_PROVISIONING_SERVER_IMPL_H

#include <Weave/DeviceLayer/internal/GenericNetworkProvisioningServerImpl.h>


namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// Instruct the compiler to instantiate the GenericNetworkProvisioningServerImpl<> template
// only when explicitly instructed to do so.
extern template class GenericNetworkProvisioningServerImpl<NetworkProvisioningServerImpl>;

/**
 * Concrete implementation of the NetworkProvisioningServer interface for the ESP32 platform.
 */
class NetworkProvisioningServerImpl
    : public NetworkProvisioningServer,
      public GenericNetworkProvisioningServerImpl<NetworkProvisioningServerImpl>
{
private:

    using GenericImplClass = GenericNetworkProvisioningServerImpl<NetworkProvisioningServerImpl>;

    // Allow the NetworkProvisioningServer interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend class NetworkProvisioningServer;

    // Allow the GenericNetworkProvisioningServerImpl base class to access helper methods
    // and types defined on this class.
    friend class GenericNetworkProvisioningServerImpl<NetworkProvisioningServerImpl>;

    // ===== Members that implement the NetworkProvisioningServer public interface.

    void _OnPlatformEvent(const WeaveDeviceEvent * event);

    // NOTE: Other public interface methods are implemented by GenericNetworkProvisioningServerImpl<>.

    // ===== Members used by GenericNetworkProvisioningServerImpl<> to invoke platform-specific
    //       operations.

    WEAVE_ERROR GetWiFiStationProvision(NetworkInfo & netInfo, bool includeCredentials);
    WEAVE_ERROR SetWiFiStationProvision(const NetworkInfo & netInfo);
    WEAVE_ERROR ClearWiFiStationProvision(void);
    WEAVE_ERROR InitiateWiFiScan(void);
    void HandleScanDone(void);
    static NetworkProvisioningServerImpl & Instance(void);
    static void HandleScanTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

    // ===== Members for internal use by the following friends.

    friend ::nl::Weave::DeviceLayer::Internal::NetworkProvisioningServer & NetworkProvisioningSvr(void);

    static NetworkProvisioningServerImpl sInstance;
};

inline NetworkProvisioningServerImpl & NetworkProvisioningServerImpl::Instance(void)
{
    return sInstance;
}

inline NetworkProvisioningServer & NetworkProvisioningSvr(void)
{
    return NetworkProvisioningServerImpl::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NETWORK_PROVISIONING_SERVER_IMPL_H

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

#ifndef GENERIC_NETWORK_PROVISIONING_SERVER_IMPL_H
#define GENERIC_NETWORK_PROVISIONING_SERVER_IMPL_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass>
class GenericNetworkProvisioningServerImpl
        : public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer,
          public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate
{
protected:

    using ServerBaseClass = ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer;
    using NetworkInfo = ::nl::Weave::DeviceLayer::Internal::NetworkInfo;
    using PacketBuffer = ::nl::Weave::System::PacketBuffer;

    // ===== Members that implement the NetworkProvisioningServer public interface

    WEAVE_ERROR _Init(void);
    NetworkProvisioningDelegate * _GetDelegate(void);
    void _StartPendingScan(void);
    bool _ScanInProgress(void);
    void _OnPlatformEvent(const WeaveDeviceEvent * event);

    // ===== Members that implement pure virtual methods on NetworkProvisioningDelegate

    virtual WEAVE_ERROR HandleScanNetworks(uint8_t networkType);
    virtual WEAVE_ERROR HandleAddNetwork(PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleUpdateNetwork(PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleRemoveNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleGetNetworks(uint8_t flags);
    virtual WEAVE_ERROR HandleEnableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleDisableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleTestConnectivity(uint32_t networkId);
    virtual WEAVE_ERROR HandleSetRendezvousMode(uint16_t rendezvousMode);

    // ===== Members that implement pure virtual methods on NetworkProvisioningServer

    virtual bool IsPairedToAccount(void) const;

    // ===== Members for use by the NetworkProvisioningServer implementation
    //       (both generic and platform-specific).

    enum State
    {
        kState_Idle = 0,
        kState_ScanNetworks_Pending,
        kState_ScanNetworks_InProgress,
        kState_TestConnectivity_WaitConnectivity
    };

    enum
    {
        kWiFiStationNetworkId  = 1
    };

    State mState;

    int16_t GetCurrentOp(void) const;
    WEAVE_ERROR ValidateWiFiStationProvision(const NetworkInfo & netInfo, uint32_t & statusProfileId, uint16_t & statusCode);
    bool RejectIfApplicationControlled(bool station);
    void ContinueTestConnectivity(void);

    static void HandleConnectivityTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

private:
    ImplClass * Impl() { return static_cast<ImplClass *>(this); }
};

template<class ImplClass>
inline ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate *
GenericNetworkProvisioningServerImpl<ImplClass>::_GetDelegate(void)
{
    return this;
}

template<class ImplClass>
inline int16_t GenericNetworkProvisioningServerImpl<ImplClass>::GetCurrentOp() const
{
    return (mCurOp != NULL) ? mCurOpType : -1;
}

template<class ImplClass>
inline bool GenericNetworkProvisioningServerImpl<ImplClass>::_ScanInProgress(void)
{
    return mState == kState_ScanNetworks_InProgress;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NETWORK_PROVISIONING_SERVER_H

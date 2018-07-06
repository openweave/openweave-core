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

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class NetworkProvisioningServer
        : public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer,
          public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();
    int16_t GetCurrentOp() const;
    void StartPendingScan(void);
    bool ScanInProgress(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

private:
    enum State
    {
        kState_Idle = 0,
        kState_ScanNetworks_Pending,
        kState_ScanNetworks_InProgress,
        kState_TestConnectivity_WaitConnectivity
    };

    State mState;

    WEAVE_ERROR GetWiFiStationProvision(::nl::Weave::Device::Internal::NetworkInfo & netInfo, bool includeCredentials);
    WEAVE_ERROR ValidateWiFiStationProvision(const ::nl::Weave::Device::Internal::NetworkInfo & netInfo,
                    uint32_t & statusProfileId, uint16_t & statusCode);
    WEAVE_ERROR SetESPStationConfig(const ::nl::Weave::Device::Internal::NetworkInfo & netInfo);
    bool RejectIfApplicationControlled(bool station);
    void HandleScanDone(void);
    void ContinueTestConnectivity(void);
    static void HandleScanTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
    static void HandleConnectivityTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);

    // NetworkProvisioningDelegate methods
    virtual WEAVE_ERROR HandleScanNetworks(uint8_t networkType);
    virtual WEAVE_ERROR HandleAddNetwork(::nl::Weave::System::PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleUpdateNetwork(::nl::Weave::System::PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleRemoveNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleGetNetworks(uint8_t flags);
    virtual WEAVE_ERROR HandleEnableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleDisableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleTestConnectivity(uint32_t networkId);
    virtual WEAVE_ERROR HandleSetRendezvousMode(uint16_t rendezvousMode);

    // NetworkProvisioningServer methods
    virtual bool IsPairedToAccount() const;
};

extern NetworkProvisioningServer NetworkProvisioningSvr;

inline int16_t NetworkProvisioningServer::GetCurrentOp() const
{
    return (mCurOp != NULL) ? mCurOpType : -1;
}

inline bool NetworkProvisioningServer::ScanInProgress(void)
{
    return mState == kState_ScanNetworks_InProgress;
}

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // NETWORK_PROVISIONING_SERVER_H

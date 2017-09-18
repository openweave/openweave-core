/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Weave Network Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKNPSERVER_H_
#define MOCKNPSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkInfo.h>

using nl::Weave::System::PacketBuffer;
using nl::Weave::WeaveExchangeManager;
using nl::Weave::Profiles::NetworkProvisioning::NetworkInfo;
using nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer;
using nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate;

class MockNetworkProvisioningServer: private NetworkProvisioningServer, private NetworkProvisioningDelegate
{
public:
    MockNetworkProvisioningServer();

    enum
    {
        kMaxScanResults = 4,
        kMaxProvisionedNetworks = 10
    };

    NetworkInfo ScanResults[kMaxScanResults];
    NetworkInfo ProvisionedNetworks[kMaxProvisionedNetworks];
    uint32_t NextNetworkId;

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    void Reset();

    void Preconfig();

protected:
    union
    {
        uint8_t networkType;
        PacketBuffer *networkInfoTLV;
        uint32_t networkId;
        uint8_t flags;
        uint16_t rendezvousMode;
    } mOpArgs;

    virtual WEAVE_ERROR HandleScanNetworks(uint8_t networkType);
    virtual WEAVE_ERROR HandleAddNetwork(PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleUpdateNetwork(PacketBuffer *networkInfoTLV);
    virtual WEAVE_ERROR HandleRemoveNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleGetNetworks(uint8_t flags);
    virtual WEAVE_ERROR HandleEnableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleDisableNetwork(uint32_t networkId);
    virtual WEAVE_ERROR HandleTestConnectivity(uint32_t networkId);
    virtual WEAVE_ERROR HandleSetRendezvousMode(uint16_t rendezvousMode);
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
    virtual bool IsPairedToAccount() const;

    void CompleteOrDelayCurrentOp(const char *opName);
    void CompleteCurrentOp();

    WEAVE_ERROR CompleteScanNetworks(uint8_t networkType);
    WEAVE_ERROR CompleteAddNetwork(PacketBuffer *networkInfoTLV);
    WEAVE_ERROR CompleteUpdateNetwork(PacketBuffer *networkInfoTLV);
    WEAVE_ERROR CompleteRemoveNetwork(uint32_t networkId);
    WEAVE_ERROR CompleteGetNetworks(uint8_t flags);
    WEAVE_ERROR CompleteEnableNetwork(uint32_t networkId);
    WEAVE_ERROR CompleteDisableNetwork(uint32_t networkId);
    WEAVE_ERROR CompleteTestConnectivity(uint32_t networkId);
    WEAVE_ERROR CompleteSetRendezvousMode(uint16_t rendezvousMode);

    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    WEAVE_ERROR ValidateNetworkConfig(NetworkInfo& netConfig);
    static void PrintNetworkInfo(NetworkInfo& netInfo, const char *prefix);
    static void HandleOpDelayComplete(nl::Weave::System::Layer* lSystemLayer, void* aAppState, nl::Weave::System::Error aError);
};

#endif /* MOCKNPSERVER_H_ */

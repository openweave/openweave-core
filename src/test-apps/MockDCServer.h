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
 *      (i.e., server) for the Weave Device Control profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKDCSERVER_H_
#define MOCKDCSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkInfo.h>

using nl::Inet::IPAddress;
using nl::Weave::Profiles::NetworkProvisioning::NetworkInfo;
using nl::Weave::WeaveExchangeManager;
using nl::Weave::Profiles::DeviceControl::DeviceControlServer;
using nl::Weave::Profiles::DeviceControl::DeviceControlDelegate;

class MockDeviceControlServer: private DeviceControlServer, private DeviceControlDelegate
{
public:
    MockDeviceControlServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

protected:
    virtual bool ShouldCloseConBeforeResetConfig(uint16_t resetFlags);
    virtual WEAVE_ERROR OnResetConfig(uint16_t resetFlags);
    virtual WEAVE_ERROR OnFailSafeArmed();
    virtual WEAVE_ERROR OnFailSafeDisarmed();
    virtual void OnRemotePassiveRendezvousStarted();
    virtual void OnRemotePassiveRendezvousDone();
    virtual void OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr);
    virtual WEAVE_ERROR WillStartRemotePassiveRendezvous();
    virtual void WillCloseRemotePassiveRendezvous();
    virtual bool IsResetAllowed(uint16_t resetFlags);
    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);
    virtual WEAVE_ERROR OnSystemTestStarted(uint32_t profileId, uint32_t testId);
    virtual WEAVE_ERROR OnSystemTestStopped();
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
    virtual bool IsPairedToAccount() const;
};

#endif /* MOCKDCSERVER_H_ */

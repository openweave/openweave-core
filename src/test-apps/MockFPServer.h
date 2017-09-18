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
 *      (i.e., server) for the Weave Fabric Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKFPSERVER_H_
#define MOCKFPSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::FabricProvisioning;

class MockFabricProvisioningServer : private FabricProvisioningServer, private FabricProvisioningDelegate
{
public:
    MockFabricProvisioningServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    void Preconfig();

    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError);

protected:
    virtual WEAVE_ERROR HandleCreateFabric();
    virtual WEAVE_ERROR HandleJoinExistingFabric();
    virtual WEAVE_ERROR HandleLeaveFabric();
    virtual WEAVE_ERROR HandleGetFabricConfig();
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
    virtual bool IsPairedToAccount() const;
};

#endif /* MOCKFPSERVER_H_ */

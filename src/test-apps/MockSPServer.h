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
 *      (i.e., server) for the Weave Service Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKSPSERVER_H_
#define MOCKSPSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::ServiceProvisioning;

enum
{
    kPairingTransport_TCP = 0,
    kPairingTransport_WRM = 1
};

class MockServiceProvisioningServer: private ServiceProvisioningServer, private ServiceProvisioningDelegate
{
public:
    MockServiceProvisioningServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    void Reset();
    void Preconfig();

    uint64_t PairingEndPointId;
    const char *PairingServerAddr;
    int PairingTransport;

protected:
    WEAVE_ERROR HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg);
    WEAVE_ERROR HandleUpdateService(UpdateServiceMessage& msg);
    WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);
#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
    void HandleIFJServiceFabricJoinResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
    virtual bool IsPairedToAccount() const;

private:
    uint64_t mPersistedServiceId;
    char *mPersistedAccountId;
    uint8_t *mPersistedServiceConfig;
    uint16_t mPersistedServiceConfigLen;
    nl::Weave::WeaveConnection *mPairingServerCon;
    nl::Weave::Binding *mPairingServerBinding;

    WEAVE_ERROR PersistNewService(uint64_t serviceId,
                                  const char *accountId, uint16_t accountIdLen,
                                  const uint8_t *serviceConfig, uint16_t serviceConfigLen);
    WEAVE_ERROR UpdatedPersistedService(const uint8_t *serviceConfig, uint16_t serviceConfigLen);
    void ClearPersistedService();

    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    WEAVE_ERROR StartConnectToPairingServer();
    static void HandlePairingServerConnectionComplete(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandlePairingServerConnectionClosed(nl::Weave::WeaveConnection *con, WEAVE_ERROR conErr);

    WEAVE_ERROR PrepareBindingForPairingServer();
    static void HandlePairingServerBindingEvent(void *const appState, const nl::Weave::Binding::EventType event,
                                                const nl::Weave::Binding::InEventParam &inParam,
                                                nl::Weave::Binding::OutEventParam &outParam);
};

#endif /* MOCKSPSERVER_H_ */

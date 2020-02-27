/*
 *
 *    Copyright (c) 2020 Google LLC.
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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Device Control profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>

#include "MockDCServer.h"
#include "MockNPServer.h"
#include "MockSPServer.h"
#include "MockCPClient.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include "ToolCommon.h"
#include "CASEOptions.h"

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DeviceControl;

extern MockNetworkProvisioningServer MockNPServer;
extern MockServiceProvisioningServer MockSPServer;
extern MockCertificateProvisioningClient MockCPClient;

MockDeviceControlServer::MockDeviceControlServer()
{
}

WEAVE_ERROR MockDeviceControlServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    err = this->DeviceControlServer::Init(exchangeMgr);
    SuccessOrExit(err);

    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR MockDeviceControlServer::Shutdown()
{
    return this->DeviceControlServer::Shutdown();
}

bool
MockDeviceControlServer::ShouldCloseConBeforeResetConfig(uint16_t resetFlags)
{
    return false;
}

WEAVE_ERROR MockDeviceControlServer::OnResetConfig(uint16_t resetFlags)
{
    printf("Resetting configuration...\n");

    if (resetFlags & kResetConfigFlag_ServiceConfig || resetFlags & kResetConfigFlag_OperationalCredentials)
    {
        printf("  Resetting service configuration\n");
        MockSPServer.Reset();
    }

    if (resetFlags & kResetConfigFlag_FabricConfig)
    {
        printf("  Resetting fabric configuration\n");
        FabricState->ClearFabricState();
    }

    if (resetFlags & kResetConfigFlag_NetworkConfig)
    {
        printf("  Resetting network configuration\n");
        MockNPServer.Reset();
    }

    if (resetFlags & kResetConfigFlag_OperationalCredentials)
    {
        printf("  Resetting operational device credentials\n");
        MockCPClient.Reset();
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockDeviceControlServer::OnFailSafeArmed()
{
    printf("Fail-safe armed\n");
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockDeviceControlServer::OnFailSafeDisarmed()
{
    printf("Fail-safe disarmed\n");
    return WEAVE_NO_ERROR;
}

void MockDeviceControlServer::OnRemotePassiveRendezvousStarted()
{
    printf("Remote Passive Rendezvous started\n");
}

void MockDeviceControlServer::OnRemotePassiveRendezvousDone()
{
    printf("Remote Passive Rendezvous done\n");
}

void MockDeviceControlServer::OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr)
{
    char ipAddrStr[64];
    peerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("Connection monitor timeout: node %" PRIX64 " (%s)\n", peerNodeId, ipAddrStr);
}

WEAVE_ERROR MockDeviceControlServer::WillStartRemotePassiveRendezvous()
{
    printf("Will start Remote Passive Rendezvous.\n");

    return WEAVE_NO_ERROR;
}

void MockDeviceControlServer::WillCloseRemotePassiveRendezvous()
{
    printf("Will close Remote Passive Rendezvous.\n");
}

bool MockDeviceControlServer::IsResetAllowed(uint16_t resetFlags)
{
    return true;
}

WEAVE_ERROR MockDeviceControlServer::OnSystemTestStarted(uint32_t profileId, uint32_t testId)
{
    if (testId % 2)
    {
        printf("System test started successfully: (0x%.8X, 0x%.8X)\n", profileId, testId);
        SendSuccessResponse();
    }
    else
    {
        printf("System test failed to start: (0x%.8X, 0x%.8X)\n", profileId, testId);
        SendStatusReport(profileId, 0xFFFF);
    }
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockDeviceControlServer::OnSystemTestStopped()
{
    printf("System test stopped\n");

    SendSuccessResponse();

    return WEAVE_NO_ERROR;
}

void MockDeviceControlServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    DeviceControlDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

bool MockDeviceControlServer::IsPairedToAccount() const
{
    return (gCASEOptions.ServiceConfig != NULL);
}

WEAVE_ERROR MockDeviceControlServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    if (statusProfileId == kWeaveProfile_Common && statusCode == Common::kStatus_Success)
        printf("Sending StatusReport: Success\n");
    else if (sysError == WEAVE_NO_ERROR)
        printf("Sending StatusReport: Status code = %u, Status profile = %lu\n", statusCode, (unsigned long)statusProfileId);
    else
        printf("Sending StatusReport: Status code = %u, Status profile = %lu, System error = %d\n", statusCode, (unsigned long)statusProfileId, sysError);
    return this->DeviceControlServer::SendStatusReport(statusProfileId, statusCode, sysError);
}

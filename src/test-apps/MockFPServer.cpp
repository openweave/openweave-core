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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Fabric Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#include "stdio.h"

#include "ToolCommon.h"
#include "MockFPServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/ErrorStr.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::FabricProvisioning;


MockFabricProvisioningServer::MockFabricProvisioningServer()
{
}

WEAVE_ERROR MockFabricProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    // Initialize the base class.
    err = this->FabricProvisioningServer::Init(exchangeMgr);
    SuccessOrExit(err);

    // Tell the base class that it should delegate service provisioning requests to us.
    SetDelegate(this);

exit:
    return err;
}

void MockFabricProvisioningServer::Preconfig()
{
    FabricState->ClearFabricState();
    FabricState->CreateFabric();

    // If a fabric id was specified on the command line, use that.
    if (gWeaveNodeOptions.FabricId != 0)
        FabricState->FabricId = gWeaveNodeOptions.FabricId;
}

WEAVE_ERROR MockFabricProvisioningServer::Shutdown()
{
    return this->FabricProvisioningServer::Shutdown();
}

WEAVE_ERROR MockFabricProvisioningServer::HandleCreateFabric()
{
    printf("Weave fabric created (fabric id %llX)\n", (unsigned long long)FabricState->FabricId);
    return SendSuccessResponse();
}

WEAVE_ERROR MockFabricProvisioningServer::HandleJoinExistingFabric()
{
    printf("Joined existing Weave fabric (fabric id %llX)\n", (unsigned long long)FabricState->FabricId);
    return SendSuccessResponse();
}

WEAVE_ERROR MockFabricProvisioningServer::HandleLeaveFabric()
{
    printf("LeaveFabric complete\n");
    return SendSuccessResponse();
}

WEAVE_ERROR MockFabricProvisioningServer::HandleGetFabricConfig()
{
    printf("GetFabricConfig complete\n");
    return WEAVE_NO_ERROR;
}

void MockFabricProvisioningServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    FabricProvisioningDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

bool MockFabricProvisioningServer::IsPairedToAccount() const
{
    return (gCASEOptions.ServiceConfig != NULL);
}

WEAVE_ERROR MockFabricProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    if (statusProfileId == kWeaveProfile_Common && statusCode == Common::kStatus_Success)
        printf("Sending StatusReport: Success\n");
    else
    {
        printf("Sending StatusReport: %s\n", nl::StatusReportStr(statusProfileId, statusCode));
        if (sysError != WEAVE_NO_ERROR)
            printf("   System error: %s\n", nl::ErrorStr(sysError));
    }
    return this->FabricProvisioningServer::SendStatusReport(statusProfileId, statusCode, sysError);
}

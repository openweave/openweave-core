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
 *      (i.e., server) for the Weave Device Description profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>

#include "ToolCommon.h"
#include "MockDDServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/ErrorStr.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DeviceDescription;

MockDeviceDescriptionServer::MockDeviceDescriptionServer()
{
}

WEAVE_ERROR MockDeviceDescriptionServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    // Initialize the base class.
    err = this->DeviceDescriptionServer::Init(exchangeMgr);
    SuccessOrExit(err);

    AppState = this;
    OnIdentifyRequestReceived = HandleIdentifyRequest;

exit:
    return err;
}


WEAVE_ERROR MockDeviceDescriptionServer::Shutdown()
{
    WEAVE_ERROR err;

    err = this->DeviceDescriptionServer::Shutdown();
    return err;
}

void MockDeviceDescriptionServer::HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg)
{
    MockDeviceDescriptionServer *server = (MockDeviceDescriptionServer *)appState;
    WeaveDeviceDescriptor deviceDesc;
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("IdentifyRequest received from node %" PRIX64 " (%s)\n", nodeId, ipAddrStr);
    printf("  Target Fabric Id: %016" PRIX64 "\n", reqMsg.TargetFabricId);
    printf("  Target Modes: %08lX\n", (unsigned long)reqMsg.TargetModes);
    printf("  Target Vendor Id: %04X\n", (unsigned)reqMsg.TargetVendorId);
    printf("  Target Product Id: %04X\n", (unsigned)reqMsg.TargetProductId);

    sendResp = true;

    gDeviceDescOptions.GetDeviceDesc(deviceDesc);

    if (!MatchTargetFabricId(server->FabricState->FabricId, reqMsg.TargetFabricId))
        sendResp = false;

    if (reqMsg.TargetModes != kTargetDeviceMode_Any && (reqMsg.TargetModes & kTargetDeviceMode_UserSelectedMode) == 0)
        sendResp = false;

    if (reqMsg.TargetVendorId != 0xFFFF && reqMsg.TargetVendorId != deviceDesc.VendorId)
        sendResp = false;

    if (reqMsg.TargetProductId != 0xFFFF && reqMsg.TargetProductId != deviceDesc.ProductId)
        sendResp = false;

    if (sendResp)
    {
        respMsg.DeviceDesc = deviceDesc;
        printf("Sending IdentifyResponse\n");
    }
    else
        printf("Ignoring IdentifyResponse\n");
}

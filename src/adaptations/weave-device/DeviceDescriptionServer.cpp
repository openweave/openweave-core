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

#include <internal/WeaveDeviceInternal.h>
#include <internal/DeviceDescriptionServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::DeviceDescription;

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

DeviceDescriptionServer DeviceDescriptionSrv;

WEAVE_ERROR DeviceDescriptionServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::Device::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the HandleIdentifyRequest function.
    OnIdentifyRequestReceived = HandleIdentifyRequest;

exit:
    return err;
}

void DeviceDescriptionServer::HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
        const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceDescriptor deviceDesc;

#if LOG_LOCAL_LEVEL >= ESP_LOG_INFO
    {
        char ipAddrStr[64];
        nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        ESP_LOGI(TAG, "IdentifyRequest received from node %" PRIX64 " (%s)", nodeId, ipAddrStr);
        ESP_LOGI(TAG, "  Target Fabric Id: %016" PRIX64, reqMsg.TargetFabricId);
        ESP_LOGI(TAG, "  Target Modes: %08" PRIX32, reqMsg.TargetModes);
        ESP_LOGI(TAG, "  Target Vendor Id: %04" PRIX16, reqMsg.TargetVendorId);
        ESP_LOGI(TAG, "  Target Product Id: %04" PRIX16, reqMsg.TargetProductId);
    }
#endif // LOG_LOCAL_LEVEL >= ESP_LOG_ERROR

    sendResp = true;

    if (!MatchTargetFabricId(::nl::Weave::Device::FabricState.FabricId, reqMsg.TargetFabricId))
    {
        ESP_LOGI(TAG, "IdentifyRequest target fabric does not match device fabric");
        sendResp = false;
    }

    if (reqMsg.TargetModes != kTargetDeviceMode_Any && (reqMsg.TargetModes & kTargetDeviceMode_UserSelectedMode) == 0)
    {
        ESP_LOGI(TAG, "IdentifyRequest target mode does not match device mode");
        sendResp = false;
    }

    if (reqMsg.TargetVendorId != 0xFFFF)
    {
        uint16_t vendorId;

        err = ConfigurationMgr.GetVendorId(vendorId);
        SuccessOrExit(err);

        if (reqMsg.TargetVendorId != vendorId)
        {
            ESP_LOGI(TAG, "IdentifyRequest target vendor does not match device vendor");
            sendResp = false;
        }
    }

    if (reqMsg.TargetProductId != 0xFFFF)
    {
        uint16_t productId;

        err = ConfigurationMgr.GetProductId(productId);
        SuccessOrExit(err);

        if (reqMsg.TargetProductId != productId)
        {
            ESP_LOGI(TAG, "IdentifyRequest target product does not match device product");
            sendResp = false;
        }
    }

    if (sendResp)
    {
        ESP_LOGI(TAG, "Sending IdentifyResponse");
    }

exit:

    if (err == WEAVE_NO_ERROR && sendResp)
    {
        err = ConfigurationMgr.GetDeviceDescriptor(respMsg.DeviceDesc);
    }

    if (err != WEAVE_NO_ERROR)
    {
        sendResp = false;
    }
}

void DeviceDescriptionServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // Nothing to do so far.
}


} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

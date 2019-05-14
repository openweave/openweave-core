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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/DeviceDescriptionServer.h>

#include <Weave/Support/TimeUtils.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::DeviceDescription;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

DeviceDescriptionServer DeviceDescriptionServer::sInstance;

WEAVE_ERROR DeviceDescriptionServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::DeviceLayer::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the HandleIdentifyRequest function.
    OnIdentifyRequestReceived = HandleIdentifyRequest;

    // Initialize various members.
    mUserSelectedModeEndTime = 0;
    mUserSelectedModeTimeoutSec = WEAVE_DEVICE_CONFIG_USER_SELECTED_MODE_TIMEOUT_SEC;

exit:
    return err;
}

bool DeviceDescriptionServer::IsUserSelectedModeActive(void)
{
    if (mUserSelectedModeEndTime != 0)
    {
        uint32_t nowShifted = static_cast<uint32_t>(System::Platform::Layer::GetClock_MonotonicMS() >> kUserSelectedModeTimeShift);
        return nowShifted <= mUserSelectedModeEndTime;
    }
    else
    {
        return false;
    }
}

void DeviceDescriptionServer::SetUserSelectedMode(bool val)
{
    if (val)
    {
        WeaveLogProgress(DeviceLayer, "User selected mode %s (timeout %" PRId16 " seconds)",
                IsUserSelectedModeActive() ? "extended" : "activated",
                mUserSelectedModeTimeoutSec);

        uint32_t timeoutMS = mUserSelectedModeTimeoutSec * kMillisecondPerSecond;
        uint64_t endTimeMS = System::Platform::Layer::GetClock_MonotonicMS() + timeoutMS;
        mUserSelectedModeEndTime = static_cast<uint32_t>(endTimeMS >> kUserSelectedModeTimeShift);
    }
    else
    {
        WeaveLogProgress(DeviceLayer, "User selected mode deactivated");

        mUserSelectedModeEndTime = 0;
    }
}

uint16_t DeviceDescriptionServer::GetUserSelectedModeTimeout(void)
{
    return mUserSelectedModeTimeoutSec;
}

void DeviceDescriptionServer::SetUserSelectedModeTimeout(uint16_t val)
{
    mUserSelectedModeTimeoutSec = val;
}

void DeviceDescriptionServer::HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
        const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceDescriptor deviceDesc;

#if WEAVE_PROGRESS_LOGGING
    {
        char ipAddrStr[64];
        nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        WeaveLogProgress(DeviceLayer, "IdentifyRequest received from node %" PRIX64 " (%s)", nodeId, ipAddrStr);
        WeaveLogProgress(DeviceLayer, "  Target Fabric Id: %016" PRIX64, reqMsg.TargetFabricId);
        WeaveLogProgress(DeviceLayer, "  Target Modes: %08" PRIX32, reqMsg.TargetModes);
        WeaveLogProgress(DeviceLayer, "  Target Vendor Id: %04" PRIX16, reqMsg.TargetVendorId);
        WeaveLogProgress(DeviceLayer, "  Target Product Id: %04" PRIX16, reqMsg.TargetProductId);
    }
#endif // WEAVE_PROGRESS_LOGGING

    sendResp = true;

    if (!MatchTargetFabricId(::nl::Weave::DeviceLayer::FabricState.FabricId, reqMsg.TargetFabricId))
    {
        WeaveLogProgress(DeviceLayer, "IdentifyRequest target fabric does not match device fabric");
        sendResp = false;
    }

    if ((reqMsg.TargetModes & ~kTargetDeviceMode_UserSelectedMode) != 0 ||
        ((reqMsg.TargetModes & kTargetDeviceMode_UserSelectedMode) != 0 && !sInstance.IsUserSelectedModeActive()))
    {
        WeaveLogProgress(DeviceLayer, "IdentifyRequest target mode does not match device mode");
        sendResp = false;
    }

    if (reqMsg.TargetVendorId != 0xFFFF)
    {
        uint16_t vendorId;

        err = ConfigurationMgr().GetVendorId(vendorId);
        SuccessOrExit(err);

        if (reqMsg.TargetVendorId != vendorId)
        {
            WeaveLogProgress(DeviceLayer, "IdentifyRequest target vendor does not match device vendor");
            sendResp = false;
        }
    }

    if (reqMsg.TargetProductId != 0xFFFF)
    {
        uint16_t productId;

        err = ConfigurationMgr().GetProductId(productId);
        SuccessOrExit(err);

        if (reqMsg.TargetProductId != productId)
        {
            WeaveLogProgress(DeviceLayer, "IdentifyRequest target product does not match device product");
            sendResp = false;
        }
    }

    if (sendResp)
    {
        WeaveLogProgress(DeviceLayer, "Sending IdentifyResponse");
    }

exit:

    if (err == WEAVE_NO_ERROR && sendResp)
    {
        err = ConfigurationMgr().GetDeviceDescriptor(respMsg.DeviceDesc);
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceLayer, "HandleIdentifyRequest failed: %s", ErrorStr(err));
        sendResp = false;
    }
}

void DeviceDescriptionServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // Nothing to do so far.
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#include <internal/WeavePlatformInternal.h>
#include <internal/DeviceDescriptionServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::DeviceDescription;

namespace WeavePlatform {
namespace Internal {

DeviceDescriptionServer DeviceDescriptionSrv;

WEAVE_ERROR DeviceDescriptionServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::WeavePlatform::ExchangeMgr);
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

#if LOG_LOCAL_LEVEL >= ESP_LOG_ERROR
    {
        char ipAddrStr[64];
        nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        ESP_LOGD(TAG, "IdentifyRequest received from node %" PRIX64 " (%s)", nodeId, ipAddrStr);
        ESP_LOGD(TAG, "  Target Fabric Id: %016" PRIX64, reqMsg.TargetFabricId);
        ESP_LOGD(TAG, "  Target Modes: %08" PRIX32, reqMsg.TargetModes);
        ESP_LOGD(TAG, "  Target Vendor Id: %04" PRIX16, reqMsg.TargetVendorId);
        ESP_LOGD(TAG, "  Target Product Id: %04" PRIX16, reqMsg.TargetProductId);
    }
#endif // LOG_LOCAL_LEVEL >= ESP_LOG_ERROR

    sendResp = true;

    if (!MatchTargetFabricId(::WeavePlatform::FabricState.FabricId, reqMsg.TargetFabricId))
        sendResp = false;

    if (reqMsg.TargetModes != kTargetDeviceMode_Any && (reqMsg.TargetModes & kTargetDeviceMode_UserSelectedMode) == 0)
        sendResp = false;

    if (reqMsg.TargetVendorId != 0xFFFF)
    {
        uint16_t vendorId;

        err = ConfigurationMgr.GetVendorId(vendorId);
        SuccessOrExit(err);

        if (reqMsg.TargetVendorId != vendorId)
        {
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
            sendResp = false;
        }
    }

    if (sendResp)
    {
        ESP_LOGD(TAG, "Sending IdentifyResponse");
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

void DeviceDescriptionServer::OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event)
{
    // Nothing to do so far.
}


} // namespace Internal
} // namespace WeavePlatform

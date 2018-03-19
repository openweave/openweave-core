#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/echo/Next/WeaveEchoServer.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Echo_Next;
using namespace ::nl::Weave::Profiles::DeviceDescription;

static WeaveEchoServer gWeaveEchoServer;
static DeviceDescriptionServer gDeviceDescriptionServer;


static void HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
        const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg);

bool InitWeaveServers()
{
    WEAVE_ERROR err;

    // Initialize the Weave echo server.
    err = gWeaveEchoServer.Init(&ExchangeMgr, WeaveEchoServer::DefaultEventHandler);
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "Weave Echo server initialization failed: %s", ErrorStr(err));
        return false;
    }
    ESP_LOGI(TAG, "Weave Echo server initialized");

    // Initialize the Weave device description server.
    err = gDeviceDescriptionServer.Init(&ExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        ESP_LOGE(TAG, "Weave Device Description server initialization failed: %s", ErrorStr(err));
        return false;
    }
    gDeviceDescriptionServer.OnIdentifyRequestReceived = HandleIdentifyRequest;
    ESP_LOGI(TAG, "Weave Device Description server initialized");

    // Initialize the Fabric Provisioning server.
    if (!InitFabricProvisioningServer())
    {
        return false;
    }

    return true;
}

static void HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
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

    if (!MatchTargetFabricId(FabricState.FabricId, reqMsg.TargetFabricId))
        sendResp = false;

    if (reqMsg.TargetModes != kTargetDeviceMode_Any && (reqMsg.TargetModes & kTargetDeviceMode_UserSelectedMode) == 0)
        sendResp = false;

    if (reqMsg.TargetVendorId != 0xFFFF)
    {
        uint16_t vendorId;

        err = ConfigMgr.GetVendorId(vendorId);
        SuccessOrExit(err);

        if (reqMsg.TargetVendorId != vendorId)
        {
            sendResp = false;
        }
    }

    if (reqMsg.TargetProductId != 0xFFFF)
    {
        uint16_t productId;

        err = ConfigMgr.GetProductId(productId);
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
        err = ConfigMgr.GetDeviceDescriptor(respMsg.DeviceDesc);
    }

    if (err != WEAVE_NO_ERROR)
    {
        sendResp = false;
    }
}


} // namespace Internal
} // namespace WeavePlatform

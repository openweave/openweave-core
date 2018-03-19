#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

#include <new>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::FabricProvisioning;

class FabricProvisioningDelegate
        : public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningDelegate
{
public:
    virtual WEAVE_ERROR HandleCreateFabric(void);
    virtual WEAVE_ERROR HandleJoinExistingFabric(void);
    virtual WEAVE_ERROR HandleLeaveFabric(void);
    virtual WEAVE_ERROR HandleGetFabricConfig(void);
    virtual bool IsPairedToAccount() const;
};

static FabricProvisioningServer gFabricProvisioningServer;
static FabricProvisioningDelegate gFabricProvisioningDelegate;

bool InitFabricProvisioningServer()
{
    WEAVE_ERROR err;

    new (&gFabricProvisioningServer) FabricProvisioningServer();

    err = gFabricProvisioningServer.Init(&ExchangeMgr);
    SuccessOrExit(err);

    new (&gFabricProvisioningDelegate) FabricProvisioningDelegate();

    gFabricProvisioningServer.SetDelegate(&gFabricProvisioningDelegate);

exit:
    if (err == WEAVE_NO_ERROR)
    {
        ESP_LOGI(TAG, "Weave Fabric Provisioning server initialized");
    }
    else
    {
        ESP_LOGE(TAG, "Weave Fabric Provisioning server initialization failed: %s", ErrorStr(err));
    }
    return (err == WEAVE_NO_ERROR);
}

WEAVE_ERROR FabricProvisioningDelegate::HandleCreateFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(FabricState.FabricId);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Weave fabric created; fabric id %016" PRIX64, FabricState.FabricId);

    err = gFabricProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningDelegate::HandleJoinExistingFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(FabricState.FabricId);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Join existing Weave fabric; fabric id %016" PRIX64, FabricState.FabricId);

    err = gFabricProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningDelegate::HandleLeaveFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(kFabricIdNotSpecified);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Leave Weave fabric");

    err = gFabricProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningDelegate::HandleGetFabricConfig(void)
{
    // Nothing to do
    return WEAVE_NO_ERROR;
}

bool FabricProvisioningDelegate::IsPairedToAccount() const
{
    return ConfigMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform

#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

#include <new>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::FabricProvisioning;

class FabricProvisioningServer
        : public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer,
          public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    virtual WEAVE_ERROR HandleCreateFabric(void);
    virtual WEAVE_ERROR HandleJoinExistingFabric(void);
    virtual WEAVE_ERROR HandleLeaveFabric(void);
    virtual WEAVE_ERROR HandleGetFabricConfig(void);
    virtual bool IsPairedToAccount() const;
};

static FabricProvisioningServer gFabricProvisioningServer;

bool InitFabricProvisioningServer()
{
    WEAVE_ERROR err;

    new (&gFabricProvisioningServer) FabricProvisioningServer();

    err = gFabricProvisioningServer.Init(&ExchangeMgr);
    SuccessOrExit(err);

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

WEAVE_ERROR FabricProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(exchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleCreateFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(::WeavePlatform::FabricState.FabricId);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Weave fabric created; fabric id %016" PRIX64, ::WeavePlatform::FabricState.FabricId);

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleJoinExistingFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(::WeavePlatform::FabricState.FabricId);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Join existing Weave fabric; fabric id %016" PRIX64, ::WeavePlatform::FabricState.FabricId);

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleLeaveFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigMgr.StoreFabricId(kFabricIdNotSpecified);
    SuccessOrExit(err);

    ESP_LOGI(TAG, "Leave Weave fabric");

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleGetFabricConfig(void)
{
    // Nothing to do
    return WEAVE_NO_ERROR;
}

bool FabricProvisioningServer::IsPairedToAccount() const
{
    return ConfigMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform

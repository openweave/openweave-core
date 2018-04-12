#include <internal/WeavePlatformInternal.h>
#include <internal/FabricProvisioningServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::FabricProvisioning;

namespace WeavePlatform {
namespace Internal {

WEAVE_ERROR FabricProvisioningServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::WeavePlatform::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR FabricProvisioningServer::HandleCreateFabric(void)
{
    WEAVE_ERROR err;

    err = ConfigurationMgr.StoreFabricId(::WeavePlatform::FabricState.FabricId);
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

    err = ConfigurationMgr.StoreFabricId(::WeavePlatform::FabricState.FabricId);
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

    err = ConfigurationMgr.StoreFabricId(kFabricIdNotSpecified);
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
    return ConfigurationMgr.IsServiceProvisioned();
}

void FabricProvisioningServer::OnPlatformEvent(const WeavePlatformEvent * event)
{
    // Nothing to do so far.
}


} // namespace Internal
} // namespace WeavePlatform

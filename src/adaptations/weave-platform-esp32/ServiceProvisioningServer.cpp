#include <internal/WeavePlatformInternal.h>
#include <internal/ServiceProvisioningServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::ServiceProvisioning;

namespace WeavePlatform {
namespace Internal {

WEAVE_ERROR ServiceProvisioningServer::Init()
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

WEAVE_ERROR ServiceProvisioningServer::HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned. If so respond with "Too Many Services".
    err = ConfigurationMgr.GetServiceId(curServiceId);
    if (err == WEAVE_NO_ERROR)
    {
        err = ServiceProvisioningSvr.SendStatusReport(kWeaveProfile_ServiceProvisioning,
                (curServiceId == msg.ServiceId) ? kStatusCode_ServiceAlreadyRegistered : kStatusCode_TooManyServices);
        ExitNow();
    }
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        err = ServiceProvisioningSvr.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // TODO: Send PairDeviceToAccount request to service

    err = ConfigurationMgr.StoreServiceProvisioningData(msg.ServiceId, msg.ServiceConfig, msg.ServiceConfigLen, msg.AccountId, msg.AccountIdLen);
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = ServiceProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUpdateService(UpdateServiceMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigurationMgr.GetServiceId(curServiceId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND || curServiceId != msg.ServiceId)
    {
        err = ServiceProvisioningSvr.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        err = ServiceProvisioningSvr.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // Save the new service configuration in device persistent storage, replacing the existing value.
    err = ConfigurationMgr.StoreServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen);
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = ServiceProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUnregisterService(uint64_t serviceId)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigurationMgr.GetServiceId(curServiceId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND || curServiceId != serviceId)
    {
        err = ServiceProvisioningSvr.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Clear the persisted service.
    err = ConfigurationMgr.ClearServiceProvisioningData();
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = ServiceProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

void ServiceProvisioningServer::HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode)
{
    // TODO: implement this
}

bool ServiceProvisioningServer::IsPairedToAccount() const
{
    return ConfigurationMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform


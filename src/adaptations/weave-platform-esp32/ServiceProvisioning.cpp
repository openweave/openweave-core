#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

#include <new>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::ServiceProvisioning;

class ServiceProvisioningDelegate
        : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
public:
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg);
    virtual WEAVE_ERROR HandleUpdateService(UpdateServiceMessage& msg);
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);
    virtual bool IsPairedToAccount() const;
};

static ServiceProvisioningServer gServiceProvisioningServer;
static ServiceProvisioningDelegate gServiceProvisioningDelegate;

bool InitServiceProvisioningServer()
{
    WEAVE_ERROR err;

    new (&gServiceProvisioningServer) ServiceProvisioningServer();

    err = gServiceProvisioningServer.Init(&ExchangeMgr);
    SuccessOrExit(err);

    new (&gServiceProvisioningDelegate) ServiceProvisioningDelegate();

    gServiceProvisioningServer.SetDelegate(&gServiceProvisioningDelegate);

exit:
    if (err == WEAVE_NO_ERROR)
    {
        ESP_LOGI(TAG, "Weave Service Provisioning server initialized");
    }
    else
    {
        ESP_LOGE(TAG, "Weave Service Provisioning server initialization failed: %s", ErrorStr(err));
    }
    return (err == WEAVE_NO_ERROR);
}

WEAVE_ERROR ServiceProvisioningDelegate::HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned.
    err = ConfigMgr.GetServiceId(curServiceId);
    SuccessOrExit(err);

    // If so respond with an appropriate error.
    if (curServiceId != 0)
    {
        // Send
        gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning,
                (curServiceId == msg.ServiceId) ? kStatusCode_ServiceAlreadyRegistered : kStatusCode_TooManyServices);
        ExitNow();
    }

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // TODO: Send PairDeviceToAccount request to service

    err = ConfigMgr.StoreServiceProvisioningData(msg.ServiceId, msg.ServiceConfig, msg.ServiceConfigLen, msg.AccountId, msg.AccountIdLen);
    SuccessOrExit(err);

    // Send a success StatusReport back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningDelegate::HandleUpdateService(UpdateServiceMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned.
    err = ConfigMgr.GetServiceId(curServiceId);
    SuccessOrExit(err);

    // Verify that the service id matches the existing service.
    //
    if (curServiceId != msg.ServiceId)
    {
        gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // Save the new service configuration in device persistent storage, replacing the existing value.
    err = ConfigMgr.StoreServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen);
    SuccessOrExit(err);

    // Send a success StatusReport back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningDelegate::HandleUnregisterService(uint64_t serviceId)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned.
    err = ConfigMgr.GetServiceId(curServiceId);
    SuccessOrExit(err);

    // Verify that the service id matches the existing service.
    //
    if (curServiceId != serviceId)
    {
        gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }

    // Clear the persisted service.
    err = ConfigMgr.ClearServiceProvisioningData();
    SuccessOrExit(err);

    // Send a success StatusReport back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

void ServiceProvisioningDelegate::HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode)
{
    // TODO: implement this
}

bool ServiceProvisioningDelegate::IsPairedToAccount() const
{
    return ConfigMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform


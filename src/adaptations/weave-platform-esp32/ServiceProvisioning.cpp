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

class ServiceProvisioningServer
        : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer,
          public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg);
    virtual WEAVE_ERROR HandleUpdateService(UpdateServiceMessage& msg);
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);
    virtual bool IsPairedToAccount() const;
};

static ServiceProvisioningServer gServiceProvisioningServer;

bool InitServiceProvisioningServer()
{
    WEAVE_ERROR err;

    new (&gServiceProvisioningServer) ServiceProvisioningServer();

    err = gServiceProvisioningServer.Init(&ExchangeMgr);
    SuccessOrExit(err);

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

WEAVE_ERROR ServiceProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
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

WEAVE_ERROR ServiceProvisioningServer::HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned. If so respond with "Too Many Services".
    err = ConfigMgr.GetServiceId(curServiceId);
    if (err == WEAVE_NO_ERROR)
    {
        err = gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning,
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
        err = gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // TODO: Send PairDeviceToAccount request to service

    err = ConfigMgr.StoreServiceProvisioningData(msg.ServiceId, msg.ServiceConfig, msg.ServiceConfigLen, msg.AccountId, msg.AccountIdLen);
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUpdateService(UpdateServiceMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigMgr.GetServiceId(curServiceId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND || curServiceId != msg.ServiceId)
    {
        err = gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        err = gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // Save the new service configuration in device persistent storage, replacing the existing value.
    err = ConfigMgr.StoreServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen);
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUnregisterService(uint64_t serviceId)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigMgr.GetServiceId(curServiceId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND || curServiceId != serviceId)
    {
        err = gServiceProvisioningServer.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Clear the persisted service.
    err = ConfigMgr.ClearServiceProvisioningData();
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = gServiceProvisioningServer.SendSuccessResponse();
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
    return ConfigMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform


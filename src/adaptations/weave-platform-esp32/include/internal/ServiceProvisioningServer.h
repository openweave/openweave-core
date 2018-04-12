#ifndef SERVICE_PROVISIONING_SERVER_H
#define SERVICE_PROVISIONING_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

namespace WeavePlatform {
namespace Internal {

class ServiceProvisioningServer
        : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer,
          public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(::nl::Weave::Profiles::ServiceProvisioning::RegisterServicePairAccountMessage& msg);
    virtual WEAVE_ERROR HandleUpdateService(::nl::Weave::Profiles::ServiceProvisioning::UpdateServiceMessage& msg);
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);
    virtual bool IsPairedToAccount() const;

    void OnPlatformEvent(const WeavePlatformEvent * event);
};

extern ServiceProvisioningServer ServiceProvisioningSvr;

} // namespace Internal
} // namespace WeavePlatform

#endif // SERVICE_PROVISIONING_SERVER_H

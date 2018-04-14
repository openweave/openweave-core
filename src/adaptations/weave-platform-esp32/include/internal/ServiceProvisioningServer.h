#ifndef SERVICE_PROVISIONING_SERVER_H
#define SERVICE_PROVISIONING_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace StatusReporting {
class StatusReport;
}
}
}
}

namespace WeavePlatform {
namespace Internal {

class ServiceProvisioningServer
        : public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer,
          public ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningDelegate
{
public:

    typedef ::nl::Weave::Profiles::ServiceProvisioning::ServiceProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init(void);
    virtual WEAVE_ERROR HandleRegisterServicePairAccount(::nl::Weave::Profiles::ServiceProvisioning::RegisterServicePairAccountMessage& msg);
    virtual WEAVE_ERROR HandleUpdateService(::nl::Weave::Profiles::ServiceProvisioning::UpdateServiceMessage& msg);
    virtual WEAVE_ERROR HandleUnregisterService(uint64_t serviceId);
    virtual bool IsPairedToAccount(void) const;

    void OnPlatformEvent(const WeavePlatformEvent * event);

private:

    ::nl::Weave::Binding * mProvServiceBinding;
    bool mAwaitingServiceConnectivity;

    void StartPairDeviceToAccount(void);
    void SendPairDeviceToAccountRequest(void);
    virtual void HandlePairDeviceToAccountResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);

    static void AsyncStartPairDeviceToAccount(intptr_t arg);
    static void HandleConnectivityTimeout(::nl::Weave::System::Layer * layer, void * appState, ::nl::Weave::System::Error err);
    static void HandleProvServiceBindingEvent(void * appState, nl::Weave::Binding::EventType eventType,
            const nl::Weave::Binding::InEventParam & inParam, nl::Weave::Binding::OutEventParam & outParam);
};

extern ServiceProvisioningServer ServiceProvisioningSvr;

} // namespace Internal
} // namespace WeavePlatform

#endif // SERVICE_PROVISIONING_SERVER_H

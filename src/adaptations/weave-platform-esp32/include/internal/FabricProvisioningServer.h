#ifndef FABRIC_PROVISIONING_SERVER_H
#define FABRIC_PROVISIONING_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>

namespace WeavePlatform {
namespace Internal {

class FabricProvisioningServer
        : public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer,
          public ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningDelegate
{
public:
    typedef ::nl::Weave::Profiles::FabricProvisioning::FabricProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();
    virtual WEAVE_ERROR HandleCreateFabric(void);
    virtual WEAVE_ERROR HandleJoinExistingFabric(void);
    virtual WEAVE_ERROR HandleLeaveFabric(void);
    virtual WEAVE_ERROR HandleGetFabricConfig(void);
    virtual bool IsPairedToAccount() const;

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);
};

extern FabricProvisioningServer FabricProvisioningSvr;

} // namespace Internal
} // namespace WeavePlatform

#endif // FABRIC_PROVISIONING_SERVER_H

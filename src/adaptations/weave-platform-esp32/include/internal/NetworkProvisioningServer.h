#ifndef NETWORK_PROVISIONING_SERVER_H
#define NETWORK_PROVISIONING_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace WeavePlatform {
namespace Internal {

class NetworkProvisioningServer
        : public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer
{
public:
    typedef ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningServer ServerBaseClass;

    WEAVE_ERROR Init();

    uint8_t GetCurrentOp() const;

    virtual bool IsPairedToAccount() const;

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);
};

extern NetworkProvisioningServer NetworkProvisioningSvr;

inline uint8_t NetworkProvisioningServer::GetCurrentOp() const
{
    return mCurOpType;
}

} // namespace Internal
} // namespace WeavePlatform

#endif // NETWORK_PROVISIONING_SERVER_H

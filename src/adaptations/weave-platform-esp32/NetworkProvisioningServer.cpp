#include <internal/WeavePlatformInternal.h>
#include <internal/NetworkProvisioningServer.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_NetworkProvisioning;

namespace WeavePlatform {
namespace Internal {

WEAVE_ERROR NetworkProvisioningServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::WeavePlatform::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(ConnectivityMgr.GetNetworkProvisioningDelegate());

exit:
    return err;
}

bool NetworkProvisioningServer::IsPairedToAccount() const
{
    return ConfigurationMgr.IsServiceProvisioned();
}

void NetworkProvisioningServer::OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event)
{
    // Nothing to do so far.
}

} // namespace Internal
} // namespace WeavePlatform

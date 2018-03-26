#include <internal/WeavePlatformInternal.h>
#include <internal/DeviceControlServer.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DeviceControl;

namespace WeavePlatform {
namespace Internal {

WEAVE_ERROR DeviceControlServer::Init()
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

bool DeviceControlServer::ShouldCloseConBeforeResetConfig(uint16_t resetFlags)
{
    return false;
}

WEAVE_ERROR DeviceControlServer::OnResetConfig(uint16_t resetFlags)
{
    // TODO: implement this
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR DeviceControlServer::OnFailSafeArmed(void)
{
    // TODO: implement this
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR DeviceControlServer::OnFailSafeDisarmed(void)
{
    // TODO: implement this
    return WEAVE_NO_ERROR;
}

void DeviceControlServer::OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr)
{
}

void DeviceControlServer::OnRemotePassiveRendezvousStarted(void)
{
    // Not used.
}

void DeviceControlServer::OnRemotePassiveRendezvousDone(void)
{
    // Not used.
}

WEAVE_ERROR DeviceControlServer::WillStartRemotePassiveRendezvous(void)
{
    // Not used.
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

void DeviceControlServer::WillCloseRemotePassiveRendezvous(void)
{
    // Not used.
}

bool DeviceControlServer::IsResetAllowed(uint16_t resetFlags)
{
    return true;
}

WEAVE_ERROR DeviceControlServer::OnSystemTestStarted(uint32_t profileId, uint32_t testId)
{
    WEAVE_ERROR err;

    err = SendStatusReport(kWeaveProfile_Common, kStatus_UnsupportedMessage);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::OnSystemTestStopped(void)
{
    WEAVE_ERROR err;

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

bool DeviceControlServer::IsPairedToAccount() const
{
    return ConfigurationMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform



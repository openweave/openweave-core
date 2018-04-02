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
    // Force connection closed on reset to factory defaults.
    return (resetFlags & kResetConfigFlag_FactoryDefaults) != 0;
}

WEAVE_ERROR DeviceControlServer::OnResetConfig(uint16_t resetFlags)
{
    if ((resetFlags & kResetConfigFlag_FactoryDefaults) != 0)
    {
        ConfigurationMgr.InitiateFactoryReset();
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR DeviceControlServer::OnFailSafeArmed(void)
{
    WEAVE_ERROR err;

    err = ConfigurationMgr.SetFailSafeArmed();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceControlServer::OnFailSafeDisarmed(void)
{
    WEAVE_ERROR err;

    err = ConfigurationMgr.ClearFailSafeArmed();
    SuccessOrExit(err);

exit:
    return err;
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
    // Only reset to factory defaults supported.
    if ((resetFlags & kResetConfigFlag_FactoryDefaults) == 0)
    {
        return false;
    }

    // Defer to the Configuration Manager to determine if the system is in a
    // state where factory reset is allowed.
    return ConfigurationMgr.CanFactoryReset();
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

void DeviceControlServer::OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event)
{
    // Nothing to do so far.
}

} // namespace Internal
} // namespace WeavePlatform



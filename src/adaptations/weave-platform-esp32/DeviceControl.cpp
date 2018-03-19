#include <WeavePlatform-ESP32-Internal.h>

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/device-control/DeviceControl.h>

#include <new>

namespace WeavePlatform {
namespace Internal {

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DeviceControl;

class DeviceControlServer
        : public ::nl::Weave::Profiles::DeviceControl::DeviceControlServer,
          public ::nl::Weave::Profiles::DeviceControl::DeviceControlDelegate
{
public:
    typedef ::nl::Weave::Profiles::DeviceControl::DeviceControlServer ServerBaseClass;

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    virtual bool ShouldCloseConBeforeResetConfig(uint16_t resetFlags);
    virtual WEAVE_ERROR OnResetConfig(uint16_t resetFlags);
    virtual WEAVE_ERROR OnFailSafeArmed(void);
    virtual WEAVE_ERROR OnFailSafeDisarmed(void);
    virtual void OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr);
    virtual void OnRemotePassiveRendezvousStarted(void);
    virtual void OnRemotePassiveRendezvousDone(void);
    virtual WEAVE_ERROR WillStartRemotePassiveRendezvous(void);
    virtual void WillCloseRemotePassiveRendezvous(void);
    virtual bool IsResetAllowed(uint16_t resetFlags);
    virtual WEAVE_ERROR OnSystemTestStarted(uint32_t profileId, uint32_t testId);
    virtual WEAVE_ERROR OnSystemTestStopped(void);
    virtual bool IsPairedToAccount() const;
};

static DeviceControlServer gDeviceControlServer;

bool InitDeviceControlServer()
{
    WEAVE_ERROR err;

    new (&gDeviceControlServer) DeviceControlServer();
    err = gDeviceControlServer.Init(&ExchangeMgr);
    SuccessOrExit(err);

exit:
    if (err == WEAVE_NO_ERROR)
    {
        ESP_LOGI(TAG, "Weave Device Control server initialized");
    }
    else
    {
        ESP_LOGE(TAG, "Weave Device Control server initialization failed: %s", ErrorStr(err));
    }
    return (err == WEAVE_NO_ERROR);
}

WEAVE_ERROR DeviceControlServer::Init(WeaveExchangeManager *exchangeMgr)
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
    return ConfigMgr.IsServiceProvisioned();
}

} // namespace Internal
} // namespace WeavePlatform



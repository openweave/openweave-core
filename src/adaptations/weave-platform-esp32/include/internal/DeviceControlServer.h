#ifndef DEVICE_CONTROL_SERVER_H
#define DEVICE_CONTROL_SERVER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/device-control/DeviceControl.h>

namespace WeavePlatform {
namespace Internal {

class DeviceControlServer
        : public ::nl::Weave::Profiles::DeviceControl::DeviceControlServer,
          public ::nl::Weave::Profiles::DeviceControl::DeviceControlDelegate
{
public:
    typedef ::nl::Weave::Profiles::DeviceControl::DeviceControlServer ServerBaseClass;

    WEAVE_ERROR Init();
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

extern DeviceControlServer DeviceControlSvr;

} // namespace Internal
} // namespace WeavePlatform

#endif // DEVICE_CONTROL_SERVER_H

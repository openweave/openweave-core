/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef DEVICE_CONTROL_SERVER_H
#define DEVICE_CONTROL_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/Profiles/device-control/DeviceControl.h>

namespace nl {
namespace Weave {
namespace Device {
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

    void OnPlatformEvent(const WeaveDeviceEvent * event);
};

extern DeviceControlServer DeviceControlSvr;

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // DEVICE_CONTROL_SERVER_H

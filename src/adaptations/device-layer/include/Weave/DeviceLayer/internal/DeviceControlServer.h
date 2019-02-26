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

/**
 *    @file
 *          Defines the Device Layer DeviceControlServer object.
 */

#ifndef DEVICE_CONTROL_SERVER_H
#define DEVICE_CONTROL_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/device-control/DeviceControl.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Implements the Weave Device Control profile for a Weave device.
 */
class DeviceControlServer final
    : public ::nl::Weave::Profiles::DeviceControl::DeviceControlServer,
      public ::nl::Weave::Profiles::DeviceControl::DeviceControlDelegate
{
    using ServerBaseClass = ::nl::Weave::Profiles::DeviceControl::DeviceControlServer;

public:

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init();
    void OnPlatformEvent(const WeaveDeviceEvent * event);

    // ===== Members that override virtual methods on DeviceControlDelegate

    bool ShouldCloseConBeforeResetConfig(uint16_t resetFlags) override;
    WEAVE_ERROR OnResetConfig(uint16_t resetFlags) override;
    WEAVE_ERROR OnFailSafeArmed(void) override;
    WEAVE_ERROR OnFailSafeDisarmed(void) override;
    void OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr) override;
    void OnRemotePassiveRendezvousStarted(void) override;
    void OnRemotePassiveRendezvousDone(void) override;
    WEAVE_ERROR WillStartRemotePassiveRendezvous(void) override;
    void WillCloseRemotePassiveRendezvous(void) override;
    bool IsResetAllowed(uint16_t resetFlags) override;
    WEAVE_ERROR OnSystemTestStarted(uint32_t profileId, uint32_t testId) override;
    WEAVE_ERROR OnSystemTestStopped(void) override;

    // ===== Members that override virtual methods on DeviceControlServer

    bool IsPairedToAccount() const override;

private:

    // ===== Members for internal use by the following friends.

    friend DeviceControlServer & DeviceControlSvr(void);

    static DeviceControlServer sInstance;
};

/**
 * Returns a reference to the DeviceControlServer singleton object.
 */
inline DeviceControlServer & DeviceControlSvr(void)
{
    return DeviceControlServer::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // DEVICE_CONTROL_SERVER_H

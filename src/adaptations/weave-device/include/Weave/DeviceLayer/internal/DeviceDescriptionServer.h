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

#ifndef DEVICE_DESCRIPTION_SERVER_H
#define DEVICE_DESCRIPTION_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class DeviceDescriptionServer : public ::nl::Weave::Profiles::DeviceDescription::DeviceDescriptionServer
{
    typedef ::nl::Weave::Profiles::DeviceDescription::DeviceDescriptionServer ServerBaseClass;

public:
    WEAVE_ERROR Init();

    void OnPlatformEvent(const WeaveDeviceEvent * event);

private:
    static void HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr,
            const ::nl::Weave::Profiles::DeviceDescription::IdentifyRequestMessage& reqMsg, bool& sendResp,
            ::nl::Weave::Profiles::DeviceDescription::IdentifyResponseMessage& respMsg);
};

extern DeviceDescriptionServer DeviceDescriptionSvr;

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl


#endif // DEVICE_DESCRIPTION_SERVER_H

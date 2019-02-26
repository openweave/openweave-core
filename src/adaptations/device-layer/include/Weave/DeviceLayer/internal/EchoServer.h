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
 *          Defines the Device Layer EchoServer object.
 */

#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/echo/Next/WeaveEchoServer.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

/**
 * Implements the Weave Echo Profile for a Weave Device.
 */
class EchoServer final
    : public ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer
{
    typedef ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer ServerBaseClass;

public:

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init();

private:

    // ===== Members for internal use by the following friends.

    friend EchoServer & EchoSvr(void);

    static EchoServer sInstance;
};

/**
 * Returns a reference to the EchoServer singleton object.
 */
inline EchoServer & EchoSvr(void)
{
    return EchoServer::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // ECHO_SERVER_H

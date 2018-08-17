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

#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/echo/Next/WeaveEchoServer.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

class EchoServer : public ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer
{
public:
    typedef ::nl::Weave::Profiles::Echo_Next::WeaveEchoServer ServerBaseClass;

    WEAVE_ERROR Init();
};

extern EchoServer EchoSvr;

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // ECHO_SERVER_H

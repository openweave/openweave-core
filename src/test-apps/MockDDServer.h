/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Weave Device Description profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKDDSERVER_H_
#define MOCKDDSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

using nl::Inet::IPAddress;
using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::DeviceDescription;

class MockDeviceDescriptionServer : public DeviceDescriptionServer
{
public:
    MockDeviceDescriptionServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

private:
    static void HandleIdentifyRequest(void *appState, uint64_t nodeId, const IPAddress& nodeAddr, const IdentifyRequestMessage& reqMsg, bool& sendResp, IdentifyResponseMessage& respMsg);
};

#endif /* MOCKDSERVER_H_ */

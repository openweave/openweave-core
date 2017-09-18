/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      (i.e., server) for the Weave Service Directory profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKSERVICEDIRECTORY_H_
#define MOCKSERVICEDIRECTORY_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::ServiceDirectory;

class MockServiceDirServer
{
public:
    MockServiceDirServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR TearDown(void);

private:
    WeaveExchangeManager *mExchangeMgr;

    static void HandleServiceDirRequest(ExchangeContext *ec, const IPPacketInfo *addrInfo,
                                        const WeaveMessageInfo *msgInfo, uint32_t profileId,
                                        uint8_t msgType, PacketBuffer *payload);
};

#endif // MOCKSERVICEDIRECTORY_H_

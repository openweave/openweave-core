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
 *      (i.e., server) for the Pair Device to Account protocol of the
 *      Weave Service Provisioning profile used for the Weave mock
 *      device command line functional testing tool.
 *
 */

#ifndef MOCKPAIRINGENDPOINT_H_
#define MOCKPAIRINGENDPOINT_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

using ::nl::Weave::System::PacketBuffer;

class MockPairingServer
{
public:
    MockPairingServer();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;

    WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    static void HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *addrInfo,
                                    const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                    uint8_t msgType, PacketBuffer *payload);
};

#endif /* MOCKPAIRINGENDPOINT_H_ */

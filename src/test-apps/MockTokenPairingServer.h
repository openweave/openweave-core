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
 *      (i.e., server) for the Weave Token Pairing profile used for the
 *      Weave mock device command line functional testing tool.
 *
 */

#ifndef MOCKTOKENPAIRINGSERVER_H_
#define MOCKTOKENPAIRINGSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>

using nl::Inet::IPAddress;
using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::TokenPairing;

class MockTokenPairingServer : private TokenPairingServer, private TokenPairingDelegate
{
public:
    MockTokenPairingServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

protected:
    bool mIsPaired;
    virtual WEAVE_ERROR OnPairTokenRequest(TokenPairingServer *server, uint8_t *pairingToken, uint32_t pairingTokenLength);
    virtual WEAVE_ERROR OnUnpairTokenRequest(TokenPairingServer *server);
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
};

#endif /* MOCKTOKENPAIRINGSERVER_H_ */

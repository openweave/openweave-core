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
 *      This file declares a mocked Dropcam Legacy Pairing server.
 *
 */

#ifndef MOCKDCLPSERVER_H_
#define MOCKDCLPSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/vendor/nestlabs/dropcam-legacy-pairing/DropcamLegacyPairing.h>

using nl::Weave::WeaveExchangeManager;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::DropcamLegacyPairing;

class MockDropcamLegacyPairingServer: private DropcamLegacyPairingServer, private DropcamLegacyPairingDelegate
{
public:
    MockDropcamLegacyPairingServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    virtual WEAVE_ERROR GetCameraSecret(uint8_t (&secret)[CAMERA_SECRET_LEN]);
    virtual WEAVE_ERROR GetCameraMACAddress(uint8_t (&macAddress)[EUI48_LEN]);
    virtual void EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result);
};

#endif /* MOCKDCLPSERVER_H_ */

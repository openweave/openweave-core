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
 *      This file defines a mock Dropcam Legacy Pairing server.
 *
 */

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>

#include "ToolCommon.h"
#include "MockDCLPServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include "MockOpActions.h"

extern MockOpActions OpActions;

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::DropcamLegacyPairing;

MockDropcamLegacyPairingServer::MockDropcamLegacyPairingServer()
{
}

WEAVE_ERROR MockDropcamLegacyPairingServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    err = this->DropcamLegacyPairingServer::Init(exchangeMgr);
    SuccessOrExit(err);

    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR MockDropcamLegacyPairingServer::Shutdown()
{
    return this->DropcamLegacyPairingServer::Shutdown();
}

WEAVE_ERROR MockDropcamLegacyPairingServer::GetCameraSecret(uint8_t (&secret)[CAMERA_SECRET_LEN])
{
    memset(secret, 7, CAMERA_SECRET_LEN);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockDropcamLegacyPairingServer::GetCameraMACAddress(uint8_t (&macAddress)[EUI48_LEN])
{
    for (int i = 0; i < EUI48_LEN; i++)
    {
        macAddress[i] = (0x11 * i);
    }

    return WEAVE_NO_ERROR;
}

void MockDropcamLegacyPairingServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    DropcamLegacyPairingDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

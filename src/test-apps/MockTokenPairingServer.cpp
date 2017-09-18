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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Token Pairing profile used for the
 *      Weave mock device command line functional testing tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>

#include "ToolCommon.h"
#include "MockTokenPairingServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/ErrorStr.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::TokenPairing;

MockTokenPairingServer::MockTokenPairingServer() :
    mIsPaired(false)
{
}

WEAVE_ERROR MockTokenPairingServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    // Initialize the base class.
    err = this->TokenPairingServer::Init(exchangeMgr);
    SuccessOrExit(err);

    SetDelegate(this);

    mIsPaired = false;

exit:
    return err;
}

WEAVE_ERROR MockTokenPairingServer::Shutdown()
{
    return this->TokenPairingServer::Shutdown();
}

WEAVE_ERROR MockTokenPairingServer::OnPairTokenRequest(TokenPairingServer *server, uint8_t *pairingToken, uint32_t pairingTokenLength)
{
    WEAVE_ERROR     err             = WEAVE_NO_ERROR;
    uint8_t*        p;
    PacketBuffer*   certificateBuf  = NULL;
    PacketBuffer*   tokenBundleBuf  = NULL;

    printf("Pair Token request received.  Pairing token:");
    for (size_t i = 0; i < pairingTokenLength; i++)
    {
        printf(" %02x", pairingToken[i]);
    }
    printf("\n");

    VerifyOrExit(mIsPaired == false, err = WEAVE_ERROR_INCORRECT_STATE);

    // Send optional certificate response
    certificateBuf = PacketBuffer::New();
    VerifyOrExit(certificateBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = certificateBuf->Start();
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    p[3] = 4;
    certificateBuf->SetDataLength(4);

    err = server->SendTokenCertificateResponse(certificateBuf);
    SuccessOrExit(err);
    certificateBuf = NULL;

    // Send tokenpaired response
    tokenBundleBuf = PacketBuffer::New();
    VerifyOrExit(tokenBundleBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = tokenBundleBuf->Start();
    p[0] = 'a';
    p[1] = 'b';
    p[2] = 'c';
    p[3] = 'd';
    tokenBundleBuf->SetDataLength(4);

    err = server->SendTokenPairedResponse(tokenBundleBuf);
    SuccessOrExit(err);
    tokenBundleBuf = NULL;

    mIsPaired = true;
exit:
    if (err != WEAVE_NO_ERROR)
    {
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_InternalError, err);
    }
    if (certificateBuf != NULL)
    {
        PacketBuffer::Free(certificateBuf);
    }
    if (tokenBundleBuf != NULL)
    {
        PacketBuffer::Free(tokenBundleBuf);
    }
    return err;
}

WEAVE_ERROR MockTokenPairingServer::OnUnpairTokenRequest(TokenPairingServer *server)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    printf("Unpair Token request received.\n");

    if (!mIsPaired)
    {
        printf("Error: Unpair command received, but device is not paired.");
        err = WEAVE_ERROR_INCORRECT_STATE;
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_BadRequest, err);
        ExitNow();
    }
    mIsPaired = false;
    server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_Success, err);

exit:
    return err;
}

void MockTokenPairingServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    TokenPairingDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

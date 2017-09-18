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
 *      (i.e., server) for the Pair Device to Account protocol of the
 *      Weave Service Provisioning profile used for the Weave mock
 *      device command line functional testing tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include "ToolCommon.h"
#include "MockPairingServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

MockPairingServer::MockPairingServer()
{
    mExchangeMgr = NULL;
}

WEAVE_ERROR MockPairingServer::Init(nl::Weave::WeaveExchangeManager *exchangeMgr)
{
    mExchangeMgr = exchangeMgr;

    // Register to receive unsolicited PairDeviceToAccount messages from the exchange manager.
    WEAVE_ERROR err =
        mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_ServiceProvisioning, kMsgType_PairDeviceToAccount, HandleClientRequest, this);

    return err;
}

WEAVE_ERROR MockPairingServer::Shutdown()
{
    if (mExchangeMgr != NULL)
        mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_ServiceProvisioning, kMsgType_PairDeviceToAccount);
    return WEAVE_NO_ERROR;
}

void MockPairingServer::HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *pktInfo,
                                            const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                            uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];
    ec->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    StatusReporting::StatusReport statusReport;

    // Fail messages for the wrong profile. This shouldn't happen, but better safe than sorry.
    if (profileId == kWeaveProfile_ServiceProvisioning)
    {
        // Decode and dispatch the message.
        switch (msgType)
        {
        case kMsgType_PairDeviceToAccount:
        {
            PairDeviceToAccountMessage msg;

            err = PairDeviceToAccountMessage::Decode(payload, msg);
            SuccessOrExit(err);

            printf("PairDeviceToAccount request received from node %" PRIX64 " (%s)\n", ec->PeerNodeId, ipAddrStr);
            printf("  Service Id: %016" PRIX64 "\n", msg.ServiceId);
            printf("  Fabric Id: %016" PRIX64 "\n", msg.FabricId);
            printf("  Account Id: "); fwrite(msg.AccountId, 1, msg.AccountIdLen, stdout); printf("\n");
            printf("  Pairing Token (%d bytes): \n", (int)msg.PairingTokenLen);
            DumpMemory(msg.PairingToken, msg.PairingTokenLen, "    ", 16);
            printf("  Pairing Init Data (%d bytes): \n", (int)msg.PairingInitDataLen);
            DumpMemory(msg.PairingInitData, msg.PairingInitDataLen, "    ", 16);
            printf("  Device Init Data (%d bytes): \n", (int)msg.DeviceInitDataLen);
            DumpMemory(msg.DeviceInitData, msg.DeviceInitDataLen, "    ", 16);

            statusReport.mProfileId = kWeaveProfile_Common;
            statusReport.mStatusCode = Common::kStatus_Success;

            break;
        }

        default:
            statusReport.mProfileId = kWeaveProfile_Common;
            statusReport.mStatusCode = Common::kStatus_BadRequest;
            break;
        }
    }
    else
    {
        statusReport.mProfileId = kWeaveProfile_Common;
        statusReport.mStatusCode = Common::kStatus_BadRequest;
    }

    payload->SetDataLength(0);
    err = statusReport.pack(payload);
    SuccessOrExit(err);

    err = ec->SendMessage(kWeaveProfile_Common, Common::kMsgType_StatusReport, payload, 0);
    payload = NULL;
    SuccessOrExit(err);

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

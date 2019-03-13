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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Service Directory profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include "MockSDServer.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles::ServiceDirectory;

MockServiceDirServer::MockServiceDirServer()
{
    mExchangeMgr = NULL;
}

WEAVE_ERROR MockServiceDirServer::Init(WeaveExchangeManager *exchangeMgr)
{
    mExchangeMgr = exchangeMgr;

    WEAVE_ERROR err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_ServiceDirectory,
                      HandleServiceDirRequest, this);

    return err;
}

WEAVE_ERROR MockServiceDirServer::TearDown(void)
{
    return mExchangeMgr->UnregisterUnsolicitedMessageHandler(kWeaveProfile_ServiceDirectory);
}

void MockServiceDirServer::HandleServiceDirRequest(ExchangeContext *ec, const IPPacketInfo *packetInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payloadReceiveInit)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *host;
    uint16_t hostLen, port;
    const char *DirectoryServerURL = "192.168.100.3";  // mock service address in Happy
    uint8_t *buf;

    PacketBuffer *payload = PacketBuffer::New();
    VerifyOrExit(payload != NULL, err = WEAVE_ERROR_NO_MEMORY);
    buf = payload->Start();

    VerifyOrExit((profileId == kWeaveProfile_ServiceDirectory) &&
           (msgType == kMsgType_ServiceEndpointQuery), err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    err = ParseHostAndPort(DirectoryServerURL, strlen(DirectoryServerURL), host, hostLen, port);
    port = WEAVE_PORT;

    // generate a simulate service directory response
    Write8(buf, 0x62);  // directory length = 2  - suffix table and time fields

    Write8(buf, 0x41);  // host/port list length 1 - entry type host/port list
    LittleEndian::Write64(buf, 0x18B4300200000002ULL);  // Software Update Profile Endpoint Id
    Write8(buf, 0x0d);  // hostid type composite - suffix and port id present
    Write8(buf, hostLen);
    memcpy(buf, host, hostLen); buf += hostLen;
    Write8(buf, 0x00);  // suffix id 0
    LittleEndian::Write16(buf, port);

    Write8(buf, 0x41);  // host/port list length 1 - entry type host/port list
    LittleEndian::Write64(buf, 0x18B4300200000001ULL);  // Directory Service Endpoint Id
    Write8(buf, 0x0d);  // hostid type composite - suffix and port id present
    Write8(buf, hostLen);
    memcpy(buf, host, hostLen); buf += hostLen;
    Write8(buf, 0x01);  // suffix id 1
    LittleEndian::Write16(buf, port);

    Write8(buf, 0x02);  // suffix table length 1
    Write8(buf, 0x00);  // suffix 0 length
    Write8(buf, 0x00);  // suffix 1 length
    LittleEndian::Write64(buf, 0x1122334455667788ULL); //query receipt time field
    LittleEndian::Write32(buf, 0x00000001); // processing time field

    payload->SetDataLength(44 + 2 * hostLen);
    err = ec->SendMessage(kWeaveProfile_ServiceDirectory, kMsgType_ServiceEndpointResponse, payload, 0);
    payload = NULL;
    SuccessOrExit(err);

exit:
    if (err == WEAVE_ERROR_INVALID_MESSAGE_TYPE)
    {
        printf("MockSDServer: sending Common:UnsupportedMessage\n");
        err = nl::Weave::WeaveServerBase::SendStatusReport(ec,
                                                           nl::Weave::Profiles::kWeaveProfile_Common,
                                                           nl::Weave::Profiles::Common::kStatus_UnsupportedMessage,
                                                           WEAVE_NO_ERROR,
                                                           ec->HasPeerRequestedAck() ? nl::Weave::ExchangeContext::kSendFlag_RequestAck : 0);
    }
    if (payloadReceiveInit)
    {
        PacketBuffer::Free(payloadReceiveInit);
        payloadReceiveInit = NULL;
    }
    if (payload)
    {
        PacketBuffer::Free(payload);
        payload = NULL;
    }
    ec->Close();
    return;
}

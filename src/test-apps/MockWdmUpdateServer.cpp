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
 *      This file implements the Weave Data Management mock Update server.
 *
 */

#define WEAVE_CONFIG_ENABLE_LOG_FILE_LINE_FUNC_ON_ERROR 1

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include "MockWdmUpdateServer.h"
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include "MockSinkTraits.h"
#include "MockSourceTraits.h"
#include "TestGroupKeyStore.h"

using nl::Weave::System::PacketBuffer;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

using namespace nl::Weave::Profiles::DataManagement;

/**
 *  Log the specified message in the form of @a aFormat.
 *
 *  @param[in]     aFormat   A pointer to a NULL-terminated C string with
 *                           C Standard Library-style format specifiers
 *                           containing the log message to be formatted and
 *                           logged.
 *  @param[in]     ...       An argument list whose elements should correspond
 *                           to the format specifiers in @a aFormat.
 *
 */
static void TLVPrettyPrinter(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    // There is no proper Weave logging routine for us to use here
    vprintf(aFormat, args);

    va_end(args);
}

static WEAVE_ERROR DebugPrettyPrint(nl::Weave::TLV::TLVReader & aReader)
{
    return nl::Weave::TLV::Debug::Dump(aReader, TLVPrettyPrinter);
}

class MockWdmUpdateServerImpl: public MockWdmUpdateServer
{
public:

    virtual WEAVE_ERROR Init (nl::Weave::WeaveExchangeManager *aExchangeMgr, const char * const aTestCaseId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        WeaveLogDetail(DataManagement, "Test Case ID: %s", aTestCaseId);

        err = aExchangeMgr->RegisterUnsolicitedMessageHandler(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_UpdateRequest, IncomingUpdateRequest, this);
        SuccessOrExit(err);

    exit:
        return err;
    }

private:

    static void IncomingUpdateRequest(nl::Weave::ExchangeContext *ec, const nl::Weave::IPPacketInfo *pktInfo,
        const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                uint8_t msgType, PacketBuffer *payload);
};

static MockWdmUpdateServerImpl gWdmUpdateServer;

MockWdmUpdateServer * MockWdmUpdateServer::GetInstance ()
{
    return &gWdmUpdateServer;
}

void MockWdmUpdateServerImpl::IncomingUpdateRequest(nl::Weave::ExchangeContext *ec, const nl::Weave::IPPacketInfo *pktInfo,
    const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload)
{
    // TODO: 1. Add Parse to check incoming update request. 2. Add statusList and version list in status report
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    WeaveLogDetail(DataManagement, "Incoming Update Request");

    nl::Weave::TLV::TLVReader reader;
    uint8_t statusReportLen = 6;
    uint8_t * p;
    reader.Init(payload);
    DebugPrettyPrint(reader);

    PacketBuffer * msgBuf   = PacketBuffer::NewWithAvailableSize(statusReportLen);
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    nl::Weave::Encoding::LittleEndian::Write32(p, nl::Weave::Profiles::kWeaveProfile_Common);
    nl::Weave::Encoding::LittleEndian::Write16(p, nl::Weave::Profiles::Common::kStatus_Success);
    msgBuf->SetDataLength(statusReportLen);

    err = ec->SendMessage(nl::Weave::Profiles::kWeaveProfile_Common, nl::Weave::Profiles::Common::kMsgType_StatusReport, msgBuf, nl::Weave::ExchangeContext::kSendFlag_RequestAck);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != payload)
    {
        PacketBuffer::Free(payload);
        payload = NULL;
    }

    if (NULL != msgBuf)
    {
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    if (NULL != ec)
    {
        ec->Close();
    }
}


#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

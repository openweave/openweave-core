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
 *      This file implements command handle for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Support/crypto/WeaveCrypto.h>

#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

using namespace nl::Weave::TLV;

Command::Command(void)
{
    (void)Init(NULL);
}

WEAVE_ERROR Command::Init(nl::Weave::ExchangeContext *aEC)
{
    mEC = aEC;
    return WEAVE_NO_ERROR;
}

void Command::Close(void)
{
    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s",
            SubscriptionEngine::GetInstance()->GetCommandObjId(this),
            (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
            __func__);

    WeaveLogIfFalse(NULL != mEC);

    if (NULL != mEC)
    {
        mEC->Close();
        mEC = NULL;
    }

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMNext_NumCommands);
}

WEAVE_ERROR Command::SendError(uint32_t aProfileId, uint16_t aStatusCode, WEAVE_ERROR aWeaveError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s profile: %" PRIu32 ", code: %" PRIu16 ", err %s",
            SubscriptionEngine::GetInstance()->GetCommandObjId(this),
            (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
            __func__, aProfileId, aStatusCode, nl::ErrorStr(aWeaveError));

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    err = nl::Weave::WeaveServerBase::SendStatusReport(mEC,
            aProfileId,
            aStatusCode,
            aWeaveError,
            nl::Weave::ExchangeContext::kSendFlag_RequestAck);

exit:
    WeaveLogFunctError(err);

    Close();

    return err;
}

WEAVE_ERROR Command::SendInProgress(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s",
            SubscriptionEngine::GetInstance()->GetCommandObjId(this),
            (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
            __func__);

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        PacketBuffer *msgBuf = PacketBuffer::NewWithAvailableSize(0);
        VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

        err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_InProgress,
                msgBuf, nl::Weave::ExchangeContext::kSendFlag_RequestAck);
        msgBuf = NULL;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

/**
 * Formulate and send a Custom Command Response message.
 *
 * @note
 *   If the application has any response data to send inside this message, it needs
 *   to pass that as an anonymous TLV structure encoded inside \p respBuf.
 *
 * @param[in]    traitInstanceVersion   the version of the trait instance.
 *
 * @param[in]    respBuf                pointer to the PacketBuffer that would have the CommandResponse encoded.
 *
 * @return WEAVE_ERROR   the appropriate WEAVE_ERROR encountered while processing this call.
 */
WEAVE_ERROR Command::SendResponse(uint32_t traitInstanceVersion, nl::Weave::System::PacketBuffer *respBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *appRespData;
    uint16_t appRespDataLen;
    nl::Weave::TLV::TLVWriter respWriter;
    TLVType containerType;
    bool res;

    enum
    {
        // The maximum number of bytes between the beginning of a WDM Command Response message
        // and the point within the message at which the application response data begins.

        kMaxCommandResponseHeaderSize =
            1 +          // Anonymous Structure
            1 + 1 + 4 +  // Version field (1 control byte + 1 context tag + 4 value bytes)
            1 + 1,       // App Response Data Structure (1 control byte + 1 context tag)
    };

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s",
            SubscriptionEngine::GetInstance()->GetCommandObjId(this),
            (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
            __func__);

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    // If the application didn't supply a response buffer, allocate one for them.
    if (respBuf == NULL)
    {
        respBuf = PacketBuffer::New(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + kMaxCommandResponseHeaderSize);
        VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    }

    // Ensure that the buffer has enough space to accommodate the command response header and the lower layer packet headers.
    res = respBuf->EnsureReservedSize(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + kMaxCommandResponseHeaderSize);
    VerifyOrExit(res, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Store the pointer to the application data that is to be inserted into the buffer later.
    appRespData = respBuf->Start();
    appRespDataLen = respBuf->DataLength();

    // If the application supplied data to be sent with the response perform some sanity checks
    if (appRespDataLen > 0)
    {
        // Verify that response data supplied by the application is encapsulated in an anonymous
        // TLV structure. All anonymous TLV structures begin with an anonymous structure control
        // byte (0x15) and end with an end-of-container control byte (0x18).
        VerifyOrExit(appRespDataLen > 2, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(appRespData[0] == kTLVElementType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(appRespData[appRespDataLen - 1] == kTLVElementType_EndOfContainer, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Strip off the anonymous structure supplied by the application, leaving just the raw
        // contents.
        appRespData += 1;
        appRespDataLen -= 1;
    }

    // Move the start pointer to prepare the buffer for encoding the Command Response message.
    respBuf->SetStart(respBuf->Start() - kMaxCommandResponseHeaderSize);

    // Set the datalength of the buffer to zero to let a TLVWriter write the entire message from the beginning.
    respBuf->SetDataLength(0);

    // Initialize a TLV writer to write the WDM command response message into the response buffer.
    respWriter.Init(respBuf);

    // Start the anonymous container that wraps the response.
    err = respWriter.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Encode the trait instance version field.
    err = respWriter.Put(ContextTag(CustomCommandResponse::kCsTag_Version), traitInstanceVersion);
    SuccessOrExit(err);

    // If the application supplied data to be sent with the response, insert it in the Command Response.
    if (appRespDataLen > 0)
    {
        // Copy the application response data into a new TLV structure field contained with the
        // response structure.  NOTE: The TLV writer will take care of moving the response data
        // to the correct location within the buffer.
        err = respWriter.PutPreEncodedContainer(ContextTag(CustomCommandResponse::kCsTag_Response), kTLVType_Structure,
                                                appRespData, appRespDataLen);
        SuccessOrExit(err);
    }

    // End the outer response structure.
    err = respWriter.EndContainer(containerType);
    SuccessOrExit(err);

    // Finalize the response message encoding.
    err = respWriter.Finalize();
    SuccessOrExit(err);

    // Call exchange context to send response
    err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_CustomCommandResponse,
                           respBuf, nl::Weave::ExchangeContext::kSendFlag_RequestAck);

    // Don't free the buffer on exit.
    respBuf = NULL;

exit:
    WeaveLogFunctError(err);

    Close();

    if (NULL != respBuf)
    {
        PacketBuffer::Free(respBuf);
    }

    return err;
}

WEAVE_ERROR Command::ValidateAuthenticator(nl::Weave::System::PacketBuffer * aRequestBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    IgnoreUnusedVariable(aRequestBuffer);

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s",
            SubscriptionEngine::GetInstance()->GetCommandObjId(this),
            (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
            __func__);

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

exit:
    WeaveLogFunctError(err);

    return err;
}

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WDM_PUBLISHER_ENABLE_CUSTOM_COMMANDS

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
#include <Weave/Support/FlagUtils.hpp>

#include <Weave/Support/crypto/WeaveCrypto.h>

#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WDM_PUBLISHER_ENABLE_CUSTOM_COMMAND_HANDLER

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

using namespace nl::Weave;
using namespace nl::Weave::TLV;

Command::Command(void)
{
    (void) Init(NULL);
}

WEAVE_ERROR Command::Init(nl::Weave::ExchangeContext * aEC)
{
    mEC = aEC;
    mFlags = 0;
    commandType = 0;
    mustBeVersion = 0;
    initiationTimeMicroSecond = 0;
    actionTimeMicroSecond = 0;
    expiryTimeMicroSecond = 0;

    return WEAVE_NO_ERROR;
}

/**
 *  Determine whether the version in the command is valid.
 *
 *  @return Returns 'true' if the version is valid, else 'false'.
 *
 */
bool Command::IsMustBeVersionValid(void) const
{
    return GetFlag(mFlags, kCommandFlag_MustBeVersionValid);
}

/**
 *  Determine whether the initiation time in the command is valid.
 *
 *  @return Returns 'true' if the initiation time is valid, else 'false'.
 *
 */
bool Command::IsInitiationTimeValid(void) const
{
    return GetFlag(mFlags, kCommandFlag_InitiationTimeValid);
}

/**
 *  Determine whether the action time in the command is valid.
 *
 *  @return Returns 'true' if the action time is valid, else 'false'.
 *
 */
bool Command::IsActionTimeValid(void) const
{
    return GetFlag(mFlags, kCommandFlag_ActionTimeValid);
}

/**
 *  Determine whether the expiry time in the command is valid.
 *
 *  @return Returns 'true' if the expiry time is valid, else 'false'.
 *
 */
bool Command::IsExpiryTimeValid(void) const
{
    return GetFlag(mFlags, kCommandFlag_ExpiryTimeValid);
}

/**
 *  Determine whether the command is one-way.
 *
 *  @return Returns 'true' if the command is one-way, else 'false'.
 *
 */
bool Command::IsOneWay(void) const
{
    return GetFlag(mFlags, kCommandFlag_IsOneWay);
}

/**
 *  Set the kCommandFlag_MustBeVersionValid flag bit.
 *
 *  @param[in]  inMustBeVersionValid  A Boolean indicating whether (true) or not
 *                                    (false) the version field in the Command is
 *                                    valid.
 *
 */
void Command::SetMustBeVersionValid(bool inMustBeVersionValid)
{
    SetFlag(mFlags, kCommandFlag_MustBeVersionValid, inMustBeVersionValid);
}

/**
 *  Set the kCommandFlag_InitiationTimeValid flag bit.
 *
 *  @param[in]  inInitiationTimeValid  A Boolean indicating whether (true) or not
 *                                     (false) the initiation time field in the
 *                                     Command is valid.
 *
 */
void Command::SetInitiationTimeValid(bool inInitiationTimeValid)
{
    SetFlag(mFlags, kCommandFlag_InitiationTimeValid, inInitiationTimeValid);
}

/**
 *  Set the kCommandFlag_ActionTimeValid flag bit.
 *
 *  @param[in]  inActionTimeValid  A Boolean indicating whether (true) or not
 *                                 (false) the action time field in the
 *                                 Command is valid.
 *
 */
void Command::SetActionTimeValid(bool inActionTimeValid)
{
    SetFlag(mFlags, kCommandFlag_ActionTimeValid, inActionTimeValid);
}

/**
 *  Set the kCommandFlag_ExpiryTimeValid flag bit.
 *
 *  @param[in]  inExpiryTimeValid  A Boolean indicating whether (true) or not
 *                                 (false) the expiry time field in the
 *                                 Command is valid.
 *
 */
void Command::SetExpiryTimeValid(bool inExpiryTimeValid)
{
    SetFlag(mFlags, kCommandFlag_ExpiryTimeValid, inExpiryTimeValid);
}

/**
 *  Set the kCommandFlag_IsOneWay flag bit.
 *
 *  @param[in]  inIsOneWay         A Boolean indicating whether (true) or not
 *                                 (false) the Command is one-way.
 *
 */
void Command::SetIsOneWay(bool inIsOneWay)
{
    SetFlag(mFlags, kCommandFlag_IsOneWay, inIsOneWay);
}

/**
 *  @brief Send no message but free all resources associated, including closing the exchange context
 *
 *  @return  #WEAVE_NO_ERROR if the message has been pushed into message layer
 */
void Command::Close(void)
{
    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s", SubscriptionEngine::GetInstance()->GetCommandObjId(this),
                   (NULL != mEC) ? mEC->ExchangeId : 0xFFFF, __func__);

    WeaveLogIfFalse(NULL != mEC);

    if (NULL != mEC)
    {
        mEC->Close();
        mEC = NULL;
    }

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDM_NumCommands);
}

/**
 *  @brief Send a Status Report message to indicate the command has failed
 *
 *  @param[in]    aProfileId        The profile for the specified status code.
 *
 *  @param[in]    aStatusCode       The status code to send.
 *
 *  @param[in]    aWeaveError       The Weave error associated with the status
 *                                  code.
 *
 *  @note Application layer doesn’t have the choice to append custom data into this message.
 *    #Close is implicitly called at the end of this function in all conditions.
 *
 *  @return  #WEAVE_NO_ERROR if the message has been pushed into message layer
 */
WEAVE_ERROR Command::SendError(uint32_t aProfileId, uint16_t aStatusCode, WEAVE_ERROR aWeaveError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s profile: %" PRIu32 ", code: %" PRIu16 ", err %s",
                   SubscriptionEngine::GetInstance()->GetCommandObjId(this), (NULL != mEC) ? mEC->ExchangeId : 0xFFFF, __func__,
                   aProfileId, aStatusCode, nl::ErrorStr(aWeaveError));

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    // Drop the response if the Command was OneWay.

    VerifyOrExit(!IsOneWay(), err = WEAVE_NO_ERROR);

    err = nl::Weave::WeaveServerBase::SendStatusReport(mEC, aProfileId, aStatusCode, aWeaveError,
                                                       nl::Weave::ExchangeContext::kSendFlag_RequestAck);

exit:
    WeaveLogFunctError(err);

    Close();

    return err;
}

/**
 *  @brief Send an In-Progress message to indicate the command is not yet completed yet
 *
 *  @note The exact timing and meaning for this message is defined by each particular trait.
 *    #Close is implicitly called at the end of this function in all conditions.
 *
 *  @return  #WEAVE_NO_ERROR if the message is successfully pushed into message layer
 */
WEAVE_ERROR Command::SendInProgress(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Drop the Send if the Command was OneWay.

    if (IsOneWay())
    {
        Close();

        ExitNow(err = WEAVE_NO_ERROR);
    }

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    {
        PacketBuffer * msgBuf = PacketBuffer::NewWithAvailableSize(0);
        VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

        err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_InProgress, msgBuf,
                               nl::Weave::ExchangeContext::kSendFlag_RequestAck);
        msgBuf = NULL;
    }

exit:
    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s %s", SubscriptionEngine::GetInstance()->GetCommandObjId(this),
                   (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
                   IsOneWay() ? "OneWay: Dropping Response to Sender in": "", __func__);

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
WEAVE_ERROR Command::SendResponse(uint32_t traitInstanceVersion, nl::Weave::System::PacketBuffer * respBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t * appRespData;
    uint16_t appRespDataLen;
    nl::Weave::TLV::TLVWriter respWriter;
    TLVType containerType;
    bool res;

    static const uint8_t encoding[] =
    {
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),                                                         // {
            nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_MustBeVersion), 0),         //      MustBeVersion = 64-
            nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_Argument))               //      Args = {
    };

    enum
    {
        // The maximum number of bytes between the beginning of a WDM Command Response message
        // and the point within the message at which the application response data begins.
        kMaxCommandResponseHeaderSize = sizeof(encoding)
    };

    // Drop the response if the Command was OneWay.
    VerifyOrExit(!IsOneWay(), err = WEAVE_NO_ERROR);

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
    appRespData    = respBuf->Start();
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
        err = respWriter.PutPreEncodedContainer(ContextTag(CustomCommandResponse::kCsTag_Response), kTLVType_Structure, appRespData,
                                                appRespDataLen);
        SuccessOrExit(err);
    }

    // End the outer response structure.
    err = respWriter.EndContainer(containerType);
    SuccessOrExit(err);

    // Finalize the response message encoding.
    err = respWriter.Finalize();
    SuccessOrExit(err);

    // Call exchange context to send response
    err = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_CustomCommandResponse, respBuf,
                           nl::Weave::ExchangeContext::kSendFlag_RequestAck);

    // Don't free the buffer on exit.
    respBuf = NULL;

exit:
    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s %s", SubscriptionEngine::GetInstance()->GetCommandObjId(this),
                   (NULL != mEC) ? mEC->ExchangeId : 0xFFFF,
                   IsOneWay() ? "OneWay: Dropping Response to Sender in": "", __func__);

    WeaveLogFunctError(err);

    Close();

    if (NULL != respBuf)
    {
        PacketBuffer::Free(respBuf);
    }

    return err;
}

/**
 *  @brief Validate the authenticator that came with the command against specified condition
 *
 *  @note We haven’t flushed out all the fields that have to go in here yet.
 *    This function has to be called BEFORE the request buffer is freed.
 *
 *  @return  #WEAVE_NO_ERROR if the command is valid
 */
WEAVE_ERROR Command::ValidateAuthenticator(nl::Weave::System::PacketBuffer * aRequestBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    IgnoreUnusedVariable(aRequestBuffer);

    WeaveLogDetail(DataManagement, "Command[%d] [%04" PRIX16 "] %s", SubscriptionEngine::GetInstance()->GetCommandObjId(this),
                   (NULL != mEC) ? mEC->ExchangeId : 0xFFFF, __func__);

    VerifyOrExit(NULL != mEC, err = WEAVE_ERROR_INCORRECT_STATE);

exit:
    WeaveLogFunctError(err);

    return err;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WDM_PUBLISHER_ENABLE_CUSTOM_COMMAND_HANDLER

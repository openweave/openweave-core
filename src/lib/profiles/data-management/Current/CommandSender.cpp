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

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_CUSTOM_COMMAND_SENDER

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

using namespace nl::Weave;
using namespace nl::Weave::TLV;

WEAVE_ERROR CommandSender::SynchronizedTraitState::Init()
{
    memset(this, 0, sizeof(CommandSender::SynchronizedTraitState));
    return WEAVE_NO_ERROR;
}

bool CommandSender::SynchronizedTraitState::HasDataCaughtUp(void)
{
    uint64_t dataSinkVersion;

    if (mDataSink == NULL || mDataSink->GetVersion() == 0)
    {
        return false;
    }

    dataSinkVersion = mDataSink->GetVersion();

    if (mPostCommandVersion == 0)
    {
        return false;
    }

    if (mPreCommandVersion == 0)
    {
        mPreCommandVersion = mPostCommandVersion - kCommandSideEffectWindowSize;
    }

    {
        bool newerThanPost = (dataSinkVersion >= mPostCommandVersion);
        bool olderThanPre = (dataSinkVersion < mPreCommandVersion);

        if (mPostCommandVersion > mPreCommandVersion)
        {
            if (newerThanPost || olderThanPre)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (newerThanPost && olderThanPre)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

/**
 * Initializes the trait path for the command (within the arguments object) given a TraitDataSink + TraitCatalog housing that sink and the commadn type.
 *
 * @param[in] aCatalog          Pointer to the TraitCatalog housing the data sink
 * @param[in] aSink             Pointer to the trait data sink
 * @param[in] aCommandType      Type of command
 *
 * @retval WEAVE_ERROR          WEAVE_NO_ERROR if success, error otherwise.
 */
WEAVE_ERROR CommandSender::SendParams::PopulateTraitPath(TraitCatalogBase<TraitDataSink> *aCatalog, TraitDataSink *aSink, uint32_t aCommandType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TraitDataHandle handle;

    err = aCatalog->Locate(aSink, handle);
    SuccessOrExit(err);

    err = aCatalog->GetInstanceId(handle, InstanceId);
    SuccessOrExit(err);

    err = aCatalog->GetResourceId(handle, ResourceId);
    SuccessOrExit(err);

    ProfileId = aSink->GetSchemaEngine()->GetProfileId();
    VersionRange.mMaxVersion = aSink->GetSchemaEngine()->GetMaxVersion();
    VersionRange.mMinVersion = aSink->GetSchemaEngine()->GetMinVersion();
    CommandType = aCommandType;

exit:
    WeaveLogFunctError(err);
    return err;
}

/**
 * Initializes some of the appropriate fields in the CommandSender object.
 *
 * @param[in] aBinding          Binding that should be used with this command for all future sends. Should be
 *                              initialized already.
 * @param[in] aEventCallback    Callback for the application to register interest in notable events. If NULL,
 *                              the DefaultEventHandler is used.
 * @param[in] aAppState         App context to be passed into the event callback
 *
 * @retval WEAVE_ERROR          WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED if the default handler isn't setup.
 *                              Otherwise, WEAVE_NO_ERROR
 */
WEAVE_ERROR CommandSender::Init(Binding *aBinding, const EventCallback aEventCallback, void * const aAppState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aBinding)
    {
        mBinding = aBinding;
        mBinding->AddRef();
    }

    if (aEventCallback)
    {
        InEventParam inParam;
        OutEventParam outParam;

        mEventCallback = aEventCallback;
        mAppState = aAppState;

        inParam.Clear();
        outParam.Clear();

        // Test the application to ensure it's calling the default handler appropriately.
        aEventCallback(aAppState, kEvent_DefaultCheck, inParam, outParam);
        VerifyOrExit(outParam.defaultHandlerCalled, err = WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED);
    }
    else
    {
        mEventCallback = DefaultEventHandler;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}

void CommandSender::Close(bool aAbortNow)
{
    if (mEC)
    {
        if (aAbortNow)
        {
            mEC->Abort();
        }
        else
        {
            mEC->Close();
        }

        mEC = NULL;
    }

    if (mBinding)
    {
        mBinding->Release();
        mBinding = NULL;
    }
}

WEAVE_ERROR CommandSender::SendCommand(nl::Weave::PacketBuffer *aPayload, nl::Weave::Binding *aBinding, ResourceIdentifier &aResourceId, uint32_t aProfileId, uint32_t aCommandType)
{
    SendParams sendParams;

    memset(&sendParams, 0, sizeof(sendParams));

    sendParams.ResourceId = aResourceId;
    sendParams.ProfileId = aProfileId;
    sendParams.CommandType = aCommandType;
    sendParams.VersionRange.mMaxVersion = 1;
    sendParams.VersionRange.mMinVersion = 1;

    return SendCommand(aPayload, aBinding, sendParams);
}

/**
 * Sends out the command given a packet buffer that contains the payload to the command, as well as arguments to the command header.
 * The caller can also override the default binding initialized earlier by passing it in.
 *
 * @param[in] aRequestBuf                   Buffer containing arguments. Can be NULL if no arguments present
 * @param[in] aBinding                      Optional binding that can over-ride the one setup earlier in Init, except when NULL is passed in.
 * @param[in] aResponseTimeoutMsOverride    Optional override default response timeout, except when 0 is passed in.
 *
 * @retval WEAVE_ERROR                      Error in sending out command
 */
WEAVE_ERROR CommandSender::SendCommand(PacketBuffer *aRequestBuf, Binding *aBinding, SendParams &aSendParams)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    Binding *binding = aBinding ? aBinding : mBinding;
    const uint8_t *appReqData;
    uint16_t appReqDataLen;
    uint16_t sendFlags = 0;
    TLVWriter reqWriter;
    uint8_t msgType = kMsgType_CustomCommandRequest;

    // Use the TLV macros to estimate the worst-case size of the command request header.
    static const uint8_t headerEncoding[] =
    {
        nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_ANONYMOUS),                                                             // {
            nlWeaveTLV_PATH(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_Path)),                               //      Path = <{ResourceId = [10-byte byte string], ProfileId = 32-bit number, InstanceId = 64-bit number}>,
                nlWeaveTLV_STRUCTURE(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_InstanceLocator)),
                    nlWeaveTLV_UTF8_STRING_1ByteLength(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_ResourceID), 10,
                            0x01, 0xc0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08),
                    nlWeaveTLV_UINT32(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_TraitProfileID), 0),
                    nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(Path::kCsTag_TraitInstanceID), 0),
                nlWeaveTLV_END_OF_CONTAINER,
            nlWeaveTLV_END_OF_CONTAINER,

            nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_CommandType), 0),                   //      CommandType = 64-bit number,
            nlWeaveTLV_INT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_ActionTime), 0),                     //      ActionTime = 64-bit number,
            nlWeaveTLV_INT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_InitiationTime), 0),                 //      InitiationTime = 64-bit number,
            nlWeaveTLV_INT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_ExpiryTime), 0),                     //      ExpiryTime = 64-bit number,
            nlWeaveTLV_UINT64(nlWeaveTLV_TAG_CONTEXT_SPECIFIC(CustomCommand::kCsTag_MustBeVersion), 0),                 //      MustBeVersion = 64-bit number,
    };

    enum
    {
        kMaxCommandRequestHeaderSize = sizeof(headerEncoding)
    };

    VerifyOrExit(binding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(aSendParams.ProfileId != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // If the user didn't pass in a request buffer that we can re-use to blit the header, allocate our own.
    if (aRequestBuf == NULL)
    {
        aRequestBuf = PacketBuffer::New(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + kMaxCommandRequestHeaderSize);
        VerifyOrExit(aRequestBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    }

    // Ensure there is enough space in the buffer to write out the header.
    err = aRequestBuf->EnsureReservedSize(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + kMaxCommandRequestHeaderSize);
    VerifyOrExit(err, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    appReqData = aRequestBuf->Start();
    appReqDataLen = aRequestBuf->DataLength();

    // Verify that response data supplied by the application is encapsulated in an anonymous
    // TLV structure. All anonymous TLV structures begin with an anonymous structure control
    // byte (0x15) and end with an end-of-container control byte (0x18).
    if (appReqDataLen > 0)
    {
        VerifyOrExit(appReqDataLen > 2, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(appReqData[0] == kTLVElementType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(appReqData[appReqDataLen - 1] == kTLVElementType_EndOfContainer, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // Strip off the anonymous structure supplied by the application, leaving just the raw
        // contents.
        appReqData += 1;
        appReqDataLen -= 1;
    }

    // Move the start pointer so that we can start writing out the header.
    aRequestBuf->SetStart(aRequestBuf->Start() - kMaxCommandRequestHeaderSize);
    aRequestBuf->SetDataLength(0);

    reqWriter.Init(aRequestBuf);

    {
        CustomCommand::Builder request;

        err = request.Init(&reqWriter);
        SuccessOrExit(err);

        Path::Builder &path = request.CreatePathBuilder();
        path.ResourceID(aSendParams.ResourceId).ProfileID(aSendParams.ProfileId, aSendParams.VersionRange).InstanceID(aSendParams.InstanceId).EndOfPath();
        err = path.GetError();
        SuccessOrExit(err);

        request.CommandType(aSendParams.CommandType);

        if (aSendParams.Flags & kCommandFlag_MustBeVersionValid)
        {
            request.MustBeVersion(aSendParams.MustBeVersion);
        }

        if (aSendParams.Flags & kCommandFlag_ExpiryTimeValid)
        {
            request.ExpiryTimeMicroSecond(aSendParams.ExpiryTimeMicroSecond);
        }

        if (aSendParams.Flags & kCommandFlag_ActionTimeValid)
        {
            request.ActionTimeMicroSecond(aSendParams.ActionTimeMicroSecond);
        }

        err = request.GetError();
        SuccessOrExit(err);

        if (appReqDataLen > 0)
        {
            // Copy the application argument data into a new TLV structure field contained with the
            // request structure.  NOTE: The TLV writer will take care of moving the app data
            // to the correct location within the buffer.
            err = reqWriter.PutPreEncodedContainer(ContextTag(CustomCommand::kCsTag_Argument), kTLVType_Structure, appReqData, appReqDataLen);
            SuccessOrExit(err);
        }

        request.EndOfCustomCommand();
        err = request.GetError();
        SuccessOrExit(err);

        err = reqWriter.Finalize();
        SuccessOrExit(err);
    }

    // If the app has setup the synchronized trait state struct, fill the pre-command version field appropriately.
    if (mSyncronizedTraitState)
    {
        mSyncronizedTraitState->mDataSink = aSendParams.Sink;
        mSyncronizedTraitState->mPreCommandVersion = mSyncronizedTraitState->mDataSink->GetVersion();
    }

    // If an existing command is in flight (i.e mEC != NULL), let's close that one out and send this new command.
    Close();

    err = binding->NewExchangeContext(mEC);
    SuccessOrExit(err);

    mEC->AppState = this;
    mEC->OnMessageReceived = OnMessageReceived;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError = OnSendError;
    mEC->OnAckRcvd = NULL;

    if (aSendParams.ResponseTimeoutMsOverride > 0)
    {
        mEC->ResponseTimeout = aSendParams.ResponseTimeoutMsOverride;
    }

    if (aSendParams.Flags & kCommandFlag_IsOneWay)
    {
        msgType = kMsgType_OneWayCommand;
    }
    else
    {
        sendFlags |= ExchangeContext::kSendFlag_ExpectResponse;
    }

    mFlags = aSendParams.Flags;

    err = mEC->SendMessage(kWeaveProfile_WDM, msgType, aRequestBuf, sendFlags);
    aRequestBuf = NULL;

exit:
    if ((err != WEAVE_NO_ERROR) && (aRequestBuf != NULL))
    {
        PacketBuffer::Free(aRequestBuf);
    }

    WeaveLogFunctError(err);
    return err;
}

void CommandSender::DefaultEventHandler(void *aAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam)
{
    // No actions required for current implementation
    aOutParam.defaultHandlerCalled = true;
}

void CommandSender::OnSendError(ExchangeContext *aEC, WEAVE_ERROR sendError, void *aMsgCtxt)
{
    CommandSender *_this = static_cast<CommandSender *>(aEC->AppState);
    InEventParam inParam;
    OutEventParam outParam;
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(_this != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(_this->mEC != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    inParam.CommunicationError.error = sendError;
    _this->mEventCallback(_this->mAppState, kEvent_CommunicationError, inParam, outParam);

    // After error, let's close out the exchange
    _this->Close();

exit:

    WeaveLogFunctError(err);
    return;
}

void CommandSender::OnResponseTimeout(ExchangeContext *aEC)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CommandSender *_this = static_cast<CommandSender *>(aEC->AppState);
    InEventParam inEventParam;
    OutEventParam outEventParam;

    VerifyOrExit(_this, err = WEAVE_ERROR_INVALID_ARGUMENT);

    inEventParam.CommunicationError.error = WEAVE_ERROR_TIMEOUT;
    _this->mEventCallback(_this->mAppState, kEvent_CommunicationError, inEventParam, outEventParam);

    _this->Close();

exit:
    return;
}

void CommandSender::OnMessageReceived(ExchangeContext *aEC, const IPPacketInfo *aPktInfo, const WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CommandSender *_this = static_cast<CommandSender *>(aEC->AppState);
    StatusReporting::StatusReport status;
    InEventParam inEventParam;
    OutEventParam outEventParam;

    VerifyOrExit(aEC == _this->mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    // One way commands shouldn't get a response back
    VerifyOrExit(!(_this->mFlags & kCommandFlag_IsOneWay), err = WEAVE_ERROR_INCORRECT_STATE);

    if (aProfileId == kWeaveProfile_WDM && aMsgType == kMsgType_InProgress)
    {
        _this->mEventCallback(_this->mAppState, kEvent_InProgressReceived, inEventParam, outEventParam);
    }
    else if (aProfileId == kWeaveProfile_Common && aMsgType == Common::kMsgType_StatusReport)
    {
        err = StatusReporting::StatusReport::parse(aPayload, status);
        SuccessOrExit(err);

        inEventParam.StatusReportReceived.statusReport = &status;

        // Tell the application about the receipt of this status report.
        _this->mEventCallback(_this->mAppState, kEvent_StatusReportReceived, inEventParam, outEventParam);

        _this->Close();
    }
    else if (aProfileId == kWeaveProfile_WDM && aMsgType == kMsgType_CustomCommandResponse)
    {
        TLVReader reader;
        CustomCommandResponse::Parser response;
        uint64_t commandDataVersion;

        reader.Init(aPayload);

        err = reader.Next();
        SuccessOrExit(err);

        err = response.Init(reader);
        SuccessOrExit(err);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        // this function only print out all recognized properties in the response
        // check the parser class to see what else is available
        err = response.CheckSchemaValidity();
        SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

        err = response.GetVersion(&commandDataVersion);
        SuccessOrExit(err);

        // If we have a synchronized trait state object setup by the application, fill in the post command version
        // with the version provided in the response
        if (_this->mSyncronizedTraitState)
        {
            _this->mSyncronizedTraitState->mPostCommandVersion = commandDataVersion;
        }

        inEventParam.ResponseReceived.traitDataVersion = commandDataVersion;

        // Setup the reader on the response payload so that the app can parse it.
        err = response.GetReaderOnResponse(&reader);
        SuccessOrExit(err);

        inEventParam.ResponseReceived.reader = &reader;
        inEventParam.ResponseReceived.packetBuf = aPayload;
        _this->mEventCallback(_this->mAppState, kEvent_ResponseReceived, inEventParam, outEventParam);

        // Close it out
        _this->Close();
    }
    else {
        err = WEAVE_ERROR_INVALID_PROFILE_ID;
    }

exit:
    WeaveLogFunctError(err);

    if (err != WEAVE_NO_ERROR)
    {
        _this->Close();
    }

    if (aPayload)
    {
        PacketBuffer::Free(aPayload);
    }

    return;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING && WEAVE_CONFIG_ENABLE_WDM_CUSTOM_COMMAND_SENDER

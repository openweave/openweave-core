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
 *      This file implements Binding for Weave
 *      Data Management (WDM) profile.
 *
 *      This implementation is a stub, which should be replaced once
 *      we have the real Binding implementation.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

// Do nothing
ViewClient::ViewClient() { }

// AddRef to Binding
// store pointers to binding and app state
// null out EC
WEAVE_ERROR ViewClient::Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback)
{
    // initialize all pointers to NULL
    (void) Cancel();

    // add reference to the binding
    apBinding->AddRef();

    // make a copy of the pointers
    mBinding             = apBinding;
    mAppState            = apAppState;
    mEventCallback       = aEventCallback;
    mPrevIsPartialChange = false;
#if WDM_ENABLE_PROTOCOL_CHECKS
    mPrevTraitDataHandle = -1;
#endif

    mCurrentMode = kMode_Initialized;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ViewClient::SendRequest(TraitCatalogBase<TraitDataSink> * apCatalog, const TraitPath aPathList[],
                                    const size_t aPathListSize)
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * MsgBuf = NULL;
    SchemaVersionRange requestedSchemaVersionRange;

    VerifyOrExit(kMode_Initialized == mCurrentMode, err = WEAVE_ERROR_INCORRECT_STATE);

    mCurrentMode     = kMode_DataSink;
    mDataSinkCatalog = apCatalog;

    MsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != MsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    {
        nl::Weave::TLV::TLVWriter writer;
        nl::Weave::TLV::TLVType dummyContainerType;
        writer.Init(MsgBuf);

        err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyContainerType);
        SuccessOrExit(err);

        PathList::Builder pathList;
        err = pathList.Init(&writer, ViewRequest::kCsTag_PathList);
        SuccessOrExit(err);

        for (size_t i = 0; i < aPathListSize; ++i)
        {
            TraitDataSink * pDataSink;
            nl::Weave::TLV::TLVType dummyContainerType2;

            // Start the TLV Path
            err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Path, dummyContainerType2);
            SuccessOrExit(err);

            // Start, fill, and close the TLV Structure that contains ResourceID, ProfileID, and InstanceID
            err = mDataSinkCatalog->HandleToAddress(aPathList[i].mTraitDataHandle, writer, requestedSchemaVersionRange);

            if (err == WEAVE_ERROR_INVALID_ARGUMENT)
            {
                // HandleToAddress() can return an error if the sink has been removed from the catalog. In that case,
                // continue to next entry
                err = WEAVE_NO_ERROR;
                continue;
            }

            SuccessOrExit(err);

            if (mDataSinkCatalog->Locate(aPathList[i].mTraitDataHandle, &pDataSink) != WEAVE_NO_ERROR)
            {
                // Ideally, this code will not be reached as Locate() should find the entry in the catalog.
                // Otherwise, the earlier HandleToAddress() call would have continued.
                // However, keeping this check here for consistency and code safety
                continue;
            }

            // Append zero or more TLV tags based on the Path Handle
            err = pDataSink->GetSchemaEngine()->MapHandleToPath(aPathList[i].mPropertyPathHandle, writer);
            SuccessOrExit(err);

            // Close the TLV Path
            err = writer.EndContainer(dummyContainerType2);
            SuccessOrExit(err);
        }

        err = pathList.EndOfPathList().GetError();
        SuccessOrExit(err);

        err = writer.EndContainer(dummyContainerType);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);
    }

    err = mBinding->NewExchangeContext(mEC);
    if (WEAVE_NO_ERROR != err)
    {
        EventParam Param;
        // Nothing to initialize in Param.mRequestFailureEventParam
        mEventCallback(mAppState, kEvent_RequestFailed, err, Param);
        ExitNow();
    }

    mEC->AppState          = this;
    mEC->OnMessageReceived = OnMessageReceived;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError       = OnSendError;

    err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_ViewRequest, MsgBuf);
    MsgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != MsgBuf)
    {
        PacketBuffer::Free(MsgBuf);
        MsgBuf = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        Cancel();
    }

    return err;
}

// acquire EC from binding, kick off send message
WEAVE_ERROR ViewClient::SendRequest(AppendToPathList const aAppendToPathList, HandleDataElement const aHandleDataElement)
{
    WEAVE_ERROR err       = WEAVE_NO_ERROR;
    PacketBuffer * MsgBuf = NULL;

    VerifyOrExit(kMode_Initialized == mCurrentMode, err = WEAVE_ERROR_INCORRECT_STATE);

    mCurrentMode       = kMode_WithoutDataSink;
    mHandleDataElement = aHandleDataElement;

    MsgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != MsgBuf, err = WEAVE_ERROR_NO_MEMORY);

    {
        nl::Weave::TLV::TLVWriter writer;
        nl::Weave::TLV::TLVType dummyContainerType;
        writer.Init(MsgBuf);

        err = writer.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, dummyContainerType);
        SuccessOrExit(err);

        PathList::Builder pathList;
        err = pathList.Init(&writer, ViewRequest::kCsTag_PathList);
        SuccessOrExit(err);

        err = aAppendToPathList(mAppState, pathList);
        SuccessOrExit(err);

        err = pathList.EndOfPathList().GetError();
        SuccessOrExit(err);

        err = writer.EndContainer(dummyContainerType);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);
    }

    {
        const uint8_t * const begin = MsgBuf->Start();
        const uint8_t * const end   = MsgBuf->Start() + MsgBuf->DataLength();
        for (const uint8_t * pch = begin; pch < end; ++pch)
        {
            // WeaveLogDetail(DataManagement, "0x%02X", *pch);
        }
    }

    err = mBinding->NewExchangeContext(mEC);
    if (WEAVE_NO_ERROR != err)
    {
        EventParam Param;
        // Nothing to initialize in Param.mRequestFailureEventParam
        mEventCallback(mAppState, kEvent_RequestFailed, err, Param);
        ExitNow();
    }

    mEC->AppState          = this;
    mEC->OnMessageReceived = OnMessageReceived;
    mEC->OnResponseTimeout = OnResponseTimeout;
    mEC->OnSendError       = OnSendError;

    err    = mEC->SendMessage(nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_ViewRequest, MsgBuf);
    MsgBuf = NULL;
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    if (NULL != MsgBuf)
    {
        PacketBuffer::Free(MsgBuf);
        MsgBuf = NULL;
    }

    if (WEAVE_NO_ERROR != err)
    {
        Cancel();
    }

    return err;
}

// release binding, close EC, null out all pointers
WEAVE_ERROR ViewClient::Cancel()
{
    if (kMode_Canceled != mCurrentMode)
    {

        mEventCallback = NULL;

        if (NULL != mBinding)
        {
            mBinding->Release();
            mBinding = NULL;
        }

        if (NULL != mEC)
        {
            mEC->Close();
            mEC = NULL;
        }

        mHandleDataElement = NULL;

        // We still need these two variables when we're canceling all data sinks
        mCurrentMode = kMode_Canceled;
        mAppState    = NULL;
    }

    return WEAVE_NO_ERROR;
}

void ViewClient::OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ViewClient * const pViewClient = reinterpret_cast<ViewClient *>(aEC->AppState);
    void * const pAppState         = pViewClient->mAppState;
    EventCallback CallbackFunc     = pViewClient->mEventCallback;

    VerifyOrExit((kMode_DataSink == pViewClient->mCurrentMode) || (kMode_WithoutDataSink == pViewClient->mCurrentMode),
                 err = WEAVE_ERROR_INCORRECT_STATE);

    pViewClient->Cancel();

    EventParam Param;
    // Nothing to initialize in Param.mRequestFailureEventParam
    CallbackFunc(pAppState, kEvent_RequestFailed, aErrorCode, Param);

exit:
    WeaveLogFunctError(err);
}

void ViewClient::OnResponseTimeout(nl::Weave::ExchangeContext * aEC)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ViewClient * pViewClient   = reinterpret_cast<ViewClient *>(aEC->AppState);
    void * const pAppState     = pViewClient->mAppState;
    EventCallback CallbackFunc = pViewClient->mEventCallback;

    VerifyOrExit((kMode_DataSink == pViewClient->mCurrentMode) || (kMode_WithoutDataSink == pViewClient->mCurrentMode),
                 err = WEAVE_ERROR_INCORRECT_STATE);

    pViewClient->Cancel();

    EventParam Param;
    // Nothing to initialize in Param.mRequestFailureEventParam
    CallbackFunc(pAppState, kEvent_RequestFailed, WEAVE_ERROR_TIMEOUT, Param);

exit:
    WeaveLogFunctError(err);
}

void ViewClient::OnMessageReceived(nl::Weave::ExchangeContext * aEC, const nl::Inet::IPPacketInfo * aPktInfo,
                                   const nl::Weave::WeaveMessageInfo * aMsgInfo, uint32_t aProfileId, uint8_t aMsgType,
                                   PacketBuffer * aPayload)
{
    WEAVE_ERROR err            = WEAVE_NO_ERROR;
    ViewClient * pViewClient   = reinterpret_cast<ViewClient *>(aEC->AppState);
    void * const pAppState     = pViewClient->mAppState;
    EventCallback CallbackFunc = pViewClient->mEventCallback;
    EventParam Param;

    VerifyOrExit((kMode_DataSink == pViewClient->mCurrentMode) || (kMode_WithoutDataSink == pViewClient->mCurrentMode),
                 err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(aEC == pViewClient->mEC, err = WEAVE_ERROR_INCORRECT_STATE);

    if ((nl::Weave::Profiles::kWeaveProfile_Common == aProfileId) &&
        (nl::Weave::Profiles::Common::kMsgType_StatusReport == aMsgType))
    {
        pViewClient->Cancel();

        Param.mStatusReportReceivedEventParam.mMessage = aPayload;
        CallbackFunc(pAppState, kEvent_StatusReportReceived, WEAVE_ERROR_STATUS_REPORT_RECEIVED, Param);
    }
    else if ((nl::Weave::Profiles::kWeaveProfile_WDM == aProfileId) && (kMsgType_ViewResponse == aMsgType))
    {
        Param.mViewResponseReceivedEventParam.mEC      = aEC;
        Param.mViewResponseReceivedEventParam.mMessage = aPayload;
        CallbackFunc(pAppState, kEvent_ViewResponseReceived, WEAVE_NO_ERROR, Param);

        nl::Weave::TLV::TLVReader reader;
        nl::Weave::TLV::TLVType dummyContainerType;
        reader.Init(aPayload);

        err = reader.Next();
        SuccessOrExit(err);

        err = reader.EnterContainer(dummyContainerType);
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        DataList::Parser dataList;
        dataList.Init(reader);

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
        // simple schema checking
        err = dataList.CheckSchemaValidity();
        SuccessOrExit(err);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

        // re-initialize the reader to point to individual data element (reuse to save stack depth)
        dataList.GetReader(&reader);

        // TODO: Verify all paths in the original request has been fulfilled!

        bool isPartialChange = false;
        uint8_t flags;

        while (WEAVE_NO_ERROR == (err = reader.Next()))
        {
            // schema checking has been done earlier with the whole data list

            if (kMode_DataSink == pViewClient->mCurrentMode)
            {
                nl::Weave::TLV::TLVReader pathReader;

                {
                    DataElement::Parser element;

                    err = element.Init(reader);
                    SuccessOrExit(err);

                    err = element.GetReaderOnPath(&pathReader);
                    SuccessOrExit(err);

                    isPartialChange = false;
                    err             = element.GetPartialChangeFlag(&isPartialChange);
                    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );
                }

                {
                    Path::Parser path;

                    err = path.Init(pathReader);
                    SuccessOrExit(err);

                    err = path.GetTags(&pathReader);
                    SuccessOrExit(err);
                }

                TraitDataSink * DataSink;
                TraitDataHandle handle;
                PropertyPathHandle pathHandle;
                SchemaVersionRange requestedSchemaVersionRange;

                err = pViewClient->mDataSinkCatalog->AddressToHandle(pathReader, handle, requestedSchemaVersionRange);

                if (err == WEAVE_ERROR_INVALID_PROFILE_ID)
                {
                    // AddressToHandle() can return an error if the sink has been removed from the catalog. In that case,
                    // continue to next entry
                    err = WEAVE_NO_ERROR;
                    continue;
                }

                SuccessOrExit(err);

                if (pViewClient->mDataSinkCatalog->Locate(handle, &DataSink) != WEAVE_NO_ERROR)
                {
                    // Ideally, this code will not be reached as Locate() should find the entry in the catalog.
                    // Otherwise, the earlier AddressToHandle() call would have continued.
                    // However, keeping this check here for consistency and code safety
                    continue;
                }

                err = DataSink->GetSchemaEngine()->MapPathToHandle(pathReader, pathHandle);
#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
                // if we're not in strict compliance mode, we can ignore data elements that refer to paths we can't map due to
                // mismatching schema. The eventual call to StoreDataElement will correctly deal with the presence of a null
                // property path handle that has been returned by the above call. It's necessary to call into StoreDataElement with
                // this null handle to ensure the requisite OnEvent calls are made to the application despite the presence of an
                // unknown tag. It's also necessary to ensure that we update the internal version tracked by the sink.
                VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_TLV_TAG_NOT_FOUND, /* no-op */);
                if (err == WEAVE_ERROR_TLV_TAG_NOT_FOUND)
                {
                    WeaveLogDetail(DataManagement, "Ignoring un-mappable path!");
                    err = WEAVE_NO_ERROR;
                }
#else
                SuccessOrExit(err);
#endif

                pathReader = reader;
                flags      = 0;

#if WDM_ENABLE_PROTOCOL_CHECKS
                bool prevHandleMatches = (pViewClient->mPrevTraitDataHandle == handle);

                // Previous and current trait data handles can only match if we were previously encountered a partial change.
                // Otherwise, it shouldn't. If there is a violation here, it should be flagged.
                if (prevHandleMatches != pViewClient->mPrevIsPartialChange)
                {
                    WeaveLogError(DataManagement, "Encountered partial change flag violation (%u, %08x, %08x)",
                                  pViewClient->mPrevIsPartialChange, pViewClient->mPrevTraitDataHandle, handle);
                    err = WEAVE_ERROR_INVALID_DATA_LIST;
                    goto exit;
                }
#endif

                if (!pViewClient->mPrevIsPartialChange)
                {
                    flags = TraitDataSink::kFirstElementInChange;
                }

                if (!isPartialChange)
                {
                    flags |= TraitDataSink::kLastElementInChange;
                }

                err = DataSink->StoreDataElement(pathHandle, pathReader, flags, NULL, NULL);
                SuccessOrExit(err);

                pViewClient->mPrevIsPartialChange = isPartialChange;

#if WDM_ENABLE_PROTOCOL_CHECKS
                // it's important to clear out mPrevTraitDataHandle if this isn't a partial change so that an ensuing notify
                // that has the first data element pointing to the same trait data instance doesn't trip up the (if
                // prevHandleMatches != mPrevIsPartialChange) logic above.
                if (!isPartialChange)
                {
                    pViewClient->mPrevTraitDataHandle = -1;
                }
                else
                {
                    pViewClient->mPrevTraitDataHandle = handle;
                }
#endif
            }
            else if (kMode_WithoutDataSink == pViewClient->mCurrentMode)
            {
                DataElement::Parser element;
                err = element.Init(reader);
                SuccessOrExit(err);

                err = pViewClient->mHandleDataElement(pViewClient->mAppState, element);
                SuccessOrExit(err);
            }
        }

        // if we have exhausted this container
        if (WEAVE_END_OF_TLV == err)
        {
            err = WEAVE_NO_ERROR;
        }

        err = reader.ExitContainer(dummyContainerType);
        SuccessOrExit(err);

        pViewClient->Cancel();

        Param.mViewResponseConsumedEventParam.mMessage = aPayload;
        CallbackFunc(pAppState, kEvent_ViewResponseConsumed, WEAVE_NO_ERROR, Param);
    }
    else
    {
        pViewClient->Cancel();

        // Nothing to initialize in Param.mRequestFailureEventParam
        CallbackFunc(pAppState, kEvent_RequestFailed, WEAVE_ERROR_INVALID_MESSAGE_TYPE, Param);
    }

exit:
    WeaveLogFunctError(err);

    // aEC should be the same as pViewClient->mEC and be closed in InternalCancel
    // If they are not the same, we're in big trouble
    pViewClient->Cancel();
    aEC = NULL;

    if (NULL != aPayload)
    {
        PacketBuffer::Free(aPayload);
        aPayload = NULL;
    }
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

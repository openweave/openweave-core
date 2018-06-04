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
 *      This file defines the generic Update Client for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_UPDATE_CLIENT_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_UPDATE_CLIENT_CURRENT_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Profiles/data-management/EventLoggingTypes.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class UpdateEncoder
{
public:
    UpdateEncoder() {}
    ~UpdateEncoder() {}

    typedef WEAVE_ERROR (*AddArgumentCallback)(UpdateClient * apClient, void *apCallState, TLV::TLVWriter & aOuterWriter);

    typedef WEAVE_ERROR (*AddElementCallback)(UpdateClient * apClient, void *apCallState, TLV::TLVWriter & aOuterWriter);

    WEAVE_ERROR StartUpdate(utc_timestamp_t aExpiryTimeMicroSecond, AddArgumentCallback aAddArgumentCallback, uint32_t maxUpdateSize);

    WEAVE_ERROR AddElement(const uint32_t & aProfileID,
                           const uint64_t & aInstanceID,
                           const ResourceIdentifier & aResourceID,
                           const DataVersion & aRequiredDataVersion,
                           const SchemaVersionRange * aSchemaVersionRange,
                           const uint64_t *aPathArray,
                           const size_t aPathLength,
                           AddElementCallback aAddElementCallback,
                           void * aCallState);

    WEAVE_ERROR StartElement(const uint32_t & aProfileID,
                             const uint64_t & aInstanceID,
                             const ResourceIdentifier & aResourceID,
                             const DataVersion & aRequiredDataVersion,
                             const SchemaVersionRange * aSchemaVersionRange,
                             const uint64_t *aPathArray,
                             const size_t aPathLength);

    WEAVE_ERROR FinalizeElement();

    WEAVE_ERROR CancelElement(TLV::TLVWriter & aOuterWriter);

    void Checkpoint(TLV::TLVWriter &aWriter);

    void Rollback(TLV::TLVWriter &aWriter);

    WEAVE_ERROR StartDataList(void);

    WEAVE_ERROR EndDataList(void);

    WEAVE_ERROR StartUpdateRequest(utc_timestamp_t aExpiryTimeMicroSecond);

    WEAVE_ERROR EndUpdateRequest(void);

    WEAVE_ERROR AddExpiryTime(utc_timestamp_t aExpiryTimeMicroSecond);

    WEAVE_ERROR AddUpdateRequestIndex(void);

    PacketBuffer *GetPBuf() const { return mBuf; mBuf = NULL; }

    void Cancel();



private:
    enum UpdateEncoderState
    {
        kState_Ready,                //< The encoder has been initialized and is ready
        kState_EncodingDataList,     //< The encoder has opened the DataList; DataElements can be added
        kState_EncodingDataElement,  //< The encoder is encoding a DataElement
        kState_Done,                 //< The DataList has been closed
    };
    TLV::TLVWriter mWriter;
    UpdateEncoderState mState;
    PacketBuffer *mBuf;
    AddArgumentCallback mAddArgumentCallback;
}

class UpdateClient
{
public:
    struct InEventParam;
    struct OutEventParam;

    enum EventType
    {
        kEvent_UpdateComplete = 1,
        kEvent_UpdateContinue = 2,
    };

    enum UpdateClientState
    {
        kState_Uninitialized = 0,    //< The update client has not been initialized
        kState_Initialized,          //< The update client has been initialized and is ready
        kState_AwaitingResponse,     //< The update client has sent the update request, and pending for response
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    static void DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    UpdateClient(void);

    WEAVE_ERROR Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback);

    WEAVE_ERROR Shutdown(void);

    WEAVE_ERROR CancelUpdate(void);

    WEAVE_ERROR SendUpdate(bool aIsPartialUpdate);

    void * mpAppState;

private:

    Binding * mpBinding;
    EventCallback mEventCallback;
    nl::Weave::ExchangeContext * mEC;
    UpdateClientState mState;
    PacketBuffer * mpBuf;
    utc_timestamp_t mExpiryTimeMicroSecond;
    uint32_t mUpdateRequestIndex;

    nl::Weave::TLV::TLVType mDataListContainerType, mDataElementContainerType;

    static void OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext);
    static void OnResponseTimeout(nl::Weave::ExchangeContext * aEC);
    static void OnMessageReceived(nl::Weave::ExchangeContext * aEC,
                                  const nl::Inet::IPPacketInfo * aPktInfo,
                                  const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                  uint32_t aProfileId,
                                  uint8_t aMsgType,
                                  PacketBuffer * aPayload);

    void MoveToState(const UpdateClientState aTargetState);

    const char * GetStateStr(void) const;

    void ClearState(void);

    static void BindingEventCallback(void * const apAppState, const Binding::EventType aEvent,
                                     const Binding::InEventParam & aInParam, Binding::OutEventParam & aOutParam);

    void FlushExistingExchangeContext(const bool aAbortNow = false);
};

struct UpdateClient::InEventParam
{

    UpdateClient *Source;
    union
    {
        struct
        {
            WEAVE_ERROR Reason;
            StatusReporting::StatusReport * StatusReportPtr;
        } UpdateComplete;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

struct UpdateClient::OutEventParam
{
    bool DefaultHandlerCalled;
    union
    {
    };
    void Clear() { memset(this, 0, sizeof(*this)); }
};
}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_UPDATE_CLIENT_CURRENT_H

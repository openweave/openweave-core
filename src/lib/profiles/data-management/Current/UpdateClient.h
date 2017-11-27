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

class UpdateClient
{
public:

    struct InEventParam;
    struct OutEventParam;

    enum
    {
        kNoTimeOut = 0,

        // Note the WDM spec says 0x7FFFFFFF, but Weave implementation can only hold timeout of much shorter
        // 32-bit in milliseconds, which is about 1200 hours
        kMaxTimeoutSec = 3600000,
    };

    enum EventType
    {
        kEvent_UpdateComplete = 1,
    };

    enum UpdateClientState
    {
        kState_Uninitialized = 0,    //< The update client has not been initialized
        kState_Initialized,          //< The update client has been initialized and is ready
        kState_BuildDataList,        //< The update client is ready to build the DataList portion of the structure
        kState_BuildDataElement,     //< The update client is ready to build the Data Element in data list
        kState_AwaitingResponse,     //< The update client has sent the update request, and pending for response
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    static void DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    typedef WEAVE_ERROR (*AddArgumentCallback)(UpdateClient * apClient, void *apCallState, TLV::TLVWriter & aOuterWriter);

    typedef WEAVE_ERROR (*AddElementCallback)(UpdateClient * apClient, void *apCallState, TLV::TLVWriter & aOuterWriter);

    UpdateClient(void);

    WEAVE_ERROR Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback);

    WEAVE_ERROR Shutdown(void);

    WEAVE_ERROR StartUpdate(PacketBuffer * aBuf, utc_timestamp_t aExpiryTimeMicroSecond, AddArgumentCallback aAddArgumentCallback);

    WEAVE_ERROR CancelUpdate(void);

    WEAVE_ERROR AddElement(const uint32_t & aProfileID,
                           const uint64_t & aInstanceID,
                           const uint64_t & aResourceID,
                           const DataVersion & aRequiredDataVersion,
                           const SchemaVersionRange * aSchemaVersionRange,
                           const uint64_t *aPathArray,
                           const size_t aPathLength,
                           AddElementCallback aAddElementCallback,
                           void * aCallState);

    WEAVE_ERROR StartElement(const uint32_t & aProfileID,
                             const uint64_t & aInstanceID,
                             const uint64_t & aResourceID,
                             const DataVersion & aRequiredDataVersion,
                             const SchemaVersionRange * aSchemaVersionRange,
                             const uint64_t *aPathArray,
                             const size_t aPathLength,
                             TLV::TLVWriter & aOuterWriter);

    WEAVE_ERROR FinalizeElement(TLV::TLVWriter & aOuterWriter);

    WEAVE_ERROR CancelElement(TLV::TLVWriter & aOuterWriter);

    WEAVE_ERROR SendUpdate(void);

    void * mAppState;

private:

    TLV::TLVWriter mWriter;
    UpdateClientState mState;
    PacketBuffer * mBuf;
    Binding * mBinding;
    EventCallback mEventCallback;
    utc_timestamp_t mExpiryTimeMicroSecond;
    AddArgumentCallback mAddArgumentCallback;

    nl::Weave::ExchangeContext * mEC;

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

    WEAVE_ERROR Checkpoint(TLV::TLVWriter &aWriter);

    WEAVE_ERROR StartDataList(void);

    WEAVE_ERROR EndDataList(void);

    WEAVE_ERROR StartUpdateRequest(utc_timestamp_t aExpiryTimeMicroSecond);

    WEAVE_ERROR EndUpdateRequest(void);

    static void BindingEventCallback(void * const apAppState, const Binding::EventType aEvent,
                                     const Binding::InEventParam & aInParam, Binding::OutEventParam & aOutParam);

    WEAVE_ERROR AddExpiryTime(utc_timestamp_t aExpiryTimeMicroSecond);

    void FlushExistingExchangeContext(const bool aAbortNow = false);
};

struct UpdateClient::InEventParam
{
    union
    {
        struct
        {
            WEAVE_ERROR Reason;
            PacketBuffer * mMessage;
            UpdateClient * mpClient;
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

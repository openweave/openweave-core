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
#include <Weave/Profiles/data-management/EventLoggingTypes.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

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
        kState_Uninitialized = 0,    ///< The update client has not been initialized
        kState_Initialized,          ///< The update client has been initialized and is ready
        kState_AwaitingResponse,     ///< The update client has sent the update request, and pending for response
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    static void DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam & aInParam, OutEventParam & aOutParam);

    UpdateClient(void);

    WEAVE_ERROR Init(Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback);

    WEAVE_ERROR Shutdown(void);

    void CloseUpdate(bool aAbort);

    void CancelUpdate(void);

    WEAVE_ERROR SendUpdate(bool aIsPartialUpdate, PacketBuffer *aPBuf, bool aIsFirstPayload);

    void * mpAppState;

    Binding * mpBinding;
private:

    EventCallback mEventCallback;
    nl::Weave::ExchangeContext * mEC;
    UpdateClientState mState;
    utc_timestamp_t mExpiryTimeMicroSecond;

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

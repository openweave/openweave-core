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
 *      This file defines the generic View Client for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_VIEW_CLIENT_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_VIEW_CLIENT_CURRENT_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/MessageDef.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class ViewClient
{
public:

    enum EventID
    {
        // Cancel is already called when this callback happens
        // Could be any reason the request failed, (WRM ACK missing, EC allocation failure, response timeout,...)
        // Check error code, mEC may be valid or NULL
        // WEAVE_ERROR_INVALID_MESSAGE_TYPE if some unrecognized message is received
        // WEAVE_ERROR_TIME_OUT if
        kEvent_RequestFailed                = 1,

        // Last chance to adjust EC, mEC is valid and can be tuned for timeout settings
        kEvent_AboutToSendRequest           = 2,

        // Response just arrived, mEC is valid
        kEvent_ViewResponseReceived         = 3,

        // Cancel is already called when this callback happens
        // Response processing has been completed, InternalCancel will be called upon return
        kEvent_ViewResponseConsumed         = 4,

        // Cancel is already called when this callback happens
        // Status Report as response just arrived, mEC is valid, InternalCancel will be called upon return
        kEvent_StatusReportReceived         = 5,
    };

    // union of structures for each event some of them might be empty
    union EventParam
    {
        // kEvent_RequestFailed
        struct
        {
        } mRequestFailureEventParam;

        // kEvent_AboutToSendRequest
        struct
        {
            // Do not close the EC
            nl::Weave::ExchangeContext * mEC;
        } mAboutToSendRequestEventParam;

        // kEvent_ViewResponseReceived
        struct
        {
            // Do not close the EC
            nl::Weave::ExchangeContext * mEC;

            // Do not modify the message content
            PacketBuffer * mMessage;
        } mViewResponseReceivedEventParam;

        // kEvent_ViewResponseConsumed
        struct
        {
            // Do not modify the message content
            PacketBuffer * mMessage;
        } mViewResponseConsumedEventParam;

        // kEvent_StatusReportReceived
        struct
        {
            // Do not modify the message content
            PacketBuffer * mMessage;
        } mStatusReportReceivedEventParam;
    };

    typedef void (*EventCallback) (void * const aAppState, EventID aEvent, WEAVE_ERROR aErrorCode, EventParam & aEventParam);


    // Do nothing
    ViewClient (void);

    // AddRef to Binding
    // store pointers to binding and delegate
    // null out EC
    WEAVE_ERROR Init (Binding * const apBinding, void * const apAppState, EventCallback const aEventCallback);

    typedef WEAVE_ERROR (*AppendToPathList) (void * const apAppState, PathList::Builder & aPathList);
    typedef WEAVE_ERROR (*HandleDataElement) (void * const apAppState, DataElement::Parser & aDataElement);

    // acquire EC from binding, kick off send message
    WEAVE_ERROR SendRequest (AppendToPathList const aAppendToPathList, HandleDataElement const aHandleDataElement);

    WEAVE_ERROR SendRequest (TraitCatalogBase<TraitDataSink>* apCatalog, const TraitPath aPathList[], const size_t aPathListSize);

    // InternalCancel(false)
    WEAVE_ERROR Cancel (void);

protected:

    enum
    {
        kMode_Canceled          = 1,
        kMode_Initialized       = 2,
        kMode_DataSink          = 3,
        kMode_WithoutDataSink   = 4,
    } mCurrentMode;

    Binding * mBinding;
    void * mAppState;
    EventCallback mEventCallback;
    bool mPrevIsPartialChange;
#if WDM_ENABLE_PROTOCOL_CHECKS
    TraitDataHandle mPrevTraitDataHandle;
#endif

    // We need to keep this for canceling before we receive a response
    nl::Weave::ExchangeContext * mEC;

    union
    {
        // Only needed if we want to use this View Client without data sinks
        HandleDataElement mHandleDataElement;

        // Only needed if we want to use this View Client with data sinks
        TraitCatalogBase<TraitDataSink> *mDataSinkCatalog;
    };

    static void DataSinkOperation_NoMoreData (void * const apOpState, TraitDataSink * const apDataSink);

    static void OnSendError (ExchangeContext *aEC, WEAVE_ERROR aErrorCode, void *aMsgSpecificContext);
    static void OnResponseTimeout (nl::Weave::ExchangeContext *aEC);
    static void OnMessageReceived (nl::Weave::ExchangeContext *aEC, const nl::Inet::IPPacketInfo *aPktInfo,
        const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId,
        uint8_t aMsgType, PacketBuffer *aPayload);
};


}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_VIEW_CLIENT_CURRENT_H

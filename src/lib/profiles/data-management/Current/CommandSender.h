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
 *      This file defines command handle for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_COMMAND_SENDER_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_COMMAND_SENDER_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/Current/TraitCatalog.h>
#include <Weave/Profiles/data-management/Current/Command.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 *  @class CommandSender
 *
 *  @note This class encapsulates the protocol details of sending commands, simplifying the mechanics involved for
 *        applications. The application provides a PacketBuffer containing the payload of the command as well as
 *        an optional set of arguments that alter the contents of the command header as well as the behavior of the command.
 *
 *        The utility of this wrapper around command handling is indeed limited, mainly due to the complexity/flexibility involved
 *        in security validation and data serialization/de-serialization.
 *
 *        The details for command validation is still TBD
 *
 *        This class also helps applications infer if the data within an associated TraitDataSink has caught up to the side-effects
 *        of the commmand (based on the version provided in the command response). The application is responsible for managing the
 *        storage of that object.
 */
class CommandSender
{
public:
    enum EventType
    {
        kEvent_CommunicationError                 = 1,        // All errors during communication (transmission error, failure to get a response timeout) will be communicated through this event
        kEvent_InProgressReceived                 = 2,        // Receipt of an 'in progress' message
        kEvent_StatusReportReceived               = 3,        // On receipt of a status report
        kEvent_ResponseReceived                   = 4,        // On receipt of a command response

        kEvent_DefaultCheck                         = 100,    //< Used to verify correct default event handling in the application.
    };

    struct InEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

        union {
            struct
            {
                Profiles::StatusReporting::StatusReport *statusReport;
            } StatusReportReceived;

            /*
             * All communication errors, including transport errors on transmission
             * as well as failure to receive a response (WEAVE_ERROR_TIMEOUT).
             */
            struct
            {
                WEAVE_ERROR error;
            } CommunicationError;

            /* On receipt of a command response, both a reader positioned at the
             * payload as well as a pointer to the packet buffer is provided. If the applicaton
             * desires to hold on to the buffer, it can do so my incrementing the ref count on the buffer
             */
            struct
            {
                uint64_t traitDataVersion;
                nl::Weave::TLV::TLVReader *reader;
                nl::Weave::PacketBuffer *packetBuf;
            } ResponseReceived;
        };
    };

    struct OutEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }
        bool defaultHandlerCalled;
    };

    typedef void (*EventCallback)(void * const aAppState, EventType aEvent, const InEventParam &aInParam, OutEventParam &aOutEventParam);

    struct Args {
        WEAVE_ERROR PopulateTraitPath(TraitCatalogBase<TraitDataSink> *aCatalog, TraitDataSink *aSink, uint32_t aCommandType);

        TraitDataSink *mSink;

        ResourceIdentifier mResourceId;
        uint32_t mProfileId;
        SchemaVersionRange mVersionRange;
        uint64_t mInstanceId;
        uint32_t mCommandType;

        uint8_t mFlags;
        uint64_t mMustBeVersion;
        uint64_t mInitiationTimeMicroSecond;
        uint64_t mActionTimeMicroSecond;
        uint64_t mExpiryTimeMicroSecond;

        // Set to non-zero number to specify command timeout that's different than the one specified in the binding
        uint32_t mResponseTimeoutMsOverride;
    };

public:
    struct SynchronizedTraitState;

    static void DefaultEventHandler(void *aAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);
    WEAVE_ERROR Init(nl::Weave::Binding *aBinding, const EventCallback aEventCallback, void * const aAppState);
    WEAVE_ERROR SendCommand(nl::Weave::PacketBuffer *aPayload, nl::Weave::Binding *aBinding, ResourceIdentifier &aResourceId, uint32_t aProfileId, uint32_t aCommandType);
    WEAVE_ERROR SendCommand(nl::Weave::PacketBuffer *aPayload, nl::Weave::Binding *aBinding, Args &aArgs);
    void Close(bool aAbortNow = false);
    void SetSynchronizedTraitState(SynchronizedTraitState *aTraitState);

    SynchronizedTraitState *mSyncronizedTraitState;
    void *mAppState;

private:
    static void OnMessageReceived(nl::Weave::ExchangeContext *aEC, const nl::Weave::IPPacketInfo *aPktInfo, const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, nl::Weave::PacketBuffer *aPayload);
    static void OnResponseTimeout(nl::Weave::ExchangeContext *aEC);
    static void OnSendError(nl::Weave::ExchangeContext *aEC, WEAVE_ERROR err, void *aMsgCtxt);

    EventCallback mEventCallback;
    nl::Weave::Binding *mBinding;
    nl::Weave::PacketBuffer *mPacketBuf;
    nl::Weave::ExchangeContext *mEC;
    uint8_t mFlags;
};

/**
 *  @class CommandSender::SynchronizedTraitState
 *
 *  @note  This class helps inform if an associated TraitDataSink has caught up to all the side-effects of a command.
 *         The CommandSender class is responsible for filling in the requisite information necessary at the time of
 *         request transmission and response reception.
 *
 *         The application can use this in one of two modalities:
 *         a) Have a valid data version in the data sink before starting to send commands
 *         b) Never having a valid data version before starting to send commands.
 *
 *         In the former case, the version of the sink prior to sending the command is known, allowing for an accurate
 *         later inference of whether the sink has caught up.
 *
 *         In the latter case, the absence of a prior version results in the logic to infer synchronization reverting
 *         to a window-based heuristic. This is due to the presence of randomized data versions that can result in the
 *         received data version from the publisher jumping to a lower number post command reception.
 *
 *         For more details, see https://docs.google.com/document/d/1r9AKKUKqzAzyBUoS2A4iNgUmb4loWdt7k7u7E-spxFo/edit#heading=h.o7axznd49f5n
 *
 */

struct CommandSender::SynchronizedTraitState
{
    WEAVE_ERROR Init();
    const bool HasDataCaughtUp(void);

private:
    // If we dont' have a pre version, it means we're still in the act of subscribing after having received the command response.
    // We have to make take a guess at what the pre version could be relative to what the received command response version is.
    // We'll need to define a 'stale window' backward of the received command response version that defines a region of versions that don't contain
    // the side-effects of the command.
    //
    // The case that drives the sizing of this window is if we receive a notify from the responder containing data prior to the command being received on the responder,
    // but the notify arriving after the command has been transmitted to the responder. Later when we get the response (with the notify arriving either before or after the response),
    // we need to ensure we still infer correctly that the data sink has not caught up.
    //
    // The worst case is if the notify was retransmitted a couple of times (but not failing). We have to figure out the effective number of data changes (i.e version up-ticks)
    // during that window of time.
    //
    // Scenario:
    //
    // 1. Responder sends out notify to receiver that doesn't contain the side-effects of the command. Data version = 10 on the trait source.
    // 2. It gets re-transmitted a couple of times (up to say 10s) before the last one gets through.
    // 3. In this time, a 100Hz temperature sensor has been ticking the version away, the version being = 1010 by now.
    // 4. The command request is received and a response is sent back (v = 1011).
    // 5. When the stale notify finally makes it to the receiver, we need to infer that 10 is still stale (i.e not caught up).
    //
    static const uint32_t kCommandSideEffectWindowSize = 1000;

    uint64_t mPreCommandVersion;
    uint64_t mPostCommandVersion;
    TraitDataSink *mDataSink;

    friend class CommandSender;
    friend class TestTdm;
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_COMMAND_CURRENT_H

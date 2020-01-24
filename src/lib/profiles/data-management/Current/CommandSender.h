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
 *
 * ## Weave Binding
 *
 * An object of this class can be intialized with a Weave Binding, which will serve as the default
 * Binding to use to send Commands. The user may also provide a Binding to each call of
 * SendCommand(), which will override the default Binding. It is not necessary to provide a default
 * Binding, however any binding provided to CommandSender must already be initialized.
 *
 * ## EventHandler
 *
 * The user must define a function of this type if they wish to be updated about
 * events that happen after the sending of the command (see "API Events" below).
 * This can be NULL if the user does not care what happens after the command is
 * sent.
 *
 * ## API Events
 *
 * The following events are the possible outcomes after sending a Command:
 *
 * ### CommunicationError
 *
 * An error occurred while forming or sending the Command, or while waiting for a response.
 * Examples of errors that can occur while waiting for a response are key errors or unexpected close
 * of a connection. The error reason will be contained in the InEventParam argument to the
 * EventCallback handler.
 *
 * ### InProgressReceived
 *
 * The recipient can send an 'in progress' message which signifies that the Command has been
 * receieved, but not yet completed. Once completed, the recipient will send a Response or
 * StatusReport. Sending an 'in progress' message is not required.
 *
 * ### StatusReportReceived
 *
 * Receipt of a StatusReport implies that there was an error in processing the Command. The
 * StatusReport can be accessed through the InEventParam.
 *
 * ### ResponseReceived
 *
 * Receiving a Response implies that the Command recipient handled the Command successfully. The
 * Response may contain a payload or it may not. If the applcation desires to keep the packet
 * buffer, it may call ExchangeContext::AddRef() to increment the ref count.
 *
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

    // Data returned to the sender by the recipient of the custom command.
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
             * desires to hold on to the buffer, it can do so by incrementing the ref count on the buffer
             */
            struct
            {
                uint64_t traitDataVersion;
                nl::Weave::TLV::TLVReader *reader;
                nl::Weave::PacketBuffer *packetBuf;
            } ResponseReceived;
        };
    };

    // Data returned back to a CommandSender object from an EventCallback
    // function.
    struct OutEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }
        bool defaultHandlerCalled;
    };

    typedef void (*EventCallback)(void * const aAppState, EventType aEvent, const InEventParam &aInParam, OutEventParam &aOutEventParam);

    struct SendParams {
        WEAVE_ERROR PopulateTraitPath(TraitCatalogBase<TraitDataSink> *aCatalog, TraitDataSink *aSink, uint32_t aCommandType);

        TraitDataSink *Sink;

        ResourceIdentifier ResourceId;
        uint32_t ProfileId;
        SchemaVersionRange VersionRange;
        uint64_t InstanceId;
        uint32_t CommandType;

        uint8_t Flags;
        uint64_t MustBeVersion;
        uint64_t InitiationTimeMicroSecond;
        uint64_t ActionTimeMicroSecond;
        uint64_t ExpiryTimeMicroSecond;

        // Set to non-zero number to specify command timeout that's different than the one specified in the binding
        uint32_t ResponseTimeoutMsOverride;
    };

public:
    struct SynchronizedTraitState;

    static void DefaultEventHandler(void *aAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);
    WEAVE_ERROR Init(nl::Weave::Binding *aBinding, const EventCallback aEventCallback, void * const aAppState);
    WEAVE_ERROR SendCommand(nl::Weave::PacketBuffer *aPayload, nl::Weave::Binding *aBinding, ResourceIdentifier &aResourceId, uint32_t aProfileId, uint32_t aCommandType);
    WEAVE_ERROR SendCommand(nl::Weave::PacketBuffer *aPayload, nl::Weave::Binding *aBinding, SendParams &aSendParams);
    void Close(bool aAbortNow = false);
    void SetSynchronizedTraitState(SynchronizedTraitState *aTraitState);

    SynchronizedTraitState *mSyncronizedTraitState = NULL;
    void *mAppState = NULL;

private:
    static void OnMessageReceived(nl::Weave::ExchangeContext *aEC, const nl::Weave::IPPacketInfo *aPktInfo, const nl::Weave::WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, nl::Weave::PacketBuffer *aPayload);
    static void OnResponseTimeout(nl::Weave::ExchangeContext *aEC);
    static void OnSendError(nl::Weave::ExchangeContext *aEC, WEAVE_ERROR err, void *aMsgCtxt);

    EventCallback mEventCallback;
    nl::Weave::Binding *mBinding = NULL;
    nl::Weave::PacketBuffer *mPacketBuf = NULL;
    nl::Weave::ExchangeContext *mEC = NULL;
    uint8_t mFlags = 0;
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

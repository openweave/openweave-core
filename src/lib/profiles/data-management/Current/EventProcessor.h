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
 *  @file
 *
 *  @brief
 *    EventProcessor base class.
 *
 *  This file contains the declaration of the EventProcessor base class.
 *  It's intended to be overridden on specific platforms as needed.
*/
#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_PROCESSOR_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_PROCESSOR_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class SubscriptionClient;

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

class EventProcessor
{
public:
    EventProcessor(uint64_t inLocalNodeId);
    virtual ~EventProcessor(void);

    WEAVE_ERROR ProcessEvents(nl::Weave::TLV::TLVReader &inReader,
                              nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient);

public:
    struct EventHeader
    {
        // Not a simple POD anymore given that we have a constructor for mDataSchemaVersionRange now.
        EventHeader() : mSource(0), mImportance(kImportanceType_Invalid), mId(0), mRelatedImportance(kImportanceType_Invalid),
                            mRelatedId(0), mUTCTimestamp(0), mSystemTimestamp(0), mResourceId(0), mTraitProfileId(0),
                            mTraitInstanceId(0), mType(0), mDeltaUTCTime(0), mDeltaSystemTime(0), mPresenceMask(0)
        { }

        uint64_t mSource;
        ImportanceType mImportance;
        uint64_t mId;

        ImportanceType mRelatedImportance;
        uint64_t mRelatedId;
        uint64_t mUTCTimestamp;
        uint64_t mSystemTimestamp;
        uint64_t mResourceId;
        uint64_t mTraitProfileId;
        uint64_t mTraitInstanceId;
        uint64_t mType;

        int64_t mDeltaUTCTime;
        int64_t mDeltaSystemTime;

        uint64_t mPresenceMask;
        SchemaVersionRange mDataSchemaVersionRange;
    };

protected:
    // Only the optional fields are represented here.  See the table
    // "Tags in TLV elements for events" in the WDM Next Protocol
    // spec.
    enum
    {
        EventHeaderFieldPresenceMask_RelatedImportance = 0x0001,
        EventHeaderFieldPresenceMask_RelatedId         = 0x0002,
        EventHeaderFieldPresenceMask_UTCTimestamp      = 0x0004,
        EventHeaderFieldPresenceMask_SystemTimestamp   = 0x0008,
        EventHeaderFieldPresenceMask_DeltaUTCTime      = 0x0010,
        EventHeaderFieldPresenceMask_DeltaSystemTime   = 0x0020,
    };

    // This includes all possible kCsTag_* fields, telling us what
    // actually came in over the wire.
    enum
    {
        ReceivedEventHeaderFieldPresenceMask_Source            = 0x0001,
        ReceivedEventHeaderFieldPresenceMask_Importance        = 0x0002,
        ReceivedEventHeaderFieldPresenceMask_Id                = 0x0004,
        ReceivedEventHeaderFieldPresenceMask_RelatedImportance = 0x0008,
        ReceivedEventHeaderFieldPresenceMask_RelatedId         = 0x0010,
        ReceivedEventHeaderFieldPresenceMask_UTCTimestamp      = 0x0020,
        ReceivedEventHeaderFieldPresenceMask_SystemTimestamp   = 0x0040,
        ReceivedEventHeaderFieldPresenceMask_TraitInstanceId   = 0x0080,
        ReceivedEventHeaderFieldPresenceMask_Type              = 0x0100,
        ReceivedEventHeaderFieldPresenceMask_DeltaUTCTime      = 0x0200,
        ReceivedEventHeaderFieldPresenceMask_DeltaSystemTime   = 0x0400,
        ReceivedEventHeaderFieldPresenceMask_Data              = 0x0800,
    };

    //
    // The "WDM Next Protocol" spec defines three categories of event
    // header fields (my terms below):
    //
    // (1) Mandatory              - Every event in the stream is sent
    //                              over the wire with this field in it's
    //                              header.
    //
    // (2) Provisionally Optional - Field may not be present on each event
    //                              in the stream, but we are required to
    //                              provide a value for it when we present
    //                              it to the user.  As far as the downstream
    //                              consumer of events is concerned, the field
    //                              is mandatory.
    //
    // (3) Purely Optional        - Field may not be present on any event
    //                              in the stream, and we're not required
    //                              to provide a value to the user.
    //
    // For header fields of type (2) above, this structure keeps track of
    // any information required from previously-parsed headers that might
    // need to be used to provide values before presenting events to the
    // user.
    //

    struct StreamParsingContext
    {
        StreamParsingContext(uint64_t inPublisherSourceId) :
            mPublisherSourceId(inPublisherSourceId),
            mCurrentEventImportance(kImportanceType_Invalid),
            mCurrentEventId(0),
            mCurrentEventType(0),
            mCurrentSystemTimestamp(0),
            mCurrentUTCTimestamp(0)
        {
        }

        // The Weave node ID of the publisher..
        uint64_t mPublisherSourceId;

        // The last event importance we parsed.
        ImportanceType mCurrentEventImportance;

        // The most recently used event ID.
        uint64_t mCurrentEventId;

        // The last event type we parsed.
        uint64_t mCurrentEventType;

        // The last system timestamp we parsed.
        uint64_t mCurrentSystemTimestamp;

        // The last UTC timestamp we parsed.
        uint64_t mCurrentUTCTimestamp;
    };

    WEAVE_ERROR ParseEventList(nl::Weave::TLV::TLVReader &inReader,
                               nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient);
    WEAVE_ERROR ParseEvent(nl::Weave::TLV::TLVReader &inReader,
                           nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient,
                           StreamParsingContext &inOutParsingContext);
    WEAVE_ERROR MapReceivedMaskToPublishedMask(const uint64_t &inReceivedMask,
                                               uint64_t &inOutPublishedMask);
    WEAVE_ERROR UpdateContextQualifyHeader(EventHeader &inOutEventHeader,
                                           StreamParsingContext &inOutContext,
                                           uint64_t inReceivedMask);
    WEAVE_ERROR UpdateGapDetection(const EventHeader &inEventHeader);

    virtual WEAVE_ERROR ProcessEvent(nl::Weave::TLV::TLVReader inReader,
                                     nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient,
                                     const EventHeader &inEventHeader) = 0;

    virtual WEAVE_ERROR GapDetected(const EventHeader &inEventHeader) = 0;

    uint64_t mLocalNodeId;

    event_id_t mLastEventId[kImportanceType_Last - kImportanceType_First + 1];

};

#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

typedef void * EventProcessor;

#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
#endif // _WEAVE_DATA_MANAGEMENT_EVENT_PROCESSOR_CURRENT_H

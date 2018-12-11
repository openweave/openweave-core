/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 * @file
 *
 * @brief
 *   Enums, types, and tags used in Weave Event Logging.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TAGS_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TAGS_CURRENT_H

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

// Tags used in LoggingConfiguration

/**
 * @brief
 *   Logging Settings tags
 */
enum
{
    /**
     * Current logging importance, the value is of type ImportanceType.
     */
    kTag_CurrentImportance = 1,
    /**
     * Time, in UTC seconds when the current elevated logging
     * settings revert to the default values.
     */
    kTag_ImportanceExpiration,
    /**
     * Minimal duration, in seconds, between automatically
     * triggered log upload attempts.
     */
    kTag_MinimumLogUploadInterval,
    /**
     * Maximal duration, in seconds, between automatically
     * triggered log upload attempts.
     */
    kTag_MaximumLogUploadInterval,
    /**
     * A URL denoting the destination of the log upload.
     */
    kTag_LoggingDestination,
    /**
     * An optional array selectively mapping chosen profiles onto
     * the higher logging levels. Each element in array takes a
     * form (profile,path,loggingimportance) to selectively
     * elevate the logging from a subset of the system.  The
     * elevated profile logging priority is only of significance
     * when the logging priority exceeds that of the
     * currentImportance and is the subject to the same expiration
     * time as the currentImportance.
     */
    kTag_TraitLoggingImportance
};

// tags for the logging capabilities

/**
 * @brief
 *   Tags for logging capabilities
 */
enum
{
    kTag_SupportedLogTransports     = 1, //< An array of supported log transport mechanisms
    kTag_SupportsStreaming          = 2, //< A boolean indicating whether the device supports streaming logs
    kTag_SupportsNonVolatileStorage = 3, //< A boolean indicating whether the device supports nonvolatile log storage
    kTag_SupportsPerTraitVerbosity  = 4, //< A boolean indicating whether the device supports per-trait verbosity settings
    kTag_LoggingVolume              = 5, //< A 32-bit unsigned integer describing the expected logging volume in kB/day
    kTag_LogBufferingCapacity       = 6  //< A 32-bit unsigned integer describing the log buffering capacity in kB
};

/**
 * @brief
 *   Tags for event metadata.  For complete semantics of the tag values, see the Event Design Specification.
 */
enum
{
    // public tags for event description

    // The next three values form the event key

    kTag_EventSource             = 1, //< NodeID of the device that generated the event.

    kTag_EventImportance         = 2, //< Importance of the event.

    kTag_EventID                 = 3, //< Sequence number of event, expressed as a 64-bit unsigned quantity.  Must be sequential,
                                      //  jumps in the sequence indicate event gaps.

    // The next two values form a key to the related event, our mechanism for event grouping.

    // Leaving space for kTag_RelatedEventSource: we know that it is
    // hard to make this work in the general case, related event IDs
    // can arise from a causal event ordering; the causal event
    // ordering eliminates (by construction) the main challenges posed
    // by events from different sources.  Furthermore, data
    // relationships are easier when the key and the reference are the
    // same.

    kTag_RelatedEventImportance  = 10, //< Optional.  Importance of the related event. If omitted, the value is equal to the value
                                       //  of kTag_EventImportance.

    kTag_RelatedEventID          = 11, //< Optional.  ID of an Event that this event is related to.  If omitted, the value is equal
                                       //  to the value of kTag_EventID.

    kTag_EventUTCTimestamp       = 12, //< Optional. UTC Timestamp of the event in milliseconds.

    kTag_EventSystemTimestamp    = 13, //< Optional. System Timestamp of the event in milliseconds.

    // The next three values are analogous to the values within WDM RootSection.

    kTag_EventResourceID         = 14, //< Optional. The value is the ID of the resource that the event pertains to.  When omitted,
                                       //  the value is the same as the value of the kTag_EventSource

    kTag_EventTraitProfileID     = 15, //< Mandatory. 32-bit unsigned integer that is equal to the ProfileID of the trait.

    kTag_EventTraitInstanceID    = 16, //< Optional, the instance of the trait that generated the event.

    kTag_EventType               = 17, //< Mandatory.  16-bit unsigned integer that is equal to the wdl.event.id for this type of event.

    // Weave Event internal tags. Only relevant to entities that need to
    // parse out the internal Weave event representation
    kTag_EventDeltaUTCTime       = 30, //< WDM internal tag, time difference from the previous event in the encoding

    kTag_EventDeltaSystemTime    = 31, //< WDM internal tag, time difference from the previous event in the encoding

    kTag_EventData               = 50, //< Optional.  Event data itself.  If empty, it defaults to an empty structure.

    kTag_ExternalEventStructure  = 99, //< Internal tag for external events.  Never transmitted across the wire, should never be used outside of Weave library

};

// Tags for the debug trait

/**
 * @brief
 *   Profile definitions for the debug trait
 */
enum
{
    kWeaveProfile_NestDebug = 0x235a0010,
};

/**
 * @brief
 *   Event types for the Nest Debug trait
 */

enum
{
    kNestDebug_StringLogEntryEvent       = 1, //< An event for freeform string debug message.
    kNestDebug_TokenizedLogEntryEvent    = 2, //< An event for tokenized debug message.
    kNestDebug_TokenizedHeaderEntryEvent = 3  //< An event for conveying the tokenized header information.
};

/**
 * @brief
 *  Tags for the kNestDebug_StringLogEntryEvent
 */
enum
{
    kTag_Region  = 1, //< An 32-bit unsigned indicating the log region, i.e. the module to which the log message pertains.
    kTag_Message = 2  //< A string containing the actual debug message.
};

/**
 * @brief
 *  Tags for the kNestDebug_TokenizedLogEntryEvent
 */
enum
{
    kTag_Token = 1, //< A 32-bit unsigned value corresponding to the token.
    kTag_Args  = 2  //< An array of arguments to be sent along with the token message.
};

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif //_WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TAGS_CURRENT_H

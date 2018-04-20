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

#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TYPES_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TYPES_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/EventLoggingTags.h>
#include <Weave/Profiles/data-management/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * @brief
 *   The importance of the log entry.
 *
 * @details
 * Importance is used as a way to filter events before they are
 * actually emitted into the log. After the event is in the log, we
 * make no further provisions to expunge it from the log.
 * The importance level serves to prioritize event storage. If an
 * event of high importance is added to a full buffer, events are
 * dropped in order of importance (and age) to accomodate it. As such,
 * importance levels only have relative value. If a system is
 * using only one importance level, events are dropped only in order
 * of age, like a ring buffer.
 */

typedef enum ImportanceType
{
    kImportanceType_Invalid = 0,
    kImportanceType_First   = 1,
    /**
     * Production Critical importance denotes events whose loss would
     * directly impact customer-facing features. Applications may use
     * loss of Production Critical events to indicate system failure.
     * On constrained devices, entries logged with Production Critical
     * importance must be accounted for in the power and memory budget,
     * as it is expected that they are always logged and offloaded
     * from the device.
     */
    ProductionCritical = 1,
    /**
     * Production importance denotes the log entries that are used in
     * the ongoing monitoring and maintenance of the Nest ecosystem.
     * On constrained devices, entries logged with Production
     * importance must be accounted for in the power and memory
     * budget, as it is expected that they are always logged and
     * offloaded from the device.
     */
    Production,
    /**
     * Info importance denotes log entries that provide extra insight
     * and diagnostics into the running system. Info logging level may
     * be used over an extended period of time in a production system,
     * or may be used as the default log level in a field trial. On
     * the constrained devices, the entries logged with Info level must
     * be accounted for in the bandwidth and memory budget, but not in
     * the power budget.
     */
    Info,
    /**
     *  Debug importance denotes log entries of interest to the
     *  developers of the system and is used primarily in the
     *  development phase. Debug importance logs are
     *  not accounted for in the bandwidth or power budgets of the
     *  constrained devices; as a result, they must be used only over
     *  a limited time span in production systems.
     */
    Debug,

    kImportanceType_Last = Debug,
} ImportanceType;

/**
 * @brief
 *   The structure that defines a schema for event metadata.
 */
struct EventSchema
{
    uint32_t mProfileId;        //!< ID of profile
    uint32_t mStructureType;    //!< Type of structure
    ImportanceType mImportance; //!< Importance
    SchemaVersion mDataSchemaVersion;
    SchemaVersion mMinCompatibleDataSchemaVersion;
};

/**
 * @typedef timestamp_t
 * Type used to describe the timestamp in milliseconds.
 */
typedef uint32_t timestamp_t;

/**
 * @typedef duration_t
 * Type used to describe the duration, in milliseconds.
 */
typedef uint32_t duration_t;

/**
 * @typedef event_id_t
 * The type of event ID.
 */
typedef uint32_t event_id_t;

/**
 * @typedef utc_timestamp_t
 * Type used to describe the UTC timestamp in milliseconds.
 */
typedef uint64_t utc_timestamp_t;

// forward declaration

struct ExternalEvents;

// Structures used to describe in detail additional options for event encoding

/**
 * @brief
 *   The structure that provides a full resolution of the trait instance.
 */
struct DetailedRootSection
{
    /**
     * Default constructor
     */
    DetailedRootSection(void) : ResourceID(ResourceIdentifier::SELF_NODE_ID) { };

    ResourceIdentifier ResourceID;      /**< The ID of the resource that the generated event pertains to.
                                   When the event resource is equal to the event source, set
                                   this value equal to ResourceIdentifier::SELF_NODE_ID */
    uint64_t TraitInstanceID; /**< Trait instance of the subject of this event. */
};

/**
 * @brief
 *   The validity and type of timestamp included in EventOptions.
 */
typedef enum TimestampType
{
    kTimestampType_Invalid = 0,
    kTimestampType_System,
    kTimestampType_UTC
} TimestampType;

/**
 * @brief
 *   The union that provides an application set system or UTC timestamp.
 */
union Timestamp
{
    /**
     * Default constructor.
     */
    Timestamp(void) : systemTimestamp(0) { };

    /**
     * UTC timestamp constructor.
     */
    Timestamp(utc_timestamp_t aUtc) : utcTimestamp(aUtc) { };

    /**
     * System timestamp constructor.
     */
    Timestamp(timestamp_t aSystem) : systemTimestamp(aSystem) { };

    timestamp_t systemTimestamp;  //< System timestamp.
    utc_timestamp_t utcTimestamp; //< UTC timestamp.
};

/**
 *   The structure that provides options for the different event fields.
 */
struct EventOptions
{
    EventOptions(void);
    EventOptions(bool);
    EventOptions(timestamp_t);
    EventOptions(utc_timestamp_t);
    EventOptions(timestamp_t, bool);
    EventOptions(utc_timestamp_t, bool);
    EventOptions(utc_timestamp_t, DetailedRootSection *, event_id_t, ImportanceType, bool);
    EventOptions(timestamp_t, DetailedRootSection *, event_id_t, ImportanceType, bool);

    Timestamp timestamp;               /**< A union holding either system or UTC timestamp. */

    DetailedRootSection * eventSource; /**< A pointer to the detailed resolution of the trait instance.  When NULL, the event source
                                            is assumed to come from the resource equal to the local node ID, and from the default
                                            instance of the trait. */

    // Facilities for event grouping
    event_id_t relatedEventID;         /**< The Event ID from the same Event Source that this event is related to.  When the event is not
                                            related to any other events, Related Event ID is shall be equal to Event ID, and may be omitted.  A
                                            value of 0 implies the absence of any related event. */
    ImportanceType relatedImportance;  /**< EventImportance of the Related Event ID.  When this event and the related event are of the
                                            same importance, the field may be omitted.  A value of kImportanceType_Invalid implies the
                                            absence of any related event. */

    TimestampType timestampType;       /**< An enum indicating if the timestamp is valid and its type. */

    bool urgent;                       /**< A flag denoting that the event is time sensitive.  When set, it causes the event log to be flushed. */
};

/**
 * @brief
 *   Structure for copying event lists on output.
 */

struct EventLoadOutContext
{
    EventLoadOutContext(nl::Weave::TLV::TLVWriter & inWriter, ImportanceType inImportance, uint32_t inStartingEventID, ExternalEvents * ioExternalEvent);

    nl::Weave::TLV::TLVWriter & mWriter;
    ImportanceType mImportance;
    uint32_t mStartingEventID;
    uint32_t mCurrentTime;
    uint32_t mCurrentEventID;
    ExternalEvents *mExternalEvents;
#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    uint64_t mCurrentUTCTime;
    bool mFirstUtc;
#endif
    bool mFirst;
};

/**
 *  @brief
 *    A function that supplies eventData element for the event logging subsystem.
 *
 *  Functions of this type are expected to provide the eventData
 *  element for the event logging subsystem. The functions of this
 *  type are called after the event subsystem has generated all
 *  required event metadata. The function is called with a
 *  nl::Weave::TLV::TLVWriter object into which it will emit a single TLV element
 *  tagged kTag_EventData; the value of that element MUST be a
 *  structure containing the event data. The event data itself must
 *  be structured using context tags.
 *
 *  @sa PlainTextWriter
 *  @sa EventWriterTLVCopy
 *
 *  @param[inout] ioWriter A reference to the nl::Weave::TLV::TLVWriter object to be
 *                         used for event data serialization.
 *
 *  @param[in]    inDataTag A context tag for the TLV we're writing out.
 *
 *  @param[in]    appData  A pointer to an application specific context.
 *
 *  @retval #WEAVE_NO_ERROR  On success.
 *
 *  @retval other           An appropriate error signaling to the
 *                          caller that the serialization of event
 *                          data could not be completed. Errors from
 *                          calls to the ioWriter should be propagated
 *                          without remapping. If the function
 *                          returns any type of error, the event
 *                          generation is aborted, and the event is not
 *                          written to the log.
 *
 */

typedef WEAVE_ERROR (*EventWriterFunct)(nl::Weave::TLV::TLVWriter & ioWriter, uint8_t inDataTag, void * appData);

/**
 *  @brief
 *    A function prototype for platform callbacks fetching event data.
 *
 *  Similar to FetchEventsSince, this fetch function returns all events from
 *  EventLoadOutContext.mStartingEventID through ExternalEvents.mLastEventID.
 *
 *  The context pointer is of type FetchExternalEventsContext. This includes
 *  the EventLoadOutContext, with some helper variables for the format of the TLV.
 *  It also includes a pointer to the ExternalEvents struct created on registration
 *  of the callback. This specifies the event ID range for the callback.
 *
 *  On returning from the function, EventLoadOutContext.mCurrentEventID should
 *  reflect the first event ID that has not been successfully written to the
 *  TLV buffer. The platform must write the events header and data to the TLV
 *  writer in the correct format, specified by the EventLogging protocol. The
 *  platform must also maintain uniqueness of events and timestamps.
 *
 *  All TLV errors should be propagated to higher levels. For instance, running
 *  out of space in the buffer will trigger a sent message, followed by another
 *  call to the callback with whichever event ID remains.
 *
 *  @retval WEAVE_ERROR_NO_MEMORY           If no space to write events.
 *  @retval WEAVE_ERROR_BUFFER_TOO_SMALL    If no space to write events.
 *  @retval WEAVE_NO_ERROR                  On success.
 *  @retval WEAVE_END_OF_TLV                On success.
 */
typedef WEAVE_ERROR (*FetchExternalEventsFunct)(EventLoadOutContext * aContext);

/**
 * @brief
 *
 *   A function prototype for a callback invoked when external events
 *   are delivered to the remote subscriber.
 *
 * When the external events are delivered to a remote subscriber, the
 * engine will provide a notification to the external event provider.
 * The callback contains the event of the last ID that was delivered,
 * and the ID of the subscriber that received the event.
 *
 * @param[in] inLastDeliveredEventID     ID of the last event delivered to
 *                                       the subscriber.
 * @param[in] inRecipientNodeID          Weave node ID of the recipient
 *
 */
typedef void (*NotifyExternalEventsDeliveredFunct)(ExternalEvents * inEv, event_id_t inLastDeliveredEventID,
                                                   uint64_t inRecipientNodeID);

/**
 *  @brief
 *    Structure for tracking platform-stored events.
 */
struct ExternalEvents
{
    ExternalEvents(void) : mFirstEventID(1), mLastEventID(0), mFetchEventsFunct(NULL), mNotifyEventsDeliveredFunct(NULL) { };

    event_id_t mFirstEventID; /**< The first event ID stored externally. */
    event_id_t mLastEventID;  /**< The last event ID stored externally. */

    FetchExternalEventsFunct mFetchEventsFunct; /**< The callback to use to fetch the above IDs. */
    NotifyExternalEventsDeliveredFunct mNotifyEventsDeliveredFunct;
    bool IsValid(void) { return mFirstEventID <= mLastEventID; };
    void Invalidate(void) { mFirstEventID = 1; mLastEventID = 0; };
};

// internal API
typedef WEAVE_ERROR (*LoggingBufferHandler)(void * inAppState, PacketBuffer * inBuffer);

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif //_WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_TYPES_CURRENT_H

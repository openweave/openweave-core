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
 * @file
 *
 * @brief
 *   Implementations of enums, types, and tags used in Weave Event Logging.
 *
 */

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

EventOptions::EventOptions(void) :
    timestamp(), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_Invalid), urgent(false)
{ }

EventOptions::EventOptions(bool aUrgent) :
    timestamp(), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_Invalid), urgent(aUrgent)
{ }

EventOptions::EventOptions(timestamp_t aSystemTimestamp) :
    timestamp(aSystemTimestamp), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_System), urgent(false)
{ }

EventOptions::EventOptions(utc_timestamp_t aUtcTimestamp) :
    timestamp(aUtcTimestamp), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_UTC), urgent(false)
{ }

EventOptions::EventOptions(timestamp_t aSystemTimestamp, bool aUrgent) :
    timestamp(aSystemTimestamp), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_System), urgent(aUrgent)
{ }

EventOptions::EventOptions(utc_timestamp_t aUtcTimestamp, bool aUrgent) :
    timestamp(aUtcTimestamp), eventSource(NULL), relatedEventID(0), relatedImportance(kImportanceType_Invalid),
    timestampType(kTimestampType_UTC), urgent(aUrgent)
{ }

EventOptions::EventOptions(timestamp_t aSystemTimestamp, DetailedRootSection * aEventSource, event_id_t aRelatedEventID,
                           ImportanceType aRelatedImportance, bool aUrgent) :
    timestamp(aSystemTimestamp),
    eventSource(aEventSource), relatedEventID(aRelatedEventID), relatedImportance(aRelatedImportance),
    timestampType(kTimestampType_System), urgent(aUrgent)
{ }

EventOptions::EventOptions(utc_timestamp_t aUtcTimestamp, DetailedRootSection * aEventSource, event_id_t aRelatedEventID,
                           ImportanceType aRelatedImportance, bool aUrgent) :
    timestamp(aUtcTimestamp),
    eventSource(aEventSource), relatedEventID(aRelatedEventID), relatedImportance(aRelatedImportance),
    timestampType(kTimestampType_UTC), urgent(aUrgent)
{ }

EventLoadOutContext::EventLoadOutContext(nl::Weave::TLV::TLVWriter & inWriter, ImportanceType inImportance,
                                         uint32_t inStartingEventID, ExternalEvents * ioExternalEvents) :
    mWriter(inWriter),
    mImportance(inImportance), mStartingEventID(inStartingEventID), mCurrentTime(0), mCurrentEventID(0),
    mExternalEvents(ioExternalEvents),
#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    mCurrentUTCTime(0), mFirstUtc(true),
#endif // WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    mFirst(true)
{ }

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl

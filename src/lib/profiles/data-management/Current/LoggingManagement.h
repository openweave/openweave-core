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
 *   Management of the Weave Event Logging.
 *
 */
#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_MANAGEMENT_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_MANAGEMENT_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCircularTLVBuffer.h>

#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
#include <Weave/Profiles/time/WeaveTime.h>
#endif

#include <Weave/Support/PersistedCounter.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * @brief
 *   Internal event buffer, built around the nl::Weave::TLV::WeaveCircularTLVBuffer
 */
struct CircularEventBuffer
{
    // for doxygen, see the CPP file
    CircularEventBuffer(uint8_t * inBuffer, size_t inBufferLength, CircularEventBuffer * inPrev, CircularEventBuffer * inNext);

    // for doxygen, see the CPP file
    bool IsFinalDestinationForImportance(ImportanceType inImportance) const;

    event_id_t VendEventID(void);
    void RemoveEvent(size_t aNumEvents);

    // for doxygen, see the CPP file
    void AddEvent(timestamp_t inEventTimestamp);

#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    // for doxygen, see the CPP file
    void AddEventUTC(utc_timestamp_t inEventTimestamp);
#endif

    nl::Weave::TLV::WeaveCircularTLVBuffer mBuffer; //< The underlying TLV buffer storing the events in a TLV representation

    CircularEventBuffer * mPrev; //< A pointer #CircularEventBuffer storing events less important events
    CircularEventBuffer * mNext; //< A pointer #CircularEventBuffer storing events more important events

    ImportanceType mImportance; //< The buffer is the final bucket for events of this importance.  Events of lesser importance are
                                //dropped when they get bumped out of this buffer

    event_id_t mFirstEventID; //< First event ID stored in the logging subsystem for this importance
    event_id_t mLastEventID;  //< Last event ID vended for this importance

    timestamp_t mFirstEventTimestamp; //< The timestamp of the first event in this buffer
    timestamp_t mLastEventTimestamp;  //< The timestamp of the last event in this buffer

#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    utc_timestamp_t mFirstEventUTCTimestamp; //< The UTC timestamp of the first event in this buffer
    utc_timestamp_t mLastEventUTCTimestamp;  //< The UTC timestamp of the last event in this buffer

    bool mUTCInitialized; //< Indicates whether UTC timestamps are initialized in this buffer
#endif

    // The counter we're going to actually use.
    nl::Weave::MonotonicallyIncreasingCounter * mEventIdCounter;

    // The backup counter to use if no counter is provided for us.
    nl::Weave::MonotonicallyIncreasingCounter mNonPersistedCounter;

    static WEAVE_ERROR GetNextBufferFunct(nl::Weave::TLV::TLVReader & ioReader, uintptr_t & inBufHandle,
                                          const uint8_t *& outBufStart, uint32_t & outBufLen);
};

/**
 * @brief
 *   A TLVReader backed by CircularEventBuffer
 */
class CircularEventReader : public nl::Weave::TLV::TLVReader
{
    friend struct CircularEventBuffer;

public:
    void Init(CircularEventBuffer * inBuf);
};

/**
 * @brief
 *  Internal structure for traversing event list.
 */
struct CopyAndAdjustDeltaTimeContext
{
    CopyAndAdjustDeltaTimeContext(nl::Weave::TLV::TLVWriter * inWriter, EventLoadOutContext * inContext);

    nl::Weave::TLV::TLVWriter * mWriter;
    EventLoadOutContext * mContext;
};

/**
 * @brief
 *  Internal structure for traversing events.
 */
struct EventEnvelopeContext
{
    EventEnvelopeContext(void);

    size_t mNumFieldsToRead;
    int32_t mDeltaTime;
#if WEAVE_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
    int64_t mDeltaUtc;
#endif
    ImportanceType mImportance;
    ExternalEvents *mExternalEvents;
};

enum LoggingManagementStates
{
    kLoggingManagementState_Idle       = 1, //< No log offload in progress, log offload can begin without any constraints
    kLoggingManagementState_InProgress = 2, //< Log offload in progress
    kLoggingManagementState_Holdoff    = 3, //< Log offload has completed; we do not restart the log until the holdoff expires
    kLoggingManagementState_Shutdown   = 4  //< Not capable of performing any logging operation
};

// forward class declaration
class LogBDXUpload;

/**
 * @brief
 *   A class for managing the in memory event logs.
 */

class LoggingManagement
{
    friend class LogBDXUpload;

public:
    LoggingManagement(nl::Weave::WeaveExchangeManager * inMgr, size_t inNumBuffers, size_t * inBufferLengths, void ** inBuffers,
                      nl::Weave::Platform::PersistedStorage::Key * inCounterKeys, const uint32_t * inCounterEpochs,
                      nl::Weave::PersistedCounter ** inCounterStorage);
    LoggingManagement(nl::Weave::WeaveExchangeManager * inMgr, size_t inNumBuffers, size_t * inBufferLengths, void ** inBuffers,
                      nl::Weave::MonotonicallyIncreasingCounter ** nWeaveCounter);
    LoggingManagement(void);

    static LoggingManagement & GetInstance(void);

    static void CreateLoggingManagement(nl::Weave::WeaveExchangeManager * inMgr, size_t inNumBuffers, size_t * inBufferLengths,
                                        void ** inBuffers, nl::Weave::Platform::PersistedStorage::Key * inCounterKeys,
                                        const uint32_t * inCounterEpochs, nl::Weave::PersistedCounter ** inCounterStorage);
    static void CreateLoggingManagement(nl::Weave::WeaveExchangeManager * inMgr, size_t inNumBuffers, size_t * inBufferLengths,
                                        void ** inBuffers, nl::Weave::MonotonicallyIncreasingCounter ** nWeaveCounter);
    static void DestroyLoggingManagement(void);

    WEAVE_ERROR SetExchangeManager(nl::Weave::WeaveExchangeManager * inMgr);

    event_id_t LogEvent(const EventSchema & inSchema, EventWriterFunct inEventWriter, void * inAppData,
                        const EventOptions * inOptions);

    WEAVE_ERROR GetEventReader(nl::Weave::TLV::TLVReader & ioReader, ImportanceType inImportance);

    WEAVE_ERROR FetchEventsSince(nl::Weave::TLV::TLVWriter & ioWriter, ImportanceType inImportance, event_id_t & ioEventID);

    WEAVE_ERROR ScheduleFlushIfNeeded(bool inFlushRequested);

    WEAVE_ERROR SetLoggingEndpoint(event_id_t * inEventEndpoints, size_t inNumImportanceLevels, size_t & outLoggingPosition);

    uint32_t GetBytesWritten(void) const;

    void NotifyEventsDelivered(ImportanceType inImportance, event_id_t inLastDeliveredEventID, uint64_t inRecipientNodeID);

    /**
     * @brief
     *   IsValid returns whether the LoggingManagement instance is valid
     *
     * @retval true  The instance is valid (initialized with the appropriate backing store)
     * @retval false Otherwise
     */
    bool IsValid(void) { return (mEventBuffer != NULL); };

    event_id_t GetLastEventID(ImportanceType inImportance);
    event_id_t GetFirstEventID(ImportanceType inImportance);

    void ThrottleLogger(void);
    void UnthrottleLogger(void);

    void SetBDXUploader(LogBDXUpload * inUploader);

#if WEAVE_CONFIG_EVENT_LOGGING_EXTERNAL_EVENT_SUPPORT
    WEAVE_ERROR RegisterEventCallbackForImportance(ImportanceType inImportance, FetchExternalEventsFunct inFetchCallback,
                                                   NotifyExternalEventsDeliveredFunct inNotifyCallback, size_t inNumEvents,
                                                   event_id_t * outLastEventID);
    WEAVE_ERROR RegisterEventCallbackForImportance(ImportanceType inImportance, FetchExternalEventsFunct inFetchCallback,
                                                   size_t inNumEvents, event_id_t * outLastEventID);
    void UnregisterEventCallbackForImportance(ImportanceType inImportance, event_id_t inEventID);
#endif
    WEAVE_ERROR BlitEvent(EventLoadOutContext * aContext, const EventSchema & inSchema, EventWriterFunct inEventWriter,
                          void * inAppData, const EventOptions * inOptions);

#if WEAVE_CONFIG_EVENT_LOGGING_WDM_OFFLOAD
    bool CheckShouldRunWDM(void);
#endif
private:
    event_id_t LogEventPrivate(const EventSchema & inSchema, EventWriterFunct inEventWriter, void * inAppData,
                               const EventOptions * inOptions);

    void FlushHandler(System::Layer * inSystemLayer, INET_ERROR inErr);
    void SignalUploadDone(void);
    WEAVE_ERROR CopyToNextBuffer(CircularEventBuffer * inEventBuffer);
    WEAVE_ERROR EnsureSpace(size_t inRequiredSpace);

    static WEAVE_ERROR CopyEventsSince(const nl::Weave::TLV::TLVReader & aReader, size_t aDepth, void * aContext);
    static WEAVE_ERROR EventIterator(const nl::Weave::TLV::TLVReader & aReader, size_t aDepth, void * aContext);
    static WEAVE_ERROR FetchEventParameters(const nl::Weave::TLV::TLVReader & aReader, size_t aDepth, void * aContext);
    static WEAVE_ERROR CopyAndAdjustDeltaTime(const nl::Weave::TLV::TLVReader & aReader, size_t aDepth, void * aContext);
    static WEAVE_ERROR EvictEvent(nl::Weave::TLV::WeaveCircularTLVBuffer & inBuffer, void * inAppData,
                                  nl::Weave::TLV::TLVReader & inReader);
    static WEAVE_ERROR AlwaysFail(nl::Weave::TLV::WeaveCircularTLVBuffer & inBuffer, void * inAppData,
                                  nl::Weave::TLV::TLVReader & inReader);
    static WEAVE_ERROR CopyEvent(const nl::Weave::TLV::TLVReader & aReader, nl::Weave::TLV::TLVWriter & aWriter,
                                 EventLoadOutContext * aContext);

    static void LoggingFlushHandler(System::Layer * systemLayer, void * appState, INET_ERROR err);

#if WEAVE_CONFIG_EVENT_LOGGING_BDX_OFFLOAD
    bool CheckShouldRunBDX(void);
#endif

    ImportanceType GetMaxImportance(void);
    ImportanceType GetCurrentImportance(uint32_t profileId);

private:
    CircularEventBuffer * GetImportanceBuffer(ImportanceType inImportance) const;

#if WEAVE_CONFIG_EVENT_LOGGING_EXTERNAL_EVENT_SUPPORT
    static WEAVE_ERROR FindExternalEvents(const nl::Weave::TLV::TLVReader & aReader, size_t aDepth, void * aContext);
    WEAVE_ERROR GetExternalEventsFromEventId(ImportanceType inImportance, event_id_t inEventId, ExternalEvents * outExternalEvents, nl::Weave::TLV::TLVReader & inReader);
    static WEAVE_ERROR BlitExternalEvent(nl::Weave::TLV::TLVWriter &inWriter, ImportanceType inImportance, ExternalEvents &inEvents);
#endif
    CircularEventBuffer * mEventBuffer;
    WeaveExchangeManager * mExchangeMgr;
    LoggingManagementStates mState;
    LogBDXUpload * mBDXUploader;
    uint32_t mBytesWritten;
    uint32_t mThrottled;
    ImportanceType mMaxImportanceBuffer;
    bool mUploadRequested;
};

namespace Platform {
extern void CriticalSectionEnter(void);
extern void CriticalSectionExit(void);
} // namespace Platform

} // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif //_WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_MANAGEMENT_CURRENT_H

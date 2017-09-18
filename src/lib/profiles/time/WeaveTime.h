/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file contains header for the Time Services feature set, which includes
 *      both time sync and time zone.
 *      WEAVE_CONFIG_TIME must be defined if Time Services are needed
 *
 *      TimeZoneUtcOffset: coding and decoding of the UTC offset packed binary format
 *      TimeChangeNotification: coding and decoding of the Time Change Notification message
 *      TimeSyncRequest: coding and decoding of the Time Sync Request message
 *      TimeSyncResponse: coding and decoding of the Time Sync Response message
 *      TimeSyncServer: protocol engine for Time Sync Server
 *      TimeSyncClient: protocol engine for Time Sync Client
 *
 */

#ifndef WEAVE_TIME_H_
#define WEAVE_TIME_H_

// __STDC_CONSTANT_MACROS must be defined for UINT64_C and INT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/ProfileCommon.h>

namespace nl {

namespace Weave {

namespace Profiles {

namespace Time {

/// type used to store and handle number of microseconds from different epoch
/// if used to express system time, the epoch is 1970/1/1 0:00:00
typedef int64_t timesync_t;

/// used as a bit mask to be applied to timesync_t
/// the highest 6 bits, including sign bit, must be zero, for valid system time
#define MASK_INVALID_TIMESYNC UINT64_C(0xFC00000000000000)

/// used to initialize timestamp (system time) to some invalid value
#define TIMESYNC_INVALID timesync_t(MASK_INVALID_TIMESYNC)

/// Maximum value can be expressed if used as system time (microsecond).
/// This is the largest value that could pass the masking of #MASK_INVALID_TIMESYNC.
#define TIMESYNC_MAX timesync_t(UINT64_MAX & ~(MASK_INVALID_TIMESYNC))

/// maximum value can be expressed if used as system time (second)
#define MAX_TIMESYNC_SEC int64_t(TIMESYNC_MAX / UINT64_C(1000000))

#if WEAVE_CONFIG_TIME_PROGRESS_LOGGING
#define WEAVE_TIME_PROGRESS_LOG WeaveLogProgress
#else // WEAVE_CONFIG_TIME_PROGRESS_LOGGING
#define WEAVE_TIME_PROGRESS_LOG(...)
#endif // WEAVE_CONFIG_TIME_PROGRESS_LOGGING

/// type of a message, used with Weave Exchange
enum
{
    kTimeMessageType_TimeSyncTimeChangeNotification = 0,
    kTimeMessageType_TimeSyncRequest = 1,
    kTimeMessageType_TimeSyncResponse = 2,
};

/// Profile-specific tags used in WDM queries for timezone information
enum
{
    kWdmTagTime_Zone_Name = 0x00,      ///< The IANA Timezone name in UTF8-String format
    kWdmTagTime_Zone_POSIX_TZ = 0x01,  ///< The POSIX TZ environment variable in UTF8-String format
    kWdmTagTime_Zone_UTC_Offset = 0x02 ///< The UTC offsets for this timezone, in packed binary format
};

/// Roles a protocol engine can play.
/// for example, a TimeSyncServer could be playing a Server or part of a Coordinator.
/// likewise, a TimeSyncClient could be playing a Client or just part of a Coordinator.
enum TimeSyncRole
{
    kTimeSyncRole_Unknown = 0,
    kTimeSyncRole_Server = 1,
    kTimeSyncRole_Coordinator = 2,
    kTimeSyncRole_Client = 3,
};

/// Codec for UTC offset of a timezone
class NL_DLL_EXPORT TimeZoneUtcOffset
{
public:

    /// conversion information
    struct UtcOffsetRecord
    {
        timesync_t mBeginAt_usec;    ///< UTC time, in usec since standard epoch,
                                     ///< of the beginning of this conversion period
        int32_t mUtcOffset_sec;      ///< Offset, in seconds, from UTC to local time
    };

    /// number of valid entries in mUtcOffsetRecord
    uint8_t mSize;

    /// entries of UTC offsets
    UtcOffsetRecord mUtcOffsetRecord[WEAVE_CONFIG_TIME_NUM_UTC_OFFSET_RECORD];

    TimeZoneUtcOffset() :
        mSize(0)
    {
    }

    /**
     * convert UTC time to local time, using the UTC offsets stored.
     *
     * @param[out] aLocalTime A pointer to the resulting local time
     *
     * @param[in]  aUtcTime   UTC time
     *
     * @return WEAVE_NO_ERROR On success. WEAVE_ERROR_KEY_NOT_FOUND if it couldn't find reasonable results
     */
    WEAVE_ERROR GetCurrentLocalTime(timesync_t * const aLocalTime, const timesync_t aUtcTime) const;

    /**
     * decode UTC offsets from a byte string, extracted from Weave TLV.
     * data type for size is the same as WeaveTLV.h
     *
     * @param[in]  aInputBuf A pointer to the input data buffer
     *
     * @param[in]  aDataSize number of bytes available
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Decode(const uint8_t * const aInputBuf, const uint32_t aDataSize);

    /// TimeZoneUtcOffset::BufferSizeForEncoding is a compile time constant, which can be used to declare byte arrays.
    /// Callers shall prepare enough buffer size for encoding to complete successfully, and BufferSizeForEncoding is
    /// the longest buffer that could be needed.
    static const uint32_t BufferSizeForEncoding = 2 + 8 + 4 + (WEAVE_CONFIG_TIME_NUM_UTC_OFFSET_RECORD - 1) * 8;

    /**
     * encode UTC offsets into a buffer.
     * data type for size is the same as WeaveTLV.h
     *
     * @param[out]    aOutputBuf A pointer to the output data buffer
     *
     * @param[in/out] aDataSize  A pointer to number of bytes available in aOutputBuf at calling and will be changed
     *                           to indicate number of bytes used after the function returns.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Encode(uint8_t * const aOutputBuf, uint32_t * const aDataSize);
};

/// codec for Time Change Notification message
class NL_DLL_EXPORT TimeChangeNotification
{
public:
    /// default constructor shall be used with Decode, as all members will be initialized through decoding
    TimeChangeNotification(void);

    /**
     * encode time change notification into an PacketBuffer.
     *
     * @param[out] aMsg A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Encode(PacketBuffer* const aMsg);

    /**
     * decode time change notification from an PacketBuffer.
     *
     * @param[out] aObject A pointer to the decoded object
     *
     * @param[in]  aMsg    A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    static WEAVE_ERROR Decode(TimeChangeNotification * const aObject, PacketBuffer* const aMsg);
};

// codec for Time Sync Request message
class NL_DLL_EXPORT TimeSyncRequest
{
public:

    /// default constructor shall be used with Decode, as all members will be initialized through decoding
    TimeSyncRequest(void);

    /**
     * initialize this object for encoding.
     *
     * @param[in] aLikelihood        intended likelihood of response for this time sync request
     *
     * @param[in] aIsTimeCoordinator true if the originator of this request is a Time Sync Coordinator
     *
     * @return WEAVE_NO_ERROR on success
     */
    void Init(const uint8_t aLikelihood, const bool aIsTimeCoordinator);

    /**
     * encode time sync request into an PacketBuffer.
     *
     * @param[out] aMsg A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Encode(PacketBuffer* const aMsg);

    /**
     * decode time sync request from an PacketBuffer.
     *
     * @param[out] aObject A pointer to the decoded object
     *
     * @param[in]  aMsg    A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    static WEAVE_ERROR Decode(TimeSyncRequest * const aObject, PacketBuffer* const aMsg);

    /// minimum and maxiumum settings for the intended likelihood of response
    /// for this time sync request.
    /// Note that we cannot put check on kLikelihoodForResponse_Min in the Encode and Decode
    /// routines because it's 0, so it's not safe to adjust it just at here
    enum
    {
        kLikelihoodForResponse_Min = 0,
        kLikelihoodForResponse_Max = 31,
    };

    // Time Sync Request Payload length
    enum
    {
        kPayloadLen = 2,
    };

    /// intended likelihood of response for this time sync request.
    uint8_t mLikelihoodForResponse;

    /// true if the originator of this request is a Time Sync Coordinator
    bool mIsTimeCoordinator;
};

// codec for Time Sync Response message
class NL_DLL_EXPORT TimeSyncResponse
{
public:
    /// default constructor shall be used with Decode, as all members will be initialized through decoding
    TimeSyncResponse(void);

    /**
     * initialize this object for encoding.
     *
     * @param[in] aRole           the role this responder is playing.
     *                            can be either kTimeSyncRole_Server or kTimeSyncRole_Coordinator
     *
     * @param[in] aTimeOfRequest  the system time when the original request was received
     *
     * @param[in] aTimeOfResponse the system time when this response is being sent out
     *
     * @param[in] aNumContributorInLastLocalSync    number of nodes contributed in the last local time sync
     *
     * @param[in] aTimeSinceLastSyncWithServer_min  number of minutes passed since last sync with a Server
     *
     */
    void Init(const TimeSyncRole aRole, const timesync_t aTimeOfRequest, const timesync_t aTimeOfResponse,
        const uint8_t aNumContributorInLastLocalSync, const uint16_t aTimeSinceLastSyncWithServer_min);

    /// maximum number of contributors in the last successful time sync operation on local fabric
    enum
    {
        kNumberOfContributor_Max = 31,
    };

    /// time, in number of minutes, since last successful time sync with some proxy of atomic time.
    /// kTimeSinceLastSyncWithServer_Invalid means this happened too long ago to be relevant, if ever
    enum
    {
        kTimeSinceLastSyncWithServer_Max = 4094,
        kTimeSinceLastSyncWithServer_Invalid = 4095,
    };

    /**
     * encode time sync response into an PacketBuffer.
     *
     * @param[out] aMsg A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Encode(PacketBuffer* const aMsg);

    /**
     * decode time sync response from an PacketBuffer.
     *
     * @param[out] aObject A pointer to the decoded object
     *
     * @param[in]  aMsg    A pointer to the PacketBuffer
     *
     * @return WEAVE_NO_ERROR on success
     */
    static WEAVE_ERROR Decode(TimeSyncResponse * const aObject, PacketBuffer* const aMsg);

    /// true if this response is constructed by a coordinator;
    /// false implies this response is constructed by a server.
    bool mIsTimeCoordinator;

    /// number of local contributors (coordinators or servers) used in last successful time sync
    uint8_t mNumContributorInLastLocalSync;

    /// time, in number of minutes, since last successful time sync with some proxy of atomic time
    uint16_t mTimeSinceLastSyncWithServer_min;

    /// system time (number of microseconds since 1970/1/1 0:00:00) when the request arrived
    timesync_t mTimeOfRequest;

    /// system time (number of microseconds since 1970/1/1 0:00:00) when the response was prepared
    timesync_t mTimeOfResponse;
};

class _TimeSyncNodeBase
{
protected:
    _TimeSyncNodeBase(void);

    void Init(WeaveFabricState * const aFabricState,
        WeaveExchangeManager * const aExchangeMgr);

public:
    WeaveFabricState * GetFabricState(void) const
    {
        return FabricState;
    }

    WeaveExchangeManager * GetExchangeMgr(void) const
    {
        return ExchangeMgr;
    }

private:
    WeaveFabricState * FabricState;
    WeaveExchangeManager *ExchangeMgr;
};

/// This is in the public because the TimeSyncNode::FilterTimeCorrectionContributor callback gives
/// a global view to higher layer.
/// It's put in the open instead of being a nested class to make
/// class declaration of TimeSyncNode shorter, and also the export declaration more explicit.
struct NL_DLL_EXPORT Contact
{
    /// contains CommState. casted to uint8_t to save space.
    /// always valid
    uint8_t mCommState;

    /// count the number of communication errors have happened for this contact.
    /// only valid when mCommState is not kCommState_Invalid
    uint8_t mCountCommError;

    /// contains ResponseStatus. casted to uint8_t to save space.
    /// only valid when mCommState is not kCommState_Invalid
    uint8_t mResponseStatus;

    /// contains TimeSyncRole. casted to uint8_t to save space
    /// only valid if response is not kResponseStatus_Invalid
    uint8_t mRole;

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    /// true if this contact is learned from time change notification
    /// only valid when mCommState is not kCommState_Invalid
    bool mIsTimeChangeNotification;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    /// only valid if response is not kResponseStatus_Invalid
    uint8_t mNumberOfContactUsedInLastLocalSync;

    /// only valid if response is not kResponseStatus_Invalid
    uint16_t mTimeSinceLastSuccessfulSync_min;

    /// node ID of this contact
    /// only valid when mCommState is not kCommState_Invalid
    uint64_t mNodeId;

    /// node address of this contact
    /// only valid when mCommState is not kCommState_Invalid
    IPAddress mNodeAddr;

    /// used to store the system time of remote node, when the
    /// response message was prepared for transmission.
    /// only valid if response is not kResponseStatus_Invalid
    timesync_t mRemoteTimestamp_usec;

    /// used to store one way flight time.
    /// only valid if response is not kResponseStatus_Invalid
    int32_t mFlightTime_usec;

    /// this is the timestamp when the response was received.
    /// only valid if response is not kResponseStatus_Invalid
    timesync_t mUnadjTimestampLastContact_usec;
};

/// used to specify contacts for calling SyncWithNodes
/// It's put in the open instead of being a nested class to make
/// class declaration of TimeSyncNode shorter, and also the export declaration more explicit.
struct NL_DLL_EXPORT ServingNode
{
    uint64_t mNodeId;
    IPAddress mNodeAddr;
};

class NL_DLL_EXPORT TimeSyncNode:
    public _TimeSyncNodeBase
{
public:

    /// current state of this Time Sync Server
    enum ServerState
    {
        kServerState_Uninitialized = 0,
        kServerState_ContructionFailed,
        kServerState_Constructed,
        kServerState_InitializationFailed,

        /// time reserved for the server to sync its system time through some other means
        /// only meaningful if aIsAlwaysFresh is true when Init is called
        kServerState_UnreliableAfterBoot,

        /// the server is ready to respond to requests with normal settings
        kServerState_Idle,

        kServerState_ShutdownCompleted,
        kServerState_ShutdownFailed,
    };

    /// current state of this Time Sync Client
    enum ClientState
    {
        kClientState_Uninitialized = 0,
        kClientState_ContructionFailed,
        kClientState_Constructed,
        kClientState_InitializationFailed,

        kClientState_BeginNormal,
        kClientState_Idle,

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        kClientState_Sync_Discovery,
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

        kClientState_Sync_1,
        kClientState_Sync_2,

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
        kClientState_ServiceSync_1,
        kClientState_ServiceSync_2,
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

        kClientState_EndNormal,

        kClientState_ShutdownNeeded,
        kClientState_ShutdownCompleted,
        kClientState_ShutdownFailed,
    };

    /// status of communication to a certain contact.
    /// This is in the public because Contact is in public
    enum CommState
    {
        kCommState_Invalid = 0,
        kCommState_Idle,
        kCommState_Active,
        kCommState_Completed,
    };

    /// status of stored response to a certain contact.
    /// This is in the public because Contact is in public
    enum ResponseStatus
    {
        kResponseStatus_Invalid = 0,
        kResponseStatus_ReliableResponse,
        kResponseStatus_LessReliableResponse,
        kResponseStatus_UnusableResponse,
    };


    TimeSyncNode(void);

    /**
     * stop the service, no matter which role it is playing.
     * This function must be called to properly reclaim resources allocated, before
     * another call to any of the init functions can be made.
     * not available in callbacks.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Shutdown(void);

#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR
    /**
     * initialize this coordinator.
     *
     * @param[in] aExchangeMgr                  A pointer to system wide Weave Exchange Manager object
     *
     * @param[in] aEncryptionType               encryption type to be used for requests and responses
     *
     * @param[in] aKeyId                        key id to be used for requests and responses
     *
     * @param[in] aSyncPeriod_msec              number of msec between syncing
     *
     * @param[in] aNominalDiscoveryPeriod_msec  shortest time between discovery, in msec, if no communication error is observed
     *
     * @param[in] aShortestDiscoveryPeriod_msec smallest number of msec between discovery, if communication error has been observed
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR InitCoordinator(nl::Weave::WeaveExchangeManager *aExchangeMgr, const uint8_t aEncryptionType =
        nl::Weave::kWeaveEncryptionType_None,
        const uint16_t aKeyId = nl::Weave::WeaveKeyId::kNone,
        const int32_t aSyncPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_SYNC_PERIOD_MSEC
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        ,
        const int32_t aNominalDiscoveryPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_NOMINAL_DISCOVERY_PERIOD_MSEC,
        const int32_t aShortestDiscoveryPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_MINIMUM_DISCOVERY_PERIOD_MSEC
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );

#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

#if WEAVE_CONFIG_TIME_ENABLE_SERVER
    /**
     * initialize for the Server role
     * must be called as the first function after object construction
     * if the intention is to be a Time Sync Server.
     * not available in callbacks
     *
     * @param[in] aApp            A pointer to higher layer data, used in callbacks to higher layer.
     *
     * @param[in] aExchangeMgr    A pointer to system wide Weave Exchange Manager object
     *
     * @param[in] aIsAlwaysFresh  could be set to true to indicate the server is always synced
     *                            except for the initial unreliable time.
     *                            shall be set to false for Coordinator.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR InitServer(void * const aApp, WeaveExchangeManager * const aExchangeMgr,
        const bool aIsAlwaysFresh = true);


    /**
     * callback to indicate we just received a time sync request.
     *
     * @param[in] aApp                A pointer to app layer data, set in Init.
     *
     * @param[in] aMsgInfo            A WeaveMessageInfo containing information about the received
     *                                time sync request, including information about the sender.
     *
     * @param[in] aLikelyhood         likelihood of response as requested by the originator
     *
     * @param[in] aIsTimeCoordinator  true if the originating node is a Time Sync Coordinator
     *
     * @return false and the engine shall ignore this request
     */
    typedef bool (*OnSyncRequestReceivedHandler)(void * const aApp, const WeaveMessageInfo *aMsgInfo,
        const uint8_t aLikelyhood,
        const bool aIsTimeCoordinator);

    /// if not set, the default implementation always returns true
    OnSyncRequestReceivedHandler OnSyncRequestReceived;

    /// simple getter for the server state
    ServerState GetServerState(void) const;

    /// Called by higher layer to indicate that we just finished a round of time sync
    /// with either any Server or through some reliable means like NTP.
    /// not available in callbacks.
    void RegisterCorrectionFromServerOrNtp(void);

    /**
     * Called by higher layer to indicate that we just finished a round of time sync with
     * other local Coordinators.
     * not available in callbacks.
     *
     * @param[in] aNumContributor  number of coordinators contributed to this time sync
     *
     */
    void RegisterLocalSyncOperation(const uint8_t aNumContributor);

    /**
     * Called by higher layer to multicast time change notification.
     * not available in callbacks.
     *
     * @param[in] aEncryptionType  type of encryption to be used for this notification
     *
     * @param[in] aKeyId           key id to be used for this notification
     *
     */
    void MulticastTimeChangeNotification(const uint8_t aEncryptionType, const uint16_t aKeyId) const;

#endif // #if WEAVE_CONFIG_TIME_ENABLE_SERVER

#if WEAVE_CONFIG_TIME_ENABLE_CLIENT

    /**
     * initialize this client.
     * not available in callbacks
     *
     * @param[in] aApp                A pointer to higher layer data, used in callbacks to higher layer.
     *
     * @param[in] aExchangeMgr        A pointer to system wide Weave Exchange Manager object
     *
     * @param[in] aRole               can be either kTimeSyncRole_Client or kTimeSyncRole_Coordinator
     *
     * @param[in] aEncryptionType     encryption type to be used for requests and responses
     *
     * @param[in] aKeyId              key id to be used for requests and responses
     *
     * @param[in] aInitialLikelyhood  initial likelihood to be used for discovery stage
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR InitClient(void * const aApp, WeaveExchangeManager *aExchangeMgr,
        const uint8_t aEncryptionType = kWeaveEncryptionType_None,
        const uint16_t aKeyId = WeaveKeyId::kNone
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        ,
        const int8_t aInitialLikelyhood = TimeSyncRequest::kLikelihoodForResponse_Min
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );

    /**
     * callback to indicate we just received a Time Change Notification.
     * if auto sync mode is enabled, a time sync would be scheduled shortly
     * after this callback automatically.
     * otherwise the application layer can choose to call Sync family of functions
     * to directly kick off sync operation not restricted by the normal
     * not-available-in-call-back rule. however, it must be noted that
     * this special callback is still on top of callback stack of weave exchange
     * layer.
     *
     * @param[in] aApp                A pointer to app layer data, set in Init.
     *
     * @param[in] aNodeId             requesting node ID
     *
     * @param[in] aNodeAddr           requesting node address
     *
     */
    typedef void (*TimeChangeNotificationHandler)(void * const aApp, const uint64_t aNodeId,
        const IPAddress & aNodeAddr);
    TimeChangeNotificationHandler OnTimeChangeNotificationReceived;

    /**
     * callback happens right before we calculate the time correction from responses.
     * application layer could overwrite aContact[i].mResponseStatus to kResponseStatus_Invalid
     * so that response would be ignored in the calculation
     *
     * @param[in] aApp                A pointer to app layer data, set in Init.
     *
     * @param[in] aContact            array of contacts and response status
     *
     * @param[in] aSize               number of records in the aContact array
     *
     */
    typedef void (*ContributorFilter)(void * const aApp, Contact aContact[], const int aSize);
    ContributorFilter FilterTimeCorrectionContributor;

    /**
     * callback happens after sync is considered successful, including auto sync,
     * but before the result is applied. Note that successful doesn't mean we have
     * applicable results. In case no response was received, aNumContributor would
     * be set to 0.
     * application layer could overwrite aContact[i].mResponseStatus to kResponseStatus_Invalid
     * so that response would be ignored in the calculation
     *
     * @param[in] aApp                A pointer to app layer data, set in Init.
     *
     * @param[in] aOffsetUsec         amount of correction in usec
     *
     * @param[in] aIsReliable         is the correction considered reliable by the built-in logic
     *
     * @param[in] aIsServer           does the correction come from Server(s)
     *
     * @param[in] aNumContributor     number of nodes which contributed to this correction.
     *                                0 means there is no results from sync operation.
     *
     * @return                        true if this offset shall be used to adjust system time.
     *                                in case aNumContributor is 0, the return value would be
     *                                ignored.
     *
     */
    typedef bool (*SyncSucceededHandler)(void * const aApp, const timesync_t aOffsetUsec, const bool aIsReliable,
        const bool aIsServer, const uint8_t aNumContributor);

    /// if not set, the default behavior is taking all results, except for very small server corrections
    SyncSucceededHandler OnSyncSucceeded;

    /**
     * callback happens when sync is considered failed, including auto sync.
     * note that callback doesn't happen if Abort is called to stop syncing
     *
     * @param[in] aApp                A pointer to app layer data, set in Init.
     *
     * @param[in] aErrorCode          reason for the failure
     *
     */
    typedef void (*SyncFailedHandler)(void * const aApp, const WEAVE_ERROR aErrorCode);
    SyncFailedHandler OnSyncFailed;

    /// simple getter for client state
    ClientState GetClientState(void) const;

    /// simple getter for the maximum number of contacts this engine is configured to store
    int GetCapacityOfContactList(void) const;

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    /**
     * extract the likelihood for persistent.
     * the result would only be valid after sync operation is completed, within callbacks of OnSyncSucceeded and OnSyncFailed.
     * otherwise it's transient and might be the current Likelihood rather than the next one to be used.
     *
     * @return                        likelihood for response to be used in the next request
     *
     */
    int8_t GetNextLikelihood(void) const;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    /**
     * enable auto sync.
     * only available in idle state.
     * discovery happens right away.
     * not available in callbacks.
     *
     * @param[in] aSyncPeriod_msec              number of msec between syncing
     *
     * @param[in] aNominalDiscoveryPeriod_msec  number of msec between discovery, if no communication error is observed
     *
     * @param[in] aShortestDiscoveryPeriod_msec shortest time between discovery, in msec, if communication error has been observed
     *
     * @return                                  WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR EnableAutoSync(
        const int32_t aSyncPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_SYNC_PERIOD_MSEC
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        ,
        const int32_t aNominalDiscoveryPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_NOMINAL_DISCOVERY_PERIOD_MSEC,
        const int32_t aShortestDiscoveryPeriod_msec = WEAVE_CONFIG_TIME_CLIENT_MINIMUM_DISCOVERY_PERIOD_MSEC
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );

    /// disable auto sync.
    /// only available in idle state.
    /// not available in callbacks.
    void DisableAutoSync(void);

    /**
     * sync using existing contacts.
     * sync operation could fail if there is no valid contacts available.
     * set aForceDiscoverAgain to true to force discovery immediately.
     * only available in idle state.
     * not available in callbacks.
     *
     * @param[in] aForceDiscoverAgain           true if all existing contacts shall be flushed and
     *                                          discovery operation performed
     *
     * @return                                  WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR Sync(
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        const bool aForceDiscoverAgain = false
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    /**
     * sync using the given TCP connection and associated encryption and key id.
     * caller must take ownership of the TCP connection after sync finishes.
     * no callback would be overwritten for the TCP connection, as a new Weave Exchange
     * would be created and callbacks set on top of that context
     * only available in idle state.
     * not available in callbacks.
     *
     * @param[in] aConnection                   A pointer to Weave connection
     *
     * @return                                  WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR SyncWithService(WeaveConnection * const aConnection);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    /**
     * sync using the given list of contacts.
     * existing contact list would be flushed.
     * only available in idle state.
     * not available in callbacks.
     *
     * @param[in] aNumNode                      number of contact in array aNodes
     *
     * @param[in] aNodes                        array of contact records
     *
     * @return                                  WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR SyncWithNodes(const int16_t aNumNode, const ServingNode aNodes[]);

    /**
     * force the engine to go back to idle state, aborting anything it is doing.
     * note no sync success or failure would be called.
     * all Weave Exchanges would be closed.
     * TCP connections would not be touched further.
     * no operation if we're already in idle state.
     * not available in callbacks.
     *
     * @return                                  WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR Abort(void);

    /// encryption method for local communication
    uint8_t mEncryptionType;

    /// key id used for local communication
    uint16_t mKeyId;
#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

protected:

    /// pointer to higher layer data
    void * mApp;

    /// Actual role of this node
    TimeSyncRole mRole;

    /// true if we're in a callback to higher layer
    bool mIsInCallback;


    WEAVE_ERROR InitState(const TimeSyncRole aRole,
        void * const aApp, WeaveExchangeManager * const aExchangeMgr);

    void ClearState(void);

#if WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

    /**
     * stop the coordinator
     * not available in callbacks.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR _ShutdownCoordinator(void);

    static bool _OnSyncSucceeded(void * const aApp, const nl::Weave::Profiles::Time::timesync_t aOffsetUsec,
        const bool aIsReliable,
        const bool aIsServer, const uint8_t aNumContributor);
#endif // WEAVE_CONFIG_TIME_ENABLE_COORDINATOR

#if WEAVE_CONFIG_TIME_ENABLE_SERVER

    ServerState mServerState;
    bool mIsAlwaysFresh;
    uint8_t mNumContributorInLastLocalSync;

    /// note it has to be boot time as we need compensation for sleep time
    timesync_t mTimestampLastCorrectionFromServerOrNtp_usec;

    /// note it has to be boot time as we need compensation for sleep time
    timesync_t mTimestampLastLocalSync_usec;

    /**
     * initialize for the Server role.
     * Intended to be used internally by Init family of public functions.
     * Must set mClientState before return.
     * not available in callbacks
     *
     * @param[in] aIsAlwaysFresh  could be set to true to indicate the server is always synced
     *                            except for the initial unreliable time.
     *                            shall be set to false for Coordinator.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR _InitServer(const bool aIsAlwaysFresh);


    /**
     * stop the server
     * not available in callbacks.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR _ShutdownServer(void);

    /// callback from Weave Exchange when a time sync request arrives
    static void HandleSyncRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    /// callback from Weave Timer when we passed the unreliable after boot barrier
    static void HandleUnreliableAfterBootTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

#endif // WEAVE_CONFIG_TIME_ENABLE_SERVER


#if WEAVE_CONFIG_TIME_ENABLE_CLIENT

    ClientState mClientState;

    //@{
    /// states used for auto sync feature.
    bool mIsAutoSyncEnabled;
    int32_t mSyncPeriod_msec;
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    bool mIsUrgentDiscoveryPending;
    int32_t mNominalDiscoveryPeriod_msec;
    int32_t mShortestDiscoveryPeriod_msec;
    timesync_t mBootTimeForNextAutoDiscovery_usec;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    //@}

    /// Contact information learned throughout discovery
    Contact mContacts[WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_CONTACTS];

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    // contact information for talking to the service. this is independent from mContacts, so talking to the service
    // doesn't wipe out results learned from discovery
    Contact mServiceContact;

    /// TCP connection used to talk to the service
    WeaveConnection * mConnectionToService;
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    //@{
    /// communication context.
    //CommToken mCommToken[WEAVE_CONFIG_TIME_CLIENT_MAX_NUM_EXCHANGE_CONTEXTS];
    Contact * mActiveContact;
    ExchangeContext * mExchageContext;
    timesync_t mUnadjTimestampLastSent_usec;
    //@}

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    int8_t mLastLikelihoodSent;
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    /**
     * initialize for the Client role.
     * Intended to be used internally by Init family of public functions.
     * Must set mClientState before return.
     * not available in callbacks
     *
     * @param[in] aEncryptionType     encryption type to be used for requests and responses
     *
     * @param[in] aKeyId              key id to be used for requests and responses
     *
     * @param[in] aInitialLikelyhood  initial likelihood to be used for discovery stage
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR _InitClient(
        const uint8_t aEncryptionType,
        const uint16_t aKeyId
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        ,
        const int8_t aInitialLikelyhood
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
        );


    /**
     * stop the client
     * not available in callbacks.
     *
     * @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR _ShutdownClient(void);

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    /// invalidate contact to the service
    void InvalidateServiceContact(void);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    /// invalidate all local contacts
    void InvalidateAllContacts(void);

    /// set all valid local contacts to idle state and clear the response.
    /// this is called before we start contacting them one by one
    int16_t SetAllValidContactsToIdleAndInvalidateResponse(void);

    /// reset all completed contacts to idle state again, but don't touch the response.
    /// this is called between two rounds of communication to the same node
    int16_t SetAllCompletedContactsToIdle(void);

    /// get the number of contacts that are valid, but we haven't talk to them yet.
    int16_t GetNumNotYetCompletedContacts(void);

    /// get the number of 'reliable' responses collected so far.
    /// called to determine if we have collected enough number of responses
    int16_t GetNumReliableResponses(void);

    /// get the next valid and idle contact to talk to
    Contact * GetNextIdleContact(void);

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    /// return a slot to store contact information
    Contact * FindReplaceableContact(const uint64_t aNodeId, const IPAddress & aNodeAddr,
        bool aIsTimeChangeNotification = false);

    /// process a response coming back from a multicast request
    void UpdateMulticastSyncResponse(const uint64_t aNodeId,
        const IPAddress & aNodeAddr, const TimeSyncResponse & aResponse);

    /// store the contact information of a node who just sent us a time change notification
    void StoreNotifyingContact(const uint64_t aNodeId, const IPAddress & aNodeAddr);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    /// process a response coming back from a unicast request
    void UpdateUnicastSyncResponse(const TimeSyncResponse & aResponse);

    // wrap up a local sync and calculate the correction
    void EndLocalSyncAndTryCalculateTimeFix(void);

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    // wrap up a sync with the service and calculate the correction
    void EndServiceSyncAndTryCalculateTimeFix(void);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE

    /// induce callback to the application layer.
    /// set aIsSuccessful to false to induce the error callback
    WEAVE_ERROR CallbackForSyncCompletion(const bool aIsSuccessful, bool aShouldUpdate,
        const bool aIsCorrectionReliable, const bool aIsFromServer, const uint8_t aNumContributor,
        const timesync_t aSystemTimestamp_usec, const timesync_t aDiffTime_usec);

    /// internal abort if aCode is not WEAVE_NO_ERROR
    void AbortOnError(const WEAVE_ERROR aCode);

    /// register communication error on a certain contact, and shorten auto discovery period if needed
    /// aContact can be NULL to indicate we have no one to talk to, and hence just shorten the auto discovery period
    void RegisterCommError(Contact * const aContact = NULL);

    /// close the Weave ExchangeContext
    bool DestroyCommContext(void);

    /// create new Weave Exchange for unicast communication
    WEAVE_ERROR SetupUnicastCommContext(Contact * const aContact);

    /// send unicast sync request to a contact.
    /// *rIsMessageSent will be set to indicate if the message has been sent out.
    /// communication errors like address not reachable is not returned,
    /// so caller shall check both the return code and *rIsMessageSent.
    WEAVE_ERROR SendSyncRequest(bool * const rIsMessageSent, Contact * const aContact);

    void SetClientState(const ClientState state);
    const char * const GetClientStateName(void) const;

    /// internal function to kick off an auto sync session
    void AutoSyncNow(void);

    //@{
    /// these state transition functions are internal and cannot return error code as the previous state would have no way
    /// to handle them. any failure shall result to eventually another state transition (could be timeout)
    /// if even the timer fails, we are out of trick and could hang in some wrong state
#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    void EnterState_Discover(void);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    void EnterState_Sync_1(void);
    void EnterState_Sync_2(void);

#if WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    void EnterState_ServiceSync_1(void);
    void EnterState_ServiceSync_2(void);
#endif // WEAVE_CONFIG_TIME_CLIENT_CONNECTION_FOR_SERVICE
    //@}

    static void HandleTimeChangeNotification(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleUnicastSyncResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

#if WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY
    static void HandleMulticastResponseTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static void HandleMulticastSyncResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleAutoDiscoveryTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
#endif // WEAVE_CONFIG_TIME_CLIENT_FABRIC_LOCAL_DISCOVERY

    static void HandleUnicastResponseTimeout(ExchangeContext * const ec);

    static void HandleAutoSyncTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

#endif // WEAVE_CONFIG_TIME_ENABLE_CLIENT

};

class NL_DLL_EXPORT SingleSourceTimeSyncClient
{
public:

    /// current state of this Time Sync Client
    enum ClientState
    {
        kClientState_Idle,      //< Initialized, waiting for Time Change Notification, but no actual time sync operation is happening
        kClientState_Sync_1,    //< Working on the first time sync attempt
        kClientState_Sync_2,    //< Working on the second time sync attempt
    };

    /**
     *  @brief
     *  Retrieve current state of this client
     *
     *  @return current state
     */
    ClientState GetClientState(void) const { return mClientState; };

    /**
     *  @brief
     *  Initialize this client. Must be called before other functions can be used.
     *  Zero/NULL initialize all internal data and register with Time Change Notification.
     *
     *  @param[in] aApp             A pointer to higher layer data, used in callbacks to higher layer.
     *
     *  @param[in] aExchangeMgr     A pointer to Exchange Manager, which would be used in registering for Time Change Notification message handler
     *
     *  @return WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(void * const aApp, WeaveExchangeManager * const aExchangeMgr);

    /**
     *  @brief
     *  Abort current time sync operation. Release Binding. Abort active exchange. Move back to idle state.
     *
     */
    void Abort(void);

    /**
     *  @brief
     *  Callback to indicate we just received a Time Change Notification.
     *  Set to NULL at #Init. If not set, Time Change Notification would be ignored.
     *  App layer is allowed to call Abort and Sync in this callback.
     *
     *  @param[in] aApp     A pointer to app layer data, set in Init.
     *  @param[in] aEC      Exchange context used for this incoming message, which can be used to validate its authenticity
     *
     */
    typedef void (*TimeChangeNotificationHandler)(void * const aApp, ExchangeContext * aEC);
    TimeChangeNotificationHandler OnTimeChangeNotificationReceived;

    /**
     *  @brief
     *  Callback after both time sync attempts have been completed. If #aErrorCode is WEAVE_NO_ERROR,
     *  at least one attempt has succeeded. Otherwise both failed at #aErrorCode indicates the latest failure.
     *
     *  @param[in] aApp                 A pointer to app layer data, set in Init.
     *
     *  @param[in] aErrorCode           #WEAVE_NO_ERROR if at least one time sync operation is successful
     *
     *  @param[in] aCorrectedSystemTime Only valid if #aErrorCode is #WEAVE_NO_ERROR
     */
    typedef void (*SyncCompletionHandler)(void * const aApp, const WEAVE_ERROR aErrorCode, const timesync_t aCorrectedSystemTime);

    /**
     *  @brief
     *  Sync using the given Binding and makes a callback using the pointer provided.
     *  If there is a time sync operation going on, it would be aborted implicitly without callback being made.
     *  Not available in callback #OnSyncCompleted, but allowed in #OnTimeChangeNotificationReceived .
     *  On error, Abort would be called implicitly before returning from this function.
     *
     *  @param[in]  aBinding            Binding to be used in contacting the time server
     *
     *  @param[in]  OnSyncCompleted     Callback function to be used after the time sync operations are completed
     *
     *  @return                         #WEAVE_NO_ERROR on success
     *
     */
    WEAVE_ERROR Sync(Binding * const aBinding, SyncCompletionHandler OnSyncCompleted);

protected:

    enum
    {
        kFlightTimeMinimum = 0,
        kFlightTimeInvalid = -1
    };

    void * mApp;
    WeaveExchangeManager * mExchangeMgr;
    Binding * mBinding;
    bool mIsInCallback;
    ClientState mClientState;

    ExchangeContext * mExchageContext;

    /// used to store one way flight time.
    int32_t mFlightTime_usec;

    timesync_t mUnadjTimestampLastSent_usec;

    /// used to store the system time of remote node, when the
    /// response message was about to be sent
    timesync_t mRemoteTimestamp_usec;

    /// used to store Timestamp when a result is registered
    timesync_t mRegisterSyncResult_usec;

    SyncCompletionHandler mOnSyncCompleted;

    void RegisterSyncResultIfNewOrBetter(const timesync_t aNow_usec, const timesync_t aRemoteTimestamp_usec, const int32_t aFlightTime_usec);

    void _AbortWithCallback(const WEAVE_ERROR aErrorCode);

    WEAVE_ERROR SendSyncRequest(void);

    void SetClientState(const ClientState state);
    const char * const GetClientStateName(void) const;

    static void HandleTimeChangeNotification(ExchangeContext *aEC, const IPPacketInfo *aPktInfo,
        const WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload);

    static void HandleSyncResponse(ExchangeContext *aEC, const IPPacketInfo *aPktInfo,
        const WeaveMessageInfo *aMsgInfo, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload);

    void OnSyncResponse(uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aPayload);

    static void HandleResponseTimeout(ExchangeContext *aEC);
    void OnResponseTimeout(void);

    /// Invalidate the registered information for time correction
    inline void InvalidateRegisteredResult(void) { mFlightTime_usec = kFlightTimeInvalid; };

    /// Check if the registered information for time correction is valid
    inline bool IsRegisteredResultValid(void) { return mFlightTime_usec >= kFlightTimeMinimum; };

    void ProceedToNextState(void);
    void EnterSync2(void);
    void FinalProcessing(void);
};

} // namespace Time

} // namespace Profiles

/// Platform-provided time routines.
namespace  Platform
{

namespace  Time
{
/**
 * get CLOCK_MONOTONIC_RAW, CLOCK_MONOTONIC, or equivalent clock reading.
 * This clock is used to timestamp events that happen with short time in between them.
 * Higher resolution is expected but doesn't have to be compensated for sleep time.
 * Note that it is okay if it comes with sleep time compensation, but higher resolution is the key.
 * Without better alternatives, this can be implemented by GetSleepCompensatedMonotonicTime.
 *
 * @param[out] p_timestamp_usec A pointer to the clock reading
 *
 * @return WEAVE_NO_ERROR on success
 */
 NL_DLL_EXPORT WEAVE_ERROR GetMonotonicRawTime(Profiles::Time::timesync_t * const p_timestamp_usec);

/**
 * get CLOCK_REALTIME or equivalent clock reading.
 *
 * @param[out] p_timestamp_usec A pointer to the clock reading
 *
 * @return WEAVE_NO_ERROR on success
 */
NL_DLL_EXPORT WEAVE_ERROR GetSystemTime(Profiles::Time::timesync_t * const p_timestamp_usec);

/**
 * get CLOCK_REALTIME or equivalent clock reading in ms.
 *
 * @param[out] p_timestamp_msec A pointer to the clock reading
 *
 * @return WEAVE_NO_ERROR on success
 */
NL_DLL_EXPORT WEAVE_ERROR GetSystemTimeMs(Profiles::Time::timesync_t * const p_timestamp_msec);

/**
 * set CLOCK_REALTIME or equivalent clock.
 *
 * @param[in] timestamp_usec intended new value for CLOCK_REALTIME
 *
 * @return WEAVE_NO_ERROR on success
 */
NL_DLL_EXPORT WEAVE_ERROR SetSystemTime(const Profiles::Time::timesync_t timestamp_usec);

/**
 * get CLOCK_BOOTTIME or equivalent clock reading.
 * This clock is used to timestamp events that happen long time apart.
 * Highest resolution is not the concern but it must be compensated for sleep time.
 *
 * @param[out] p_timestamp_usec A pointer to the clock reading
 *
 * @return WEAVE_NO_ERROR on success
 */
NL_DLL_EXPORT WEAVE_ERROR GetSleepCompensatedMonotonicTime(Profiles::Time::timesync_t * const p_timestamp_usec);

} // namespace Time

} // namespace Platform

} // namespace Weave

} // namespace nl

#endif // WEAVE_TIME_H_

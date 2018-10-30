/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file declares mock event generators
 *
 */
#ifndef MOCKEVENTS_H
#define MOCKEVENTS_H

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <InetLayer/Inet.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/ProfileCommon.h>

// We want and assume the default managed namespace is Current and that is, explicitly, the managed namespace this code desires.
#include <Weave/Profiles/data-management/DataManagement.h>
#include <nest/test/trait/TestETrait.h>
#include <SystemLayer/SystemTimer.h>
#include <MockLoggingManager.h>
#include <Weave/Profiles/data-management/EventLoggingTypes.h>
// Several event definitions, hand generated at this time

#define PHOENIX_RESOURCE_STRINGS

#ifdef PHOENIX_RESOURCE_STRINGS
typedef uint8_t * user_id_t;
#else
typedef uint64_t user_id_t;
#endif /* PHOENIX_RESOURCE_STRINGS */

/************************************************************************/

// Debug trait may be accessed via
// nl::Weave::Profiles::DataManagement::LogFreeform function.  Debug is
// interesting because it is a non-production event, and it contains a
// freeform string

/************************************************************************/

// Liveness event.  Interesting because it is about a different resource

enum LivenessDeviceStatus
{
    LIVENESS_DEVICE_STATUS_UNSPECIFIED    = 0, /// Device status is unspecified
    LIVENESS_DEVICE_STATUS_ONLINE         = 1, /// Device is sending messages
    LIVENESS_DEVICE_STATUS_UNREACHABLE    = 2, /// Device is not reachable over network
    LIVENESS_DEVICE_STATUS_UNINITIALIZED  = 3, /// Device has not been initialized
    LIVENESS_DEVICE_STATUS_REBOOTING      = 4, /// Device being rebooted
    LIVENESS_DEVICE_STATUS_UPGRADING      = 5, /// Device offline while upgrading
    LIVENESS_DEVICE_STATUS_SCHEDULED_DOWN = 6  /// Device on a scheduled downtime
};

nl::Weave::Profiles::DataManagement::event_id_t LogLiveness(uint64_t inNodeID, LivenessDeviceStatus inStatus);

/************************************************************************/

// Pincode input trait.  Interesting because it may explore the related events with the Bolt lock trait

enum PincodeEntryResult
{
    PINCODE_ENTRY_RESULT_UNSPECIFIED              = 0,
    PINCODE_ENTRY_RESULT_FAILURE_INVALID_PINCODE  = 1, /**< Pincode entered does not exist ad is invalid*/
    PINCODE_ENTRY_RESULT_FAILURE_OUT_OF_SCHEDULE  = 2, /**< Pincode used outside the schedule time limits  */
    PINCODE_ENTRY_RESULT_FAILURE_PINCODE_DISABLED = 3, /**< Pincode used is disabled  */
    PINCODE_ENTRY_RESULT_SUCCESS                  = 4  /**< Pincode entry successful  */
};

enum CredentialStatus
{
    CREDENTIAL_STATUS_UNSPECIFIED                 = 0,
    CREDENTIAL_STATUS_ENABLED                     = 1, /**< Credential is usable */
    CREDENTIAL_STATUS_DISABLED                    = 2, /**< Credential is disabled and cannot be used */
    CREDENTIAL_STATUS_DOES_NOT_EXIST              = 3  /**< Credential does not exist */
};

struct KeypadEntryEventStruct
{
    KeypadEntryEventStruct();
    user_id_t user_id;
    uint32_t invalidEntryCount;
    int16_t status;
    int16_t entryResult;
};

struct UserDisabledEventStruct
{
    UserDisabledEventStruct();
    user_id_t user_id;
    bool disabled;
};

nl::Weave::Profiles::DataManagement::event_id_t LogKeypadEntry(CredentialStatus inCred, PincodeEntryResult inResult, user_id_t inUserID);

nl::Weave::Profiles::DataManagement::event_id_t LogKeypadEnable(bool inEnable, user_id_t inUserID);

/************************************************************************/

// Bolt lock trait.  Interesting because it interacts with the keypad event.

enum BoltState
{
    BOLT_STATE_STATE_UNSPECIFIED             = 0,
    BOLT_STATE_RETRACTED                     = 1, /**< Bolt is fully retracted */
    BOLT_STATE_EXTENDED                      = 2  /**< Bolt is AT LEAST partially extended. */
};

enum BoltLockActorMethod {
    BOLT_LOCK_ACTOR_METHOD_UNSPECIFIED = 0,

    /**
      * Change produced by an unspecified entity that doesn't fall into one of the defined categories below
      *  - Who may not be present
      *  - Expected to be correlated with an event
      */
    BOLT_LOCK_ACTOR_METHOD_OTHER = 1,

    /// On-device actor methods
    /**
      * Change produced by the physical interaction on the lock but with no PIN entered (Typically for locking or unlocking with the thumb turn)
      *  - Who MUST NOT be present
      */
    BOLT_LOCK_ACTOR_METHOD_PHYSICAL = 2,

    /**
      * Change produced by the physical keypad by typing in a PIN
      *  - Who MUST be present
      *  - Expected to be correlated with an event that represents "keypad pin entered successfully" (see PinCodeInputTrait for an example event)
      */
    BOLT_LOCK_ACTOR_METHOD_KEYPAD_PIN = 3,

    /**
      * Change produced by the device internally. For instance, if the device has a mechanisim to automatically lock
      * after a period of time.
      * - Who may not be present.
    */
    BOLT_LOCK_ACTOR_METHOD_LOCAL_IMPLICIT = 4,

    /// App actor methods
    /**
      * Change produced by a user using the app
      *  - Who may not be present
      */
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_EXPLICIT = 5,

    /**
      * Change produced by a user with implicit intent
      * ex. Toggling structure mode with the app mode switcher
      *  - Who may not be present
      *  - Expected to be correlated with a structure mode change event?
      */
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_IMPLICIT = 6,

    /**
      * Change produced by a user that does not fit in one of the
      * other app remote categories
      */
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_OTHER = 7,

    /// Other actor methods
    /**
      * Change produced by a remote service with no user intent (ex: Goose)
      *  - Who may not be present
      *  - Expected to be correlated with an event (???)
      */
    BOLT_LOCK_ACTOR_METHOD_REMOTE_DELEGATE = 8,

    /**
      * Lock will (???) on its own before a critically low battery shutdown
      */
    BOLT_LOCK_ACTOR_METHOD_LOW_POWER_SHUTDOWN = 9,
};

enum BoltActuatorState {
    BOLT_ACTUATOR_STATE_UNSPECIFIED = 0,
    BOLT_ACTUATOR_STATE_OK = 1,         /// Actuator is not moving and is in a fully settled position.  If its not settled use BOLT_ACTUATOR_STATE_MOVING
    BOLT_ACTUATOR_STATE_LOCKING = 2,    /// Actuator is attempting to lock
    BOLT_ACTUATOR_STATE_UNLOCKING = 3,  /// Actuator is attempting to unlock
    /** Actuator is moving though it’s not necessarily clear the intended direction
     * This is typically used when the user is physically manipulating the bolt
     */
    BOLT_ACTUATOR_STATE_MOVING = 4,
    BOLT_ACTUATOR_STATE_JAMMED_LOCKING = 5,     /// Actuator jammed while the device was locking and cannot move.
    BOLT_ACTUATOR_STATE_JAMMED_UNLOCKING = 6,   /// Actuator jammed while the device was unlocking and cannot move.
    BOLT_ACTUATOR_STATE_JAMMED_OTHER = 7,       /// Actuator jammed and cannot move. The direction the lock was moving was not provided.
};

enum BoltLockedState {
    BOLT_LOCKED_STATE_UNSPECIFIED = 0,
    /** The lock as a whole is not locked to the best knowledge of the implementor.
     * i.e. the door that this lock is attached to can be opened
     */

    BOLT_LOCKED_STATE_UNLOCKED = 1,
    /** The lock as a whole is locked to the best knowledge of the implementor.
     *  i.e. the door that this lock is attached to cannot be opened
     */

    BOLT_LOCKED_STATE_LOCKED = 2,
    /** The implementor is not able to determine whether the door is locked or unlocked.
     *  i.e. "Locked" sensing is based on a history of readings and that history is now gone or lost
     */
    BOLT_LOCKED_STATE_UNKNOWN = 3,
};

struct BoltLockActorStruct
{
    BoltLockActorStruct(void);

    int16_t method;
    user_id_t user_id;
};

struct BoltActuatorEventStruct
{
    BoltActuatorEventStruct(void);

    int16_t state;
    int16_t actuatorState;
    int16_t lockedState;
    BoltLockActorStruct bolt_lock_actor;
    uint64_t locked_state_last_changed_at;
};

nl::Weave::Profiles::DataManagement::event_id_t LogBoltStateChange(BoltState inState, BoltActuatorState inActuatorState, BoltLockedState inLockedState, BoltLockActorStruct inBolt_lock_actor, nl::Weave::Profiles::DataManagement::utc_timestamp_t inLocked_state_last_changed_at, nl::Weave::Profiles::DataManagement::event_id_t inEventID);

/************************************************************************/

// open/close event.  Interesting because it is the canonical example of Maldives.

enum OpenCloseState {
  OPEN_CLOSE_STATE_UNSPECIFIED   = 0,  /// Reserved for internal use, please do not use.
  OPEN_CLOSE_STATE_CLOSED        = 1,
  OPEN_CLOSE_STATE_OPEN          = 2,
  OPEN_CLOSE_STATE_UNKNOWN       = 3   /// State unknown or not applicable
};

nl::Weave::Profiles::DataManagement::event_id_t LogOpenClose(OpenCloseState inState);

/************************************************************************/

class DebugEventGenerator : public EventGenerator
{
public:
    DebugEventGenerator(void);
    void Generate(void);
private:
    const char **mLogLines;
};

class LivenessEventGenerator : public EventGenerator
{
public:
    LivenessEventGenerator(void);
    void Generate(void);
};

class SecurityEventGenerator : public EventGenerator
{
public:
    SecurityEventGenerator(void);
    void Generate(void);

private:
    nl::Weave::Profiles::DataManagement::event_id_t mRelatedEvent;
};

class TelemetryEventGenerator : public EventGenerator
{
public:
    TelemetryEventGenerator(void);
    void Generate(void);
};

class TestTraitEventGenerator : public EventGenerator
{
public:
    TestTraitEventGenerator(void);
    void Generate(void);

private:
    Schema::Nest::Test::Trait::TestETrait::TestEEvent mEvent;
    Schema::Nest::Test::Trait::TestETrait::TestENullableEvent mNullableEvent;
    uint8_t tek_buf[20];
    uint8_t ten_buf[10];
    uint16_t ten_resource_type;
};
#endif /* MOCKEVENTS_H */

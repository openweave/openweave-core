/*
 *
 *    Copyright (c) 2018 Google LLC.
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements a few mock event generators
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <new>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nlbyteorder.h>
#include "ToolCommon.h"

#include <InetLayer/Inet.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveTLVDebug.hpp>
#include <Weave/Core/WeaveTLVUtilities.hpp>
#include <Weave/Core/WeaveTLVData.hpp>
#include <Weave/Core/WeaveSecurityMgr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/TraitEventUtils.h>

#include <MockEvents.h>

#include "schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.h"
#include <schema/weave/common/DayOfWeekEnum.h>
#include <schema/nest/test/trait/TestCommon.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Maldives_prototype::Trait::Flintstone::NetworkWiFiTelemetryTrait;
using namespace Schema::Nest::Test::Trait::TestETrait;
using namespace Schema::Nest::Test::Trait;
using namespace Schema::Weave::Common;

#ifdef PHOENIX_RESOURCE_STRINGS
#define USER_ID_INITIAL NULL
static uint8_t kTestUserId[1]         = { 1 };
#else
#define USER_ID_INITIAL 0
static const user_id_t kTestUserId    = 0x0123456789ULL;
#endif /* PHOENIX_RESOURCE_STRINGS */

static const uint64_t kTestNodeId     = 0x18B4300001408362ULL;
static const uint64_t kTestNodeId1    = 0x18B43000002DCF71ULL;

// ResourceID helper

// tags for ResourceID structure
#ifdef PHOENIX_RESOURCE_STRINGS

WEAVE_ERROR WriteResourceID(nl::Weave::TLV::TLVWriter & writer, const uint64_t & tag, user_id_t resourceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(resourceId != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = writer.PutBytes(tag, resourceId, sizeof(resourceId));
    SuccessOrExit(err);

exit:
    return err;
}

#else // PHOENIX_RESOURCE_STRINGS

WEAVE_ERROR WriteResourceID(nl::Weave::TLV::TLVWriter & writer, const uint64_t & tag, user_id_t resourceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (resourceId != 0)
        err = writer.Put(tag, resourceId);

    return err;
}
#endif // PHOENIX_RESOURCE_STRINGS


const uint32_t kLivenessTraitID           = 0x00000022;
const uint32_t kLivenessChangeEvent       = 1;
const uint64_t kLivenessDeviceStatus      = nl::Weave::TLV::ContextTag(1);

static WEAVE_ERROR WriteLivenessStatusEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType liveness;
    int32_t * context = static_cast<int32_t *>(anAppState);

    VerifyOrExit(context != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, liveness);
    SuccessOrExit(err);

    err = writer.Put(kLivenessDeviceStatus, *context);
    SuccessOrExit(err);

    err = writer.EndContainer(liveness);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

event_id_t LogLiveness(uint64_t inNodeID, LivenessDeviceStatus inStatus)
{
    nl::Weave::Profiles::DataManagement::EventOptions options;
    nl::Weave::Profiles::DataManagement::DetailedRootSection root;
    static EventSchema schema = {
        kLivenessTraitID,
        kLivenessChangeEvent,
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };
    int32_t status = static_cast<int32_t>(inStatus);

    root.ResourceID = inNodeID;
    root.TraitInstanceID = 0;

    options = EventOptions((uint32_t)0, &root, 0, nl::Weave::Profiles::DataManagement::kImportanceType_Invalid, true);

    return nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteLivenessStatusEvent,
        &status,
        &options);
}

/************************************************************************/

// Pincode input trait.
const uint32_t kPincodeInputTraitID               = 0x00000e05;
const uint32_t kKeypadEntryEvent                  = 1;
const uint32_t kUserDisabledEvent                 = 2;

const uint64_t kPincodeStatus                     = nl::Weave::TLV::ContextTag(1);
const uint64_t kUserID                            = nl::Weave::TLV::ContextTag(2);
const uint64_t kInvalidEntryCount                 = nl::Weave::TLV::ContextTag(3);
const uint64_t kPincodeEntryResult                = nl::Weave::TLV::ContextTag(4);

const uint64_t kUserDisabled                      = nl::Weave::TLV::ContextTag(1);

KeypadEntryEventStruct::KeypadEntryEventStruct() :
    user_id(USER_ID_INITIAL),
    invalidEntryCount(0),
    status(0),
    entryResult(0)
{
}

UserDisabledEventStruct::UserDisabledEventStruct() :
    user_id(USER_ID_INITIAL),
    disabled(false)
{
}

static WEAVE_ERROR WriteKeypadEntryEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType keypadEntry;
    KeypadEntryEventStruct * context = static_cast<KeypadEntryEventStruct *>(anAppState);

    VerifyOrExit(context != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, keypadEntry);
    SuccessOrExit(err);

    err = writer.Put(kPincodeStatus, context->status);
    SuccessOrExit(err);

    err = WriteResourceID(writer, kUserID, context->user_id);
    SuccessOrExit(err);

    err = writer.Put(kInvalidEntryCount, context->invalidEntryCount);
    SuccessOrExit(err);

    err = writer.Put(kPincodeEntryResult, context->entryResult);
    SuccessOrExit(err);

    err = writer.EndContainer(keypadEntry);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

event_id_t LogKeypadEntry(CredentialStatus inStatus, PincodeEntryResult inResult, user_id_t inUserID)
{
    static KeypadEntryEventStruct eventStruct;
    static EventSchema schema = {
        kPincodeInputTraitID,
        kKeypadEntryEvent,
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    if (inResult == PINCODE_ENTRY_RESULT_SUCCESS)
    {
        eventStruct.invalidEntryCount = 0;
    }
    else
    {
        eventStruct.invalidEntryCount++;
    }

    eventStruct.user_id = inUserID;
    eventStruct.status = inStatus;
    eventStruct.entryResult = inResult;

    return nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteKeypadEntryEvent,
        &eventStruct);
}

static WEAVE_ERROR WriteUserDisabledEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType userDisabled;
    UserDisabledEventStruct * context = static_cast<UserDisabledEventStruct *>(anAppState);

    VerifyOrExit(context != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, userDisabled);
    SuccessOrExit(err);

    err = writer.PutBoolean(kUserDisabled, context->disabled);
    SuccessOrExit(err);

    err = WriteResourceID(writer, kUserID, context->user_id);
    SuccessOrExit(err);

    err = writer.EndContainer(userDisabled);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

event_id_t LogKeypadEnable(bool inEnable, user_id_t inUserID)
{
    UserDisabledEventStruct eventStruct;
    static EventSchema schema = {
        kPincodeInputTraitID,
        kUserDisabledEvent,
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    eventStruct.user_id = inUserID;
    eventStruct.disabled = !inEnable;

    return nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteUserDisabledEvent,
        &eventStruct);
}

/************************************************************************/

// Bolt lock trait.

const uint32_t kBoltLockTraitID                     = 0x00000e02;
const uint32_t kBoltActuatorStateChangeEvent        = 1;

const uint64_t kBoltState                           = nl::Weave::TLV::ContextTag(1);
const uint64_t kActuatorState                       = nl::Weave::TLV::ContextTag(2);
const uint64_t kLockedState                         = nl::Weave::TLV::ContextTag(3);
const uint64_t kbolt_lock_actor                     = nl::Weave::TLV::ContextTag(4);
const uint64_t klocked_state_last_changed_at        = nl::Weave::TLV::ContextTag(5);

const uint64_t kbolt_lock_actor_method                     = nl::Weave::TLV::ContextTag(1);
const uint64_t kbolt_lock_actor_user_id                     = nl::Weave::TLV::ContextTag(2);

BoltActuatorEventStruct::BoltActuatorEventStruct() :
    state(0),
    actuatorState(0),
    lockedState(0),
    bolt_lock_actor(),
    locked_state_last_changed_at(0)
{
    return;
}

BoltLockActorStruct::BoltLockActorStruct() :
    method(0),
    user_id(USER_ID_INITIAL)
{
    return;
}

static WEAVE_ERROR WriteBoltActuatorStateChangeEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType userDisabled, lockActor;
    BoltActuatorEventStruct * context = static_cast<BoltActuatorEventStruct *>(anAppState);

    VerifyOrExit(context != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, userDisabled);
    SuccessOrExit(err);

    err = writer.Put(kBoltState, context->state);
    SuccessOrExit(err);

    err = writer.Put(kActuatorState, context->actuatorState);
    SuccessOrExit(err);

    err = writer.Put(kLockedState, context->lockedState);
    SuccessOrExit(err);

    err = writer.StartContainer(kbolt_lock_actor, nl::Weave::TLV::kTLVType_Structure, lockActor);
    SuccessOrExit(err);

    err = writer.Put(kbolt_lock_actor_method, context->bolt_lock_actor.method);
    SuccessOrExit(err);

    err = WriteResourceID(writer, kbolt_lock_actor_user_id, context->bolt_lock_actor.user_id);
    SuccessOrExit(err);

    err = writer.EndContainer(lockActor);
    SuccessOrExit(err);

    err = writer.Put(klocked_state_last_changed_at, context->locked_state_last_changed_at);
    SuccessOrExit(err);

    err = writer.EndContainer(userDisabled);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

event_id_t LogBoltStateChange(BoltState inState, BoltActuatorState inActuatorState, BoltLockedState inLockedState, BoltLockActorStruct inBolt_lock_actor, utc_timestamp_t inLocked_state_last_changed_at, event_id_t inEventID)
{
    nl::Weave::Profiles::DataManagement::EventOptions options;
    BoltActuatorEventStruct eventStruct;
    static EventSchema schema = {
        kBoltLockTraitID,
        kBoltActuatorStateChangeEvent,
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };

    eventStruct.state = static_cast<int16_t>(inState);
    eventStruct.actuatorState = static_cast<int16_t>(inActuatorState);
    eventStruct.lockedState = static_cast<int16_t>(inLockedState);
    eventStruct.bolt_lock_actor = static_cast<BoltLockActorStruct>(inBolt_lock_actor);
    eventStruct.locked_state_last_changed_at = static_cast<uint64_t>(inLocked_state_last_changed_at);

    if (inEventID != 0)
    {
        options = EventOptions((uint32_t)0, NULL, inEventID, nl::Weave::Profiles::DataManagement::Production, false);
    }
    else
    {
        options = EventOptions();
    }


    return nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteBoltActuatorStateChangeEvent,
        &eventStruct,
        &options
        );
}

/************************************************************************/

// open/close event.  Interesting because it is the canonical example of Maldives.
const uint32_t kOpenCloseTraitID = 0x235A0208;
const uint32_t kOpenCloseEvent   = 1;

const uint64_t kOpenCloseState   = nl::Weave::TLV::ContextTag(1);

static WEAVE_ERROR WriteOpenCloseEvent(nl::Weave::TLV::TLVWriter & writer, uint8_t inDataTag, void * anAppState)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVType userDisabled;
    int32_t * context = static_cast<int32_t *>(anAppState);

    VerifyOrExit(context != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    err = writer.StartContainer(ContextTag(nl::Weave::Profiles::DataManagement::kTag_EventData), nl::Weave::TLV::kTLVType_Structure, userDisabled);
    SuccessOrExit(err);

    err = writer.Put(kOpenCloseState, *context);
    SuccessOrExit(err);

    err = writer.EndContainer(userDisabled);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

event_id_t LogOpenClose(OpenCloseState inState)
{
    static EventSchema schema = {
        kOpenCloseTraitID,
        kOpenCloseEvent,
        nl::Weave::Profiles::DataManagement::Production,
        1,
        1
    };
    int32_t eventState = static_cast<int32_t>(inState);

    return nl::Weave::Profiles::DataManagement::LogEvent(
        schema,
        WriteOpenCloseEvent,
        &eventState);
}

/************************************************************************/
// Event generators

EventGenerator::EventGenerator(size_t aNumStates, size_t aInitialState) :
    mNumStates(aNumStates),
    mState(aInitialState)
{
}

size_t EventGenerator::GetNumStates()
{
    return mNumStates;
}

// premiere selection of log lines from helloweave app

static const char* gLogLines[] =
{
    "Initializing weave platform",
    "StartWeave Setting LocalNodeId: 18b4300001408362",
    "Setting FabricId 93abf1086e41822",
    "Init: configuration Settings = 00000009",
    "RequestInvokeActions",
    "Init NM Daemon",
    "Watchdog ID = 1.",
    "nlWirelessCalPlatformLoadFromSysEnv: Loading From Environment for 'nlwirelessregcal.em357'",
    "setting application tag for task 0x20004148 ('NMGR') to 0x20009e20",
    "Init NM Client",
    "Waiting for events!",
    "Init pair fail: -1355284483",
    "setting application tag for task 0x20004194 ('VNCP') to 0x2000bc30",
    "Enabling watchdog tracking. ID = 2",
    "netif init",
    "[TECHBASE] \"6LoWPAN\" (0x66f6ae45) persistently enabled.",
    "[SILBS] SiLabs :: Probe - Starting 6lowpan status:0",
    "ember reset with cause 6",
    "CurrentNetwork id=NEST-PAN-1822, xPanId=B6096E5D00EBACD1, panId=be51, chan=19, nodeType=4, txPwr=-12, old status=0, new status=1, reason=0",
    "emberInitReturn (status: 0)",
    "[SILBS] HandleNetworkParametersChanged, status:1, name:NEST-PAN-1822, device role:4",
    "[SILBS] UpdateCurrentService: Creating new service",
    "[SVCBASE] NEST-PAN-1822: 1 (create) : 0 (unknown) -> 1 (idle)",
    "[SVCCTLR] NEST-PAN-1822: 0 (unknown) -> 1 (idle)",
    "[PROVCTLR] Non-partial Service \"NEST-PAN-1822\" (w/ Prov=0x1591d382) has score 3 against prov 0x1591d382.",
    "[PROVCTLR] ProvisionAction on Service \"NEST-PAN-1822\" (w/ Prov=0x1591d382) has score 3 against prov 0x1591d382.",
    "[PSKMIXIN] 0x2000a458: SetPSK set to (type:1, length16)",
    "[PROVDRVRBASE] Svc NEST-PAN-1822 matches prov 0x1591d382. Request AutoConnect.",
    "[SILBS] HandleSavedNetworkStatus: Resuming network",
    "(wpan) Thread bin.mgmt.ver:3328, stack.ver:1.0.5.0, build.ver:536 (Jun  1 2016 14:36:21)",
    "emberSetTxPowerModeReturn (status 0)",
    "CurrentNetwork id=NEST-PAN-1822, xPanId=B6096E5D00EBACD1, panId=be51, chan=19, nodeType=4, txPwr=-12, old status=0, new status=1, reason=0",
    "[SVCCTLR] RunAutoConnect: Start (1)",
    "[SVCCTLR] RunAutoConnect: Start (2)",
    "[SVCCTLR] RunAutoConnect: Start (3)",
    "[SVCCTLR] AC svc found NEST-PAN-1822: state=idle flags=auto_conn ",
    "[SVCCTLR] RunAutoConnect: 0,0",
    "[SVCCTLR] Internally-initiated connection to \"NEST-PAN-1822\" (0xb505acd3)",
    "[SVCCTLR] RunAutoConnect: Start (1)",
    "[SVCCTLR] RunAutoConnect: Start (2)",
    "[SVCCTLR] RunAutoConnect: Start (3)",
    "[SVCCTLR] RunAutoConnect: Exit due to CanProxy",
    "[SILBS] HandleNetworkParametersChanged, status:1, name:NEST-PAN-1822, device role:4",
    "[SILBS] HandleSavedNetworkStatus: In the process of resuming already.",
    "emberInitReturn (status: 0)",
    "(wpan) Thread bin.mgmt.ver:3328, stack.ver:1.0.5.0, build.ver:536 (Jun  1 2016 14:36:21)",
    "CurrentNetwork id=NEST-PAN-1822, xPanId=B6096E5D00EBACD1, panId=be51, chan=19, nodeType=4, txPwr=-12, old status=1, new status=5, reason=0",
    "resume network return (status: 0)",
    "[SILBS] Resume status:0",
    "[SVCBASE] NEST-PAN-1822: 7 (connect) : 1 (idle) -> 2 (association)",
    "[SVCCTLR] NEST-PAN-1822: 1 (idle) -> 2 (association)",
    "[SILBS] Connect: Join network NEST-PAN-1822",
    "[SILBS] HandleNetworkParametersChanged, status:5, name:NEST-PAN-1822, device role:4",
    "CurrentNetwork id=NEST-PAN-1822, xPanId=B6096E5D00EBACD1, panId=be51, chan=19, nodeType=3, txPwr=-12, old status=5, new status=5, reason=0",
    "[SILBS] HandleNetworkParametersChanged, status:5, name:NEST-PAN-1822, device role:3"
};

DebugEventGenerator::DebugEventGenerator() :
    EventGenerator(sizeof(gLogLines) / sizeof(char*), 0),
    mLogLines(gLogLines)
{
}

void DebugEventGenerator::Generate(void)
{
    nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Production, mLogLines[mState]);
    mState = (mState + 1) % mNumStates;
}

LivenessEventGenerator::LivenessEventGenerator(void) :
    EventGenerator(10, 0)
{
}

void LivenessEventGenerator::Generate(void)
{
    // Scenario: monitoring liveness for two devices -- self and remote.  Remote device goes offline and returns.
    switch (mState)
    {
    case 0:
        LogLiveness(kTestNodeId, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 1:
        LogLiveness(kTestNodeId1, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 2:
        LogLiveness(kTestNodeId, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 3:
        LogLiveness(kTestNodeId1, LIVENESS_DEVICE_STATUS_UNREACHABLE);
        break;

    case 4:
        LogLiveness(kTestNodeId, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 5:
        LogLiveness(kTestNodeId1, LIVENESS_DEVICE_STATUS_REBOOTING);
        break;

    case 6:
        LogLiveness(kTestNodeId, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 7:
        LogLiveness(kTestNodeId1, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 8:
        LogLiveness(kTestNodeId, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    case 9:
        LogLiveness(kTestNodeId1, LIVENESS_DEVICE_STATUS_ONLINE);
        break;

    default:
        mState = 0;
    }

    mState = (mState + 1) % mNumStates;
}

SecurityEventGenerator::SecurityEventGenerator(void) :
    EventGenerator(14, 0),
    mRelatedEvent(0)
{
}

void SecurityEventGenerator::Generate(void)
{
    // Scenario: debug logs are happening in the background. The user
    // of the device enters a wrong pincode, corrects it subsequently,
    // the bolt unlocks.  The door opens, and closes soon
    // thereafter. The user disables the pincode. Subsequently someone
    // else attempts to activate the keypad.

    uint64_t            now         = 0;
    now = System::Timer::GetCurrentEpoch();
    BoltLockActorStruct inBolt_lock_actor;

    switch (mState)
    {
    case 0:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Keypad Activated");
        break;

    case 1:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Wrong pincode: %-5d", 12345);
        break;

    case 2:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Keypad Activated");
        break;

    case 3:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Correct pincode: %-5d", 55555);
        break;

    case 4:
        // valid credential, trigger the BoltStateChange
        inBolt_lock_actor.method = BOLT_LOCK_ACTOR_METHOD_KEYPAD_PIN;
        inBolt_lock_actor.user_id = kTestUserId;

        LogBoltStateChange(BOLT_STATE_EXTENDED, BOLT_ACTUATOR_STATE_UNLOCKING, BOLT_LOCKED_STATE_LOCKED, inBolt_lock_actor, now, mRelatedEvent);
        break;

    case 5:
        inBolt_lock_actor.method = BOLT_LOCK_ACTOR_METHOD_KEYPAD_PIN;
        inBolt_lock_actor.user_id = kTestUserId;

        LogBoltStateChange(BOLT_STATE_RETRACTED, BOLT_ACTUATOR_STATE_OK, BOLT_LOCKED_STATE_UNLOCKED, inBolt_lock_actor, now, mRelatedEvent);

        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Successful unlock");
        break;

    case 6:

        break;

    case 7:

        break;

    case 8:
        // lets lock the door manually (no known user ID)
        inBolt_lock_actor.method = BOLT_LOCK_ACTOR_METHOD_PHYSICAL;
        inBolt_lock_actor.user_id = 0UL;
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Manual locking from inside");
        LogBoltStateChange(BOLT_STATE_RETRACTED, BOLT_ACTUATOR_STATE_LOCKING, BOLT_LOCKED_STATE_UNLOCKED, inBolt_lock_actor, now, 0);
        break;

    case 9:
        inBolt_lock_actor.method = BOLT_LOCK_ACTOR_METHOD_PHYSICAL;
        inBolt_lock_actor.user_id = 0UL;
        LogBoltStateChange(BOLT_STATE_EXTENDED, BOLT_ACTUATOR_STATE_OK, BOLT_LOCKED_STATE_LOCKED, inBolt_lock_actor, now, 0);
        break;

    case 10:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Keypad Activated");
        break;

    case 11:
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Correct pincode: %-5d", 55555);
        break;

    case 12:
        // and disable the keypad
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Keypad disabled");
        break;

    case 13:
        // and disable the keypad
        nl::Weave::Profiles::DataManagement::LogFreeform(nl::Weave::Profiles::DataManagement::Debug, "Keypad disabled");
        break;

    default:
        mState = 0;
    }

    mState = (mState + 1) % mNumStates;
}


TelemetryEventGenerator::TelemetryEventGenerator(void) :
    EventGenerator(8, 0)
{
}

void TelemetryEventGenerator::Generate(void)
{
    NetworkWiFiStatsEvent event;
    NetworkWiFiDeauthEvent deauth;
    NetworkWiFiInvalidKeyEvent invalidkey;
    NetworkWiFiDHCPFailureEvent dhcpfail;

    event.bssid = 0x01de;
    event.freq = 11;
    event.rssi = -62;
    event.bcnRecvd = 0;
    event.bcnLost = 0;
    event.pktMCastRX = 0;
    event.pktUCastRX = 0;
    event.currRXRate = 6;
    event.currTXRate = 6;
    event.sleepTimePercent = 70;
    event.numOfAP = 1;

    // TelemetryEventGenerator generates WiFi telemetry events:
    // StatsEvent, DeauthEvent, InvalidKeyEvent and DHCPFailureEvent in sequence.
    //
    // The first 5 events are StatsEvents with bcnRecvd/pktUcastRX/sleepTimePercent udpated
    // The 6th event is DeauthEvent
    // The 7th event is InvalidKeyEvent
    // The 8th event is DHCPFailureEvent

    if (mState < 5)
    {
        event.bcnRecvd += mState;
        event.pktUCastRX += mState;
        event.sleepTimePercent += mState;
        LogNetworkWiFiStatsEvent(&event, nl::Weave::Profiles::DataManagement::Production);
    }
    else if (mState == 5)
    {
        deauth.reason = -16;
        LogNetworkWiFiDeauthEvent(&deauth, nl::Weave::Profiles::DataManagement::Production);
    }
    else if (mState == 6)
    {
        invalidkey.reason = -10;
        LogNetworkWiFiInvalidKeyEvent(&invalidkey, nl::Weave::Profiles::DataManagement::Production);
    }
    else if (mState == 7)
    {
        dhcpfail.reason = -40;
        LogNetworkWiFiDHCPFailureEvent(&dhcpfail, nl::Weave::Profiles::DataManagement::Production);
    }

    mState = (mState + 1) % mNumStates;
}

TestTraitEventGenerator::TestTraitEventGenerator(void) :
    EventGenerator(3, 0)
{
    mEvent = { 0 };
    mNullableEvent = { 0 };
}

void TestTraitEventGenerator::Generate(void)
{
    static const char *kTestString = "teststring";
    switch (mState)
    {
        case 0:
            // init state
            mEvent.teA = 5;
            mEvent.teB = -5;
            mEvent.teC = true;
            mEvent.teD = ENUM_E_VALUE_1;
            mEvent.teE.seA = 200;
            mEvent.teE.seB = true;
            mEvent.teE.seC = TestCommon::COMMON_ENUM_E_VALUE_2;
            mEvent.teF = TestCommon::COMMON_ENUM_E_VALUE_1;
            mEvent.teG.seA = 200;
            mEvent.teG.seB = true;
            mEvent.teJ = -900;

            memset(&tek_buf[0], 0xAA, sizeof(tek_buf));
            mEvent.teK.mBuf = &tek_buf[0];
            mEvent.teK.mLen = sizeof(tek_buf);

            // day of week
            mEvent.teL = DAY_OF_WEEK_SUNDAY;

            //implicit resource id
            mEvent.teM = 0x18b4300000000001ULL;

            // explicit resource id
            ten_resource_type = (ten_resource_type + 1) % 8;
            {
                uint8_t *tmp = &ten_buf[0];
                nl::Weave::Encoding::LittleEndian::Write16(tmp, ten_resource_type);
                nl::Weave::Encoding::LittleEndian::Write64(tmp, mEvent.teM);
            }
            mEvent.teN.mBuf = &ten_buf[0];
            mEvent.teN.mLen = sizeof(ten_buf);

            // timestamp
            mEvent.teO = 1493336639;
            mEvent.teP = 1493336639000;

            // duration
            mEvent.teQ = -1000;
            mEvent.teR = 1000;
            mEvent.teS = 20000;

            nl::LogEvent(&mEvent);

            mNullableEvent.neA = 300;
            mNullableEvent.neB = -300;
            mNullableEvent.neC = true;
            mNullableEvent.neD = kTestString;
            mNullableEvent.neI = kTestString;
            mNullableEvent.neE = 600;
            mNullableEvent.neJ.neA = 100;
            mNullableEvent.neJ.neA = true;

            nl::LogEvent(&mNullableEvent);
            break;

        case 1:
            // day of week
            mEvent.teL ^= DAY_OF_WEEK_FRIDAY;
            nl::LogEvent(&mEvent);
            break;
        case 2:
            // implicit resource id
            ten_resource_type = (ten_resource_type + 1) % 8;
            {
                uint8_t *tmp = &ten_buf[0];
                nl::Weave::Encoding::LittleEndian::Write16(tmp, ten_resource_type);
                nl::Weave::Encoding::LittleEndian::Write64(tmp, mEvent.teM);
            }
            mEvent.teN.mBuf = &ten_buf[0];
            mEvent.teN.mLen = sizeof(ten_buf);

            nl::LogEvent(&mEvent);
            break;
        case 3:
            // timestamp/duration
            // timestamp
            mEvent.teO++;
            mEvent.teP++;

            // duration
            mEvent.teQ++;
            mEvent.teR++;
            mEvent.teS++;

            nl::LogEvent(&mEvent);
            break;
        case 4:
        default:
            // null
            mEvent.SetTeJNull();
            mEvent.SetTeMNull();
            mEvent.SetTeNNull();
            mEvent.SetTePNull();
            mEvent.SetTeSNull();

            nl::LogEvent(&mEvent);

            mNullableEvent.neJ.SetNeANull();

            nl::LogEvent(&mNullableEvent);

            mEvent.SetTeJPresent();
            mEvent.SetTeMPresent();
            mEvent.SetTeNPresent();
            mEvent.SetTePPresent();
            mEvent.SetTeSPresent();

            mNullableEvent.neJ.SetNeAPresent();
            mNullableEvent.SetNeJNull();

            nl::LogEvent(&mNullableEvent);
            break;
    }

    mState = (mState + 1) % mNumStates;
}

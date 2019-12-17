
/*
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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

/*
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.c.h
 *    SOURCE PROTO: weave/trait/security/bolt_lock_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__BOLT_LOCK_TRAIT_C_H_
#define _WEAVE_TRAIT_SECURITY__BOLT_LOCK_TRAIT_C_H_



    //
    // Commands
    //

    typedef enum
    {
      kBoltLockChangeRequestId = 0x1,
    } schema_weave_security_bolt_lock_trait_command_id_t;


    // BoltLockChangeRequest Parameters
    typedef enum
    {
        kBoltLockChangeRequestParameter_State = 1,
        kBoltLockChangeRequestParameter_BoltLockActor = 4,
    } schema_weave_security_bolt_lock_trait_bolt_lock_change_request_param_t;



    //
    // Enums
    //

    // BoltState
    typedef enum
    {
    BOLT_STATE_RETRACTED = 1,
    BOLT_STATE_EXTENDED = 2,
    } schema_weave_security_bolt_lock_trait_bolt_state_t;
    // BoltLockActorMethod
    typedef enum
    {
    BOLT_LOCK_ACTOR_METHOD_OTHER = 1,
    BOLT_LOCK_ACTOR_METHOD_PHYSICAL = 2,
    BOLT_LOCK_ACTOR_METHOD_KEYPAD_PIN = 3,
    BOLT_LOCK_ACTOR_METHOD_LOCAL_IMPLICIT = 4,
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_EXPLICIT = 5,
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_IMPLICIT = 6,
    BOLT_LOCK_ACTOR_METHOD_REMOTE_USER_OTHER = 7,
    BOLT_LOCK_ACTOR_METHOD_REMOTE_DELEGATE = 8,
    BOLT_LOCK_ACTOR_METHOD_LOW_POWER_SHUTDOWN = 9,
    BOLT_LOCK_ACTOR_METHOD_VOICE_ASSISTANT = 10,
    } schema_weave_security_bolt_lock_trait_bolt_lock_actor_method_t;
    // BoltActuatorState
    typedef enum
    {
    BOLT_ACTUATOR_STATE_OK = 1,
    BOLT_ACTUATOR_STATE_LOCKING = 2,
    BOLT_ACTUATOR_STATE_UNLOCKING = 3,
    BOLT_ACTUATOR_STATE_MOVING = 4,
    BOLT_ACTUATOR_STATE_JAMMED_LOCKING = 5,
    BOLT_ACTUATOR_STATE_JAMMED_UNLOCKING = 6,
    BOLT_ACTUATOR_STATE_JAMMED_OTHER = 7,
    } schema_weave_security_bolt_lock_trait_bolt_actuator_state_t;
    // BoltLockedState
    typedef enum
    {
    BOLT_LOCKED_STATE_UNLOCKED = 1,
    BOLT_LOCKED_STATE_LOCKED = 2,
    BOLT_LOCKED_STATE_UNKNOWN = 3,
    } schema_weave_security_bolt_lock_trait_bolt_locked_state_t;




#endif // _WEAVE_TRAIT_SECURITY__BOLT_LOCK_TRAIT_C_H_

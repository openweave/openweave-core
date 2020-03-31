
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
 *    SOURCE PROTO: weave/trait/security/security_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_C_H_
#define _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_C_H_




    //
    // Enums
    //

    // ArmState
    typedef enum
    {
    ARM_STATE_DISARMED = 1,
    ARM_STATE_ARMING = 2,
    ARM_STATE_ARMED = 3,
    } schema_weave_security_security_trait_arm_state_t;
    // ArmMode
    typedef enum
    {
    ARM_MODE_DISARMED = 1,
    ARM_MODE_PERIMETER = 2,
    ARM_MODE_PERIMETER_AND_MOTION = 3,
    } schema_weave_security_security_trait_arm_mode_t;
    // AlarmState
    typedef enum
    {
    ALARM_STATE_NOT_ALARMING = 1,
    ALARM_STATE_PREALARMING = 2,
    ALARM_STATE_ALARMING = 3,
    } schema_weave_security_security_trait_alarm_state_t;




#endif // _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_C_H_

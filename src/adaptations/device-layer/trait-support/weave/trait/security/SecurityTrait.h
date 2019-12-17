
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
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/security/security_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace SecurityTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe06U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  arm_mode                            ArmMode                              int               NO              NO
    //
    kPropertyHandle_ArmMode = 2,

    //
    //  arm_state                           ArmState                             int               NO              NO
    //
    kPropertyHandle_ArmState = 3,

    //
    //  alarm_state                         AlarmState                           int               NO              NO
    //
    kPropertyHandle_AlarmState = 4,

    //
    //  expected_arm_time                   google.protobuf.Timestamp            uint              NO              NO
    //
    kPropertyHandle_ExpectedArmTime = 5,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 5,
};

//
// Enums
//

enum ArmState {
    ARM_STATE_DISARMED = 1,
    ARM_STATE_ARMING = 2,
    ARM_STATE_ARMED = 3,
};

enum ArmMode {
    ARM_MODE_DISARMED = 1,
    ARM_MODE_PERIMETER = 2,
    ARM_MODE_PERIMETER_AND_MOTION = 3,
};

enum AlarmState {
    ALARM_STATE_NOT_ALARMING = 1,
    ALARM_STATE_PREALARMING = 2,
    ALARM_STATE_ALARMING = 3,
};

} // namespace SecurityTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__SECURITY_TRAIT_H_

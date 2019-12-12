
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
 *    SOURCE PROTO: weave/trait/heartbeat/low_power_heartbeat_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_HEARTBEAT__LOW_POWER_HEARTBEAT_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_HEARTBEAT__LOW_POWER_HEARTBEAT_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Heartbeat {
namespace LowPowerHeartbeatSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x13U
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
    //  destination                         uint64                               uint64            NO              YES
    //
    kPropertyHandle_Destination = 2,

    //
    //  window                              google.protobuf.Duration             uint32 millisecondsNO              NO
    //
    kPropertyHandle_Window = 3,

    //
    //  offset                              google.protobuf.Duration             uint32 millisecondsNO              NO
    //
    kPropertyHandle_Offset = 4,

    //
    //  interval                            google.protobuf.Duration             uint32 millisecondsNO              NO
    //
    kPropertyHandle_Interval = 5,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 5,
};

} // namespace LowPowerHeartbeatSettingsTrait
} // namespace Heartbeat
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_HEARTBEAT__LOW_POWER_HEARTBEAT_SETTINGS_TRAIT_H_

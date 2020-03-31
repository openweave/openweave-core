
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
 *    SOURCE PROTO: weave/trait/power/power_source_capabilities_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_POWER__POWER_SOURCE_CAPABILITIES_TRAIT_H_
#define _WEAVE_TRAIT_POWER__POWER_SOURCE_CAPABILITIES_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Power {
namespace PowerSourceCapabilitiesTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x18U
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
    //  type                                PowerSourceType                      int               NO              NO
    //
    kPropertyHandle_Type = 2,

    //
    //  description                         weave.common.StringRef               union             YES             YES
    //
    kPropertyHandle_Description = 3,

    //
    //  nominal_voltage                     float                                uint32            NO              NO
    //
    kPropertyHandle_NominalVoltage = 4,

    //
    //  maximum_current                     float                                uint32            YES             YES
    //
    kPropertyHandle_MaximumCurrent = 5,

    //
    //  current_type                        PowerSourceCurrentType               int               NO              NO
    //
    kPropertyHandle_CurrentType = 6,

    //
    //  order                               uint32                               uint32            NO              NO
    //
    kPropertyHandle_Order = 7,

    //
    //  removable                           bool                                 bool              NO              NO
    //
    kPropertyHandle_Removable = 8,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 8,
};

//
// Enums
//

enum PowerSourceType {
    POWER_SOURCE_TYPE_BATTERY = 1,
};

enum PowerSourceCurrentType {
    POWER_SOURCE_CURRENT_TYPE_DC = 1,
    POWER_SOURCE_CURRENT_TYPE_AC = 2,
};

} // namespace PowerSourceCapabilitiesTrait
} // namespace Power
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_POWER__POWER_SOURCE_CAPABILITIES_TRAIT_H_

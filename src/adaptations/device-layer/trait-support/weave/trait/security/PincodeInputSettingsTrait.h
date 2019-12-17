
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
 *    SOURCE PROTO: weave/trait/security/pincode_input_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace PincodeInputSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe0bU
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
    //  wrong_entry_code_limit              uint32                               uint32            NO              NO
    //
    kPropertyHandle_WrongEntryCodeLimit = 2,

    //
    //  wrong_entry_disable_time            google.protobuf.Duration             uint32 seconds    NO              NO
    //
    kPropertyHandle_WrongEntryDisableTime = 3,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 3,
};

} // namespace PincodeInputSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_SETTINGS_TRAIT_H_

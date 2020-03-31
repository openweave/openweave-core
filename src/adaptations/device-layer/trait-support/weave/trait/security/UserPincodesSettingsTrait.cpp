
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
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: weave/trait/security/user_pincodes_settings_trait.proto
 *
 */

#include <weave/trait/security/UserPincodesSettingsTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserPincodesSettingsTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // user_pincodes
    { kPropertyHandle_UserPincodes, 0 }, // value
    { kPropertyHandle_UserPincodes_Value, 1 }, // user_id
    { kPropertyHandle_UserPincodes_Value, 2 }, // pincode
    { kPropertyHandle_UserPincodes_Value, 3 }, // pincode_credential_enabled
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x1
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x10
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        3,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        IsDictionaryTypeHandleBitfield,
        NULL,
        NULL,
        &IsNullableHandleBitfield[0],
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};

//
// Event Structs
//

const nl::FieldDescriptor UserPincodeFieldDescriptors[] =
{
    {
        NULL, offsetof(UserPincode, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(UserPincode, pincode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 2
    },

    {
        NULL, offsetof(UserPincode, pincodeCredentialEnabled), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 3
    },

};

const nl::SchemaFieldDescriptor UserPincode::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(UserPincodeFieldDescriptors)/sizeof(UserPincodeFieldDescriptors[0]),
    .mFields = UserPincodeFieldDescriptors,
    .mSize = sizeof(UserPincode)
};

} // namespace UserPincodesSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema

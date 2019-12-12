
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_settings_trait.proto
 *
 */

#include <weave/trait/security/UserNFCTokenSettingsTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokenSettingsTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // user_nfc_tokens
    { kPropertyHandle_UserNfcTokens, 0 }, // value
    { kPropertyHandle_UserNfcTokens_Value, 1 }, // user_id
    { kPropertyHandle_UserNfcTokens_Value, 2 }, // token_device_id
    { kPropertyHandle_UserNfcTokens_Value, 3 }, // public_key
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x1
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
        NULL,
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

const nl::FieldDescriptor UserNFCTokenFieldDescriptors[] =
{
    {
        NULL, offsetof(UserNFCToken, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(UserNFCToken, tokenDeviceId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 2
    },

    {
        NULL, offsetof(UserNFCToken, publicKey), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 3
    },

};

const nl::SchemaFieldDescriptor UserNFCToken::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(UserNFCTokenFieldDescriptors)/sizeof(UserNFCTokenFieldDescriptors[0]),
    .mFields = UserNFCTokenFieldDescriptors,
    .mSize = sizeof(UserNFCToken)
};

} // namespace UserNFCTokenSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema

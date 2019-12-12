
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_tokens_trait.proto
 *
 */

#include <weave/trait/security/UserNFCTokensTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokensTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // user_nfc_tokens
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        NULL,
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

const nl::FieldDescriptor UserNFCTokenDataFieldDescriptors[] =
{
    {
        NULL, offsetof(UserNFCTokenData, userId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },

    {
        NULL, offsetof(UserNFCTokenData, tokenDeviceId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 2
    },

    {
        NULL, offsetof(UserNFCTokenData, enabled), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 3
    },

    {
        NULL, offsetof(UserNFCTokenData, structureIds) + offsetof(nl::SerializedFieldTypeUInt64_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 4
    },
    {
        NULL, offsetof(UserNFCTokenData, structureIds) + offsetof(nl::SerializedFieldTypeUInt64_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 4
    },

    {
        NULL, offsetof(UserNFCTokenData, label), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 5
    },

    {
        &Schema::Weave::Trait::Security::UserNFCTokenMetadataTrait::Metadata::FieldSchema, offsetof(UserNFCTokenData, metadata), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 6
    },

};

const nl::SchemaFieldDescriptor UserNFCTokenData::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(UserNFCTokenDataFieldDescriptors)/sizeof(UserNFCTokenDataFieldDescriptors[0]),
    .mFields = UserNFCTokenDataFieldDescriptors,
    .mSize = sizeof(UserNFCTokenData)
};

} // namespace UserNFCTokensTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema

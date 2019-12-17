
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
 *    SOURCE PROTO: weave/trait/auth/application_keys_trait.proto
 *
 */

#include <weave/trait/auth/ApplicationKeysTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Auth {
namespace ApplicationKeysTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // epoch_keys
    { kPropertyHandle_Root, 2 }, // master_keys
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

const nl::FieldDescriptor EpochKeyFieldDescriptors[] =
{
    {
        NULL, offsetof(EpochKey, keyId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(EpochKey, startTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 2
    },

    {
        NULL, offsetof(EpochKey, key), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 3
    },

};

const nl::SchemaFieldDescriptor EpochKey::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(EpochKeyFieldDescriptors)/sizeof(EpochKeyFieldDescriptors[0]),
    .mFields = EpochKeyFieldDescriptors,
    .mSize = sizeof(EpochKey)
};


const nl::FieldDescriptor ApplicationMasterKeyFieldDescriptors[] =
{
    {
        NULL, offsetof(ApplicationMasterKey, applicationGroupGlobalId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

    {
        NULL, offsetof(ApplicationMasterKey, applicationGroupShortId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        NULL, offsetof(ApplicationMasterKey, key), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 3
    },

};

const nl::SchemaFieldDescriptor ApplicationMasterKey::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(ApplicationMasterKeyFieldDescriptors)/sizeof(ApplicationMasterKeyFieldDescriptors[0]),
    .mFields = ApplicationMasterKeyFieldDescriptors,
    .mSize = sizeof(ApplicationMasterKey)
};

} // namespace ApplicationKeysTrait
} // namespace Auth
} // namespace Trait
} // namespace Weave
} // namespace Schema


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
 *    SOURCE PROTO: weave/trait/synchronization/synchronization_trait.proto
 *
 */

#include <weave/trait/synchronization/SynchronizationTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Synchronization {
namespace SynchronizationTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // trait_id
    { kPropertyHandle_Root, 2 }, // num_devices
    { kPropertyHandle_Root, 3 }, // num_unsynced_devices
    { kPropertyHandle_Root, 4 }, // current_revision
    { kPropertyHandle_Root, 5 }, // resource_sync_status
    { kPropertyHandle_Root, 6 }, // last_sync_time
    { kPropertyHandle_ResourceSyncStatus, 0 }, // value
    { kPropertyHandle_ResourceSyncStatus_Value, 1 }, // sync_status
    { kPropertyHandle_ResourceSyncStatus_Value, 2 }, // last_synced_time
    { kPropertyHandle_ResourceSyncStatus_Value, 3 }, // current_revision
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x10, 0x0
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

const nl::FieldDescriptor SynchronizationEntryFieldDescriptors[] =
{
    {
        NULL, offsetof(SynchronizationEntry, syncStatus), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(SynchronizationEntry, lastSyncedTime), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 2
    },

    {
        NULL, offsetof(SynchronizationEntry, currentRevision), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 3
    },

};

const nl::SchemaFieldDescriptor SynchronizationEntry::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(SynchronizationEntryFieldDescriptors)/sizeof(SynchronizationEntryFieldDescriptors[0]),
    .mFields = SynchronizationEntryFieldDescriptors,
    .mSize = sizeof(SynchronizationEntry)
};

} // namespace SynchronizationTrait
} // namespace Synchronization
} // namespace Trait
} // namespace Weave
} // namespace Schema

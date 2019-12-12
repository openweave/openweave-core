
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
 *    SOURCE PROTO: weave/trait/synchronization/synchronization_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_H_
#define _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Synchronization {
namespace SynchronizationTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x1201U
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
    //  trait_id                            repeated uint32                      array             NO              NO
    //
    kPropertyHandle_TraitId = 2,

    //
    //  num_devices                         uint32                               uint32            NO              NO
    //
    kPropertyHandle_NumDevices = 3,

    //
    //  num_unsynced_devices                uint32                               uint32            NO              NO
    //
    kPropertyHandle_NumUnsyncedDevices = 4,

    //
    //  current_revision                    uint32                               uint32            NO              NO
    //
    kPropertyHandle_CurrentRevision = 5,

    //
    //  resource_sync_status                map <string,SynchronizationEntry>    map <string, structure>NO              NO
    //
    kPropertyHandle_ResourceSyncStatus = 6,

    //
    //  last_sync_time                      google.protobuf.Timestamp            uint              NO              NO
    //
    kPropertyHandle_LastSyncTime = 7,

    //
    //  value                               SynchronizationEntry                 structure         NO              NO
    //
    kPropertyHandle_ResourceSyncStatus_Value = 8,

    //
    //  sync_status                         SyncronizationStatus                 int               NO              NO
    //
    kPropertyHandle_ResourceSyncStatus_Value_SyncStatus = 9,

    //
    //  last_synced_time                    google.protobuf.Timestamp            uint              NO              NO
    //
    kPropertyHandle_ResourceSyncStatus_Value_LastSyncedTime = 10,

    //
    //  current_revision                    uint32                               uint32            NO              NO
    //
    kPropertyHandle_ResourceSyncStatus_Value_CurrentRevision = 11,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 11,
};

//
// Event Structs
//

struct SynchronizationEntry
{
    int32_t syncStatus;
    int64_t lastSyncedTime;
    uint32_t currentRevision;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct SynchronizationEntry_array {
    uint32_t num;
    SynchronizationEntry *buf;
};

//
// Enums
//

enum SyncronizationStatus {
    SYNCRONIZATION_STATUS_SYNCHRONIZED = 1,
    SYNCRONIZATION_STATUS_PENDING = 2,
    SYNCRONIZATION_STATUS_TIMEOUT = 3,
    SYNCRONIZATION_STATUS_FAILED_RETRY = 4,
    SYNCRONIZATION_STATUS_FAILED_FATAL = 5,
};

} // namespace SynchronizationTrait
} // namespace Synchronization
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_H_

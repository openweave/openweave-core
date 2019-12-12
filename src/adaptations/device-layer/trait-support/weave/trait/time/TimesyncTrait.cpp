
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
 *    SOURCE PROTO: weave/trait/time/timesync_trait.proto
 *
 */

#include <weave/trait/time/TimesyncTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Time {
namespace TimesyncTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
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
    // Events
    //

const nl::FieldDescriptor TimeSyncEventFieldDescriptors[] =
{
    {
        NULL, offsetof(TimeSyncEvent, offset), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt64, 0), 1
    },

    {
        NULL, offsetof(TimeSyncEvent, serverId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 1), 2
    },

    {
        NULL, offsetof(TimeSyncEvent, serverAddress), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

    {
        NULL, offsetof(TimeSyncEvent, slew), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 4
    },

};

const nl::SchemaFieldDescriptor TimeSyncEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(TimeSyncEventFieldDescriptors)/sizeof(TimeSyncEventFieldDescriptors[0]),
    .mFields = TimeSyncEventFieldDescriptors,
    .mSize = sizeof(TimeSyncEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema TimeSyncEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TimesyncTrait
} // namespace Time
} // namespace Trait
} // namespace Weave
} // namespace Schema

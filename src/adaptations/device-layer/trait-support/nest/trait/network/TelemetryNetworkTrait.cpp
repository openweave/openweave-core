
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/network/telemetry_network_trait.proto
 *
 */

#include <nest/trait/network/TelemetryNetworkTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Network {
namespace TelemetryNetworkTrait {

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

const nl::FieldDescriptor NetworkDHCPFailureEventFieldDescriptors[] =
{
    {
        NULL, offsetof(NetworkDHCPFailureEvent, reason), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },

};

const nl::SchemaFieldDescriptor NetworkDHCPFailureEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(NetworkDHCPFailureEventFieldDescriptors)/sizeof(NetworkDHCPFailureEventFieldDescriptors[0]),
    .mFields = NetworkDHCPFailureEventFieldDescriptors,
    .mSize = sizeof(NetworkDHCPFailureEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema NetworkDHCPFailureEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Debug,
    .mDataSchemaVersion = 1,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace TelemetryNetworkTrait
} // namespace Network
} // namespace Trait
} // namespace Nest
} // namespace Schema

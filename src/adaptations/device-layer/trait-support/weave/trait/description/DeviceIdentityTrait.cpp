
/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: weave/trait/description/device_identity_trait.proto
 *
 */

#include <weave/trait/description/DeviceIdentityTrait.h>

namespace Schema {
namespace Weave {
namespace Trait {
namespace Description {
namespace DeviceIdentityTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // vendor_id
    { kPropertyHandle_Root, 2 }, // vendor_id_description
    { kPropertyHandle_Root, 3 }, // vendor_product_id
    { kPropertyHandle_Root, 4 }, // product_id_description
    { kPropertyHandle_Root, 5 }, // product_revision
    { kPropertyHandle_Root, 6 }, // serial_number
    { kPropertyHandle_Root, 7 }, // software_version
    { kPropertyHandle_Root, 8 }, // manufacturing_date
    { kPropertyHandle_Root, 9 }, // device_id
    { kPropertyHandle_Root, 10 }, // fabric_id
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x8a, 0x3
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x8a, 0x0
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
        &IsOptionalHandleBitfield[0],
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

} // namespace DeviceIdentityTrait
} // namespace Description
} // namespace Trait
} // namespace Weave
} // namespace Schema

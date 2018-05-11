
/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/description/device_identity_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_DESCRIPTION__DEVICE_IDENTITY_TRAIT_H_
#define _WEAVE_TRAIT_DESCRIPTION__DEVICE_IDENTITY_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Description {
namespace DeviceIdentityTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0x17U
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
    //  vendor_id                           uint32                               uint16            NO              NO
    //
    kPropertyHandle_VendorId = 2,

    //
    //  vendor_id_description               weave.common.StringRef               union             YES             YES
    //
    kPropertyHandle_VendorIdDescription = 3,

    //
    //  vendor_product_id                   uint32                               uint16            NO              NO
    //
    kPropertyHandle_VendorProductId = 4,

    //
    //  product_id_description              weave.common.StringRef               union             YES             YES
    //
    kPropertyHandle_ProductIdDescription = 5,

    //
    //  product_revision                    uint32                               uint16            NO              NO
    //
    kPropertyHandle_ProductRevision = 6,

    //
    //  serial_number                       string                               string            NO              NO
    //
    kPropertyHandle_SerialNumber = 7,

    //
    //  software_version                    string                               string            NO              NO
    //
    kPropertyHandle_SoftwareVersion = 8,

    //
    //  manufacturing_date                  string                               string            YES             YES
    //
    kPropertyHandle_ManufacturingDate = 9,

    //
    //  device_id                           uint64                               uint64            YES             NO
    //
    kPropertyHandle_DeviceId = 10,

    //
    //  fabric_id                           uint64                               uint64            YES             NO
    //
    kPropertyHandle_FabricId = 11,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 11,
};

} // namespace DeviceIdentityTrait
} // namespace Description
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_DESCRIPTION__DEVICE_IDENTITY_TRAIT_H_

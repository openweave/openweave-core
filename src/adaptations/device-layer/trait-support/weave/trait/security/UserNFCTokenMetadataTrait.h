
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_metadata_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_METADATA_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_METADATA_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokenMetadataTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe12U
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
    //  metadata                            Metadata                             structure         NO              NO
    //
    kPropertyHandle_Metadata = 2,

    //
    //  serial_number                       string                               string            NO              NO
    //
    kPropertyHandle_Metadata_SerialNumber = 3,

    //
    //  tag_number                          string                               string            NO              NO
    //
    kPropertyHandle_Metadata_TagNumber = 4,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 4,
};

//
// Event Structs
//

struct Metadata
{
    const char * serialNumber;
    const char * tagNumber;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct Metadata_array {
    uint32_t num;
    Metadata *buf;
};

} // namespace UserNFCTokenMetadataTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_METADATA_TRAIT_H_


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
 *    SOURCE PROTO: weave/trait/security/user_nfc_tokens_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKENS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKENS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/trait/security/UserNFCTokenMetadataTrait.h>


namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokensTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe11U
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
    //  user_nfc_tokens                     repeated UserNFCTokenData            array             NO              NO
    //
    kPropertyHandle_UserNfcTokens = 2,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 2,
};

//
// Event Structs
//

struct UserNFCTokenData
{
    nl::SerializedByteString userId;
    uint64_t tokenDeviceId;
    bool enabled;
    nl::SerializedFieldTypeUInt64_array  structureIds;
    const char * label;
    Schema::Weave::Trait::Security::UserNFCTokenMetadataTrait::Metadata metadata;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct UserNFCTokenData_array {
    uint32_t num;
    UserNFCTokenData *buf;
};

} // namespace UserNFCTokensTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKENS_TRAIT_H_

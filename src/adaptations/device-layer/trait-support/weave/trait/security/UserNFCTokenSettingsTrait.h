
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokenSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe0fU
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
    //  user_nfc_tokens                     map <uint32,UserNFCToken>            map <uint16, structure>NO              NO
    //
    kPropertyHandle_UserNfcTokens = 2,

    //
    //  value                               UserNFCToken                         structure         NO              NO
    //
    kPropertyHandle_UserNfcTokens_Value = 3,

    //
    //  user_id                             weave.common.ResourceId              bytes             NO              NO
    //
    kPropertyHandle_UserNfcTokens_Value_UserId = 4,

    //
    //  token_device_id                     weave.common.ResourceId              uint64            NO              NO
    //
    kPropertyHandle_UserNfcTokens_Value_TokenDeviceId = 5,

    //
    //  public_key                          bytes                                bytes             NO              NO
    //
    kPropertyHandle_UserNfcTokens_Value_PublicKey = 6,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 6,
};

//
// Event Structs
//

struct UserNFCToken
{
    nl::SerializedByteString userId;
    uint64_t tokenDeviceId;
    nl::SerializedByteString publicKey;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct UserNFCToken_array {
    uint32_t num;
    UserNFCToken *buf;
};

} // namespace UserNFCTokenSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_SETTINGS_TRAIT_H_


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
 *    SOURCE PROTO: weave/trait/security/user_pincodes_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_PINCODES_SETTINGS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_PINCODES_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserPincodesSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe01U
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
    //  user_pincodes                       map <uint32,UserPincode>             map <uint16, structure>NO              NO
    //
    kPropertyHandle_UserPincodes = 2,

    //
    //  value                               UserPincode                          structure         NO              NO
    //
    kPropertyHandle_UserPincodes_Value = 3,

    //
    //  user_id                             weave.common.ResourceId              bytes             NO              NO
    //
    kPropertyHandle_UserPincodes_Value_UserId = 4,

    //
    //  pincode                             bytes                                bytes             NO              NO
    //
    kPropertyHandle_UserPincodes_Value_Pincode = 5,

    //
    //  pincode_credential_enabled          bool                                 bool              NO              YES
    //
    kPropertyHandle_UserPincodes_Value_PincodeCredentialEnabled = 6,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 6,
};

//
// Event Structs
//

struct UserPincode
{
    nl::SerializedByteString userId;
    nl::SerializedByteString pincode;
    bool pincodeCredentialEnabled;
    void SetPincodeCredentialEnabledNull(void);
    void SetPincodeCredentialEnabledPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsPincodeCredentialEnabledPresent(void);
#endif
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct UserPincode_array {
    uint32_t num;
    UserPincode *buf;
};

inline void UserPincode::SetPincodeCredentialEnabledNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void UserPincode::SetPincodeCredentialEnabledPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool UserPincode::IsPincodeCredentialEnabledPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
//
// Commands
//

enum {
    kSetUserPincodeRequestId = 0x1,
    kGetUserPincodeRequestId = 0x2,
    kDeleteUserPincodeRequestId = 0x3,
};

enum SetUserPincodeRequestParameters {
    kSetUserPincodeRequestParameter_UserPincode = 1,
};

enum GetUserPincodeRequestParameters {
    kGetUserPincodeRequestParameter_UserId = 1,
};

enum DeleteUserPincodeRequestParameters {
    kDeleteUserPincodeRequestParameter_UserId = 1,
};

enum SetUserPincodeResponseParameters {
    kSetUserPincodeResponseParameter_Status = 1,
};

enum GetUserPincodeResponseParameters {
    kGetUserPincodeResponseParameter_UserPincode = 1,
};

enum DeleteUserPincodeResponseParameters {
    kDeleteUserPincodeResponseParameter_Status = 1,
};

//
// Enums
//

enum PincodeErrorCodes {
    PINCODE_ERROR_CODES_DUPLICATE_PINCODE = 1,
    PINCODE_ERROR_CODES_TOO_MANY_PINCODES = 2,
    PINCODE_ERROR_CODES_INVALID_PINCODE = 3,
    PINCODE_ERROR_CODES_SUCCESS_PINCODE_DELETED = 4,
    PINCODE_ERROR_CODES_SUCCESS_PINCODE_STATUS = 5,
    PINCODE_ERROR_CODES_DUPLICATE_NONCE = 6,
    PINCODE_ERROR_CODES_EXCEEDED_RATE_LIMIT = 7,
};

} // namespace UserPincodesSettingsTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_PINCODES_SETTINGS_TRAIT_H_

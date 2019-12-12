
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_access_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_ACCESS_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_ACCESS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokenAccessTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe13U
};

//
// Events
//
struct UserNFCTokenAccessEvent
{
    int32_t result;
    uint64_t tokenId;
    nl::SerializedByteString userId;
    void SetUserIdNull(void);
    void SetUserIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsUserIdPresent(void);
#endif
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xe13U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct UserNFCTokenAccessEvent_array {
    uint32_t num;
    UserNFCTokenAccessEvent *buf;
};

inline void UserNFCTokenAccessEvent::SetUserIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void UserNFCTokenAccessEvent::SetUserIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool UserNFCTokenAccessEvent::IsUserIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif

//
// Enums
//

enum UserNFCTokenAccessResult {
    USER_NFC_TOKEN_ACCESS_RESULT_SUCCESS = 1,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_UNKNOWN_TOKEN = 2,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_INVALID_TOKEN = 3,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_OUT_OF_SCHEDULE = 4,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_TOKEN_DISABLED = 5,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_INVALID_VERSION = 6,
    USER_NFC_TOKEN_ACCESS_RESULT_FAILURE_OTHER_REASON = 7,
};

} // namespace UserNFCTokenAccessTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_ACCESS_TRAIT_H_

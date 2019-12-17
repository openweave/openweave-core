
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
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_management_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/trait/security/UserNFCTokenMetadataTrait.h>
#include <weave/trait/security/UserNFCTokensTrait.h>


namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace UserNFCTokenManagementTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe10U
};

//
// Events
//
struct UserNFCTokenManagementEvent
{
    int32_t nfcTokenManagementEvent;
    Schema::Weave::Trait::Security::UserNFCTokensTrait::UserNFCTokenData userNfcToken;
    uint64_t initiatingUserId;
    nl::SerializedByteString previousUserId;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xe10U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct UserNFCTokenManagementEvent_array {
    uint32_t num;
    UserNFCTokenManagementEvent *buf;
};


//
// Commands
//

enum {
    kTransferUserNFCTokenRequestId = 0x1,
    kSetUserNFCTokenEnableStateRequestId = 0x2,
    kAuthUserNFCTokenToStructureRequestId = 0x3,
};

enum TransferUserNFCTokenRequestParameters {
    kTransferUserNFCTokenRequestParameter_TargetUserId = 1,
    kTransferUserNFCTokenRequestParameter_TokenDeviceId = 2,
};

enum SetUserNFCTokenEnableStateRequestParameters {
    kSetUserNFCTokenEnableStateRequestParameter_TokenDeviceId = 1,
    kSetUserNFCTokenEnableStateRequestParameter_Enabled = 2,
};

enum AuthUserNFCTokenToStructureRequestParameters {
    kAuthUserNFCTokenToStructureRequestParameter_TokenDeviceId = 1,
    kAuthUserNFCTokenToStructureRequestParameter_Authorized = 2,
    kAuthUserNFCTokenToStructureRequestParameter_StructureId = 3,
};

//
// Enums
//

enum NFCTokenEvent {
    NFC_TOKEN_EVENT_PAIRED = 1,
    NFC_TOKEN_EVENT_UNPAIRED = 2,
    NFC_TOKEN_EVENT_STRUCTURE_AUTH = 3,
    NFC_TOKEN_EVENT_STRUCTURE_UNAUTH = 4,
    NFC_TOKEN_EVENT_TRANSFERRED = 5,
    NFC_TOKEN_EVENT_DISABLED = 6,
    NFC_TOKEN_EVENT_ENABLED = 7,
};

} // namespace UserNFCTokenManagementTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_H_


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
 *    SOURCE TEMPLATE: trait.c.h
 *    SOURCE PROTO: weave/trait/security/user_nfc_token_management_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_C_H_
#define _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_C_H_



    //
    // Commands
    //

    typedef enum
    {
      kTransferUserNFCTokenRequestId = 0x1,
      kSetUserNFCTokenEnableStateRequestId = 0x2,
      kAuthUserNFCTokenToStructureRequestId = 0x3,
    } schema_weave_security_user_nfc_token_management_trait_command_id_t;


    // TransferUserNFCTokenRequest Parameters
    typedef enum
    {
        kTransferUserNFCTokenRequestParameter_TargetUserId = 1,
        kTransferUserNFCTokenRequestParameter_TokenDeviceId = 2,
    } schema_weave_security_user_nfc_token_management_trait_transfer_user_nfc_token_request_param_t;
    // SetUserNFCTokenEnableStateRequest Parameters
    typedef enum
    {
        kSetUserNFCTokenEnableStateRequestParameter_TokenDeviceId = 1,
        kSetUserNFCTokenEnableStateRequestParameter_Enabled = 2,
    } schema_weave_security_user_nfc_token_management_trait_set_user_nfc_token_enable_state_request_param_t;
    // AuthUserNFCTokenToStructureRequest Parameters
    typedef enum
    {
        kAuthUserNFCTokenToStructureRequestParameter_TokenDeviceId = 1,
        kAuthUserNFCTokenToStructureRequestParameter_Authorized = 2,
        kAuthUserNFCTokenToStructureRequestParameter_StructureId = 3,
    } schema_weave_security_user_nfc_token_management_trait_auth_user_nfc_token_to_structure_request_param_t;



    //
    // Enums
    //

    // NFCTokenEvent
    typedef enum
    {
    NFC_TOKEN_EVENT_PAIRED = 1,
    NFC_TOKEN_EVENT_UNPAIRED = 2,
    NFC_TOKEN_EVENT_STRUCTURE_AUTH = 3,
    NFC_TOKEN_EVENT_STRUCTURE_UNAUTH = 4,
    NFC_TOKEN_EVENT_TRANSFERRED = 5,
    NFC_TOKEN_EVENT_DISABLED = 6,
    NFC_TOKEN_EVENT_ENABLED = 7,
    } schema_weave_security_user_nfc_token_management_trait_nfc_token_event_t;




#endif // _WEAVE_TRAIT_SECURITY__USER_NFC_TOKEN_MANAGEMENT_TRAIT_C_H_

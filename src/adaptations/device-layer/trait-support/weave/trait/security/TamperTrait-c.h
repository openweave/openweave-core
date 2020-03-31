
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
 *    SOURCE PROTO: weave/trait/security/tamper_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_C_H_
#define _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_C_H_



    //
    // Commands
    //

    typedef enum
    {
      kResetTamperRequestId = 0x1,
    } schema_weave_security_tamper_trait_command_id_t;





    //
    // Enums
    //

    // TamperState
    typedef enum
    {
    TAMPER_STATE_CLEAR = 1,
    TAMPER_STATE_TAMPERED = 2,
    TAMPER_STATE_UNKNOWN = 3,
    } schema_weave_security_tamper_trait_tamper_state_t;




#endif // _WEAVE_TRAIT_SECURITY__TAMPER_TRAIT_C_H_

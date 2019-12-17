
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
 *    SOURCE PROTO: weave/trait/security/pincode_input_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_C_H_
#define _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_C_H_




    //
    // Enums
    //

    // PincodeEntryResult
    typedef enum
    {
    PINCODE_ENTRY_RESULT_FAILURE_INVALID_PINCODE = 1,
    PINCODE_ENTRY_RESULT_FAILURE_OUT_OF_SCHEDULE = 2,
    PINCODE_ENTRY_RESULT_FAILURE_PINCODE_DISABLED = 3,
    PINCODE_ENTRY_RESULT_SUCCESS = 4,
    } schema_weave_security_pincode_input_trait_pincode_entry_result_t;
    // PincodeInputState
    typedef enum
    {
    PINCODE_INPUT_STATE_ENABLED = 1,
    PINCODE_INPUT_STATE_DISABLED = 2,
    } schema_weave_security_pincode_input_trait_pincode_input_state_t;




#endif // _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_C_H_

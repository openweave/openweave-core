
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
 *    SOURCE PROTO: nest/test/trait/test_a_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_A_TRAIT_C_H_
#define _NEST_TEST_TRAIT__TEST_A_TRAIT_C_H_



    //
    // Commands
    //

    typedef enum
    {
      kCommandARequestId = 0x1,
      kCommandBRequestId = 0x2,
    } schema_nest_test_test_a_trait_command_id_t;


    // CommandARequest Parameters
    typedef enum
    {
        kCommandARequestParameter_A = 1,
        kCommandARequestParameter_B = 2,
    } schema_nest_test_test_a_trait_command_a_request_param_t;
    // CommandBRequest Parameters
    typedef enum
    {
        kCommandBRequestParameter_A = 1,
        kCommandBRequestParameter_B = 2,
    } schema_nest_test_test_a_trait_command_b_request_param_t;


    // CommandBResponse Parameters
    typedef enum
    {
        kCommandBResponseParameter_A = 1,
        kCommandBResponseParameter_B = 2,
    } schema_nest_test_test_a_trait_command_b_response_param_t;

    //
    // Enums
    //

    // EnumA
    typedef enum
    {
    ENUM_A_VALUE_1 = 1,
    ENUM_A_VALUE_2 = 2,
    ENUM_A_VALUE_3 = 3,
    } schema_nest_test_test_a_trait_enum_a_t;
    // EnumAA
    typedef enum
    {
    ENUM_AA_VALUE_1 = 1,
    ENUM_AA_VALUE_2 = 2,
    ENUM_AA_VALUE_3 = 3,
    } schema_nest_test_test_a_trait_enum_aa_t;


    //
    // Constants
    //

    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_1 {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_2 {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_3 {0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_4 {0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_5 {0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_6 {0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_7 {0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_8 {0x00, 0x08, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}

    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_1_IMP (0x0000000000000000ULL) // DEVICE_00000000
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_2_IMP (0x0000000000000001ULL) // USER_00000001
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_3_IMP (0x0000000000000002ULL) // ACCOUNT_00000002
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_4_IMP (0x0000000000000003ULL) // AREA_00000003
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_5_IMP (0x0000000000000004ULL) // FIXTURE_00000004
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_6_IMP (0x0000000000000005ULL) // GROUP_00000005
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_7_IMP (0x0000000000000006ULL) // ANNOTATION_00000006
    #define SCHEMA_NEST_TEST_TEST_A_TRAIT_CONSTANT_A_VALUE_8_IMP (0x1122334455667788ULL) // STRUCTURE_1122334455667788



#endif // _NEST_TEST_TRAIT__TEST_A_TRAIT_C_H_

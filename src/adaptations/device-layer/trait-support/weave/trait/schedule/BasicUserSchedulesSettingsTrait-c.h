
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
 *    SOURCE PROTO: weave/trait/schedule/basic_user_schedules_settings_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_C_H_
#define _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_C_H_



    //
    // Commands
    //

    typedef enum
    {
      kSetUserScheduleRequestId = 0x1,
      kGetUserScheduleRequestId = 0x2,
      kDeleteUserScheduleRequestId = 0x3,
    } schema_weave_schedule_basic_user_schedules_settings_trait_command_id_t;


    // SetUserScheduleRequest Parameters
    typedef enum
    {
        kSetUserScheduleRequestParameter_UserSchedule = 2,
    } schema_weave_schedule_basic_user_schedules_settings_trait_set_user_schedule_request_param_t;
    // GetUserScheduleRequest Parameters
    typedef enum
    {
        kGetUserScheduleRequestParameter_UserId = 1,
    } schema_weave_schedule_basic_user_schedules_settings_trait_get_user_schedule_request_param_t;
    // DeleteUserScheduleRequest Parameters
    typedef enum
    {
        kDeleteUserScheduleRequestParameter_UserId = 1,
    } schema_weave_schedule_basic_user_schedules_settings_trait_delete_user_schedule_request_param_t;


    // SetUserScheduleResponse Parameters
    typedef enum
    {
        kSetUserScheduleResponseParameter_Status = 1,
    } schema_weave_schedule_basic_user_schedules_settings_trait_set_user_schedule_response_param_t;
    // GetUserScheduleResponse Parameters
    typedef enum
    {
        kGetUserScheduleResponseParameter_Status = 1,
        kGetUserScheduleResponseParameter_UserSchedule = 2,
    } schema_weave_schedule_basic_user_schedules_settings_trait_get_user_schedule_response_param_t;
    // DeleteUserScheduleResponse Parameters
    typedef enum
    {
        kDeleteUserScheduleResponseParameter_Status = 1,
    } schema_weave_schedule_basic_user_schedules_settings_trait_delete_user_schedule_response_param_t;

    //
    // Enums
    //

    // ScheduleErrorCodes
    typedef enum
    {
    SCHEDULE_ERROR_CODES_SUCCESS_STATUS = 1,
    SCHEDULE_ERROR_CODES_DUPLICATE_ENTRY = 2,
    SCHEDULE_ERROR_CODES_INDEX_OUT_OF_RANGE = 3,
    SCHEDULE_ERROR_CODES_EMPTY_SCHEDULE_ENTRY = 4,
    SCHEDULE_ERROR_CODES_INVALID_SCHEDULE = 5,
    } schema_weave_schedule_basic_user_schedules_settings_trait_schedule_error_codes_t;




#endif // _WEAVE_TRAIT_SCHEDULE__BASIC_USER_SCHEDULES_SETTINGS_TRAIT_C_H_

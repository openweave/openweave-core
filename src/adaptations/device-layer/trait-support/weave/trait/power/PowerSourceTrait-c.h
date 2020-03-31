
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
 *    SOURCE PROTO: weave/trait/power/power_source_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_C_H_
#define _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_C_H_




    //
    // Enums
    //

    // PowerSourceCondition
    typedef enum
    {
    POWER_SOURCE_CONDITION_NOMINAL = 1,
    POWER_SOURCE_CONDITION_CRITICAL = 2,
    } schema_weave_power_power_source_trait_power_source_condition_t;
    // PowerSourceStatus
    typedef enum
    {
    POWER_SOURCE_STATUS_ACTIVE = 1,
    POWER_SOURCE_STATUS_STANDBY = 2,
    POWER_SOURCE_STATUS_INACTIVE = 3,
    } schema_weave_power_power_source_trait_power_source_status_t;




#endif // _WEAVE_TRAIT_POWER__POWER_SOURCE_TRAIT_C_H_

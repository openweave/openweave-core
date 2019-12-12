
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
 *    SOURCE PROTO: weave/trait/synchronization/synchronization_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_C_H_
#define _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_C_H_




    //
    // Enums
    //

    // SyncronizationStatus
    typedef enum
    {
    SYNCRONIZATION_STATUS_SYNCHRONIZED = 1,
    SYNCRONIZATION_STATUS_PENDING = 2,
    SYNCRONIZATION_STATUS_TIMEOUT = 3,
    SYNCRONIZATION_STATUS_FAILED_RETRY = 4,
    SYNCRONIZATION_STATUS_FAILED_FATAL = 5,
    } schema_weave_synchronization_synchronization_trait_syncronization_status_t;




#endif // _WEAVE_TRAIT_SYNCHRONIZATION__SYNCHRONIZATION_TRAIT_C_H_

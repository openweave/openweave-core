
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
 *    SOURCE PROTO: weave/trait/heartbeat/liveness_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_C_H_
#define _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_C_H_




    //
    // Enums
    //

    // LivenessDeviceStatus
    typedef enum
    {
    LIVENESS_DEVICE_STATUS_ONLINE = 1,
    LIVENESS_DEVICE_STATUS_UNREACHABLE = 2,
    LIVENESS_DEVICE_STATUS_UNINITIALIZED = 3,
    LIVENESS_DEVICE_STATUS_REBOOTING = 4,
    LIVENESS_DEVICE_STATUS_UPGRADING = 5,
    LIVENESS_DEVICE_STATUS_SCHEDULED_DOWN = 6,
    } schema_weave_heartbeat_liveness_trait_liveness_device_status_t;




#endif // _WEAVE_TRAIT_HEARTBEAT__LIVENESS_TRAIT_C_H_

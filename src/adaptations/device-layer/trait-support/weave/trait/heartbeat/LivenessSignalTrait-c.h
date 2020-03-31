
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
 *    SOURCE PROTO: weave/trait/heartbeat/liveness_signal_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_C_H_
#define _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_C_H_




    //
    // Enums
    //

    // LivenessSignalType
    typedef enum
    {
    LIVENESS_SIGNAL_TYPE_MUTUAL_SUBSCRIPTION_ESTABLISHED = 1,
    LIVENESS_SIGNAL_TYPE_SUBSCRIPTION_HEARTBEAT = 2,
    LIVENESS_SIGNAL_TYPE_NON_SUBSCRIPTION_HEARTBEAT = 3,
    LIVENESS_SIGNAL_TYPE_NOTIFY_REQUEST_UNDELIVERED = 4,
    LIVENESS_SIGNAL_TYPE_COMMAND_REQUEST_UNDELIVERED = 5,
    } schema_weave_heartbeat_liveness_signal_trait_liveness_signal_type_t;




#endif // _WEAVE_TRAIT_HEARTBEAT__LIVENESS_SIGNAL_TRAIT_C_H_

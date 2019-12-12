
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
 *    SOURCE PROTO: weave/trait/telemetry/tunnel/telemetry_tunnel_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_C_H_
#define _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_C_H_




    //
    // Enums
    //

    // TunnelType
    typedef enum
    {
    TUNNEL_TYPE_NONE = 1,
    TUNNEL_TYPE_PRIMARY = 2,
    TUNNEL_TYPE_BACKUP = 3,
    TUNNEL_TYPE_SHORTCUT = 4,
    } schema_weave_telemetry_tunnel_telemetry_tunnel_trait_tunnel_type_t;
    // TunnelState
    typedef enum
    {
    TUNNEL_STATE_NO_TUNNEL = 1,
    TUNNEL_STATE_PRIMARY_ESTABLISHED = 2,
    TUNNEL_STATE_BACKUP_ONLY_ESTABLISHED = 3,
    TUNNEL_STATE_PRIMARY_AND_BACKUP_ESTABLISHED = 4,
    } schema_weave_telemetry_tunnel_telemetry_tunnel_trait_tunnel_state_t;




#endif // _WEAVE_TRAIT_TELEMETRY_TUNNEL__TELEMETRY_TUNNEL_TRAIT_C_H_

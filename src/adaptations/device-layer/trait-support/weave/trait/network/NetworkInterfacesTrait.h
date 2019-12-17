
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
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/network/network_interfaces_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_NETWORK__NETWORK_INTERFACES_TRAIT_H_
#define _WEAVE_TRAIT_NETWORK__NETWORK_INTERFACES_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Network {
namespace NetworkInterfacesTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xb03U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  is_network_interface_id_list_dynamicbool                                 bool              NO              NO
    //
    kPropertyHandle_IsNetworkInterfaceIdListDynamic = 2,

    //
    //  network_interface_id_list           repeated uint32                      array             NO              NO
    //
    kPropertyHandle_NetworkInterfaceIdList = 3,

    //
    //  primary_network_interface_id_ipv4   uint32                               uint32            YES             NO
    //
    kPropertyHandle_PrimaryNetworkInterfaceIdIpv4 = 4,

    //
    //  primary_network_interface_id_ipv6   uint32                               uint32            YES             NO
    //
    kPropertyHandle_PrimaryNetworkInterfaceIdIpv6 = 5,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 5,
};

} // namespace NetworkInterfacesTrait
} // namespace Network
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_NETWORK__NETWORK_INTERFACES_TRAIT_H_

/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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

/**
 *    @file
 *      This file defines constant enumerations for all public (or
 *      common) and Nest Labs vendor-unique Nest Weave profiles.
 *
 */

#ifndef WEAVEPROFILES_H_
#define WEAVEPROFILES_H_

#include <Weave/Core/WeaveVendorIdentifiers.hpp>

/**
 *   @namespace nl::Weave::Profiles
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for Weave
 *     profiles, both Common and Nest Labs vendor-specific.
 */

namespace nl {
namespace Weave {
namespace Profiles {

//
// Weave Profile Ids (32-bits max)
//

enum WeaveProfileId
{
    // Common Profiles
    //
    // NOTE: Do not attempt to allocate these values yourself. These
    //       values are under administration by Nest Labs.

    kWeaveProfile_Common                        = (kWeaveVendor_Common << 16) | 0x0000,           // Common Profile
    kWeaveProfile_Core_Deprecated               = (kWeaveVendor_Common << 16) | 0x0000,           // Core (deprecated, renamed Common) Profile
    kWeaveProfile_Echo                          = (kWeaveVendor_Common << 16) | 0x0001,           // Echo Profile
    kWeaveProfile_StatusReport_Deprecated       = (kWeaveVendor_Common << 16) | 0x0002,           // Status Report Profile
    kWeaveProfile_NetworkProvisioning           = (kWeaveVendor_Common << 16) | 0x0003,           // Network Provisioning Profile
    kWeaveProfile_Security                      = (kWeaveVendor_Common << 16) | 0x0004,           // Network Security Profile
    kWeaveProfile_FabricProvisioning            = (kWeaveVendor_Common << 16) | 0x0005,           // Fabric Provisioning Profile
    kWeaveProfile_DeviceControl                 = (kWeaveVendor_Common << 16) | 0x0006,           // Device Control Profile
    kWeaveProfile_Time                          = (kWeaveVendor_Common << 16) | 0x0007,           // Time Services Profile
    kWeaveProfile_WDM                           = (kWeaveVendor_Common << 16) | 0x000B,           // Data Management Profile
    kWeaveProfile_SWU                           = (kWeaveVendor_Common << 16) | 0x000C,           // Software Update Profile
    kWeaveProfile_BDX                           = (kWeaveVendor_Common << 16) | 0x000D,           // Bulk Data Transfer Profile
    kWeaveProfile_DeviceDescription             = (kWeaveVendor_Common << 16) | 0x000E,           // Weave Device Description Profile
    kWeaveProfile_ServiceProvisioning           = (kWeaveVendor_Common << 16) | 0x000F,           // Service Provisioning Profile
    kWeaveProfile_ServiceDirectory              = (kWeaveVendor_Common << 16) | 0x0010,           // Service Directory Profile
    kWeaveProfile_Locale                        = (kWeaveVendor_Common << 16) | 0x0011,           // Locale Profile
    kWeaveProfile_Tunneling                     = (kWeaveVendor_Common << 16) | 0x0012,           // Service Routing by Tunneling
    kWeaveProfile_Heartbeat                     = (kWeaveVendor_Common << 16) | 0x0013,           // Heartbeat
    kWeaveProfile_ApplicationKeys               = (kWeaveVendor_Common << 16) | 0x001D,           // Application Keys
    kWeaveProfile_TokenPairing                  = (kWeaveVendor_Common << 16) | 0x0020,           // Token Pairing Profile
    kWeaveProfile_DictionaryKey                 = (kWeaveVendor_Common << 16) | 0x0021,           // Dictionary Key in Data Management Profile

    // Nest Labs Profiles
    //
    // NOTE: Do not attempt to allocate these values yourself. These
    //       values are under administration by Nest Labs. Please make a
    //       formal request using the "Nest Weave: Nest Labs Profile
    //       Identifier Registry" <https://docs.google.com/a/nestlabs.com/document/d/1Gw1uM3Z5mTxWL1RAEl3DKiLDkqC5A9tnD03qgzTfOU4>.

    kWeaveProfile_Occupancy                     = (kWeaveVendor_NestLabs << 16) | 0x0001,       // Nest Occupancy profile
    kWeaveProfile_Structure                     = (kWeaveVendor_NestLabs << 16) | 0x0002,       // Nest Structure profile
    kWeaveProfile_NestProtect                   = (kWeaveVendor_NestLabs << 16) | 0x0003,       // Nest Protect product profile
    kWeaveProfile_TimeVariantData               = (kWeaveVendor_NestLabs << 16) | 0x0004,       // Weave TVD profile
    kWeaveProfile_Alarm                         = (kWeaveVendor_NestLabs << 16) | 0x0006,       // Weave Alarm profile
    kWeaveProfile_HeatLink                      = (kWeaveVendor_NestLabs << 16) | 0x0007,       // Nest Heat-Link product profile
    kWeaveProfile_Safety                        = (kWeaveVendor_NestLabs << 16) | 0x0008,       // Nest Safety profile
    kWeaveProfile_SafetySummary                 = (kWeaveVendor_NestLabs << 16) | 0x0009,       // Nest Safety Summary profile
    kWeaveProfile_NestThermostat                = (kWeaveVendor_NestLabs << 16) | 0x000A,       // Nest Thermostat product profile
    kWeaveProfile_NestBoiler                    = (kWeaveVendor_NestLabs << 16) | 0x000B,       // Nest Boiler profile
    kWeaveProfile_NestHvacEquipmentControl      = (kWeaveVendor_NestLabs << 16) | 0x000C,       // Nest HVAC Equipment Control profile
    kWeaveProfile_NestDomesticHotWater          = (kWeaveVendor_NestLabs << 16) | 0x000D,       // Nest Domestic Hot Water profile
    kWeaveProfile_TopazHistory                  = (kWeaveVendor_NestLabs << 16) | 0x000F,       // Nest Topaz History profile
    kWeaveProfile_DropcamLegacyPairing          = (kWeaveVendor_NestLabs << 16) | 0x0010,       // Nest Dropcam Legacy Pairing profile

    kWeaveProfile_NestNetworkManager            = (kWeaveVendor_NestLabs << 16) | 0xE000,       // Nest Network Manager (nonvolatile storage-only) profile

    // Profiles reserved for internal protocol use
    //
    // NOTE: Do not attempt to allocate these values yourself. These
    //       values are under administration by Nest Labs.

    kWeaveProfile_NotSpecified                  = (kWeaveVendor_NotSpecified << 16) | 0xFFFF,   // The profile ID is either not specified or a wildcard
};

} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVEPROFILES_H_ */

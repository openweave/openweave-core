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
 *      Constants and enums for bringing the Weave constants into Objective C
 *
 */

#import <Foundation/Foundation.h>

#ifndef __NLWEAVEDEVICEMANAGERTYPES_H__
#define __NLWEAVEDEVICEMANAGERTYPES_H__

// Because there doesn't seem to be a very easy way to bring over C++ types nested inside namespaces
// into our pure Objective-C project

// Network Type
//
typedef NS_ENUM(NSInteger, NLNetworkType)
{
    kNLNetworkType_NotSpecified                     = -1,

    kNLNetworkType_WiFi                             = 1,
    kNLNetworkType_Thread                           = 2

};


// WiFi Operating Modes
//
typedef NS_ENUM(NSInteger, NLWiFiMode)
{
    kNLWiFiMode_NotSpecified                        = -1,

    kNLWiFiMode_AdHoc                               = 1,
    kNLWiFiMode_Managed                             = 2
};


// Device WiFi Role
//
typedef NS_ENUM(NSInteger, NLWiFiRole)
{
    kNLWiFiRole_NotSpecified                        = -1,

    kNLWiFiRole_Station                             = 1,
    kNLWiFiRole_AccessPoint                         = 2
};


// Rendezvous Mode Flags
//
typedef NS_ENUM(NSInteger, NLRendezvousModeFlags)
{
    kNLRendezvousMode_EnableWiFiRendezvousNetwork   = 0x0001,
    kNLRendezvousMode_Enable802154RendezvousNetwork = 0x0002,
    kNLRendezvousMode_EnableFabricRendezvousAddress = 0x0004
};

// Get Network Flags
//
typedef NS_ENUM(NSInteger, NLGetNetworkFlags)
{
    kNLGetNetwork_IncludeCredentials                = 0x01
};

// WiFi Security Modes
//
typedef NS_ENUM(NSInteger, NLWiFiSecurityType)
{
    kNLWiFiSecurityType_NotSpecified                = -1,

    kNLWiFiSecurityType_None                        = 1,
    kNLWiFiSecurityType_WEP                         = 2,
    kNLWiFiSecurityType_WPAPersonal                 = 3,
    kNLWiFiSecurityType_WPA2Personal                = 4,
    kNLWiFiSecurityType_WPA2MixedPersonal           = 5,
    kNLWiFiSecurityType_WPAEnterprise               = 6,
    kNLWiFiSecurityType_WPA2Enterprise              = 7,
    kNLWiFiSecurityType_WPA2MixedEnterprise         = 8
};

// Device Descriptor
//
typedef struct NLManufacturingDate
{
    NSInteger NLManufacturingDateYear;
    NSInteger NLManufacturingDateMonth;
    NSInteger NLManufacturingDateDay;
} NLManufacturingDate;


typedef NS_ENUM(NSInteger, NLWeaveRequestError)
{
    NLWeaveRequestError_ProfileStatusError          = 1,
    NLWeaveRequestError_WeaveError                  = 2,
    NLWeaveRequestError_UnknownError,

};


typedef NS_ENUM(NSInteger, NLWeaveVendorId)
{
    kNLWeaveVendor_Any                                = 0xFFFF,
    kNLWeaveVendor_Core                               = 0x0000,
    kNLWeaveVendor_NestLabs                           = 0x235A,
    kNLWeaveVendor_Yale                               = 0xE727
};

//
// Nest Labs Weave Product Identifiers (16 bits max)
//

typedef NS_ENUM(NSInteger, NestWeaveProductId)
{
    kNestWeaveProduct_NotSpecified               = -1,
    kNestWeaveProduct_Diamond                    = 0x0001,
    kNestWeaveProduct_DiamondBackplate           = 0x0002,
    kNestWeaveProduct_Diamond2                   = 0x0003,
    kNestWeaveProduct_Diamond2Backplate          = 0x0004,
    kNestWeaveProduct_Topaz                      = 0x0005,
    kNestWeaveProduct_AmberBackplate             = 0x0006,
    kNestWeaveProduct_Amber                      = 0x0007,  // DEPRECATED -- Use kNestWeaveProduct_AmberHeatLink
    kNestWeaveProduct_AmberHeatLink              = 0x0007,
    kNestWeaveProduct_Pinna                      = 0x0008,
    kNestWeaveProduct_Topaz2                     = 0x0009,
    kNestWeaveProduct_Diamond3                   = 0x000A,
    kNestWeaveProduct_Diamond3Backplate          = 0x000B,
    kNestWeaveProduct_Flintstone                 = 0x000C,
    kNestWeaveProduct_Quartz                     = 0x000D,
    kNestWeaveProduct_Keshi                      = 0x000E,
    kNestWeaveProduct_Amber2HeatLink             = 0x000F,
    kNestWeaveProduct_SmokyQuartz                = 0x0010,
    kNestWeaveProduct_Quartz2                    = 0x0011,
    kNestWeaveProduct_BlackQuartz                = 0x0012,
    kNestWeaveProduct_Nevis                      = 0x0013,
    kNestWeaveProduct_Onyx                       = 0x0014,
    kNestWeaveProduct_OnyxBackplate              = 0x0015,
    kNestWeaveProduct_Antigua                    = 0x0016,
    kNestWeaveProduct_RoseQuartz                 = 0x0017,
    kNestWeaveProduct_Moonstone                  = 0x0018
};

//
// Weave Profile Ids (32-bits max)
//

typedef NS_ENUM(NSInteger, NLWeaveProfileId)
{
    // Core profiles
    kWeaveProfile_Common                            = (kNLWeaveVendor_Core << 16) | 0x0000,
    kWeaveProfile_Echo                              = (kNLWeaveVendor_Core << 16) | 0x0001,           // Echo Profile
    kWeaveProfile_StatusReport_Deprecated           = (kNLWeaveVendor_Core << 16) | 0x0002,           // Status Report Profile
    kWeaveProfile_NetworkProvisioning               = (kNLWeaveVendor_Core << 16) | 0x0003,           // Network Provisioning Profile
    kWeaveProfile_Security                          = (kNLWeaveVendor_Core << 16) | 0x0004,           // Network Security Profile
    kWeaveProfile_FabricProvisioning                = (kNLWeaveVendor_Core << 16) | 0x0005,           // Fabric Provisioning Profile
    kWeaveProfile_DeviceControl                     = (kNLWeaveVendor_Core << 16) | 0x0006,           // Device Control Profile
    kWeaveProfile_WDM                               = (kNLWeaveVendor_Core << 16) | 0x000B,           // Data Management Profile
    kWeaveProfile_SWU                               = (kNLWeaveVendor_Core << 16) | 0x000C,           // Software Update Profile
    kWeaveProfile_BDX                               = (kNLWeaveVendor_Core << 16) | 0x000D,           // Bulk Data Transfer Profile
    kWeaveProfile_DeviceDescription                 = (kNLWeaveVendor_Core << 16) | 0x000E,           // Weave Device Description Profile
    kWeaveProfile_ServiceProvisioning               = (kNLWeaveVendor_Core << 16) | 0x000F,           // Service Provisioning Profile
    kWeaveProfile_ServiceDirectory                  = (kNLWeaveVendor_Core << 16) | 0x0010,           // Service Directory Profile

    // Nest Labs profiles
    kWeaveProfile_Occupancy                         = (kNLWeaveVendor_NestLabs << 16) | 0x0001,       // Weave Occupancy profile
    kWeaveProfile_Structure                         = (kNLWeaveVendor_NestLabs << 16) | 0x0002,       // Weave Structure profile
    kWeaveProfile_NestProtect                       = (kNLWeaveVendor_NestLabs << 16) | 0x0003,       // Weave Topaz profile
    kWeaveProfile_TimeVariantData                   = (kNLWeaveVendor_NestLabs << 16) | 0x0004,       // Weave TVD profile
};

//
// status/error codes for BDX
//
typedef NS_ENUM(NSInteger, NLBDXStatus) {
    kNLStatus_Overflow                              = 0x0011,
    kNLStatus_LengthTooShort                        = 0x0013,
    kNLStatus_XferFailedUnknownErr                  = 0x001F,
    kNLStatus_XferMethodNotSupported                = 0x0050,
    kNLStatus_UnknownFile                           = 0x0051,
    kNLStatus_StartOffsetNotSupported               = 0x0052,
    kNLStatus_Unknown                               = 0x005F,
};


// Common Profile Status Codes
//
typedef NS_ENUM(NSInteger, NLCommonProfileStatus)
{
    kNLStatus_Success                               = 0,                    // The operation completed without error.

    kNLStatus_BadRequest                            = 0x0010,               // The request was unrecognized or malformed.
    kNLStatus_UnsupportedMessage                    = 0x0011,               // An unrecognized or unsupported message was received.
    kNLStatus_UnexpectedMessage                     = 0x0012,               // A message was received at an unexpected time or in an unexpected context.
    kNLStatus_OutOfMemory                           = 0x0017,               // Out of memory

    kNLStatus_Relocated                             = 0x0030,               // Request could not be completed because it was made to the wrong endpoint.
                                                                            // Client should query its directory server for an updated endpoint list and try again.

    kNLStatus_Busy                                  = 0x0040,               // The current state of the sender prevents the operation from being performed.
    kNLStatus_Timeout                               = 0x0041,               // The operation or protocol interaction failed to complete in the allotted time.

    kNLStatus_InternalError                         = 0x0050,               // An internal failure prevented an operation from completing.

    kNLStatus_Continue                              = 0x0090,               // Context-specific signal to proceed.
};

// Data Management Status Codes
//
typedef NS_ENUM(NSInteger, NLDataManagementStatus)
{
    kNLStatus_InvalidPath                           = 0x0013,               // A path from the path list of a view or update request frame
                                                                            // did not match the node-resident schema of the responder.

    kNLStatus_UnknownTopic                          = 0x0014,               // The topic given in a cancel request did not match any
                                                                            // subscription extant on the receiving node.

    kNLStatus_IllegalReadRequest                    = 0x0015,               // The node making a request to read a particular data item does
                                                                            // not have permission to do so.

    kNLStatus_IllegalWriteRequest                   = 0x0016,               // The node making a request to write a particular data item does
                                                                            // not have permission to do so.
};

// Device Control Status Codes
//
typedef NS_ENUM(NSInteger, NLDeviceControlStatus)
{
    kNLStatusCode_FailSafeAlreadyActive             = 1,                    // A fail-safe is already active.
    kNLStatusCode_NoFailSafeActive                  = 2,                    // No fail-safe is active.
    kNLStatusCode_NoMatchingFailSafeActive          = 3,                    // The fail-safe token did not match the active fail-safe.
    kNLStatusCode_UnsupportedFailSafeMode           = 4                     // The specified fail-safe mode is not supported by the device.
};


// Fabric Provisioning Status Codes
//
typedef NS_ENUM(NSInteger, NLFabricProvisioningStatus)
{
    kNLStatusCode_AlreadyMemberOfFabric             = 1,                    // The recipient is already a member of a fabric.
    kNLStatusCode_NotMemberOfFabric                 = 2,                    // The recipient is not a member of a fabric.
    kNLStatusCode_InvalidFabricConfig               = 3                     // The specified fabric configuration was invalid.
};


// Network Provisioning Status Codes
//
typedef NS_ENUM(NSInteger, NLNetworkProvisioningStatus)
{
    kNLStatusCode_UnknownNetwork                    = 1,
    kNLStatusCode_TooManyNetworks                   = 2,
    kNLStatusCode_InvalidNetworkConfiguration       = 3,
    kNLStatusCode_UnsupportedNetworkType            = 4,
    kNLStatusCode_UnsupportedWiFiMode               = 5,
    kNLStatusCode_UnsupportedWiFiRole               = 6,
    kNLStatusCode_UnsupportedWiFiSecurityType       = 7,
    kNLStatusCode_InvalidState                      = 8,
    kNLStatusCode_TestNetworkFailed                 = 9,                    // XXX Placeholder for more detailed errors to come before we ship 1.0
};


// Weave Security Status Codes
//
typedef NS_ENUM(NSInteger, NLWeaveSecurityStatus)
{
    kNLStatusCode_SessionAborted                    = 1,                    // The sender has aborted the session establishment process.
    kNLStatusCode_PASESupportsOnlyConfig1           = 2,                    // PASE only supoprts Config1.
    kNLStatusCode_UnsupportedEncryptionType         = 3,                    // The requested encryption type is not support.
    kNLStatusCode_InvalidKeyId                      = 4,                    // An invalid key id was requested.
    kNLStatusCode_DuplicateKeyId                    = 5,                    // The specified key id is already in use.
    kNLStatusCode_KeyConfirmationFailed             = 6,                    // The derived session keys do not agree.
    kNLStatusCode_InternalError                     = 7,                    // The sender encounters an internal error (e.g. no memory, etc...).
    kNLStatusCode_AuthenitcationFailed              = 8,                    // The sender rejected the authentication attempt.
    kNLStatusCode_NoCommonPASEConfigurations        = 9                     // No supported PASE configurations in common.
};

// Service Directory status codes
typedef NS_ENUM(NSInteger, NLServiceDirectoryStatus) {
    kNLStatus_DirectoryUnavailable                  = 0x0051
};


// Service Provisioning Status Codes
//
typedef NS_ENUM(NSInteger, NLServiceProvisioningStatus)
{
    kNLStatusCode_TooManyServices                   = 1,                    // There are too many service's registered on the device.
    kNLStatusCode_ServiceAlreadyRegistered          = 2,                    // The specified service is already registered on the device.
    kNLStatusCode_InvalidServiceConfig              = 3,                    // The specified service configuration is invalid.
    kNLStatusCode_NoSuchService                     = 4,                    // The specified id does not match a service registered on the device.
    kNLStatusCode_PairingServerError                = 5                     // The device could not complete service pairing because it failed to talk to the pairing server.
};


//
// Software Update
//
typedef NS_ENUM(NSInteger, NLSoftwareUpdateStatus) {
    kNLStatus_NoUpdateAvailable                     = 0x0001,
    kNLStatus_UpdateFailed                          = 0x0010,
    kNLStatus_InvalidInstructions                   = 0x0050,
    kNLStatus_DownloadFailed                        = 0x0051,
    kNLStatus_IntegrityCheckFailed                  = 0x0052,
    kNLStatus_Abort                                 = 0x0053,
    kNLStatus_Retry                                 = 0x0091,
};


//
// Fail Safe modes
//
typedef NS_ENUM(NSInteger, NLFailSafeMode) {
    kFailSafeModeUndefined                          = 0,
    kFailSafeModeNew                                = 1,
    kFailSafeModeReset,
    kFailSafeModeResume,
};


// Special target fabric ids
typedef NS_ENUM(uint64_t, NLTargetFabricIds)
{
    NLTargetFabricId_NotInFabric                   = 0,                    // Specifies that only devices that are NOT a member of a fabric should respond.
    NLTargetFabricId_AnyFabric                     = 0xFFFFFFFFFFFFFF00ULL,// Specifies that only devices that ARE a member of a fabric should respond.
    NLTargetFabricId_Any                           = 0xFFFFFFFFFFFFFFFFULL,// Specifies that all devices should respond regardless of fabric membership.
};

// Special node id values.
typedef NS_ENUM(uint64_t, NLTargetDeviceIds)
{
    NLTargetDeviceId_NotSpecified                  = 0ULL,                 // Node Id is not specified
    NLTargetDeviceId_AnyNodeId                     = 0xFFFFFFFFFFFFFFFFULL,// Any node id is accepted
};

typedef NS_ENUM(NSInteger, NLTargetDeviceModes) {
    NLTargetDeviceModeAny                         = 0,                      // Locate all devices regardless of mode.

    NLTargetDeviceModeUserSelectedMode            = 1,                      // Locate all devices in 'user-selected' mode -- i.e. where the device has has been
                                                                            // directly identified by a user, e.g. by pressing a button.
};

typedef enum NLDeviceFeatures {
    kNLDeviceFeatureNone                           = 0x0000,
    kNLDeviceFeature_LinePowered                   = 0x0002,                 // Indicates a device that requires line power.
} NLDeviceFeatures;

typedef NS_ENUM(NSInteger, NLProductWildcardId) {
    NLProductWildcardId_NestThermostat             = 0xFFF0,
    NLProductWildcardId_NestProtect                = 0xFFF1,
    NLProductWildcardId_NestCam                    = 0xFFF2,
};

#endif // __NLWEAVEDEVICEMANAGERTYPES_H__

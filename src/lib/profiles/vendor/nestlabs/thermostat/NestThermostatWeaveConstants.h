/*
 *
 *    Copyright (c) 2013-2014 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

/**
 *    @file
 *      This file defines Weave-specific constants used by the
 *      Nest Thermostat.
 */

#ifndef WEAVE_PROFILES_NEST_THERMOSTAT_CONSTANTS
#define WEAVE_PROFILES_NEST_THERMOSTAT_CONSTANTS

namespace nl {

namespace Weave {

namespace Profiles {

namespace Vendor {

namespace Nestlabs {

namespace Thermostat {

// Thermostat-specific status codes

typedef enum
{
    kStatus_InFieldJoining_Unknown =                                  -1,     // Unknown
    kStatus_InFieldJoining_Null =                                      0,     // In-field joining started by the service
    kStatus_InFieldJoining_Succeeded =                                 1,     // In-field joining succeeded
    kStatus_InFieldJoining_CannotLocateAssistingDevice =               2,     // Failure to locate assisting device
    kStatus_InFieldJoining_CannotConnectAssistingDevice =              3,     // Failure to connect to assisting device
    kStatus_InFieldJoining_CannotAuthAssistingDevice =                 4,     // Failure to authenticate to assisting device
    kStatus_InFieldJoining_ConfigExtractionError =                     5,     // Error extracting configuration from assisting device
    kStatus_InFieldJoining_PANFormError =                              6,     // Failure to form 802.15.4 PAN
    kStatus_InFieldJoining_PANJoinError =                              7,     // Failure to join 802.15.4 PAN
    kStatus_InFieldJoining_HVACCycleInProgress =                       8,     // HVAC cycle in progress
    kStatus_InFieldJoining_HeatLinkJoinInProgress =                    9,     // HeatLink join in progress
    kStatus_InFieldJoining_HeatLinkUpdateInProgress =                 10,     // HeatLink software update in progress
    kStatus_InFieldJoining_HeatLinkManualHeatActive =                 11,     // Heatlink manual heat active
    kStatus_InFieldJoining_IncorrectHeatLinkSoftwareVersion =         12,     // Incorrect HeatLink software version
    kStatus_InFieldJoining_FailureToFetchAccessToken =                13,     // Failure to fetch access token
    kStatus_InFieldJoining_DeviceNotWeaveProvisioned =                14,     // Device not Weave provisioned
    kStatus_InFieldJoining_HeatLinkResetFailed =                      15,     // Failed to factory reset HeatLink
    kStatus_InFieldJoining_DestroyFabricFailed =                      16,     // Failed to destroy existing fabric
    kStatus_InFieldJoining_CannotJoinExistingFabric =                 17,     // Failed to join existing fabric
    kStatus_InFieldJoining_CannotCreateFabric =                       18,     // Failed to create new fabric
    kStatus_InFieldJoining_NetworkReset =                             19,     // Network was reset on the device
    kStatus_InFieldJoining_JoiningInProgress =                        20,     // Device already in-field joining
    kStatus_InFieldJoining_FailureToMakePanJoinable =                 21,     // Assisting device failed to make it's PAN joinable
    kStatus_InFieldJoining_WeaveConnectionTimeoutStillActive =        22,     // Timeout used to keep us awake while connected to another device still active
    kStatus_InFieldJoining_HeatLinkNotJoined =                        23,     // HeatLink not joined to head unit
    kStatus_InFieldJoining_HeatLinkNotInContact =                     24,     // HeatLink not in contact with head unit
    kStatus_InFieldJoining_WiFiTechNotEnabled =                       25,     // WiFi technology is not enabled
    kStatus_InFieldJoining_15_4_TechNotEnabled =                      26,     // 15.4 technology is not enabled
    kStatus_InFieldJoining_StandaloneFabricCreationInProgress =       27,     // Standalone fabric creation is in progress
    kStatus_InFieldJoining_NotConnectedToPower =                      28,     // Backplate not connected to any power
    kStatus_InFieldJoining_OperationNotPermitted =                    29,     // In-field joining not permitted
    kStatus_InFieldJoining_ServiceTimedOut =                         100,     // Joining operation timed out (set by service)
    kStatus_InFieldJoining_DeviceTimedOut =                          101,     // Joining operation timed out (set by device)
    kStatus_InFieldJoining_InternalError =                           200,     // Internal error during in-field joining

    kStatus_InFieldJoining_MinComplete = kStatus_InFieldJoining_Succeeded,
    kStatus_InFieldJoining_MaxComplete = kStatus_InFieldJoining_InternalError,
} InFieldJoiningStatus;

#define IFJ_STATUS_COMPLETE(_result) (((_result) >= kStatus_InFieldJoining_MinComplete) && ((_result) <= kStatus_InFieldJoining_MaxComplete))

const char *IfjStatusStr(InFieldJoiningStatus status);

// Thermostat-specific tags
//
//                                Tag        Tag               Element
//                                Number     Category          Type           Constraints   Disposition  Readability  Writability
//                                -----------------------------------------------------------------------------------------------

enum {
    kTag_LegacyEntryKey         = 0x0001, // Profile-specific  UTF-8 String   None          Optional     Any          -
    kTag_SystemTestStatusKey    = 0x0002, // Profile-specific  Unsigned Int   See tbl 4     Optional     Any          -
};

// Thermostat-specific status codes
enum {
    kStatus_ServiceUnreachable  = 0x0001, // Cannot contact the Service to retrieve the Legacy Entry Key
    kStatus_DeviceAlreadyPaired = 0x0002, // The device is already paired to a Nest account
};

// Thermostat-specific system test codes. Do not change these values even if a test gets deprecated.
// They are used in kTag_SystemTestStartKey Update requests and kTag_SystemTestStatusKey view responses
typedef enum {
    kSystemTestCode_None                   = 0x00,
    kSystemTestCode_Cooling                = 0x01,
    kSystemTestCode_Heating                = 0x02,
    kSystemTestCode_AlternateHeating       = 0x03,
    kSystemTestCode_AuxiliaryHeating       = 0x04,
    kSystemTestCode_Dehumidifier           = 0x05,
    kSystemTestCode_EmergencyHeating       = 0x06,
    kSystemTestCode_Fan                    = 0x07,
    kSystemTestCode_Humidifier             = 0x08,
    kSystemTestCode_AlternateHeatStage2    = 0x09,
    kSystemTestCode_CoolingStage2          = 0x0A,
    kSystemTestCode_HeatingStage2          = 0x0B,
    kSystemTestCode_HeatingStage3          = 0x0C,
} SystemTestCode;


// Thermostat-specific system test status codes. Do not change these values even if a test status gets deprecated.
// They are used in kTag_SystemTestStatusKey view responses.  The lower byte of these values will be used to store
// specific test codes from the SystemTestCode enum above.
typedef enum {
    kSystemTestStatusCode_Idle              = 0x0000,
    kSystemTestStatusCode_Running           = 0x0100,
    kSystemTestStatusCode_Timeout           = 0x0200,
    kSystemTestStatusCode_TooHot            = 0x0300,
    kSystemTestStatusCode_TooCold           = 0x0400,
    kSystemTestStatusCode_CompressorLockout = 0x0500,
    kSystemTestStatusCode_Invalid           = 0x0600,
    kSystemTestStatusCode_WeatherRequired   = 0x0700,
} SystemTestStatusCode;

// Thermostat-specific system status codes. Do not change these values even if a status gets deprecated.
// They are used in kTag_SystemStatusKey view responses
typedef enum {
    kSystemStatusCode_Idle                      = 0x0000,  //No error codes or special conditions
    kSystemStatusCode_NoWeatherInfo             = 0x0001,  //No weather info, system tests not possible
    kSystemStatusCode_MandatoryUpdateUnknown    = 0x0002,  //Still checking if a mandatory update is needed
    kSystemStatusCode_MandatoryUpdateInProgress = 0x0004,  //Mandatory update in progress
} SystemStatusCode;

}; // namespace Thermostat

}; // namespace Nestlabs

}; // namespace Vendor

}; // namespace Profiles

}; // namespace Weave

}; // namespace nl

#endif // WEAVE_PROFILES_NEST_THERMOSTAT_CONSTANTS

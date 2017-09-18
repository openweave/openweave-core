/*
 *
 *    Copyright (c) 2016 Nest Labs, Inc.
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
 *      This file...
 *
 */

#include <stdio.h>

#include "NestThermostatWeaveConstants.h"

#define kMaxIfjStatusStrLen 1024

namespace nl {

namespace Weave {

namespace Profiles {

namespace Vendor {

namespace Nestlabs {

namespace Thermostat {

static char sStatusStr[kMaxIfjStatusStrLen];

const char *FormatIfjStatus(const char *format, InFieldJoiningStatus status)
{
    snprintf(sStatusStr, sizeof(sStatusStr) - 2, format, status);
    sStatusStr[sizeof(sStatusStr) - 1] = 0;
    return sStatusStr;
}

const char *IfjStatusStr(InFieldJoiningStatus status)
{
    switch (status)
    {
    case kStatus_InFieldJoining_Unknown:                          return FormatIfjStatus("IFJ Status %d: Unknown",                                                      status);
    case kStatus_InFieldJoining_Succeeded:                        return FormatIfjStatus("IFJ Status %d: Succeeded",                                                    status);
    case kStatus_InFieldJoining_CannotLocateAssistingDevice:      return FormatIfjStatus("IFJ Status %d: Cannot locate assisting device",                               status);
    case kStatus_InFieldJoining_CannotConnectAssistingDevice:     return FormatIfjStatus("IFJ Status %d: Cannot connect to assisting device",                           status);
    case kStatus_InFieldJoining_CannotAuthAssistingDevice:        return FormatIfjStatus("IFJ Status %d: Cannot authenticate with assisting device",                    status);
    case kStatus_InFieldJoining_ConfigExtractionError:            return FormatIfjStatus("IFJ Status %d: Error extracting network/fabric config from assisting device", status);
    case kStatus_InFieldJoining_PANFormError:                     return FormatIfjStatus("IFJ Status %d: Error forming PAN",                                            status);
    case kStatus_InFieldJoining_PANJoinError:                     return FormatIfjStatus("IFJ Status %d: Error joining PAN",                                            status);
    case kStatus_InFieldJoining_HVACCycleInProgress:              return FormatIfjStatus("IFJ Status %d: HVAC cycle in progress",                                       status);
    case kStatus_InFieldJoining_HeatLinkJoinInProgress:           return FormatIfjStatus("IFJ Status %d: Heat-link joining in progress",                                status);
    case kStatus_InFieldJoining_HeatLinkUpdateInProgress:         return FormatIfjStatus("IFJ Status %d: Heat-link software update in progress",                        status);
    case kStatus_InFieldJoining_HeatLinkManualHeatActive:         return FormatIfjStatus("IFJ Status %d: Heat-link in manual heating mode",                             status);
    case kStatus_InFieldJoining_IncorrectHeatLinkSoftwareVersion: return FormatIfjStatus("IFJ Status %d: Heat-link software version incorrect",                         status);
    case kStatus_InFieldJoining_FailureToFetchAccessToken:        return FormatIfjStatus("IFJ Status %d: Failed to fetch access token",                                 status);
    case kStatus_InFieldJoining_DeviceNotWeaveProvisioned:        return FormatIfjStatus("IFJ Status %d: Device not Weave provisioned",                                 status);
    case kStatus_InFieldJoining_HeatLinkResetFailed:              return FormatIfjStatus("IFJ Status %d: Failed to factory reset heat-link",                            status);
    case kStatus_InFieldJoining_DestroyFabricFailed:              return FormatIfjStatus("IFJ Status %d: Failed to destroy existing fabric",                            status);
    case kStatus_InFieldJoining_CannotJoinExistingFabric:         return FormatIfjStatus("IFJ Status %d: Failed to join existing fabric",                               status);
    case kStatus_InFieldJoining_CannotCreateFabric:               return FormatIfjStatus("IFJ Status %d: Failed to create new fabric",                                  status);
    case kStatus_InFieldJoining_NetworkReset:                     return FormatIfjStatus("IFJ Status %d: Network reset on device",                                      status);
    case kStatus_InFieldJoining_JoiningInProgress:                return FormatIfjStatus("IFJ Status %d: In-field joining already in progress",                         status);
    case kStatus_InFieldJoining_FailureToMakePanJoinable:         return FormatIfjStatus("IFJ Status %d: Assisting device failed to make PAN joinable",                 status);
    case kStatus_InFieldJoining_WeaveConnectionTimeoutStillActive:return FormatIfjStatus("IFJ Status %d: Weave connection timeout still active",                        status);
    case kStatus_InFieldJoining_HeatLinkNotJoined:                return FormatIfjStatus("IFJ Status %d: HeatLink not joined to head unit",                             status);
    case kStatus_InFieldJoining_HeatLinkNotInContact:             return FormatIfjStatus("IFJ Status %d: HeatLink not in contact with head unit",                       status);
    case kStatus_InFieldJoining_WiFiTechNotEnabled:               return FormatIfjStatus("IFJ Status %d: WiFi technology is not enabled",                               status);
    case kStatus_InFieldJoining_15_4_TechNotEnabled:              return FormatIfjStatus("IFJ Status %d: 15.4 technology is not enabled",                               status);
    case kStatus_InFieldJoining_StandaloneFabricCreationInProgress: return FormatIfjStatus("Standalone fabric creation is in progress",                                 status);
    case kStatus_InFieldJoining_NotConnectedToPower:              return FormatIfjStatus("Not connected to any power",                                                  status);
    case kStatus_InFieldJoining_OperationNotPermitted:            return FormatIfjStatus("In-field joining not permitted",                                              status);
    case kStatus_InFieldJoining_ServiceTimedOut:                  return FormatIfjStatus("IFJ Status %d: In-field joining timed out on the service",                    status);
    case kStatus_InFieldJoining_DeviceTimedOut:                   return FormatIfjStatus("IFJ Status %d: In-field joining timed out on device",                         status);
    case kStatus_InFieldJoining_InternalError:                    return FormatIfjStatus("IFJ Status %d: Internal error",                                               status);
    default:                                                      return FormatIfjStatus("IFJ Status %d: Invalid status",                                               status);
    }
}

}; // namespace Thermostat

}; // namespace Nestlabs

}; // namespace Vendor

}; // namespace Profiles

}; // namespace Weave

}; // namespace nl

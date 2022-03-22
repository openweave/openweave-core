/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
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
 *      This file implements a utility function for formatting a
 *      human-readable, NULL-terminated string describing the status
 *      code within a particular Weave profile identifier.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdio.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#if WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Development
#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#else
#include <Weave/Profiles/bulk-data-transfer/BulkDataTransfer.h>
#endif
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Current
#include <Weave/Profiles/data-management/Current/MessageDef.h>

#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelControl.h>
#include <Weave/Support/ProfileStringSupport.hpp>

namespace nl {

using namespace nl::Weave::Profiles;

#if !WEAVE_CONFIG_SHORT_ERROR_STR
static const char *FindStatusReportStr(uint32_t inProfileId, uint16_t inStatusCode);
#endif // #if !WEAVE_CONFIG_SHORT_ERROR_STR

/**
 *  This routine returns a human-readable NULL-terminated C string
 *  describing the provided status code associated with the specfied
 *  profile.
 *
 *  @param[in]  profileId   The Weave profile identifier associated with
 *                          @statusCode.
 *
 *  @param[in]  statusCode  The status code in @a profileId to provide a
 *                          descriptive string for.
 *
 *  @return A pointer to a NULL-terminated C string describing the
 *          provided status code within the specfied profile.
 *
 *  @sa #WEAVE_CONFIG_SHORT_ERROR_STR
 */

#if WEAVE_CONFIG_SHORT_ERROR_STR

/**
 * static buffer to store the short version of error string for status report
 * "0x", 32-bit profile id in hex, ' ', "0x", 16-bit status code in hex, '\0'
 * An example would be "0xDEADBEEF,0xABCD"
 */
static char sErrorStr[2 + 8 + 1 + 2 + 4 + 1];

NL_DLL_EXPORT const char *StatusReportStr(uint32_t profileId, uint16_t statusCode)
{
    (void)snprintf(sErrorStr, sizeof(sErrorStr), "0x%" PRIx32 " 0x%" PRIx16, profileId, statusCode);

    return sErrorStr;
}

#else

static char sErrorStr[1024];

NL_DLL_EXPORT const char *StatusReportStr(uint32_t profileId, uint16_t statusCode)
{
    const char *fmt = "[ %s(%08" PRIX32 "):%" PRIu16 " ] %s";
    const char *profileName = NULL;
    const char *errorMsg = "";

    switch (profileId)
    {
    case kWeaveProfile_BDX:
        profileName = "BDX";
        switch (statusCode)
        {
#if WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Development
        case BulkDataTransfer::kStatus_Overflow                            : errorMsg = "Overflow"; break;
        case BulkDataTransfer::kStatus_LengthTooShort                      : errorMsg = "Length too short"; break;
        case BulkDataTransfer::kStatus_XferFailedUnknownErr                : errorMsg = "Transfer failed for unknown reason"; break;
        case BulkDataTransfer::kStatus_XferMethodNotSupported              : errorMsg = "Transfer method not supported"; break;
        case BulkDataTransfer::kStatus_UnknownFile                         : errorMsg = "Unknown file"; break;
        case BulkDataTransfer::kStatus_StartOffsetNotSupported             : errorMsg = "Start offset not support"; break;
        case BulkDataTransfer::kStatus_Unknown                             : errorMsg = "Unknown error"; break;
#else
        case BulkDataTransfer::kStatus_Overflow                            : errorMsg = "Overflow"; break;
        case BulkDataTransfer::kStatus_LengthTooLarge                      : errorMsg = "Length too long"; break;
        case BulkDataTransfer::kStatus_LengthTooShort                      : errorMsg = "Length too short"; break;
        case BulkDataTransfer::kStatus_LengthMismatch                      : errorMsg = "Length mismatch"; break;
        case BulkDataTransfer::kStatus_LengthRequired                      : errorMsg = "Length required"; break;
        case BulkDataTransfer::kStatus_BadMessageContents                  : errorMsg = "Bad message contents"; break;
        case BulkDataTransfer::kStatus_BadBlockCounter                     : errorMsg = "Bad block counter"; break;
        case BulkDataTransfer::kStatus_XferFailedUnknownErr                : errorMsg = "Transfer failed for unknown reason"; break;
        case BulkDataTransfer::kStatus_ServerBadState                      : errorMsg = "Server is in incorrect state"; break;
        case BulkDataTransfer::kStatus_FailureToSend                       : errorMsg = "Failure to send"; break;
        case BulkDataTransfer::kStatus_XferMethodNotSupported              : errorMsg = "Transfer method not supported"; break;
        case BulkDataTransfer::kStatus_UnknownFile                         : errorMsg = "Unknown file"; break;
        case BulkDataTransfer::kStatus_StartOffsetNotSupported             : errorMsg = "Start offset not support"; break;
        case BulkDataTransfer::kStatus_VersionNotSupported                 : errorMsg = "Protocol version not supported"; break;
        case BulkDataTransfer::kStatus_Unknown                             : errorMsg = "Unknown error"; break;
#endif // WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Development
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_Common:
        profileName = "Common";
        switch (statusCode)
        {
        case nl::Weave::Profiles::Common::kStatus_Success                  : errorMsg = "Success"; break;
        case nl::Weave::Profiles::Common::kStatus_Canceled                 : errorMsg = "Canceled"; break;
        case nl::Weave::Profiles::Common::kStatus_BadRequest               : errorMsg = "Bad/malformed request"; break;
        case nl::Weave::Profiles::Common::kStatus_UnsupportedMessage       : errorMsg = "Unrecognized/unsupported message"; break;
        case nl::Weave::Profiles::Common::kStatus_UnexpectedMessage        : errorMsg = "Unexpected message"; break;
        case nl::Weave::Profiles::Common::kStatus_AuthenticationRequired   : errorMsg = "Authentication required"; break;
        case nl::Weave::Profiles::Common::kStatus_AccessDenied             : errorMsg = "Access denied"; break;
        case nl::Weave::Profiles::Common::kStatus_OutOfMemory              : errorMsg = "Out of memory"; break;
        case nl::Weave::Profiles::Common::kStatus_NotAvailable             : errorMsg = "Not available"; break;
        case nl::Weave::Profiles::Common::kStatus_LocalSetupRequired       : errorMsg = "Local setup required"; break;
        case nl::Weave::Profiles::Common::kStatus_Relocated                : errorMsg = "Relocated"; break;
        case nl::Weave::Profiles::Common::kStatus_Busy                     : errorMsg = "Sender busy"; break;
        case nl::Weave::Profiles::Common::kStatus_Timeout                  : errorMsg = "Timeout"; break;
        case nl::Weave::Profiles::Common::kStatus_InternalError            : errorMsg = "Internal error"; break;
        case nl::Weave::Profiles::Common::kStatus_Continue                 : errorMsg = "Continue"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_WDM:
        profileName = "WDM";
        switch (statusCode)
            {
        case DataManagement_Legacy::kStatus_CancelSuccess                  : errorMsg = "Subscription canceled"; break;
        case DataManagement_Legacy::kStatus_InvalidPath                    : errorMsg = "Invalid path"; break;
        case DataManagement_Legacy::kStatus_UnknownTopic                   : errorMsg = "Unknown topic"; break;
        case DataManagement_Legacy::kStatus_IllegalReadRequest             : errorMsg = "Illegal read request"; break;
        case DataManagement_Legacy::kStatus_IllegalWriteRequest            : errorMsg = "Illegal write request"; break;
        case DataManagement_Legacy::kStatus_InvalidVersion                 : errorMsg = "Invalid version"; break;
        case DataManagement_Legacy::kStatus_UnsupportedSubscriptionMode    : errorMsg = "Unsupported subscription mode"; break;

        case DataManagement_Current::kStatus_InvalidValueInNotification    : errorMsg = "Invalid value in notification"; break;
        case DataManagement_Current::kStatus_InvalidPath                   : errorMsg = "Invalid path"; break;
        case DataManagement_Current::kStatus_ExpiryTimeNotSupported        : errorMsg = "Expiry time not supported"; break;
        case DataManagement_Current::kStatus_NotTimeSyncedYet              : errorMsg = "Not time-synced yet"; break;
        case DataManagement_Current::kStatus_RequestExpiredInTime          : errorMsg = "Request expired in time"; break;
        case DataManagement_Current::kStatus_VersionMismatch               : errorMsg = "Version mismatch"; break;
        case DataManagement_Current::kStatus_GeneralProtocolError          : errorMsg = "General protocol error"; break;
        case DataManagement_Current::kStatus_SecurityError                 : errorMsg = "Security error"; break;
        case DataManagement_Current::kStatus_InvalidSubscriptionID         : errorMsg = "Invalid subscription ID"; break;
        case DataManagement_Current::kStatus_GeneralSchemaViolation        : errorMsg = "General schema violation"; break;
        case DataManagement_Current::kStatus_UnpairedDeviceRejected        : errorMsg = "Unpaired device rejected"; break;
        case DataManagement_Current::kStatus_IncompatibleDataSchemaVersion : errorMsg = "Incompatible data schema violation"; break;
        case DataManagement_Current::kStatus_MultipleFailures              : errorMsg = "Multiple failures"; break;
        case DataManagement_Current::kStatus_UpdateOutOfSequence           : errorMsg = "Update out of sequence"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_DeviceControl:
        profileName = "DeviceControl";
        switch (statusCode)
        {
        case DeviceControl::kStatusCode_FailSafeAlreadyActive              : errorMsg = "Fail-safe already active"; break;
        case DeviceControl::kStatusCode_NoFailSafeActive                   : errorMsg = "No fail-safe active"; break;
        case DeviceControl::kStatusCode_NoMatchingFailSafeActive           : errorMsg = "No matching fail-safe active"; break;
        case DeviceControl::kStatusCode_UnsupportedFailSafeMode            : errorMsg = "Unsupported fail-safe mode"; break;
        case DeviceControl::kStatusCode_RemotePassiveRendezvousTimedOut    : errorMsg = "Remote Passive Rendezvous timed out"; break;
        case DeviceControl::kStatusCode_UnsecuredListenPreempted           : errorMsg = "Unsecured Listen pre-empted"; break;
        case DeviceControl::kStatusCode_ResetSuccessCloseCon               : errorMsg = "ResetConfig will succeed after connection close"; break;
        case DeviceControl::kStatusCode_ResetNotAllowed                    : errorMsg = "Reset not allowed"; break;
        case DeviceControl::kStatusCode_NoSystemTestDelegate               : errorMsg = "System test cannot run without a delegate"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_DeviceDescription:
        profileName = "DeviceDescription";
        switch (statusCode)
        {
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_Echo:
        profileName = "Echo";
        switch (statusCode)
        {
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_FabricProvisioning:
        profileName = "FabricProvisioning";
        switch (statusCode)
        {
        case FabricProvisioning::kStatusCode_AlreadyMemberOfFabric         : errorMsg = "Already member of fabric"; break;
        case FabricProvisioning::kStatusCode_NotMemberOfFabric             : errorMsg = "Not member of fabric"; break;
        case FabricProvisioning::kStatusCode_InvalidFabricConfig           : errorMsg = "Invalid fabric config"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_NetworkProvisioning:
        profileName = "NetworkProvisioning";
        switch (statusCode)
        {
        case NetworkProvisioning::kStatusCode_UnknownNetwork               : errorMsg = "Unknown network"; break;
        case NetworkProvisioning::kStatusCode_TooManyNetworks              : errorMsg = "Too many networks"; break;
        case NetworkProvisioning::kStatusCode_InvalidNetworkConfiguration  : errorMsg = "Invalid network configuration"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedNetworkType       : errorMsg = "Unsupported network configuration"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiMode          : errorMsg = "Unsupported WiFi mode"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiRole          : errorMsg = "Unsupported WiFi role"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiSecurityType  : errorMsg = "Unsupported WiFi security type"; break;
        case NetworkProvisioning::kStatusCode_InvalidState                 : errorMsg = "Invalid state"; break;
        case NetworkProvisioning::kStatusCode_TestNetworkFailed            : errorMsg = "Test network failed"; break;
        case NetworkProvisioning::kStatusCode_NetworkConnectFailed         : errorMsg = "Network connect failed"; break;
        case NetworkProvisioning::kStatusCode_NoRouterAvailable            : errorMsg = "No router available"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedRegulatoryDomain  : errorMsg = "Unsupported wireless regulatory domain"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedOperatingLocation : errorMsg = "Unsupported wireless operating location"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_Security:
        profileName = "Security";
        switch (statusCode)
        {
        case Security::kStatusCode_SessionAborted                          : errorMsg = "Session aborted"; break;
        case Security::kStatusCode_PASESupportsOnlyConfig1                 : errorMsg = "PASE Engine only supports Config1"; break;
        case Security::kStatusCode_UnsupportedEncryptionType               : errorMsg = "Unsupported encryption type"; break;
        case Security::kStatusCode_InvalidKeyId                            : errorMsg = "Invalid key id"; break;
        case Security::kStatusCode_DuplicateKeyId                          : errorMsg = "Duplicate key id"; break;
        case Security::kStatusCode_KeyConfirmationFailed                   : errorMsg = "Key confirmation failed"; break;
        case Security::kStatusCode_InternalError                           : errorMsg = "Internal error"; break;
        case Security::kStatusCode_AuthenticationFailed                    : errorMsg = "Authentication failed"; break;
        case Security::kStatusCode_UnsupportedCASEConfiguration            : errorMsg = "Unsupported CASE configuration"; break;
        case Security::kStatusCode_UnsupportedCertificate                  : errorMsg = "Unsupported certificate"; break;
        case Security::kStatusCode_NoCommonPASEConfigurations              : errorMsg = "No supported PASE configurations in common"; break;
        case Security::kStatusCode_KeyNotFound                             : errorMsg = "Key not found"; break;
        case Security::kStatusCode_WrongEncryptionType                     : errorMsg = "Wrong encryption type"; break;
        case Security::kStatusCode_UnknownKeyType                          : errorMsg = "Unknown key type"; break;
        case Security::kStatusCode_InvalidUseOfSessionKey                  : errorMsg = "Invalid use of session key"; break;
        case Security::kStatusCode_InternalKeyError                        : errorMsg = "Internal key error"; break;
        case Security::kStatusCode_NoCommonKeyExportConfiguration          : errorMsg = "No common key export configuration"; break;
        case Security::kStatusCode_UnauthorizedKeyExportRequest            : errorMsg = "Unauthorized key export request"; break;
        case Security::kStatusCode_NoNewOperationalCertRequired            : errorMsg = "No new operational certificate required"; break;
        case Security::kStatusCode_OperationalNodeIdInUse                  : errorMsg = "Operational node Id collision"; break;
        case Security::kStatusCode_InvalidOperationalNodeId                : errorMsg = "Invalid operational node Id"; break;
        case Security::kStatusCode_InvalidOperationalCertificate           : errorMsg = "Invalid operational certificate"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kWeaveProfile_ServiceDirectory:
        profileName = "ServiceDirectory";
        switch (statusCode)
        {
        case ServiceDirectory::kStatus_DirectoryUnavailable                : errorMsg = "Service directory unavailable"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;
#endif
    case kWeaveProfile_ServiceProvisioning:
        profileName = "ServiceProvisioning";
        switch (statusCode)
        {
        case ServiceProvisioning::kStatusCode_TooManyServices              : errorMsg = "Too many services"; break;
        case ServiceProvisioning::kStatusCode_ServiceAlreadyRegistered     : errorMsg = "Service already registered"; break;
        case ServiceProvisioning::kStatusCode_InvalidServiceConfig         : errorMsg = "Invalid service configuration"; break;
        case ServiceProvisioning::kStatusCode_NoSuchService                : errorMsg = "No such service"; break;
        case ServiceProvisioning::kStatusCode_PairingServerError           : errorMsg = "Error talking to pairing server"; break;
        case ServiceProvisioning::kStatusCode_InvalidPairingToken          : errorMsg = "Invalid pairing token"; break;
        case ServiceProvisioning::kStatusCode_PairingTokenOld              : errorMsg = "Pairing token no longer valid"; break;
        case ServiceProvisioning::kStatusCode_ServiceCommunicationError    : errorMsg = "Service communication error"; break;
        case ServiceProvisioning::kStatusCode_ServiceConfigTooLarge        : errorMsg = "Service configuration too large"; break;
        case ServiceProvisioning::kStatusCode_WrongFabric                  : errorMsg = "Wrong fabric"; break;
        case ServiceProvisioning::kStatusCode_TooManyFabrics               : errorMsg = "Too many fabrics"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;

    case kWeaveProfile_SWU:
        profileName = "SWU";
        switch (statusCode)
        {
        case SoftwareUpdate::kStatus_NoUpdateAvailable                     : errorMsg = "No software update available"; break;
        case SoftwareUpdate::kStatus_UpdateFailed                          : errorMsg = "Software update failed"; break;
        case SoftwareUpdate::kStatus_InvalidInstructions                   : errorMsg = "Invalid software image download instructions"; break;
        case SoftwareUpdate::kStatus_DownloadFailed                        : errorMsg = "Software image download failed"; break;
        case SoftwareUpdate::kStatus_IntegrityCheckFailed                  : errorMsg = "Software image integrity check failed"; break;
        case SoftwareUpdate::kStatus_Abort                                 : errorMsg = "Software image query aborted"; break;
        case SoftwareUpdate::kStatus_Retry                                 : errorMsg = "Retry software image query"; break;
        default                                                            : errorMsg = ""; break;
        }
        break;
    case kWeaveProfile_Tunneling:
        profileName = "WeaveTunnel";
        switch (statusCode)
	{
#if WEAVE_CONFIG_ENABLE_TUNNELING
	case WeaveTunnel::kStatusCode_TunnelOpenFail                       : errorMsg = "Tunnel open failed"; break;
	case WeaveTunnel::kStatusCode_TunnelCloseFail                      : errorMsg = "Tunnel close failed"; break;
	case WeaveTunnel::kStatusCode_TunnelRouteUpdateFail                : errorMsg = "Tunnel route update failed"; break;
	case WeaveTunnel::kStatusCode_TunnelReconnectFail                  : errorMsg = "Tunnel reconnect failed"; break;
#endif
	default                                                            : errorMsg = ""; break;
	}
        break;

    case kWeaveProfile_StatusReport_Deprecated:
        profileName = "Security";
        switch (statusCode)
        {
        default                                                            : errorMsg = ""; break;
        }
        break;

    default:
        fmt = FindStatusReportStr(profileId, statusCode);
        break;

    }

    if (profileName != NULL)
    {
        snprintf(sErrorStr, sizeof(sErrorStr) - 2, fmt, profileName, profileId, statusCode, errorMsg);
    }
    else
    {
        if (fmt == NULL)
            fmt = "[ %08" PRIX32 ":%" PRIu16 " ]";

        snprintf(sErrorStr, sizeof(sErrorStr) - 2, fmt, profileId, statusCode);
    }

    sErrorStr[sizeof(sErrorStr) - 1] = 0;
    return sErrorStr;
}
#endif // #if WEAVE_CONFIG_SHORT_ERROR_STR

#if !WEAVE_CONFIG_SHORT_ERROR_STR
static const char *FindStatusReportStr(uint32_t inProfileId, uint16_t inStatusCode)
{
    const Weave::Support::ProfileStringInfo *info;
    const char *result = NULL;

    info = Weave::Support::FindProfileStringInfo(inProfileId);

    if (info != NULL && info->mStatusReportFormatStringFunct != NULL)
    {
        result = info->mStatusReportFormatStringFunct(inProfileId, inStatusCode);
    }

    return (result);
}
#endif // #if !WEAVE_CONFIG_SHORT_ERROR_STR

} // namespace nl

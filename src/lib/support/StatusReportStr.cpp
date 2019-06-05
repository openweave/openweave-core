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
    const char *fmt = NULL;

    switch (profileId)
    {
    case kWeaveProfile_BDX:
        switch (statusCode)
        {
#if WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Development
        case BulkDataTransfer::kStatus_Overflow                                         : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Overflow"; break;
        case BulkDataTransfer::kStatus_LengthTooShort                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Length too short"; break;
        case BulkDataTransfer::kStatus_XferFailedUnknownErr                             : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Transfer failed for unknown reason"; break;
        case BulkDataTransfer::kStatus_XferMethodNotSupported                           : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Transfer method not supported"; break;
        case BulkDataTransfer::kStatus_UnknownFile                                      : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Unknown file"; break;
        case BulkDataTransfer::kStatus_StartOffsetNotSupported                          : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Start offset not support"; break;
        case BulkDataTransfer::kStatus_Unknown                                          : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Unknown error"; break;
#else
        case BulkDataTransfer::kStatus_Overflow                                         : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Overflow"; break;
        case BulkDataTransfer::kStatus_LengthTooLarge                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Length too long"; break;
        case BulkDataTransfer::kStatus_LengthTooShort                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Length too short"; break;
        case BulkDataTransfer::kStatus_LengthMismatch                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Length mismatch"; break;
        case BulkDataTransfer::kStatus_LengthRequired                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Length required"; break;
        case BulkDataTransfer::kStatus_BadMessageContents                               : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Bad message contents"; break;
        case BulkDataTransfer::kStatus_BadBlockCounter                                  : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Bad block counter"; break;
        case BulkDataTransfer::kStatus_XferFailedUnknownErr                             : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Transfer failed for unknown reason"; break;
        case BulkDataTransfer::kStatus_ServerBadState                                   : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Server is in incorrect state"; break;
        case BulkDataTransfer::kStatus_FailureToSend                                    : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Failure to send"; break;
        case BulkDataTransfer::kStatus_XferMethodNotSupported                           : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Transfer method not supported"; break;
        case BulkDataTransfer::kStatus_UnknownFile                                      : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Unknown file"; break;
        case BulkDataTransfer::kStatus_StartOffsetNotSupported                          : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Start offset not support"; break;
        case BulkDataTransfer::kStatus_VersionNotSupported                              : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Protocol version not supported"; break;
        case BulkDataTransfer::kStatus_Unknown                                          : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ] Unknown error"; break;
#endif // WEAVE_CONFIG_BDX_NAMESPACE == kWeaveManagedNamespace_Development
        default                                                                         : fmt = "[ BDX(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_Common:
        switch (statusCode)
        {
        case nl::Weave::Profiles::Common::kStatus_Success                               : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Success"; break;
        case nl::Weave::Profiles::Common::kStatus_BadRequest                            : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Bad/malformed request"; break;
        case nl::Weave::Profiles::Common::kStatus_UnsupportedMessage                    : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Unrecognized/unsupported message"; break;
        case nl::Weave::Profiles::Common::kStatus_UnexpectedMessage                     : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Unexpected message"; break;
        case nl::Weave::Profiles::Common::kStatus_AuthenticationRequired                : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Authentication required"; break;
        case nl::Weave::Profiles::Common::kStatus_AccessDenied                          : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Access denied"; break;
        case nl::Weave::Profiles::Common::kStatus_OutOfMemory                           : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Out of memory"; break;
        case nl::Weave::Profiles::Common::kStatus_NotAvailable                          : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Not available"; break;
        case nl::Weave::Profiles::Common::kStatus_LocalSetupRequired                    : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Local setup required"; break;
        case nl::Weave::Profiles::Common::kStatus_Relocated                             : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Relocated"; break;
        case nl::Weave::Profiles::Common::kStatus_Busy                                  : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Sender busy"; break;
        case nl::Weave::Profiles::Common::kStatus_Timeout                               : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Timeout"; break;
        case nl::Weave::Profiles::Common::kStatus_InternalError                         : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Internal error"; break;
        case nl::Weave::Profiles::Common::kStatus_Continue                              : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ] Continue"; break;
        default                                                                         : fmt = "[ Common(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_WDM:
        switch (statusCode)
            {
        case DataManagement_Legacy::kStatus_CancelSuccess                               : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Subscription canceled"; break;
        case DataManagement_Legacy::kStatus_InvalidPath                                 : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Invalid path"; break;
        case DataManagement_Legacy::kStatus_UnknownTopic                                : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Unknown topic"; break;
        case DataManagement_Legacy::kStatus_IllegalReadRequest                          : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Illegal read request"; break;
        case DataManagement_Legacy::kStatus_IllegalWriteRequest                         : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Illegal write request"; break;
        case DataManagement_Legacy::kStatus_InvalidVersion                              : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Invalid version"; break;
        case DataManagement_Legacy::kStatus_UnsupportedSubscriptionMode                 : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Unsupported subscription mode"; break;

        case DataManagement_Current::kStatus_InvalidValueInNotification                 : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Invalid value in notification"; break;
        case DataManagement_Current::kStatus_InvalidPath                                : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Invalid path"; break;
        case DataManagement_Current::kStatus_ExpiryTimeNotSupported                     : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Expiry time not supported"; break;
        case DataManagement_Current::kStatus_NotTimeSyncedYet                           : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Not time-synced yet"; break;
        case DataManagement_Current::kStatus_RequestExpiredInTime                       : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Request expired in time"; break;
        case DataManagement_Current::kStatus_VersionMismatch                            : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Version mismatch"; break;
        case DataManagement_Current::kStatus_GeneralProtocolError                       : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] General protocol error"; break;
        case DataManagement_Current::kStatus_SecurityError                              : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Security error"; break;
        case DataManagement_Current::kStatus_InvalidSubscriptionID                      : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Invalid subscription ID"; break;
        case DataManagement_Current::kStatus_GeneralSchemaViolation                     : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] General schema violation"; break;
        case DataManagement_Current::kStatus_UnpairedDeviceRejected                     : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Unpaired device rejected"; break;
        case DataManagement_Current::kStatus_IncompatibleDataSchemaVersion              : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Incompatible data schema violation"; break;
        case DataManagement_Current::kStatus_MultipleFailures                           : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Multiple failures"; break;
        case DataManagement_Current::kStatus_UpdateOutOfSequence                        : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ] Update out of sequence"; break;
        default                                                                         : fmt = "[ WDM(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_DeviceControl:
        switch (statusCode)
        {
        case DeviceControl::kStatusCode_FailSafeAlreadyActive                           : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] Fail-safe already active"; break;
        case DeviceControl::kStatusCode_NoFailSafeActive                                : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] No fail-safe active"; break;
        case DeviceControl::kStatusCode_NoMatchingFailSafeActive                        : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] No matching fail-safe active"; break;
        case DeviceControl::kStatusCode_UnsupportedFailSafeMode                         : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] Unsupported fail-safe mode"; break;
        case DeviceControl::kStatusCode_RemotePassiveRendezvousTimedOut                 : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] Remote Passive Rendezvous timed out"; break;
        case DeviceControl::kStatusCode_UnsecuredListenPreempted                        : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] Unsecured Listen pre-empted"; break;
        case DeviceControl::kStatusCode_ResetSuccessCloseCon                            : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] ResetConfig will succeed after connection close"; break;
        case DeviceControl::kStatusCode_ResetNotAllowed                                 : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] Reset not allowed"; break;
        case DeviceControl::kStatusCode_NoSystemTestDelegate                            : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ] System test cannot run without a delegate"; break;
        default                                                                         : fmt = "[ DeviceControl(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_DeviceDescription:
        switch (statusCode)
        {
        default                                                                         : fmt = "[ DeviceDescription(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_Echo:
        switch (statusCode)
        {
        default                                                                         : fmt = "[ Echo(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_FabricProvisioning:
        switch (statusCode)
        {
        case FabricProvisioning::kStatusCode_AlreadyMemberOfFabric                      : fmt = "[ FabricProvisioning(%08" PRIX32 "):%" PRIu16 " ] Already member of fabric"; break;
        case FabricProvisioning::kStatusCode_NotMemberOfFabric                          : fmt = "[ FabricProvisioning(%08" PRIX32 "):%" PRIu16 " ] Not member of fabric"; break;
        case FabricProvisioning::kStatusCode_InvalidFabricConfig                        : fmt = "[ FabricProvisioning(%08" PRIX32 "):%" PRIu16 " ] Invalid fabric config"; break;
        default                                                                         : fmt = "[ FabricProvisioning(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_NetworkProvisioning:
        switch (statusCode)
        {
        case NetworkProvisioning::kStatusCode_UnknownNetwork                            : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Unknown network"; break;
        case NetworkProvisioning::kStatusCode_TooManyNetworks                           : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Too many networks"; break;
        case NetworkProvisioning::kStatusCode_InvalidNetworkConfiguration               : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Invalid network configuration"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedNetworkType                    : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Unsupported network configuration"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiMode                       : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Unsupported WiFi mode"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiRole                       : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Unsupported WiFi role"; break;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiSecurityType               : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Unsupported WiFi security type"; break;
        case NetworkProvisioning::kStatusCode_InvalidState                              : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Invalid state"; break;
        case NetworkProvisioning::kStatusCode_TestNetworkFailed                         : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Test network failed"; break;
        case NetworkProvisioning::kStatusCode_NetworkConnectFailed                      : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] Network connect failed"; break;
        case NetworkProvisioning::kStatusCode_NoRouterAvailable                         : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ] No router available"; break;
        default                                                                         : fmt = "[ NetworkProvisioning(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_Security:
        switch (statusCode)
        {
        case Security::kStatusCode_SessionAborted                                       : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Session aborted"; break;
        case Security::kStatusCode_PASESupportsOnlyConfig1                              : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] PASE Engine only supports Config1"; break;
        case Security::kStatusCode_UnsupportedEncryptionType                            : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unsupported encryption type"; break;
        case Security::kStatusCode_InvalidKeyId                                         : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Invalid key id"; break;
        case Security::kStatusCode_DuplicateKeyId                                       : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Duplicate key id"; break;
        case Security::kStatusCode_KeyConfirmationFailed                                : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Key confirmation failed"; break;
        case Security::kStatusCode_InternalError                                        : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Internal error"; break;
        case Security::kStatusCode_AuthenticationFailed                                 : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Authentication failed"; break;
        case Security::kStatusCode_UnsupportedCASEConfiguration                         : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unsupported CASE configuration"; break;
        case Security::kStatusCode_UnsupportedCertificate                               : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unsupported certificate"; break;
        case Security::kStatusCode_NoCommonPASEConfigurations                           : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] No supported PASE configurations in common"; break;
        case Security::kStatusCode_KeyNotFound                                          : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Key not found"; break;
        case Security::kStatusCode_WrongEncryptionType                                  : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Wrong encryption type"; break;
        case Security::kStatusCode_UnknownKeyType                                       : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unknown key type"; break;
        case Security::kStatusCode_InvalidUseOfSessionKey                               : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Invalid use of session key"; break;
        case Security::kStatusCode_InternalKeyError                                     : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Internal key error"; break;
        case Security::kStatusCode_NoCommonKeyExportConfiguration                       : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] No common key export configuration"; break;
        case Security::kStatusCode_UnathorizedKeyExportRequest                          : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unauthorized key export request"; break;
        case Security::kStatusCode_UnathorizedGetCertRequest                            : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] Unauthorized get certificate request"; break;
        case Security::kStatusCode_NoNewCertRequired                                    : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ] No new certificate required"; break;
        default                                                                         : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case kWeaveProfile_ServiceDirectory:
        switch (statusCode)
        {
        case ServiceDirectory::kStatus_DirectoryUnavailable                             : fmt = "[ ServiceDirectory(%08" PRIX32 "):%" PRIu16 " ] Service directory unavailable"; break;
        default                                                                         : fmt = "[ ServiceDirectory(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;
#endif
    case kWeaveProfile_ServiceProvisioning:
        switch (statusCode)
        {
        case ServiceProvisioning::kStatusCode_TooManyServices                           : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Too many services"; break;
        case ServiceProvisioning::kStatusCode_ServiceAlreadyRegistered                  : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Service already registered"; break;
        case ServiceProvisioning::kStatusCode_InvalidServiceConfig                      : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Invalid service configuration"; break;
        case ServiceProvisioning::kStatusCode_NoSuchService                             : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] No such service"; break;
        case ServiceProvisioning::kStatusCode_PairingServerError                        : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Error talking to pairing server"; break;
        case ServiceProvisioning::kStatusCode_InvalidPairingToken                       : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Invalid pairing token"; break;
        case ServiceProvisioning::kStatusCode_PairingTokenOld                           : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Pairing token no longer valid"; break;
        case ServiceProvisioning::kStatusCode_ServiceCommunicationError                 : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Service communication error"; break;
        case ServiceProvisioning::kStatusCode_ServiceConfigTooLarge                     : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Service configuration too large"; break;
        case ServiceProvisioning::kStatusCode_WrongFabric                               : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Wrong fabric"; break;
        case ServiceProvisioning::kStatusCode_TooManyFabrics                            : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ] Too many fabrics"; break;
        default                                                                         : fmt = "[ ServiceProvisioning(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    case kWeaveProfile_SWU:
        switch (statusCode)
        {
        case SoftwareUpdate::kStatus_NoUpdateAvailable                                  : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] No software update available"; break;
        case SoftwareUpdate::kStatus_UpdateFailed                                       : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Software update failed"; break;
        case SoftwareUpdate::kStatus_InvalidInstructions                                : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Invalid software image download instructions"; break;
        case SoftwareUpdate::kStatus_DownloadFailed                                     : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Software image download failed"; break;
        case SoftwareUpdate::kStatus_IntegrityCheckFailed                               : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Software image integrity check failed"; break;
        case SoftwareUpdate::kStatus_Abort                                              : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Software image query aborted"; break;
        case SoftwareUpdate::kStatus_Retry                                              : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ] Retry software image query"; break;
        default                                                                         : fmt = "[ SWU(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;
    case kWeaveProfile_Tunneling:
        switch (statusCode)
	{
#if WEAVE_CONFIG_ENABLE_TUNNELING
	case WeaveTunnel::kStatusCode_TunnelOpenFail                                    : fmt = "[ WeaveTunnel(%08" PRIX32 "):%" PRIu16 " ] Tunnel open failed"; break;
	case WeaveTunnel::kStatusCode_TunnelCloseFail                                   : fmt = "[ WeaveTunnel(%08" PRIX32 "):%" PRIu16 " ] Tunnel close failed"; break;
	case WeaveTunnel::kStatusCode_TunnelRouteUpdateFail                             : fmt = "[ WeaveTunnel(%08" PRIX32 "):%" PRIu16 " ] Tunnel route update failed"; break;
	case WeaveTunnel::kStatusCode_TunnelReconnectFail                               : fmt = "[ WeaveTunnel(%08" PRIX32 "):%" PRIu16 " ] Tunnel reconnect failed"; break;
#endif
	default                                                                         : fmt = "[ WeaveTunnel(%08" PRIX32 "):%" PRIu16 " ]"; break;
	}
        break;

    case kWeaveProfile_StatusReport_Deprecated:
        switch (statusCode)
        {
        default                                                                         : fmt = "[ Security(%08" PRIX32 "):%" PRIu16 " ]"; break;
        }
        break;

    default:
        fmt = FindStatusReportStr(profileId, statusCode);
        break;

    }

    if (fmt == NULL)
        fmt = "[ %08" PRIX32 ":%" PRIu16 " ]";

    snprintf(sErrorStr, sizeof(sErrorStr) - 2, fmt, profileId, statusCode);
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

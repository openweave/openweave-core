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
 *      Objective-C representation of a Status Report from the common profile.
 *
 */

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#import "NLProfileStatusError.h"
#import "NLWeaveDeviceManager.h"

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/bulk-data-transfer/BulkDataTransfer.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>

using namespace nl::Weave::Profiles;

@interface NLProfileStatusError()
{
    NSString * _statusReport;
}

@end

@implementation NLProfileStatusError

- (id)initWithProfileId:(uint32_t)profileId statusCode:(uint16_t)statusCode errorCode:(uint32_t)errorCode statusReport:(NSString*)statusReport
{
    if (self = [super init])
    {
        _profileId = profileId;
        _statusCode = statusCode;
        _errorCode = errorCode;
        _statusReport = statusReport;
    }
    return self;
}

- (NSInteger)translateErrorCode
{
    switch (self.profileId)
    {

    case nl::Weave::Profiles::kWeaveProfile_BDX:
        switch (self.statusCode)
        {
        case BulkDataTransfer::kStatus_Overflow                                         : return kNLStatus_Overflow;
        case BulkDataTransfer::kStatus_LengthTooShort                                   : return kNLStatus_LengthTooShort;
        case BulkDataTransfer::kStatus_XferFailedUnknownErr                             : return kNLStatus_XferFailedUnknownErr;
        case BulkDataTransfer::kStatus_XferMethodNotSupported                           : return kNLStatus_XferMethodNotSupported;
        case BulkDataTransfer::kStatus_UnknownFile                                      : return kNLStatus_UnknownFile;
        case BulkDataTransfer::kStatus_StartOffsetNotSupported                          : return kNLStatus_StartOffsetNotSupported;
        case BulkDataTransfer::kStatus_Unknown                                          : return kNLStatus_Unknown;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_Common:
        switch (self.statusCode)
        {
        case nl::Weave::Profiles::Common::kStatus_Success                               : return kNLStatus_Success;
        case nl::Weave::Profiles::Common::kStatus_BadRequest                            : return kNLStatus_BadRequest;
        case nl::Weave::Profiles::Common::kStatus_UnsupportedMessage                    : return kNLStatus_UnsupportedMessage;
        case nl::Weave::Profiles::Common::kStatus_UnexpectedMessage                     : return kNLStatus_UnexpectedMessage;
        case nl::Weave::Profiles::Common::kStatus_OutOfMemory                           : return kNLStatus_OutOfMemory;
        case nl::Weave::Profiles::Common::kStatus_Relocated                             : return kNLStatus_Relocated;
        case nl::Weave::Profiles::Common::kStatus_Busy                                  : return kNLStatus_Busy;
        case nl::Weave::Profiles::Common::kStatus_Timeout                               : return kNLStatus_Timeout;
        case nl::Weave::Profiles::Common::kStatus_InternalError                         : return kNLStatus_InternalError;
        case nl::Weave::Profiles::Common::kStatus_Continue                              : return kNLStatus_Continue;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_WDM:
        switch (self.statusCode)
        {
        case DataManagement_Legacy::kStatus_InvalidPath                                  : return kNLStatus_InvalidPath;
        case DataManagement_Legacy::kStatus_UnknownTopic                                 : return kNLStatus_UnknownTopic;
        case DataManagement_Legacy::kStatus_IllegalReadRequest                           : return kNLStatus_IllegalReadRequest;
        case DataManagement_Legacy::kStatus_IllegalWriteRequest                          : return kNLStatus_IllegalWriteRequest;
        default                                                                          : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_DeviceControl:
        switch (self.statusCode)
        {
        case DeviceControl::kStatusCode_FailSafeAlreadyActive                           : return kNLStatusCode_FailSafeAlreadyActive;
        case DeviceControl::kStatusCode_NoFailSafeActive                                : return kNLStatusCode_NoFailSafeActive;
        case DeviceControl::kStatusCode_NoMatchingFailSafeActive                        : return kNLStatusCode_NoMatchingFailSafeActive;
        case DeviceControl::kStatusCode_UnsupportedFailSafeMode                         : return kNLStatusCode_UnsupportedFailSafeMode;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_DeviceDescription:
        switch (self.statusCode)
        {
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_Echo:
        switch (self.statusCode)
        {
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_FabricProvisioning:
        switch (self.statusCode)
        {
        case FabricProvisioning::kStatusCode_AlreadyMemberOfFabric                      : return kNLStatusCode_AlreadyMemberOfFabric;
        case FabricProvisioning::kStatusCode_NotMemberOfFabric                          : return kNLStatusCode_NotMemberOfFabric;
        case FabricProvisioning::kStatusCode_InvalidFabricConfig                        : return kNLStatusCode_InvalidFabricConfig;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_NetworkProvisioning:
        switch (self.statusCode)
        {
        case NetworkProvisioning::kStatusCode_UnknownNetwork                            : return kNLStatusCode_UnknownNetwork;
        case NetworkProvisioning::kStatusCode_TooManyNetworks                           : return kNLStatusCode_TooManyNetworks;
        case NetworkProvisioning::kStatusCode_InvalidNetworkConfiguration               : return kNLStatusCode_InvalidNetworkConfiguration;
        case NetworkProvisioning::kStatusCode_UnsupportedNetworkType                    : return kNLStatusCode_UnsupportedNetworkType;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiMode                       : return kNLStatusCode_UnsupportedWiFiMode;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiRole                       : return kNLStatusCode_UnsupportedWiFiRole;
        case NetworkProvisioning::kStatusCode_UnsupportedWiFiSecurityType               : return kNLStatusCode_UnsupportedWiFiSecurityType;
        case NetworkProvisioning::kStatusCode_InvalidState                              : return kNLStatusCode_InvalidState;
        case NetworkProvisioning::kStatusCode_TestNetworkFailed                         : return kNLStatusCode_TestNetworkFailed;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_Security:
        switch (self.statusCode)
        {
        case Security::kStatusCode_SessionAborted                                       : return kNLStatusCode_SessionAborted;
        case Security::kStatusCode_PASESupportsOnlyConfig1                              : return kNLStatusCode_PASESupportsOnlyConfig1;
        case Security::kStatusCode_NoCommonPASEConfigurations                           : return kNLStatusCode_NoCommonPASEConfigurations;
        case Security::kStatusCode_UnsupportedEncryptionType                            : return kNLStatusCode_UnsupportedEncryptionType;
        case Security::kStatusCode_InvalidKeyId                                         : return kNLStatusCode_InvalidKeyId;
        case Security::kStatusCode_DuplicateKeyId                                       : return kNLStatusCode_DuplicateKeyId;
        case Security::kStatusCode_KeyConfirmationFailed                                : return kNLStatusCode_KeyConfirmationFailed;
        case Security::kStatusCode_InternalError                                        : return kNLStatusCode_InternalError;
        default                                                                         : return self.statusCode;
        }
        break;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    case nl::Weave::Profiles::kWeaveProfile_ServiceDirectory:
        switch (self.statusCode)
        {
        case ServiceDirectory::kStatus_DirectoryUnavailable                             : return kNLStatus_DirectoryUnavailable;
        default                                                                         : return self.statusCode;
        }
        break;
#endif

    case nl::Weave::Profiles::kWeaveProfile_ServiceProvisioning:
        switch (self.statusCode)
        {
        case ServiceProvisioning::kStatusCode_TooManyServices                           : return kNLStatusCode_TooManyServices;
        case ServiceProvisioning::kStatusCode_ServiceAlreadyRegistered                  : return kNLStatusCode_ServiceAlreadyRegistered;
        case ServiceProvisioning::kStatusCode_InvalidServiceConfig                      : return kNLStatusCode_InvalidServiceConfig;
        case ServiceProvisioning::kStatusCode_NoSuchService                             : return kNLStatusCode_NoSuchService;
        case ServiceProvisioning::kStatusCode_PairingServerError                        : return kNLStatusCode_PairingServerError;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_SWU:
        switch (self.statusCode)
        {
        case SoftwareUpdate::kStatus_NoUpdateAvailable                                  : return kNLStatus_NoUpdateAvailable;
        case SoftwareUpdate::kStatus_UpdateFailed                                       : return kNLStatus_UpdateFailed;
        case SoftwareUpdate::kStatus_InvalidInstructions                                : return kNLStatus_InvalidInstructions;
        case SoftwareUpdate::kStatus_DownloadFailed                                     : return kNLStatus_DownloadFailed;
        case SoftwareUpdate::kStatus_IntegrityCheckFailed                               : return kNLStatus_IntegrityCheckFailed;
        case SoftwareUpdate::kStatus_Abort                                              : return kNLStatus_Abort;
        case SoftwareUpdate::kStatus_Retry                                              : return kNLStatus_Retry;
        default                                                                         : return self.statusCode;
        }
        break;

    case nl::Weave::Profiles::kWeaveProfile_StatusReport_Deprecated:
        switch (self.statusCode)
        {
        default                                                                         : return self.statusCode;
        }
        break;

    default                                                                             : return self.statusCode;
    }
}

- (NSString *)description
{
    NSString *emptyStatusReport = [NSString stringWithFormat:@"No StatusReport available. profileId = %ld, statusCode = %ld, SysErrorCode = %ld", (long)self.profileId, (long)self.statusCode, (long)self.errorCode];

    return _statusReport ? ([NSString stringWithFormat:@"%@, SysErrorCode = %ld", _statusReport, (long)self.errorCode]) : emptyStatusReport;
}

@end

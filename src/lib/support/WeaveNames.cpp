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
 *      This file defines functions for converting various ids into human-readable strings.
 *
 */

#include <stdlib.h>

// Note that the choice of namespace alias must be made up front for each and every compile unit
// This is because many include paths could set the default alias to unintended target.
#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/Profiles/bulk-data-transfer/BulkDataTransfer.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXConstants.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/heartbeat/WeaveHeartbeat.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>
#include <Weave/Support/ProfileStringSupport.hpp>

namespace nl {
namespace Weave {

using namespace nl::Weave::Profiles;

const char *GetVendorName(uint16_t vendorId)
{
    switch (vendorId) {
    case kWeaveVendor_Common                                                : return "Common";
    case kWeaveVendor_NestLabs                                              : return "Nest";
    case kWeaveVendor_Yale                                                  : return "Yale";
    case kWeaveVendor_Google                                                : return "Google";
    }

    return NULL;
}

static const char *FindProfileName(uint32_t inProfileId)
{
    const Support::ProfileStringInfo *info;
    const char *result = NULL;

    info = Support::FindProfileStringInfo(inProfileId);

    if (info != NULL && info->mProfileNameFunct != NULL)
    {
        result = info->mProfileNameFunct(inProfileId);
    }

    return (result);
}

static const char *FindMessageName(uint32_t inProfileId, uint8_t inMsgType)
{
    const Support::ProfileStringInfo *info;
    const char *result = NULL;

    info = Support::FindProfileStringInfo(inProfileId);

    if (info != NULL && info->mMessageNameFunct != NULL)
    {
        result = info->mMessageNameFunct(inProfileId, inMsgType);
    }

    return (result);
}

const char *GetProfileName(uint32_t profileId)
{
    const char *result = NULL;

    switch (profileId) {
    case kWeaveProfile_Common                                               : return "Common";
    case kWeaveProfile_Echo                                                 : return "Echo";
    case kWeaveProfile_NetworkProvisioning                                  : return "NetworkProvisioning";
    case kWeaveProfile_Security                                             : return "Security";
    case kWeaveProfile_FabricProvisioning                                   : return "FabricProvisioning";
    case kWeaveProfile_DeviceControl                                        : return "DeviceControl";
    case kWeaveProfile_Time                                                 : return "Time";
    case kWeaveProfile_WDM                                                  : return "WDM";
    case kWeaveProfile_SWU                                                  : return "SWU";
    case kWeaveProfile_BDX                                                  : return "BDX";
    case kWeaveProfile_DeviceDescription                                    : return "DeviceDescription";
    case kWeaveProfile_ServiceProvisioning                                  : return "ServiceProvisioning";
    case kWeaveProfile_ServiceDirectory                                     : return "ServiceDirectory";
    case kWeaveProfile_Locale                                               : return "Locale";
    case kWeaveProfile_Tunneling                                            : return "Tunneling";
    case kWeaveProfile_Heartbeat                                            : return "Heartbeat";
    case kWeaveProfile_TokenPairing                                         : return "TokenPairing";
    case kWeaveProfile_DictionaryKey                                        : return "DictionaryKey";

    case kWeaveProfile_Occupancy                                            : return "Nest:Occupancy";
    case kWeaveProfile_Structure                                            : return "Nest:Structure";
    case kWeaveProfile_NestProtect                                          : return "Nest:Protect";
    case kWeaveProfile_TimeVariantData                                      : return "Nest:TimeVariantData";
    case kWeaveProfile_HeatLink                                             : return "Nest:HeatLink";
    case kWeaveProfile_Safety                                               : return "Nest:Safety";
    case kWeaveProfile_SafetySummary                                        : return "Nest:SafetySummary";
    case kWeaveProfile_NestThermostat                                       : return "Nest:Thermostat";
    case kWeaveProfile_NestBoiler                                           : return "Nest:Boiler";
    case kWeaveProfile_NestHvacEquipmentControl                             : return "Nest:HvacEquipmentControl";
    case kWeaveProfile_NestDomesticHotWater                                 : return "Nest:DomesticHotWater";
    case kWeaveProfile_TopazHistory                                         : return "Nest:TopazHistory";
    case kWeaveProfile_NestNetworkManager                                   : return "Nest:NetworkManager";
    }

    result = FindProfileName(profileId);

    return (result);
}

const char *GetMessageName(uint32_t profileId, uint8_t msgType)
{
    const char *result = NULL;

    switch (profileId) {
    case kWeaveProfile_Common:
        switch (msgType) {
        case Common::kMsgType_StatusReport                                  : return "StatusReport";
        case Common::kMsgType_Null                                          : return "Null";
        case Common::kMsgType_WRMP_Delayed_Delivery                         : return "DelayedDelivery";
        case Common::kMsgType_WRMP_Throttle_Flow                            : return "ThrottleFlow";
        }
        break;
    case kWeaveProfile_Echo:
        switch (msgType) {
        case kEchoMessageType_EchoRequest                                   : return "EchoRequest";
        case kEchoMessageType_EchoResponse                                  : return "EchoResponse";
        }
        break;
    case kWeaveProfile_NetworkProvisioning:
        switch (msgType) {
        case NetworkProvisioning::kMsgType_ScanNetworks                     : return "ScanNetworks";
        case NetworkProvisioning::kMsgType_NetworkScanComplete              : return "NetworkScanComplete";
#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
        case NetworkProvisioning::kMsgType_AddNetwork                       : return "AddNetwork";
#endif
        case NetworkProvisioning::kMsgType_AddNetworkComplete               : return "AddNetworkComplete";
        case NetworkProvisioning::kMsgType_UpdateNetwork                    : return "UpdateNetwork";
        case NetworkProvisioning::kMsgType_RemoveNetwork                    : return "RemoveNetwork";
        case NetworkProvisioning::kMsgType_EnableNetwork                    : return "EnableNetwork";
        case NetworkProvisioning::kMsgType_DisableNetwork                   : return "DisableNetwork";
        case NetworkProvisioning::kMsgType_TestConnectivity                 : return "TestConnectivity";
        case NetworkProvisioning::kMsgType_SetRendezvousMode                : return "SetRendezvousMode";
        case NetworkProvisioning::kMsgType_GetNetworks                      : return "GetNetworks";
        case NetworkProvisioning::kMsgType_GetNetworksComplete              : return "GetNetworksComplete";
        case NetworkProvisioning::kMsgType_GetLastResult                    : return "GetLastResult";
        case NetworkProvisioning::kMsgType_AddNetworkV2                     : return "AddNetworkV2";
        }
        break;
    case kWeaveProfile_Security:
        switch (msgType) {
        case Security::kMsgType_PASEInitiatorStep1                          : return "PASEInitiatorStep1";
        case Security::kMsgType_PASEResponderStep1                          : return "PASEResponderStep1";
        case Security::kMsgType_PASEResponderStep2                          : return "PASEResponderStep2";
        case Security::kMsgType_PASEInitiatorStep2                          : return "PASEInitiatorStep2";
        case Security::kMsgType_PASEResponderKeyConfirm                     : return "PASEResponderKeyConfirm";
        case Security::kMsgType_PASEResponderReconfigure                    : return "PASEReconfigure";
        case Security::kMsgType_CASEBeginSessionRequest                     : return "CASEBeginSessionRequest";
        case Security::kMsgType_CASEBeginSessionResponse                    : return "CASEBeginSessionResponse";
        case Security::kMsgType_CASEInitiatorKeyConfirm                     : return "CASEInitiatorKeyConfirm";
        case Security::kMsgType_CASEReconfigure                             : return "CASEReconfigure";
        case Security::kMsgType_TAKEIdentifyToken                           : return "TAKEIdentifyToken";
        case Security::kMsgType_TAKEIdentifyTokenResponse                   : return "TAKEIdentifyTokenResponse";
        case Security::kMsgType_TAKETokenReconfigure                        : return "TAKETokenReconfigure";
        case Security::kMsgType_TAKEAuthenticateToken                       : return "TAKEAuthenticateToken";
        case Security::kMsgType_TAKEAuthenticateTokenResponse               : return "TAKEAuthenticateTokenResponse";
        case Security::kMsgType_TAKEReAuthenticateToken                     : return "TAKEReAuthenticateToken";
        case Security::kMsgType_TAKEReAuthenticateTokenResponse             : return "TAKEReAuthenticateTokenResponse";
        case Security::kMsgType_EndSession                                  : return "EndSession";
        case Security::kMsgType_KeyError                                    : return "KeyError";
        }
        break;
    case kWeaveProfile_FabricProvisioning:
        switch (msgType) {
        case FabricProvisioning::kMsgType_CreateFabric                      : return "CreateFabric";
        case FabricProvisioning::kMsgType_LeaveFabric                       : return "LeaveFabric";
        case FabricProvisioning::kMsgType_GetFabricConfig                   : return "GetFabricConfig";
        case FabricProvisioning::kMsgType_GetFabricConfigComplete           : return "GetFabricConfigComplete";
        case FabricProvisioning::kMsgType_JoinExistingFabric                : return "JoinExistingFabric";
        }
        break;
    case kWeaveProfile_DeviceControl:
        switch (msgType) {
        case DeviceControl::kMsgType_ResetConfig                            : return "ResetConfig";
        case DeviceControl::kMsgType_ArmFailSafe                            : return "ArmFailSafe";
        case DeviceControl::kMsgType_DisarmFailSafe                         : return "DisarmFailSafe";
        case DeviceControl::kMsgType_EnableConnectionMonitor                : return "EnableConnectionMonitor";
        case DeviceControl::kMsgType_DisableConnectionMonitor               : return "DisableConnectionMonitor";
        case DeviceControl::kMsgType_RemotePassiveRendezvous                : return "RemotePassiveRendezvous";
        case DeviceControl::kMsgType_RemoteConnectionComplete               : return "RemoteConnectionComplete";
        case DeviceControl::kMsgType_StartSystemTest                        : return "StartSystemTest";
        case DeviceControl::kMsgType_StopSystemTest                         : return "StopSystemTest";
        }
        break;
    case kWeaveProfile_Time:
        switch (msgType) {
        case Time::kTimeMessageType_TimeSyncTimeChangeNotification          : return "TimeSyncTimeChangeNotification";
        case Time::kTimeMessageType_TimeSyncRequest                         : return "TimeSyncRequest";
        case Time::kTimeMessageType_TimeSyncResponse                        : return "TimeSyncResponse";
        }
        break;
    case kWeaveProfile_WDM:
        switch (msgType) {
        case DataManagement::kMsgType_ViewRequest                           : return "ViewRequest";
        case DataManagement::kMsgType_ViewResponse                          : return "ViewResponse";
        case DataManagement::kMsgType_UpdateRequest                         : return "UpdateRequest";
        case DataManagement::kMsgType_InProgress                            : return "InProgress";
        case DataManagement::kMsgType_SubscribeRequest                      : return "SubscribeRequest";
        case DataManagement::kMsgType_SubscribeResponse                     : return "SubscribeResponse";
        case DataManagement::kMsgType_SubscribeCancelRequest                : return "SubscribeCancelRequest";
        case DataManagement::kMsgType_SubscribeConfirmRequest               : return "SubscribeConfirmRequest";
        case DataManagement::kMsgType_NotificationRequest                   : return "NotificationRequest";
        case DataManagement::kMsgType_CustomCommandRequest                  : return "CommandRequest";
        case DataManagement::kMsgType_CustomCommandResponse                 : return "CommandResponse";
        }
        break;
    case kWeaveProfile_SWU:
        switch (msgType) {
        case SoftwareUpdate::kMsgType_ImageAnnounce                         : return "ImageAnnounce";
        case SoftwareUpdate::kMsgType_ImageQuery                            : return "ImageQuery";
        case SoftwareUpdate::kMsgType_ImageQueryResponse                    : return "ImageQueryResponse";
        case SoftwareUpdate::kMsgType_DownloadNotify                        : return "DownloadNotify";
        case SoftwareUpdate::kMsgType_NotifyResponse                        : return "NotifyResponse";
        case SoftwareUpdate::kMsgType_UpdateNotify                          : return "UpdateNotify";
        case SoftwareUpdate::kMsgType_ImageQueryStatus                      : return "ImageQueryStatus";
        }
        break;
    case kWeaveProfile_BDX:
        switch (msgType) {
        case BDX_Development::kMsgType_SendInit                             : return "SendInit";
        case BDX_Development::kMsgType_SendAccept                           : return "SendAccept";
        case BDX_Development::kMsgType_SendReject                           : return "SendReject";
        case BDX_Development::kMsgType_ReceiveInit                          : return "ReceiveInit";
        case BDX_Development::kMsgType_ReceiveAccept                        : return "ReceiveAccept";
        case BDX_Development::kMsgType_ReceiveReject                        : return "ReceiveReject";
        case BDX_Development::kMsgType_BlockQuery                           : return "BlockQuery";
        case BDX_Development::kMsgType_BlockSend                            : return "BlockSend";
        case BDX_Development::kMsgType_BlockEOF                             : return "BlockEOF";
        case BDX_Development::kMsgType_BlockAck                             : return "BlockAck";
        case BDX_Development::kMsgType_BlockEOFAck                          : return "BlockEOFAck";
        case BDX_Development::kMsgType_TransferError                        : return "TransferError";
        case BDX_Development::kMsgType_BlockQueryV1                         : return "BlockQueryV1";
        case BDX_Development::kMsgType_BlockSendV1                          : return "BlockSendV1";
        case BDX_Development::kMsgType_BlockEOFV1                           : return "BlockEOFV1";
        case BDX_Development::kMsgType_BlockAckV1                           : return "BlockAckV1";
        case BDX_Development::kMsgType_BlockEOFAckV1                        : return "BlockEOFAckV1";
        }
        break;
    case kWeaveProfile_DeviceDescription:
        switch (msgType) {
        case DeviceDescription::kMessageType_IdentifyRequest                : return "IdentifyRequest";
        case DeviceDescription::kMessageType_IdentifyResponse               : return "IdentifyResponse";
        }
        break;
    case kWeaveProfile_ServiceProvisioning:
        switch (msgType) {
        case ServiceProvisioning::kMsgType_RegisterServicePairAccount       : return "RegisterServicePairAccount";
        case ServiceProvisioning::kMsgType_UpdateService                    : return "UpdateService";
        case ServiceProvisioning::kMsgType_UnregisterService                : return "UnregisterService";
        case ServiceProvisioning::kMsgType_UnpairDeviceFromAccount          : return "UnpairDeviceFromAccount";
        case ServiceProvisioning::kMsgType_PairDeviceToAccount              : return "PairDeviceToAccount";
        }
        break;
    case kWeaveProfile_ServiceDirectory:
        switch (msgType) {
        case ServiceDirectory::kMsgType_ServiceEndpointQuery                : return "ServiceEndpointQuery";
        case ServiceDirectory::kMsgType_ServiceEndpointResponse             : return "ServiceEndpointResponse";
        }
        break;
#if WEAVE_CONFIG_ENABLE_TUNNELING
    case kWeaveProfile_Tunneling:
        switch (msgType) {
        case WeaveTunnel::kMsgType_TunnelOpen                               : return "TunnelOpen";
        case WeaveTunnel::kMsgType_TunnelOpenV2                             : return "TunnelOpenV2";
        case WeaveTunnel::kMsgType_TunnelRouteUpdate                        : return "TunnelRouteUpdate";
        case WeaveTunnel::kMsgType_TunnelClose                              : return "TunnelClose";
        case WeaveTunnel::kMsgType_TunnelReconnect                          : return "TunnelReconnect";
        case WeaveTunnel::kMsgType_TunnelRouterAdvertise                    : return "TunnelRouterAdvertise";
        case WeaveTunnel::kMsgType_TunnelMobileClientAdvertise              : return "TunnelMobileClientAdvertise";
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_TUNNELING
    case kWeaveProfile_Heartbeat:
        switch (msgType) {
        case Heartbeat::kHeartbeatMessageType_Heartbeat                     : return "Heartbeat";
        }
        break;
    case kWeaveProfile_TokenPairing:
        switch (msgType) {
        case TokenPairing::kMsgType_PairTokenRequest                        : return "PairTokenRequest";
        case TokenPairing::kMsgType_TokenCertificateResponse                : return "TokenCertificateResponse";
        case TokenPairing::kMsgType_TokenPairedResponse                     : return "TokenPairedResponse";
        case TokenPairing::kMsgType_UnpairTokenRequest                      : return "UnpairTokenRequest";
        }
        break;
    }

    result = FindMessageName(profileId, msgType);

    return (result);
}

} // namespace Weave
} // namespace nl

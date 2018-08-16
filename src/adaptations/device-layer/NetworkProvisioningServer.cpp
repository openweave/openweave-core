/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/NetworkInfo.h>
#include <Weave/DeviceLayer/internal/ESPUtils.h>

#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>

#include "esp_event.h"
#include "esp_wifi.h"

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_Common;
using Profiles::kWeaveProfile_NetworkProvisioning;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

enum
{
    kWiFiStationNetworkId  = 1
};

WEAVE_ERROR NetworkProvisioningServer::Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::DeviceLayer::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

    mState = kState_Idle;

exit:
    return err;
}

void NetworkProvisioningServer::StartPendingScan()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_scan_config_t scanConfig;

    // Do nothing if there's no pending ScanNetworks request outstanding, or if a scan is already in progress.
    if (mState != kState_ScanNetworks_Pending)
    {
        ExitNow();
    }

    // Defer the scan if the WiFi station is in the process of connecting. The Connection Manager will
    // call this method again when the connect attempt is complete.
    if (!ConnectivityMgr.CanStartWiFiScan())
    {
        ExitNow();
    }

    // Initiate an active scan using the default dwell times.  Configure the scan to return hidden networks.
    memset(&scanConfig, 0, sizeof(scanConfig));
    scanConfig.show_hidden = 1;
    scanConfig.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    err = esp_wifi_scan_start(&scanConfig, false);
    SuccessOrExit(err);

#if WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
    // Arm timer in case we never get the scan done event.
    SystemLayer.StartTimer(WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT, HandleScanTimeOut, NULL);
#endif // WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT

    mState = kState_ScanNetworks_InProgress;

exit:
    // If an error occurred, send a Internal Error back to the requestor.
    if (err != WEAVE_NO_ERROR)
    {
        SendStatusReport(kWeaveProfile_Common, kStatus_InternalError, err);
        mState = kState_Idle;
    }
}

void NetworkProvisioningServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    WEAVE_ERROR err;

    // Handle ESP system events...
    if (event->Type == WeaveDeviceEvent::kEventType_ESPSystemEvent)
    {
        switch(event->ESPSystemEvent.event_id) {
        case SYSTEM_EVENT_SCAN_DONE:
            WeaveLogProgress(DeviceLayer, "SYSTEM_EVENT_SCAN_DONE");
            HandleScanDone();
            break;
        default:
            break;
        }
    }

    // Handle a change in WiFi connectivity...
    else if (event->Type == WeaveDeviceEvent::kEventType_WiFiConnectivityChange)
    {
        // Whenever WiFi connectivity is established, update the persisted WiFi
        // station security type to match that used by the connected AP.
        if (event->WiFiConnectivityChange.Result == kConnectivity_Established)
        {
            wifi_ap_record_t ap_info;

            err = esp_wifi_sta_get_ap_info(&ap_info);
            SuccessOrExit(err);

            WiFiSecurityType secType = ESPUtils::WiFiAuthModeToWeaveWiFiSecurityType(ap_info.authmode);

            err = ConfigurationMgr().UpdateWiFiStationSecurityType(secType);
            SuccessOrExit(err);
        }
    }

    // Handle a change in Internet connectivity...
    else if (event->Type == WeaveDeviceEvent::kEventType_InternetConnectivityChange)
    {
        // If the system now has IPv4 Internet connectivity, continue the process of
        // performing the TestConnectivity request.
        if (ConnectivityMgr.HaveIPv4InternetConnectivity())
        {
            ContinueTestConnectivity();
        }
    }

exit:
    return;
}

// ==================== NetworkProvisioningServer Private Methods ====================

WEAVE_ERROR NetworkProvisioningServer::GetWiFiStationProvision(NetworkInfo & netInfo, bool includeCredentials)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_config_t stationConfig;

    netInfo.Reset();

    err = esp_wifi_get_config(ESP_IF_WIFI_STA, &stationConfig);
    SuccessOrExit(err);

    VerifyOrExit(stationConfig.sta.ssid[0] != 0, err = WEAVE_ERROR_INCORRECT_STATE);

    netInfo.NetworkId = kWiFiStationNetworkId;
    netInfo.NetworkIdPresent = true;
    netInfo.NetworkType = kNetworkType_WiFi;
    memcpy(netInfo.WiFiSSID, stationConfig.sta.ssid, min(strlen((char *)stationConfig.sta.ssid) + 1, sizeof(netInfo.WiFiSSID)));
    netInfo.WiFiMode = kWiFiMode_Managed;
    netInfo.WiFiRole = kWiFiRole_Station;

    err = ConfigurationMgr().GetWiFiStationSecurityType(netInfo.WiFiSecurityType);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    if (includeCredentials)
    {
        netInfo.WiFiKeyLen = min(strlen((char *)stationConfig.sta.password), sizeof(netInfo.WiFiKey));
        memcpy(netInfo.WiFiKey, stationConfig.sta.password, netInfo.WiFiKeyLen);
    }

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::ValidateWiFiStationProvision(
        const ::nl::Weave::DeviceLayer::Internal::NetworkInfo & netInfo,
        uint32_t & statusProfileId, uint16_t & statusCode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (netInfo.NetworkType != kNetworkType_WiFi)
    {
        WeaveLogError(DeviceLayer, "ConnectivityManager: Unsupported WiFi station network type: %d", netInfo.NetworkType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedNetworkType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSSID[0] == 0)
    {
        WeaveLogError(DeviceLayer, "ConnectivityManager: Missing WiFi station SSID");
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiMode != kWiFiMode_Managed)
    {
        if (netInfo.WiFiMode == kWiFiMode_NotSpecified)
        {
            WeaveLogError(DeviceLayer, "ConnectivityManager: Missing WiFi station mode");
        }
        else
        {
            WeaveLogError(DeviceLayer, "ConnectivityManager: Unsupported WiFi station mode: %d", netInfo.WiFiMode);
        }
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiRole != kWiFiRole_Station)
    {
        if (netInfo.WiFiRole == kWiFiRole_NotSpecified)
        {
            WeaveLogError(DeviceLayer, "ConnectivityManager: Missing WiFi station role");
        }
        else
        {
            WeaveLogError(DeviceLayer, "ConnectivityManager: Unsupported WiFi station role: %d", netInfo.WiFiRole);
        }
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSecurityType != kWiFiSecurityType_None &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WEP &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPAPersonal &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPA2Personal &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPA2Enterprise)
    {
        WeaveLogError(DeviceLayer, "ConnectivityManager: Unsupported WiFi station security type: %d", netInfo.WiFiSecurityType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedWiFiSecurityType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSecurityType != kWiFiSecurityType_None && netInfo.WiFiKeyLen == 0)
    {
        WeaveLogError(DeviceLayer, "NetworkProvisioning: Missing WiFi Key");
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::SetESPStationConfig(const ::nl::Weave::DeviceLayer::Internal::NetworkInfo  & netInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_config_t wifiConfig;

    // Ensure that ESP station mode is enabled.  This is required before esp_wifi_set_config(ESP_IF_WIFI_STA,...)
    // can be called.
    err = ESPUtils::EnableStationMode();
    SuccessOrExit(err);

    // Initialize an ESP wifi_config_t structure based on the new provision information.
    memset(&wifiConfig, 0, sizeof(wifiConfig));
    memcpy(wifiConfig.sta.ssid, netInfo.WiFiSSID, min(strlen(netInfo.WiFiSSID) + 1, sizeof(wifiConfig.sta.ssid)));
    memcpy(wifiConfig.sta.password, netInfo.WiFiKey, min((size_t)netInfo.WiFiKeyLen, sizeof(wifiConfig.sta.password)));
    if (netInfo.WiFiSecurityType == kWiFiSecurityType_NotSpecified)
    {
        wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    }
    else
    {
        wifiConfig.sta.scan_method = WIFI_FAST_SCAN;
        wifiConfig.sta.threshold.rssi = 0;
        switch (netInfo.WiFiSecurityType)
        {
        case kWiFiSecurityType_None:
            wifiConfig.sta.threshold.authmode = WIFI_AUTH_OPEN;
            break;
        case kWiFiSecurityType_WEP:
            wifiConfig.sta.threshold.authmode = WIFI_AUTH_WEP;
            break;
        case kWiFiSecurityType_WPAPersonal:
            wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
            break;
        case kWiFiSecurityType_WPA2Personal:
            wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            break;
        case kWiFiSecurityType_WPA2Enterprise:
            wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_ENTERPRISE;
            break;
        default:
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }
    wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    // Configure the ESP WiFi interface.
    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);
    if (err != ESP_OK)
    {
        WeaveLogError(DeviceLayer, "esp_wifi_set_config() failed: %s", nl::ErrorStr(err));
    }
    SuccessOrExit(err);

    // Store the WiFi Station security type separately in NVS.  This is necessary because the ESP wifi API
    // does not provide a way to query the configured WiFi auth mode.
    err = ConfigurationMgr().UpdateWiFiStationSecurityType(netInfo.WiFiSecurityType);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "WiFi station provision set (SSID: %s)", netInfo.WiFiSSID);

exit:
    return err;
}

bool NetworkProvisioningServer::RejectIfApplicationControlled(bool station)
{
    bool isAppControlled = (station)
        ? ConnectivityMgr.IsWiFiStationApplicationControlled()
        : ConnectivityMgr.IsWiFiAPApplicationControlled();

    // Reject the request if the application is currently in control of the WiFi station.
    if (isAppControlled)
    {
        SendStatusReport(kWeaveProfile_Common, kStatus_NotAvailable);
    }

    return isAppControlled;
}

void NetworkProvisioningServer::HandleScanDone()
{
    WEAVE_ERROR err;
    wifi_ap_record_t * scanResults = NULL;
    uint16_t scanResultCount;
    uint16_t encodedResultCount;
    PacketBuffer * respBuf = NULL;

    // If we receive a SCAN DONE event for a scan that we didn't initiate, simply ignore it.
    VerifyOrExit(mState == kState_ScanNetworks_InProgress, err = WEAVE_NO_ERROR);

    mState = kState_Idle;

#if WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT
    // Cancel the scan timeout timer.
    SystemLayer.CancelTimer(HandleScanTimeOut, NULL);
#endif // WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT

    // Determine the number of scan results found.
    err = esp_wifi_scan_get_ap_num(&scanResultCount);
    SuccessOrExit(err);

    // Only return up to WEAVE_DEVICE_CONFIG_MAX_SCAN_NETWORKS_RESULTS.
    scanResultCount = min(scanResultCount, (uint16_t)WEAVE_DEVICE_CONFIG_MAX_SCAN_NETWORKS_RESULTS);

    // Allocate a buffer to hold the scan results array.
    scanResults = (wifi_ap_record_t *)malloc(scanResultCount * sizeof(wifi_ap_record_t));
    VerifyOrExit(scanResults != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Collect the scan results from the ESP WiFi driver.  Note that this also *frees*
    // the internal copy of the results.
    err = esp_wifi_scan_get_ap_records(&scanResultCount, scanResults);
    SuccessOrExit(err);

    // If the ScanNetworks request is still outstanding...
    if (GetCurrentOp() == kMsgType_ScanNetworks)
    {
        nl::Weave::TLV::TLVWriter writer;
        TLVType outerContainerType;

        // Sort results by rssi.
        qsort(scanResults, scanResultCount, sizeof(*scanResults), ESPUtils::OrderScanResultsByRSSI);

        // Allocate a packet buffer to hold the encoded scan results.
        respBuf = PacketBuffer::New(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE + 1);
        VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Encode the list of scan results into the response buffer.  If the encoded size of all
        // the results exceeds the size of the buffer, encode only what will fit.
        writer.Init(respBuf, respBuf->AvailableDataLength() - 1);
        err = writer.StartContainer(AnonymousTag, kTLVType_Array, outerContainerType);
        SuccessOrExit(err);
        for (encodedResultCount = 0; encodedResultCount < scanResultCount; encodedResultCount++)
        {
            NetworkInfo netInfo;
            const wifi_ap_record_t & scanResult = scanResults[encodedResultCount];

            netInfo.Reset();
            netInfo.NetworkType = kNetworkType_WiFi;
            memcpy(netInfo.WiFiSSID, scanResult.ssid, min(strlen((char *)scanResult.ssid) + 1, (size_t)NetworkInfo::kMaxWiFiSSIDLength));
            netInfo.WiFiSSID[NetworkInfo::kMaxWiFiSSIDLength] = 0;
            netInfo.WiFiMode = kWiFiMode_Managed;
            netInfo.WiFiRole = kWiFiRole_Station;
            netInfo.WiFiSecurityType = ESPUtils::WiFiAuthModeToWeaveWiFiSecurityType(scanResult.authmode);
            netInfo.WirelessSignalStrength = scanResult.rssi;

            {
                nl::Weave::TLV::TLVWriter savePoint = writer;
                err = netInfo.Encode(writer);
                if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
                {
                    writer = savePoint;
                    break;
                }
            }
            SuccessOrExit(err);
        }
        err = writer.EndContainer(outerContainerType);
        SuccessOrExit(err);
        err = writer.Finalize();
        SuccessOrExit(err);

        // Send the scan results to the requestor.  Note that this method takes ownership of the
        // buffer, success or fail.
        err = SendNetworkScanComplete(encodedResultCount, respBuf);
        respBuf = NULL;
        SuccessOrExit(err);
    }

exit:
    PacketBuffer::Free(respBuf);

    // If an error occurred and we haven't yet responded, send a Internal Error back to the
    // requestor.
    if (err != WEAVE_NO_ERROR && GetCurrentOp() == kMsgType_ScanNetworks)
    {
        SendStatusReport(kWeaveProfile_Common, kStatus_InternalError, err);
    }

    // Tell the ConnectivityManager that the WiFi scan is now done.  This allows it to continue
    // any activities that were deferred while the scan was in progress.
    ConnectivityMgr.OnWiFiScanDone();
}

void NetworkProvisioningServer::ContinueTestConnectivity(void)
{
    // If waiting for Internet connectivity to be established, and IPv4 Internet connectivity
    // now exists...
    if (mState == kState_TestConnectivity_WaitConnectivity && ConnectivityMgr.HaveIPv4InternetConnectivity())
    {
        // Reset the state.
        mState = kState_Idle;
        SystemLayer.CancelTimer(HandleConnectivityTimeOut, NULL);

        // Verify that the TestConnectivity request is still outstanding; if so...
        if (GetCurrentOp() == kMsgType_TestConnectivity)
        {
            // TODO: perform positive test of connectivity to the Internet.

            // Send a Success response to the client.
            SendSuccessResponse();
        }
    }
}

#if WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT

void NetworkProvisioningServer::HandleScanTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError)
{
    WeaveLogError(DeviceLayer, "WiFi scan timed out");

    // Reset the state.
    NetworkProvisioningSvr.mState = kState_Idle;

    // Verify that the ScanNetworks request is still outstanding; if so, send a
    // Common:InternalError StatusReport to the client.
    if (NetworkProvisioningSvr.GetCurrentOp() == kMsgType_ScanNetworks)
    {
        NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_Common, kStatus_InternalError, WEAVE_ERROR_TIMEOUT);
    }

    // Tell the ConnectivityManager that the WiFi scan is now done.
    ConnectivityMgr.OnWiFiScanDone();
}

#endif // WEAVE_DEVICE_CONFIG_WIFI_SCAN_COMPLETION_TIMEOUT

void NetworkProvisioningServer::HandleConnectivityTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError)
{
    WeaveLogError(DeviceLayer, "Time out waiting for Internet connectivity");

    // Reset the state.
    NetworkProvisioningSvr.mState = kState_Idle;
    SystemLayer.CancelTimer(HandleConnectivityTimeOut, NULL);

    // Verify that the TestConnectivity request is still outstanding; if so, send a
    // NetworkProvisioning:NetworkConnectFailed StatusReport to the client.
    if (NetworkProvisioningSvr.GetCurrentOp() == kMsgType_TestConnectivity)
    {
        NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_NetworkConnectFailed, WEAVE_ERROR_TIMEOUT);
    }
}

WEAVE_ERROR NetworkProvisioningServer::HandleScanNetworks(uint8_t networkType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify the expected network type.
    if (networkType != kNetworkType_WiFi)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Enter the ScanNetworks Pending state and attempt to start the scan.
    mState = kState_ScanNetworks_Pending;
    StartPendingScan();

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleAddNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Parse the supplied network configuration info.
    {
        TLV::TLVReader reader;
        reader.Init(networkInfoTLV);
        err = netInfo.Decode(reader);
        SuccessOrExit(err);
    }

    // Discard the request buffer.
    PacketBuffer::Free(networkInfoTLV);
    networkInfoTLV = NULL;

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Check the validity of the new WiFi station provision. If not acceptable, respond to
    // the requestor with an appropriate StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // If the WiFi station is not already configured, disable the WiFi station interface.
    // This ensures that the device will not automatically connect to the new network until
    // an EnableNetwork request is received.
    if (!ConnectivityMgr.IsWiFiStationProvisioned())
    {
        err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled);
        SuccessOrExit(err);
    }

    // Set the ESP WiFi station configuration.
    err = SetESPStationConfig(netInfo);
    SuccessOrExit(err);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr.OnWiFiStationProvisionChange();

    // Send an AddNetworkComplete message back to the requestor.
    SendAddNetworkComplete(kWiFiStationNetworkId);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleUpdateNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo, netInfoUpdates;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Parse the supplied network configuration info.
    {
        TLV::TLVReader reader;
        reader.Init(networkInfoTLV);
        err = netInfoUpdates.Decode(reader);
        SuccessOrExit(err);
    }

    // Discard the request buffer.
    PacketBuffer::Free(networkInfoTLV);
    networkInfoTLV = NULL;

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // If the network id field isn't present, immediately reply with an error.
    if (!netInfoUpdates.NetworkIdPresent)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || netInfoUpdates.NetworkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Get the existing station provision.
    err = GetWiFiStationProvision(netInfo, true);
    SuccessOrExit(err);

    // Merge in the updated information.
    err = netInfoUpdates.MergeTo(netInfo);
    SuccessOrExit(err);

    // Check the validity of the updated station provision. If not acceptable, respond to the requestor with an
    // appropriate StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // Set the ESP WiFi station configuration.
    err = SetESPStationConfig(netInfo);
    SuccessOrExit(err);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr.OnWiFiStationProvisionChange();

    // Tell the requestor we succeeded.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleRemoveNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_config_t stationConfig;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Clear the ESP WiFi station configuration.
    memset(&stationConfig, 0, sizeof(stationConfig));
    esp_wifi_set_config(ESP_IF_WIFI_STA, &stationConfig);

    // Clear the persisted WiFi station security type.
    ConfigurationMgr().UpdateWiFiStationSecurityType(kWiFiSecurityType_NotSpecified);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr.OnWiFiStationProvisionChange();

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleGetNetworks(uint8_t flags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;
    PacketBuffer * respBuf = NULL;
    TLVWriter writer;
    uint8_t resultCount;
    const bool includeCredentials = (flags & kGetNetwork_IncludeCredentials) != 0;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    respBuf = PacketBuffer::New();
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(respBuf);

    err = GetWiFiStationProvision(netInfo, includeCredentials);
    if (err == WEAVE_NO_ERROR)
    {
        resultCount = 1;
    }
    else if (err == WEAVE_ERROR_INCORRECT_STATE)
    {
        resultCount = 0;
    }
    else
    {
        ExitNow();
    }

    err = NetworkInfo::EncodeArray(writer, &netInfo, resultCount);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    err = SendGetNetworksComplete(resultCount, respBuf);
    respBuf = NULL;
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(respBuf);
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleEnableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Tell the ConnectivityManager to enable the WiFi station interface.
    // Note that any effects of enabling the WiFi station interface (e.g. connecting to an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
    SuccessOrExit(err);

    // Send a Success response back to the client.
    SendSuccessResponse();

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleDisableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    // Tell the ConnectivityManager to disable the WiFi station interface.
    // Note that any effects of disabling the WiFi station interface (e.g. disconnecting from an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled);
    SuccessOrExit(err);

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleTestConnectivity(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Tell the ConnectivityManager to enable the WiFi station interface if it hasn't been done already.
    // Note that any effects of enabling the WiFi station interface (e.g. connecting to an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
    SuccessOrExit(err);

    // Record that we're waiting for the WiFi station interface to establish connectivity
    // with the Internet and arm a timer that will generate an error if connectivity isn't established
    // within a certain amount of time.
    mState = kState_TestConnectivity_WaitConnectivity;
    SystemLayer.StartTimer(WEAVE_DEVICE_CONFIG_WIFI_CONNECTIVITY_TIMEOUT, HandleConnectivityTimeOut, NULL);

    // Go check for connectivity now.
    ContinueTestConnectivity();

exit:
    return err;
}

WEAVE_ERROR NetworkProvisioningServer::HandleSetRendezvousMode(uint16_t rendezvousMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // If any modes other than EnableWiFiRendezvousNetwork or EnableThreadRendezvous
    // were specified, fail with Common:UnsupportedMessage.
    if ((rendezvousMode & ~(kRendezvousMode_EnableWiFiRendezvousNetwork | kRendezvousMode_EnableThreadRendezvous)) != 0)
    {
        err = SendStatusReport(kWeaveProfile_Common, kStatus_UnsupportedMessage);
        ExitNow();
    }

    // If EnableThreadRendezvous was requested, fail with NetworkProvisioning:UnsupportedNetworkType.
    if ((rendezvousMode & kRendezvousMode_EnableThreadRendezvous) != 0)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // Reject the request if the application is currently in control of the WiFi AP.
    if (RejectIfApplicationControlled(false))
    {
        ExitNow();
    }

    // If the request is to start the WiFi "rendezvous network" (a.k.a. the WiFi AP interface)...
    if (rendezvousMode != 0)
    {
        // If the AP interface has been expressly disabled by the application, fail with Common:NotAvailable.
        if (ConnectivityMgr.GetWiFiAPMode() == ConnectivityManager::kWiFiAPMode_Disabled)
        {
            err = SendStatusReport(kWeaveProfile_Common, kStatus_NotAvailable);
            ExitNow();
        }

        // Otherwise, request the ConnectivityManager to demand start the WiFi AP interface.
        // If the interface is already active this will have no immediate effect, except if the
        // interface is in the "demand" mode, in which case this will serve to extend the
        // active time.
        ConnectivityMgr.DemandStartWiFiAP();
    }

    // Otherwise the request is to stop the WiFi rendezvous network, so request the ConnectivityManager
    // to stop the AP interface if it has been demand started.  This will have no effect if the
    // interface is already stopped, or if the application has expressly enabled the interface.
    else
    {
        ConnectivityMgr.StopOnDemandWiFiAP();
    }

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

bool NetworkProvisioningServer::IsPairedToAccount() const
{
    return ConfigurationMgr().IsServiceProvisioned() && ConfigurationMgr().IsPairedToAccount();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

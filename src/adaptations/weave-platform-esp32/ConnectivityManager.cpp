#include <internal/WeavePlatformInternal.h>
#include <ConnectivityManager.h>
#include <internal/NetworkProvisioningServer.h>
#include <internal/NetworkInfo.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include "esp_wifi.h"
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;
using namespace ::WeavePlatform::Internal;

using Profiles::kWeaveProfile_Common;
using Profiles::kWeaveProfile_NetworkProvisioning;

namespace WeavePlatform {

namespace {

enum
{
    kWiFiStationNetworkId  = 1
};

extern const char *ESPWiFiModeToStr(wifi_mode_t wifiMode);
extern WEAVE_ERROR ChangeESPWiFiMode(esp_interface_t intf, bool enabled);

} // namespace Internal


// ==================== ConnectivityManager Public Methods ====================

ConnectivityManager::WiFiStationMode ConnectivityManager::GetWiFiStationMode(void) const
{
    bool autoConnect;
    return (esp_wifi_get_auto_connect(&autoConnect) == ESP_OK && autoConnect)
            ? kWiFiStationMode_Enabled : kWiFiStationMode_Disabled;
}

bool ConnectivityManager::IsWiFiStationEnabled(void) const
{
    return GetWiFiStationMode() == kWiFiStationMode_Enabled;
}

WEAVE_ERROR ConnectivityManager::SetWiFiStationMode(WiFiStationMode val)
{
    esp_err_t err;
    bool autoConnect;

    VerifyOrExit(val == kWiFiStationMode_Enabled || val == kWiFiStationMode_Disabled, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = esp_wifi_get_auto_connect(&autoConnect);
    SuccessOrExit(err);

    if (autoConnect != (val == kWiFiStationMode_Enabled))
    {
        autoConnect = (val == kWiFiStationMode_Enabled);
        err = esp_wifi_set_auto_connect(val);
        SuccessOrExit(err);

        ESP_LOGI(TAG, "WiFi station interface %s", (autoConnect) ? "enabled" : "disabled");

        SystemLayer.ScheduleWork(DriveStationState, NULL);
    }

exit:
    return err;
}

bool ConnectivityManager::IsWiFiStationProvisioned(void) const
{
    wifi_config_t stationConfig;
    return (esp_wifi_get_config(ESP_IF_WIFI_STA, &stationConfig) == ERR_OK && stationConfig.sta.ssid[0] != 0);
}

void ConnectivityManager::ClearWiFiStationProvision(void)
{
    wifi_config_t stationConfig;

    memset(&stationConfig, 0, sizeof(stationConfig));
    esp_wifi_set_config(ESP_IF_WIFI_STA, &stationConfig);

    SystemLayer.ScheduleWork(DriveStationState, NULL);
}

uint32_t ConnectivityManager::GetWiFiStationNetworkId(void) const
{
    return kWiFiStationNetworkId;
}

WEAVE_ERROR ConnectivityManager::SetWiFiAPMode(WiFiAPMode val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(val == kWiFiAPMode_Disabled ||
                 val == kWiFiAPMode_Enabled ||
                 val == kWiFiAPMode_OnDemand ||
                 val == kWiFiAPMode_OnDemand_NoStationProvision,
                 err = WEAVE_ERROR_INVALID_ARGUMENT);

    mWiFiAPMode = val;

    SystemLayer.ScheduleWork(DriveAPState, NULL);

exit:
    return err;
}

void ConnectivityManager::DemandStartWiFiAP(void)
{
    if (mWiFiAPMode == kWiFiAPMode_OnDemand ||
        mWiFiAPMode == kWiFiAPMode_OnDemand_NoStationProvision)
    {
        mLastAPDemandTime = SystemLayer.GetSystemTimeMS();
        SystemLayer.ScheduleWork(DriveAPState, NULL);
    }
}

void ConnectivityManager::StopOnDemandWiFiAP(void)
{
    if (mWiFiAPMode == kWiFiAPMode_OnDemand ||
        mWiFiAPMode == kWiFiAPMode_OnDemand_NoStationProvision)
    {
        mLastAPDemandTime = 0;
        SystemLayer.ScheduleWork(DriveAPState, NULL);
    }
}

void ConnectivityManager::MaintainOnDemandWiFiAP(void)
{
    if (mWiFiAPMode == kWiFiAPMode_OnDemand ||
        mWiFiAPMode == kWiFiAPMode_OnDemand_NoStationProvision)
    {
        if (mWiFiAPState == kWiFiAPState_Started || mWiFiAPState == kWiFiAPState_Starting)
        {
            mLastAPDemandTime = SystemLayer.GetSystemTimeMS();
        }
    }
}

void ConnectivityManager::SetWiFiAPTimeoutMS(uint32_t val)
{
    mWiFiAPTimeoutMS = val;
    SystemLayer.ScheduleWork(DriveAPState, NULL);
}

// ==================== ConnectivityManager Platform Internal Methods ====================

WEAVE_ERROR ConnectivityManager::Init()
{
    WEAVE_ERROR err;

    mLastStationConnectTime = 0;
    mLastAPDemandTime = 0;
    mWiFiStationState = kWiFiStationState_Disabled;
    mWiFiAPMode = kWiFiAPMode_Disabled;
    mWiFiAPState = kWiFiAPState_Stopped;
    mWiFiStationReconnectIntervalMS = 5000; // TODO: make configurable
    mWiFiAPTimeoutMS = 30000; // TODO: make configurable

    // If there is no persistent station provision...
    if (!IsWiFiStationProvisioned())
    {
        // Switch to station mode temporarily so that the configuration can be changed.
        err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_wifi_set_mode() failed: %s", nl::ErrorStr(err));
        }

        // If the code has been compiled with a default WiFi station provision, configure that now.
        if (CONFIG_DEFAULT_WIFI_SSID[0] != 0)
        {
            ESP_LOGI(TAG, "Setting default WiFi station configuration (SSID: %s)", CONFIG_DEFAULT_WIFI_SSID);

            // Set a default station configuration.
            wifi_config_t wifiConfig;
            memset(&wifiConfig, 0, sizeof(wifiConfig));
            memcpy(wifiConfig.sta.ssid, CONFIG_DEFAULT_WIFI_SSID, strlen(CONFIG_DEFAULT_WIFI_SSID) + 1);
            memcpy(wifiConfig.sta.password, CONFIG_DEFAULT_WIFI_PASSWORD, strlen(CONFIG_DEFAULT_WIFI_PASSWORD) + 1);
            wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
            wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
            err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_set_config() failed: %s", nl::ErrorStr(err));
            }
            err = WEAVE_NO_ERROR;

            // Enable WiFi station mode.
            err = esp_wifi_set_auto_connect(true);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_set_auto_connect() failed: %s", nl::ErrorStr(err));
            }
        }

        // Otherwise, ensure WiFi station mode is disabled.
        else
        {
            err = esp_wifi_set_auto_connect(false);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_set_auto_connect() failed: %s", nl::ErrorStr(err));
            }
            SuccessOrExit(err);
        }
    }

    // Disable both AP and STA mode.  The AP and station state machines will re-enable these as needed.
    err = esp_wifi_set_mode(WIFI_MODE_NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode() failed: %s", nl::ErrorStr(err));
    }
    SuccessOrExit(err);

    // Queue work items to bootstrap the AP and station state machines once the Weave event loop is running.
    err = SystemLayer.ScheduleWork(DriveStationState, NULL);
    SuccessOrExit(err);
    err = SystemLayer.ScheduleWork(DriveAPState, NULL);
    SuccessOrExit(err);

exit:
    return err;
}

NetworkProvisioningDelegate * ConnectivityManager::GetNetworkProvisioningDelegate()
{
    return &mNetProvDelegate;
}

void ConnectivityManager::OnPlatformEvent(const struct WeavePlatformEvent * event)
{
    WEAVE_ERROR err;

    // Handle ESP system events...
    if (event->Type == WeavePlatformEvent::kEventType_ESPSystemEvent)
    {
        switch(event->ESPSystemEvent.event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            DriveStationState();
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            DriveStationState();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            DriveStationState();
            break;
        case SYSTEM_EVENT_STA_STOP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
            DriveStationState();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
            {
                ESP_LOGE(TAG, "Error returned by MessageLayer.RefreshEndpoints(): %s", nl::ErrorStr(err));
            }
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_LOST_IP");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
            {
                ESP_LOGE(TAG, "Error returned by MessageLayer.RefreshEndpoints(): %s", nl::ErrorStr(err));
            }
            break;
        case SYSTEM_EVENT_GOT_IP6:
            ESP_LOGI(TAG, "SYSTEM_EVENT_GOT_IP6");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
            {
                ESP_LOGE(TAG, "Error returned by MessageLayer.RefreshEndpoints(): %s", nl::ErrorStr(err));
            }
            break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
            if (mWiFiAPState == kWiFiAPState_Starting)
            {
                mWiFiAPState = kWiFiAPState_Started;
            }
            DriveAPState();
            break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");
            if (mWiFiAPState == kWiFiAPState_Stopping)
            {
                mWiFiAPState = kWiFiAPState_Stopped;
            }
            DriveAPState();
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
            MaintainOnDemandWiFiAP();
            break;
        default:
            break;
        }
    }
}

// ==================== ConnectivityManager Private Methods ====================

void ConnectivityManager::DriveStationState()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WiFiStationState curState;

    // Determine the current state of the WiFi station interface.
    {
        wifi_mode_t wifiMode;
        if (esp_wifi_get_mode(&wifiMode) == ESP_OK && (wifiMode == WIFI_MODE_STA || wifiMode == WIFI_MODE_APSTA))
        {
            wifi_ap_record_t apInfo;
            // Determine if the station is currently connected to an AP.
            if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK)
            {
                curState = kWiFiStationState_Connected;
            }
            else
            {
                curState = kWiFiStationState_NotConnected;
            }
        }
        else
        {
            curState = kWiFiStationState_Disabled;
        }
    }

    // If the station state has changed...
    if (curState != mWiFiStationState)
    {
        // Handle a transition to the Connected state.
        if (curState == kWiFiStationState_Connected)
        {
            ESP_LOGI(TAG, "WiFi station interface connected");
            OnStationConnected();
        }

        // Handle a transition FROM the Connected state to Disconnected or Disabled.
        else if (mWiFiStationState == kWiFiStationState_Connected)
        {
            ESP_LOGI(TAG, "WiFi station interface disconnected");
            mLastStationConnectTime = 0;
            OnStationDisconnected();
        }

        mWiFiStationState = curState;
    }

    // If ESP station mode is currently disabled, enable it.
    if (mWiFiStationState == kWiFiStationState_Disabled)
    {
        err = ChangeESPWiFiMode(ESP_IF_WIFI_STA, true);
        SuccessOrExit(err);
    }

    // Otherwise ESP station mode is currently enables, so...
    // if station mode is enabled at the Weave level and a station provision exists...
    else if (IsWiFiStationEnabled() && IsWiFiStationProvisioned())
    {
        // If the station is not presently connected to an AP...
        if (mWiFiStationState == kWiFiStationState_NotConnected)
        {
            uint64_t now = SystemLayer.GetSystemTimeMS();

            // Initiate a connection to the AP if we haven't done so before, or if enough
            // time has passed since the last attempt.
            if (mLastStationConnectTime == 0 || now >= mLastStationConnectTime + mWiFiStationReconnectIntervalMS)
            {
                mLastStationConnectTime = now;

                ESP_LOGI(TAG, "Attempting to connect WiFi station interface");
                err = esp_wifi_connect();
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "esp_wifi_connect() failed: %s", nl::ErrorStr(err));
                }
                SuccessOrExit(err);
            }

            // Otherwise arrange another connection attempt at a suitable point in the future.
            else
            {
                uint32_t timeToNextConnect = (uint32_t)((mLastStationConnectTime + mWiFiStationReconnectIntervalMS) - now);

                ESP_LOGI(TAG, "Next WiFi station reconnect in %" PRIu32 " ms", timeToNextConnect);

                err = SystemLayer.StartTimer(timeToNextConnect, DriveStationState, NULL);
                SuccessOrExit(err);
            }
        }
    }

    // Otherwise station mode is DISABLED at the Weave level or no station provision exists, so...
    else
    {
        // If the station is currently connected to an AP, disconnect now.
        if (mWiFiStationState == kWiFiStationState_Connected)
        {
            ESP_LOGI(TAG, "Disconnecting WiFi station interface");
            err = esp_wifi_disconnect();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_disconnect() failed: %s", nl::ErrorStr(err));
            }
            SuccessOrExit(err);
        }
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        SetWiFiStationMode(kWiFiStationMode_Disabled);
    }
}

void ConnectivityManager::DriveAPState()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WiFiAPState targetState;
    uint32_t apTimeout = 0;

    if (mWiFiAPMode == kWiFiAPMode_Disabled)
    {
        targetState = kWiFiAPState_Stopped;
    }

    else if (mWiFiAPMode == kWiFiAPMode_Enabled)
    {
        targetState = kWiFiAPState_Started;
    }

    else if (mWiFiAPMode == kWiFiAPMode_OnDemand_NoStationProvision && !IsWiFiStationProvisioned())
    {
        targetState = kWiFiAPState_Started;
    }

    else if (mWiFiAPMode == kWiFiAPMode_OnDemand ||
             mWiFiAPMode == kWiFiAPMode_OnDemand_NoStationProvision)
    {
        uint64_t now = SystemLayer.GetSystemTimeMS();

        if (mLastAPDemandTime != 0 && now < (mLastAPDemandTime + mWiFiAPTimeoutMS))
        {
            targetState = kWiFiAPState_Started;
            apTimeout = (uint32_t)((mLastAPDemandTime + mWiFiAPTimeoutMS) - now);
        }
        else
        {
            targetState = kWiFiAPState_Stopped;
        }
    }
    else
    {
        targetState = kWiFiAPState_Stopped;
    }

    if (mWiFiAPState != targetState)
    {
        if (targetState == kWiFiAPState_Started)
        {
            wifi_config_t wifiConfig;

            err = ChangeESPWiFiMode(ESP_IF_WIFI_AP, true);
            SuccessOrExit(err);

            memset(&wifiConfig, 0, sizeof(wifiConfig));
            strcpy((char *)wifiConfig.ap.ssid, "ESP-TEST");
            wifiConfig.ap.channel = 1;
            wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
            wifiConfig.ap.max_connection = 4;
            wifiConfig.ap.beacon_interval = 100;
            err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfig);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_set_config(ESP_IF_WIFI_AP) failed: %s", nl::ErrorStr(err));
            }
            SuccessOrExit(err);

            if (mWiFiAPState == kWiFiAPState_Stopped)
            {
                mWiFiAPState = kWiFiAPState_Starting;
            }
        }
        else
        {
            err = ChangeESPWiFiMode(ESP_IF_WIFI_AP, false);
            SuccessOrExit(err);

            if (mWiFiAPState == kWiFiAPState_Started)
            {
                mWiFiAPState = kWiFiAPState_Stopping;
            }
        }
    }

    if (apTimeout != 0)
    {
        ESP_LOGI(TAG, "Next WiFi AP timeout in %" PRIu32 " ms", apTimeout);

        err = SystemLayer.StartTimer(apTimeout, DriveAPState, NULL);
        SuccessOrExit(err);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        SetWiFiAPMode(kWiFiAPMode_Disabled);
    }
}

void ConnectivityManager::OnStationConnected()
{
    // TODO: alert other subsystems of connected state

    // Assign an IPv6 link local address to the station interface.
    tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
}

void ConnectivityManager::OnStationDisconnected()
{
    // TODO: alert other subsystems of disconnected state
}

void ConnectivityManager::DriveStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError)
{
    ConnectivityMgr.DriveStationState();
}

void ConnectivityManager::DriveAPState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError)
{
    ConnectivityMgr.DriveAPState();
}

// ==================== ConnectivityManager::NetworkProvisioningDelegate Public Methods ====================

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleScanNetworks(uint8_t networkType)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleAddNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;

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

    // Delegate to the ConnectivityManager to check the validity of the new WiFi station provision.
    // If the new provision is not acceptable, respond to the requestor with an appropriate
    // StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = NetworkProvisioningSvr.SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // Set the ESP WiFi station configuration.
    err = SetESPStationConfig(netInfo);
    SuccessOrExit(err);

    // Schedule a call to the ConnectivityManager's DriveStationState methods to adjust the station
    // state based on the new provision.
    SystemLayer.ScheduleWork(ConnectivityMgr.DriveStationState, NULL);

    // Send an AddNetworkComplete message back to the requestor.
    NetworkProvisioningSvr.SendAddNetworkComplete(kWiFiStationNetworkId);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleUpdateNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;

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

    // If the network id field isn't present, immediately reply with an error.
    if (!netInfo.NetworkIdPresent)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || netInfo.NetworkId != ConnectivityMgr.GetWiFiStationNetworkId())
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Delegate to the ConnectivityManager to check the validity of the new WiFi station provision.
    // If the new provision is not acceptable, respond to the requestor with an appropriate
    // StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = NetworkProvisioningSvr.SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // Set the ESP WiFi station configuration.
    err = SetESPStationConfig(netInfo);
    SuccessOrExit(err);

    // Schedule a call to the ConnectivityManager's DriveStationState methods to adjust the station
    // state based on the new provision.
    SystemLayer.ScheduleWork(ConnectivityMgr.DriveStationState, NULL);

    // Respond with a Success response.
    err = NetworkProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleRemoveNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_config_t stationConfig;

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Clear the ESP WiFi station configuration.
    memset(&stationConfig, 0, sizeof(stationConfig));
    esp_wifi_set_config(ESP_IF_WIFI_STA, &stationConfig);

    // Schedule a call to the ConnectivityManager's DriveStationState methods to adjust the station state.
    SystemLayer.ScheduleWork(DriveStationState, NULL);

    // Respond with a Success response.
    err = NetworkProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleGetNetworks(uint8_t flags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_config_t stationConfig;
    NetworkInfo netInfo;
    PacketBuffer * respBuf = NULL;
    TLVWriter writer;

    respBuf = PacketBuffer::New();
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(respBuf);

    err = esp_wifi_get_config(ESP_IF_WIFI_STA, &stationConfig);
    SuccessOrExit(err);

    if (stationConfig.sta.ssid[0] != 0)
    {
        netInfo.Reset();
        netInfo.NetworkId = kWiFiStationNetworkId;
        netInfo.NetworkIdPresent = true;
        netInfo.NetworkType = kNetworkType_WiFi;
        memcpy(netInfo.WiFiSSID, stationConfig.sta.ssid, min(strlen((char *)stationConfig.sta.ssid) + 1, sizeof(netInfo.WiFiSSID)));
        netInfo.WiFiMode = kWiFiMode_Managed;
        netInfo.WiFiRole = kWiFiRole_Station;
        printf("stationConfig.sta.threshold.authmode = %d\n", (int)stationConfig.sta.threshold.authmode);
        // TODO: this is broken
        switch (stationConfig.sta.threshold.authmode)
        {
        case WIFI_AUTH_OPEN:
            netInfo.WiFiSecurityType = kWiFiSecurityType_None;
            break;
        case WIFI_AUTH_WEP:
            netInfo.WiFiSecurityType = kWiFiSecurityType_WEP;
            break;
        case WIFI_AUTH_WPA_PSK:
            netInfo.WiFiSecurityType = kWiFiSecurityType_WPAPersonal;
            break;
        case WIFI_AUTH_WPA2_PSK:
            netInfo.WiFiSecurityType = kWiFiSecurityType_WPA2Personal;
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            netInfo.WiFiSecurityType = kWiFiSecurityType_WPA2MixedPersonal;
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            netInfo.WiFiSecurityType = kWiFiSecurityType_WPA2Enterprise;
            break;
        default:
            netInfo.WiFiSecurityType = kWiFiSecurityType_NotSpecified;
            break;
        }
        netInfo.WiFiKeyLen = min(strlen((char *)stationConfig.sta.password), sizeof(netInfo.WiFiKey));
        memcpy(netInfo.WiFiKey, stationConfig.sta.password, netInfo.WiFiKeyLen);

        err = NetworkInfo::EncodeArray(writer, &netInfo, 1);
        SuccessOrExit(err);
    }
    else
    {
        err = NetworkInfo::EncodeArray(writer, NULL, 0);
        SuccessOrExit(err);
    }

    err = writer.Finalize();
    SuccessOrExit(err);

    err = NetworkProvisioningSvr.SendGetNetworksComplete(1, respBuf);
    respBuf = NULL;
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(respBuf);
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleEnableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Tell the ConnectivityManager to enable the WiFi station interface.
    // Note that any effects of enabling the WiFi station interface (e.g. connecting to an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
    SuccessOrExit(err);

    // Respond with a Success response.
    err = NetworkProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleDisableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that the specified network exists.
    if (!ConnectivityMgr.IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    // Tell the ConnectivityManager to disable the WiFi station interface.
    // Note that any effects of disabling the WiFi station interface (e.g. disconnecting from an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr.SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled);
    SuccessOrExit(err);

    // Respond with a Success response.
    err = NetworkProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleTestConnectivity(uint32_t networkId)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::HandleSetRendezvousMode(uint16_t rendezvousMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If any modes other than EnableWiFiRendezvousNetwork or EnableThreadRendezvous
    // were specified, fail with Common:UnsupportedMessage.
    if ((rendezvousMode & ~(kRendezvousMode_EnableWiFiRendezvousNetwork | kRendezvousMode_EnableThreadRendezvous)) != 0)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_Common, kStatus_UnsupportedMessage);
        ExitNow();
    }

    // If EnableThreadRendezvous was requested, fail with NetworkProvisioning:UnsupportedNetworkType.
    if ((rendezvousMode & kRendezvousMode_EnableThreadRendezvous) != 0)
    {
        err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // If the request is to start the WiFi "rendezvous network" (a.k.a. the WiFi AP interface)...
    if (rendezvousMode != 0)
    {
        // If the AP interface has been expressly disabled by the application, fail with Common:NotAvailable.
        if (ConnectivityMgr.GetWiFiAPMode() == ConnectivityManager::kWiFiAPMode_Disabled)
        {
            err = NetworkProvisioningSvr.SendStatusReport(kWeaveProfile_Common, kStatus_NotAvailable);
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
    err = NetworkProvisioningSvr.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

// ==================== ConnectivityManager::NetworkProvisioningDelegate Private Methods ====================

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::ValidateWiFiStationProvision(
        const ::WeavePlatform::Internal::NetworkInfo & netInfo,
        uint32_t & statusProfileId, uint16_t & statusCode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (netInfo.NetworkType != kNetworkType_WiFi)
    {
        ESP_LOGE(TAG, "ConnectivityManager: Unsupported WiFi station network type: %d", netInfo.NetworkType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedNetworkType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSSID[0] == 0)
    {
        ESP_LOGE(TAG, "ConnectivityManager: Missing WiFi station SSID");
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiMode != kWiFiMode_Managed)
    {
        if (netInfo.WiFiMode == kWiFiMode_NotSpecified)
        {
            ESP_LOGE(TAG, "ConnectivityManager: Missing WiFi station mode");
        }
        else
        {
            ESP_LOGE(TAG, "ConnectivityManager: Unsupported WiFi station mode: %d", netInfo.WiFiMode);
        }
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiRole != kWiFiRole_Station)
    {
        if (netInfo.WiFiRole == kWiFiRole_NotSpecified)
        {
            ESP_LOGE(TAG, "ConnectivityManager: Missing WiFi station role");
        }
        else
        {
            ESP_LOGE(TAG, "ConnectivityManager: Unsupported WiFi station role: %d", netInfo.WiFiRole);
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
        ESP_LOGE(TAG, "ConnectivityManager: Unsupported WiFi station security type: %d", netInfo.WiFiSecurityType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedWiFiSecurityType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSecurityType != kWiFiSecurityType_None && netInfo.WiFiKeyLen == 0)
    {
        ESP_LOGE(TAG, "NetworkProvisioning: Missing WiFi Key");
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

WEAVE_ERROR ConnectivityManager::NetworkProvisioningDelegate::SetESPStationConfig(const ::WeavePlatform::Internal::NetworkInfo  & netInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    wifi_mode_t wifiMode;
    wifi_config_t wifiConfig;
    bool restoreMode = false;

    // Inspect the current ESP wifi mode.  If the station interface is not enabled, enable it now.
    // The ESP wifi station interface must be enabled before esp_wifi_set_config(ESP_IF_WIFI_STA,...)
    // can be called.
    if (esp_wifi_get_mode(&wifiMode) == ESP_OK && (wifiMode != WIFI_MODE_STA && wifiMode != WIFI_MODE_APSTA))
    {
        err = ChangeESPWiFiMode(ESP_IF_WIFI_STA, true);
        SuccessOrExit(err);
        restoreMode = true;
    }

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
    printf("wifiConfig.sta.threshold.authmode = %d\n", (int)wifiConfig.sta.threshold.authmode);

    // Configure the ESP WiFi interface.
    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config() failed: %s", nl::ErrorStr(err));
    }
    SuccessOrExit(err);

    ESP_LOGI(TAG, "WiFi station provision set (SSID: %s)", CONFIG_DEFAULT_WIFI_SSID);

exit:
    if (restoreMode)
    {
        WEAVE_ERROR setModeErr = esp_wifi_set_mode(wifiMode);
        if (setModeErr != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_wifi_set_mode() failed: %s", nl::ErrorStr(setModeErr));
        }
    }
    return err;
}

// ==================== Local Utility Functions ====================

namespace {

const char *ESPWiFiModeToStr(wifi_mode_t wifiMode)
{
    switch (wifiMode)
    {
    case WIFI_MODE_NULL:
        return "NULL";
    case WIFI_MODE_STA:
        return "STA";
    case WIFI_MODE_AP:
        return "AP";
    case WIFI_MODE_APSTA:
        return "STA+AP";
    default:
        return "(unknown)";
    }
}

WEAVE_ERROR ChangeESPWiFiMode(esp_interface_t intf, bool enabled)
{
    WEAVE_ERROR err;
    wifi_mode_t curWiFiMode, targetWiFiMode;
    bool stationEnabled, apEnabled;

    VerifyOrExit(intf == ESP_IF_WIFI_STA || intf == ESP_IF_WIFI_AP, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = esp_wifi_get_mode(&curWiFiMode);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_get_mode() failed: %s", nl::ErrorStr(err));
    }
    SuccessOrExit(err);

    stationEnabled = (curWiFiMode == WIFI_MODE_STA || curWiFiMode == WIFI_MODE_APSTA);
    apEnabled = (curWiFiMode == WIFI_MODE_AP || curWiFiMode == WIFI_MODE_APSTA);

    if (intf == ESP_IF_WIFI_STA)
    {
        stationEnabled = enabled;
    }
    else
    {
        apEnabled = enabled;
    }

    targetWiFiMode = (stationEnabled)
        ? ((apEnabled) ? WIFI_MODE_APSTA : WIFI_MODE_STA)
        : ((apEnabled) ? WIFI_MODE_AP : WIFI_MODE_NULL);

    if (targetWiFiMode != curWiFiMode)
    {
        ESP_LOGI(TAG, "Changing ESP WiFi mode: %s -> %s", ESPWiFiModeToStr(curWiFiMode), ESPWiFiModeToStr(targetWiFiMode));

        err = esp_wifi_set_mode(targetWiFiMode);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_wifi_set_mode() failed: %s", nl::ErrorStr(err));
        }
        SuccessOrExit(err);
    }

exit:
    return err;
}

} // unnamed namespace

} // namespace WeavePlatform

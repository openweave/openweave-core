#include <WeavePlatform-ESP32-Internal.h>

#include "esp_wifi.h"

namespace WeavePlatform {

namespace Internal {

static WEAVE_ERROR ChangeESPWiFiMode(esp_interface_t intf, bool enabled);

} // namespace Internal

using namespace Internal;
using namespace nl::Weave;

WEAVE_ERROR ConnectivityManager::Init()
{
    WEAVE_ERROR err;

    mWiFiStationState = kWiFiStationState_Disabled;
    mWiFiStationReconnectIntervalMS = 5000; // TODO: make configurable
    mLastStationConnectTime = 0;

    err = SystemLayer.ScheduleWork(DoDriveStationState, NULL);
    SuccessOrExit(err);

exit:
    return err;
}

void ConnectivityManager::OnPlatformEvent(const struct WeavePlatformEvent * event)
{
    WEAVE_ERROR err;

    if (event->Type == WeavePlatformEvent::kType_ESPSystemEvent)
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
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
            {
                ESP_LOGE(TAG, "Error returned by MessageLayer.RefreshEndpoints(): %s", nl::ErrorStr(err));
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            DriveStationState();
            break;
        case SYSTEM_EVENT_STA_STOP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
            DriveStationState();
        default:
            break;
        }
    }
}

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

                ESP_LOGI(TAG, "Waiting %" PRId32 " ms for WiFi station reconnect", timeToNextConnect);

                err = SystemLayer.StartTimer(timeToNextConnect, DoDriveStationState, NULL);
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
        SetWifiStationMode(kWiFiStationMode_Disabled);
    }
}

ConnectivityManager::WiFiStationMode ConnectivityManager::GetWifiStationMode(void) const
{
    bool autoConnect;
    return (esp_wifi_get_auto_connect(&autoConnect) == ESP_OK && autoConnect)
            ? kWiFiStationMode_Enabled : kWiFiStationMode_Disabled;
}

bool ConnectivityManager::IsWiFiStationEnabled(void) const
{
    return GetWifiStationMode() == kWiFiStationMode_Enabled;
}

WEAVE_ERROR ConnectivityManager::SetWifiStationMode(WiFiStationMode val)
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

        SystemLayer.ScheduleWork(DoDriveStationState, NULL);
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

    SystemLayer.ScheduleWork(DoDriveStationState, NULL);
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

void ConnectivityManager::DoDriveStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError)
{
    ConnectivityMgr.DriveStationState();
}

namespace Internal {

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

} // namespace Internal

} // namespace WeavePlatform

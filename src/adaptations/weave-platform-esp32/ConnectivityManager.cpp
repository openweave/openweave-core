#include <WeavePlatform-ESP32-Internal.h>

#include "esp_wifi.h"

namespace WeavePlatform {

using namespace Internal;
using namespace nl::Weave;

WEAVE_ERROR ConnectivityManager::Init()
{
    mStationState = kStationState_Disconnected;
    mStationReconnectIntervalMS = 5000; // TODO: make configurable

    esp_wifi_get_auto_connect(&mStationEnabled);

    RefreshStationProvisionedState();

    ESP_LOGI(TAG, "mStationEnabled = %s", mStationEnabled ? "true" : "false");
    ESP_LOGI(TAG, "mStationProvisioned = %s", mStationProvisioned ? "true" : "false");

    return WEAVE_NO_ERROR;
}

void ConnectivityManager::OnPlatformEvent(const struct WeavePlatformEvent * event)
{
    WEAVE_ERROR err;

    if (event->Type == WeavePlatformEvent::kType_ESPSystemEvent)
    {
        switch(event->ESPSystemEvent.event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            RefreshStationProvisionedState();
            AdvanceStationState(mStationState);
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            AdvanceStationState(kStationState_Connected);
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
            AdvanceStationState(kStationState_Disconnected);
            break;
        default:
            break;
        }
    }
}

void ConnectivityManager::AdvanceStationState(StationState newState)
{
    WEAVE_ERROR err;

    if (GetWiFiStationEnabled() && IsStationProvisioned())
    {
        if (newState == kStationState_Disconnected)
        {
            if (mStationState == kStationState_Connecting)
            {
                newState = kStationState_ReconnectWait;
            }

            else
            {
                newState = kStationState_Connecting;
            }
        }
    }
    else
    {
        if (newState == kStationState_Connecting || newState == kStationState_Connected)
        {
            newState = kStationState_Disconnecting;
        }
    }

    if (newState != mStationState)
    {
        mStationState = newState;

        switch (newState)
        {
        case kStationState_Connecting:
            ESP_LOGI(TAG, "Attempting to connect WiFi station interface");
            err = esp_wifi_connect();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_connect() failed: %s", nl::ErrorStr(err));
                AdvanceStationState(kStationState_Disconnected);
            }
            break;
        case kStationState_Connected:
            ESP_LOGI(TAG, "WiFi station interface connected");
            OnStationConnected();
            break;
        case kStationState_Disconnecting:
            ESP_LOGI(TAG, "Disconnecting WiFi station interface");
            err = esp_wifi_disconnect();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "esp_wifi_disconnect() failed: %s", nl::ErrorStr(err));
                AdvanceStationState(kStationState_Disconnected);
            }
            break;
        case kStationState_Disconnected:
            ESP_LOGI(TAG, "WiFi station interface disconnected");
            OnStationDisconnected();
            break;
        case kStationState_ReconnectWait:
            ESP_LOGI(TAG, "Waiting %" PRId32 "ms for WiFi station reconnect", mStationReconnectIntervalMS);
            err = SystemLayer.StartTimer(mStationReconnectIntervalMS, HandleStationReconnectTimer, NULL);
            if (err != WEAVE_NO_ERROR)
            {
                ESP_LOGE(TAG, "SystemLayer.StartTimer() failed: %s", nl::ErrorStr(err));
                AdvanceStationState(kStationState_Disconnected);
            }
            break;
        default:
            WeaveDie();
        }
    }
}

void ConnectivityManager::SetWiFiStationEnabled(bool val)
{
    esp_wifi_get_auto_connect(&mStationEnabled);

    if (val != mStationEnabled)
    {
        mStationEnabled = val;
        esp_wifi_set_auto_connect(val);

        ESP_LOGI(TAG, "WiFi station interface %s", (val) ? "enabled" : "disabled");

        SystemLayer.ScheduleWork(AdvanceStationState, NULL);
    }
}

void ConnectivityManager::ClearStationProvision(void)
{
    wifi_config_t stationConfig;

    memset(&stationConfig, 0, sizeof(stationConfig));
    esp_wifi_set_config(ESP_IF_WIFI_STA, &stationConfig);

    mStationProvisioned = false;

    SystemLayer.ScheduleWork(AdvanceStationState, NULL);
}

void ConnectivityManager::HandleStationReconnectTimer(System::Layer * aLayer, void * aAppState, System::Error aError)
{
    ConnectivityMgr.AdvanceStationState(kStationState_Connecting);
}

void ConnectivityManager::OnStationConnected()
{
    // TODO: alert other systems of connected state

    // Assign an IPv6 link local address to the station interface.
    tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
}

void ConnectivityManager::OnStationDisconnected()
{
    // TODO: alert other systems of disconnected state
}

void ConnectivityManager::RefreshStationProvisionedState(void)
{
    WEAVE_ERROR err;
    wifi_config_t stationConfig;

    err = esp_wifi_get_config(ESP_IF_WIFI_STA, &stationConfig);
    mStationProvisioned = (err == ERR_OK && stationConfig.sta.ssid[0] != 0);
}

void ConnectivityManager::AdvanceStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError)
{
    ConnectivityMgr.AdvanceStationState(ConnectivityMgr.mStationState);
}

} // namespace WeavePlatform

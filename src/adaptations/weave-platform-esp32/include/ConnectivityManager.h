#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

namespace WeavePlatform {

namespace Internal {

struct WeavePlatformEvent;

extern void DispatchEvent(const WeavePlatformEvent * event);

} // namespace Internal

class ConnectivityManager
{
    friend bool InitWeaveStack();
    friend void Internal::DispatchEvent(const struct Internal::WeavePlatformEvent * event);

public:
    enum WiFiStationMode
    {
        kWiFiStationMode_Disabled           = 0,
        kWiFiStationMode_Enabled            = 1,

        kWiFiStationMode_NotSupported       = -1,
    };

    WiFiStationMode GetWifiStationMode(void) const;
    WEAVE_ERROR SetWifiStationMode(WiFiStationMode val);
    bool IsWiFiStationEnabled(void) const;

    uint32_t GetWiFiStationReconnectIntervalMS(void) const;
    WEAVE_ERROR SetWiFiStationReconnectIntervalMS(uint32_t val) const;

    bool IsWiFiStationProvisioned(void) const;
    void ClearWiFiStationProvision(void);

private:
    enum WiFiStationState
    {
        kWiFiStationState_Disabled,
        kWiFiStationState_NotConnected,
        kWiFiStationState_Connected,
    };

    uint64_t mLastStationConnectTime;
    WiFiStationState mWiFiStationState;
    uint32_t mWiFiStationReconnectIntervalMS;

    WEAVE_ERROR Init();

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);
    void DriveStationState();
    void OnStationConnected(void);
    void OnStationDisconnected(void);

    static void DoDriveStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError);

};

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void) const
{
    return mWiFiStationReconnectIntervalMS;
}

} // namespace WeavePlatform

#endif // CONNECTIVITY_MANAGER_H

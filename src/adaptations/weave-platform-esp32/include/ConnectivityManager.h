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
        kWiFiStationMode_Disabled                   = 0,
        kWiFiStationMode_Enabled                    = 1,

        kWiFiStationMode_NotSupported               = -1,
    };

    enum WiFiAPMode
    {
        kWiFiAPMode_Disabled                        = 0,
        kWiFiAPMode_Enabled                         = 1,
        kWiFiAPMode_OnDemand                        = 2,
        kWiFiAPMode_OnDemand_NoStationProvision     = 3,

        kWiFiAPMode_NotSupported                    = -1,
    };

    WiFiStationMode GetWiFiStationMode(void) const;
    WEAVE_ERROR SetWiFiStationMode(WiFiStationMode val);
    bool IsWiFiStationEnabled(void) const;

    uint32_t GetWiFiStationReconnectIntervalMS(void) const;
    WEAVE_ERROR SetWiFiStationReconnectIntervalMS(uint32_t val) const;

    bool IsWiFiStationProvisioned(void) const;
    void ClearWiFiStationProvision(void);

    WiFiAPMode GetWiFiAPMode(void) const;
    WEAVE_ERROR SetWiFiAPMode(WiFiAPMode val);

    void DemandStartWiFiAP(void);
    void StopOnDemandWiFiAP(void);
    void MaintainOnDemandWiFiAP(void);

    uint32_t GetWiFiAPTimeoutMS(void) const;
    void SetWiFiAPTimeoutMS(uint32_t val);

private:
    enum WiFiStationState
    {
        kWiFiStationState_Disabled,
        kWiFiStationState_NotConnected,
        kWiFiStationState_Connected,
    };

    enum WiFiAPState
    {
        kWiFiAPState_Stopped,
        kWiFiAPState_Starting,
        kWiFiAPState_Started,
        kWiFiAPState_Stopping,
    };

    uint64_t mLastStationConnectTime;
    uint64_t mLastAPDemandTime;
    WiFiStationState mWiFiStationState;
    WiFiAPMode mWiFiAPMode;
    WiFiAPState mWiFiAPState;
    uint32_t mWiFiStationReconnectIntervalMS;
    uint32_t mWiFiAPTimeoutMS;

    WEAVE_ERROR Init();

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);
    void DriveStationState();
    void DriveAPState();
    void OnStationConnected(void);
    void OnStationDisconnected(void);

    static void DriveStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError);
    static void DriveAPState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError);
};

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void) const
{
    return mWiFiStationReconnectIntervalMS;
}

inline ConnectivityManager::WiFiAPMode ConnectivityManager::GetWiFiAPMode(void) const
{
    return mWiFiAPMode;
}

inline uint32_t ConnectivityManager::GetWiFiAPTimeoutMS(void) const
{
    return mWiFiAPTimeoutMS;
}


} // namespace WeavePlatform

#endif // CONNECTIVITY_MANAGER_H

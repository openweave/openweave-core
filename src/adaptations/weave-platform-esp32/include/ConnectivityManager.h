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
    bool GetWiFiStationEnabled(void) const;
    void SetWiFiStationEnabled(bool val);

    uint32_t GetWiFiStationReconnectIntervalMS(void) const;
    void SetWiFiStationReconnectIntervalMS(uint32_t val) const;

    bool IsStationProvisioned(void) const;
    void ClearStationProvision(void);

private:
    enum StationState
    {
        kStationState_Disconnected,
        kStationState_Connecting,
        kStationState_Connected,
        kStationState_Disconnecting,
        kStationState_ReconnectWait,
    };

    StationState mStationState;
    uint32_t mStationReconnectIntervalMS;
    bool mStationEnabled;
    bool mStationProvisioned;

    WEAVE_ERROR Init();

    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);
    void AdvanceStationState(StationState newState);
    void OnStationConnected(void);
    void OnStationDisconnected(void);
    void RefreshStationProvisionedState(void);

    static void HandleStationReconnectTimer(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError);
    static void AdvanceStationState(nl::Weave::System::Layer * aLayer, void * aAppState, nl::Weave::System::Error aError);
};

inline bool ConnectivityManager::GetWiFiStationEnabled(void) const
{
    return mStationEnabled;
}

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void) const
{
    return mStationReconnectIntervalMS;
}

inline bool ConnectivityManager::IsStationProvisioned(void) const
{
    return mStationProvisioned;
}

} // namespace WeavePlatform

#endif // CONNECTIVITY_MANAGER_H

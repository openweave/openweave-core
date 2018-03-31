#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>

namespace WeavePlatform {

namespace Internal {
struct WeavePlatformEvent;
class NetworkInfo;
class NetworkProvisioningServer;
} // namespace Internal

class ConnectivityManager
{
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
    uint32_t GetWiFiStationNetworkId(void) const;

    WiFiAPMode GetWiFiAPMode(void) const;
    WEAVE_ERROR SetWiFiAPMode(WiFiAPMode val);

    void DemandStartWiFiAP(void);
    void StopOnDemandWiFiAP(void);
    void MaintainOnDemandWiFiAP(void);

    uint32_t GetWiFiAPTimeoutMS(void) const;
    void SetWiFiAPTimeoutMS(uint32_t val);

private:

    // NOTE: These members are for internal use by the following friend classes.

    friend class ::WeavePlatform::PlatformManager;
    friend class ::WeavePlatform::Internal::NetworkProvisioningServer;

    WEAVE_ERROR Init();
    ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate * GetNetworkProvisioningDelegate();
    void OnPlatformEvent(const struct ::WeavePlatform::Internal::WeavePlatformEvent * event);

private:

    class NetworkProvisioningDelegate
            : public ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate
    {
    public:
        virtual WEAVE_ERROR HandleScanNetworks(uint8_t networkType);
        virtual WEAVE_ERROR HandleAddNetwork(::nl::Weave::System::PacketBuffer *networkInfoTLV);
        virtual WEAVE_ERROR HandleUpdateNetwork(::nl::Weave::System::PacketBuffer *networkInfoTLV);
        virtual WEAVE_ERROR HandleRemoveNetwork(uint32_t networkId);
        virtual WEAVE_ERROR HandleGetNetworks(uint8_t flags);
        virtual WEAVE_ERROR HandleEnableNetwork(uint32_t networkId);
        virtual WEAVE_ERROR HandleDisableNetwork(uint32_t networkId);
        virtual WEAVE_ERROR HandleTestConnectivity(uint32_t networkId);
        virtual WEAVE_ERROR HandleSetRendezvousMode(uint16_t rendezvousMode);

    private:
        WEAVE_ERROR ValidateWiFiStationProvision(const ::WeavePlatform::Internal::NetworkInfo & netInfo,
                        uint32_t & statusProfileId, uint16_t & statusCode);
        WEAVE_ERROR SetESPStationConfig(const ::WeavePlatform::Internal::NetworkInfo & netInfo);
    };

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

    NetworkProvisioningDelegate mNetProvDelegate;
    uint64_t mLastStationConnectTime;
    uint64_t mLastAPDemandTime;
    WiFiStationState mWiFiStationState;
    WiFiAPMode mWiFiAPMode;
    WiFiAPState mWiFiAPState;
    uint32_t mWiFiStationReconnectIntervalMS;
    uint32_t mWiFiAPTimeoutMS;

    void DriveStationState();
    void DriveAPState();
    void OnStationConnected(void);
    void OnStationDisconnected(void);

    static void DriveStationState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
    static void DriveAPState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
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

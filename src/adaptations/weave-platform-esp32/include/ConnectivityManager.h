#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelConnectionMgr.h>
#include "esp_event.h"

namespace nl {
namespace Inet {
class IPAddress;
} // namespace Inet
} // namespace nl

namespace WeavePlatform {

namespace Internal {

struct WeavePlatformEvent;
class NetworkInfo;
class NetworkProvisioningServer;

extern const char *CharacterizeIPv6Address(const ::nl::Inet::IPAddress & ipAddr);

} // namespace Internal

class ConnectivityManager
{
public:

    enum WiFiStationMode
    {
        kWiFiStationMode_NotSupported               = -1,
        kWiFiStationMode_ApplicationControlled      = 0,
        kWiFiStationMode_Disabled                   = 1,
        kWiFiStationMode_Enabled                    = 2,
    };

    enum WiFiAPMode
    {
        kWiFiAPMode_NotSupported                    = -1,
        kWiFiAPMode_ApplicationControlled           = 0,
        kWiFiAPMode_Disabled                        = 1,
        kWiFiAPMode_Enabled                         = 2,
        kWiFiAPMode_OnDemand                        = 3,
        kWiFiAPMode_OnDemand_NoStationProvision     = 4,
    };

    WiFiStationMode GetWiFiStationMode(void);
    WEAVE_ERROR SetWiFiStationMode(WiFiStationMode val);
    bool IsWiFiStationEnabled(void);
    bool IsWiFiStationApplicationControlled(void) const;

    uint32_t GetWiFiStationReconnectIntervalMS(void) const;
    WEAVE_ERROR SetWiFiStationReconnectIntervalMS(uint32_t val) const;

    bool IsWiFiStationProvisioned(void) const;
    void ClearWiFiStationProvision(void);
    uint32_t GetWiFiStationNetworkId(void) const;

    WiFiAPMode GetWiFiAPMode(void) const;
    WEAVE_ERROR SetWiFiAPMode(WiFiAPMode val);
    bool IsWiFiAPApplicationControlled(void) const;

    void DemandStartWiFiAP(void);
    void StopOnDemandWiFiAP(void);
    void MaintainOnDemandWiFiAP(void);

    uint32_t GetWiFiAPIdleTimeoutMS(void) const;
    void SetWiFiAPIdleTimeoutMS(uint32_t val);

private:

    // NOTE: These members are for internal use by the following friend classes.

    friend class ::WeavePlatform::PlatformManager;
    friend class ::WeavePlatform::Internal::NetworkProvisioningServer;

    WEAVE_ERROR Init(void);
    ::nl::Weave::Profiles::NetworkProvisioning::NetworkProvisioningDelegate * GetNetworkProvisioningDelegate(void);
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

        void StartPendingScan(void);
        void HandleScanDone(void);

    private:
        WEAVE_ERROR GetWiFiStationProvision(::WeavePlatform::Internal::NetworkInfo & netInfo, bool includeCredentials);
        WEAVE_ERROR ValidateWiFiStationProvision(const ::WeavePlatform::Internal::NetworkInfo & netInfo,
                        uint32_t & statusProfileId, uint16_t & statusCode);
        WEAVE_ERROR SetESPStationConfig(const ::WeavePlatform::Internal::NetworkInfo & netInfo);
        bool RejectIfApplicationControlled(bool station);
        static void HandleScanTimeOut(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
    };

    enum WiFiStationState
    {
        kWiFiStationState_Disabled,
        kWiFiStationState_Enabling,
        kWiFiStationState_NotConnected,
        kWiFiStationState_Connecting,
        kWiFiStationState_Connecting_Succeeded,
        kWiFiStationState_Connecting_Failed,
        kWiFiStationState_Connected,
        kWiFiStationState_Disconnecting,
    };

    enum WiFiAPState
    {
        kWiFiAPState_NotActive,
        kWiFiAPState_Activating,
        kWiFiAPState_Active,
        kWiFiAPState_Deactivating,
    };

    NetworkProvisioningDelegate mNetProvDelegate;
    uint64_t mLastStationConnectFailTime;
    uint64_t mLastAPDemandTime;
    WiFiStationMode mWiFiStationMode;
    WiFiStationState mWiFiStationState;
    WiFiAPMode mWiFiAPMode;
    WiFiAPState mWiFiAPState;
    uint32_t mWiFiStationReconnectIntervalMS;
    uint32_t mWiFiAPIdleTimeoutMS;
    bool mScanInProgress;

    void DriveStationState(void);
    void DriveAPState(void);
    WEAVE_ERROR ConfigureWiFiAP(void);
    void OnStationConnected(void);
    void OnStationDisconnected(void);
    void OnStationIPv4AddressAvailable(const system_event_sta_got_ip_t & got_ip);
    void OnStationIPv4AddressLost(void);
    void OnIPv6AddressAvailable(const system_event_got_ip6_t & got_ip);
    void ChangeWiFiStationState(WiFiStationState newState);
    void ChangeWiFiAPState(WiFiAPState newState);

    static const char * WiFiStationModeToStr(WiFiStationMode mode);
    static const char * WiFiStationStateToStr(WiFiStationState state);
    static const char * WiFiAPModeToStr(WiFiAPMode mode);
    static const char * WiFiAPStateToStr(WiFiAPState state);
    static void DriveStationState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
    static void DriveAPState(::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError);
    static void RefreshMessageLayer(void);
    static void HandleServiceTunnelNotification(::nl::Weave::Profiles::WeaveTunnel::WeaveTunnelConnectionMgr::TunnelConnNotifyReasons reason,
            WEAVE_ERROR err, void *appCtxt);
};

inline bool ConnectivityManager::IsWiFiStationApplicationControlled(void) const
{
    return mWiFiStationMode == kWiFiStationMode_ApplicationControlled;
}

inline bool ConnectivityManager::IsWiFiAPApplicationControlled(void) const
{
    return mWiFiAPMode == kWiFiAPMode_ApplicationControlled;
}

inline uint32_t ConnectivityManager::GetWiFiStationReconnectIntervalMS(void) const
{
    return mWiFiStationReconnectIntervalMS;
}

inline ConnectivityManager::WiFiAPMode ConnectivityManager::GetWiFiAPMode(void) const
{
    return mWiFiAPMode;
}

inline uint32_t ConnectivityManager::GetWiFiAPIdleTimeoutMS(void) const
{
    return mWiFiAPIdleTimeoutMS;
}


} // namespace WeavePlatform

#endif // CONNECTIVITY_MANAGER_H

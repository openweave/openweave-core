/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file contains headers for the TunnelAgent module that coordinates routing between
 *      peripheral networks and the Nest Service within the Border Gateway and Mobile devices.
 *
 */

#ifndef _WEAVE_TUNNEL_AGENT_H_
#define _WEAVE_TUNNEL_AGENT_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelConnectionMgr.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

#define TUN_DEFAULT_INTF_NAME                 "weav-tun0"
#define PRIMARY_TUNNEL_DEFAULT_INTF_NAME      "wlan0"
#define BACKUP_TUNNEL_DEFAULT_INTF_NAME       "ppp0"
#define TUN_INTF_NAME_MAX_LEN                 (64)
#define WEAVE_ULA_FABRIC_DEFAULT_PREFIX_LEN   (48)

#define TUNNEL_PACKET_QUEUE_INVALID_INDEX     (-1)

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveTunnel {

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::ServiceDirectory;
#endif

class WeaveTunnelConnectionMgr;

/// Platform provided Weave Addressing and Routing routines
namespace Platform {
    /**
     *  @brief
     *    The set of states for Weave tunnel availability.
     */
    typedef enum TunnelAvailabilityMode
    {
        kMode_Primary           = 1,  /**< Set when the Weave Service Tunnel is available over the primary interface */
        kMode_PrimaryAndBackup  = 2,  /**< Set when the Weave Service Tunnel is available over both priary and backup interface */
        kMode_BackupOnly        = 3,  /**< Set when the Weave Service Tunnel is available over the backup interface only */
    } TunnelAvailabilityMode;

    // Following APIs must be implemented by platform:

    // Perform address/route assignment tasks when the Weave tunnel interface is brought up.

    extern void TunnelInterfaceUp(InterfaceId tunIf);

    // Perform address/route deletion tasks when the Weave tunnel interface is brought down.

    extern void TunnelInterfaceDown(InterfaceId tunIf);

    // Perform address/route assignment tasks when the Service tunnel connection is established.

    extern void ServiceTunnelEstablished(InterfaceId tunIf, TunnelAvailabilityMode tunMode);

    // Perform address and route assignment tasks when the Service tunnel connection is torn down.

    extern void ServiceTunnelDisconnected(InterfaceId tunIf);

    /// Perform address and route assignment tasks when the Service tunnel connection avalability state
    /// changes.

    extern void ServiceTunnelModeChange(InterfaceId tunIf, TunnelAvailabilityMode tunMode);

} // namespace Platform

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
/**
 *  This structure contains the relevant statistics counters that is common
 *  to the Primary and Backup Tunnels
 */
typedef struct WeaveTunnelCommonStatistics
{
    uint64_t     mTxBytesToService;                                        /**< Number of bytes transmitted to the Service. */
    uint64_t     mRxBytesFromService;                                      /**< Number of bytes received from the Service. */
    uint32_t     mTxMessagesToService;                                     /**< Number of messages transmitted to the Service. */
    uint32_t     mRxMessagesFromService;                                   /**< Number of messages received from the Service. */
    uint32_t     mTunnelDownCount;                                         /**< Counter for the Weave Tunnel Down events. */
    uint32_t     mTunnelConnAttemptCount;                                  /**< Counter for the Weave Tunnel Connection attempts. */
    WEAVE_ERROR  mLastTunnelDownError;                                     /**< The Weave error encountered when the tunnel last went down. */
    uint64_t     mLastTimeTunnelWentDown;                                  /**< Last time Weave Tunnel went Down. */
    uint64_t     mLastTimeTunnelEstablished;                               /**< Last time Weave Tunnel was Established. */
} WeaveTunnelCommonStatistics;

/**
 *  This structure contains the relevant statistics counters for the WeaveTunnel.
 */
typedef struct WeaveTunnelStatistics
{
    WeaveTunnelCommonStatistics mPrimaryStats;                             /**< Primary Weave Tunnel statistics counters. */
    uint32_t     mDroppedMessagesCount;                                    /**< Number of dropped messages by the WeaveTunnelAgent. */
    TunnelType   mCurrentActiveTunnel;                                     /**< The Weave tunnel that is currently being used for data traffic. */
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    WeaveTunnelCommonStatistics mBackupStats;                              /**< Backup Weave Tunnel statistics counters. */
    uint32_t     mTunnelFailoverCount;                                     /**< Counter for the Weave Tunnel Failover events. */
    WEAVE_ERROR  mLastTunnelFailoverError;                                 /**< The Weave error encountered when the tunnel last failed over from Primary to Backup. */
    uint64_t     mLastTimeForTunnelFailover;                               /**< Last time Weave Tunnel failed over to Backup. */
    uint64_t     mLastTimeWhenPrimaryAndBackupWentDown;                    /**< Last time when both Primary and Backup Weave Tunnel went down. */
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
} WeaveTunnelStatistics;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

class WeaveTunnelAgent
{
  friend class WeaveTunnelControl;
  friend class WeaveTunnelConnectionMgr;

public:

/// States of the Tunnel Agent in relation to its connection(s) to
/// the Service;
    typedef enum AgentState
    {
        kState_NotInitialized                      = 0, ///<Used to indicate that the Tunnel Agent is not initialized.
        kState_Initialized_NoTunnel                = 1, ///<Used to indicate that the Tunnel Agent is initialized but no tunnel has been established.
        kState_PrimaryTunModeEstablished           = 2, ///<Used to indicate that the Primary tunnel to the Service has been established.
        kState_BkupOnlyTunModeEstablished          = 3, ///<Used to indicate that the Backup tunnel to the Service has been established.
        kState_PrimaryAndBkupTunModeEstablished    = 4, ///<Used to indicate that both the Primary and the Backup tunnel has been established.
    } AgentState;

/// Weave Tunnel flag bits.
    typedef enum WeaveTunnelFlags
    {
        kTunnelFlag_PrimaryEnabled  = 0x01,  ///< Set when the primary tunnel is enabled.
        kTunnelFlag_BackupEnabled   = 0x02,  ///< Set when the backup tunnel is enabled.
    } WeaveTunnelFlags;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/// Service Manager pointer to use to lookup and connect to Service.
    WeaveServiceManager *mServiceMgr;
#endif

    WeaveTunnelAgent(void);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/**
 * Initialize the Tunnel agent. This creates te Tunnel endpoint object, sets up the tunnel
 * interface, initializes member variables, callbacks and WeaveTunnelControl.
 */
    WEAVE_ERROR Init(InetLayer *inet, WeaveExchangeManager *exchMgr,
                     uint64_t dstNodeId, WeaveAuthMode authMode,
                     WeaveServiceManager *svcMgr,
                     const char *intfName = TUN_DEFAULT_INTF_NAME,
                     uint8_t role = kClientRole_BorderGateway,
                     void *appContext = NULL);
#endif

/**
 * Initialize the Tunnel agent. This creates te Tunnel endpoint object, sets up the tunnel
 * interface, initializes member variables, callbacks and WeaveTunnelControl.
 */
    WEAVE_ERROR Init(InetLayer *inet, WeaveExchangeManager *exchMgr,
                     uint64_t dstNodeId, IPAddress dstIPAddr, WeaveAuthMode authMode,
                     const char *intfName = TUN_DEFAULT_INTF_NAME,
                     uint8_t role = kClientRole_BorderGateway,
                     void *appContext = NULL);

/**
 * Set the WeaveAuthMode for the Tunnel.
 *
 */
    void SetAuthMode(const WeaveAuthMode authMode);

/**
 * Set the destination nodeId, IPAddress and port for the Tunnel.
 *
 */
    void SetDestination(const uint64_t nodeId, const IPAddress ipAddr,
                        const uint16_t servicePort = WEAVE_PORT);

/**
 * Set the Tunneling device role(BorderGateway, StandaloneDevice, MobileDevice) for the Tunnel.
 *
 */
    void SetTunnelingDeviceRole(const Role role);

/**
 * Get the TunnelAgent state.
 *
 */
    AgentState GetWeaveTunnelAgentState(void);

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
/**
 *  Configure the TCP user timeout option on the primary tunnel connection.
 */
    WEAVE_ERROR ConfigurePrimaryTunnelTimeout(uint16_t maxTimeoutSecs);
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 *  Configure and enable the TCP keepalive option on the primary
 *  tunnel connection.
 */
    WEAVE_ERROR ConfigureAndEnablePrimaryTunnelTCPKeepAlive(uint16_t keepAliveIntervalSecs,
                                                            uint16_t maxNumProbes);
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Primary Tunnel Liveness interval.
 */
    void ConfigurePrimaryTunnelLivenessInterval(uint16_t livenessIntervalSecs);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 *  Check if the primary tunnel is enabled.
 */
    bool IsPrimaryTunnelEnabled(void) const;

/**
 * Enable Primary Tunnel.
 */
    void EnablePrimaryTunnel(void);

/**
 * Disable Primary Tunnel.
 */
    void DisablePrimaryTunnel(void);

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

/**
 *  Check if the backup tunnel is enabled.
 */
    bool IsBackupTunnelEnabled(void) const;

/**
 * Enable Backup Tunnel.
 */
    void EnableBackupTunnel(void);

/**
 * Disable Backup Tunnel.
 */
    void DisableBackupTunnel(void);

/**
 * Start Primary Tunnel.
 */
    void StartPrimaryTunnel(void);

/**
 * Stop Primary Tunnel.
 */
    void StopPrimaryTunnel(void);

/**
 * Start Backup Tunnel.
 */
    void StartBackupTunnel(void);

/**
 * Stop Backup Tunnel.
 */
    void StopBackupTunnel(void);

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
/**
 *  Configure the TCP user timeout option on the backup tunnel connection.
 */
    WEAVE_ERROR ConfigureBackupTunnelTimeout(uint16_t maxTimeoutSecs);
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 *  Configure and enable the TCP keepalive option on the backup
 *  tunnel connection.
 */
    WEAVE_ERROR ConfigureAndEnableBackupTunnelTCPKeepAlive(uint16_t keepAliveIntervalSecs,
                                                           uint16_t maxNumProbes);
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Backup Tunnel Liveness interval.
 */
    void ConfigureBackupTunnelLivenessInterval(uint16_t livenessIntervalSecs);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Set the primary tunnel interface name
 *
 */
    void SetPrimaryTunnelInterface(const char *primaryIntfName);

/**
 * Set the backup tunnel interface name
 *
 */
    void SetBackupTunnelInterface(const char *backupIntfName);

/**
 * Set the primary tunnel interface type
 *
 */
    void SetPrimaryTunnelInterfaceType(const SrcInterfaceType primaryIntfType);

/**
 * Set the backup tunnel interface type
 *
 */
    void SetBackupTunnelInterfaceType(const SrcInterfaceType backupIntfType);

#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

/**
 * Shutdown the Tunnel Agent. This tears down connection to the Service and closes the TunEndPoint
 * interface after removing addresses and routes associated with the tunnel interface.
 */
    WEAVE_ERROR Shutdown(void);

/**
 * Start the Service Tunnel. This enables the tunnel and tries to establish a connection
 * to the Service.
 */
    WEAVE_ERROR StartServiceTunnel(void);

/**
 * Start the Service Tunnel. This enables the tunnel and tries to establish a connection
 * to the Service.
 */
    WEAVE_ERROR StartServiceTunnel(uint64_t dstNodeId,
                                   IPAddress dstIPAddr,
                                   WeaveAuthMode authMode);

/**
 * Close the Tunnel connection to the Service.
 */
    void StopServiceTunnel(void);

/**
 * Close the Tunnel connection to the Service.
 */
    void StopServiceTunnel(WEAVE_ERROR err);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
/**
 * Get the WeaveTunnel statistics counters.
 *
 * @param[out] tunnelStats  A reference to the WeaveTunnelStatistics structure.
 *
 * @return WEAVE_ERROR  Weave error encountered when getting tunnel statistics.
 */
   WEAVE_ERROR GetWeaveTunnelStatistics(WeaveTunnelStatistics &tunnelStats);
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

/**
 * Function pointer to handler set by a higher layer to act upon various notifications related to
 * the tunnel to the Service.
 *
 * @param[in] reason        The reason for the status notification to the application.
 *
 * @param[in] err           Weave error encountered, if any
 *
 * @param[in] appCtxt       A pointer to an application context object
 *
 * @return void.
 */
    typedef void (*OnServiceTunnelStatusNotifyFunct)(WeaveTunnelConnectionMgr::TunnelConnNotifyReasons reason,
                                                     WEAVE_ERROR err, void *appCtxt);

/**
 * Function pointer to handler set by a higher layer to act upon various notifications related to
 * the tunnel to the Service.
 *
 */
    OnServiceTunnelStatusNotifyFunct OnServiceTunStatusNotify;

/**
 * Function pointer to handler set by a higher layer when a Tunnel Reconnect is received from the Service
 *
 * @param[in] tunType          The tunnel type, Primary or Backup.
 *
 * @param[in] reconnectHost    The hostname provided by Service to reconnect to.
 *
 * @param[in] reconnectPort    The destination port provided by Service to reconnect to.
 *
 * @param[in] appCtxt       A pointer to an application context object
 *
 * @return void.
 */
    typedef void (*OnServiceTunnelReconnectNotifyFunct)(TunnelType tunType,
                                                        const char *reconnectHost,
                                                        const uint16_t reconnectPort,
                                                        void *appCtxt);

    OnServiceTunnelReconnectNotifyFunct OnServiceTunReconnectNotify;

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
/**
 * Function pointer to handler set by a higher layer to decode and log contents of IP packets.
 *
 * @param[in] pkt         A const reference to an IP packet buffer.
 *
 * @param[in] direction   The direction of the tunneled packet(inbound or outbound).
 *
 * @param[in] type        The type of the Tunnel.
 *
 * @param[out] toDrop     indicates whether the packet needs to be dropped based on some application policy.
 *
 * @return void.
 */
    typedef void (*OnPacketTransitFunct)(const PacketBuffer &pkt,
                                         nl::Weave::Profiles::WeaveTunnel::TunnelPktDirection direction,
                                         nl::Weave::Profiles::WeaveTunnel::TunnelType type, bool &toDrop);

    OnPacketTransitFunct OnTunneledPacketTransit;

#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

/**
 * Handler to receive IPv6 packets from the Tunnel EndPoint interface and forward, either to the Service
 * via the Service TCP connection after encapsulating IPv6 packet inside the tunnel header or to the Mobile
 * client over a shortcut tunnel. If the Service connection is not yet up, the message is queued until the
 * connection is set up. For tunneling to the Mobile client device, the nexthop neighbor table is referenced.
 */
    static void RecvdFromTunnelEndPoint(TunEndPoint *tunEP, PacketBuffer *message);

/**
 * Handler to receive tunneled IPv6 packets over the shortcut UDP tunnel between the border gateway and the mobile
 * device and forward to the Tunnel EndPoint interface after decapsulating the raw IPv6 packet from inside the
 * tunnel header.
 */
    static void RecvdFromShortcutUDPTunnel (WeaveMessageLayer *msgLayer, PacketBuffer *message);

/**
 * Get the WeaveTunnelAgentState name
 *
 */
    const char *GetAgentStateName(const AgentState state);

/**
 * Get the system time in milliseconds
 *
 */
    uint64_t GetTimeMsec(void);
private:

    // Node Id of the Service endpoint

    uint64_t mPeerNodeId;

    // IP address of the Service endpoint

    IPAddress mServiceAddress;

    // Port of the Service endpoint

    uint16_t mServicePort;

    // Authentication mode to be used

    WeaveAuthMode mAuthMode;

    // Queued messages for Service; pending until connection established.

    PacketBuffer *queuedMsgs[WEAVE_CONFIG_TUNNELING_MAX_NUM_PACKETS_QUEUED];
    int qFront, qRear;

    // Role; Border gateway or Mobile device

    uint8_t mRole;

    // Flag for enabling/disabling specific Tunnel

    uint8_t mTunnelFlags;

    InetLayer *mInet;                       // Associated InetLayer object

#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
    // Interface name of TunEndPoint

    char mIntfName[TUN_INTF_NAME_MAX_LEN];
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP

    TunEndPoint *mTunEP;

    // Handle to the WeaveExchangeManager object.

    WeaveExchangeManager *mExchangeMgr;

    // Weave Tunnel Connection Manager object for the Primary tunnel

    WeaveTunnelConnectionMgr mPrimaryTunConnMgr;

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    // Weave Tunnel Connection Manager object for the Backup tunnel

    WeaveTunnelConnectionMgr mBackupTunConnMgr;
#endif

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    // Weave Tunnel Control for Tunnel shortcut

    WeaveTunnelControl mTunShortcutControl;
#endif

    // WeaveTunnelAgent state

    AgentState  mTunAgentState;

    // Application context
    void *mAppContext;

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    WeaveTunnelStatistics mWeaveTunnelStats;

    void UpdateOutboundMessageStatistics(const TunnelType tunType, const uint64_t msgLen);
    void UpdateTunnelDownStatistics(const TunnelType tunType, const WEAVE_ERROR conErr);
    WeaveTunnelCommonStatistics *GetCommonTunnelStatistics(const TunnelType tunType);
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

    void SetState(AgentState toState);
    // TunEndPoint management functions

    WEAVE_ERROR CreateTunEndPoint(void);
    WEAVE_ERROR SetupTunEndPoint(void);
    WEAVE_ERROR TeardownTunEndPoint(void);

    // Tunnel management and maintenance functions

    WEAVE_ERROR ConfigureAndInit (InetLayer *inet, WeaveExchangeManager *exchMgr,
                                  uint64_t dstNodeId, IPAddress dstIPAddr, WeaveAuthMode authMode,
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
                                  WeaveServiceManager *svcMgr,
#endif
                                  const char *intfName, uint8_t role, void *appContext);

    // Message encapsulating/decapsulating functions for sending between Tunnel and other interfaces

    WEAVE_ERROR AddTunnelHdrToMsg(PacketBuffer *msg);
    WEAVE_ERROR HandleSendingToService(PacketBuffer *msg);
    WEAVE_ERROR HandleTunneledReceive(PacketBuffer *msg, TunnelType tunType);

    /// Decide based on lookup of nexthop table and send locally
    /// via UDP tunnel or remotely via Service TCP connection.

    WEAVE_ERROR DecideAndSendShortcutOrRemoteTunnel(uint64_t nodeId, PacketBuffer *msg);

    void PopulateTunnelMsgHeader(WeaveMessageInfo *msgInfo, const WeaveTunnelConnectionMgr *connMgr);

    void ParseDestinationIPAddress(const PacketBuffer &inMsg, IPAddress &outDest);

    WEAVE_ERROR SendMessageUponPktTransitAnalysis(const WeaveTunnelConnectionMgr *connMgr,
                                                  TunnelPktDirection pktDir,
                                                  TunnelType tunType,
                                                  WeaveMessageInfo *msgInfo,
                                                  PacketBuffer *msg,
                                                  bool &dropPacket);

    static void ServiceMgrStatusHandler(void* appState, WEAVE_ERROR err, StatusReport *report);

    // Service queue management functions

    void SendQueuedMessages(const WeaveTunnelConnectionMgr *connMgr);
    WEAVE_ERROR EnQueuePacket(PacketBuffer *pkt);
    PacketBuffer *DeQueuePacket(void);
    void DumpQueuedMessages(void);

    // Tunnel Control post-processing functions

    void WeaveTunnelConnectionUp(const WeaveMessageInfo *msgInfo,
                                 const WeaveTunnelConnectionMgr *connMgr);
    void WeaveTunnelConnectionDown(const WeaveTunnelConnectionMgr *connMgr, WEAVE_ERROR conErr);
    void WeaveTunnelServiceReconnectRequested(const WeaveTunnelConnectionMgr *connMgr,
                                              const char *redirectHost, const uint16_t redirectPort);
    void WeaveTunnelDownNotifyAndSetState(WEAVE_ERROR conErr);
    void WeaveTunnelUpNotifyAndSetState(AgentState state,
                                        Platform::TunnelAvailabilityMode tunMode,
                                        WeaveTunnelConnectionMgr::TunnelConnNotifyReasons notifyReason,
                                        WeaveTunnelConnectionMgr *connMgr);
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    void WeaveTunnelModeChangeNotifyAndSetState(AgentState state,
                                                Platform::TunnelAvailabilityMode tunMode,
                                                WeaveTunnelConnectionMgr::TunnelConnNotifyReasons notifyReason,
                                                WEAVE_ERROR conErr);
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    void WeaveTunnelConnectionErrorNotify(const WeaveTunnelConnectionMgr *connMgr, WEAVE_ERROR conErr);

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    void RestartTunnelLivenessTimer(TunnelType tunType);

    void NotifyTunnelLiveness(TunnelType tunType, WEAVE_ERROR err);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

};


} // namespace WeaveTunnel
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif// _WEAVE_TUNNEL_AGENT_H_

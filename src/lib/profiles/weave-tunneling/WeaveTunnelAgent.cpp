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
 *      This file implements the Tunnel Agent class APIs for coordinating
 *      and managing IPv6 packet routing between peripheral network devices
 *      and the Nest Service.
 *
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/MathUtils.h>
#include <Weave/Support/FlagUtils.hpp>
#include <SystemLayer/SystemTimer.h>
#include <Weave/Profiles/time/WeaveTime.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelControl.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#endif

#include <inttypes.h>

using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Inet;

using nl::Weave::System::PacketBuffer;

WeaveTunnelAgent::WeaveTunnelAgent()
{
    mInet                     = NULL;
    mExchangeMgr              = NULL;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    mServiceMgr               = NULL;
#endif

    mPeerNodeId               = 0;
    qFront                    = TUNNEL_PACKET_QUEUE_INVALID_INDEX;
    qRear                     = TUNNEL_PACKET_QUEUE_INVALID_INDEX;
    mTunAgentState            = kState_NotInitialized;
    mPeerNodeId               = kNodeIdNotSpecified;
    mServiceAddress           = IPAddress::Any;
    mServicePort              = WEAVE_PORT;
    mAuthMode                 = kWeaveAuthMode_Unauthenticated;
    mAppContext               = NULL;
}

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/**
 * Initialize the Tunnel agent. This creates te Tunnel endpoint object, sets up the tunnel
 * interface, initializes member variables, callbacks and WeaveTunnelControl.
 *
 * @param[in] inet                         A pointer to the InetLayer object.
 *
 * @param[in] exchMgr                      A pointer to the WeaveExchangeManager object.
 *
 * @param[in] dstNodeId                    Node ID of the destination node.
 *
 * @param[in] authMode                     Weave authentication mode used with peer.
 *
 * @param[in] svcMgr                       Pointer to ServiceManager object for lookup and connection
 *                                         to Service.
 *
 * @param[in] intfName                     Interface name given by user; defaults to "weav-tun0".
 *
 * @param[in] role                         Role assumed by Tunnel Agent; Border Gateway, Standalone or Mobile Device.
 *
 * @param[in] appContext                   A pointer to an application level context object.
 *
 * @return WEAVE_NO_ERROR on success, else a corresponding WEAVE_ERROR type.
 */
WEAVE_ERROR WeaveTunnelAgent::Init (InetLayer *inet, WeaveExchangeManager *exchMgr,
                                    uint64_t dstNodeId, WeaveAuthMode authMode,
                                    WeaveServiceManager *svcMgr,
                                    const char *intfName, uint8_t role, void *appContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(svcMgr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = ConfigureAndInit(inet, exchMgr, dstNodeId, IPAddress::Any, authMode,
                           svcMgr,
                           intfName, role, appContext);

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/**
 * Initialize the Tunnel agent. This creates te Tunnel endpoint object, sets up the tunnel
 * interface, initializes member variables, callbacks and WeaveTunnelControl.
 *
 * @param[in] inet                         A pointer to the InetLayer object.
 *
 * @param[in] exchMgr                      A pointer to the WeaveExchangeManager object.
 *
 * @param[in] dstNodeId                    Node ID of the destination node.
 *
 * @param[in] dstIPAddr                    IP address of the destination node.
 *
 * @param[in] authMode                     Weave authentication mode used with peer.
 *
 * @param[in] intfName                     Interface name given by user; defaults to "weav-tun0".
 *
 * @param[in] role                         Role assumed by Tunnel Agent; Border Gateway, Standalone or Mobile Device.
 *
 * @param[in] appContext                   A pointer to an application level context object.
 *
 * @return WEAVE_NO_ERROR on success, else a corresponding WEAVE_ERROR type.
 */
WEAVE_ERROR WeaveTunnelAgent::Init (InetLayer *inet, WeaveExchangeManager *exchMgr,
                                    uint64_t dstNodeId, IPAddress dstIPAddr, WeaveAuthMode authMode,
                                    const char *intfName, uint8_t role, void *appContext)
{
    return ConfigureAndInit(inet, exchMgr, dstNodeId, dstIPAddr, authMode,
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
                            NULL,
#endif
                            intfName, role, appContext);
}

WEAVE_ERROR WeaveTunnelAgent::ConfigureAndInit (InetLayer *inet, WeaveExchangeManager *exchMgr,
                                                uint64_t dstNodeId, IPAddress dstIPAddr, WeaveAuthMode authMode,
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
                                                WeaveServiceManager *svcMgr,
#endif
                                                const char *intfName, uint8_t role, void *appContext)
{
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    mInet                    = inet;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    mServiceMgr              = svcMgr;
#endif
    mExchangeMgr             = exchMgr;
    mPeerNodeId              = dstNodeId;
    mServiceAddress          = dstIPAddr;
    mRole                    = role;
    mAuthMode                = authMode;
    mAppContext              = appContext;
    memset(queuedMsgs, 0, sizeof(queuedMsgs));
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    memset(&mWeaveTunnelStats, 0, sizeof(mWeaveTunnelStats));
#endif

    EnablePrimaryTunnel();
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    DisableBackupTunnel();
#endif

    // Set the TunnelAgent object pointer in WeaveMessageLayer for local UDP tunneling.

    mExchangeMgr->MessageLayer->AppState = this;
    mExchangeMgr->MessageLayer->OnUDPTunneledMessageReceived = RecvdFromShortcutUDPTunnel;

#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
    if (sizeof(intfName) > TUN_INTF_NAME_MAX_LEN)
    {
        WeaveLogDetail(WeaveTunnel, "Interface name size too big; may be truncated\n");
    }
    strncpy(mIntfName, intfName, sizeof(mIntfName) - 1);
    mIntfName[sizeof(mIntfName) - 1] = '\0';
#endif

    // Create Tunnel EndPoint and populate into member mTunEP

    err = CreateTunEndPoint();
    SuccessOrExit(err);

    err = SetupTunEndPoint();
    SuccessOrExit(err);

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    // Initialize WeaveTunnelControl for the Tunnel Shortcut

    err = mTunShortcutControl.Init(this);
    SuccessOrExit(err);
#endif

    // Initialize the WeaveTunnelConnectionMgr

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    mPrimaryTunConnMgr.Init(this, kType_TunnelPrimary, kSrcInterface_WiFi, PRIMARY_TUNNEL_DEFAULT_INTF_NAME);

    mBackupTunConnMgr.Init(this, kType_TunnelBackup, kSrcInterface_Cellular, BACKUP_TUNNEL_DEFAULT_INTF_NAME);
#else // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    // Initialize the Primary Tunnel ConnectionManager. By default, set the source interface type to WiFi.

    mPrimaryTunConnMgr.Init(this, kType_TunnelPrimary, kSrcInterface_WiFi);

#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    // Register Recv function for TunEndPoint

    mTunEP->OnPacketReceived = RecvdFromTunnelEndPoint;

    // Set the TunEndPoint appState to the WeaveTunnelAgent.

    mTunEP->AppState = this;

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    // Enable Shortcut tunneling advertisments

    mTunShortcutControl.EnableShortcutTunneling();
#endif

    // Set the TunnelAgent state.

    mTunAgentState  = kState_Initialized_NoTunnel;

exit:
    return err;
}

/**
 * Set the WeaveAuthMode for the Tunnel.
 *
 * @note
 *   The application needs to stop and then start the tunnel for this configuration change
 *   to have effect.
 *
 * @param[in] authMode                 Weave authentication mode used with peer.
 *
 */
void WeaveTunnelAgent::SetAuthMode(const WeaveAuthMode authMode)
{
    mAuthMode         = authMode;
}

/**
 * Set the destination nodeId and IPAddress for the Tunnel.
 *
 * @note
 *   The application needs to stop and then start the tunnel for this configuration change
 *   to have effect.
 *
 * @param[in] nodeId                   Node ID of the destination node.
 *
 * @param[in] ipAddr                   IP address of the destination node.
 *
 * @param[in] servicePort              Port  of the destination node.
 */
void WeaveTunnelAgent::SetDestination(const uint64_t nodeId, const IPAddress ipAddr, const uint16_t servicePort)
{
    mPeerNodeId       = nodeId;
    mServiceAddress   = ipAddr;
    mServicePort      = servicePort;
}

/**
 * Set the Tunneling device role(BorderGateway vs Standalone) for the Tunnel.
 *
 * @note
 *   The application needs to stop and then start the tunnel for this configuration change
 *   to have effect.
 *
 * @param[in] role                     Role assumed by Tunnel Agent; Border Gateway, Standalone or Mobile Device.
 */
void WeaveTunnelAgent::SetTunnelingDeviceRole(const Role role)
{
    mRole             = role;
}

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
/**
 * Set the primary tunnel interface name.
 *
 * @param[in] primaryIntfName          Primary interface name for Service tunnel connection.
 *
 */
void WeaveTunnelAgent::SetPrimaryTunnelInterface(const char *primaryIntfName)
{
    mPrimaryTunConnMgr.SetInterfaceName(primaryIntfName);
}

/**
 * Set the primary tunnel interface type
 *
 * @param[in] primaryIntfType          The network technology type of the primary interface for Service tunnel connection.
 */
void WeaveTunnelAgent::SetPrimaryTunnelInterfaceType (const SrcInterfaceType primaryIntfType)
{
    mPrimaryTunConnMgr.SetInterfaceType(primaryIntfType);
}

/**
 * Set the backup tunnel interface name.
 *
 * @param[in] backupIntfName           Backup interface name for Service tunnel connection.
 *
 */
void WeaveTunnelAgent::SetBackupTunnelInterface(const char *backupIntfName)
{
    mBackupTunConnMgr.SetInterfaceName(backupIntfName);
}

/**
 * Set the backup tunnel interface type
 *
 * @param[in] backupIntfType           The network technology type of the backup interface for Service tunnel connection.
 */
void WeaveTunnelAgent::SetBackupTunnelInterfaceType (const SrcInterfaceType backupIntfType)
{
    mPrimaryTunConnMgr.SetInterfaceType(backupIntfType);
}

#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

/**
 * Get the TunnelAgent state.
 *
 * @return AgentState the current state of the WeaveTunnelAgent.
 *
 */
WeaveTunnelAgent::AgentState WeaveTunnelAgent::GetWeaveTunnelAgentState(void)
{
    return mTunAgentState;
}

/**
 * Shutdown the Tunnel Agent. This tears down connection to the Service and closes the TunEndPoint
 * interface after removing addresses and routes associated with the tunnel interface.
 *
 * @return WEAVE_NO_ERROR on success, else a corresponding WEAVE_ERROR type.
 */
WEAVE_ERROR WeaveTunnelAgent::Shutdown(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify that Tunnel Agent was at least initialized

    VerifyOrExit(mTunAgentState != kState_NotInitialized,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    // Stop the tunnel to the Service

    StopServiceTunnel();

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    // Disable Shortcut tunneling advertisments

    mTunShortcutControl.DisableShortcutTunneling();
#endif

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    mPrimaryTunConnMgr.Shutdown();

    mBackupTunConnMgr.Shutdown();

#else // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    // Shutdown the Primary Tunnel ConnectionManager.

    mPrimaryTunConnMgr.Shutdown();

#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    SetState(kState_NotInitialized);

    // Tear down the tun endpoint setup

    err = TeardownTunEndPoint();

exit:
    return err;
}

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
/**
 *  Configure the TCP user timeout option on the primary tunnel connection.
 *
 *  @param[in]  maxTimeoutSecs
 *    The maximum timeout for the TCP connection.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful configuration.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to the TCP endpoint.
 *
 */

WEAVE_ERROR WeaveTunnelAgent::ConfigurePrimaryTunnelTimeout(uint16_t maxTimeoutSecs)
{
    return mPrimaryTunConnMgr.ConfigureConnTimeout(maxTimeoutSecs);
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 *  Configure and enable the TCP keepalive option on the primary
 *  tunnel connection.
 *
 *  @param[in]  keepAliveIntervalSecs
 *    The interval (in seconds) between keepalive probes.  This value also controls
 *    the time between last data packet sent and the transmission of the first keepalive
 *    probe.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful enabling of TCP keepalive probes
 *                                               on the connection.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to the TCP endpoint enable keepalive operation.
 *
 */
WEAVE_ERROR WeaveTunnelAgent::ConfigureAndEnablePrimaryTunnelTCPKeepAlive(uint16_t keepAliveIntervalSecs,
                                                                          uint16_t maxNumProbes)
{
    return mPrimaryTunConnMgr.ConfigureAndEnableTCPKeepAlive(keepAliveIntervalSecs,
                                                             maxNumProbes);
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Primary Tunnel Liveness interval.
 */
void WeaveTunnelAgent::ConfigurePrimaryTunnelLivenessInterval(uint16_t livenessIntervalSecs)
{
    mPrimaryTunConnMgr.ConfigureTunnelLivenessInterval(livenessIntervalSecs);
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 *  Check if the primary tunnel is enabled.
 *
 *  @return true if it is enabled, else false.
 */
bool WeaveTunnelAgent::IsPrimaryTunnelEnabled(void) const
{
    return GetFlag(mTunnelFlags, kTunnelFlag_PrimaryEnabled);
}

/**
 *  Enable the Primary Tunnel.
 *
 *  @note
 *    This is a configuration change only and the tunnel needs to be
 *    explicitly started by calling StartServiceTunnel().
 */
void WeaveTunnelAgent::EnablePrimaryTunnel(void)
{
    SetFlag(mTunnelFlags, kTunnelFlag_PrimaryEnabled, true);
}

/**
 *  Disable the Primary Tunnel.
 *
 *  @note
 *    This is a configuration change only and the tunnel needs to be
 *    explicitly stopped by calling StopServiceTunnel().
 */
void WeaveTunnelAgent::DisablePrimaryTunnel(void)
{
    SetFlag(mTunnelFlags, kTunnelFlag_PrimaryEnabled, false);
}

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

/**
 *  Check if the backup tunnel is enabled.
 *
 *  @return true if it is enabled, else false.
 */
bool WeaveTunnelAgent::IsBackupTunnelEnabled(void) const
{
    return GetFlag(mTunnelFlags, kTunnelFlag_BackupEnabled);
}

/**
 *  Enable the Backup Tunnel.
 *
 *  @note
 *    This is a configuration change only and the tunnel needs to be
 *    explicitly started by calling StartServiceTunnel().
 */
void WeaveTunnelAgent::EnableBackupTunnel(void)
{
    SetFlag(mTunnelFlags, kTunnelFlag_BackupEnabled, true);
}

/**
 *  Disable the Backup Tunnel.
 *
 *  @note
 *    This is a configuration change only and the tunnel needs to be
 *    explicitly stopped by calling StopServiceTunnel().
 */
void WeaveTunnelAgent::DisableBackupTunnel(void)
{
    SetFlag(mTunnelFlags, kTunnelFlag_BackupEnabled, false);
}

/**
 *  Start the Primary Tunnel.
 *
 *
 */
void WeaveTunnelAgent::StartPrimaryTunnel(void)
{
    // Abort the primary tunnel if there is an outstanding connection.

    StopPrimaryTunnel();

    EnablePrimaryTunnel();

    // Try establishing the primary tunnel.

    mPrimaryTunConnMgr.ScheduleConnect(CONNECT_NO_DELAY);
}

/**
 *  Stop the Primary Tunnel.
 *
 */
void WeaveTunnelAgent::StopPrimaryTunnel(void)
{
    DisablePrimaryTunnel();

    // Abort the primary tunnel if there is an outstanding connection.

    mPrimaryTunConnMgr.ServiceTunnelClose(WEAVE_ERROR_TUNNEL_FORCE_ABORT);
}

/**
 *  Enable the Backup Tunnel.
 */
void WeaveTunnelAgent::StartBackupTunnel(void)
{
    // Abort the backup tunnel if there is an outstanding connection.

    StopBackupTunnel();

    EnableBackupTunnel();

    // Try establishing the backup tunnel.

    mBackupTunConnMgr.ScheduleConnect(CONNECT_NO_DELAY);
}

/**
 *  Disable the Backup Tunnel.
 */
void WeaveTunnelAgent::StopBackupTunnel(void)
{
    // Stop the Backup tunnel connection.

    DisableBackupTunnel();

    mBackupTunConnMgr.ServiceTunnelClose(WEAVE_ERROR_TUNNEL_FORCE_ABORT);
}

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
/**
 *  Configure the TCP user timeout option on the backup tunnel connection.
 *
 *  @param[in]  maxTimeoutSecs
 *    The maximum timeout for the TCP connection.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful configuration.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to the TCP endpoint.
 *
 */

WEAVE_ERROR WeaveTunnelAgent::ConfigureBackupTunnelTimeout(uint16_t maxTimeoutSecs)
{
    return mBackupTunConnMgr.ConfigureConnTimeout(maxTimeoutSecs);
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 *  Configure and enable the TCP keepalive option on the primary
 *  tunnel connection.
 *
 *  @param[in]  keepAliveIntervalSecs
 *    The interval (in seconds) between keepalive probes.  This value also controls
 *    the time between last data packet sent and the transmission of the first keepalive
 *    probe.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful enabling of TCP keepalive probes
 *                                               on the connection.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to the TCP endpoint enable keepalive operation.
 *
 */
WEAVE_ERROR WeaveTunnelAgent::ConfigureAndEnableBackupTunnelTCPKeepAlive(uint16_t keepAliveIntervalSecs,
                                                                         uint16_t maxNumProbes)
{
    return mBackupTunConnMgr.ConfigureAndEnableTCPKeepAlive(keepAliveIntervalSecs,
                                                            maxTimeoutSecs);
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Backup Tunnel Liveness interval.
 */
void WeaveTunnelAgent::ConfigureBackupTunnelLivenessInterval(uint16_t livenessIntervalSecs)
{
    mBackupTunConnMgr.ConfigureTunnelLivenessInterval(livenessIntervalSecs);
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

/**
 * Start the Service Tunnel. This tries to establish a connection to the Service and also
 * sets the fabric route to the tunnel interface.
 *
 * @return WEAVE_NO_ERROR on success, else a corresponding WEAVE_ERROR type.
 */
WEAVE_ERROR WeaveTunnelAgent::StartServiceTunnel (void)
{
    return StartServiceTunnel(mPeerNodeId, mServiceAddress, mAuthMode);
}

/**
 * Start the Service Tunnel. This tries to establish a connection to the Service and also
 * sets the fabric route to the tunnel interface.
 *
 * @param[in] dstNodeId                    Node ID of the destination node.
 *
 * @param[in] dstIPAddr                    IP address of the destination node.
 *
 * @param[in] authMode                     Weave authentication mode used with peer.
 *
 *
 * @return WEAVE_NO_ERROR
 */
WEAVE_ERROR WeaveTunnelAgent::StartServiceTunnel(uint64_t dstNodeId,
                                                 IPAddress dstIPAddr,
                                                 WeaveAuthMode authMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Set the parameters

    mPeerNodeId              = dstNodeId;
    mServiceAddress          = dstIPAddr;
    mAuthMode                = authMode;

    // Make sure that the Weave Tunnel Agent has been initialized.
    VerifyOrExit(mTunAgentState != kState_NotInitialized,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    // Abort any outstanding connections, if any, and reap resources.
    if (mTunAgentState > kState_Initialized_NoTunnel)
    {
        mPrimaryTunConnMgr.ReleaseResourcesAndStopTunnelConn(WEAVE_ERROR_TUNNEL_FORCE_ABORT);

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
        mBackupTunConnMgr.ReleaseResourcesAndStopTunnelConn(WEAVE_ERROR_TUNNEL_FORCE_ABORT);
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    }

    // Initiate TCP connection with Service and route exchange.

    if (IsPrimaryTunnelEnabled())
    {
        mPrimaryTunConnMgr.ScheduleConnect(CONNECT_NO_DELAY);
    }

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    if (IsBackupTunnelEnabled())
    {
        mBackupTunConnMgr.ScheduleConnect(CONNECT_NO_DELAY);
    }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

exit:
    return err;
}

/**
 * Close the Tunnel connection to the Service.
 *
 * @return void
 */
void WeaveTunnelAgent::StopServiceTunnel(void)
{
    StopServiceTunnel(WEAVE_NO_ERROR);
}

/**
 * Close the Tunnel connection to the Service.
 *
 * @param[in] err WEAVE_NO_ERROR if there is no specific reason for this
 *   StopServiceTunnel request, otherwise the cause of error would be passed down.
 *
 * @return void
 */
void WeaveTunnelAgent::StopServiceTunnel(WEAVE_ERROR err)
{
    // Set the WeaveTunnelAgent state to indicate that tunneling is disabled.

    // Send a Tunnel Close control message

    if (IsPrimaryTunnelEnabled())
    {
        mPrimaryTunConnMgr.ServiceTunnelClose(err);
    }

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    if (IsBackupTunnelEnabled())
    {
        mBackupTunConnMgr.ServiceTunnelClose(err);
    }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

}

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
/**
 * Get the WeaveTunnel statistics counters.
 *
 * @param[out] tunnelStats  A reference to the WeaveTunnelStatistics structure.
 *
 * @return WEAVE_ERROR  Weave error encountered when getting tunnel statistics.
 *
 */
WEAVE_ERROR WeaveTunnelAgent::GetWeaveTunnelStatistics(WeaveTunnelStatistics &tunnelStats)
{
    memcpy(&tunnelStats, &mWeaveTunnelStats, sizeof(WeaveTunnelStatistics));

    return WEAVE_NO_ERROR;
}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

void WeaveTunnelAgent::SetState(AgentState toState)
{

    WeaveLogDetail(WeaveTunnel, "FromState:%s ToState:%s\n", GetAgentStateName(mTunAgentState),
                                 GetAgentStateName(toState));
    mTunAgentState = toState;
}

/**
 * Parse the destination IP address of an IP packet within a PacketBuffer.
 *
 * @param[in]  inMsg        A constant reference to the PacketBuffer object containing the IPv6 packet.
 *
 * @param[out] outDest      A reference to the IPaddress object holding the destination address.
 *
 * @return void
 */
void WeaveTunnelAgent::ParseDestinationIPAddress(const PacketBuffer &inMsg, IPAddress &outDest)
{
    const uint8_t     *p   = NULL;
    struct ip6_hdr *ip6hdr = NULL;
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ip6_addr_t ipv6Addr;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    // Extract the IPv6 header to look at the destination address

    p = inMsg.Start();
    ip6hdr = (struct ip6_hdr *)p;

    // Check destination address
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    ip6_addr_copy(ipv6Addr, ip6hdr->dest);
    outDest = IPAddress::FromIPv6(ipv6Addr);
#else // !WEAVE_SYSTEM_CONFIG_USE_LWIP
    outDest = IPAddress::FromIPv6(ip6hdr->ip6_dst);
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP
}

/**
 * Handler to receive IPv6 packets from the Tunnel EndPoint interface and forward, either to the Service
 * via the Service TCP connection after encapsulating IPv6 packet inside the tunnel header or to the Mobile
 * client over a local tunnel. If the Service connection is not yet up, the message is queued until the
 * connection is set up. For tunneling to the Mobile client device, the nexthop neighbor table is referenced.
 *
 * @param[in] tunEP                        A pointer to the TunEndPoint object.
 *
 * @param[in] message                      A pointer to the PacketBuffer object holding the raw IPv6 packet.
 *
 * @return void
 */
void WeaveTunnelAgent::RecvdFromTunnelEndPoint(TunEndPoint *tunEP, PacketBuffer *msg)
{
    WEAVE_ERROR err             = WEAVE_NO_ERROR;
    uint64_t nodeId;
    IPAddress destIP6Addr;
    WeaveTunnelAgent *tAgent    = static_cast<WeaveTunnelAgent *>(tunEP->AppState);

    tAgent->ParseDestinationIPAddress(*msg, destIP6Addr);

    err = tAgent->AddTunnelHdrToMsg(msg);
    SuccessOrExit(err);

    nodeId = destIP6Addr.InterfaceId();
    if (destIP6Addr.Subnet() == kWeaveSubnetId_Service)
    {
        // Destined for Service

        err = tAgent->HandleSendingToService(msg);
        msg = NULL;
        SuccessOrExit(err);
    }
    else if (destIP6Addr.Subnet() == kWeaveSubnetId_MobileDevice)
    {
        if (tAgent->mRole == kClientRole_BorderGateway)
        {
            // Decide based on lookup of nexthop table and send locally
            // via UDP tunnel or remotely via Service TCP connection.

            err = tAgent->DecideAndSendShortcutOrRemoteTunnel(nodeId, msg);
            msg = NULL;
            SuccessOrExit(err);
        }
    }
    else if ((destIP6Addr.Subnet() == kWeaveSubnetId_PrimaryWiFi) ||
             (destIP6Addr.Subnet() == kWeaveSubnetId_ThreadMesh))
    {
        // Generated locally on Mobile phone; Needs to go via local tunnel or Service

        if (tAgent->mRole == kClientRole_MobileDevice)
        {
            // Decide based on lookup of nexthop table and send locally
            // via UDP tunnel or remotely via Service TCP connection.

            err = tAgent->DecideAndSendShortcutOrRemoteTunnel(tAgent->mExchangeMgr->FabricState->FabricId, msg);
            msg = NULL;
            SuccessOrExit(err);
        }
    }

exit:
    if (msg != NULL)
    {
        PacketBuffer::Free(msg);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        // Update tunnel statistics
        tAgent->mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    }

    return;
}

/**
 * Handler to receive tunneled IPv6 packets from the Service TCP connection and forward to the Tunnel
 * EndPoint interface after decapsulating the raw IPv6 packet from inside the tunnel header.
 *
 * @param[in] con                          A pointer to the WeaveConnection object.
 *
 * @param[in] msgInfo                      A pointer to the WeaveMessageInfo object.
 *
 * @param[in] message                      A pointer to the PacketBuffer object holding the tunneled IPv6 packet.
 *
 * @return void
 */
void WeaveTunnelAgent::RecvdFromShortcutUDPTunnel(WeaveMessageLayer *msgLayer, PacketBuffer *msg)
{
    WeaveTunnelAgent *tAgent = static_cast<WeaveTunnelAgent *>(msgLayer->AppState);

    tAgent->HandleTunneledReceive(msg, kType_TunnelShortcut);

    return;
}

/**
 * Get the WeaveTunnelAgentState name
 *
 */
const char *WeaveTunnelAgent::GetAgentStateName(const AgentState state)
{

    switch (state)
    {
      case kState_NotInitialized                       : return "NotInitialized";
      case kState_Initialized_NoTunnel                 : return "Initialized_NoTunnel";
      case kState_PrimaryTunModeEstablished            : return "PrimaryTunnelEstablished";
      case kState_BkupOnlyTunModeEstablished           : return "BackupTunnelEstablished";
      case kState_PrimaryAndBkupTunModeEstablished     : return "PrimaryAndBackupTunnelEstablished";
    }

    return NULL;
}

/* Create a new Tunnel endpoint */
WEAVE_ERROR WeaveTunnelAgent::CreateTunEndPoint(void)
{
    WEAVE_ERROR res = WEAVE_NO_ERROR;

    res = mInet->NewTunEndPoint(&mTunEP);
    SuccessOrExit(res);

    mTunEP->Init(mInet);

exit:

    return res;
}

/* Setup the TunEndPoint interface and configure the link-local address and fabric default route */
WEAVE_ERROR WeaveTunnelAgent::SetupTunEndPoint(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    err = mTunEP->Open();
#else
    err = mTunEP->Open(mIntfName);
#endif
    SuccessOrExit(err);

    if (!mTunEP->IsInterfaceUp())
    {
        // Bring interface up

        err = mTunEP->InterfaceUp();
        SuccessOrExit(err);
    }

    // Perform address and route additions when tunnel interface is
    // brought up.

    Platform::TunnelInterfaceUp(mTunEP->GetTunnelInterfaceId());

exit:
    if (err != WEAVE_NO_ERROR)
    {
        mTunEP->Free();
        mTunEP = NULL;
    }

    return err;
}

/* Tear down TunEndpoint interface and remove the link-local address and fabric default route */
WEAVE_ERROR WeaveTunnelAgent::TeardownTunEndPoint(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mTunEP != NULL)
    {
        // Perform address and route deletions when tunnel interface is
        // brought down.

        Platform::TunnelInterfaceDown(mTunEP->GetTunnelInterfaceId());

        if (mTunEP->IsInterfaceUp())
        {
            // Bring interface down

            err = mTunEP->InterfaceDown();
        }
        // Free Tunnel Endpoint

        mTunEP->Free();
        mTunEP = NULL;
    }

    return err;
}

/* Utility function for populating a message header */
void WeaveTunnelAgent::PopulateTunnelMsgHeader(WeaveMessageInfo *msgInfo,
                                               const WeaveTunnelConnectionMgr *connMgr)
{
    msgInfo->Clear();

    // Set to no-encryption when not using a tunnel to the Service

    if (!connMgr)
    {
        msgInfo->KeyId            = WeaveKeyId::kNone;
        msgInfo->EncryptionType   = kWeaveEncryptionType_None;
    }
    else
    {
        msgInfo->KeyId = connMgr->mServiceCon->DefaultKeyId;
        msgInfo->EncryptionType = connMgr->mServiceCon->DefaultEncryptionType;
    }

    // Set the source node id

    msgInfo->SourceNodeId = mExchangeMgr->FabricState->LocalNodeId;
}

/* Prepare message for tunneling by encapsulating in the tunnel header */
WEAVE_ERROR WeaveTunnelAgent::AddTunnelHdrToMsg(PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveTunnelHeader tunHeader;

    // Ensure Reserved size for Tunnel header

    msg->EnsureReservedSize(TUN_HDR_SIZE_IN_BYTES);

    // Set version to V1

    tunHeader.Version = kWeaveTunnelVersion_V1;

    // Encapsulate with Tunnel Header and metadata

    err = WeaveTunnelHeader::EncodeTunnelHeader(&tunHeader, msg);

    return err;
}

WEAVE_ERROR WeaveTunnelAgent::SendMessageUponPktTransitAnalysis(const WeaveTunnelConnectionMgr *connMgr,
                                                                TunnelPktDirection pktDir,
                                                                TunnelType tunType,
                                                                WeaveMessageInfo *msgInfo,
                                                                PacketBuffer *msg,
                                                                bool &dropPacket)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgLen = 0;

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    if (OnTunneledPacketTransit)
    {
        // Skip the Tunnel header and send the IP packet.
        OnTunneledPacketTransit(*msg, pktDir, tunType, dropPacket);
    }
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

    if (!dropPacket)
    {
        msgLen = msg->DataLength();
        err = connMgr->mServiceCon->SendTunneledMessage(msgInfo, msg);
        SuccessOrExit(err);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        UpdateOutboundMessageStatistics(connMgr->mTunType, msgLen);
        mWeaveTunnelStats.mCurrentActiveTunnel = connMgr->mTunType;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
        // Received message over tunnel. Restart the liveness timer.

        RestartTunnelLivenessTimer(tunType);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    }

exit:
    return err;
}

/* Prepare message and send to Service via Remote tunnel */
WEAVE_ERROR WeaveTunnelAgent::HandleSendingToService(PacketBuffer *msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool dropPacket = false;

    if (mPrimaryTunConnMgr.mConnectionState != WeaveTunnelConnectionMgr::kState_TunnelOpen
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
        && mBackupTunConnMgr.mConnectionState != WeaveTunnelConnectionMgr::kState_TunnelOpen
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
       )
    {
        // Enqueue message until Service tunnel established

        WeaveLogDetail(WeaveTunnel, "Tunnel connection not up: Enqueuing message\n");
        err = EnQueuePacket(msg);

        if (err == WEAVE_NO_ERROR)
        {
            msg = NULL;
        }

        ExitNow();
    }

    // Send on primary tunnel if open; else send over backup tunnel

    if (mPrimaryTunConnMgr.mConnectionState == WeaveTunnelConnectionMgr::kState_TunnelOpen)
    {
        WeaveMessageInfo msgInfo;

        PopulateTunnelMsgHeader(&msgInfo, &mPrimaryTunConnMgr);

        err = SendMessageUponPktTransitAnalysis(&mPrimaryTunConnMgr, kDir_Outbound, kType_TunnelPrimary,
                                                &msgInfo, msg, dropPacket);
    }
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    else if (mBackupTunConnMgr.mConnectionState == WeaveTunnelConnectionMgr::kState_TunnelOpen)
    {
        WeaveMessageInfo msgInfo;

        PopulateTunnelMsgHeader(&msgInfo, &mBackupTunConnMgr);

        err = SendMessageUponPktTransitAnalysis(&mBackupTunConnMgr, kDir_Outbound, kType_TunnelBackup,
                                                &msgInfo, msg, dropPacket);
    }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

    if (!dropPacket)
    {
        msg = NULL;
    }

exit:
    if (msg != NULL || dropPacket)
    {
        PacketBuffer::Free(msg);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        // Update tunnel statistics
        mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    }

    return err;
}

/* Prepare message and send to Service via Remote tunnel */
WEAVE_ERROR WeaveTunnelAgent::DecideAndSendShortcutOrRemoteTunnel(uint64_t peerId, PacketBuffer *msg)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    bool dropPacket = false;

#if WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED

    // Lookup nexthop table

    if (mTunShortcutControl.IsPeerInShortcutTunnelCache(peerId))
    {
        WeaveMessageInfo msgInfo;

        PopulateTunnelMsgHeader(&msgInfo, NULL);

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
        if (OnTunneledPacketTransit)
        {
            // Skip the Tunnel header and send the IP packet.
            OnTunneledPacketTransit(*msg, kDir_Outbound, kType_TunnelShortcut, dropPacket);
        }
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

        // Send over UDP tunnel

        if (!dropPacket)
        {
            err = mTunShortcutControl.SendMessageOverTunnelShortcut(peerId, &msgInfo, msg);
            msg = NULL;
        }
    }
    else
#endif // WEAVE_CONFIG_TUNNEL_SHORTCUT_SUPPORTED
    {
        // Not found in nexthop table; default to sending to Service

        err = HandleSendingToService(msg);
        msg = NULL;
    }

    if (msg != NULL || dropPacket)
    {
        PacketBuffer::Free(msg);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        // Update tunnel statistics
        mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    }

    return err;
}

/* handle a message received over tunnel and decode tunnel header and send
 * via appropriate interface */
WEAVE_ERROR WeaveTunnelAgent::HandleTunneledReceive(PacketBuffer *msg, TunnelType tunType)
{
    WEAVE_ERROR err =  WEAVE_NO_ERROR;
    IPAddress destIP6Addr;
    WeaveTunnelHeader tunHeader;
    bool dropPacket = false;

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    // Received message over tunnel. Restart the liveness timer.

    RestartTunnelLivenessTimer(tunType);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    WeaveTunnelCommonStatistics *tunStats = NULL;

    // Update tunnel statistics
    tunStats = GetCommonTunnelStatistics(tunType);

    if (tunStats != NULL)
    {
        tunStats->mRxBytesFromService += msg->DataLength();
        tunStats->mRxMessagesFromService++;
    }

    mWeaveTunnelStats.mCurrentActiveTunnel = tunType;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

#if WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK
    if (OnTunneledPacketTransit)
    {
        OnTunneledPacketTransit(*msg, kDir_Inbound, tunType, dropPacket);
    }

    if (dropPacket)
    {
        ExitNow();
    }
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_TRANSIT_CALLBACK

    // Decapsulate Tunnel header

    err = WeaveTunnelHeader::DecodeTunnelHeader(&tunHeader, msg);
    SuccessOrExit(err);

    ParseDestinationIPAddress(*msg, destIP6Addr);

    // Send down Tunnel Endpoint to be routed out to peripheral networks for
    // the Border gateway, or to percolate up the LwIP stack to the application
    // for the Mobile device.

    if (destIP6Addr.Subnet() == kWeaveSubnetId_MobileDevice ||
        destIP6Addr.Subnet() == kWeaveSubnetId_PrimaryWiFi ||
        destIP6Addr.Subnet() == kWeaveSubnetId_ThreadMesh)
    {
        mTunEP->Send(msg);
        msg = NULL;
    }

exit:

    if (msg != NULL || dropPacket)
    {
        WeaveLogProgress(WeaveTunnel, "Msg Rx Err %d", err);
        PacketBuffer::Free(msg);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        // Update tunnel statistics
        mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    }

    return err;
}

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
WeaveTunnelCommonStatistics *WeaveTunnelAgent::GetCommonTunnelStatistics(const TunnelType tunType)
{
    WeaveTunnelCommonStatistics *tunStats = NULL;

    switch (tunType)
    {
      case kType_TunnelPrimary:
        tunStats = &mWeaveTunnelStats.mPrimaryStats;
        break;

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
      case kType_TunnelBackup:
        tunStats = &mWeaveTunnelStats.mBackupStats;
        break;
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
      default:
        break;
    }

    return tunStats;
}

void WeaveTunnelAgent::UpdateOutboundMessageStatistics(const TunnelType tunType, const uint64_t msgLen)
{
    WeaveTunnelCommonStatistics *tunStats = NULL;

    // Update tunnel statistics
    tunStats = GetCommonTunnelStatistics(tunType);

    if (tunStats != NULL)
    {
        tunStats->mTxBytesToService += msgLen;
        tunStats->mTxMessagesToService++;
    }
}

void WeaveTunnelAgent::UpdateTunnelDownStatistics(const TunnelType tunType, const WEAVE_ERROR conErr)
{
    WeaveTunnelCommonStatistics *tunStats = NULL;
    // Update tunnel statistics
    tunStats = GetCommonTunnelStatistics(tunType);

    if (tunStats != NULL)
    {
        tunStats->mTunnelDownCount++;
        tunStats->mLastTunnelDownError = conErr;
        tunStats->mLastTimeTunnelWentDown = GetTimeMsec();
    }
}
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

/* Queue packet for Remote tunnel connection to get established */
WEAVE_ERROR WeaveTunnelAgent::EnQueuePacket(PacketBuffer *pkt)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((qFront == qRear + 1) || (qFront == 0 && qRear == WEAVE_CONFIG_TUNNELING_MAX_NUM_PACKETS_QUEUED - 1))
    {
        // Queue full;

        err = WEAVE_ERROR_TUNNEL_SERVICE_QUEUE_FULL;
        ExitNow();
    }
    else
    {
        if (qFront == TUNNEL_PACKET_QUEUE_INVALID_INDEX)
        {
            qFront = 0;
        }
        qRear = (qRear + 1) % WEAVE_CONFIG_TUNNELING_MAX_NUM_PACKETS_QUEUED;
        queuedMsgs[qRear] = pkt;
    }

exit:

    return err;
}

void WeaveTunnelAgent::DumpQueuedMessages(void)
{
    PacketBuffer *queuedPkt = NULL;

    while (qFront != TUNNEL_PACKET_QUEUE_INVALID_INDEX)
    {
        queuedPkt = DeQueuePacket();
        if (queuedPkt)
        {
            PacketBuffer::Free(queuedPkt);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
            // Update tunnel statistics
            mWeaveTunnelStats.mDroppedMessagesCount++;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

            queuedPkt = NULL;
        }
    }
}

/* Dequeue a packet for sending via Service tunnel */
PacketBuffer *WeaveTunnelAgent::DeQueuePacket(void)
{
    PacketBuffer *retPkt = NULL;
    if (qFront != TUNNEL_PACKET_QUEUE_INVALID_INDEX)
    {
        retPkt = queuedMsgs[qFront];
        if (qFront == qRear) // Only one packet in queue
        {
            qFront = TUNNEL_PACKET_QUEUE_INVALID_INDEX;
            qRear  = TUNNEL_PACKET_QUEUE_INVALID_INDEX;
        }
        else
        {
            qFront = (qFront + 1) % WEAVE_CONFIG_TUNNELING_MAX_NUM_PACKETS_QUEUED;
        }
    }

    return retPkt;
}

/* Flush queued messages that were pending because Service tunnel
 * was not setup */
void WeaveTunnelAgent::SendQueuedMessages(const WeaveTunnelConnectionMgr *connMgr)
{
    WeaveMessageInfo  msgInfo;
    PacketBuffer*     queuedPkt   = NULL;

    while ((queuedPkt = DeQueuePacket()) != NULL)
    {
        PopulateTunnelMsgHeader(&msgInfo, connMgr);

        // Send over TCP Connection

        msgInfo.DestNodeId = connMgr->mServiceCon->PeerNodeId;
        connMgr->mServiceCon->SendTunneledMessage(&msgInfo, queuedPkt);

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
        UpdateOutboundMessageStatistics(connMgr->mTunType, queuedPkt->DataLength());
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

        queuedPkt = NULL;
    }

    return;
}

/* Post processing function after Tunnel has been opened */
void WeaveTunnelAgent::WeaveTunnelConnectionUp(const WeaveMessageInfo *msgInfo,
                                               const WeaveTunnelConnectionMgr *connMgr)
{
   // Update the Weave Tunnel Agent mode on tunnel establishment.

    switch (mTunAgentState)
    {
      case kState_Initialized_NoTunnel:
          // When Primary tunnel is established

          if (connMgr->mTunType == kType_TunnelPrimary)
          {
              WeaveTunnelUpNotifyAndSetState(kState_PrimaryTunModeEstablished,
                                             Platform::kMode_Primary,
                                             WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp,
                                             &mPrimaryTunConnMgr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              // Update tunnel statistics
              mWeaveTunnelStats.mPrimaryStats.mLastTimeTunnelEstablished = GetTimeMsec();
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelPrimary;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          else if (connMgr->mTunType == kType_TunnelBackup)
          {
              WeaveTunnelUpNotifyAndSetState(kState_BkupOnlyTunModeEstablished,
                                             Platform::kMode_BackupOnly,
                                             WeaveTunnelConnectionMgr::kStatus_TunBackupUp,
                                             &mBackupTunConnMgr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              // Update tunnel statistics
              mWeaveTunnelStats.mBackupStats.mLastTimeTunnelEstablished = GetTimeMsec();
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelBackup;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

          break;

      case kState_PrimaryTunModeEstablished:
          if (connMgr->mTunType == kType_TunnelPrimary)
          {
              WeaveTunnelUpNotifyAndSetState(kState_PrimaryTunModeEstablished,
                                             Platform::kMode_Primary,
                                             WeaveTunnelConnectionMgr::kStatus_TunPrimaryUp,
                                             &mPrimaryTunConnMgr);
          }
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          // When BackUp tunnel is established after Primary
          if (connMgr->mTunType == kType_TunnelBackup)
          {
              WeaveTunnelUpNotifyAndSetState(kState_PrimaryAndBkupTunModeEstablished,
                                             Platform::kMode_PrimaryAndBackup,
                                             WeaveTunnelConnectionMgr::kStatus_TunPrimaryAndBackupUp,
                                             &mBackupTunConnMgr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              // Update tunnel statistics
              mWeaveTunnelStats.mBackupStats.mLastTimeTunnelEstablished = GetTimeMsec();
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

          break;

      case kState_BkupOnlyTunModeEstablished:
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          // When Primary tunnel is established after Backup

          if (connMgr->mTunType == kType_TunnelPrimary)
          {
              WeaveTunnelUpNotifyAndSetState(kState_PrimaryAndBkupTunModeEstablished,
                                             Platform::kMode_PrimaryAndBackup,
                                             WeaveTunnelConnectionMgr::kStatus_TunPrimaryAndBackupUp,
                                             &mPrimaryTunConnMgr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              // Update tunnel statistics
              mWeaveTunnelStats.mBackupStats.mLastTimeTunnelEstablished = GetTimeMsec();
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelPrimary;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
          else if (connMgr->mTunType == kType_TunnelBackup)
          {
              WeaveTunnelUpNotifyAndSetState(kState_BkupOnlyTunModeEstablished,
                                             Platform::kMode_BackupOnly,
                                             WeaveTunnelConnectionMgr::kStatus_TunBackupUp,
                                             &mBackupTunConnMgr);
          }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          break;

      case kState_PrimaryAndBkupTunModeEstablished:

          break;

      default:
        break;
    }
}

/* Tunnel connection error notifier */
void WeaveTunnelAgent::WeaveTunnelConnectionErrorNotify(const WeaveTunnelConnectionMgr *connMgr, WEAVE_ERROR conErr)
{
    if (OnServiceTunStatusNotify)
    {
        if (connMgr->mTunType == kType_TunnelPrimary)
        {
            OnServiceTunStatusNotify(WeaveTunnelConnectionMgr::kStatus_TunPrimaryConnError, conErr, mAppContext);
        }
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
        else if (connMgr->mTunType == kType_TunnelBackup)
        {
            OnServiceTunStatusNotify(WeaveTunnelConnectionMgr::kStatus_TunBackupConnError, conErr, mAppContext);
        }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
    }
}

void WeaveTunnelAgent::WeaveTunnelServiceReconnectRequested(const WeaveTunnelConnectionMgr *connMgr,
                                                            const char *reconnectHost,
                                                            const uint16_t reconnectPort)
{
    if (OnServiceTunReconnectNotify)
    {
        // Call application handler to report connection closing.

        OnServiceTunReconnectNotify(connMgr->mTunType, reconnectHost, reconnectPort, mAppContext);
    }
}

/* Post processing function after Tunnel has been closed */
void WeaveTunnelAgent::WeaveTunnelConnectionDown(const WeaveTunnelConnectionMgr *connMgr, WEAVE_ERROR conErr)
{
    // Update the Weave Tunnel mode and the Agent state upon tunnel closure.

    switch (mTunAgentState)
    {
      case kState_Initialized_NoTunnel:

          WeaveTunnelDownNotifyAndSetState(conErr);

          break;

      case kState_PrimaryTunModeEstablished:
          // When Primary tunnel fails

          if (connMgr->mTunType == kType_TunnelPrimary)
          {
              WeaveTunnelDownNotifyAndSetState(conErr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              UpdateTunnelDownStatistics(kType_TunnelPrimary, conErr);
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelUnknown;
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
              mWeaveTunnelStats.mLastTimeWhenPrimaryAndBackupWentDown = GetTimeMsec();
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
          break;

      case kState_BkupOnlyTunModeEstablished:
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          // When Backup tunnel fails

          if (connMgr->mTunType == kType_TunnelBackup)
          {
              WeaveTunnelDownNotifyAndSetState(conErr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              UpdateTunnelDownStatistics(kType_TunnelBackup, conErr);
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelUnknown;
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
              mWeaveTunnelStats.mLastTimeWhenPrimaryAndBackupWentDown = GetTimeMsec();
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          break;

      case kState_PrimaryAndBkupTunModeEstablished:
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          // When Primary tunnel fails

          if (connMgr->mTunType == kType_TunnelPrimary)
          {
              WeaveTunnelModeChangeNotifyAndSetState(kState_BkupOnlyTunModeEstablished,
                                                     Platform::kMode_BackupOnly,
                                                     WeaveTunnelConnectionMgr::kStatus_TunFailoverToBackup,
                                                     conErr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              // Update tunnel statistics
              UpdateTunnelDownStatistics(kType_TunnelPrimary, conErr);
              mWeaveTunnelStats.mTunnelFailoverCount++;
              mWeaveTunnelStats.mLastTimeForTunnelFailover = GetTimeMsec();
              mWeaveTunnelStats.mLastTunnelFailoverError = conErr;
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelBackup;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

          }
          else if (connMgr->mTunType == kType_TunnelBackup)
          {
              WeaveTunnelModeChangeNotifyAndSetState(kState_PrimaryTunModeEstablished,
                                                     Platform::kMode_Primary,
                                                     WeaveTunnelConnectionMgr::kStatus_TunBackupOnlyDown,
                                                     conErr);
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
              UpdateTunnelDownStatistics(kType_TunnelBackup, conErr);
              mWeaveTunnelStats.mCurrentActiveTunnel = kType_TunnelPrimary;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
          }
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          break;

      default:
        break;
    }
}

void WeaveTunnelAgent::WeaveTunnelDownNotifyAndSetState(WEAVE_ERROR conErr)
{
    // Change TunnelAgent state

    SetState(kState_Initialized_NoTunnel);

    // Notify platform about tunnel disconnection

    Platform::ServiceTunnelDisconnected(mTunEP->GetTunnelInterfaceId());

    // When tunnel is down dump all queued messages

    DumpQueuedMessages();

    // Call application handler to report connection closing.

    if (OnServiceTunStatusNotify)
    {
       OnServiceTunStatusNotify(WeaveTunnelConnectionMgr::kStatus_TunDown, conErr, mAppContext);
    }
}

void WeaveTunnelAgent::WeaveTunnelUpNotifyAndSetState(AgentState state,
                                                      Platform::TunnelAvailabilityMode tunMode,
                                                      WeaveTunnelConnectionMgr::TunnelConnNotifyReasons notifyReason,
                                                      WeaveTunnelConnectionMgr *connMgr)
{
    // Change TunnelAgent state

    SetState(state);

    // Perform address and route additions when the Service tunnel connection
    // is established.

    Platform::ServiceTunnelEstablished(mTunEP->GetTunnelInterfaceId(),
                                       tunMode);

    // Check if queue is non-empty; then send queued packets through established tunnel;

    if (qFront != TUNNEL_PACKET_QUEUE_INVALID_INDEX)
    {
        SendQueuedMessages(connMgr);
    }

    // Notify application of successful tunnel establishment

    if (OnServiceTunStatusNotify)
    {
         OnServiceTunStatusNotify(notifyReason, WEAVE_NO_ERROR,
                                  mAppContext);
    }
}

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
void WeaveTunnelAgent::WeaveTunnelModeChangeNotifyAndSetState(AgentState state,
                                                              Platform::TunnelAvailabilityMode tunMode,
                                                              WeaveTunnelConnectionMgr::TunnelConnNotifyReasons notifyReason,
                                                              WEAVE_ERROR conErr)
{
    // Change TunnelAgent state

    SetState(state);

    // Notify platform about tunnel disconnection

    Platform::ServiceTunnelModeChange(mTunEP->GetTunnelInterfaceId(),
                                      tunMode);

    // Call application handler to report connection closing.

    if (OnServiceTunStatusNotify)
    {
       OnServiceTunStatusNotify(notifyReason, conErr, mAppContext);
    }
}
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
void WeaveTunnelAgent::RestartTunnelLivenessTimer(TunnelType tunType)
{
    switch (tunType)
    {
      case kType_TunnelPrimary:
        mPrimaryTunConnMgr.RestartLivenessTimer();
        break;

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
      case kType_TunnelBackup:
        mBackupTunConnMgr.RestartLivenessTimer();
        break;
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
      default:
        break;
    }
}

void WeaveTunnelAgent::NotifyTunnelLiveness(TunnelType tunType, WEAVE_ERROR err)
{
    if (OnServiceTunStatusNotify)
    {
        switch (tunType)
        {
          case kType_TunnelPrimary:
            OnServiceTunStatusNotify(WeaveTunnelConnectionMgr::kStatus_TunPrimaryLiveness, err, mAppContext);
            break;

#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          case kType_TunnelBackup:
            OnServiceTunStatusNotify(WeaveTunnelConnectionMgr::kStatus_TunBackupLiveness, err, mAppContext);
            break;
#endif // WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
          default:
            break;
        }
    }
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Get system time or monotonic time in milliseconds if system time is not available.
 *
 * @note
 *   If GetSystemTimeMs(..) fails we resort to use monotonic time. Fetching
 *   monotonic time in linux based systems uses an unspecified starting point
 *   that may not match with any expected epoch, e.g., system boottime.
 */
uint64_t WeaveTunnelAgent::GetTimeMsec(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t time = 0;

    err = nl::Weave::Platform::Time::GetSystemTimeMs((Profiles::Time::timesync_t *)&time);

    if ((time == 0) || (err != WEAVE_NO_ERROR))
    {
        err = nl::Weave::Platform::Time::GetMonotonicRawTime((Profiles::Time::timesync_t *)&time);

        time = static_cast<uint64_t>(nl::Weave::Platform::DivideBy1000(time));
    }

    return time;
}

#endif // WEAVE_CONFIG_ENABLE_TUNNELING

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
 *      This file implements the Weave Tunnel Connection Manager that
 *      manages the connection state machine.
 *
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef _WEAVE_TUNNEL_CONNECTION_MGR_H_
#define _WEAVE_TUNNEL_CONNECTION_MGR_H_

#include <Weave/Core/WeaveCore.h>
#include <InetLayer/InetInterface.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelControl.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

#define SERVICE_CONN_INTF_NAME_MAX_LEN                 (16)

#define CONNECT_NO_DELAY                               (0)

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveTunnel {

class WeaveTunnelAgent;

/**
 * The reconnect policy parameters that are used to govern the way the
 * tunnel reconnects to the Service.
 */
class ReconnectParam
{
  public:
    void PopulateReconnectParam(WEAVE_ERROR mLastConnectError = WEAVE_NO_ERROR,
                                uint32_t mStatusProfileId = kWeaveProfile_Common,
                                uint16_t mStatusCode = Common::kStatus_Success,
                                uint32_t mMinDelayToConnectSecs = 0);
    WEAVE_ERROR mLastConnectError;
    uint32_t mMinDelayToConnectSecs;
    uint32_t mStatusProfileId;
    uint16_t mStatusCode;
};

/**
 *  @class WeaveTunnelConnectionMgr
 *
 *  @brief
 *    This class encapsulates all the Weave tunnel connection states
 *    and the associated management logic and functions. An instance
 *    of this class would be used to manage the tunnel over each
 *    interface through which the Weave tunnel to the Service would
 *    exist.
 *
 */
class NL_DLL_EXPORT WeaveTunnelConnectionMgr
{
  friend class WeaveTunnelAgent;
  friend class WeaveTunnelControl;

  public:
    typedef enum TunnelConnectionState
    {
        kState_NotConnected            = 0,   /**< Used to indicate that the Weave Tunnel is not connected */
        kState_Connecting              = 1,   /**< Used to indicate that the Weave Tunnel connection has been initiated */
        kState_ConnectionEstablished   = 2,   /**< Used to indicate that the Weave Tunnel connection is established and route information is being exchanged */
        kState_TunnelOpen              = 3,   /**< Used to indicate that the Weave Tunnel is open and ready for data traffic transit */
        kState_TunnelClosing           = 4,   /**< Used to indicate that the Weave Tunnel is closing and the connection is being torn down */
        kState_ReconnectRecvd          = 5,   /**< Used to indicate that the Service wants the border gateway to reconnect after a directory lookup */
    } TunnelConnectionState;

    typedef enum TunnelConnNotifyReasons
    {
        kStatus_TunDown                    = 0,   /**< Used to indicate that the Weave tunnel has gone down */
        kStatus_TunPrimaryUp               = 1,   /**< Used to indicate that the primary Weave tunnel is up */
        kStatus_TunPrimaryConnError        = 2,   /**< Used to indicate that a primary tunnel connection attempt has failed or an existing one was locally aborted or closed by peer */
        kStatus_TunBackupConnError         = 3,   /**< Used to indicate that a backup tunnel connection attempt has failed or an existing one was locally aborted or closed by peer */
        kStatus_TunFailoverToBackup        = 4,   /**< Used to indicate that the primary tunnel is down and switchover to backup tunnel has occurred */
        kStatus_TunBackupOnlyDown          = 5,   /**< Used to indicate that the backup tunnel is down */
        kStatus_TunBackupUp                = 6,   /**< Used to indicate that the Backup Weave tunnel is up */
        kStatus_TunPrimaryAndBackupUp      = 7,   /**< Used to indicate that both the Primary and Backup Weave tunnel are up */
        kStatus_TunPrimaryReconnectRcvd    = 8,   /**< Used to indicate that the Service has requested a reconnect for the Primary Weave tunnel */
        kStatus_TunBackupReconnectRcvd     = 9,   /**< Used to indicate that the Service has requested a reconnect for the Backup Weave tunnel */
        kStatus_TunPrimaryLiveness         = 10,  /**< Used to indicate information about the Tunnel Liveness probe on the Primary Weave tunnel */
        kStatus_TunBackupLiveness          = 11,  /**< Used to indicate information about the Tunnel Liveness probe on the Backup Weave tunnel */

    } TunnelConnNotifyReasons;

    WeaveTunnelConnectionMgr(void);

/**
 * Initialize the WeaveTunnelConnectionMgr.
 */
    WEAVE_ERROR Init(WeaveTunnelAgent *tunAgent, TunnelType tunType, SrcInterfaceType srcIntfType,
                     const char *connIntfName = NULL);

/**
 * Shutdown the WeaveTunnelConnectionMgr.
 *
 */
    void Shutdown(void);

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
/**
 * Configure the TCP user timeout.
 */
    WEAVE_ERROR ConfigureConnTimeout(uint16_t maxTimeoutSecs);
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 * Configure the TCP keepalive parameters.
 */
    WEAVE_ERROR ConfigureAndEnableTCPKeepAlive(uint16_t intervalSecs, uint16_t maxNumProbes);
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Tunnel Liveness interval.
 */
    void ConfigureTunnelLivenessInterval(uint16_t livenessIntervalSecs);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Set InterfaceName for Tunnel connection.
 */
    void SetInterfaceName(const char *intfName);

/**
 * Set SrcInterfaceType for Tunnel connection.
 */
    void SetInterfaceType(const SrcInterfaceType srcIntfType);

/**
 * Handler invoked if the Service Manager failed to establish the TCP connection to the Service.
 */
    static void ServiceMgrStatusHandler(void* appState, WEAVE_ERROR err, StatusReport *report);

/**
 * Handler invoked when Service TCP connection is completed. The device proceeds to initiate Tunnel control
 * commands to the Service from this function.
 */
    static void HandleServiceConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);

/**
 * Handler invoked when Service TCP connection is closed. The device tries to re-establish the connection to
 * the Service if the mServiceConKeepAlive is set to true.
 */
    static void HandleServiceConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

/**
 * Handler to receive tunneled IPv6 packets from the Service TCP connection and forward to the Tunnel
 * EndPoint interface after decapsulating the raw IPv6 packet from inside the tunnel header.
 */
    static void RecvdFromService(WeaveConnection *con, const WeaveMessageInfo *msgInfo,
                                 PacketBuffer *message);

/**
 * @brief Callback to fetch the interval of time to wait before the next
 * tunnel reconnect.
 *
 * @param appState[in]          App state pointer set during initialization of
 *                              the SubscriptionClient.
 * @param reconnectParam[in]    Structure with parameters that influence the calculation of the
 *                              reconnection delay.
 *
 * @param delayMsec[out]        Time in milliseconds to wait before next
 *                              reconnect attempt.
 */
    typedef void (*ConnectPolicyCallback) (void * const appState,
                                           ReconnectParam & reconnectParam,
                                           uint32_t & delayMsec);

    ConnectPolicyCallback mServiceConnDelayPolicyCallback;

    static void DefaultReconnectPolicyCallback(void * const appstate,
                                               ReconnectParam & reconnectParam,
                                               uint32_t & delayMsec);

/**
 * Attempt to establish a connection to the Service.
 */
    WEAVE_ERROR TryConnectingNow(void);

/**
 * Close the Service tunnel.
 */
    void ServiceTunnelClose(WEAVE_ERROR err);

/**
 * Stop Service tunnel connection and attempt to reconnect again.
 */
    void StopAndReconnectTunnelConn(ReconnectParam &reconnParam);

  private:

    WEAVE_ERROR StartServiceTunnelConn(uint64_t destNodeId, IPAddress destIPAddr,
                                       uint16_t destPort,
                                       WeaveAuthMode authMode,
                                       InterfaceId connIntfId);
    void StopServiceTunnelConn(WEAVE_ERROR err);
    void AttemptReconnect(ReconnectParam &reconnParam);
    void DecideOnReconnect(ReconnectParam &reconnParam);
    void ResetCacheAndScheduleConnect(uint32_t delay);
    void ScheduleConnect(uint32_t delay);
    void CancelDelayedReconnect(void);
    void ReleaseResourcesAndStopTunnelConn(WEAVE_ERROR err);
    static void ServiceConnectTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    void StartLivenessTimer(void);
    void StopLivenessTimer(void);
    void RestartLivenessTimer(void);
    static void TunnelLivenessTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    // Pointer to a Weave Tunnel Agent object.

    WeaveTunnelAgent *mTunAgent;

    // Tunnel control object

    WeaveTunnelControl mTunControl;

    // State of the Weave Tunnel Connection

    TunnelConnectionState mConnectionState;

    // Number of failed Service Tunnel connection attempts in a row

    uint16_t mTunFailedConnAttemptsInRow;

    // Maximum number of failed Service connection attempts before notification

    uint16_t mMaxFailedConAttemptsBeforeNotify;

    // The tunnel type of this connection

    TunnelType mTunType;

    // The interface type for this connection

    SrcInterfaceType mSrcInterfaceType;

    // The interface identifier for the tunnel connection to the Service

    char mServiceConIntf[SERVICE_CONN_INTF_NAME_MAX_LEN];

    // Handle to the Weave connection object.

    WeaveConnection *mServiceCon;

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
    // TCP connection keepalive interval (in secs)

    uint16_t mKeepAliveIntervalSecs;

    // The max number of TCP KeepAlive probes

    uint16_t mMaxNumProbes;
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
    // Tunnel TCP connection max user timeout (in secs)

    uint16_t mMaxUserTimeoutSecs;
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    // Tunnel Liveness interval

    uint16_t mTunnelLivenessInterval;
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

};

} // namespace WeaveTunnel
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif//_WEAVE_TUNNEL_CONNECTION_MGR_H_

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

#include <Weave/Profiles/weave-tunneling/WeaveTunnelConnectionMgr.h>

#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelCommon.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/RandUtils.h>
#include <Weave/Support/FibonacciUtils.h>
#include <SystemLayer/SystemTimer.h>

#if WEAVE_CONFIG_ENABLE_TUNNELING

using namespace nl::Inet;
using namespace nl::Weave::Profiles::WeaveTunnel;
using namespace nl::Weave::Profiles::StatusReporting;

using nl::Weave::System::PacketBuffer;

WeaveTunnelConnectionMgr::WeaveTunnelConnectionMgr(void)
{
    mConnectionState            = kState_NotConnected;
    mServiceCon                 = NULL;
    mTunFailedConnAttemptsInRow = 0;
}

/*
 * Initialize the WeaveTunnelConnectionMgr.
 *
 * @param[in] tunAgent              A pointer to the WeaveTunnelAgent object.
 *
 * @return WEAVE_NO_ERROR unconditionally.
 */
WEAVE_ERROR WeaveTunnelConnectionMgr::Init(WeaveTunnelAgent *tunAgent,
                                           TunnelType tunType,
                                           SrcInterfaceType srcIntfType,
                                           const char *connIntfName)
{
    WEAVE_ERROR err                   = WEAVE_NO_ERROR;

    // Check if the WeaveTunnelAgent object is valid.

    VerifyOrExit(tunAgent != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mTunAgent                         = tunAgent;
    mConnectionState                  = kState_NotConnected;
    mServiceCon                       = NULL;
    mTunFailedConnAttemptsInRow       = 0;
    mTunReconnectFibonacciIndex       = 0;
    mTunType                          = tunType;
    mSrcInterfaceType                 = srcIntfType;
    mMaxFailedConAttemptsBeforeNotify = WEAVE_CONFIG_TUNNELING_MAX_NUM_CONNECT_BEFORE_NOTIFY;
    mServiceConnDelayPolicyCallback   = DefaultReconnectPolicyCallback;
    mResetReconnectArmed              = false;
    if (connIntfName)
    {
        strncpy(mServiceConIntf, connIntfName, sizeof(mServiceConIntf) - 1);
        mServiceConIntf[sizeof(mServiceConIntf) - 1] = '\0';
    }
    else
    {
        mServiceConIntf[0] = '\0';
    }

    // Configure default values for TCP User timeout, TCP KeepAlives, and
    // Tunnel Liveness.

    if (tunType == kType_TunnelPrimary)
    {
#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
        mMaxUserTimeoutSecs           = WEAVE_CONFIG_PRIMARY_TUNNEL_MAX_TIMEOUT_SECS;
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
        mKeepAliveIntervalSecs        = WEAVE_CONFIG_PRIMARY_TUNNEL_KEEPALIVE_INTERVAL_SECS;
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
        mTunnelLivenessInterval       = WEAVE_CONFIG_PRIMARY_TUNNEL_LIVENESS_INTERVAL_SECS;
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    }
    else
    {
#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
        mMaxUserTimeoutSecs           = WEAVE_CONFIG_BACKUP_TUNNEL_MAX_TIMEOUT_SECS;
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
        mKeepAliveIntervalSecs        = WEAVE_CONFIG_BACKUP_TUNNEL_KEEPALIVE_INTERVAL_SECS;
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
        mTunnelLivenessInterval       = WEAVE_CONFIG_BACKUP_TUNNEL_LIVENESS_INTERVAL_SECS;
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
    }

    mOnlineCheckInterval = WEAVE_CONFIG_TUNNELING_ONLINE_CHECK_FAST_FREQ_SECS;

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
    mMaxNumProbes                     = WEAVE_CONFIG_TUNNEL_MAX_KEEPALIVE_PROBES;
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

    // Initialize WeaveTunnelControl

    err = mTunControl.Init(mTunAgent);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Shutdown the WeaveTunnelConnectionMgr.
 *
 */
void WeaveTunnelConnectionMgr::Shutdown(void)
{

    StopOnlineCheck();

    // Close the Tunnel Control

    mTunControl.Close();

    // Reset the handle to the TunnelAgent and the Service connection objects.
    mTunAgent = NULL;
    mServiceCon = NULL;

}

/**
 * Set InterfaceName for Tunnel connection.
 *
 * @param[in] tunIntf              The InterfaceName for setting the Service tunnel connection.
 *
 */
void WeaveTunnelConnectionMgr::SetInterfaceName(const char *tunIntf)
{
    if (tunIntf)
    {
        strncpy(mServiceConIntf, tunIntf, sizeof(mServiceConIntf) - 1);
        mServiceConIntf[sizeof(mServiceConIntf) - 1] = '\0';
    }
}

/**
 * Set SrcInterfaceType for Tunnel connection.
 *
 * @param[in] srcIntfType          The network technology type of the interface for Service tunnel connection.
 */
void WeaveTunnelConnectionMgr::SetInterfaceType (const SrcInterfaceType srcIntfType)
{
    mSrcInterfaceType    = srcIntfType;
}

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

/**
 * Configure the TCP user timeout.
 *
 * @param[in]  maxTimeoutSecs
 *    The maximum timeout for the TCP connection.
 *
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful enabling of TCP User Timeout
 *                                               on the connection.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to TCP endpoint setting the user timeout option.
 */
WEAVE_ERROR WeaveTunnelConnectionMgr::ConfigureConnTimeout(uint16_t maxTimeoutSecs)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mServiceCon->SetUserTimeout(maxTimeoutSecs * nl::Weave::System::kTimerFactor_milli_per_unit);
    if (err == INET_ERROR_NOT_IMPLEMENTED)
    {
        err = WEAVE_NO_ERROR;
    }
    VerifyOrExit(err == WEAVE_NO_ERROR,
                 WeaveLogDetail(WeaveTunnel,
                               "Error setting TCP user timeout: %d", err);
                );

    // Now set the member configurations

    mMaxUserTimeoutSecs    = maxTimeoutSecs;

exit:
    return err;
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
/**
 * Configure and Enable the TCP KeepAlive for the tunnel.
 *
 * @param[in]  keepAliveIntervalSecs
 *    The TCP keepalive interval in seconds.
 *
 * @param[in]  maxNumProbes
 *    The maximum number of TCP keepalive probes.
 *
 *  @retval  #WEAVE_NO_ERROR                     on successful enabling of TCP keepalive probes
 *                                               on the connection.
 *  @retval  #WEAVE_ERROR_INCORRECT_STATE        if the WeaveConnection object is not
 *                                               in the correct state for sending messages.
 *  @retval  other Inet layer errors related to the TCP endpoint enable keepalive operation.
 */
WEAVE_ERROR WeaveTunnelConnectionMgr::ConfigureAndEnableTCPKeepAlive(uint16_t intervalSecs, uint16_t maxNumProbes)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Try enabling keepalive on the connection.

    err = mServiceCon->EnableKeepAlive(intervalSecs, maxNumProbes);
    SuccessOrExit(err);

    // Now set the member configurations

    mKeepAliveIntervalSecs = intervalSecs;
    mMaxNumProbes          = maxNumProbes;

exit:
    return err;
}
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
/**
 * Configure the Tunnel Liveness interval.
 */
void WeaveTunnelConnectionMgr::ConfigureTunnelLivenessInterval(uint16_t livenessIntervalSecs)
{
    mTunnelLivenessInterval = livenessIntervalSecs;
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

/**
 * Try to establish a connecttion to the Service either using
 * ServiceManager or directly
 *
 */
WEAVE_ERROR WeaveTunnelConnectionMgr::TryConnectingNow(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InterfaceId connIntfId = INET_NULL_INTERFACEID;
#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    WeaveTunnelCommonStatistics *tunStats = NULL;
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS

    WeaveLogDetail(WeaveTunnel, "TryConnectingNow on %s tunnel", mTunType == kType_TunnelPrimary?"primary":"backup");

    // Get the InterfaceId from the interface name.

    if (mServiceConIntf[0] != '\0')
    {
        // Get the InterfaceId from the interface name.

        err = InterfaceNameToId(mServiceConIntf, connIntfId);
        SuccessOrExit(err);
    }

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    // Initiate TCP connection with Service.

    if (mTunAgent->mServiceMgr)
    {
        err = mTunAgent->mServiceMgr->connect(mTunAgent->mPeerNodeId,
                                              mTunAgent->mAuthMode, this,
                                              ServiceMgrStatusHandler,
                                              HandleServiceConnectionComplete,
                                              WEAVE_CONFIG_TUNNEL_CONNECT_TIMEOUT_SECS * nl::Weave::System::kTimerFactor_milli_per_unit,
                                              connIntfId);
    }
    else
#endif
    {
        err = StartServiceTunnelConn(mTunAgent->mPeerNodeId,
                                     mTunAgent->mServiceAddress,
                                     mTunAgent->mServicePort,
                                     mTunAgent->mAuthMode,
                                     connIntfId);
    }

    SuccessOrExit(err);

    // Change the connection state to connecting.

    mConnectionState = kState_Connecting;

#if WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
    // Update tunnel statistics
    tunStats = mTunAgent->GetCommonTunnelStatistics(mTunType);

    if (tunStats != NULL)
    {
        tunStats->mTunnelConnAttemptCount++;
    }
#endif // WEAVE_CONFIG_TUNNEL_ENABLE_STATISTICS
exit:
    return err;
}

/* Decide whether and how(fast or slow) to reconnect again to the Service */
void WeaveTunnelConnectionMgr::DecideOnReconnect(ReconnectParam &reconnParam)
{
    uint32_t delayMsecs;
    // Exit if we do not need to reconnect.

    if ((mTunType == kType_TunnelPrimary && !mTunAgent->IsPrimaryTunnelEnabled())
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
        ||
        (mTunType == kType_TunnelBackup && !mTunAgent->IsBackupTunnelEnabled())
#endif
       )
    {
        ExitNow();
    }

    // Fetch the backoff delay before connecting.
    mServiceConnDelayPolicyCallback(this, reconnParam, delayMsecs);

    // Retry connecting using a backoff mechanism  upto a maximum number of retries
    // before failover to a backup tunnel connection if one exists.
    //
    // For the first retry do not go to Service directory and reconnect
    // to the same IP address in cache but, thereafter, clear cache and
    // fetch hostname from Service directory before connecting.

    if (mTunFailedConnAttemptsInRow < mMaxFailedConAttemptsBeforeNotify)
    {
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        if (mTunFailedConnAttemptsInRow > 1)
        {
            // Clear the Service directory cache

            if (mTunAgent->mServiceMgr)
            {
                mTunAgent->mServiceMgr->clearCache();
            }
        }
#endif
        // Try to reconnect with Service.

        ScheduleConnect(delayMsecs);
    }
    else
    {
        ResetCacheAndScheduleConnect(delayMsecs);
    }

    // Notify application appropriately

    if  (mTunFailedConnAttemptsInRow == mMaxFailedConAttemptsBeforeNotify)
    {
        // Notify about Tunnel down or failover.

        mTunAgent->WeaveTunnelConnectionDown(this, reconnParam.mLastConnectError);

        // Connection went down; Start network online check at the fast interval.

        SetOnlineCheckIntervalFast(true);

        StartOnlineCheck();
    }
    else
    {
        // Notify connection error

        mTunAgent->WeaveTunnelConnectionErrorNotify(this, reconnParam.mLastConnectError);
    }

exit:
    return;
}

/* Handler for reconnecting to the Service after wait period timeout */
void WeaveTunnelConnectionMgr::ServiceConnectTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    ReconnectParam reconnParam;
    WeaveTunnelConnectionMgr* tConnMgr = static_cast<WeaveTunnelConnectionMgr*>(aAppState);

    // Exit if we do not need to reconnect.
    // We need to check to evaluate if, in the meantime, the application has disabled
    // the Tunnel(by a call to DisablePrimaryTunnel() or DisableBackupTunnel()) or not.

    if ((tConnMgr->mTunType == kType_TunnelPrimary && !tConnMgr->mTunAgent->IsPrimaryTunnelEnabled())
#if WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED
        ||
        (tConnMgr->mTunType == kType_TunnelBackup && !tConnMgr->mTunAgent->IsBackupTunnelEnabled())
#endif
       )
    {
        ExitNow();
    }

    /* Check if the WeaveConnectionManager is in the correct state to effect a
     * reconnect.
     */
    VerifyOrExit(tConnMgr->mConnectionState == kState_NotConnected,
                 /* NO_OP */);

    // Reconnect to Service.

    WeaveLogDetail(WeaveTunnel, "Connecting to node %" PRIx64 "\n",
                   tConnMgr->mTunAgent->mPeerNodeId);

    // Reset the reconnect armed flag.

    tConnMgr->mResetReconnectArmed = false;

    aError = tConnMgr->TryConnectingNow();

    if (aError != WEAVE_NO_ERROR)
    {
        reconnParam.PopulateReconnectParam(aError);

        tConnMgr->StopAndReconnectTunnelConn(reconnParam);
    }

exit:
    return;
}

/* Reset the address cache in the Service Directory and schedule a delayed reconnect. */
void WeaveTunnelConnectionMgr::ResetCacheAndScheduleConnect(uint32_t delay)
{
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        // Reset the Service directory

    if (mTunAgent->mServiceMgr)
    {
        mTunAgent->mServiceMgr->clearCache();
    }
#endif

    ScheduleConnect(delay);
}

/* Schedule a reconnect timer */
void WeaveTunnelConnectionMgr::ScheduleConnect(uint32_t delay)
{
    mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(delay, ServiceConnectTimeout, this);
}

void WeaveTunnelConnectionMgr::CancelDelayedReconnect(void)
{
    mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(ServiceConnectTimeout, this);
}

/* Start the connection to the Service */
WEAVE_ERROR WeaveTunnelConnectionMgr::StartServiceTunnelConn(uint64_t destNodeId, IPAddress destIPAddr,
                                                             uint16_t destPort,
                                                             WeaveAuthMode authMode,
                                                             InterfaceId connIntfId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mServiceCon != NULL && mConnectionState == kState_NotConnected)
    {
        // Remove previous connection(currently closed)

        mServiceCon->Close();
        mServiceCon = NULL;
    }

    // Do nothing if a connect attempt is already in progress.

    VerifyOrExit(mServiceCon == NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create a new WeaveConnection object

    mServiceCon = mTunAgent->mExchangeMgr->MessageLayer->NewConnection();

    VerifyOrExit(mServiceCon != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Setup connection handlers

    mServiceCon->OnConnectionComplete = HandleServiceConnectionComplete;

    // Set app state to WeaveTunnelConnectionMgr

    mServiceCon->AppState = this;

    // Set the connection timeout

    mServiceCon->SetConnectTimeout(WEAVE_CONFIG_TUNNEL_CONNECT_TIMEOUT_SECS * nl::Weave::System::kTimerFactor_milli_per_unit);

    err = mServiceCon->Connect(destNodeId, authMode, destIPAddr, destPort, connIntfId);

exit:
    return err;
}

/* Stop the connection to the Service */
void WeaveTunnelConnectionMgr::StopServiceTunnelConn(WEAVE_ERROR err)
{
    // Close connection to the Service

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (mTunAgent->mServiceMgr)
    {
        mTunAgent->mServiceMgr->cancel(mTunAgent->mPeerNodeId, this);
    }
#endif

    if (mServiceCon)
    {
        if (err == WEAVE_NO_ERROR)
        {
            if (mServiceCon->Close() != WEAVE_NO_ERROR)
            {
                mServiceCon->Abort();
            }
        }
        else
        {
            mServiceCon->Abort();
        }

        mServiceCon = NULL;
    }

    mConnectionState = kState_NotConnected;
}

/**
 * Stop Service tunnel connection and attempt to reconnect again.
 *
 * @param[in]  err    A WEAVE_ERROR passed in from the caller.
 *
 */
void WeaveTunnelConnectionMgr::StopAndReconnectTunnelConn(ReconnectParam & reconnParam)
{
    ReleaseResourcesAndStopTunnelConn(reconnParam.mLastConnectError);

    // Attempt Reconnecting

    AttemptReconnect(reconnParam);
}

/**
 * Close the Service tunnel.
 *
 * @param[in]  err    A WEAVE_ERROR passed in from the caller.
 *
 * @note
 *   The WeaveTunnelConnectionMgr would attempt a graceful close by sending
 *   a Tunnel Close message to the Service if the tunnel is in the
 *   kState_TunnelOpen state; else it will close the TCP connection from its
 *   end.
 */
void WeaveTunnelConnectionMgr::ServiceTunnelClose(WEAVE_ERROR err)
{
    bool release = true;

    // Send a Tunnel Close control message

    if (mConnectionState == WeaveTunnelConnectionMgr::kState_TunnelOpen && err == WEAVE_NO_ERROR)
    {
        err = mTunControl.SendTunnelClose(this);
        if (err == WEAVE_NO_ERROR)
        {
            // Set the appropriate connection state to Closing.

            mConnectionState = WeaveTunnelConnectionMgr::kState_TunnelClosing;
            release = false;
        }
    }

    if (release)
    {
        // Release held resources(e.g., ExchangeContext, Timers) if any and stop tunnel connection.

        ReleaseResourcesAndStopTunnelConn(err);

        // Set the WeaveTunnelAgent state to tunnel disabled so that we do not reconnect.

        mTunAgent->WeaveTunnelConnectionDown(this, err);

        // Stop the online checker if running.

        StopOnlineCheck();
    }

    // Reset the failed connection attempts in a row as tunnel is being disabled.

    mTunFailedConnAttemptsInRow = 0;

    mTunReconnectFibonacciIndex = 0;
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
 */
void WeaveTunnelConnectionMgr::RecvdFromService (WeaveConnection *con,
                                                 const WeaveMessageInfo *msgInfo,
                                                 PacketBuffer *msg)
{
    WeaveTunnelConnectionMgr *tConnMgr = static_cast<WeaveTunnelConnectionMgr *>(con->AppState);

    tConnMgr->mTunAgent->HandleTunneledReceive(msg, tConnMgr->mTunType);

    return;
}

void WeaveTunnelConnectionMgr::ServiceMgrStatusHandler(void* appState, WEAVE_ERROR err, StatusReport *report)
{
    ReconnectParam reconnParam;

    WeaveTunnelConnectionMgr *tConnMgr = static_cast<WeaveTunnelConnectionMgr *>(appState);

    WeaveLogError(WeaveTunnel, "ServiceManager reported err %s, status %s\n",
            nl::ErrorStr(err), report ? nl::StatusReportStr(report->mProfileId, report->mStatusCode) : "none");

    if (err == WEAVE_NO_ERROR)
    {
        err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
    }

    // The connection is closed by Service Manager; Set the connection state before reconnecting.

    tConnMgr->mConnectionState = WeaveTunnelConnectionMgr::kState_NotConnected;

    tConnMgr->mServiceCon = NULL;

    // Increment the failed connection attempts counter.

    if (report)
    {
        reconnParam.PopulateReconnectParam(err, report->mProfileId, report->mStatusCode);
    }
    else
    {
        reconnParam.PopulateReconnectParam(err);
    }

    tConnMgr->ReleaseResourcesAndStopTunnelConn(err);

    tConnMgr->AttemptReconnect(reconnParam);
}

/**
 * Handler invoked when Service TCP connection is completed. The device proceeds to initiate Tunnel control
 * commands to the Service from this function.
 *
 * @param[in] con                          A pointer to the WeaveConnection object.
 *
 * @param[in] conErr                       Any error within the WeaveConnection or WEAVE_NO_ERROR.
 *
 */
void WeaveTunnelConnectionMgr::HandleServiceConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    WeaveTunnelRoute tunRoute;
    ReconnectParam reconnParam;
    uint64_t globalId = 0;
    uint8_t routePriority = WeaveTunnelRoute::kRoutePriority_Medium;

    WeaveTunnelConnectionMgr *tConnMgr = static_cast<WeaveTunnelConnectionMgr *>(con->AppState);
    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    tConnMgr->mServiceCon = con;

    SuccessOrExit(conErr);

    WeaveLogDetail(WeaveTunnel, "Connection established to node %" PRIx64 " (%s) on %s tunnel\n",
                   con->PeerNodeId, ipAddrStr, tConnMgr->mTunType == kType_TunnelPrimary?"primary":"backup");

    // Set here the Tunneled Data handler and ConnectionClosed handler.

    tConnMgr->mServiceCon->OnConnectionClosed = HandleServiceConnectionClosed;
    tConnMgr->mServiceCon->OnTunneledMessageReceived = RecvdFromService;

    // Set the appropriate route priority based on the tunnel type

    if (tConnMgr->mTunType == kType_TunnelBackup)
    {
        routePriority = WeaveTunnelRoute::kRoutePriority_Low;
    }

    // Create tunnel route for Service and send Tunnel control message.

    memset(&tunRoute, 0, sizeof(tunRoute));
    globalId = WeaveFabricIdToIPv6GlobalId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->FabricId);
    if (tConnMgr->mTunAgent->mRole == kClientRole_BorderGateway)
    {
        tunRoute.tunnelRoutePrefix[0].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_ThreadMesh, 0);
        tunRoute.tunnelRoutePrefix[0].Length = NL_INET_IPV6_DEFAULT_PREFIX_LEN;
        tunRoute.tunnelRoutePrefix[1].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi, 0);
        tunRoute.tunnelRoutePrefix[1].Length = NL_INET_IPV6_DEFAULT_PREFIX_LEN;
        tunRoute.tunnelRoutePrefix[2].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi,
                                                           WeaveNodeIdToIPv6InterfaceId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->LocalNodeId));
        tunRoute.tunnelRoutePrefix[2].Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        tunRoute.tunnelRoutePrefix[3].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_ThreadMesh,
                                                           WeaveNodeIdToIPv6InterfaceId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->LocalNodeId));
        tunRoute.tunnelRoutePrefix[3].Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        tunRoute.priority[0] = tunRoute.priority[1] = tunRoute.priority[2] = tunRoute.priority[3] = routePriority;
        tunRoute.numOfPrefixes = 4;
    }
    else if (tConnMgr->mTunAgent->mRole == kClientRole_MobileDevice)
    {
        tunRoute.tunnelRoutePrefix[0].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_MobileDevice,
                                                           WeaveNodeIdToIPv6InterfaceId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->LocalNodeId));
        tunRoute.tunnelRoutePrefix[0].Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        tunRoute.priority[0] = routePriority;
        tunRoute.numOfPrefixes = 1;

    }
    else if (tConnMgr->mTunAgent->mRole == kClientRole_StandaloneDevice)
    {
        tunRoute.tunnelRoutePrefix[0].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_PrimaryWiFi,
                                                           WeaveNodeIdToIPv6InterfaceId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->LocalNodeId));
        tunRoute.tunnelRoutePrefix[0].Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        tunRoute.tunnelRoutePrefix[1].IPAddr = IPAddress::MakeULA(globalId, kWeaveSubnetId_ThreadMesh,
                                                           WeaveNodeIdToIPv6InterfaceId(tConnMgr->mTunAgent->mExchangeMgr->FabricState->LocalNodeId));
        tunRoute.tunnelRoutePrefix[1].Length = NL_INET_IPV6_MAX_PREFIX_LEN;
        tunRoute.priority[0] = tunRoute.priority[1] = routePriority;
        tunRoute.numOfPrefixes = 2;
    }

    conErr = tConnMgr->mTunControl.SendTunnelOpen(tConnMgr, &tunRoute);
    SuccessOrExit(conErr);

    // Set state variables to indicate successful connection.

    tConnMgr->mConnectionState = kState_ConnectionEstablished;

#if WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED
    // With the connection established, configure the User timeout on the connection

    conErr = tConnMgr->ConfigureConnTimeout(tConnMgr->mMaxUserTimeoutSecs);
    SuccessOrExit(conErr);
#endif // WEAVE_CONFIG_TUNNEL_TCP_USER_TIMEOUT_SUPPORTED

#if WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED
    // With the connection established, configure the User timeout on the connection

    conErr = tConnMgr->ConfigureAndEnableTCPKeepAlive(tConnMgr->mKeepAliveIntervalSecs, tConnMgr->mMaxNumProbes);
    SuccessOrExit(conErr);
#endif // WEAVE_CONFIG_TUNNEL_TCP_KEEPALIVE_SUPPORTED

exit:

    if (conErr != WEAVE_NO_ERROR)
    {
        WeaveLogError(WeaveTunnel, "Connection FAILED to node %" PRIx64 " (%s): %ld: Try to reconnect on %s tunnel\n",
                      con ? con->PeerNodeId : 0, ipAddrStr, (long)conErr, tConnMgr->mTunType == kType_TunnelPrimary?"primary":"backup");

        // Attempt to reconnect to Service.
        reconnParam.PopulateReconnectParam(conErr);

        tConnMgr->StopAndReconnectTunnelConn(reconnParam);
    }

    return;
}

/**
 * Handler invoked when Service TCP connection is closed. The device, subsequently, tries to
 * re-establish the connection to the Service.
 *
 * @param[in] con                          A pointer to the WeaveConnection object.
 *
 * @param[in] conErr                       Any error within the WeaveConnection or WEAVE_NO_ERROR.
 *
 */
void WeaveTunnelConnectionMgr::HandleServiceConnectionClosed (WeaveConnection *con, WEAVE_ERROR conErr)
{
    char ipAddrStr[64];
    uint64_t peerNodeId;
    ReconnectParam reconnParam;

    WeaveTunnelConnectionMgr *tConnMgr = static_cast<WeaveTunnelConnectionMgr *>(con->AppState);

    con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    peerNodeId = con->PeerNodeId;

    if (conErr == WEAVE_NO_ERROR)
    {
        WeaveLogDetail(WeaveTunnel, "Connection closed to node %" PRIx64 " (%s) on %s tunnel\n",
                       peerNodeId, ipAddrStr, tConnMgr->mTunType == kType_TunnelPrimary?"primary":"backup");

        conErr = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;

    }
    else
    {
        WeaveLogError(WeaveTunnel, "Connection ABORTED to node %" PRIx64 " (%s): %ld on %s tunnel\n",
                      peerNodeId, ipAddrStr, (long)conErr, tConnMgr->mTunType == kType_TunnelPrimary?"primary":"backup");
    }

    reconnParam.PopulateReconnectParam(conErr);

    tConnMgr->StopAndReconnectTunnelConn(reconnParam);
}

/**
 * Release ExchangeContext and Reconnect timer
 */
void WeaveTunnelConnectionMgr::ReleaseResourcesAndStopTunnelConn(WEAVE_ERROR err)
{
    // Release the ExchangeContext if being held

    if (mTunControl.mServiceExchangeCtxt != NULL)
    {
        mTunControl.mServiceExchangeCtxt->Close();
        mTunControl.mServiceExchangeCtxt = NULL;
    }

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
        // Stop the Tunnel Liveness timer

        StopLivenessTimer();
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

    // Cancel the Reconnect timer

    CancelDelayedReconnect();

    // Stop the tunnel connection

    StopServiceTunnelConn(err);
}

/**
 * Increment the connection attempt counter and try to reconnect to Service
 */
void WeaveTunnelConnectionMgr::AttemptReconnect(ReconnectParam &reconnParam)
{
    mTunFailedConnAttemptsInRow++;

    DecideOnReconnect(reconnParam);
}

/**
 * Reset the reconnect timeout to make the tunnel connect promptly
 * after potentially backing off a random time within a configured period.
 *
 */
WEAVE_ERROR WeaveTunnelConnectionMgr::ResetReconnectBackoff(bool reconnectImmediately)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t waitTimeInMsec = 0;

    /* A reconnect reset request is not honored when a previous one has
     * not been executed yet.
     */
    VerifyOrExit(!mResetReconnectArmed, err = WEAVE_ERROR_TUNNEL_RESET_RECONNECT_ALREADY_ARMED);

    // Cancel the currently running Reconnect timer

    CancelDelayedReconnect();

    // Reset the fibonacci index so that it starts from the beginning when
    // connection fails.

    mTunReconnectFibonacciIndex = 0;

    if (reconnectImmediately)
    {
        // Schedule the connect attempt immediately.

        ResetCacheAndScheduleConnect(CONNECT_NO_DELAY);
    }
    else
    {
        waitTimeInMsec = GetRandU32() % (WEAVE_CONFIG_TUNNELING_RESET_RECONNECT_TIMEOUT_SECS *
                                         nl::Weave::System::kTimerFactor_milli_per_unit);

        ResetCacheAndScheduleConnect(waitTimeInMsec);
    }

    mResetReconnectArmed = true;
exit:
    return err;
}

#if WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED
void WeaveTunnelConnectionMgr::TunnelLivenessTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ReconnectParam reconnParam;

    WeaveTunnelConnectionMgr* tConnMgr = static_cast<WeaveTunnelConnectionMgr*>(aAppState);

    WeaveLogDetail(WeaveTunnel, "Sending Tunnel liveness probe on %s tunnel\n",
                   tConnMgr->mTunType == kType_TunnelPrimary ? "primary" : "backup");

    err = tConnMgr->mTunControl.SendTunnelLiveness(tConnMgr);

    if (err != WEAVE_NO_ERROR)
    {
        reconnParam.PopulateReconnectParam(err);

        tConnMgr->StopAndReconnectTunnelConn(reconnParam);
    }

    return;
}

/* Schedule a timer for sending a Tunnel Liveness control message */
void WeaveTunnelConnectionMgr::StartLivenessTimer(void)
{
    mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(mTunnelLivenessInterval * nl::Weave::System::kTimerFactor_milli_per_unit, TunnelLivenessTimeout, this);
}

/* Stop the Tunnel Liveness timer for sending a Tunnel Liveness control message */
void WeaveTunnelConnectionMgr::StopLivenessTimer(void)
{
    mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(TunnelLivenessTimeout, this);
}

/* Restart the timer for sending a Tunnel Liveness control message */
void WeaveTunnelConnectionMgr::RestartLivenessTimer(void)
{
    StopLivenessTimer();

    StartLivenessTimer();
}
#endif // WEAVE_CONFIG_TUNNEL_LIVENESS_SUPPORTED

void WeaveTunnelConnectionMgr::OnlineCheckTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveTunnelConnectionMgr* tConnMgr = static_cast<WeaveTunnelConnectionMgr*>(aAppState);

    WeaveLogDetail(WeaveTunnel, "Sending Online check probe for %s tunnel\n",
                   tConnMgr->mTunType == kType_TunnelPrimary ? "primary" : "backup");

    if (tConnMgr->mTunAgent->NetworkOnlineCheck)
    {
        tConnMgr->mTunAgent->NetworkOnlineCheck(tConnMgr->mTunType, tConnMgr->mTunAgent->mAppContext);
    }

    // Schedule the next online check.

    tConnMgr->StartOnlineCheck();
}

void WeaveTunnelConnectionMgr::SetOnlineCheckIntervalFast(bool aProbeFast)
{
    mOnlineCheckInterval = aProbeFast ? WEAVE_CONFIG_TUNNELING_ONLINE_CHECK_FAST_FREQ_SECS :
                                       (mTunType == kType_TunnelPrimary ? WEAVE_CONFIG_TUNNELING_ONLINE_CHECK_PRIMARY_SLOW_FREQ_SECS :
                                                                          WEAVE_CONFIG_TUNNELING_ONLINE_CHECK_BACKUP_SLOW_FREQ_SECS);
}

void WeaveTunnelConnectionMgr::StartOnlineCheck(void)
{
    if (mTunAgent->NetworkOnlineCheck)
    {
        mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->StartTimer(mOnlineCheckInterval * nl::Weave::System::kTimerFactor_milli_per_unit,
                                                                       OnlineCheckTimeout, this);
    }
    else
    {
        WeaveLogError(WeaveTunnel, "Online check failure: Platform application has not registered the Online check handler.");
    }
}

void WeaveTunnelConnectionMgr::StopOnlineCheck(void)
{
    mTunAgent->mExchangeMgr->MessageLayer->SystemLayer->CancelTimer(OnlineCheckTimeout, this);
}

void WeaveTunnelConnectionMgr::ReStartOnlineCheck(void)
{
    StopOnlineCheck();

    StartOnlineCheck();
}

void WeaveTunnelConnectionMgr::HandleOnlineCheckResult(bool isOnline)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool reconnectImmediately = true;

    if (isOnline)
    {
        SetOnlineCheckIntervalFast(false);

        // On the transition from offline to online reset tunnel backoff.
        if (!mIsNetworkOnline)
        {
            // Reset the tunnel backoff and reconnect after a short randomized wait.

            WeaveLogDetail(WeaveTunnel, "Tunnel Reconnecting on OnlineCheck success for %s tunnel : %s",
                           mTunType == kType_TunnelPrimary ? "primary" : "backup");
            err = ResetReconnectBackoff(!reconnectImmediately);
            if (err != WEAVE_NO_ERROR)
            {
                WeaveLogError(WeaveTunnel, "Tunnel ResetReconnectBackoff failed for %s tunnel : %s",
                              mTunType == kType_TunnelPrimary ? "primary" : "backup", ErrorStr(err));
            }
        }
    }

    // Set the current state.

    mIsNetworkOnline = isOnline;
}

/**
 * @brief The default policy implementation for fetching the next time to
 * connect to the Service. This policy picks a random timeslot (with
 * millisecond resolution) over an increasing window, following a fibonacci
 * sequence upto WEAVE_CONFIG_TUNNELING_RECONNECT_MAX_FIBONACCI_INDEX.
 *
 * @param[in] appState          App state pointer set during initialization of
 *                              the SubscriptionClient.
 * @param[in] reconnectParam    Structure with parameters that influence the calculation of the
 *                              reconnection delay.
 *
 * @param[out] delayMsec        Time in milliseconds to wait before next
 *                              reconnect attempt.
 */

void WeaveTunnelConnectionMgr::DefaultReconnectPolicyCallback(void * const appState,
                                                              ReconnectParam & reconnParam,
                                                              uint32_t & delayMsec)
{
    uint32_t fibonacciNum = 0;
    uint32_t maxWaitTimeInMsec = 0;
    uint32_t waitTimeInMsec = 0;
    uint32_t minWaitTimeInMsec = 0;

    WeaveTunnelConnectionMgr* tConnMgr = static_cast<WeaveTunnelConnectionMgr*>(appState);

    if (tConnMgr->mTunReconnectFibonacciIndex > WEAVE_CONFIG_TUNNELING_RECONNECT_MAX_FIBONACCI_INDEX)
    {
        tConnMgr->mTunReconnectFibonacciIndex = WEAVE_CONFIG_TUNNELING_RECONNECT_MAX_FIBONACCI_INDEX;
    }

    fibonacciNum = GetFibonacciForIndex(tConnMgr->mTunReconnectFibonacciIndex);

    maxWaitTimeInMsec = fibonacciNum * WEAVE_CONFIG_TUNNELING_CONNECT_WAIT_TIME_MULTIPLIER_SECS * System::kTimerFactor_milli_per_unit;

    if (maxWaitTimeInMsec != 0)
    {
        // If the reconnectParam comes with a minimum wait time that is greater
        // than the normally configured min wait time(as a percentage of the
        // maxWaitTime), then preferentially use the one in the reconnectParam.

        minWaitTimeInMsec = (reconnParam.mMinDelayToConnectSecs * System::kTimerFactor_milli_per_unit) >
                            (WEAVE_CONFIG_TUNNELING_MIN_WAIT_TIME_INTERVAL_PERCENT * maxWaitTimeInMsec) / 100 ?
                            reconnParam.mMinDelayToConnectSecs * System::kTimerFactor_milli_per_unit :
                            (WEAVE_CONFIG_TUNNELING_MIN_WAIT_TIME_INTERVAL_PERCENT * maxWaitTimeInMsec) / 100;

        waitTimeInMsec = minWaitTimeInMsec + (GetRandU32() % (maxWaitTimeInMsec - minWaitTimeInMsec));
    }

    delayMsec = waitTimeInMsec;

    tConnMgr->mTunReconnectFibonacciIndex++;

    WeaveLogDetail(WeaveTunnel, "Tunnel reconnect policy: attempts %" PRIu32 ", max wait time %" PRIu32 " ms, selected wait time %" PRIu32 " ms",
                   tConnMgr->mTunFailedConnAttemptsInRow, maxWaitTimeInMsec, waitTimeInMsec);

    return;
}

/**
 * Populate the fields of the ReconnectParam structure
 */
void ReconnectParam::PopulateReconnectParam(WEAVE_ERROR lastConnectError,
                                            uint32_t profileId,
                                            uint16_t statusCode,
                                            uint32_t minDelayToConnectSecs)
{
    mStatusProfileId = profileId;
    mStatusCode = statusCode;
    mLastConnectError = lastConnectError;
    mMinDelayToConnectSecs = minDelayToConnectSecs;
}

#endif // WEAVE_CONFIG_ENABLE_TUNNELING

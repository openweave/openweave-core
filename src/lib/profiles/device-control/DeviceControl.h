/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file defines the Device Control Profile, which provides
 *      operations and utilities used during device setup and provisioning.
 */

#ifndef DEVICECONTROL_H_
#define DEVICECONTROL_H_

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>

/**
 *   @namespace nl::Weave::Profiles::DeviceControl
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Device Control profile.
 *
 *     The Device Control Profile facilitates client-server operations
 *     such that the client (the controlling device) can trigger specific
 *     utility functionality on the server (the device undergoing setup)
 *     to assist with and enable the device setup and provisioning process.
 *     This includes, for example, resetting the server device's
 *     configuration and enabling fail safes that define the behavior when
 *     the setup procedure is prematurely aborted.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace DeviceControl {

using nl::Inet::IPAddress;

/**
 * Device Control Status Codes
 */
enum
{
    kStatusCode_FailSafeAlreadyActive           = 1,    /**< A provisioning fail-safe is already active. */
    kStatusCode_NoFailSafeActive                = 2,    /**< No provisioning fail-safe is active. */
    kStatusCode_NoMatchingFailSafeActive        = 3,    /**< The provisioning fail-safe token did not match the active fail-safe. */
    kStatusCode_UnsupportedFailSafeMode         = 4,    /**< The specified fail-safe mode is not supported by the device. */
    kStatusCode_RemotePassiveRendezvousTimedOut = 5,    /**< No devices rendezvoused with the Device Control server
                                                               during the client-specified rendezvous period. */
    kStatusCode_UnsecuredListenPreempted        = 6,    /**< Another application has forcibly replaced Device Control
                                                               server as this Weave stack's unsecured connection handler. */
    kStatusCode_ResetSuccessCloseCon            = 7,    /**< The ResetConfig method will succeed, but will close the connection first. */
    kStatusCode_ResetNotAllowed                 = 8,    /**< The device refused to allow the requested reset. */
    kStatusCode_NoSystemTestDelegate            = 9     /**< The system test cannot run without a delegate. */
};

/**
 * Device Control Message Types
 */
enum
{
    kMsgType_ResetConfig                     = 1,    /**< Reset the configuration state of the device. */
    kMsgType_ArmFailSafe                     = 2,    /**< Arm the configuration fail-safe mechanism on the device. */
    kMsgType_DisarmFailSafe                  = 3,    /**< Disarm an active configuration fail-safe. */
    kMsgType_EnableConnectionMonitor         = 4,    /**< Enable connection liveness monitoring. */
    kMsgType_DisableConnectionMonitor        = 5,    /**< Disable connection liveness monitoring. */
    kMsgType_RemotePassiveRendezvous         = 6,    /**< Request Remote Passive Rendezvous with Device Control server. */
    kMsgType_RemoteConnectionComplete        = 7,    /**< Indicate to Device Control client that Remote Passive Rendezvous
                                                            has completed successfully and connection tunnel is open. */
    kMsgType_StartSystemTest                 = 8,    /**< Start the system test. */
    kMsgType_StopSystemTest                  = 9,    /**< Stop the system test. */
    kMsgType_LookingToRendezvous             = 10    /**< Looking to Rendezvouz message. The payload is empty, the only meaningful signal within is the source node id. */
};

/**
 * ArmFailSafe Mode Values
 */
enum
{
    kArmMode_New                                = 1,    /**< Arm a new fail-safe; return an error if one is already active. */
    kArmMode_Reset                              = 2,    /**< Reset all device device configuration and arm a new fail-safe. */
    kArmMode_ResumeExisting                     = 3     /**< Resume a fail-safe already in progress; return an error if no fail-safe
                                                               in progress, or if fail-safe token does not match. */
};

/**
 * ResetConfig Flags
 */
enum
{
    kResetConfigFlag_All                        = 0x00FF, /**< Reset all device configuration information. */
    kResetConfigFlag_NetworkConfig              = 0x0001, /**< Reset network configuration information. */
    kResetConfigFlag_FabricConfig               = 0x0002, /**< Reset fabric configuration information. */
    kResetConfigFlag_ServiceConfig              = 0x0004, /**< Reset network configuration information. */
    kResetConfigFlag_OperationalCredentials     = 0x4000, /**< Reset device operational credentials. */
    kResetConfigFlag_FactoryDefaults            = 0x8000  /**< Reset device to full factory defaults. */
};

/**
 * Message Lengths
 */
enum
{
    kMessageLength_ResetConfig              = 2,    /**< Reset Config message length. */
    kMessageLength_ArmFailsafe              = 5,    /**< Arm Failsafe message length. */
    kMessageLength_DisarmFailsafe           = 0,    /**< Disarm Failsafe message length. */
    kMessageLength_EnableConnectionMonitor  = 4,    /**< Enable Connection Monitor message length. */
    kMessageLength_DisableConnectionMonitor = 0,    /**< Disable Connection Monitor message length. */
    kMessageLength_RemotePassiveRendezvous  = 20,   /**< Remote Passive Rendezvous message length. */
    kMessageLength_StartSystemTest          = 8,    /**< Start System Test message length. */
    kMessageLength_StopSystemTest           = 0,    /**< Stop System Test message length. */
};

/**
 * Delegate class for implementing incoming Device Control operations on the server device.
 */
class DeviceControlDelegate : public WeaveServerDelegateBase
{
public:
    /**
     * Determine whether a server connection, if present, should be closed prior to
     * a configuration reset.
     *
     * This function is used to query the delegate for the desired behavior when
     * processing a configuration reset request.  If a server connection is currently active,
     * a TRUE response to this method will cause that connection to be closed prior
     * to the configuration reset being triggered via the OnResetConfig method.
     *
     * @param[in]   resetFlags  The flags specifying which configuration to reset.
     *
     * @retval true     if the connection needs to be closed.
     * @retval false    if the connection does not need to be closed.
     */
    virtual bool ShouldCloseConBeforeResetConfig(uint16_t resetFlags) = 0;

    /**
     * Reset all or part of the device configuration.
     *
     * This function's implementation is expected to reset any combination of
     * network, Weave fabric, or service configurations to a known state, according
     * to the reset flags.
     *
     * @param[in]   resetFlags  The flags specifying which configuration to reset.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the device from resetting its configuration.
     */
    virtual WEAVE_ERROR OnResetConfig(uint16_t resetFlags) = 0;

    /**
     * Indicate that the device configuration fail safe has been armed.
     *
     * This function is called when the server device configuration fail safe
     * has been armed in response to a request from the client.  The
     * fail safe automatically resets the device configuration to a known state
     * should the configuration process fail to complete successfully.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the fail safe from arming.
     */
    virtual WEAVE_ERROR OnFailSafeArmed(void) = 0;

    /**
     * Indicate that the device configuration fail safe has been disarmed.
     *
     * This function is called when the server device configuration fail safe
     * has been disarmed in response to a request from the client.  The
     * client will disarm the fail safe after configuration has completed.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the fail safe from disarming.
     */
    virtual WEAVE_ERROR OnFailSafeDisarmed(void) = 0;

    /**
     * Indicate that there has been a connection monitor timeout.
     *
     * This function is called when a Connection Monitor timeout has occurred,
     * that is, when liveness checks have not been detected from the remote host
     * for a certain amount of time.
     *
     * @param[in]   peerNodeId  The node ID of the remote peer to which the connection
     *                          liveness has timed out.
     * @param[in]   peerAddr    The address of the remote peer.
     */
    virtual void OnConnectionMonitorTimeout(uint64_t peerNodeId, IPAddress peerAddr) = 0;

    /**
     * Indicates that the Remote Passive Rendezvous process has started.
     */
    virtual void OnRemotePassiveRendezvousStarted(void) = 0;

    /**
     * Indicates that the Remote Passive Rendezvous process has finished.
     */
    virtual void OnRemotePassiveRendezvousDone(void) = 0;

    /**
     * Prepare for a Remote Passive Rendezvous.  For example,
     * make the 15.4/Thread network joinable.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred while preparing to start Remote Passive Rendezvous.
     */
    // TODO: More detail regarding usage required.
    virtual WEAVE_ERROR WillStartRemotePassiveRendezvous(void) = 0;

    /**
     * Prepare to stop Remote Passive Rendezvous.
     *
     * @sa WillStartRemotePassiveRendezvous(void)
     */
    // TODO: More detail regarding usage required.
    virtual void WillCloseRemotePassiveRendezvous(void) = 0;

    /**
     * Check if resetting the specified configuration is allowed.
     *
     * @param[in]   resetFlags  The flags specifying which configuration to reset.
     *
     * @retval  TRUE    if resetting the configuration is allowed.
     * @retval  FALSE   if resetting the configuration is not allowed.
     */
    virtual bool IsResetAllowed(uint16_t resetFlags) = 0;

    /**
     * Start the specified system test.
     *
     * @param[in]   profileId   The ID of the profile of the requested test.
     * @param[in]   testId      The ID of the requested test.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the starting of the system test.
     */
    virtual WEAVE_ERROR OnSystemTestStarted(uint32_t profileId, uint32_t testId) = 0;

    /**
     * Stop the system test in progress.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     * @retval other            Other Weave or platform-specific error codes indicating that an error
     *                          occurred preventing the stopping of the system test.
     */
    virtual WEAVE_ERROR OnSystemTestStopped(void) = 0;

    /**
     * Enforce message-level access control for an incoming DeviceControl request message.
     *
     * @param[in] ec            The ExchangeContext over which the message was received.
     * @param[in] msgProfileId  The profile id of the received message.
     * @param[in] msgType       The message type of the received message.
     * @param[in] msgInfo       A WeaveMessageInfo structure containing information about the received message.
     * @param[inout] result     An enumerated value describing the result of access control policy evaluation for
     *                          the received message. Upon entry to the method, the value represents the tentative
     *                          result at the current point in the evaluation process.  Upon return, the result
     *                          is expected to represent the final assessment of access control policy for the
     *                          message.
     */
    virtual void EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
                const WeaveMessageInfo *msgInfo, AccessControlResult& result);

    /**
     * Called to determine if the device is currently paired to an account.
     */
    // TODO: make this pure virtual when product code provides appropriate implementations.
    virtual bool IsPairedToAccount() const;
};

/**
 * Server class for implementing the Device Control profile.
 */
// TODO: Additional documentation detail required (i.e. expected class usage, number in the system, instantiation requirements, lifetime).
class NL_DLL_EXPORT DeviceControlServer : public WeaveServerBase
{
public:
    DeviceControlServer(void);

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void SetDelegate(DeviceControlDelegate *delegate);
    bool IsRemotePassiveRendezvousInProgress(void) const;

    void SystemTestTimeout(void);

    virtual WEAVE_ERROR SendSuccessResponse(void);
    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

protected:
    ExchangeContext *mCurClientOp;
    ExchangeContext *mRemotePassiveRendezvousOp;
    DeviceControlDelegate *mDelegate;
    WeaveConnection *mRemotePassiveRendezvousClientCon;
    WeaveConnection *mRemotePassiveRendezvousJoinerCon;
    WeaveConnectionTunnel *mRemotePassiveRendezvousTunnel;
    IPAddress mRemotePassiveRendezvousJoinerAddr;
    uint32_t mFailSafeToken;
    uint16_t mRemotePassiveRendezvousTimeout; // in sec
    uint16_t mTunnelInactivityTimeout; // in sec
    uint16_t mRemotePassiveRendezvousKeyId;
    uint8_t mRemotePassiveRendezvousEncryptionType;
    uint16_t mResetFlags;
    bool mFailSafeArmed;

private:
    void StartMonitorTimer(ExchangeContext *monitorOp);
    void CancelMonitorTimer(ExchangeContext *monitorOp);
    void CloseClientOp(void);

    WEAVE_ERROR SetConnectionMonitor(uint64_t peerNodeId, WeaveConnection *peerCon, uint16_t idleTimeout,
            uint16_t monitorInterval);

    // ----- Top-level message dispatch functions -----
    WEAVE_ERROR HandleResetConfig(uint8_t *p, WeaveConnection *curCon);
    WEAVE_ERROR HandleArmFailSafe(uint8_t *p);
    WEAVE_ERROR HandleDisarmFailSafe(void);
    WEAVE_ERROR HandleEnableConnectionMonitor(uint8_t *p, const WeaveMessageInfo *msgInfo, ExchangeContext *ec);
    WEAVE_ERROR HandleDisableConnectionMonitor(const WeaveMessageInfo *msgInfo, ExchangeContext *ec);
    WEAVE_ERROR HandleRemotePassiveRendezvous(uint8_t *p, ExchangeContext *ec);
    WEAVE_ERROR HandleStartSystemTest(uint8_t *p);
    WEAVE_ERROR HandleStopSystemTest(void);
    WEAVE_ERROR HandleLookingToRendezvousMessage(const WeaveMessageInfo *msgInfo, ExchangeContext *ec);

    // ----- Weave callbacks -----

	// ResetConfig connection closed
    static void HandleResetConfigConnectionClose(WeaveConnection *con, WEAVE_ERROR conErr);

    // Connection received
    static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);

    // OnUnsecuredConnetionReceived callback removed
    static void HandleUnsecuredConnectionCallbackRemoved(void *appState);

    // Message received
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleRendezvousIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleMonitorResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    // InetLayer timer expiration
    static void HandleMonitorTimer(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static void HandleRemotePassiveRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static void HandleLookingToRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static void HandleLookingToRendezvousClosed(WeaveConnection *con, WEAVE_ERROR conErr);

    // RPR tunnel shutdown
    static void HandleTunnelShutdown(WeaveConnectionTunnel *tunnel);

    // ExchangeContext timeouts
    static void HandleRendezvousIdentifyResponseTimeout(ExchangeContext *ec);
    static void HandleRendezvousIdentifyRetransmissionTimeout(ExchangeContext *ec);

    // Connection closed
    static void HandleRemotePassiveRendezvousConnectionClosed(ExchangeContext *ec, WeaveConnection *con,
            WEAVE_ERROR conErr);
    static void HandleRendezvousIdentifyConnectionClosed(ExchangeContext *ec, WeaveConnection *con,
            WEAVE_ERROR conErr);
    static void HandleMonitorConnectionClose(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);


    WEAVE_ERROR VerifyRendezvousedDeviceIdentity(WeaveConnection *con);
    void HandleIdentifyFailed(void);
    WEAVE_ERROR CompleteRemotePassiveRendezvous(WeaveConnection *con);
    void CancelRemotePassiveRendezvousListen(void);
    void CloseRemotePassiveRendezvous(void);
    WEAVE_ERROR ArmRemotePassiveRendezvousTimer(void);

    DeviceControlServer(const DeviceControlServer&);   // not defined

    static DeviceControlServer *sRemotePassiveRendezvousServer;
};

WEAVE_ERROR SendLookingToRendezvous(ExchangeContext *ec);

} // namespace DeviceControl
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* DEVICECONTROL_H_ */

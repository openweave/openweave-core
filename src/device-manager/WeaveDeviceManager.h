/*
 *
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
 *      Declaration of Weave Device Manager, a common class
 *      that implements discovery, pairing and provisioning of Weave
 *      devices.
 *
 */

#ifndef __WEAVEDEVICEMANAGER_H
#define __WEAVEDEVICEMANAGER_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/common/WeaveMessage.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/locale/LocaleProfile.hpp>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/network-provisioning/NetworkInfo.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCASE.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>
#include <Weave/Profiles/vendor/nestlabs/dropcam-legacy-pairing/DropcamLegacyPairing.h>
#include <Weave/Profiles/vendor/nestlabs/thermostat/NestThermostatWeaveConstants.h>

namespace nl {
namespace Weave {
namespace DeviceManager {

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::DropcamLegacyPairing;
using namespace nl::Weave::Profiles::NetworkProvisioning;

using NetworkProvisioning::NetworkType;
using NetworkProvisioning::WiFiMode;
using NetworkProvisioning::WiFiRole;
using NetworkProvisioning::WiFiSecurityType;
using DeviceDescription::IdentifyDeviceCriteria;

static const uint16_t MAX_ENUMERATED_DEVICES = 256; // Size (arbitrarily chosen) of table to de-dupe enumerated node IDs.


class DeviceStatus
{
public:
    uint32_t StatusProfileId;
    uint16_t StatusCode;
    uint32_t SystemErrorCode;
};

class WeaveDeviceManager;

extern "C"
{
typedef void (*CompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState);
typedef void (*IdentifyDeviceCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, const DeviceDescription::WeaveDeviceDescriptor *devdesc);
typedef void (*DeviceEnumerationResponseFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, const DeviceDescription::WeaveDeviceDescriptor *devdesc,
                                               IPAddress deviceAddr, InterfaceId deviceIntf);
typedef void (*NetworkScanCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, uint16_t netCount, const NetworkInfo *netInfoList);
typedef void (*GetNetworksCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, uint16_t netCount, const NetworkInfo *netInfoList);
typedef void (*AddNetworkCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, uint32_t networkId);
typedef void (*GetRendezvousModeCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, uint16_t modeFlags);
typedef void (*GetFabricConfigCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, const uint8_t *fabricConfig, uint32_t fabricConfigLen);
typedef void (*ErrorFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, WEAVE_ERROR err, DeviceStatus *devStatus);
typedef void (*StartFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, WeaveConnection *con);
typedef void (*ConnectionClosedFunc)(WeaveDeviceManager *deviceMgr, void *appReqState, WeaveConnection *con, WEAVE_ERROR conErr);
typedef void (*PairTokenCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, const uint8_t *tokenPairingBundle, uint32_t tokenPairingBunldeLen);
typedef void (*UnpairTokenCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState);
typedef void (*GetCameraAuthDataCompleteFunct)(WeaveDeviceManager *deviceMgr, void *appReqState, const char *macAddress, const char *authData);
};

class NL_DLL_EXPORT WeaveDeviceManager : private Security::CASE::WeaveCASEAuthDelegate
{
public:
    enum
    {
        kState_NotInitialized = 0,
        kState_Initialized = 1
    } State;                        // [READ-ONLY] Current state

    WeaveDeviceManager();
    void *AppState;

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMsg, WeaveSecurityManager *securityMgr);
    WEAVE_ERROR Shutdown();

    // ----- Device Information -----
    WEAVE_ERROR GetDeviceId(uint64_t& deviceId);
    WEAVE_ERROR GetDeviceAddress(IPAddress& deviceAddr);
    WEAVE_ERROR IdentifyDevice(void *appReqState, IdentifyDeviceCompleteFunct onComplete, ErrorFunct onError);

    WEAVE_ERROR StartDeviceEnumeration(void *appReqState, const IdentifyDeviceCriteria &deviceCriteria, DeviceEnumerationResponseFunct onResponse, ErrorFunct onError);
    void StopDeviceEnumeration();

    // ----- Connection Management -----
    // TODO WEAV-1573: Add support for InterfaceId specification for ConnectDevice APIs
    WEAVE_ERROR ConnectDevice(uint64_t deviceId, IPAddress deviceAddr,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR ConnectDevice(uint64_t deviceId, IPAddress deviceAddr, const char *pairingCode,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR ConnectDevice(uint64_t deviceId, IPAddress deviceAddr, const uint8_t *accessToken, uint32_t accessTokenLen,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR RendezvousDevice(const IdentifyDeviceCriteria &deviceCriteria,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR RendezvousDevice(const char *pairingCode, const IdentifyDeviceCriteria &deviceCriteria,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR RendezvousDevice(const uint8_t *accessToken, uint32_t accessTokenLen, const IdentifyDeviceCriteria &deviceCriteria,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR PassiveRendezvousDevice(void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR PassiveRendezvousDevice(const char *pairingCode, void *appReqState, CompleteFunct onComplete, ErrorFunct onError, StartFunct onStart = NULL);
    WEAVE_ERROR PassiveRendezvousDevice(const uint8_t *accessToken, uint32_t accessTokenLen, void *appReqState, CompleteFunct onComplete, ErrorFunct onError);
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR ConnectBle(BLE_CONNECTION_OBJECT connObj,
        void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose = true);
    WEAVE_ERROR ConnectBle(BLE_CONNECTION_OBJECT connObj, const char *pairingCode,
        void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose = true);
    WEAVE_ERROR ConnectBle(BLE_CONNECTION_OBJECT connObj, const uint8_t *accessToken, uint32_t accessTokenLen,
        void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose = true);
#endif // CONFIG_NETWORK_LAYER_BLE

    // ----- Remote Passive Rendezvous -----

    // In all 3 functions below, the rendezvousDeviceAddr parameter may be specified as IPAddress::Any. Otherwise,
    // the assisting device will turn away all would-be joiners if their IP address does not match the specified value.
    // The assisting device turns these devices away before ever forming a tunnel between the DM and would-be joiner.

    // Perform Remote Passive Rendezvous with CASE authentication for rendezvoused device. DM will attempt to
    // authenticate each rendezvoused, would-be joiner using the given CASE credentials. If a device fails to
    // authenticate, the DM will close its tunneled connection to that device and reconnect to the assisting device,
    // starting over the RPR process to listen for new connections on its unsecured Weave port. This cycle repeats
    // until either the rendezvous timeout expires or a joiner successfully authenticates.
    //
    // The CASE variant of this function is provided for the sake of completeness. It is expected that only the PASE
    // variant will be used to perform RPR in the case of Thread-assisted pairing.
    WEAVE_ERROR RemotePassiveRendezvous(const IPAddress rendezvousDeviceAddr, const uint8_t *accessToken,
            uint32_t accessTokenLen, const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec,
            void *appReqState, CompleteFunct onComplete, ErrorFunct onError);

    // Perform Remote Passive Rendezvous with PASE authentication for rendezvoused device. DM will attempt to
    // authenticate each rendezvoused, would-be joiner using the given PASE credentials. If a device fails to
    // authenticate, the DM will close its tunneled connection to that device and reconnect to the assisting device,
    // starting over the RPR process to listen for new connections on its unsecured Weave port. This cycle repeats
    // until either the rendezvous timeout expires or a joiner successfully authenticates.
    //
    // It is expected that this function will be used to perform RPR in the case of Thread-assisted pairing.
    WEAVE_ERROR RemotePassiveRendezvous(const IPAddress rendezvousDeviceAddr, const char *pairingCode,
            const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec, void *appReqState,
            CompleteFunct onComplete, ErrorFunct onError);

    // Perform Remote Passive Rendezvous with no authentication for rendezvoused device, i.e. DM will accept
    // first tunnel handed to it by assisting device without question.
    //
    // The no-auth variant of this function is provided for the sake of completeness. It is expected that only the
    // PASE varient will be used to perform RPR in the case of Thread-assisted pairing.
    WEAVE_ERROR RemotePassiveRendezvous(const IPAddress rendezvousDeviceAddr, const uint16_t rendezvousTimeoutSec,
            const uint16_t inactivityTimeoutSec, void *appReqState, CompleteFunct onComplete, ErrorFunct onError);

    WEAVE_ERROR ReconnectDevice(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR EnableConnectionMonitor(uint16_t interval, uint16_t timeout, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR DisableConnectionMonitor(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR SetRendezvousAddress(IPAddress addr);
    WEAVE_ERROR SetRendezvousAddress(IPAddress addr, InterfaceId rendezvousIntf);
    WEAVE_ERROR SetAutoReconnect(bool autoReconnect);
    WEAVE_ERROR SetUseAccessToken(bool useAccessToken);
    WEAVE_ERROR SetRendezvousLinkLocal(bool RendezvousLinkLocal);
    WEAVE_ERROR SetConnectTimeout(uint32_t timeoutMS);
    void Close();
    void Close(bool graceful);
    void CloseDeviceConnection();
    void CloseDeviceConnection(bool graceful);
    bool IsConnected() const;
    void SetConnectionClosedCallback(ConnectionClosedFunc onConnecionClosedFunc, void *onConnecionClosedAppReq);

    // ----- Network Provisioning -----
    WEAVE_ERROR ScanNetworks(NetworkType networkType, void* appReqState, NetworkScanCompleteFunct onComplete,
            ErrorFunct onError);
    WEAVE_ERROR AddNetwork(const NetworkInfo *netInfo, void* appReqState, AddNetworkCompleteFunct onComplete,
            ErrorFunct onError);
    WEAVE_ERROR UpdateNetwork(const NetworkInfo *netInfo, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR RemoveNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR GetNetworks(uint8_t flags, void* appReqState, GetNetworksCompleteFunct onComplete,
            ErrorFunct onError);
    WEAVE_ERROR EnableNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR DisableNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR TestNetworkConnectivity(uint32_t networkId, void* appReqState, CompleteFunct onComplete,
            ErrorFunct onError);
    WEAVE_ERROR GetRendezvousMode(void* appReqState, GetRendezvousModeCompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR SetRendezvousMode(uint16_t modeFlags, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR GetLastNetworkProvisioningResult(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    // ----- Fabric Provisioning -----
    WEAVE_ERROR CreateFabric(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR LeaveFabric(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR GetFabricConfig(void* appReqState, GetFabricConfigCompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR JoinExistingFabric(const uint8_t *fabricConfig, uint32_t fabricConfigLen, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    // ----- Service Provisioning -----
    WEAVE_ERROR RegisterServicePairAccount(uint64_t serviceId, const char *accountId,
                                           const uint8_t *serviceConfig, uint16_t serviceConfigLen,
                                           const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                           const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                           void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR UpdateService(uint64_t serviceId, const uint8_t *serviceConfig, uint16_t serviceConfigLen,
                              void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR UnregisterService(uint64_t serviceId, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    // ----- Device Control -----
    WEAVE_ERROR ArmFailSafe(uint8_t armMode, uint32_t failSafeToken, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR DisarmFailSafe(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR GetCameraAuthData(const char* nonce, void* appReqState, GetCameraAuthDataCompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR ResetConfig(uint16_t resetFlags, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    WEAVE_ERROR StartSystemTest(void* appReqState, uint32_t profileId, uint32_t testId, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR StopSystemTest(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    // ---- Token Pairing ----
    WEAVE_ERROR PairToken(const uint8_t *pairingToken, uint32_t pairingTokenLen, void* appReqState, PairTokenCompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR UnpairToken(void* appReqState, UnpairTokenCompleteFunct onComplete, ErrorFunct onError);

    // ----- Misc Operations -----
    WEAVE_ERROR Ping(void* appReqState, CompleteFunct onComplete, ErrorFunct onError);
    WEAVE_ERROR Ping(void* appReqState, int32_t payloadSize, CompleteFunct onComplete, ErrorFunct onError);
    static bool IsValidPairingCode(const char *pairingCode);

    // ----- Node ID List Utils -----
    static WEAVE_ERROR AddNodeToList(uint64_t nodeId, uint64_t* &list, uint32_t &listLen, uint32_t &listMaxLen, uint32_t initialMaxLen);
    static bool IsNodeInList(uint64_t nodeId, uint64_t* list, uint32_t listLen);

    // ----- DEPRECATED Methods -----
    WEAVE_ERROR SetWiFiRendezvousAddress(IPAddress addr); // DEPRECATED -- Use SetRendezvousAddress()
    WEAVE_ERROR RendezvousDevice(const char *pairingCode, void* appReqState, CompleteFunct onComplete, ErrorFunct onError);

    WEAVE_ERROR ConfigureBinding(Binding * const apBinding);
private:
    enum OpState
    {
        kOpState_Idle                                   = 0,
        kOpState_RendezvousDevice                       = 1,
        kOpState_ConnectDevice                          = 2,
        kOpState_ReconnectDevice                        = 3,
        kOpState_IdentifyDevice                         = 5,
        kOpState_ScanNetworks                           = 6,
        kOpState_GetNetworks                            = 7,
        kOpState_GetNetworkConfig                       = 8,
        kOpState_AddNetwork                             = 9,
        kOpState_UpdateNetwork                          = 10,
        kOpState_RemoveNetwork                          = 11,
        kOpState_EnableNetwork                          = 12,
        kOpState_DisableNetwork                         = 13,
        kOpState_TestNetworkConnectivity                = 14,
        kOpState_GetRendezvousMode                      = 15,
        kOpState_SetRendezvousMode                      = 16,
        kOpState_CreateFabric                           = 17,
        kOpState_LeaveFabric                            = 18,
        kOpState_GetFabricConfig                        = 19,
        kOpState_JoinExistingFabric                     = 20,
        kOpState_RegisterServicePairAccount             = 21,
        kOpState_UpdateService                          = 22,
        kOpState_UnregisterService                      = 23,
        kOpState_ArmFailSafe                            = 24,
        kOpState_DisarmFailSafe                         = 25,
        kOpState_ResetConfig                            = 26,
        kOpState_Ping                                   = 27,
        kOpState_EnableConnectionMonitor                = 28,
        kOpState_DisableConnectionMonitor               = 29,
        kOpState_GetLastNPResult                        = 30,
        kOpState_PassiveRendezvousDevice                = 31,
        kOpState_RemotePassiveRendezvousRequest         = 32,
        kOpState_AwaitingRemoteConnectionComplete       = 33,
        kOpState_RemotePassiveRendezvousAuthenticate    = 34,
        kOpState_RestartRemotePassiveRendezvous         = 35,
        kOpState_InitializeBleConnection                = 36,
        kOpState_Hush                                   = 37,
        kOpState_StartSystemTest                        = 38,
        kOpState_StopSystemTest                         = 39,
        kOpState_PairToken                              = 40,
        kOpState_UnpairToken                            = 41,
        kOpState_GetCameraAuthData                      = 42,
        kOpState_EnumerateDevices                       = 43,
        kOpState_RemotePassiveRendezvousTimedOut        = 44,
    };

    enum ConnectionState
    {
        kConnectionState_NotConnected                             = 0,
        kConnectionState_WaitDeviceConnect                        = 1,
        kConnectionState_IdentifyDevice                           = 2,
        kConnectionState_ConnectDevice                            = 3,
        kConnectionState_StartSession                             = 4,
        kConnectionState_ReenableConnectionMonitor                = 5,
        kConnectionState_Connected                                = 6,
        kConnectionState_IdentifyRemoteDevice                     = 7
    };

    enum
    {
        kMaxPairingCodeLength                   = 16,

        kConRetryInterval                       = 500,  // ms
        kEnumerateDevicesRetryInterval          = 500, // ms
        kSessionRetryInterval                   = 1000, // ms
        kMaxSessionRetryCount                   = 20,
    };

    enum
    {
        kAuthType_None                          = 0,
        kAuthType_PASEWithPairingCode           = 1,
        kAuthType_CASEWithAccessToken           = 2
    };

    enum
    {
        // Max Validation Certs -- This controls the maximum number of certificates
        // that can be involved in the validation of peer's certificate. It must
        // include room for the peer's entity cert, our trust anchors and any intermediate
        // certs given to us by the peer in the CASE BeginSession request/response.
        kMaxCASECerts = 10,

        // Certificate Decode Buffer Size -- Size of the temporary buffer used to decode
        // certs. The buffer must be big enough to hold the ASN1 DER encoding of the
        // TBSCertificate portion of the largest cert involved in CASE authentication
        // of the peer, not including the trust anchors.  Note that all certificates
        // by the peer are decoded using this buffer, even if they are ultimately not
        // involved in authenticating the peer.
        kCertDecodeBufferSize = 1024
    };

    System::Layer* mSystemLayer;
    WeaveMessageLayer *mMessageLayer;
    WeaveExchangeManager *mExchangeMgr;
    WeaveSecurityManager *mSecurityMgr;
    ConnectionState mConState;
    WeaveConnection *mDeviceCon;
    OpState mOpState;
    void *mAppReqState;
    union
    {
        CompleteFunct General;
        IdentifyDeviceCompleteFunct IdentifyDevice;
        NetworkScanCompleteFunct ScanNetworks;
        GetNetworksCompleteFunct GetNetworks;
        AddNetworkCompleteFunct AddNetwork;
        GetRendezvousModeCompleteFunct GetRendezvousMode;
        GetFabricConfigCompleteFunct GetFabricConfig;
        PairTokenCompleteFunct PairToken;
        UnpairTokenCompleteFunct UnpairToken;
        GetCameraAuthDataCompleteFunct GetCameraAuthData;
        DeviceEnumerationResponseFunct DeviceEnumeration;
    } mOnComplete;
    CompleteFunct mOnRemotePassiveRendezvousComplete;
    ErrorFunct mOnError;
    StartFunct mOnStart;
    ConnectionClosedFunc mOnConnectionClosedFunc;
    void *mOnConnectionClosedAppReq;
    ExchangeContext *mCurReq;
    uint32_t mCurReqProfileId;
    uint16_t mCurReqMsgType;
    PacketBuffer *mCurReqMsg;
    PacketBuffer *mCurReqMsgRetained;
#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
    bool mCurReqCreateThreadNetwork;
#endif
    ExchangeContext::MessageReceiveFunct mCurReqRcvFunct;
    IPAddress mRendezvousAddr;
    IPAddress mDeviceAddr;
    IPAddress mAssistingDeviceAddr;
    IPAddress mRemoteDeviceAddr;
    InterfaceId mDeviceIntf;
    InterfaceId mAssistingDeviceIntf;
    InterfaceId mRendezvousIntf;
    IdentifyDeviceCriteria mDeviceCriteria;
    uint64_t mDeviceId;
    uint64_t mAssistingDeviceId;
    uint32_t mConTimeout;                                       // in ms; 0 means disabled.
    uint32_t mConTryCount;
    uint16_t mSessionKeyId;
    uint8_t mEncType;
    uint8_t mAuthType;
    uint8_t mAssistingDeviceAuthType;
    uint8_t mRemoteDeviceAuthType;
    char *mCameraNonce;
    void *mAuthKey;
    void *mAssistingDeviceAuthKey;
    void *mRemoteDeviceAuthKey;
    uint32_t mAuthKeyLen;
    uint32_t mAssistingDeviceAuthKeyLen;
    uint32_t mRemoteDeviceAuthKeyLen;
    uint32_t mConMonitorInterval;                               // in ms; 0 means disabled.
    uint32_t mConMonitorTimeout;                                // in ms; 0 means disabled.
    uint32_t mRemotePassiveRendezvousTimeout;                   // in seconds; 0 means disabled.
    uint32_t mRemotePassiveRendezvousInactivityTimeout;         // in seconds; 0 means disabled.
    bool mRemotePassiveRendezvousTimerIsRunning;
    bool mConMonitorEnabled;
    bool mAutoReconnect;
    bool mRendezvousLinkLocal;
    bool mUseAccessToken;
    bool mConnectedToRemoteDevice;                              // Used to determine whether to allow auto-reconnect.
    bool mIsUnsecuredConnectionListenerSet;
    uint32_t mPingSize;
    uint8_t *mTokenPairingCertificate;
    uint32_t mTokenPairingCertificateLen;
    uint64_t *mEnumeratedNodes;
    uint32_t mEnumeratedNodesLen;
    uint32_t mEnumeratedNodesMaxLen;

    // Use by static HandleConnectionReceived callback.
    static WeaveDeviceManager *sListeningDeviceMgr;

    WEAVE_ERROR DoRemotePassiveRendezvous(const IPAddress rendezvousDeviceAddr, const uint16_t rendezvousTimeout,
            const uint16_t inactivityTimeout, void *appReqState, CompleteFunct onComplete, ErrorFunct onError);

    WEAVE_ERROR SendRequest(uint32_t profileId, uint16_t msgType, PacketBuffer *msgBuf,
            ExchangeContext::MessageReceiveFunct onMsgRcvd);
    WEAVE_ERROR SendPendingRequest();

    WEAVE_ERROR SaveAuthKey(const char *pairingCode);
    WEAVE_ERROR SaveAuthKey(const uint8_t *accessToken, uint32_t accessTokenLen);
    void ClearAuthKey();
    void ClearAuthKey(void *&authKey, uint32_t &authKeyLen);

    WEAVE_ERROR SaveRemoteDeviceAuthInfo(uint8_t authType, const char *authKey, uint32_t authKeyLen);
    void ClearRemoteDeviceAuthInfo();

    void ClearRequestState();
    void ClearOpState();

    static void HandleRequestConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);

    WEAVE_ERROR ValidateIdentifyRequest(const IdentifyRequestMessage &reqMsg);
    WEAVE_ERROR InitiateDeviceEnumeration();
    WEAVE_ERROR InitiateConnection();
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR InitiateBleConnection(BLE_CONNECTION_OBJECT connObj, void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose);
#endif // CONFIG_NETWORK_LAYER_BLE

    static WEAVE_ERROR FilterIdentifyResponse(IdentifyResponseMessage &respMsg, IdentifyDeviceCriteria criteria, uint64_t sourceNodeId, bool& isMatch);

    static void HandleDeviceEnumerationIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleDeviceEnumerationTimeout(System::Layer *aSystemLayer, void *aAppState, System::Error aError);

    static void HandleConnectionIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleConnectionIdentifyTimeout(System::Layer *aSystemLayer, void *aAppState, System::Error aError);

    WEAVE_ERROR SetUnsecuredConnectionHandler();
    WEAVE_ERROR ClearUnsecuredConnectionHandler();

    WEAVE_ERROR StartConnectDevice(uint64_t deviceId, IPAddress deviceAddr);
    static void HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con);
    static void HandleUnsecuredConnectionCallbackRemoved(void *appState);

    WEAVE_ERROR StartSession();
    static void HandleSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *appReqState,
            uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType);
    static void HandleSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *appReqState,
            WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport);
    static void RetrySession(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

    void ReenableConnectionMonitor();
    static void HandleReenableConnectionMonitorResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    void HandleConnectionReady();

    static void HandleIdentifyDeviceResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleNetworkProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleGetCameraAuthDataResponseEntry(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleServiceProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleFabricProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    void HandleGetCameraAuthDataResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleDeviceControlResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleRemotePassiveRendezvousComplete(ExchangeContext *ec, const IPPacketInfo *pktInfo, const
            WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandlePingResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandlePairTokenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    static void HandleUnpairTokenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);

    void StartConnectionMonitorTimer();
    void CancelConnectionMonitorTimer();
    static void HandleEchoRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
            uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleConnectionMonitorTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);

    void HandleRemoteConnectionComplete();
    void CancelRemotePassiveRendezvous();

    WEAVE_ERROR StartRemotePassiveRendezvousTimer();
    void CancelRemotePassiveRendezvousTimer();

    void RestartRemotePassiveRendezvousListen();
    WEAVE_ERROR StartReconnectToAssistingDevice();

    // Below methods juggle peer address, authentication, and secure session state so we can reuse existing DM
    // components for the "RPR PASE failed -> reconnect to assisting device -> send another RPR request" code path.
    WEAVE_ERROR SaveAssistingDeviceConnectionInfo();    // Save all information required to reconnect to the assisting
                                                        // device so we can continue with Remote Passive Rendezvous
                                                        // attempt after PASE authentication with a particular remote
                                                        // host fails.
                                                        //
    void RestoreAssistingDeviceAddressInfo();           // Restore saved node ID, IP address, and interface
                                                        // information for assisting device.
                                                        //
    WEAVE_ERROR RestoreAssistingDeviceAuthInfo();       // Restore saved assisting device auth info so we can
                                                        // reauthenticate with it from scratch.
                                                        //
    void ResetConnectionInfo();                         // Clear current connection and authentication information.

    void HandleRemotePassiveRendezvousReconnectComplete();
    static void HandleAssistingDeviceReconnectWithSessionCompleteEntry(WeaveDeviceManager *deviceMgr,
            void *appReqState);
    static void HandleAssistingDeviceReconnectCompleteEntry(WeaveDeviceManager *deviceMgr, void *appReqState);
    static void HandleRemotePassiveRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError);
    static void HandleRemoteConnectionComplete(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleRemoteIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
            const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload);
    static void HandleRemoteIdentifyConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleRemoteIdentifyTimeout(ExchangeContext *ec);

    static WEAVE_ERROR DecodeStatusReport(PacketBuffer *msgBuf, DeviceStatus& status);
    static WEAVE_ERROR DecodeNetworkInfoList(PacketBuffer *buf, uint16_t& count, NetworkInfo *& netInfoList);

#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Methods that implement the WeaveCASEAuthDelegate interface

    WEAVE_ERROR EncodeNodeCertInfo(const Security::CASE::BeginSessionContext & msgCtx, TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateNodeSignature(const Security::CASE::BeginSessionContext & msgCtx,
            const uint8_t * msgHash, uint8_t msgHashLen,
            TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeNodePayload(const Security::CASE::BeginSessionContext & msgCtx,
            uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen) __OVERRIDE;
    WEAVE_ERROR BeginValidation(const Security::CASE::BeginSessionContext & msgCtx, Security::ValidationContext & validCtx,
            Security::WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR HandleValidationResult(const Security::CASE::BeginSessionContext & msgCtx, Security::ValidationContext & validCtx,
            Security::WeaveCertificateSet & certSet, WEAVE_ERROR & validRes) __OVERRIDE;
    void EndValidation(const Security::CASE::BeginSessionContext & msgCtx, Security::ValidationContext & validCtx,
            Security::WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);
    WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey);
    WEAVE_ERROR BeginCertValidation(bool isInitiator, Security::WeaveCertificateSet& certSet,
            Security::ValidationContext& validContext);
    WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, Security::WeaveCertificateData *peerCert,
            uint64_t peerNodeId, Security::WeaveCertificateSet& certSet, Security::ValidationContext& validContext);
    WEAVE_ERROR EndCertValidation(Security::WeaveCertificateSet& certSet, Security::ValidationContext& validContext);

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Methods that implement the legacy WeaveCASEAuthDelegate interface

    WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen) __OVERRIDE;
    WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen) __OVERRIDE;
    WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen) __OVERRIDE;
    WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey) __OVERRIDE;
    WEAVE_ERROR BeginCertValidation(bool isInitiator, Security::WeaveCertificateSet& certSet,
            Security::ValidationContext& validContext) __OVERRIDE;
    WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, Security::WeaveCertificateData *peerCert,
            uint64_t peerNodeId, Security::WeaveCertificateSet& certSet, Security::ValidationContext& validContext) __OVERRIDE;
    WEAVE_ERROR EndCertValidation(Security::WeaveCertificateSet& certSet, Security::ValidationContext& validContext) __OVERRIDE;

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // Convert EUI-48 value to string of hex characters. String buffer must be at least EUI48_STR_LEN bytes in length.
    static void Eui48ToString(char *strBuf, uint8_t (&eui)[EUI48_LEN]);
};

// ProductWildcardId -- Special Values that can be placed in the IdentifyDeviceCriteria.TargetProductId
// field to identify particular groups of Nest devices.
enum ProductWildcardId
{
    kProductWildcardId_RangeStart         = 0xFFF0,
    kProductWildcardId_RangeEnd           = 0xFFFE,

    kProductWildcardId_NestThermostat     = 0xFFF0,
    kProductWildcardId_NestProtect        = 0xFFF1,
    kProductWildcardId_NestCam            = 0xFFF2
};

} // namespace DeviceManager
} // namespace Weave
} // namespace nl

#endif // __WEAVEDEVICEMANAGER_H

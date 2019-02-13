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
 *      Implementation of Weave Device Manager, a common class
 *      that implements discovery, pairing and provisioning of Weave
 *      devices.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

// This module uses Legacy WDM (V2)
#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/Base64.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Profiles/fabric-provisioning/FabricProvisioning.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/device-control/DeviceControl.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Profiles/token-pairing/TokenPairing.h>
#include "WeaveDeviceManager.h"
#include <Weave/Support/verhoeff/Verhoeff.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Profiles/vendor/nestlabs/dropcam-legacy-pairing/DropcamLegacyPairing.h>

namespace nl {
namespace Weave {
namespace DeviceManager {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DataManagement;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Profiles::NetworkProvisioning;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace nl::Weave::Profiles::TokenPairing;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::DeviceDescription;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::DropcamLegacyPairing;
using namespace nl::Weave::Profiles::Vendor::Nestlabs::Thermostat;
using namespace nl::Weave::TLV;

static bool IsProductWildcard(uint16_t productId);

static const uint32_t ENUMERATED_NODES_LIST_INITIAL_SIZE = 256;

WeaveDeviceManager *WeaveDeviceManager::sListeningDeviceMgr = NULL;

WeaveDeviceManager::WeaveDeviceManager()
{
    State = kState_NotInitialized;
}

WEAVE_ERROR WeaveDeviceManager::Init(WeaveExchangeManager *exchangeMgr, WeaveSecurityManager *securityMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    AppState = NULL;
    mMessageLayer = exchangeMgr->MessageLayer;
    mSystemLayer = mMessageLayer->SystemLayer;
    mExchangeMgr = exchangeMgr;
    mSecurityMgr = securityMgr;
    mConState = kConnectionState_NotConnected;
    mDeviceCon = NULL;
    mOpState = kOpState_Idle;
    mCurReq = NULL;
    mCurReqMsg = NULL;
    mCurReqMsgRetained = NULL;
    mAppReqState = NULL;
    memset(&mOnComplete, 0, sizeof(mOnComplete));
    mOnError = NULL;
    mOnStart = NULL;
    mOnConnectionClosedFunc = NULL;
    mOnConnectionClosedAppReq = NULL;
    mDeviceAddr = IPAddress::Any;
    mAssistingDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mAssistingDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceId = kNodeIdNotSpecified;
    mAssistingDeviceId = kNodeIdNotSpecified;
    mConTimeout = secondsToMilliseconds(60);
    mConTryCount = 0;
    mSessionKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mAuthType = kAuthType_None;
    mAssistingDeviceAuthType = kAuthType_None;
    mRemoteDeviceAuthType = kAuthType_None;
    mAuthKey = NULL;
    mAssistingDeviceAuthKey = NULL;
    mRemoteDeviceAuthKey = NULL;
    mAuthKeyLen = 0;
    mAssistingDeviceAuthKeyLen = 0;
    mRemoteDeviceAuthKeyLen = 0;
    mConMonitorTimeout = 0;
    mConMonitorInterval = 0;
    mConMonitorEnabled = false;
    mRemotePassiveRendezvousTimeout = 0;
    mRemotePassiveRendezvousInactivityTimeout = 0;
    mRemotePassiveRendezvousTimerIsRunning = false;
    mAutoReconnect = true;
    mRendezvousLinkLocal = true;
    mUseAccessToken = true;
    mConnectedToRemoteDevice = false;
    mIsUnsecuredConnectionListenerSet = false;
    mActiveLocale = NULL;
    mPingSize = 0;
    mTokenPairingCertificate = NULL;
    mTokenPairingCertificateLen = 0;
    mCameraNonce = NULL;
    mEnumeratedNodes = NULL;
    mEnumeratedNodesLen = 0;
    mEnumeratedNodesMaxLen = 0;

    // By default, rendezvous messages are sent to the IPv6 link-local, all-nodes multicast address.
    mRendezvousAddr = IPAddress::MakeIPv6WellKnownMulticast(kIPv6MulticastScope_Link, kIPV6MulticastGroup_AllNodes);

    State = kState_Initialized;

    err = mDMClient.InitClient(this, exchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "mDMClient.Init() failed: %s", ErrorStr(err));
    }

    return err;
}

WEAVE_ERROR WeaveDeviceManager::Shutdown()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    State = kState_NotInitialized;

    if (mCurReq != NULL)
    {
        mCurReq->Close();
        mCurReq = NULL;
    }

    if (mCurReqMsg != NULL)
    {
        PacketBuffer::Free(mCurReqMsg);
        mCurReqMsg = NULL;
    }

    if (mCurReqMsgRetained != NULL)
    {
        PacketBuffer::Free(mCurReqMsgRetained);
        mCurReqMsgRetained = NULL;
    }

    if (mDeviceCon != NULL)
    {
        mDeviceCon->Abort();
        mDeviceCon = NULL;
    }

    if (mSystemLayer != NULL)
    {
        mSystemLayer->CancelTimer(HandleConnectionIdentifyTimeout, this);
        mSystemLayer->CancelTimer(RetrySession, this);
        mSystemLayer->CancelTimer(HandleDeviceEnumerationTimeout, this);
        CancelConnectionMonitorTimer();
        CancelRemotePassiveRendezvousTimer();
    }

    ClearAuthKey();

    if (mTokenPairingCertificate != NULL)
    {
        free(mTokenPairingCertificate);
        mTokenPairingCertificate = NULL;
        mTokenPairingCertificateLen = 0;
    }

    mSystemLayer = NULL;
    mMessageLayer = NULL;
    mExchangeMgr = NULL;
    mSecurityMgr = NULL;
    mConState = kConnectionState_NotConnected;
    mOpState = kOpState_Idle;
    mAppReqState = NULL;
    memset(&mOnComplete, 0, sizeof(mOnComplete));
    mOnError = NULL;
    mOnStart = NULL;

    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetDeviceId(uint64_t& deviceId)
{
    deviceId = mDeviceId;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::GetDeviceAddress(IPAddress& deviceAddr)
{
    deviceAddr = mDeviceAddr;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::ConnectDevice(uint64_t deviceId, IPAddress deviceAddr,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((mOpState != kOpState_Idle && mOpState != kOpState_RestartRemotePassiveRendezvous) ||
            mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceId;
    mDeviceAddr = deviceAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria.Reset();

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mAuthType = kAuthType_None;
    ClearAuthKey();

    mConMonitorEnabled = false;

    mOpState = kOpState_ConnectDevice;

    err = InitiateConnection();
    if (err != WEAVE_NO_ERROR)
        ClearOpState();

    return err;
}

WEAVE_ERROR WeaveDeviceManager::ConnectDevice(uint64_t deviceId, IPAddress deviceAddr, const char *pairingCode,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((mOpState != kOpState_Idle && mOpState != kOpState_RestartRemotePassiveRendezvous) ||
            mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceId;
    mDeviceAddr = deviceAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria.Reset();

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mAuthType = kAuthType_PASEWithPairingCode;
    err = SaveAuthKey(pairingCode);
    SuccessOrExit(err);

    mConMonitorEnabled = false;

    mOpState = kOpState_ConnectDevice;
    err = InitiateConnection();

exit:
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ConnectDevice(uint64_t deviceId, IPAddress deviceAddr, const uint8_t *accessToken, uint32_t accessTokenLen,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((mOpState != kOpState_Idle && mOpState != kOpState_RestartRemotePassiveRendezvous) ||
        mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceId;
    mDeviceAddr = deviceAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria.Reset();

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    if (mUseAccessToken && accessTokenLen != 0)
    {
        mAuthType = kAuthType_CASEWithAccessToken;
        err = SaveAuthKey(accessToken, accessTokenLen);
        SuccessOrExit(err);
    }
    else
    {
        mAuthType = kAuthType_None;
        ClearAuthKey();
    }

    mConMonitorEnabled = false;

    mOpState = kOpState_ConnectDevice;
    err = InitiateConnection();

exit:
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::StartDeviceEnumeration(void* appReqState, const IdentifyDeviceCriteria &deviceCriteria,
        DeviceEnumerationResponseFunct onResponse, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mOpState == kOpState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    mDeviceCriteria = deviceCriteria;

    mAppReqState = appReqState;
    mOnComplete.DeviceEnumeration = onResponse;
    mOnError = onError;

    mOpState = kOpState_EnumerateDevices;

    err = InitiateDeviceEnumeration();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOpState();
    }

    return err;
}

WEAVE_ERROR WeaveDeviceManager::InitiateDeviceEnumeration()
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf  = NULL;
    IdentifyRequestMessage  reqMsg;
    uint16_t                sendFlags;

    VerifyOrExit(kOpState_EnumerateDevices == mOpState, err = WEAVE_ERROR_INCORRECT_STATE);

    // Refresh the message layer endpoints to cope with changes in network interface status
    // (e.g. new addresses being assigned).
    err = mMessageLayer->RefreshEndpoints();
    SuccessOrExit(err);

    // Form an Identify device request containing the device criteria specified by the application.
    reqMsg.TargetFabricId = mDeviceCriteria.TargetFabricId;
    reqMsg.TargetModes = mDeviceCriteria.TargetModes;
    reqMsg.TargetVendorId = mDeviceCriteria.TargetVendorId;

    if (mDeviceCriteria.TargetVendorId == kWeaveVendor_NestLabs && IsProductWildcard(mDeviceCriteria.TargetProductId))
    {
        reqMsg.TargetProductId = 0xFFFF;
    }
    else
    {
        reqMsg.TargetProductId = mDeviceCriteria.TargetProductId;
    }

    // Encode the Identify device request message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(NULL != msgBuf, err = WEAVE_ERROR_NO_MEMORY);

    err = reqMsg.Encode(msgBuf);
    SuccessOrExit(err);

    // Construct an exchange context if necessary. Otherwise, reuse existing multicast ExchangeContext.
    if (NULL == mCurReq)
    {
        mCurReq = mExchangeMgr->NewContext(kAnyNodeId, mRendezvousAddr, this);
        VerifyOrExit(NULL != mCurReq, err = WEAVE_ERROR_NO_MEMORY);
        mCurReq->OnMessageReceived = HandleDeviceEnumerationIdentifyResponse;
    }

    WeaveLogProgress(DeviceManager, "Sending IdentifyRequest to enumerate devices");

    // Send the Identify message.
    //
    // If the 'enumerate-devices link-local' option is enabled, AND the message layer is not bound to
    // a specific local IPv6 address, THEN ...
    //
    // Send the multicast identify request from the host's link-local addresses, rather than
    // from its site-local or global addresses. This results in the device responding using its
    // link-local address, which in turn causes the device manager to connect to the device using the
    // link-local address. This is all to work around a bug in OS X/iOS that prevents those systems
    // from communicating on any site-local IPv6 subnets when in the presence of a router that is
    // advertising a default route to the Internet at large. (See DOLO-2479).
    //
    // We disable the 'enumerate-devices link-local' feature when the message layer is bound to a specific
    // address because binding to a specific address is generally used when testing the device manager
    // and a mock-device running on a single host with a single interface.  In this case multicasting
    // using the interface's single link-local address doesn't work.
    //
    sendFlags = (mRendezvousLinkLocal && !mMessageLayer->IsBoundToLocalIPv6Address())
            ? ExchangeContext::kSendFlag_MulticastFromLinkLocal
            : 0;
    err = mCurReq->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, msgBuf, sendFlags);
    msgBuf = NULL;

    if (err == System::MapErrorPOSIX(ENETUNREACH) || err == System::MapErrorPOSIX(EHOSTUNREACH) ||
        err == System::MapErrorPOSIX(EPIPE))
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Arm the retry timer.
    err = mSystemLayer->StartTimer(kEnumerateDevicesRetryInterval, HandleDeviceEnumerationTimeout, this);
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    return err;
}

void WeaveDeviceManager::StopDeviceEnumeration()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(kOpState_EnumerateDevices == mOpState, err = WEAVE_ERROR_INCORRECT_STATE);

    mSystemLayer->CancelTimer(HandleDeviceEnumerationTimeout, this);

    if (mEnumeratedNodes)
    {
        free(mEnumeratedNodes);
    }

    mEnumeratedNodes = NULL;
    mEnumeratedNodesLen = 0;
    mEnumeratedNodesMaxLen = 0;

    ClearOpState();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "StopDeviceEnumeration failure: err = %d", err);
    }
}


WEAVE_ERROR WeaveDeviceManager::RendezvousDevice(const IdentifyDeviceCriteria &deviceCriteria,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceCriteria.TargetDeviceId;
    mDeviceAddr = mRendezvousAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria = deviceCriteria;

    mAuthType = kAuthType_None;
    ClearAuthKey();

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mConMonitorEnabled = false;

    mOpState = kOpState_RendezvousDevice;

    err = InitiateConnection();
    if (err != WEAVE_NO_ERROR)
        ClearOpState();

    return err;
}

WEAVE_ERROR WeaveDeviceManager::RendezvousDevice(const char *pairingCode, void* appReqState,
        CompleteFunct onComplete, ErrorFunct onError)
{
    IdentifyDeviceCriteria deviceCriteria;
    deviceCriteria.TargetFabricId = kTargetFabricId_Any;
    deviceCriteria.TargetModes = kTargetDeviceMode_UserSelectedMode;
    deviceCriteria.TargetVendorId = kWeaveVendor_NestLabs;
    deviceCriteria.TargetProductId = 5; // Topaz

    return RendezvousDevice(pairingCode, deviceCriteria, appReqState, onComplete, onError);
}

WEAVE_ERROR WeaveDeviceManager::RendezvousDevice(const char *pairingCode, const IdentifyDeviceCriteria &deviceCriteria,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceCriteria.TargetDeviceId;
    mDeviceAddr = mRendezvousAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria = deviceCriteria;

    mAuthType = kAuthType_PASEWithPairingCode;
    err = SaveAuthKey(pairingCode);
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mConMonitorEnabled = false;

    mOpState = kOpState_RendezvousDevice;

    err = InitiateConnection();

exit:
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::RendezvousDevice(const uint8_t *accessToken, uint32_t accessTokenLen,
        const IdentifyDeviceCriteria &deviceCriteria,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = deviceCriteria.TargetDeviceId;
    mDeviceAddr = mRendezvousAddr;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria = deviceCriteria;

    if (mUseAccessToken && accessTokenLen != 0)
    {
        mAuthType = kAuthType_CASEWithAccessToken;
        err = SaveAuthKey(accessToken, accessTokenLen);
        SuccessOrExit(err);
    }
    else
    {
        mAuthType = kAuthType_None;
        ClearAuthKey();
    }

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mConMonitorEnabled = false;

    mOpState = kOpState_RendezvousDevice;

    err = InitiateConnection();

exit:
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::PassiveRendezvousDevice(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected ||
            sListeningDeviceMgr != NULL)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    mDeviceId = kAnyNodeId;
    mDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;

    mAuthType = kAuthType_None;
    ClearAuthKey();

    mConMonitorEnabled = false;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    err = SetUnsecuredConnectionHandler();
    SuccessOrExit(err);

    mOpState = kOpState_PassiveRendezvousDevice;
    mConState = kConnectionState_WaitDeviceConnect;

    // Setup pointer to device manager instance is currently doing a passive rendezvous.  Because the
    // device connects to the device manager in the passive rendezvous case there can only be one
    // device manager instance in this mode at a time.
    sListeningDeviceMgr = this;

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::PassiveRendezvousDevice(const char *pairingCode, void* appReqState, CompleteFunct onComplete, ErrorFunct onError, StartFunct onStart)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected || sListeningDeviceMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = kAnyNodeId;
    mDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;

    mAuthType = kAuthType_PASEWithPairingCode;
    err = SaveAuthKey(pairingCode);
    SuccessOrExit(err);

    mConMonitorEnabled = false;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOnStart = onStart;

    err = SetUnsecuredConnectionHandler();
    SuccessOrExit(err);

    mOpState = kOpState_PassiveRendezvousDevice;
    mConState = kConnectionState_WaitDeviceConnect;

    // Setup pointer to device manager instance is currently doing a passive rendezvous.  Because the
    // device connects to the device manager in the passive rendezvous case there can only be one
    // device manager instance in this mode at a time.
    sListeningDeviceMgr = this;

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::PassiveRendezvousDevice(const uint8_t *accessToken, uint32_t accessTokenLen, void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected || sListeningDeviceMgr != NULL)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceId = kAnyNodeId;
    mDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;

    if (mUseAccessToken && accessTokenLen != 0)
    {
        mAuthType = kAuthType_CASEWithAccessToken;
        err = SaveAuthKey(accessToken, accessTokenLen);
        SuccessOrExit(err);
    }
    else
    {
        mAuthType = kAuthType_None;
        ClearAuthKey();
    }

    mConMonitorEnabled = false;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    err = SetUnsecuredConnectionHandler();
    SuccessOrExit(err);

    mConState = kConnectionState_WaitDeviceConnect;
    mOpState = kOpState_PassiveRendezvousDevice;

    // Setup pointer to device manager instance is currently doing a passive rendezvous.  Because the
    // device connects to the device manager in the passive rendezvous case there can only be one
    // device manager instance in this mode at a time.
    sListeningDeviceMgr = this;

exit:
    return err;
}

#if CONFIG_NETWORK_LAYER_BLE
WEAVE_ERROR WeaveDeviceManager::ConnectBle(BLE_CONNECTION_OBJECT connObj,
                                           void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected ||
            sListeningDeviceMgr != NULL)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    mAuthType = kAuthType_None;
    ClearAuthKey();

    err = InitiateBleConnection(connObj, appReqState, onComplete, onError, autoClose);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ConnectBle(BLE_CONNECTION_OBJECT connObj, const char *pairingCode,
                                           void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected ||
            sListeningDeviceMgr != NULL)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    mAuthType = kAuthType_PASEWithPairingCode;
    err = SaveAuthKey(pairingCode);
    SuccessOrExit(err);

    err = InitiateBleConnection(connObj, appReqState, onComplete, onError, autoClose);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ConnectBle(BLE_CONNECTION_OBJECT connObj, const uint8_t *accessToken,
                                           uint32_t accessTokenLen, void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected ||
            sListeningDeviceMgr != NULL)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }

    if (mUseAccessToken && accessTokenLen != 0)
    {
        mAuthType = kAuthType_CASEWithAccessToken;
        err = SaveAuthKey(accessToken, accessTokenLen);
        SuccessOrExit(err);
    }
    else
    {
        mAuthType = kAuthType_None;
        ClearAuthKey();
    }

    err = InitiateBleConnection(connObj, appReqState, onComplete, onError, autoClose);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::InitiateBleConnection(BLE_CONNECTION_OBJECT connObj,
                                                      void *appReqState, CompleteFunct onComplete, ErrorFunct onError, bool autoClose)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveConnection *bleCon;

    mDeviceId = kAnyNodeId;
    mDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;
    mDeviceCriteria.Reset();

    mConMonitorEnabled = false;

    // We can't auto-reconnect via BLE, since BLE connection management occurs outside of Weave.
    mAutoReconnect = false;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mOpState = kOpState_InitializeBleConnection;
    mConState = kConnectionState_ConnectDevice;

    // Setup pointer to listening device manager. This lets us reuse the code in static HandleConnectionReceived
    // callback also used by PassiveRendezvous.
    sListeningDeviceMgr = this;

    // Bind BLE connection object to new WeaveConnection.
    bleCon = mMessageLayer->NewConnection();
    VerifyOrExit(bleCon != NULL, err = WEAVE_ERROR_TOO_MANY_CONNECTIONS);

    bleCon->AppState = this;
    bleCon->OnConnectionComplete = HandleConnectionComplete;
    bleCon->OnConnectionClosed = HandleConnectionClosed;

    err = bleCon->ConnectBle(connObj, kWeaveAuthMode_Unauthenticated, autoClose);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOpState();
        mConState = kConnectionState_NotConnected;
        sListeningDeviceMgr = NULL;
    }

    return err;
}

#endif /* CONFIG_NETWORK_LAYER_BLE */

WEAVE_ERROR WeaveDeviceManager::RemotePassiveRendezvous(IPAddress rendezvousDeviceAddr, const uint8_t *accessToken,
        uint32_t accessTokenLen, const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec,
        void *appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Save remote device authentication info, including auth type. Can't just SaveAuthKey() here, as this would
    // clear the auth key for the assisting device. We must preserve this key in case the Device Manager needs to
    // reconnect to the assisting device before it can send the RPR request.
    err = SaveRemoteDeviceAuthInfo(kAuthType_CASEWithAccessToken, (const char *)accessToken, accessTokenLen);
    SuccessOrExit(err);

    err = DoRemotePassiveRendezvous(rendezvousDeviceAddr, rendezvousTimeoutSec, inactivityTimeoutSec, appReqState, onComplete,
            onError);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::RemotePassiveRendezvous(IPAddress rendezvousDeviceAddr, const char *pairingCode,
        const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec, void *appReqState,
        CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Save remote device authentication info, including auth type. Can't just SaveAuthKey() here, as this would
    // clear the auth key for the assisting device. We must preserve this key in case the Device Manager needs to
    // reconnect to the assisting device before it can send the RPR request.
    err = SaveRemoteDeviceAuthInfo(kAuthType_PASEWithPairingCode, pairingCode, 0);
    SuccessOrExit(err);

    err = DoRemotePassiveRendezvous(rendezvousDeviceAddr, rendezvousTimeoutSec, inactivityTimeoutSec, appReqState, onComplete,
            onError);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::RemotePassiveRendezvous(IPAddress rendezvousDeviceAddr,
        const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec, void *appReqState,
        CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Save remote device authentication info, including auth type. Can't just SaveAuthKey() here, as this would
    // clear the auth key for the assisting device. We must preserve this key in case the Device Manager needs to
    // reconnect to the assisting device before it can send the RPR request.
    err = SaveRemoteDeviceAuthInfo(kAuthType_None, NULL, 0);
    SuccessOrExit(err);

    err = DoRemotePassiveRendezvous(rendezvousDeviceAddr, rendezvousTimeoutSec, inactivityTimeoutSec, appReqState, onComplete,
            onError);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::DoRemotePassiveRendezvous(IPAddress rendezvousDeviceAddr,
        const uint16_t rendezvousTimeoutSec, const uint16_t inactivityTimeoutSec, void *appReqState,
        CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

#if WEAVE_DETAIL_LOGGING
    char rendezvousDeviceAddrStr[48]; // max(INET6_ADDRSTRLEN, 40) - see inet_ntop (POSIX), ip6addr_ntoa (LwIP)

    WeaveLogDetail(DeviceManager, "RemotePassiveRendezvous (");
    WeaveLogDetail(DeviceManager, "   rendezvousDeviceAddr = %s,", rendezvousDeviceAddr.ToString(
            rendezvousDeviceAddrStr, 48));
    WeaveLogDetail(DeviceManager, "   rendezvousTimeoutSec   = %u,", rendezvousTimeoutSec);
    WeaveLogDetail(DeviceManager, "   inactivityTimeoutSec   = %u )", inactivityTimeoutSec);
#endif // WEAVE_DETAIL_LOGGING

    // Ensure DM is in the correct state to perform a Remote Passive Rendezvous.
    if (mOpState != kOpState_Idle || mConMonitorEnabled)
    {
        if (mConMonitorEnabled)
        {
            WeaveLogError(DeviceManager, "Must disable ConnectionMonitor before RPR");
        }
        else
        {
            WeaveLogError(DeviceManager, "RPR failed, other operation in progress, opState = %d", mOpState);
        }
        err = WEAVE_ERROR_INCORRECT_STATE;
        ExitNow();
    }
    else if (onComplete == NULL || onError == NULL)
    {
        if (onComplete == NULL)
        {
            WeaveLogError(DeviceManager, "null onComplete");
        }
        else
        {
            WeaveLogError(DeviceManager, "null onError");
        }
        err = WEAVE_ERROR_INVALID_ARGUMENT;
        ExitNow();
    }

    // Save rendezvous and inactivity timeout values, in case we need to reestablish RPR with assisting device and
    // pack these values into another RPR request.
    mRemotePassiveRendezvousTimeout = rendezvousTimeoutSec;
    mRemotePassiveRendezvousInactivityTimeout = inactivityTimeoutSec;
    mRemoteDeviceAddr = rendezvousDeviceAddr;

    // Construct Remote Passive Rendezvous Request.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    LittleEndian::Write16(p, rendezvousTimeoutSec);
    LittleEndian::Write16(p, inactivityTimeoutSec);

    // Encode filter address in standard big-endian, big-wordian (sic) form.
    rendezvousDeviceAddr.WriteAddress(p);

    // Set message buffer data length.
    msgBuf->SetDataLength(DeviceControl::kMessageLength_RemotePassiveRendezvous);

    // Hook DM return callbacks, app state, and OpState.
    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_RemotePassiveRendezvousRequest;

    // Start client-side timer for rendezvous with remote host.
    if (!mRemotePassiveRendezvousTimerIsRunning) // In retry case, don't restart timer
    {
        err = StartRemotePassiveRendezvousTimer();
        SuccessOrExit(err);
    }

    WeaveLogProgress(DeviceManager, "Sending RPR request...");
    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_RemotePassiveRendezvous,
            msgBuf, HandleDeviceControlResponse);
    msgBuf = NULL;
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "RemotePassiveRendezvous failed, err = %d", err);

        // Cancel RemotePassiveRendezvous timer, clear OpState and free saved copy of pairing code, leaving
        // connection to assisting device open.
        CancelRemotePassiveRendezvous();
    }

    return err;
}

WEAVE_ERROR WeaveDeviceManager::SaveAssistingDeviceConnectionInfo()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Save info needed to reconnect to assisting device.
    mAssistingDeviceAddr = mDeviceAddr;
    mAssistingDeviceIntf = mDeviceIntf;
    mAssistingDeviceId = mDeviceId;

    // Clear previous copy of assisting device auth key, if any.
    ClearAuthKey(mAssistingDeviceAuthKey, mAssistingDeviceAuthKeyLen);

    // Save copy of info needed to reauthenticate with assisting device from scratch.
    mAssistingDeviceAuthType = mAuthType;
    mAssistingDeviceAuthKeyLen = mAuthKeyLen;

    mAssistingDeviceAuthKey = malloc(mAuthKeyLen);
    VerifyOrExit(mAssistingDeviceAuthKey != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(mAssistingDeviceAuthKey, mAuthKey, mAuthKeyLen);

exit:
    return err;
}

void WeaveDeviceManager::RestoreAssistingDeviceAddressInfo()
{
    // Restore info needed to reconnect to assisting device.
    mDeviceAddr = mAssistingDeviceAddr;
    mDeviceIntf = mAssistingDeviceIntf;
    mDeviceId = mAssistingDeviceId;
}

WEAVE_ERROR WeaveDeviceManager::RestoreAssistingDeviceAuthInfo()
{
    // Restore info needed to reestablish secure session with assisting device from scratch
    mAuthType = mAssistingDeviceAuthType;

    // SaveAuthKey securely clears existing mAuthKey, if any
    return SaveAuthKey(static_cast<const uint8_t *>(mAssistingDeviceAuthKey), mAssistingDeviceAuthKeyLen);
}

void WeaveDeviceManager::ResetConnectionInfo()
{
    mSessionKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mDeviceCon->PeerNodeId = kNodeIdNotSpecified;
    mDeviceId = kNodeIdNotSpecified;
    mDeviceAddr = IPAddress::Any;
    mDeviceIntf = INET_NULL_INTERFACEID;
}

void WeaveDeviceManager::HandleAssistingDeviceReconnectCompleteEntry(WeaveDeviceManager *devMgr, void *appReqState)
{
    devMgr->HandleRemotePassiveRendezvousReconnectComplete();
}

void WeaveDeviceManager::HandleRemotePassiveRendezvousReconnectComplete()
{
    WEAVE_ERROR err = RemotePassiveRendezvous(mRemoteDeviceAddr, static_cast<const char*>(mRemoteDeviceAuthKey),
            mRemotePassiveRendezvousTimeout, mRemotePassiveRendezvousInactivityTimeout,
            mAppReqState, mOnRemotePassiveRendezvousComplete, mOnError);

    if (err != WEAVE_NO_ERROR)
    {
        mOnError(this, mAppReqState, err, NULL);
    }
}

WEAVE_ERROR WeaveDeviceManager::StartReconnectToAssistingDevice()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Save application's OnComplete callback. mOnComplete will be temporarily overwritten by arg to ConnectDevice()
    // below while the DM attempts to reconnect to the assisting device.
    mOnRemotePassiveRendezvousComplete = mOnComplete.General;

    // Restore assisting device address info for use by common ConnectDevice() code path.
    RestoreAssistingDeviceAddressInfo();

    // Restore auth info for assisting device so we attempt re-authentication from scratch.
    err = RestoreAssistingDeviceAuthInfo();
    SuccessOrExit(err);

    // Reconnect to assisting device, using same auth type and credentials with which we last connected to it.
    switch (mAuthType)
    {
    case kAuthType_PASEWithPairingCode:
        WeaveLogProgress(DeviceManager, "Reconnecting to assisting device with PASE auth");
        err = ConnectDevice(mDeviceId, mDeviceAddr, static_cast<const char *>(mAuthKey), mAppReqState,
                HandleAssistingDeviceReconnectCompleteEntry, mOnError);
        break;

    case kAuthType_CASEWithAccessToken:
        WeaveLogProgress(DeviceManager, "Reconnecting to assisting device with CASE auth");
        err = ConnectDevice(mDeviceId, mDeviceAddr, static_cast<const uint8_t *>(mAuthKey), mAuthKeyLen, mAppReqState,
                HandleAssistingDeviceReconnectCompleteEntry, mOnError);
        break;

    case kAuthType_None:
        WeaveLogProgress(DeviceManager, "Reconnecting to assisting device without authentication");
        err = ConnectDevice(mDeviceId, mDeviceAddr, mAppReqState,
                HandleAssistingDeviceReconnectCompleteEntry, mOnError);
        break;

    default:
        err = WEAVE_ERROR_INCORRECT_STATE;
        break;
    }

exit:
    return err;
}

void WeaveDeviceManager::CancelRemotePassiveRendezvous()
{
    // Clear any OpState set by Remote Passive Rendezvous process.
    ClearOpState();

    // Clear dynamically-allocated copy of assisting device auth key.
    ClearAuthKey(mAssistingDeviceAuthKey, mAssistingDeviceAuthKeyLen);

    // Clear dynamically-allocated remote device auth key.
    ClearAuthKey(mRemoteDeviceAuthKey, mRemoteDeviceAuthKeyLen);

    // Cancel Remote Passive Rendezvous timer if it's running.
    CancelRemotePassiveRendezvousTimer();
}

WEAVE_ERROR WeaveDeviceManager::ReconnectDevice(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mOpState != kOpState_Idle || mConState != kConnectionState_NotConnected)
        return WEAVE_ERROR_INCORRECT_STATE;

    if (mDeviceId == kNodeIdNotSpecified || mDeviceAddr == IPAddress::Any)
        return WEAVE_ERROR_INCORRECT_STATE;

    mDeviceCriteria.Reset();

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;

    mOpState = kOpState_ReconnectDevice;

    err = InitiateConnection();
    if (err != WEAVE_NO_ERROR)
        ClearOpState();

    return err;
}

WEAVE_ERROR WeaveDeviceManager::EnableConnectionMonitor(uint16_t interval, uint16_t timeout, void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    LittleEndian::Write16(p, timeout);
    LittleEndian::Write16(p, interval);
    msgBuf->SetDataLength(4);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_EnableConnectionMonitor;

    CancelConnectionMonitorTimer();
    mConMonitorEnabled = false;
    mConMonitorInterval = interval;
    mConMonitorTimeout = timeout;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_EnableConnectionMonitor, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::DisableConnectionMonitor(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    CancelConnectionMonitorTimer();
    mConMonitorEnabled = false;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_DisableConnectionMonitor;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_DisableConnectionMonitor, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

void WeaveDeviceManager::Close()
{
    Close(false);
}

void WeaveDeviceManager::Close(bool graceful)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Cancel outstanding Remote Passive Rendezvous attempt, if any, and clear associated state.
    CancelRemotePassiveRendezvous();

    // Close connection to device, if any, and clear associated state.
    CloseDeviceConnection(graceful);

    // Cancel our unsecured listen, if enabled.
    err = ClearUnsecuredConnectionHandler();
    if (err != WEAVE_NO_ERROR)
        WeaveLogProgress(DeviceControl, "ClearUnsecuredConnectionListener failed, err = %d", err);

    // If this instance of the device manager was performing a passive rendezvous, clear any associated state.
    if (sListeningDeviceMgr == this)
        sListeningDeviceMgr = NULL;
}

void WeaveDeviceManager::CloseDeviceConnection()
{
    CloseDeviceConnection(false);
}

void WeaveDeviceManager::CloseDeviceConnection(bool graceful)
{
    WeaveLogProgress(DeviceManager, "Closing connection to device");

    // Clear the current operation state.  NOTE that calling CloseDeviceConnection() with an operation outstanding
    // results in the operation's completion functions never being called.
    ClearOpState();

    // Close the connection to the device.
    if (mDeviceCon != NULL)
    {
        if (graceful)
        {
            mDeviceCon->Close();
        }
        else
        {
            mDeviceCon->OnConnectionComplete = NULL;
            mDeviceCon->OnConnectionClosed = NULL;
            mDeviceCon->Abort();
            mDeviceCon = NULL;
        }
    }

    // Cancel any outstanding timers.
    mSystemLayer->CancelTimer(HandleConnectionIdentifyTimeout, this);
    mSystemLayer->CancelTimer(RetrySession, this);
    CancelConnectionMonitorTimer();

    // Reset various state.
    //
    // NOTE: The following are expressly not reset when Close() is called to
    // allow internal callers to continue to use these values during clean-up
    // and error reporting.
    //
    //     mDeviceId/mDeviceAddr/mDeviceIntf
    //     mRendezvousAddr
    //     mAuthType/mAuthKey/mAuthKeyLen
    //     mOpTimeout
    //     mAutoReconnect
    //     mOnComplete/mOnError
    //     mAppReqState
    //     mConMonitorEnabled/mConMonitorInterval/mConMonitorTimeout
    //
    //
    mConState = kConnectionState_NotConnected;
    mConTryCount = 0;
    mSessionKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mConnectedToRemoteDevice = false;
    if (mTokenPairingCertificate != NULL)
    {
        free(mTokenPairingCertificate);
        mTokenPairingCertificate = NULL;
        mTokenPairingCertificateLen = 0;
    }

}

bool WeaveDeviceManager::IsConnected() const
{
    return mConState == kConnectionState_Connected;
}

void WeaveDeviceManager::SetConnectionClosedCallback(ConnectionClosedFunc onConnecionClosedFunc, void *onConnecionClosedAppReq)
{
    mOnConnectionClosedFunc = onConnecionClosedFunc;
    mOnConnectionClosedAppReq = onConnecionClosedAppReq;
}

WEAVE_ERROR WeaveDeviceManager::IdentifyDevice(void* appReqState, IdentifyDeviceCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf  = NULL;
    IdentifyRequestMessage  reqMsg;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    reqMsg.TargetFabricId = kTargetFabricId_Any;
    reqMsg.TargetModes = kTargetDeviceMode_Any;
    reqMsg.TargetVendorId = 0xFFFF; // Any vendor
    reqMsg.TargetProductId = 0xFFFF; // Any product

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = reqMsg.Encode(msgBuf);
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.IdentifyDevice = onComplete;
    mOnError = onError;
    mOpState = kOpState_IdentifyDevice;

    err = SendRequest(kWeaveProfile_DeviceDescription, DeviceDescription::kMessageType_IdentifyRequest,
            msgBuf, HandleIdentifyDeviceResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::PairToken(const uint8_t *pairingToken, uint32_t pairingTokenLen, void* appReqState, PairTokenCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    VerifyOrExit(msgBuf->AvailableDataLength() > pairingTokenLen, err = WEAVE_ERROR_MESSAGE_TOO_LONG);
    memcpy(p, pairingToken, pairingTokenLen);
    msgBuf->SetDataLength(pairingTokenLen);

    mAppReqState = appReqState;
    mOnComplete.PairToken = onComplete;
    mOnError = onError;
    mOpState = kOpState_PairToken;

    if (mTokenPairingCertificate != NULL)
    {
        WeaveLogError(DeviceManager, "% TokenPairingCertificate not NULL.", __FUNCTION__);
        mTokenPairingCertificate = NULL;
        mTokenPairingCertificateLen = 0;
    }

    err = SendRequest(kWeaveProfile_TokenPairing, TokenPairing::kMsgType_PairTokenRequest,
            msgBuf, HandlePairTokenResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::UnpairToken(void* appReqState, UnpairTokenCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mAppReqState = appReqState;
    mOnComplete.UnpairToken = onComplete;
    mOnError = onError;
    mOpState = kOpState_UnpairToken;

    err = SendRequest(kWeaveProfile_TokenPairing, TokenPairing::kMsgType_UnpairTokenRequest,
            msgBuf, HandleUnpairTokenResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ScanNetworks(NetworkType networkType, void* appReqState,
        NetworkScanCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    Put8(msgBuf->Start(), (uint8_t) networkType);
    msgBuf->SetDataLength(1);

    mAppReqState = appReqState;
    mOnComplete.ScanNetworks = onComplete;
    mOnError = onError;
    mOpState = kOpState_ScanNetworks;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_ScanNetworks, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::AddNetwork(const NetworkInfo *netInfo, void* appReqState,
        AddNetworkCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint16_t        msgType = kMsgType_AddNetworkV2;
    TLVWriter       writer;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(msgBuf);

    err = netInfo->Encode(writer, NetworkInfo::kEncodeFlag_All);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.AddNetwork = onComplete;
    mOnError = onError;
    mOpState = kOpState_AddNetwork;

#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE

#if WEAVE_CONFIG_ALWAYS_USE_LEGACY_ADD_NETWORK_MESSAGE
    // Revert to the deprecated, legacy msg type
    msgType = kMsgType_AddNetwork;
#else
    // Create duplicate of a message buffer.
    // If device returns error indicating that the new message type is not supported
    // this retained message will be re-sent to the device as an old AddNetwork() message type.
    mCurReqMsgRetained = PacketBuffer::NewWithAvailableSize(msgBuf->DataLength());
    VerifyOrExit(mCurReqMsgRetained != NULL, err = WEAVE_ERROR_NO_MEMORY);

    memcpy(mCurReqMsgRetained->Start(), msgBuf->Start(), msgBuf->DataLength());
    mCurReqMsgRetained->SetDataLength(msgBuf->DataLength());

    // Identify if this request creates new Thread network.
    mCurReqCreateThreadNetwork = (netInfo->NetworkType == kNetworkType_Thread) && (netInfo->ThreadExtendedPANId == NULL);
#endif // WEAVE_CONFIG_ALWAYS_USE_LEGACY_ADD_NETWORK_MESSAGE

#endif // WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE

    err = SendRequest(kWeaveProfile_NetworkProvisioning, msgType, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::UpdateNetwork(const NetworkInfo *netInfo, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    TLVWriter       writer;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(msgBuf);

    err = netInfo->Encode(writer, NetworkInfo::kEncodeFlag_All);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_UpdateNetwork;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, kMsgType_UpdateNetwork, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::RemoveNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    LittleEndian::Put32(msgBuf->Start(), networkId);
    msgBuf->SetDataLength(4);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_RemoveNetwork;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_RemoveNetwork, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetCameraAuthData(const char* nonce, void* appReqState,
        GetCameraAuthDataCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *msgBuf = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    // Validate args
    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(NULL != nonce, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(strlen(nonce) == CAMERA_NONCE_LEN, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    VerifyOrExit(mCameraNonce == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Save copy of nonce for HandleGetCameraAuthResponse callback
    VerifyOrExit(asprintf(&mCameraNonce, "%s", nonce) > 0, err = WEAVE_ERROR_NO_MEMORY);

    err = EncodeCameraAuthDataRequest(msgBuf, nonce);
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.GetCameraAuthData = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetCameraAuthData;

    err = SendRequest(kWeaveProfile_DropcamLegacyPairing, kMsgType_CameraAuthDataRequest, msgBuf,
            HandleGetCameraAuthDataResponseEntry);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetNetworks(uint8_t flags, void* appReqState,
        GetNetworksCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    Put8(msgBuf->Start(), (uint8_t) flags);
    msgBuf->SetDataLength(1);

    mAppReqState = appReqState;
    mOnComplete.GetNetworks = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetNetworks;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_GetNetworks, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetActiveLocale(void* appReqState, GetActiveLocaleCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t deviceId = kNodeIdNotSpecified;
    const uint16_t txnId = 1;
    const uint32_t timeout = 10000; // milliseconds
    ReferencedTLVData pathList;

    mAppReqState = appReqState;
    mOnComplete.GetActiveLocale = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetActiveLocale;

    err = GetDeviceId(deviceId);
    SuccessOrExit(err);

    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDMClient.BindRequest(mDeviceCon);
    SuccessOrExit(err);

    err = pathList.init(WriteLocaleRequest, this);
    SuccessOrExit(err);

    err = mDMClient.ViewRequest(pathList, txnId, timeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        ClearOpState();
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetAvailableLocales(void* appReqState, GetAvailableLocalesCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t deviceId = kNodeIdNotSpecified;
    const uint16_t txnId = 1;
    const uint32_t timeout = 10000; // milliseconds
    ReferencedTLVData pathList;

    mAppReqState = appReqState;
    mOnComplete.GetAvailableLocales = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetAvailableLocales;

    err = GetDeviceId(deviceId);
    SuccessOrExit(err);

    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDMClient.BindRequest(mDeviceCon);
    SuccessOrExit(err);

    err = pathList.init(WriteLocaleRequest, this);
    SuccessOrExit(err);

    err = mDMClient.ViewRequest(pathList, txnId, timeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        ClearOpState();
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::SetActiveLocale(void* appReqState, const char *aLocale, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t deviceId = kNodeIdNotSpecified;
    const uint16_t txnId = 1;
    const uint32_t timeout = 10000; // milliseconds
    ReferencedTLVData dataList;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_SetActiveLocale;
    mActiveLocale = aLocale;

    err = GetDeviceId(deviceId);
    SuccessOrExit(err);

    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDMClient.BindRequest(mDeviceCon);
    SuccessOrExit(err);

    err = dataList.init(WriteLocaleRequest, this);
    SuccessOrExit(err);

    err = mDMClient.UpdateRequest(dataList, txnId, timeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        ClearOpState();
    }
    return err;
}

void WeaveDeviceManager::WriteLocaleRequest(TLVWriter &aWriter, void *ctx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint16_t pathLen = 1;

    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *) ctx;

    switch (deviceMgr->mOpState)
    {
        case kOpState_GetActiveLocale:
            err = StartPathList(aWriter);
            SuccessOrExit(err);

            err = EncodePath(aWriter,
                             AnonymousTag,
                             kWeaveProfile_Locale,
                             kInstanceIdNotSpecified,
                             pathLen,
                             ProfileTag(kWeaveProfile_Locale, Locale::kTag_ActiveLocale)
                             );
            SuccessOrExit(err);

            err =  EndList(aWriter);
            SuccessOrExit(err);

            err = aWriter.Finalize();
            break;

        case kOpState_SetActiveLocale:
            WeaveLogProgress(DeviceManager, "Set active locale to %s", deviceMgr->mActiveLocale);

            err = StartDataList(aWriter);
            SuccessOrExit(err);

            err = StartDataListElement(aWriter);
            SuccessOrExit(err);

            err = EncodePath(aWriter,
                             ContextTag(kTag_WDMDataListElementPath),
                             kWeaveProfile_Locale,
                             kInstanceIdNotSpecified,
                             pathLen,
                             ProfileTag(kWeaveProfile_Locale, Locale::kTag_ActiveLocale)
                             );
            SuccessOrExit(err);

            err = aWriter.Put(ContextTag(kTag_WDMDataListElementVersion), (uint64_t)1);
            SuccessOrExit(err);

            err = aWriter.PutString(ContextTag(kTag_WDMDataListElementData), deviceMgr->mActiveLocale);
            SuccessOrExit(err);

            err = EndDataListElement(aWriter);
            SuccessOrExit(err);

            err = EndList(aWriter);
            SuccessOrExit(err);

            err = aWriter.Finalize();
            break;

        case kOpState_GetAvailableLocales:
            err = StartPathList(aWriter);
            SuccessOrExit(err);

            err = EncodePath(aWriter,
                             AnonymousTag,
                             kWeaveProfile_Locale,
                             kInstanceIdNotSpecified,
                             pathLen,
                             ProfileTag(kWeaveProfile_Locale, Locale::kTag_AvailableLocales)
                             );
            SuccessOrExit(err);

            err =  EndList(aWriter);
            SuccessOrExit(err);

            err = aWriter.Finalize();
            break;

        default:
            WeaveLogError(DeviceManager, "Incorrect OpState for %s: %d", __FUNCTION__, deviceMgr->mOpState);
            err = WEAVE_ERROR_INCORRECT_STATE;
            break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
    }
}

WEAVE_ERROR WeaveDeviceManager::ThermostatGetEntryKey(void* appReqState, ThermostatGetEntryKeyCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t deviceId = kNodeIdNotSpecified;
    const uint16_t txnId = 1;
    const uint32_t timeout = 10000; // milliseconds
    ReferencedTLVData pathList;

    mAppReqState = appReqState;
    mOnComplete.ThermostatGetEntryKey = onComplete;
    mOnError = onError;
    mOpState = kOpState_ThermostatGetEntryKey;

    err = GetDeviceId(deviceId);
    SuccessOrExit(err);

    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDMClient.BindRequest(mDeviceCon);
    SuccessOrExit(err);

    err = pathList.init(WriteThermostatRequest, this);
    SuccessOrExit(err);

    err = mDMClient.ViewRequest(pathList, txnId, timeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        ClearOpState();
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ThermostatSystemTestStatus(void* appReqState, ThermostatSystemTestStatusCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t deviceId = kNodeIdNotSpecified;
    const uint16_t txnId = 1;
    const uint32_t timeout = 10000; // milliseconds
    ReferencedTLVData pathList;

    mAppReqState = appReqState;
    mOnComplete.ThermostatSystemStatus = onComplete;
    mOnError = onError;
    mOpState = kOpState_ThermostatSystemTestStatus;

    err = GetDeviceId(deviceId);
    SuccessOrExit(err);

    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    err = mDMClient.BindRequest(mDeviceCon);
    SuccessOrExit(err);

    err = pathList.init(WriteThermostatRequest, this);
    SuccessOrExit(err);

    err = mDMClient.ViewRequest(pathList, txnId, timeout);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        ClearOpState();
    }
    return err;
}

void WeaveDeviceManager::WriteThermostatRequest(TLVWriter &aWriter, void *ctx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint16_t pathLen = 1;

    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *) ctx;

    switch (deviceMgr->mOpState)
    {
        case kOpState_ThermostatGetEntryKey:
            err = StartPathList(aWriter);
            SuccessOrExit(err);

            err = EncodePath(aWriter,
                             AnonymousTag,
                             kWeaveProfile_NestThermostat,
                             kInstanceIdNotSpecified,
                             pathLen,
                             ProfileTag(kWeaveProfile_NestThermostat, Vendor::Nestlabs::Thermostat::kTag_LegacyEntryKey)
                             );
            SuccessOrExit(err);

            err =  EndList(aWriter);
            SuccessOrExit(err);

            err = aWriter.Finalize();
            break;

        case kOpState_ThermostatSystemTestStatus:
            err = StartPathList(aWriter);
            SuccessOrExit(err);

            err = EncodePath(aWriter,
                             AnonymousTag,
                             kWeaveProfile_NestThermostat,
                             kInstanceIdNotSpecified,
                             pathLen,
                             ProfileTag(kWeaveProfile_NestThermostat, Vendor::Nestlabs::Thermostat::kTag_SystemTestStatusKey)
                             );
            SuccessOrExit(err);

            err =  EndList(aWriter);
            SuccessOrExit(err);

            err = aWriter.Finalize();
            break;

        default:
            WeaveLogError(DeviceManager, "Incorrect OpState for %s: %d", __FUNCTION__, deviceMgr->mOpState);
            err = WEAVE_ERROR_INCORRECT_STATE;
            break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
    }
}

WEAVE_ERROR WeaveDeviceManager::EnableNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    LittleEndian::Put32(msgBuf->Start(), networkId);
    msgBuf->SetDataLength(4);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_EnableNetwork;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_EnableNetwork, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::DisableNetwork(uint32_t networkId, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    LittleEndian::Put32(msgBuf->Start(), networkId);
    msgBuf->SetDataLength(4);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_DisableNetwork;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_DisableNetwork, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::TestNetworkConnectivity(uint32_t networkId, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    LittleEndian::Put32(msgBuf->Start(), networkId);
    msgBuf->SetDataLength(4);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_TestNetworkConnectivity;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_TestConnectivity, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetRendezvousMode(void* appReqState, GetRendezvousModeCompleteFunct onComplete,
        ErrorFunct onError)
{
    return WEAVE_ERROR_NOT_IMPLEMENTED;
}

WEAVE_ERROR WeaveDeviceManager::SetRendezvousMode(uint16_t modeFlags, void* appReqState, CompleteFunct onComplete,
        ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    LittleEndian::Put16(msgBuf->Start(), modeFlags);
    msgBuf->SetDataLength(2);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_SetRendezvousMode;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_SetRendezvousMode, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetLastNetworkProvisioningResult(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetLastNPResult;

    err = SendRequest(kWeaveProfile_NetworkProvisioning, NetworkProvisioning::kMsgType_GetLastResult, msgBuf,
            HandleNetworkProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::CreateFabric(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    msgBuf->SetDataLength(0);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_CreateFabric;

    err = SendRequest(kWeaveProfile_FabricProvisioning, FabricProvisioning::kMsgType_CreateFabric, msgBuf,
            HandleFabricProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::LeaveFabric(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    msgBuf->SetDataLength(0);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_LeaveFabric;

    err = SendRequest(kWeaveProfile_FabricProvisioning, FabricProvisioning::kMsgType_LeaveFabric, msgBuf,
            HandleFabricProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GetFabricConfig(void* appReqState, GetFabricConfigCompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    msgBuf->SetDataLength(0);

    mAppReqState = appReqState;
    mOnComplete.GetFabricConfig = onComplete;
    mOnError = onError;
    mOpState = kOpState_GetFabricConfig;

    err = SendRequest(kWeaveProfile_FabricProvisioning, FabricProvisioning::kMsgType_GetFabricConfig, msgBuf,
            HandleFabricProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::JoinExistingFabric(const uint8_t *fabricConfig, uint32_t fabricConfigLen,
        void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    VerifyOrExit(msgBuf->AvailableDataLength() >= fabricConfigLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
    memcpy(msgBuf->Start(), fabricConfig, fabricConfigLen);
    msgBuf->SetDataLength(fabricConfigLen);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_JoinExistingFabric;

    err = SendRequest(kWeaveProfile_FabricProvisioning, FabricProvisioning::kMsgType_JoinExistingFabric, msgBuf,
            HandleFabricProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::RegisterServicePairAccount(uint64_t serviceId, const char *accountId,
                                                           const uint8_t *serviceConfig, uint16_t serviceConfigLen,
                                                           const uint8_t *pairingToken, uint16_t pairingTokenLen,
                                                           const uint8_t *pairingInitData, uint16_t pairingInitDataLen,
                                                           void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR                         err             = WEAVE_NO_ERROR;
    PacketBuffer*                       msgBuf          = NULL;
    size_t                              accountIdLen    = strlen(accountId);
    RegisterServicePairAccountMessage   msg;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msg.ServiceId = serviceId;
    msg.AccountId = accountId;
    msg.AccountIdLen = accountIdLen;
    msg.ServiceConfig = serviceConfig;
    msg.ServiceConfigLen = serviceConfigLen;
    msg.PairingToken = pairingToken;
    msg.PairingTokenLen = pairingTokenLen;
    msg.PairingInitData = pairingInitData;
    msg.PairingInitDataLen = pairingInitDataLen;

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_RegisterServicePairAccount;

    err = SendRequest(kWeaveProfile_ServiceProvisioning, ServiceProvisioning::kMsgType_RegisterServicePairAccount, msgBuf,
            HandleServiceProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::UpdateService(uint64_t serviceId, const uint8_t *serviceConfig, uint16_t serviceConfigLen,
                                              void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf  = NULL;
    UpdateServiceMessage    msg;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    msg.ServiceId = serviceId;
    msg.ServiceConfig = serviceConfig;
    msg.ServiceConfigLen = serviceConfigLen;

    err = msg.Encode(msgBuf);
    SuccessOrExit(err);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_UpdateService;

    err = SendRequest(kWeaveProfile_ServiceProvisioning, ServiceProvisioning::kMsgType_UpdateService, msgBuf,
            HandleServiceProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::UnregisterService(uint64_t serviceId, void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    LittleEndian::Write64(p, serviceId);
    msgBuf->SetDataLength(8);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_UnregisterService;

    err = SendRequest(kWeaveProfile_ServiceProvisioning, ServiceProvisioning::kMsgType_UnregisterService, msgBuf,
            HandleServiceProvisioningResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ArmFailSafe(uint8_t armMode, uint32_t failSafeToken, void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    Write8(p, armMode);
    LittleEndian::Write32(p, failSafeToken);
    msgBuf->SetDataLength(5);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_ArmFailSafe;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_ArmFailSafe, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::DisarmFailSafe(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    msgBuf->SetDataLength(0);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_DisarmFailSafe;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_DisarmFailSafe, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::StartSystemTest(void* appReqState, uint32_t profileId, uint32_t testId, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    LittleEndian::Write32(p, profileId);
    LittleEndian::Write32(p, testId);
    msgBuf->SetDataLength(DeviceControl::kMessageLength_StartSystemTest);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_StartSystemTest;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_StartSystemTest, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::StopSystemTest(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    msgBuf->SetDataLength(DeviceControl::kMessageLength_StopSystemTest);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_StopSystemTest;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_StopSystemTest, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ResetConfig(uint16_t resetFlags, void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    p = msgBuf->Start();
    LittleEndian::Write16(p, resetFlags);
    msgBuf->SetDataLength(2);

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_ResetConfig;

    err = SendRequest(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_ResetConfig, msgBuf,
            HandleDeviceControlResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::Ping(void* appReqState, CompleteFunct onComplete, ErrorFunct onError)
{
    return Ping(appReqState, 0, onComplete, onError);
}

WEAVE_ERROR WeaveDeviceManager::Ping(void* appReqState, int32_t payloadSize, CompleteFunct onComplete, ErrorFunct onError)
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;

    if (mOpState != kOpState_Idle)
        return WEAVE_ERROR_INCORRECT_STATE;

    VerifyOrExit(onComplete != NULL && onError != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    msgBuf->SetDataLength(payloadSize);

    // Require ping message to fit within one PacketBuffer.
    WeaveLogProgress(DeviceManager, "DataLength: %d, payload: %d, next: %p", msgBuf->DataLength(), payloadSize, msgBuf->Next());
    VerifyOrExit(((msgBuf->DataLength() == payloadSize) && (msgBuf->Next() == NULL)), err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    if (payloadSize > 0) {
        // fill with test pattern
        uint8_t *data = msgBuf->Start();
        for (int i = 0; i < payloadSize; i++) {
            *data++ = 0xff & i;
        }
    }
    // Store ping size so return function can check for truncation.
    mPingSize = payloadSize;

    mAppReqState = appReqState;
    mOnComplete.General = onComplete;
    mOnError = onError;
    mOpState = kOpState_Ping;

    err = SendRequest(kWeaveProfile_Echo, kEchoMessageType_EchoRequest, msgBuf, HandlePingResponse);
    msgBuf = NULL;

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        ClearOpState();
    return err;
}

bool WeaveDeviceManager::IsValidPairingCode(const char *pairingCode)
{
    if (pairingCode == NULL)
        return false;

    size_t len = strlen(pairingCode);

    if (len < 6)
        return false;

    return Verhoeff32::ValidateCheckChar(pairingCode, len);
}

WEAVE_ERROR WeaveDeviceManager::SetRendezvousAddress(IPAddress addr)
{
    if (addr == IPAddress::Any)
        addr = IPAddress::MakeIPv6WellKnownMulticast(kIPv6MulticastScope_Link, kIPV6MulticastGroup_AllNodes);
    mRendezvousAddr = addr;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::SetAutoReconnect(bool autoReconnect)
{
    WEAVE_ERROR ret = WEAVE_NO_ERROR;

    VerifyOrExit(!mConnectedToRemoteDevice, ret = WEAVE_ERROR_INCORRECT_STATE);

    mAutoReconnect = autoReconnect;

exit:
    return ret;
}

WEAVE_ERROR WeaveDeviceManager::SetUseAccessToken(bool useAccessToken)
{
    mUseAccessToken = useAccessToken;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::SetRendezvousLinkLocal(bool RendezvousLinkLocal)
{
    mRendezvousLinkLocal = RendezvousLinkLocal;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::SetConnectTimeout(uint32_t timeoutMS)
{
    mConTimeout = timeoutMS;
    return WEAVE_NO_ERROR;
}

// DEPRECATED
WEAVE_ERROR WeaveDeviceManager::SetWiFiRendezvousAddress(IPAddress addr)
{
    return SetRendezvousAddress(addr);
}

WEAVE_ERROR WeaveDeviceManager::SendRequest(uint32_t profileId, uint16_t msgType, PacketBuffer *msgBuf,
        ExchangeContext::MessageReceiveFunct onMsgRcvd)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there isn't a request already outstanding.
    VerifyOrExit(mCurReq == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Save the information about the new request.
    mCurReqProfileId = profileId;
    mCurReqMsgType = msgType;
    mCurReqMsg = msgBuf;
    msgBuf = NULL;
    mCurReqRcvFunct = onMsgRcvd;

    // If not already connected...
    if (!IsConnected())
    {
        // Return an error if auto-reconnect not enabled.
        VerifyOrExit(mAutoReconnect, err = WEAVE_ERROR_NOT_CONNECTED);

        // Return an error if we haven't previously connected.
        VerifyOrExit(mDeviceId != kNodeIdNotSpecified && mDeviceAddr != IPAddress::Any, err = WEAVE_ERROR_NOT_CONNECTED);

        // Initiate a new connection to the previously connected device.
        mDeviceCriteria.Reset();
        err = InitiateConnection();
    }

    // Otherwise, there is a connection so send the request immediately.
    else
        err = SendPendingRequest();

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
    {
        if (mCurReqMsgRetained != NULL)
        {
            PacketBuffer::Free(mCurReqMsgRetained);
            mCurReqMsgRetained = NULL;
        }
        ClearRequestState();
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::SendPendingRequest()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there's a request ready to go.
    VerifyOrExit(mCurReqMsg != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify we have a connection.
    VerifyOrExit(IsConnected(), err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify there isn't already a request in progress.
    VerifyOrExit(mCurReq == NULL && mCurReqMsg != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // Create and initialize an exchange context for the request.
    mCurReq = mExchangeMgr->NewContext(mDeviceId, this);
    VerifyOrExit(mCurReq != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mCurReq->Con = mDeviceCon;
    mCurReq->KeyId = mSessionKeyId;
    mCurReq->EncryptionType = mEncType;
    mCurReq->OnMessageReceived = mCurReqRcvFunct;
    mCurReq->OnConnectionClosed = HandleRequestConnectionClosed;

    // TODO: setup request timeout

    // Send the current request over the connection.
    err = mCurReq->SendMessage(mCurReqProfileId, mCurReqMsgType, mCurReqMsg, 0);
    mCurReqMsg = NULL;

exit:
    if (mCurReqMsg)
    {
        PacketBuffer::Free(mCurReqMsg);
        mCurReqMsg = NULL;
    }

    if (err != WEAVE_NO_ERROR)
        ClearRequestState();
    return err;
}

WEAVE_ERROR WeaveDeviceManager::SaveAuthKey(const char *pairingCode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(pairingCode != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (mAuthKey != pairingCode)
    {
        ClearAuthKey();
        mAuthKey = strdup(pairingCode);
        VerifyOrExit(mAuthKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

        VerifyOrExit(mMessageLayer != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
        VerifyOrExit(mMessageLayer->FabricState != NULL, err = WEAVE_ERROR_INCORRECT_STATE);
        mMessageLayer->FabricState->PairingCode = static_cast<const char *>(mAuthKey);
    }

    mAuthKeyLen = strlen(pairingCode);
    VerifyOrExit(mAuthKeyLen <= kMaxPairingCodeLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    if (err != WEAVE_NO_ERROR)
        ClearAuthKey();

    return err;
}

WEAVE_ERROR WeaveDeviceManager::SaveAuthKey(const uint8_t *accessToken, uint32_t accessTokenLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(accessToken != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (mAuthKey != accessToken)
    {
        ClearAuthKey();
        mAuthKey = malloc(accessTokenLen);
        VerifyOrExit(mAuthKey != NULL, err = WEAVE_ERROR_NO_MEMORY);
        memcpy(mAuthKey, accessToken, accessTokenLen);
    }

    mAuthKeyLen = accessTokenLen;

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::SaveRemoteDeviceAuthInfo(uint8_t authType, const char *authKey, uint32_t authKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mRemoteDeviceAuthType = authType;

    switch (authType)
    {
    case kAuthType_PASEWithPairingCode:
        if (mRemoteDeviceAuthKey != authKey)
        {
            ClearAuthKey(mRemoteDeviceAuthKey, mRemoteDeviceAuthKeyLen);
            mRemoteDeviceAuthKey = strdup(authKey);
            VerifyOrExit(mRemoteDeviceAuthKey != NULL, err = WEAVE_ERROR_NO_MEMORY);
        }

        mRemoteDeviceAuthKeyLen = strlen(authKey);
        VerifyOrExit(mRemoteDeviceAuthKeyLen <= kMaxPairingCodeLength, err = WEAVE_ERROR_INVALID_ARGUMENT);
        break;

    case kAuthType_CASEWithAccessToken:
        if (mRemoteDeviceAuthKey != authKey)
        {
            ClearAuthKey(mRemoteDeviceAuthKey, mRemoteDeviceAuthKeyLen);
            mRemoteDeviceAuthKey = malloc(authKeyLen);
            VerifyOrExit(mRemoteDeviceAuthKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

            memcpy(mRemoteDeviceAuthKey, authKey, authKeyLen);
        }

        mRemoteDeviceAuthKeyLen = authKeyLen;
        break;

    case kAuthType_None:
        break;

    default:
        err = WEAVE_ERROR_INVALID_ARGUMENT;
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearAuthKey(mRemoteDeviceAuthKey, mRemoteDeviceAuthKeyLen);
        mRemoteDeviceAuthType = kAuthType_None;
    }

    return err;
}

void WeaveDeviceManager::ClearAuthKey()
{
    ClearAuthKey(mAuthKey, mAuthKeyLen);

    if (mMessageLayer && mMessageLayer->FabricState)
    {
        mMessageLayer->FabricState->PairingCode = NULL;
    }
}

void WeaveDeviceManager::ClearAuthKey(void *&authKey, uint32_t &authKeyLen)
{
    if (authKey != NULL)
    {
        nl::Weave::Crypto::ClearSecretData((uint8_t *)authKey, authKeyLen);
        free(authKey);
        authKey = NULL;
    }

    authKeyLen = 0;
}

void WeaveDeviceManager::ClearRequestState()
{
    if (mCurReq != NULL)
    {
        mCurReq->Close();
        mCurReq = NULL;
    }

    if (mCurReqMsg != NULL)
    {
        PacketBuffer::Free(mCurReqMsg);
        mCurReqMsg = NULL;
    }

    if (mCameraNonce != NULL)
    {
        free(mCameraNonce);
        mCameraNonce = NULL;
    }

    mCurReqProfileId = 0;
    mCurReqMsgType = 0;
    mCurReqRcvFunct = NULL;
}

void WeaveDeviceManager::ClearOpState()
{
    if (mCurReqMsgRetained != NULL)
    {
        PacketBuffer::Free(mCurReqMsgRetained);
        mCurReqMsgRetained = NULL;
    }

    ClearRequestState();

    mOpState = kOpState_Idle;
}

void WeaveDeviceManager::HandleUnsecuredConnectionCallbackRemoved(void *appState)
{
    WeaveDeviceManager *devMgr = static_cast<WeaveDeviceManager *>(appState);

    // Ensure we don't call ClearUnsecuredConnectionListener after our listener has been removed.
    devMgr->mIsUnsecuredConnectionListenerSet = false;

    // If another application has pre-empted our (Remote)PassiveRendezvous, close it down.
    devMgr->Close();

    // Tell the application we can't complete the requested operation.
    devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_CALLBACK_REPLACED, NULL);
}

void WeaveDeviceManager::HandleRequestConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    if (devMgr->mOpState == kOpState_Idle || ec != devMgr->mCurReq)
    {
        ec->Close();
        return;
    }

    // Cancel timers and clear state.
    devMgr->Close();

    // Call the user's OnError callback.
    devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY, NULL);
}

WEAVE_ERROR WeaveDeviceManager::InitiateConnection()
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    PacketBuffer*           msgBuf  = NULL;
    IdentifyRequestMessage  reqMsg;
    uint16_t                sendFlags;

    VerifyOrExit(mConState == kConnectionState_NotConnected || mConState == kConnectionState_IdentifyDevice,
        err = WEAVE_ERROR_INCORRECT_STATE);


    // If starting from the NotConnected state, reset the connection identify count.
    if (mConState == kConnectionState_NotConnected)
    {
        WeaveLogProgress(DeviceManager, "Initiating connection to device");
        mConTryCount = 0;
    }

    // Refresh the message layer endpoints to cope with changes in network interface status
    // (e.g. new addresses being assigned).
    err = mMessageLayer->RefreshEndpoints();
    SuccessOrExit(err);

    // Form an Identify device request containing the device criteria specified by the application.
    reqMsg.TargetFabricId = mDeviceCriteria.TargetFabricId;
    reqMsg.TargetModes = mDeviceCriteria.TargetModes;
    reqMsg.TargetVendorId = mDeviceCriteria.TargetVendorId;

    if (mDeviceCriteria.TargetVendorId == kWeaveVendor_NestLabs && IsProductWildcard(mDeviceCriteria.TargetProductId))
    {
        reqMsg.TargetProductId = 0xFFFF;
    }
    else
    {
        reqMsg.TargetProductId = mDeviceCriteria.TargetProductId;
    }

    // Encode the Identify device request message.
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = reqMsg.Encode(msgBuf);
    SuccessOrExit(err);

    // Construct an exchange context if needed.
    if (mCurReq == NULL)
    {
        InterfaceId targetIntf = (mDeviceAddr.IsIPv6LinkLocal()) ? mDeviceIntf : INET_NULL_INTERFACEID;
        mCurReq = mExchangeMgr->NewContext(mDeviceId, mDeviceAddr, WEAVE_PORT, targetIntf, this);
        VerifyOrExit(mCurReq != NULL, err = WEAVE_ERROR_NO_MEMORY);
        mCurReq->OnMessageReceived = HandleConnectionIdentifyResponse;

        // TODO setup request timeout
    }

    WeaveLogProgress(DeviceManager, "Sending IdentifyRequest to locate device");

    mConState = kConnectionState_IdentifyDevice;

    // Send the Identify message.
    //
    // If performing a multicast identify, AND the 'rendezvous link-local' option is enabled,
    // AND the message layer is not bound to a specific local IPv6 address, THEN ...
    //
    // Send the multicast identify request from the host's link-local addresses, rather than
    // from its site-local or global addresses. This results in the device responding using its
    // link-local address, which in turn causes the device manager to connect to the device using the
    // link-local address. This is all to work around a bug in OS X/iOS that prevents those systems
    // from communicating on any site-local IPv6 subnets when in the presence of a router that is
    // advertising a default route to the Internet at large. (See DOLO-2479).
    //
    // We disable the 'rendezvous link-local' feature when the message layer is bound to a specific
    // address because binding to a specific address is generally used when testing the device manager
    // and a mock-device running on a single host with a single interface.  In this case multicasting
    // using the interface's single link-local address doesn't work.
    //
    sendFlags = (mDeviceAddr.IsMulticast() && mRendezvousLinkLocal && !mMessageLayer->IsBoundToLocalIPv6Address())
            ? ExchangeContext::kSendFlag_MulticastFromLinkLocal
            : 0;
    err = mCurReq->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, msgBuf, sendFlags);
    msgBuf = NULL;
    if (err == System::MapErrorPOSIX(ENETUNREACH) || err == System::MapErrorPOSIX(EHOSTUNREACH) ||
        err == System::MapErrorPOSIX(EPIPE))
        err = WEAVE_NO_ERROR;
    SuccessOrExit(err);

    // Arm the retry timer.
    err = mSystemLayer->StartTimer(kConRetryInterval, HandleConnectionIdentifyTimeout, this);
    SuccessOrExit(err);

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR)
        Close();
    return err;
}

bool WeaveDeviceManager::IsNodeInList(uint64_t nodeId, uint64_t *list, uint32_t listLen)
{
    uint32_t idx;

    for (idx = 0; idx < listLen; idx++)
    {
        if (list[idx] == nodeId)
        {
            return true;
        }
    }

    return false;
}

WEAVE_ERROR WeaveDeviceManager::AddNodeToList(uint64_t nodeId, uint64_t *&list, uint32_t &listLen, uint32_t &listMaxLen, uint32_t initialMaxLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t *tmp;

    // If list is uninitialized, allocate default amount of initial space
    if (!list)
    {
        list = static_cast<uint64_t *>(malloc(sizeof(uint64_t) * initialMaxLen));
        VerifyOrExit(NULL != list, err = WEAVE_ERROR_NO_MEMORY);

        listMaxLen = initialMaxLen;
    }
    // Resize list (double the current space) if necessary
    else if (listLen == listMaxLen)
    {
        VerifyOrExit((static_cast<uint64_t>(listMaxLen) * 2) < UINT32_MAX, err = WEAVE_ERROR_NO_MEMORY);

        tmp = static_cast<uint64_t *>(realloc(list, listMaxLen * 2));
        VerifyOrExit(NULL != tmp, err = WEAVE_ERROR_NO_MEMORY);

        list = tmp;
        listMaxLen *= 2;
    }

    list[listLen] = nodeId;
    listLen++;

exit:
    return err;
}

void WeaveDeviceManager::HandleDeviceEnumerationIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
    uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    IdentifyResponseMessage respMsg;
    bool isMatch;

    VerifyOrExit(kOpState_EnumerateDevices == devMgr->mOpState, err = WEAVE_ERROR_INCORRECT_STATE);

    // If we got an Identify response, check that it matches the requested criteria and ignore it
    // if not.
    VerifyOrExit(profileId == kWeaveProfile_DeviceDescription && msgType == kMessageType_IdentifyResponse, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    // Parse the identify response.
    err = IdentifyResponseMessage::Decode(payload, respMsg);
    SuccessOrExit(err);

    err = FilterIdentifyResponse(respMsg, devMgr->mDeviceCriteria, msgInfo->SourceNodeId, isMatch);
    SuccessOrExit(err);

    // Exit silently if responder doesn't match search critera
    VerifyOrExit(isMatch, err = WEAVE_NO_ERROR);

    // Exit silently if responder ID's already enumerated
    VerifyOrExit(!IsNodeInList(msgInfo->SourceNodeId, devMgr->mEnumeratedNodes, devMgr->mEnumeratedNodesLen), err = WEAVE_NO_ERROR);

    // Mark responder ID as enumerated
    err = AddNodeToList(msgInfo->SourceNodeId, devMgr->mEnumeratedNodes, devMgr->mEnumeratedNodesLen, devMgr->mEnumeratedNodesMaxLen,
                        ENUMERATED_NODES_LIST_INITIAL_SIZE);
    SuccessOrExit(err);

    // Notify the application
    devMgr->mOnComplete.DeviceEnumeration(devMgr, devMgr->mAppReqState, const_cast<const WeaveDeviceDescriptor *>(&respMsg.DeviceDesc), pktInfo->SrcAddress, pktInfo->Interface);

exit:
    if (payload)
    {
        PacketBuffer::Free(payload);
    }

    if (WEAVE_NO_ERROR != err)
    {
        WeaveLogError(DeviceManager, "HandleDeviceEnumerationIdentifyResponse failure: err = %d", err);
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleConnectionIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    IdentifyResponseMessage respMsg;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding operation.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // Verify that we're in the correct connection state.
    VerifyOrExit(devMgr->mConState == kConnectionState_IdentifyDevice, err = WEAVE_ERROR_INCORRECT_STATE);

    // If we got an Identify response, check that it matches the requested criteria and ignore it
    // if not.
    if (profileId == kWeaveProfile_DeviceDescription && msgType == kMessageType_IdentifyResponse)
    {
        // Parse the identify response.
        err = IdentifyResponseMessage::Decode(payload, respMsg);
        SuccessOrExit(err);

        bool isMatch;
        err = FilterIdentifyResponse(respMsg, devMgr->mDeviceCriteria, msgInfo->SourceNodeId, isMatch);
        SuccessOrExit(err);
        if (!isMatch)
            ExitNow();
    }

    // Discard the current exchange context.
    devMgr->mCurReq->Close();
    devMgr->mCurReq = NULL;

    // Cancel the identify timer.
    devMgr->mSystemLayer->CancelTimer(HandleConnectionIdentifyTimeout, devMgr);

    // If we got an Identify response...
    if (profileId == kWeaveProfile_DeviceDescription && msgType == kMessageType_IdentifyResponse)
    {
#if WEAVE_PROGRESS_LOGGING
        {
            char msgSourceStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];
            WeaveMessageSourceToStr(msgSourceStr, sizeof(msgSourceStr), msgInfo);
            WeaveLogProgress(DeviceManager, "Received identify response from device %s", msgSourceStr);
        }
#endif

        // Save the id and address of the device that responded, along with the interface over which
        // the response was received.
        //
        // NOTE that, since this interaction was unsecured, this is only the PURPORTED id
        // of the device.  Once we establish a secure session with the device, we will know for sure
        // that this is the device's id.

        devMgr->mDeviceId = msgInfo->SourceNodeId;
        if (pktInfo != NULL)
        {
            devMgr->mDeviceAddr = pktInfo->SrcAddress;
            devMgr->mDeviceIntf = pktInfo->Interface;
        }
        else
        {
            devMgr->mDeviceAddr = IPAddress::Any;
            devMgr->mDeviceIntf = INET_NULL_INTERFACEID;
        }
        if (devMgr->mDeviceCon != NULL && devMgr->mDeviceCon->PeerNodeId == kNodeIdNotSpecified)
            devMgr->mDeviceCon->PeerNodeId = msgInfo->SourceNodeId;

        // If performing a passive rendezvous or initializing a Weave BLE connection...
        if (devMgr->mOpState == kOpState_PassiveRendezvousDevice ||
            devMgr->mOpState == kOpState_InitializeBleConnection)
        {
            // Initiate a secure session. If this fails, fail the passive rendezvous or BLE connection
            // initialization.
            err = devMgr->StartSession();
            SuccessOrExit(err);
        }

        // Otherwise we're performing a connect or an active rendezvous, so...
        else
        {
            // Initiate a connection to the node id/ip address from which the response was sent.
            err = devMgr->StartConnectDevice(devMgr->mDeviceId, devMgr->mDeviceAddr);
            SuccessOrExit(err);
        }
    }

    // If we got a status report message...
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        // End the connection process.
        devMgr->Close();

        // Decode the supplied status report.
        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        // Call the app's OnError callback.
        devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
    }

    // Fail if we got an unexpected response.
    else
        ExitNow(err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (err != WEAVE_NO_ERROR)
    {
        devMgr->Close();
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
}

WEAVE_ERROR WeaveDeviceManager::FilterIdentifyResponse(IdentifyResponseMessage &respMsg, IdentifyDeviceCriteria criteria, uint64_t sourceNodeId,
    bool& isMatch)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    isMatch = false;

    if (criteria.TargetFabricId != kTargetFabricId_Any)
    {
        if (criteria.TargetFabricId == kTargetFabricId_AnyFabric && respMsg.DeviceDesc.FabricId == 0)
            ExitNow();
        else if (criteria.TargetFabricId == kTargetFabricId_NotInFabric && respMsg.DeviceDesc.FabricId != 0)
            ExitNow();
        else if (criteria.TargetFabricId != respMsg.DeviceDesc.FabricId)
            ExitNow();
    }

    if (criteria.TargetVendorId != 0xFFFF)
    {
        if (criteria.TargetVendorId != respMsg.DeviceDesc.VendorId)
            ExitNow();

        if (criteria.TargetVendorId == kWeaveVendor_NestLabs)
        {
            if (criteria.TargetProductId == kProductWildcardId_NestThermostat)
            {
                if (respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Diamond &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Diamond2 &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Diamond3)
                    ExitNow();
            }
            else if (criteria.TargetProductId == kProductWildcardId_NestProtect)
            {
                if (respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Topaz &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Topaz2)
                    ExitNow();
            }
            else if (criteria.TargetProductId == kProductWildcardId_NestCam)
            {
                if (respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Quartz &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_SmokyQuartz &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_Quartz2 &&
                    respMsg.DeviceDesc.ProductId != kNestWeaveProduct_BlackQuartz)
                    ExitNow();
            }
            else if (criteria.TargetProductId != 0xFFFF)
            {
                if (respMsg.DeviceDesc.ProductId != criteria.TargetProductId)
                    ExitNow();
            }
        }
    }

    if (criteria.TargetDeviceId != kAnyNodeId)
    {
        if (sourceNodeId != criteria.TargetDeviceId)
            ExitNow();
    }

    isMatch = true;

exit:
    return err;
}

void WeaveDeviceManager::HandleDeviceEnumerationTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveDeviceManager* lDevMgr  = reinterpret_cast<WeaveDeviceManager*>(aAppState);
    WEAVE_ERROR         lError = static_cast<WEAVE_ERROR>(aError);

    // Bail immediately if no device enumeration in progress. (This should never be the case.)
    VerifyOrExit(kOpState_EnumerateDevices == lDevMgr->mOpState, lError = WEAVE_ERROR_INCORRECT_STATE);

    // Restart the device enumeration process.
    lError = lDevMgr->InitiateDeviceEnumeration();
    SuccessOrExit(lError);

exit:
    if (lError != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "HandleDeviceEnumerationTimeout failure, err = %d", lError);
    }
}

void WeaveDeviceManager::HandleConnectionIdentifyTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveDeviceManager* lDevMgr  = reinterpret_cast<WeaveDeviceManager*>(aAppState);
    WEAVE_ERROR         lError = static_cast<WEAVE_ERROR>(aError);

    // Bail immediately if not in the right state. (This should never be the case.)
    if (lDevMgr->mConState != kConnectionState_IdentifyDevice)
        return;

    // If we've reached the retry limit, fail the operation with a timeout error.
    if (lDevMgr->mConTimeout != 0 && lDevMgr->mConTryCount * kConRetryInterval >= lDevMgr->mConTimeout)
    {
        WeaveLogProgress(DeviceManager, "Failed to locate device");
        ExitNow(lError = WEAVE_ERROR_DEVICE_LOCATE_TIMEOUT);
    }

    // Otherwise, try again...
    lDevMgr->mConTryCount++;

    // Restart the connection process.
    lError = lDevMgr->InitiateConnection();
    SuccessOrExit(lError);

exit:
    if (lError != WEAVE_NO_ERROR)
    {
        lDevMgr->Close();
        lDevMgr->mOnError(lDevMgr, lDevMgr->mAppReqState, lError, NULL);
    }
}

WEAVE_ERROR WeaveDeviceManager::SetUnsecuredConnectionHandler()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (!mIsUnsecuredConnectionListenerSet)
    {
        err = mMessageLayer->SetUnsecuredConnectionListener(HandleConnectionReceived,
            HandleUnsecuredConnectionCallbackRemoved, true, this);
        SuccessOrExit(err);

        mIsUnsecuredConnectionListenerSet = true;
    }

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::ClearUnsecuredConnectionHandler()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mIsUnsecuredConnectionListenerSet)
    {
        err = mMessageLayer->ClearUnsecuredConnectionListener(HandleConnectionReceived,
            HandleUnsecuredConnectionCallbackRemoved);
        SuccessOrExit(err);

        mIsUnsecuredConnectionListenerSet = false;
    }

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::StartConnectDevice(uint64_t deviceId, IPAddress deviceAddr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InterfaceId targetIntf;

    VerifyOrExit(mDeviceCon == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

#if WEAVE_PROGRESS_LOGGING
    {
        char ipAddrStr[64];
        deviceAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        WeaveLogProgress(DeviceManager, "Initiating weave connection to device %llX (%s)", deviceId, ipAddrStr);
    }
#endif

    mDeviceCon = mMessageLayer->NewConnection();
    VerifyOrExit(mDeviceCon != NULL, err = WEAVE_ERROR_TOO_MANY_CONNECTIONS);

    mDeviceCon->AppState = this;
    mDeviceCon->OnConnectionComplete = HandleConnectionComplete;
    mDeviceCon->OnConnectionClosed = HandleConnectionClosed;

    mConState = kConnectionState_ConnectDevice;

    targetIntf = (mDeviceAddr.IsIPv6LinkLocal()) ? mDeviceIntf : INET_NULL_INTERFACEID;

    err = mDeviceCon->Connect(deviceId, kWeaveAuthMode_Unauthenticated, deviceAddr, WEAVE_PORT, targetIntf);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
        Close();
    return err;
}

void WeaveDeviceManager::HandleConnectionComplete(WeaveConnection *con, WEAVE_ERROR err)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) con->AppState;

    // Bail immediately if not in the correct state.
    if (devMgr->mConState != kConnectionState_ConnectDevice)
    {
        WeaveLogProgress(DeviceManager, "Connection completed in wrong state = %d", devMgr->mConState);
        con->Close();
        return;
    }

    // If the connection succeeded...
    if (err == WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceManager, "Connected to device");

        if (devMgr->mOpState == kOpState_InitializeBleConnection) // If BLE connection...
        {
            //TODO Clean up this kludge:
            devMgr->mConState = kConnectionState_WaitDeviceConnect;

            devMgr->HandleConnectionReceived(devMgr->mMessageLayer, con);
        }
        else // Else, TCP connection...
        {
            // Reset the connection try counter. We will reuse this during session establishment.
            devMgr->mConTryCount = 0;

            // Initiate a secure session.
            err = devMgr->StartSession();
        }
    }
    else
    {
        if (err == WEAVE_ERROR_TIMEOUT )
        {
            err = WEAVE_ERROR_DEVICE_CONNECT_TIMEOUT;
        }
        WeaveLogProgress(DeviceManager, "Failed to connect to device: %s", ErrorStr(err));
    }

    if (err != WEAVE_NO_ERROR)
    {
        devMgr->Close();
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleConnectionReceived(WeaveMessageLayer *msgLayer, WeaveConnection *con)
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    WeaveDeviceManager*     devMgr  = sListeningDeviceMgr;
    PacketBuffer*           msgBuf  = NULL;
    IdentifyRequestMessage  reqMsg;

    if (devMgr != NULL && devMgr->mConState == kConnectionState_WaitDeviceConnect)
    {

#if WEAVE_PROGRESS_LOGGING
        if (devMgr->mOpState == kOpState_PassiveRendezvousDevice)
        {
            char ipAddrStr[64];
            con->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
            WeaveLogProgress(DeviceManager, "Received connection from device (%s)", ipAddrStr);
        }
        else if (devMgr->mOpState == kOpState_InitializeBleConnection)
        {
            WeaveLogProgress(DeviceManager, "Initializing Weave BLE connection");
        }
#endif

        // Let the app know we're starting the
        // authentication/provisioning process.
        if (devMgr->mOnStart)
        {
            devMgr->mOnStart(devMgr, devMgr->mAppReqState, con);
        }

        // Capture the connection object.
        devMgr->mDeviceCon = con;
        devMgr->mDeviceCon->AppState = devMgr;
        devMgr->mDeviceCon->OnConnectionClosed = HandleConnectionClosed;

        // Disallow further incoming connections. Since we can only process one connection at a time
        // we must do this even if the connecting device isn't the one we want to talk to.
        sListeningDeviceMgr = NULL;

        // Remove unsecured incoming connection handler if performing passive rendezvous.
        if (devMgr->mOpState == kOpState_PassiveRendezvousDevice)
        {
            err = devMgr->ClearUnsecuredConnectionHandler();
            SuccessOrExit(err);
        }

        // Encode an Identify device request message. Since we're doing this solely to get the device's
        // node id, we leave all criteria fields blank (i.e. wildcarded).
        msgBuf = PacketBuffer::New();
        VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
        reqMsg.Reset();
        err = reqMsg.Encode(msgBuf);
        SuccessOrExit(err);

        // Construct an exchange context.
        devMgr->mCurReq = devMgr->mExchangeMgr->NewContext(con, devMgr);
        VerifyOrExit(devMgr->mCurReq != NULL, err = WEAVE_ERROR_NO_MEMORY);
        devMgr->mCurReq->OnMessageReceived = HandleConnectionIdentifyResponse;

        // Since we don't know the device's id yet, arrange to send the identify request to the 'Any' node id.
        devMgr->mCurReq->PeerNodeId = kAnyNodeId;

        WeaveLogProgress(DeviceManager, "Sending IdentifyRequest to device");

        devMgr->mConState = kConnectionState_IdentifyDevice;

        // Send the Identify message.
        err = devMgr->mCurReq->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, msgBuf, 0);
        msgBuf = NULL;
        SuccessOrExit(err);
    }
    else
    {
        WeaveLogError(DeviceManager, "Unexpected connection rxd, closing");
        con->Close();
    }

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);
    if (err != WEAVE_NO_ERROR && devMgr != NULL)
    {
        devMgr->Close();
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) con->AppState;

    devMgr->mConState = kConnectionState_NotConnected;

    if (devMgr->mDeviceCon == con)
        devMgr->mDeviceCon = NULL;
    con->Close();

    // Clear connection security info, cancel any timers, and clear OpState.
    devMgr->Close();

    // If we have a callback, invoke it,
    if (devMgr->mOnConnectionClosedFunc)
    {
        devMgr->mOnConnectionClosedFunc(devMgr, devMgr->mOnConnectionClosedAppReq, con, conErr);
    }

    WeaveLogProgress(DeviceManager, "Connection to device closed");
}

WEAVE_ERROR WeaveDeviceManager::StartSession()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Bump the counter every time we attempt to establish a secure session.
    mConTryCount++;

    switch (mAuthType)
    {
    case kAuthType_PASEWithPairingCode:
        WeaveLogProgress(DeviceManager, "Initiating PASE session");
        mConState = kConnectionState_StartSession;
        err = mSecurityMgr->StartPASESession(mDeviceCon, kWeaveAuthMode_PASE_PairingCode, this,
                                             HandleSessionEstablished, HandleSessionError,
                                             (const uint8_t *)mAuthKey, mAuthKeyLen);
        break;

    case kAuthType_CASEWithAccessToken:
        WeaveLogProgress(DeviceManager, "Initiating CASE session");
        mConState = kConnectionState_StartSession;
        // For compatibility with devices that pre-date CASE Config2, propose CASE Config1 in the
        // initial CASE BeginSessionRequest.  Later devices will recognize that the device manager
        // supports Config2 and force a reconfigure.
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
        mSecurityMgr->InitiatorCASEConfig = Profiles::Security::CASE::kCASEConfig_Config1;
#endif
        err = mSecurityMgr->StartCASESession(mDeviceCon, mDeviceCon->PeerNodeId, mDeviceCon->PeerAddr, mDeviceCon->PeerPort,
                                             kWeaveAuthMode_CASE_Device, this,
                                             HandleSessionEstablished, HandleSessionError, this);
        break;

    case kAuthType_None:
        mSessionKeyId = WeaveKeyId::kNone;
        mEncType = kWeaveEncryptionType_None;
        ReenableConnectionMonitor();
        break;

    default:
        err = WEAVE_ERROR_INCORRECT_STATE;
        break;
    }

    return err;
}

void WeaveDeviceManager::HandleSessionEstablished(WeaveSecurityManager *sm, WeaveConnection *con, void *appReqState,
        uint16_t sessionKeyId, uint64_t peerNodeId, uint8_t encType)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *)appReqState;

    // Bail immediately if not in the correct state.
    if (devMgr->mConState != kConnectionState_StartSession || con != devMgr->mDeviceCon)
    {
        WeaveLogError(DeviceManager, "Session established, wrong conState, closing connection");
        con->Close();
        return;
    }

    WeaveLogProgress(DeviceManager, "Secure session established");

    if (devMgr->mOpState == kOpState_RemotePassiveRendezvousAuthenticate)
    {
        WeaveLogProgress(DeviceManager, "Successfully authenticated remote device.");

        // Cancel RemotePassiveRendezvous timer. We have now completed the Remote Passive Rendezvous.
        devMgr->CancelRemotePassiveRendezvousTimer();
    }

    // Save the session key and encryption type for the new session. We will use these later when making requests
    // to the device.
    devMgr->mSessionKeyId = sessionKeyId;
    devMgr->mEncType = encType;

    // Re-enable the connection monitor if needed.
    devMgr->ReenableConnectionMonitor();
}

void WeaveDeviceManager::HandleSessionError(WeaveSecurityManager *sm, WeaveConnection *con, void *appReqState, WEAVE_ERROR localErr, uint64_t peerNodeId, StatusReport *statusReport)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *)appReqState;
    DeviceStatus devStatus;
    DeviceStatus *devStatusArg = NULL;

    // Bail immediately if not in the correct state. May occur if the connection closes abruptly and the
    // SecurityManager's HandleConnectionClosed callback fires _after_ the DeviceManager's own callback.
    // In this case, con is already closed and mOnError has already been called, so we should just exit.
    if (devMgr->mConState != kConnectionState_StartSession)
    {
        return;
    }

    // Report the result.
    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
    {
        WeaveLogProgress(DeviceManager, "Secure session failed: %s", StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode));
    }
    else
    {
        if (localErr == WEAVE_ERROR_TIMEOUT)
        {
            localErr = WEAVE_ERROR_DEVICE_AUTH_TIMEOUT;
        }
        WeaveLogProgress(DeviceManager, "Secure session failed: %s", ErrorStr(localErr));
    }

    // If the device returned a Common:Busy response, it likely means it's in a state where it can't perform
    // the crypto operations necessary to initiate a new session (e.g. because it's busy establishing a secure
    // session with the service).  In this situation, we wait a little bit and retry the operation, but
    // only for a limited number of times.
    if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL &&
        statusReport->mProfileId == kWeaveProfile_Common && statusReport->mStatusCode == Common::kStatus_Busy)
    {
        // If we haven't reached the retry limit yet...
        if (devMgr->mConTryCount < kMaxSessionRetryCount)
        {
            // Arm the retry timer.
            localErr = devMgr->mSystemLayer->StartTimer(kSessionRetryInterval, RetrySession, devMgr);
            if (localErr == WEAVE_NO_ERROR)
            {
                WeaveLogProgress(DeviceManager, "Retrying session establishment after %d ms", kSessionRetryInterval);
                return;
            }
        }
    }

    if (devMgr->mOpState == kOpState_RemotePassiveRendezvousAuthenticate)
    {
        // If we failed to authenticate a remote device as part of Remote Passive Rendezvous, give up on that
        // particular device and listen for the next remote passive rendezvous connection.
        devMgr->RestartRemotePassiveRendezvousListen();
    }
    else
    {
        // Close the connection.
        devMgr->Close();

        // Call the user's error callback.
        if (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
        {
            devStatus.StatusProfileId = statusReport->mProfileId;
            devStatus.StatusCode = statusReport->mStatusCode;
            devStatus.SystemErrorCode = 0;
            devStatusArg = &devStatus;
        }

        devMgr->mOnError(devMgr, devMgr->mAppReqState, localErr, devStatusArg);
    }
}

void WeaveDeviceManager::RetrySession(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveDeviceManager* lDevMgr  = reinterpret_cast<WeaveDeviceManager*>(aAppState);
    WEAVE_ERROR         lError = static_cast<WEAVE_ERROR>(aError);

    // Bail immediately if not in the right state. (This should never be the case.)
    if (lDevMgr->mConState != kConnectionState_StartSession)
        return;

    // Try again to establish a secure session.
    lError = lDevMgr->StartSession();

    if (lError != WEAVE_NO_ERROR)
    {
        lDevMgr->Close();
        lDevMgr->mOnError(lDevMgr, lDevMgr->mAppReqState, lError, NULL);
    }
}

void WeaveDeviceManager::RestartRemotePassiveRendezvousListen()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Close tunneled connection to remote device, if any, and reset rendezvous timer.
    CloseDeviceConnection();

    // Do not attempt reconnect as we timed out during authentication.
    if (mOpState == kOpState_RemotePassiveRendezvousTimedOut)
    {
        WeaveLogProgress(DeviceManager, "RemotePassiveRendezvous timed-out, not restarting");
        err = WEAVE_ERROR_TIMEOUT;
    }
    else
    {
        WeaveLogProgress(DeviceManager, "Restarting Remote Passive Rendezvous");

        // Nobody else is allowed to try anything while we're reconnecting to the assisting device.
        mOpState = kOpState_RestartRemotePassiveRendezvous;

        // Reconnect to assisting device and attempt to reuse existing secure session. Establish new secure session from
        // scratch if necessary.
        err = StartReconnectToAssistingDevice();
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceManager, "RestartRemotePassiveRendezvous failed");

        // Something went wrong, and we couldn't reconnect to the assisting device to continue the Remote Passive
        // Rendezvous. Use big DeviceManager hammer to reset timers and other state.
        Close();

        // Call application's OnError callback.
        mOnError(this, mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::ReenableConnectionMonitor()
{
    WEAVE_ERROR     err     = WEAVE_NO_ERROR;
    PacketBuffer*   msgBuf  = NULL;
    uint8_t*        p;

    if (mConMonitorEnabled)
    {
        mConState = kConnectionState_ReenableConnectionMonitor;

        msgBuf = PacketBuffer::New();
        VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        p = msgBuf->Start();
        LittleEndian::Write16(p, mConMonitorTimeout);
        LittleEndian::Write16(p, mConMonitorInterval);
        msgBuf->SetDataLength(4);

        // Create and initialize an exchange context for the request.
        mCurReq = mExchangeMgr->NewContext(mDeviceId, this);
        VerifyOrExit(mCurReq != NULL, err = WEAVE_ERROR_NO_MEMORY);
        mCurReq->Con = mDeviceCon;
        mCurReq->KeyId = mSessionKeyId;
        mCurReq->EncryptionType = mEncType;
        mCurReq->OnMessageReceived = HandleReenableConnectionMonitorResponse;
        mCurReq->OnConnectionClosed = HandleRequestConnectionClosed;
        // OnResponseTimeout, OnRetransmit

        // TODO: setup request timeout

        // Send the current request over the connection.
        err = mCurReq->SendMessage(kWeaveProfile_DeviceControl, DeviceControl::kMsgType_EnableConnectionMonitor,
                msgBuf, 0);
        msgBuf = NULL;

    }
    else
        HandleConnectionReady();

exit:
    if (msgBuf != NULL)
        PacketBuffer::Free(msgBuf);

    if (err != WEAVE_NO_ERROR)
    {
        Close();
        mOnError(this, mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleReenableConnectionMonitorResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    ec->Close();
    if (ec != devMgr->mCurReq)
        ExitNow();
    devMgr->mCurReq = NULL;

    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
        {
            devMgr->StartConnectionMonitorTimer();

            devMgr->HandleConnectionReady();
        }
        else
        {
            devMgr->Close();
            devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
        }
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);
    if (err != WEAVE_NO_ERROR)
    {
        devMgr->Close();
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleConnectionReady()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mConState = kConnectionState_Connected;

    // Register to receive unsolicited EchoRequest messages from the device.
    err = mExchangeMgr->RegisterUnsolicitedMessageHandler(kWeaveProfile_Echo, kEchoMessageType_EchoRequest,
            mDeviceCon, HandleEchoRequest, this);
    SuccessOrExit(err);

    // If the operation being performed is RendezvousDevice, PassiveRendezvousDevice, ConnectDevice,
    // RemotePassiveRendezvousAuthenticate, InitializeBleConnection, or ReconnectDevice, complete the operation and
    // call the app's callback.
    if (mOpState == kOpState_ConnectDevice || mOpState == kOpState_RendezvousDevice ||
        mOpState == kOpState_PassiveRendezvousDevice || mOpState == kOpState_ReconnectDevice ||
        mOpState == kOpState_RemotePassiveRendezvousAuthenticate || mOpState == kOpState_RemotePassiveRendezvousTimedOut ||
        mOpState == kOpState_InitializeBleConnection)
    {
        ClearOpState();
        mOnComplete.General(this, mAppReqState);
    }

    // Otherwise, if another operation is waiting for the connection to become ready, send the operation's
    // request now.
    else if (mOpState != kOpState_Idle)
    {
        err = SendPendingRequest();
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOpState();
        mOnError(this, mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleIdentifyDeviceResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState opState = devMgr->mOpState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // At this point the operation is effectively complete. Therefore we clear the current operation before
    // continuing. This is important because the user could start another operation during one of the callbacks
    // that happen below.
    devMgr->ClearOpState();

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_DeviceDescription && msgType == DeviceDescription::kMessageType_IdentifyResponse)
    {
        IdentifyResponseMessage respMsg;

        VerifyOrExit(opState == kOpState_IdentifyDevice, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        // Parse the identify response.
        err = IdentifyResponseMessage::Decode(payload, respMsg);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        // Call the user's completion function.
        devMgr->mOnComplete.IdentifyDevice(devMgr, devMgr->mAppReqState, &respMsg.DeviceDesc);
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (err != WEAVE_NO_ERROR)
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandlePairTokenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState opState = devMgr->mOpState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    VerifyOrExit(opState == kOpState_PairToken, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    if (profileId == kWeaveProfile_TokenPairing && msgType == TokenPairing::kMsgType_TokenCertificateResponse)
    {
        VerifyOrExit(devMgr->mTokenPairingCertificate == NULL, err = WEAVE_ERROR_INCORRECT_STATE);
        devMgr->mTokenPairingCertificate = (uint8_t *)malloc(payload->DataLength());
        VerifyOrExit(devMgr->mTokenPairingCertificate != NULL, err = WEAVE_ERROR_NO_MEMORY);
        memcpy(devMgr->mTokenPairingCertificate, payload->Start(), payload->DataLength());
        devMgr->mTokenPairingCertificateLen = payload->DataLength();
        // Do not clear op state yet.
    }
    else if (profileId == kWeaveProfile_TokenPairing && msgType == TokenPairing::kMsgType_TokenPairedResponse)
    {
        devMgr->ClearOpState();

        if (devMgr->mTokenPairingCertificate != NULL)
        {
            // TODO(jay): StitchTogether(payload, mTokenPairingCertificate, mTokenPairingCertificateLen);
            free(devMgr->mTokenPairingCertificate);
            devMgr->mTokenPairingCertificate = NULL;
            devMgr->mTokenPairingCertificateLen = 0;
        }

        devMgr->mOnComplete.PairToken(devMgr, devMgr->mAppReqState, payload->Start(), payload->DataLength());
    }
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        devMgr->ClearOpState();

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
        {
          // Protocol should only send kWeaveProfile_Common status on errors.
          err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
        }
        else
        {
          devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
          devMgr->ClearOpState();
        }
    }
    else
    {
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        devMgr->ClearOpState();
        if (devMgr->mTokenPairingCertificate != NULL)
        {
            free(devMgr->mTokenPairingCertificate);
            devMgr->mTokenPairingCertificate = NULL;
            devMgr->mTokenPairingCertificateLen = 0;
        }

        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandleUnpairTokenResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState opState = devMgr->mOpState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    devMgr->ClearOpState();

    VerifyOrExit(opState == kOpState_UnpairToken, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
            devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
        else
            devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
    }
    else
    {
      err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    }

exit:
    if (err != WEAVE_NO_ERROR)
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandleNetworkProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState opState = devMgr->mOpState;
#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
    uint16_t curReqMsgType = devMgr->mCurReqMsgType;
#endif

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        // At this point the current request is effectively complete but the operation might still continue. Specifically,
        // there is still possibility that older version of the AddNetwork() message should be sent to the device.
        devMgr->ClearRequestState();
    }
    else
#endif
    {
        // At this point the operation is effectively complete. Therefore we clear the current operation before
        // continuing. This is important because the user could start another operation during one of the callbacks
        // that happen below.
        devMgr->ClearOpState();
    }

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_NetworkProvisioning && msgType == NetworkProvisioning::kMsgType_NetworkScanComplete)
    {
        VerifyOrExit(opState == kOpState_ScanNetworks, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        uint16_t resultCount = 0;
        NetworkInfo *netInfoList = NULL;

        err = DecodeNetworkInfoList(payload, resultCount, netInfoList);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        devMgr->mOnComplete.ScanNetworks(devMgr, devMgr->mAppReqState, resultCount, netInfoList);

        delete[] netInfoList;
    }

    else if (profileId == kWeaveProfile_NetworkProvisioning &&
             msgType == NetworkProvisioning::kMsgType_AddNetworkComplete)
    {
        VerifyOrExit(opState == kOpState_AddNetwork, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        uint16_t dataLen = payload->DataLength();
        VerifyOrExit(dataLen == 4, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

        uint32_t networkId = LittleEndian::Get32(payload->Start());

        PacketBuffer::Free(payload);
        payload = NULL;

        devMgr->mOnComplete.AddNetwork(devMgr, devMgr->mAppReqState, networkId);
    }

    else if (profileId == kWeaveProfile_NetworkProvisioning && msgType == NetworkProvisioning::kMsgType_GetNetworksComplete)
    {
        VerifyOrExit(opState == kOpState_GetNetworks, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        uint16_t resultCount = 0;
        NetworkInfo *netInfoList = NULL;

        err = DecodeNetworkInfoList(payload, resultCount, netInfoList);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        devMgr->mOnComplete.GetNetworks(devMgr, devMgr->mAppReqState, resultCount, netInfoList);

        delete[] netInfoList;
    }

    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

#if WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
        // If legacy device doesn't support new version of AddNetwork() message.
        if (curReqMsgType == kMsgType_AddNetworkV2 &&
            devStatus.StatusProfileId == kWeaveProfile_Common &&
            (devStatus.StatusCode == Common::kStatus_UnsupportedMessage ||
             // Additional check is required because in some cases legacy devices return
             // "bad request" status code in response to unsupported message type.
             devStatus.StatusCode == Common::kStatus_BadRequest))
        {
            // Legacy devices don't support standalone Thread network creation.
            VerifyOrExit(!devMgr->mCurReqCreateThreadNetwork, err = WEAVE_ERROR_UNSUPPORTED_THREAD_NETWORK_CREATE);

            // Verify that a copy of the message is retained in a separate buffer.
            VerifyOrExit(devMgr->mCurReqMsgRetained != NULL, err = WEAVE_ERROR_INCORRECT_STATE);

            // Send old version of AddNetwork() message.
            err = devMgr->SendRequest(kWeaveProfile_NetworkProvisioning, kMsgType_AddNetwork, devMgr->mCurReqMsgRetained,
                                      devMgr->HandleNetworkProvisioningResponse);
            devMgr->mCurReqMsgRetained = NULL;
        }
        else
#endif // WEAVE_CONFIG_SUPPORT_LEGACY_ADD_NETWORK_MESSAGE
        {
            devMgr->ClearOpState();

            if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
                devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
            else
                devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
        }
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        devMgr->ClearOpState();
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    }
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandleServiceProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // At this point the operation is effectively complete. Therefore we clear the current operation before
    // continuing. This is important because the user could start another operation during one of the callbacks
    // that happen below.
    devMgr->ClearOpState();

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
            devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
        else
            devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (err != WEAVE_NO_ERROR)
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandleFabricProvisioningResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState savedOpState = devMgr->mOpState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // At this point the operation is effectively complete. Therefore we clear the current operation before
    // continuing. This is important because the user could start another operation during one of the callbacks
    // that happen below.
    devMgr->ClearOpState();

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
            devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
        else
            devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
    }

    else if (profileId == kWeaveProfile_FabricProvisioning && msgType == nl::Weave::Profiles::FabricProvisioning::kMsgType_GetFabricConfigComplete)
    {
        VerifyOrExit(savedOpState == kOpState_GetFabricConfig, err = WEAVE_ERROR_INCORRECT_STATE);
        devMgr->mOnComplete.GetFabricConfig(devMgr, devMgr->mAppReqState, payload->Start(), payload->DataLength());
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (err != WEAVE_NO_ERROR)
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::HandleGetCameraAuthDataResponseEntry(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    devMgr->HandleGetCameraAuthDataResponse(ec, pktInfo, msgInfo, profileId, msgType, payload);
}

void WeaveDeviceManager::Eui48ToString(char *strBuf, uint8_t (&eui)[EUI48_LEN])
{
    // Generate string representation of camera's EUI-48 MAC address
    for (int idx = 0; idx < EUI48_LEN; idx++)
    {
        snprintf(&strBuf[idx * 2], 3, "%02X", eui[idx]);
    }
    strBuf[EUI48_STR_LEN - 1] = '\0';
}

void WeaveDeviceManager::HandleGetCameraAuthDataResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveLogDetail(DeviceManager, "Entering HandleGetCameraAuthDataResponse");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DeviceStatus devStatus;
    DeviceStatus *retDevStatus = NULL;
    OpState prevOpState = mOpState;
    uint8_t macAddress[EUI48_LEN];
    uint8_t hmac[HMAC_BUF_LEN];
    uint8_t authData[CAMERA_AUTH_DATA_LEN];
    char authDataStr[CAMERA_AUTH_DATA_LEN * 2]; // base64-encoded authData
    char macAddressStr[EUI48_STR_LEN];
    uint8_t *cursor;
    uint8_t idx;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_DropcamLegacyPairing && msgType == kMsgType_CameraAuthDataResponse)
    {
        VerifyOrExit(prevOpState == kOpState_GetCameraAuthData, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);

        err = DecodeCameraAuthDataResponse(payload, macAddress, hmac);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        Eui48ToString(macAddressStr, macAddress);

        // Generate auth_data argument string for Dropcam setup.weave_start web API
        cursor = authData;

        memcpy(cursor, static_cast<void *>(macAddress), EUI48_LEN);
        cursor += EUI48_LEN;

        memcpy(cursor, static_cast<void *>(mCameraNonce), CAMERA_NONCE_LEN);
        cursor += CAMERA_NONCE_LEN;

        memcpy(cursor, hmac, CAMERA_HMAC_LEN);

        idx = Base64URLEncode(authData, CAMERA_AUTH_DATA_LEN, authDataStr);
        VerifyOrExit(idx > 0, err = WEAVE_END_OF_INPUT);
        authDataStr[idx] = '\0';

        // At this point the operation is effectively complete. Therefore we clear the current operation before
        // continuing. This is important because the user could start another operation in the callback below.
        ClearOpState();

        mOnComplete.GetCameraAuthData(this, mAppReqState, macAddressStr, authDataStr);
    }
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        err = DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
        retDevStatus = &devStatus;
    }
    else
    {
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    }

exit:
    if (payload != NULL)
    {
        PacketBuffer::Free(payload);
    }

    if (err != WEAVE_NO_ERROR)
    {
        // At this point the operation is effectively complete. Therefore we clear the current operation before
        // continuing. This is important because the user could start another operation in the callback below.
        ClearOpState();

        // Call application's error callback.
        mOnError(this, mAppReqState, err, retDevStatus);
    }
}

void WeaveDeviceManager::HandleDeviceControlResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveLogDetail(DeviceManager, "Entering HandleDeviceControlReponse");

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DeviceStatus devStatus;
    DeviceStatus *retDevStatus = NULL;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState prevOpState = devMgr->mOpState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // If we don't need to keep the exchange open for further Remote Passive Rendezovus messages, then at this point
    // the operation is effectively complete. Therefore we clear the current operation before continuing. This is
    // important because the user could start another operation during one of the callbacks that happen below.
    if (prevOpState != kOpState_RemotePassiveRendezvousRequest)
        devMgr->ClearOpState();

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_Common && devStatus.StatusCode == Common::kStatus_Success)
        {
            // If operation was Remote Passive Rendezvous, don't notify the application of the RPR requests's success
            // via OnComplete, as we still need to actually rendezvous with a remote device. Instead, prepare to
            // receive a RemoteConnectionComplete or error messages, and set mOpState to prevent other DM clients from
            // sending messages over our connection to the assisting device until the RPR succeeds or fails.
            if (prevOpState == kOpState_RemotePassiveRendezvousRequest)
            {
                WeaveLogProgress(DeviceManager, "RemotePassiveRendezvousRequest succeeded");

                // Prepare to receive RemoteConnectionComplete or error message.
                devMgr->mCurReq->OnMessageReceived = HandleRemotePassiveRendezvousComplete;

                // Set mOpState to prevent Device Manager from sending messages to assisting device while
                // waiting for RemoteConnectionComplete message.
                devMgr->mOpState = kOpState_AwaitingRemoteConnectionComplete;
                SuccessOrExit(err);

                WeaveLogProgress(DeviceManager, "Waiting for RemoteConnectionComplete...");
            }
            else
            {
                // If operation was an EnableConnectionMonitor, and positive values were supplied for the interval
                // and timeout, then mark connection monitoring enabled locally and start the connection monitor
                // timer.
                if (prevOpState == kOpState_EnableConnectionMonitor &&
                    devMgr->mConMonitorInterval > 0 && devMgr->mConMonitorTimeout > 0)
                {
                    WeaveLogProgress(DeviceManager, "EnableConnectionMonitor Request succeeded");

                    devMgr->mConMonitorEnabled = true;
                    devMgr->StartConnectionMonitorTimer();
                }

                // Notify application of DeviceControl request's success.
                devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
            }
        }
        else
        {
            err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
            retDevStatus = &devStatus;
        }
    }
    else
    {
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    }

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (err != WEAVE_NO_ERROR)
    {
        if (prevOpState == kOpState_RemotePassiveRendezvousRequest)
        {
            // Must close connection if we performed a Remote Passive Rendezvous request; if it succeeded, this
            // request revoked our ability to send further messages to the assisting device over the current
            // connection.
            devMgr->Close();
        }

        // Call application's error callback.
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, retDevStatus);
    }
}

void WeaveDeviceManager::HandleRemotePassiveRendezvousComplete(ExchangeContext *ec, const IPPacketInfo *pktInfo, const
        WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "Entering HandleRemotePassiveRendezvousComplete");

    DeviceStatus devStatus;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // At this point the operation is effectively complete. Therefore we clear the current operation before
    // continuing. This is important because the user could start another operation during one of the callbacks
    // that happen below.
    devMgr->ClearOpState();

    // Dispatch message.
    if (profileId == kWeaveProfile_DeviceControl && msgType == DeviceControl::kMsgType_RemoteConnectionComplete)
    {
        WeaveLogProgress(DeviceManager, "Received RemoteConnectionComplete");
        devMgr->HandleRemoteConnectionComplete();
    }
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        if (devStatus.StatusProfileId == kWeaveProfile_DeviceControl && devStatus.StatusCode ==
                DeviceControl::kStatusCode_RemotePassiveRendezvousTimedOut)
        {
            WeaveLogProgress(DeviceManager, "RemotePassiveRendezvous timed out on assisting device");
            devMgr->CancelRemotePassiveRendezvous();
            err = WEAVE_ERROR_TIMEOUT;
        }
        else
        {
            WeaveLogProgress(DeviceManager, "Received unexpected status report, profile = %u, code = %u",
                    devStatus.StatusProfileId, devStatus.StatusCode);
            err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
        }
    }
    else
    {
        WeaveLogProgress(DeviceManager, "Received unexpected message type = %u", msgType);
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
    }

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (err != WEAVE_NO_ERROR)
    {
        // Call application's error callback.
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, &devStatus);
    }
}

void WeaveDeviceManager::HandlePingResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    OpState opState = devMgr->mOpState;
    int i;
    uint8_t *data;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding request.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        ExitNow();
    }

    // At this point the operation is effectively complete. Therefore we clear the current operation before
    // continuing. This is important because the user could start another operation during one of the callbacks
    // that happen below.
    devMgr->ClearOpState();

    // Verify that the outstanding operation is a ping.
    VerifyOrExit(opState == kOpState_Ping, err = WEAVE_ERROR_INCORRECT_STATE);

    // Decode and dispatch the response message.
    if (profileId == kWeaveProfile_Echo && msgType == kEchoMessageType_EchoResponse)
    {
        VerifyOrExit(payload->DataLength() == devMgr->mPingSize, err = WEAVE_ERROR_INVALID_ARGUMENT);

        // check test pattern
        data = payload->Start();
        for (i = 0; i < payload->DataLength(); i++) {
            VerifyOrExit(data[i] == (0xff & i), err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        PacketBuffer::Free(payload);
        payload = NULL;

        devMgr->mOnComplete.General(devMgr, devMgr->mAppReqState);
    }
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        DeviceStatus devStatus;

        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        devMgr->mOnError(devMgr, devMgr->mAppReqState, WEAVE_ERROR_STATUS_REPORT_RECEIVED, &devStatus);
    }

    else
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;

exit:
    if (err != WEAVE_NO_ERROR)
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, NULL);
    if (payload != NULL)
        PacketBuffer::Free(payload);
}

void WeaveDeviceManager::StartConnectionMonitorTimer()
{
    if (mConMonitorEnabled && mConMonitorTimeout != 0)
    {
        mSystemLayer->StartTimer(mConMonitorTimeout, HandleConnectionMonitorTimeout, this);
    }
}

void WeaveDeviceManager::CancelConnectionMonitorTimer()
{
    mSystemLayer->CancelTimer(HandleConnectionMonitorTimeout, this);
}

void WeaveDeviceManager::HandleEchoRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo,
        uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *)ec->AppState;

    WeaveLogProgress(DeviceManager, "EchoRequest received from device");

    // Send an Echo Response back to the device.
    ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload);

    // Discard the exchange context.
    ec->Close();

    devMgr->StartConnectionMonitorTimer();
}

void WeaveDeviceManager::HandleConnectionMonitorTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveDeviceManager* lDevMgr  = reinterpret_cast<WeaveDeviceManager*>(aAppState);

    if (lDevMgr->mConMonitorEnabled)
    {
        OpState prevOpState = lDevMgr->mOpState;

        WeaveLogProgress(DeviceManager, "Connection monitor timeout");

        lDevMgr->Close();

        if (prevOpState != kOpState_Idle)
            lDevMgr->mOnError(lDevMgr, lDevMgr->mAppReqState, WEAVE_ERROR_TIMEOUT, NULL);
    }
}

WEAVE_ERROR WeaveDeviceManager::StartRemotePassiveRendezvousTimer()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mSystemLayer->CancelTimer(HandleRemotePassiveRendezvousTimeout, this);

    if (mRemotePassiveRendezvousTimeout > 0)
    {
        // Start timer for mRemotePassiveRendezvousTimeout + 2 seconds. This allows time for the assisting device to
        // send the client an error message on timeout, in which case the client can keep its connection to the
        // assisting device open for further communication.
        err = mSystemLayer->StartTimer(secondsToMilliseconds(mRemotePassiveRendezvousTimeout) +
                secondsToMilliseconds(2),
                HandleRemotePassiveRendezvousTimeout,
                this);
        SuccessOrExit(err);

        mRemotePassiveRendezvousTimerIsRunning = true;
    }

exit:
    return err;
}

void WeaveDeviceManager::CancelRemotePassiveRendezvousTimer()
{
    // Mark timer as no longer running.
    mRemotePassiveRendezvousTimerIsRunning = false;

    mSystemLayer->CancelTimer(HandleRemotePassiveRendezvousTimeout, this);
}

void WeaveDeviceManager::HandleRemotePassiveRendezvousTimeout(System::Layer* aSystemLayer, void* aAppState, System::Error aError)
{
    WeaveDeviceManager* lDevMgr  = reinterpret_cast<WeaveDeviceManager*>(aAppState);

    // Mark timer as no longer running.
    lDevMgr->mRemotePassiveRendezvousTimerIsRunning = false;

    // Close() existing connection to assisting device or remote device, if any, and reset associated state.
    if (lDevMgr->mOpState != kOpState_RemotePassiveRendezvousAuthenticate)
    {
        WeaveLogProgress(DeviceManager, "Remote Passive Rendezvous timed out");

        lDevMgr->Close();
        lDevMgr->mOnError(lDevMgr, lDevMgr->mAppReqState, WEAVE_ERROR_TIMEOUT, NULL);
    }
    else
    {
        lDevMgr->mOpState = kOpState_RemotePassiveRendezvousTimedOut;
    }
}

void WeaveDeviceManager::HandleRemoteConnectionComplete()
{
    WEAVE_ERROR             err     = WEAVE_NO_ERROR;
    IdentifyRequestMessage  reqMsg;
    PacketBuffer*           msgBuf  = NULL;

    // We can't auto-reconnect to a remote device, as it may not even be on our network.
    mAutoReconnect = false;

    // Set OpState. No other actions allowed until we've identified and authenticated the remote device.
    mOpState = kOpState_RemotePassiveRendezvousAuthenticate;

    // Save info required to reconnect to assisting device in case we don't immediately rendezvous with the correct
    // joiner.
    err = SaveAssistingDeviceConnectionInfo();
    SuccessOrExit(err);

    // We are now connected to a remote device via a WeaveConnectionTunnel, so enable this flag to prevent
    // the use of auto-reconnect, as to do so wouldn't make sense in this context.
    mConnectedToRemoteDevice = true;

    // Reset existing session and connection state, as we've effectively connected to a new device.
    ResetConnectionInfo();

    // We must explicitly encode a source node ID in every message sent to the remote host. If we do not, the
    // Weave stack assumes that the recipient of our messages can infer this ID from our ULA. However, in the
    // Remote Passive Rendezvous case, the remote host cannot 'see' our ULA, only the address of the assisting
    // device. Thus we must explictly encode our source node ID so the remote host returns messages to us with
    // the correct destination ID.
    mDeviceCon->SendSourceNodeId = true;

    // Prepare to authenticate with the new device.
    mAuthType = mRemoteDeviceAuthType;
    if (mAuthType != kAuthType_None)
    {
        err = SaveAuthKey(static_cast<const uint8_t *>(mRemoteDeviceAuthKey), mRemoteDeviceAuthKeyLen);
        SuccessOrExit(err);
    }

    // Encode an Identify device request message. Since we're doing this solely to get the device's
    // node id, we leave all criteria fields blank (i.e. wildcarded).
    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    reqMsg.Reset();
    err = reqMsg.Encode(msgBuf);
    SuccessOrExit(err);

    // Construct an exchange context.
    mCurReq = mExchangeMgr->NewContext(mDeviceCon, this);
    VerifyOrExit(mCurReq != NULL, err = WEAVE_ERROR_NO_MEMORY);
    mCurReq->ResponseTimeout = secondsToMilliseconds(5);
    mCurReq->OnMessageReceived = HandleRemoteIdentifyResponse;
    mCurReq->OnConnectionClosed = HandleRemoteIdentifyConnectionClosed;
    mCurReq->OnRetransmissionTimeout = HandleRemoteIdentifyTimeout;
    mCurReq->OnResponseTimeout = HandleRemoteIdentifyTimeout;

    // Since we don't know the device's id yet, arrange to send the identify request to the 'Any' node id.
    mCurReq->PeerNodeId = kAnyNodeId;

    WeaveLogProgress(DeviceManager, "Sending RPR IdentifyRequest to remote device");

    mConState = kConnectionState_IdentifyRemoteDevice;

    // Send the Identify message.
    err = mCurReq->SendMessage(kWeaveProfile_DeviceDescription, kMessageType_IdentifyRequest, msgBuf, 0);
    msgBuf = NULL;
    SuccessOrExit(err);

    WeaveLogProgress(DeviceManager, "Sent IdentifyRequest successfully");

exit:
    if (msgBuf != NULL)
    {
        PacketBuffer::Free(msgBuf);
    }

    if (err == WEAVE_ERROR_WRONG_ENCRYPTION_TYPE)
    {
        // If message had wrong encryption type, i.e. was potentially spoofed, ignore it and continue listening for
        // the authentic Remote Connection Complete.
        WeaveLogError(DeviceManager, "Rxd RemoteConnectionComplete w/ bogus encryption, discarding");
    }
    else if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "Failed send RPR Identify req, err = %d", err);

        // Halt Remote Passive Rendezvous process and close connection to assisting device, as we entered Remote
        // Passive Rendezvous connected state and can no longer send Weave messages to the assisting device on the
        // current connection.
        Close();

        // Call application's OnError callback.
        mOnError(this, mAppReqState, err, NULL);
    }
}

void WeaveDeviceManager::HandleRemoteIdentifyResponse(ExchangeContext *ec, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t profileId, uint8_t msgType, PacketBuffer *payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;
    DeviceStatus devStatus;
    DeviceStatus *devStatusPtr = NULL;

    // Sanity check that the passed-in exchange context is in fact the one that represents the current
    // outstanding operation.
    if (ec != devMgr->mCurReq)
    {
        ec->Close();
        PacketBuffer::Free(payload);
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // Discard the exchange context.
    devMgr->mCurReq->Close();
    devMgr->mCurReq = NULL;

    // Verify that we're in the correct connection state.
    VerifyOrExit(devMgr->mConState == kConnectionState_IdentifyRemoteDevice,
            err = WEAVE_ERROR_INCORRECT_STATE);

    // If we got an Identify response...
    if (profileId == kWeaveProfile_DeviceDescription && msgType == kMessageType_IdentifyResponse)
    {
#if WEAVE_PROGRESS_LOGGING
        {
            char msgSourceStr[WEAVE_MAX_MESSAGE_SOURCE_STR_LENGTH];
            WeaveMessageSourceToStr(msgSourceStr, sizeof(msgSourceStr), msgInfo);
            WeaveLogProgress(DeviceManager, "Received RPR identify response from device %s", msgSourceStr); //TODO get remote IP address from RemoteConnectionComplete msg
        }
#endif

        // Save only the id of the device that responded. Since we've connected to this device
        // via Remote Passive Rendezvous, the device address and interface are not useful to us.
        //
        // NOTE that, since this interaction was unsecured, this is only the PURPORTED id
        // of the device.  Once we establish a secure session with the device, we will know for sure
        // that this is the device's id.
        IdentifyResponseMessage respMsg;
        err = IdentifyResponseMessage::Decode(payload, respMsg);
        SuccessOrExit(err);

        PacketBuffer::Free(payload);
        payload = NULL;

        // Note that usually device ID in DeviceDesc is not set, and receiver is supposed to
        // use the source node id in message header as the purported device ID
        devMgr->mDeviceId = msgInfo->SourceNodeId;

        if (devMgr->mDeviceCon != NULL && devMgr->mDeviceCon->PeerNodeId == kNodeIdNotSpecified)
        {
            WeaveLogProgress(DeviceManager, "Setting mDeviceCon source node ID = %llX", devMgr->mDeviceId);
            devMgr->mDeviceCon->PeerNodeId = devMgr->mDeviceId;
        }

        // Initiate a secure session.
        err = devMgr->StartSession();
        SuccessOrExit(err);
    }
    // If we got a status report...
    else if (profileId == kWeaveProfile_Common && msgType == nl::Weave::Profiles::Common::kMsgType_StatusReport)
    {
        // Decode the supplied status report.
        err = devMgr->DecodeStatusReport(payload, devStatus);
        SuccessOrExit(err);

        devStatusPtr = &devStatus;

        // Disconnect from remote device and listen for the next rendezvous connection if we got a status report.
        devMgr->RestartRemotePassiveRendezvousListen();
    }
    // If we got something unexpected...
    else
    {
        // Disconnect from remote device and listen for the next rendezvous connection if we got an unexpected
        // profile or message type.
        devMgr->RestartRemotePassiveRendezvousListen();
    }

exit:
    if (payload != NULL)
        PacketBuffer::Free(payload);

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "Failed handle RPR Id rx, err = %d", err);

        // Halt Remote Passive Rendezvous process and close connection to assisting device, as we entered Remote
        // Passive Rendezvous connected state and can no longer send Weave messages to the assisting device on the
        // current connection.
        devMgr->Close();

        // Call application's OnError callback.
        devMgr->mOnError(devMgr, devMgr->mAppReqState, err, devStatusPtr);
    }
}

void WeaveDeviceManager::HandleRemoteIdentifyConnectionClosed(ExchangeContext *ec, WeaveConnection *con,
    WEAVE_ERROR conErr)
{
    WeaveLogError(DeviceManager, "RPR connection closed during remote Id");
    WeaveDeviceManager *devMgr = (WeaveDeviceManager *) ec->AppState;

    if (con == devMgr->mDeviceCon)
        devMgr->mDeviceCon = NULL;

    // Continue with RPR regardless of conErr, as there may be other devices with which to rendezvous.
    devMgr->RestartRemotePassiveRendezvousListen();
}

void WeaveDeviceManager::HandleRemoteIdentifyTimeout(ExchangeContext *ec)
{
    WeaveLogError(DeviceManager, "RPR Id timed out");
    WeaveDeviceManager *devMgr = static_cast<WeaveDeviceManager *>(ec->AppState);

    // Continue with RPR, as there may be other devices with which to rendezvous.
    devMgr->RestartRemotePassiveRendezvousListen();
}

WEAVE_ERROR WeaveDeviceManager::DecodeStatusReport(PacketBuffer *msgBuf, DeviceStatus& status)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    uint8_t *p = msgBuf->Start();
    uint16_t dataLen = msgBuf->DataLength();
    TLVType containingType;

    static const uint64_t SystemErrorCodeTag = ProfileTag(kWeaveProfile_Common, Common::kTag_SystemErrorCode);

    VerifyOrExit(dataLen >= 6, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    status.StatusProfileId = LittleEndian::Read32(p);
    status.StatusCode = LittleEndian::Read16(p);

    if (dataLen > 6)
    {
        msgBuf->SetStart(p);

        reader.Init(msgBuf);

        err = reader.Next();
        SuccessOrExit(err);

        VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

        err = reader.EnterContainer(containingType);
        SuccessOrExit(err);

        while ((err = reader.Next()) == WEAVE_NO_ERROR)
        {
            if (reader.GetTag() == SystemErrorCodeTag)
            {
                VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
                err = reader.Get(status.SystemErrorCode);
                SuccessOrExit(err);
            }
        }

        if (err != WEAVE_END_OF_TLV)
            ExitNow();

        err = reader.ExitContainer(containingType);
        SuccessOrExit(err);

        err = reader.Next();
        if (err != WEAVE_END_OF_TLV)
            ExitNow(err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        err = WEAVE_NO_ERROR;
    }
    else
        status.SystemErrorCode = WEAVE_NO_ERROR;

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceManager::DecodeNetworkInfoList(PacketBuffer *msgBuf, uint16_t& count,
        NetworkInfo *& netInfoList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    uint8_t *p = msgBuf->Start();
    uint16_t dataLen = msgBuf->DataLength();

    netInfoList = NULL;

    VerifyOrExit(dataLen >= 2, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    count = Read8(p);

    msgBuf->SetStart(p);
    reader.Init(msgBuf);
    reader.ImplicitProfileId = kWeaveProfile_NetworkProvisioning;

    err = reader.Next();
    SuccessOrExit(err);

    err = NetworkInfo::DecodeList(reader, count, netInfoList);
    SuccessOrExit(err);

    err = reader.Next();
    if (err != WEAVE_END_OF_TLV)
    {
        if (err == WEAVE_NO_ERROR)
            err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT;
        ExitNow();
    }

    return WEAVE_NO_ERROR;

exit:
    if (err != WEAVE_NO_ERROR)
        delete[] netInfoList;
    return err;
}


#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

WEAVE_ERROR WeaveDeviceManager::EncodeNodeCertInfo(const Security::CASE::BeginSessionContext & msgCtx,
        TLVWriter & writer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;

    // Initialize a reader to read the access token.
    reader.Init((const uint8_t *)mAuthKey, mAuthKeyLen);
    reader.ImplicitProfileId = kWeaveProfile_Security;

    // Generate a CASE CertificateInformation structure from the information in the access token.
    err = CASECertInfoFromAccessToken(reader, writer);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
        err = WEAVE_ERROR_INVALID_ACCESS_TOKEN;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::GenerateNodeSignature(const Security::CASE::BeginSessionContext & msgCtx,
        const uint8_t * msgHash, uint8_t msgHashLen,
        TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t * privKey = NULL;
    uint16_t privKeyLen;

    // Get the private key from the access token.
    err = GetNodePrivateKey(msgCtx.IsInitiator(), privKey, privKeyLen);
    SuccessOrExit(err);

    // Generate a signature using the access token private key.
    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, msgHash, msgHashLen, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    if (privKey != NULL)
    {
        WEAVE_ERROR relErr = ReleaseNodePrivateKey(privKey);
        err = (err == WEAVE_NO_ERROR) ? relErr : err;
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::EncodeNodePayload(const Security::CASE::BeginSessionContext & msgCtx,
        uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen)
{
    // No payload
    payloadLen = 0;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceManager::BeginValidation(const Security::CASE::BeginSessionContext & msgCtx,
        Security::ValidationContext & validCtx, Security::WeaveCertificateSet & certSet)
{
    return BeginCertValidation(msgCtx.IsInitiator(), certSet, validCtx);
}

WEAVE_ERROR WeaveDeviceManager::HandleValidationResult(const Security::CASE::BeginSessionContext & msgCtx,
        Security::ValidationContext & validCtx,
        Security::WeaveCertificateSet & certSet, WEAVE_ERROR & validRes)
{
    return HandleCertValidationResult(msgCtx.IsInitiator(), validRes, validCtx.SigningCert, msgCtx.PeerNodeId, certSet, validCtx);
}

void WeaveDeviceManager::EndValidation(const Security::CASE::BeginSessionContext & msgCtx,
        Security::ValidationContext & validCtx, Security::WeaveCertificateSet & certSet)
{
    EndCertValidation(certSet, validCtx);
}

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

WEAVE_ERROR WeaveDeviceManager::GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen)
{
    WEAVE_ERROR err;

    // Decode the supplied access token and generate a CASE Certificate Info TLV structure
    // containing the certificate(s) from the access token.
    err = CASECertInfoFromAccessToken((const uint8_t *)mAuthKey, mAuthKeyLen, buf, bufSize, certInfoLen);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
        err = WEAVE_ERROR_INVALID_ACCESS_TOKEN;
    return err;
}

// Get payload information, if any, to be included in the message to the peer.
WEAVE_ERROR WeaveDeviceManager::GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen)
{
    // No payload
    payloadLen = 0;
    return WEAVE_NO_ERROR;
}

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

// Get the local node's private key.
WEAVE_ERROR WeaveDeviceManager::GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
{
    WEAVE_ERROR err;
    uint8_t *privKeyBuf = NULL;

    // Allocate a buffer to hold the private key.  Since the key is held within the access token, a
    // buffer as big as the access token will always be sufficient.
    privKeyBuf = (uint8_t *)malloc(mAuthKeyLen);
    VerifyOrExit(privKeyBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Extract the private key from the access token, converting the encoding to a EllipticCurvePrivateKey TLV object.
    err = ExtractPrivateKeyFromAccessToken((const uint8_t *)mAuthKey, mAuthKeyLen, privKeyBuf, mAuthKeyLen, weavePrivKeyLen);
    SuccessOrExit(err);

    // Pass the extracted key back to the caller.
    weavePrivKey = privKeyBuf;
    privKeyBuf = NULL;

exit:
    if (err != WEAVE_NO_ERROR)
        err = WEAVE_ERROR_INVALID_ACCESS_TOKEN;
    if (privKeyBuf != NULL)
        free(privKeyBuf);
    return err;
}

// Called when the CASE engine is done with the buffer returned by GetNodePrivateKey().
WEAVE_ERROR WeaveDeviceManager::ReleaseNodePrivateKey(const uint8_t *weavePrivKey)
{
    free((void *)weavePrivKey);
    return WEAVE_NO_ERROR;
}

// Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
// This method is responsible for loading the trust anchors into the certificate set.
WEAVE_ERROR WeaveDeviceManager::BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    err = certSet.Init(kMaxCASECerts, kCertDecodeBufferSize);
    SuccessOrExit(err);

    err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
    SuccessOrExit(err);
    cert->CertFlags |= kCertFlag_IsTrusted;

    err = certSet.LoadCert(nl::NestCerts::Production::Root::Cert, nl::NestCerts::Production::Root::CertLength, 0, cert);
    SuccessOrExit(err);
    cert->CertFlags |= kCertFlag_IsTrusted;

    err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
    SuccessOrExit(err);

    err = certSet.LoadCert(nl::NestCerts::Production::DeviceCA::Cert, nl::NestCerts::Production::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
    SuccessOrExit(err);

    memset(&validContext, 0, sizeof(validContext));
    validContext.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.RequiredKeyPurposes = (isInitiator) ? kKeyPurposeFlag_ServerAuth : kKeyPurposeFlag_ClientAuth;

exit:
    return err;
}

// Called with the results of validating the peer's certificate.
WEAVE_ERROR WeaveDeviceManager::HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes,
        WeaveCertificateData *peerCert, uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    // If the device's certificate is otherwise valid, make sure its subject DN matches the expected device id.
    if (validRes == WEAVE_NO_ERROR)
    {
        // Verify that the device authenticated with a device certificate. If so...
        if (peerCert->CertType == kCertType_Device)
        {
            // Get the node id from the certificate subject.
            uint64_t certDeviceId = peerCert->SubjectDN.AttrValue.WeaveId;

            // This is a work-around for Nest DVT devices that were built with incorrect certificates.
            // Specifically, the device id in the certificate didn't include Nest's OUI (the first
            // 3 bytes of the EUI-64 that makes up the id). Here we grandfather these in by assuming
            // anything that has an OUI of 0 is in fact a Nest device.
            if ((certDeviceId & 0xFFFFFF0000000000ULL) == 0)
                certDeviceId |= 0x18B4300000000000ULL;

            // Verify the target device id against the device id in the certificate.
            if (mDeviceId != kAnyNodeId && certDeviceId != mDeviceId)
                validRes = WEAVE_ERROR_WRONG_CERT_SUBJECT;
        }

        // Otherwise reject the session.
        else
        {
            validRes = WEAVE_ERROR_WRONG_CERT_TYPE;
        }
    }

    return WEAVE_NO_ERROR;
}

// Called when peer certificate validation is complete.
WEAVE_ERROR WeaveDeviceManager::EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    // Nothing to do
    return WEAVE_NO_ERROR;
}

bool IsProductWildcard(uint16_t productId)
{
    return (productId >= kProductWildcardId_RangeStart && productId <= kProductWildcardId_RangeEnd);
}

WeaveDeviceManager::WDMDMClient::WDMDMClient(void)
{
    mDeviceMgr = NULL;
}

WeaveDeviceManager::WDMDMClient::~WDMDMClient(void)
{
    mDeviceMgr = NULL;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::InitClient(WeaveDeviceManager *aDeviceMgr, WeaveExchangeManager *aExchangeMgr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = DMClient::Init(aExchangeMgr);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s DMClient::Init() failed: %s", __PRETTY_FUNCTION__, ErrorStr(err));
    }
    else
    {
        mDeviceMgr = aDeviceMgr;
    }

    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::ViewConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "%s - non-success status", __PRETTY_FUNCTION__);

    if (mDeviceMgr)
    {
        mDeviceMgr->ClearOpState();
        mDeviceMgr->mOnComplete.General(mDeviceMgr, mDeviceMgr->mAppReqState);
    }

    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::ViewConfirm(const uint64_t &aResponderId, ReferencedTLVData &aDataList, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TLVReader dataListRdr;
    TLVReader pathRdr;
    TLVReader containerRdr;
    TLVType pathContainer;
    TLVType profileContainer;
    uint64_t version;
    uint32_t profileId;
    uint64_t tagViewed;
    uint32_t bufSize;
    char *buf = NULL;
    uint16_t index;
    uint16_t localeNum = 0;
    char **localeList = NULL;

    WeaveLogProgress(DeviceManager, "%s - success status", __PRETTY_FUNCTION__);
    if (mDeviceMgr)
    {
        mDeviceMgr->ClearOpState();
    }

    OpenDataList(aDataList, dataListRdr);
    SuccessOrExit(err);

    err = dataListRdr.Next();
    SuccessOrExit(err);

    err = OpenDataListElement(dataListRdr, pathRdr, version);
    SuccessOrExit(err);

    VerifyOrExit(pathRdr.GetType() == kTLVType_Path, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = ValidateWDMTag(kTag_WDMDataListElementPath, pathRdr);
    SuccessOrExit(err);

    err = pathRdr.EnterContainer(pathContainer);
    SuccessOrExit(err);

    err = pathRdr.Next();
    SuccessOrExit(err);

    VerifyOrExit(pathRdr.GetType() == kTLVType_Structure, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = ValidateWDMTag(kTag_WDMPathProfile, pathRdr);
    SuccessOrExit(err);

    err = pathRdr.EnterContainer(profileContainer);
    SuccessOrExit(err);

    err = pathRdr.Next();
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfileId, pathRdr);
    SuccessOrExit(err);

    err = pathRdr.Get(profileId);
    SuccessOrExit(err);

    err = pathRdr.ExitContainer(profileContainer);
    SuccessOrExit(err);

    err = pathRdr.Next();
    SuccessOrExit(err);

    tagViewed = pathRdr.GetTag();
    switch (profileId)
    {
        case kWeaveProfile_NestThermostat:
            WeaveLogProgress(DeviceManager, "View Nest Thermostat");
            if (tagViewed == ProfileTag(kWeaveProfile_NestThermostat, Vendor::Nestlabs::Thermostat::kTag_LegacyEntryKey))
            {
                bufSize = dataListRdr.GetLength() + 1;
                buf = (char *)malloc(bufSize);
                err = dataListRdr.GetString(buf, bufSize);
                SuccessOrExit(err);
                WeaveLogProgress(DeviceManager, "entry key = %s", buf);
                if (mDeviceMgr)
                {
                    mDeviceMgr->mOnComplete.ThermostatGetEntryKey(mDeviceMgr, mDeviceMgr->mAppReqState, buf);
                }
            }
            else if (tagViewed == ProfileTag(kWeaveProfile_NestThermostat, Vendor::Nestlabs::Thermostat::kTag_SystemTestStatusKey))
            {
                uint64_t status = UINT64_MAX;
                err = dataListRdr.Get(status);
                SuccessOrExit(err);
                WeaveLogProgress(DeviceManager, "system test status = %llu", status);
                if (mDeviceMgr)
                {
                    mDeviceMgr->mOnComplete.ThermostatSystemStatus(mDeviceMgr, mDeviceMgr->mAppReqState, status);
                }
            }
            else
            {
                WeaveLogError(DeviceManager, "Unsupported nest thermostat tag: %llu", tagViewed);
                err = WEAVE_ERROR_INCORRECT_STATE;
            }
            break;

        case kWeaveProfile_Locale:
            WeaveLogProgress(DeviceManager, "View Locale");
            if (tagViewed == ProfileTag(kWeaveProfile_Locale, Locale::kTag_ActiveLocale))
            {
                bufSize = dataListRdr.GetLength() + 1;
                buf = (char *)malloc(bufSize);
                err = dataListRdr.GetString(buf, bufSize);
                SuccessOrExit(err);
                WeaveLogProgress(DeviceManager, "active locale = %s", buf);
                if (mDeviceMgr)
                {
                    mDeviceMgr->mOnComplete.GetActiveLocale(mDeviceMgr, mDeviceMgr->mAppReqState, buf);
                }
            }
            else if (tagViewed == ProfileTag(kWeaveProfile_Locale, Locale::kTag_AvailableLocales))
            {
                err = dataListRdr.OpenContainer(containerRdr);
                SuccessOrExit(err);

                while (containerRdr.Next() == WEAVE_NO_ERROR)
                {
                    localeNum++;
                }
                WeaveLogProgress(DeviceManager, "#available locales = %d", localeNum);

                err = dataListRdr.OpenContainer(containerRdr);
                SuccessOrExit(err);

                localeList = (char **)malloc(sizeof(char *) * localeNum);
                for (index = 0; containerRdr.Next() == WEAVE_NO_ERROR; index++)
                {
                    localeList[index] = (char *)malloc(128);
                    err = containerRdr.GetString(localeList[index], 128);
                    SuccessOrExit(err);
                    WeaveLogProgress(DeviceManager, "\t%s", localeList[index]);
                }

                err = dataListRdr.CloseContainer(containerRdr);
                SuccessOrExit(err);

                if (mDeviceMgr)
                {
                    mDeviceMgr->mOnComplete.GetAvailableLocales(mDeviceMgr, mDeviceMgr->mAppReqState, localeNum, (const char **)localeList);
                }
            }
            else
            {
                WeaveLogError(DeviceManager, "Unsupported nest thermostat tag: %llu", tagViewed);
                err = WEAVE_ERROR_INCORRECT_STATE;
            }
            break;

        default:
            WeaveLogError(DeviceManager, "Unknown profileId: %llu", profileId);
            err = WEAVE_ERROR_INCORRECT_STATE;
            break;
    }

exit:
    if (buf != NULL)
    {
        free(buf);
    }
    if (localeList != NULL && localeNum > 0)
    {
        for (index = 0; index < localeNum; index++)
        {
            free(localeList[index]);
        }
    }
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceManager, "%s failed: %s", __FUNCTION__, ErrorStr(err));
        if (mDeviceMgr)
        {
            mDeviceMgr->mOnError(mDeviceMgr, mDeviceMgr->mAppReqState, err, NULL);
        }
    }
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::UpdateConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "%s", __PRETTY_FUNCTION__);

    if (mDeviceMgr)
    {
        mDeviceMgr->ClearOpState();
        mDeviceMgr->mOnComplete.General(mDeviceMgr, mDeviceMgr->mAppReqState);
    }

    return err;
}

void WeaveDeviceManager::WDMDMClient::IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport)
{

/*
 * this was added as a result of the fix to WEAV-142 and related
 * jiras. the code that belongs here is whatever the application wants
 * to do in case of a binding failure. at this point, the main (really
 * only) reason a binding will fail is the unexpected closure of a TCP
 * connection that supports it. in future, other failure scenarios may
 * arise.
 */

}

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::SubscribeConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::UnsubscribeIndication(const uint64_t &aPublisherId, const TopicIdentifier &aTopicId, StatusReport &aReport)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::CancelSubscriptionConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

WEAVE_ERROR WeaveDeviceManager::WDMDMClient::NotifyIndication(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    return err;
}

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_SUBSCRIPTION

} // namespace DeviceManager
} // namespace Weave
} // namespace nl

/*
 *
 *    Copyright (c) 2019-2020 Google LLC
 *    Copyright (c) 2013-2018 Nest Labs, Inc.
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
 *      Implementation of the native methods expected by the Java
 *      version of Weave Device Manager.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string>
#include <jni.h>

#include <SystemLayer/SystemLayer.h>
#include <InetLayer/InetLayer.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Support/platform/PersistedStorage.h>
#include <Weave/DeviceManager/WeaveDeviceManager.h>
#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#include <Weave/DeviceManager/WeaveDataManagementClient.h>
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#include "AndroidBleApplicationDelegate.h"
#include "AndroidBlePlatformDelegate.h"

#include <net/if.h>
#include <errno.h>

#ifndef PTHREAD_NULL
#define PTHREAD_NULL 0
#endif

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::DeviceManager;
using namespace nl::Weave::Profiles::NetworkProvisioning;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Profiles::DataManagement;

using nl::Weave::Profiles::DeviceDescription::IdentifyDeviceCriteria;
using nl::Weave::System::PacketBuffer;

#define WDM_JNI_ERROR_MIN     10000
#define WDM_JNI_ERROR_MAX     10999

#define _WDM_JNI_ERROR(e) (WDM_JNI_ERROR_MIN + (e))

#define WDM_JNI_ERROR_EXCEPTION_THROWN          _WDM_JNI_ERROR(0)
#define WDM_JNI_ERROR_TYPE_NOT_FOUND            _WDM_JNI_ERROR(1)
#define WDM_JNI_ERROR_METHOD_NOT_FOUND          _WDM_JNI_ERROR(2)
#define WDM_JNI_ERROR_FIELD_NOT_FOUND           _WDM_JNI_ERROR(3)

#define WDM_JNI_CALLBACK_LOCAL_REF_COUNT 256

static JavaVM *sJVM;
static System::Layer sSystemLayer;
static InetLayer sInet;
#if CONFIG_NETWORK_LAYER_BLE
static BleLayer sBle;
static AndroidBleApplicationDelegate sBleApplicationDelegate;
static AndroidBlePlatformDelegate sBlePlatformDelegate(&sBle);
#endif
static WeaveFabricState sFabricState;
static WeaveMessageLayer sMessageLayer;
static WeaveExchangeManager sExchangeMgr;
static WeaveSecurityManager sSecurityMgr;
static pthread_mutex_t sStackLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t sIOThread = PTHREAD_NULL;
static bool sShutdown = false;
static jclass sNetworkInfoCls = NULL;
static jclass sWeaveDeviceExceptionCls = NULL;
static jclass sWeaveDeviceManagerExceptionCls = NULL;
static jclass sWeaveDeviceDescriptorCls = NULL;
static jclass sWirelessRegulatoryConfigCls = NULL;
static jclass sWeaveDeviceManagerCls = NULL;
static jclass sWeaveStackCls = NULL;
static jclass sWdmClientFlushUpdateDeviceExceptionCls = NULL;
static jclass sWdmClientFlushUpdateExceptionCls = NULL;

extern "C" {
    NL_DLL_EXPORT jint JNI_OnLoad(JavaVM *jvm, void *reserved);
    NL_DLL_EXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved);
    NL_DLL_EXPORT jlong Java_nl_Weave_DeviceManager_WeaveDeviceManager_newDeviceManager(JNIEnv *env, jobject self);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_deleteDeviceManager(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBleNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBlePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose, jstring pairingCodeObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBleAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose, jbyteArray accessToken);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDeviceNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDevicePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj, jstring pairingCodeObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDeviceAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj, jbyteArray accessToken);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDeviceNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject deviceCriteria);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDevicePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring pairingCodeObj, jobject deviceCriteria);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDeviceAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray accessToken, jobject deviceCriteria);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring rendezvousAddrObj, jint rendezvousTimeoutSec, jint inactivityTimeoutSec);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousPairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring pairingCodeObj, jstring rendezvousAddrObj, jint rendezvousTimeoutSec, jint inactivityTimeoutSec);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray accessToken, jstring rendezvousAddrObj, jint rendezvousTimeoutSec, jint inactivityTimeoutSec);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginReconnectDevice(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginIdentifyDevice(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginScanNetworks(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint networkType);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetNetworks(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint flags);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetCameraAuthData(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring nonce);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginAddNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject networkInfoObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUpdateNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject networkInfoObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemoveNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginEnableNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisableNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginTestNetworkConnectivity(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetRendezvousMode(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginSetRendezvousMode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint rendezvousFlags);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetLastNetworkProvisioningResult(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPing(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPingWithSize(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint payloadSize);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRegisterServicePairAccount(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong serviceId, jstring accountId, jbyteArray serviceConfig, jbyteArray pairingToken, jbyteArray pairingInitData);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUnregisterService(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong serviceId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setRendezvousAddress(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring rendezvousAddrObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setAutoReconnect(JNIEnv *env, jobject self, jlong deviceMgrPtr, jboolean autoReconnect);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setRendezvousLinkLocal(JNIEnv *env, jobject self, jlong deviceMgrPtr, jboolean rendezvousLinkLocal);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setConnectTimeout(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint timeoutMS);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginCreateFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginLeaveFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetFabricConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginJoinExistingFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray fabricConfig);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginArmFailSafe(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint armMode, jint failSafeToken);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisarmFailSafe(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginStartSystemTest(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong profileId, jlong testId);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginStopSystemTest(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginResetConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint resetFlags);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginEnableConnectionMonitor(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint interval, jint timeout);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisableConnectionMonitor(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPairToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray pairingToken);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUnpairToken(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetWirelessRegulatoryConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginSetWirelessRegulatoryConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject regConfigObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_close(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT jboolean Java_nl_Weave_DeviceManager_WeaveDeviceManager_isConnected(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT jlong Java_nl_Weave_DeviceManager_WeaveDeviceManager_deviceId(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT jstring Java_nl_Weave_DeviceManager_WeaveDeviceManager_deviceAddress(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT jboolean Java_nl_Weave_DeviceManager_WeaveDeviceManager_isValidPairingCode(JNIEnv *env, jclass cls, jstring pairingCodeObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_closeEndpoints(JNIEnv *env, jclass cls);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setLogFilter(JNIEnv *env, jclass cls, jint logLevel);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_startDeviceEnumeration(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject deviceCriteriaObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveDeviceManager_stopDeviceEnumeration(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveStack_handleWriteConfirmation(JNIEnv *env, jobject self, jint connObj, jbyteArray svcIdObj, jbyteArray charIdObj, jboolean success);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveStack_handleIndicationReceived(JNIEnv *env, jobject self, jint connObj, jbyteArray svcIdObj, jbyteArray charIdObj, jbyteArray dataObj);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveStack_handleSubscribeComplete(JNIEnv *env, jobject self, jint connObj, jbyteArray svcIdObj, jbyteArray charIdObj, jboolean success);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveStack_handleUnsubscribeComplete(JNIEnv *env, jobject self, jint connObj, jbyteArray svcIdObj, jbyteArray charIdObj, jboolean success);
    NL_DLL_EXPORT void Java_nl_Weave_DeviceManager_WeaveStack_handleConnectionError(JNIEnv *env, jobject self, jint connObj);
    NL_DLL_EXPORT jobject Java_nl_Weave_DeviceManager_WeaveDeviceDescriptor_decode(JNIEnv *env, jclass cls, jbyteArray encodedDesc);
#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_WdmClientImpl_init(JNIEnv *env, jobject self);
    NL_DLL_EXPORT jlong Java_nl_Weave_DataManagement_WdmClientImpl_newWdmClient(JNIEnv *env, jobject self, jlong deviceMgrPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_WdmClientImpl_deleteWdmClient(JNIEnv *env, jobject self, jlong wdmClientPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_WdmClientImpl_setNodeId(JNIEnv *env, jobject self, jlong wdmClientPtr, jlong nodeId);
    NL_DLL_EXPORT jlong Java_nl_Weave_DataManagement_WdmClientImpl_newDataSink(JNIEnv *env, jobject self, jlong wdmClientPtr, jobject resourceIdentifierObj, jlong profileId, jlong instanceId, jstring path);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_WdmClientImpl_beginFlushUpdate(JNIEnv *env, jobject self, jlong wdmClientPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_WdmClientImpl_beginRefreshData(JNIEnv *env, jobject self, jlong wdmClientPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_init(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_shutdown(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_clear(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_beginRefreshData(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setInt(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jlong value, jboolean isConditional, jboolean isSigned);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setDouble(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jdouble value, jboolean isConditional);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setBoolean(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jboolean value, jboolean isConditional);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setString(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jstring value, jboolean isConditional);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setNull(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jboolean isConditional);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setBytes(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jbyteArray value, jboolean isConditional);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setStringArray(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jobjectArray stringArray, jboolean isConditional);
    NL_DLL_EXPORT jlong Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getLong(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jdouble Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getDouble(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jboolean Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getBoolean(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jstring Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getString(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jboolean Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_isNull(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jbyteArray Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getBytes(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jobjectArray Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getStringArray(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
    NL_DLL_EXPORT jlong Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getVersion(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr);
    NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_deleteData(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
};

static void HandleIdentifyDeviceComplete(WeaveDeviceManager *deviceMgr, void *reqState, const WeaveDeviceDescriptor *deviceDesc);
static void HandleNetworkScanComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t netCount, const NetworkInfo *netInfoList);
static void HandleGetNetworksComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t netCount, const NetworkInfo *netInfoList);
static void HandleGetCameraAuthDataComplete(WeaveDeviceManager *deviceMgr, void *reqState, const char *macAddress, const char *signedPayload);
static void HandleAddNetworkComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint32_t networkId);
static void HandleGetRendezvousModeComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t modeFlags);
static void HandleGetFabricConfigComplete(WeaveDeviceManager *deviceMgr, void *reqState, const uint8_t *fabricConfig, uint32_t fabricConfigLen);
static void HandleDeviceEnumerationResponse(WeaveDeviceManager *deviceMgr, void *reqState, const DeviceDescription::WeaveDeviceDescriptor *devdesc, IPAddress deviceAddr, InterfaceId deviceIntf);
static void HandleGenericOperationComplete(jobject obj, void *reqState);
static void HandleSimpleOperationComplete(WeaveDeviceManager *deviceMgr, void *reqState);
static void HandleNotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj);
static bool HandleSendCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId, const uint8_t *characteristicData, uint32_t characteristicDataLen);
static bool HandleSubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId);
static bool HandleUnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId);
static void HandlePairTokenComplete(WeaveDeviceManager *deviceMgr, void *reqState, const uint8_t *pairingTokenBundle, uint32_t pairingTokenBundleLen);
static void HandleUnpairTokenComplete(WeaveDeviceManager *deviceMgr, void *reqState);
static void HandleGetWirelessRegulatoryConfigComplete(WeaveDeviceManager *deviceMgr, void *reqState, const WirelessRegConfig * regConfig);
static bool HandleCloseConnection(BLE_CONNECTION_OBJECT connObj);
static uint16_t HandleGetMTU(BLE_CONNECTION_OBJECT connObj);
static void HandleGenericError(jobject obj, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus);
static void HandleError(WeaveDeviceManager *deviceMgr, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus);
static void ThrowError(JNIEnv *env, WEAVE_ERROR errToThrow);
static void ReportError(JNIEnv *env, WEAVE_ERROR cbErr, const char *cbName);
static void *IOThreadMain(void *arg);
static WEAVE_ERROR J2N_ByteArray(JNIEnv *env, jbyteArray inArray, uint8_t *& outArray, uint32_t& outArrayLen);
static WEAVE_ERROR J2N_ByteArrayInPlace(JNIEnv *env, jbyteArray inArray, uint8_t * outArray, uint32_t maxArrayLen);
static WEAVE_ERROR N2J_ByteArray(JNIEnv *env, const uint8_t *inArray, uint32_t inArrayLen, jbyteArray& outArray);
static WEAVE_ERROR N2J_NewStringUTF(JNIEnv *env, const char *inStr, jstring& outString);
static WEAVE_ERROR N2J_NewStringUTF(JNIEnv *env, const char *inStr, size_t inStrLen, jstring& outString);
static WEAVE_ERROR J2N_EnumFieldVal(JNIEnv *env, jobject obj, const char *fieldName, const char *fieldType, int& outVal);
static WEAVE_ERROR J2N_ShortFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jshort& outVal);
static WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal);
static WEAVE_ERROR J2N_LongFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jlong& outVal);
static WEAVE_ERROR J2N_StringFieldVal(JNIEnv *env, jobject obj, const char *fieldName, char *& outVal);
static WEAVE_ERROR J2N_ByteArrayFieldVal(JNIEnv *env, jobject obj, const char *fieldName, uint8_t *& outArray, uint32_t& outArrayLen);
static WEAVE_ERROR J2N_IdentifyDeviceCriteria(JNIEnv *env, jobject inCriteria, IdentifyDeviceCriteria& outCriteria);
static WEAVE_ERROR J2N_NetworkInfo(JNIEnv *env, jobject inNetworkInfo, NetworkInfo& outNetworkInfo);
static WEAVE_ERROR N2J_NetworkInfo(JNIEnv *env, const NetworkInfo& inNetworkInfo, jobject& outNetworkInfo);
static WEAVE_ERROR N2J_NetworkInfoArray(JNIEnv *env, const NetworkInfo *inArray, uint32_t inArrayLen, jobjectArray& outArray);
static WEAVE_ERROR N2J_DeviceDescriptor(JNIEnv *env, const WeaveDeviceDescriptor& inDeviceDesc, jobject& outDeviceDesc);
static WEAVE_ERROR J2N_WirelessRegulatoryConfig(JNIEnv *env, jobject inRegConfig, WirelessRegConfig & outRegConfig);
static WEAVE_ERROR N2J_WirelessRegulatoryConfig(JNIEnv *env, const WirelessRegConfig& inRegConfig, jobject& outRegConfig);
static WEAVE_ERROR N2J_Error(JNIEnv *env, WEAVE_ERROR inErr, jthrowable& outEx);
static WEAVE_ERROR J2N_ResourceIdentifier(JNIEnv *env, jobject inResourceIdentifier, ResourceIdentifier& outResourceIdentifier);
static WEAVE_ERROR GetClassRef(JNIEnv *env, const char *clsType, jclass& outCls);
static WEAVE_ERROR J2N_StdString(JNIEnv *env, jstring inString,  std::string& outString);
template<typename GetElementFunct>
WEAVE_ERROR N2J_GenericArray(JNIEnv *env, uint32_t arrayLen, jclass arrayElemCls, jobjectArray& outArray, GetElementFunct getElem);

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
static void HandleWdmClientComplete(void *context, void *reqState);
static void HandleGenericTraitUpdatableDataSinkComplete(void *context, void *reqState, DeviceStatus *deviceStatus);
static void HandleWdmClientError(void *appState, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus);
static void HandleGenericTraitUpdatableDataSinkError(void *context, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus);
static void HandleWdmClientFlushUpdateComplete(void *context, void *reqState, uint16_t pathCount, WdmClientFlushUpdateStatus *statusResults);
static WEAVE_ERROR N2J_DeviceStatus(JNIEnv *env, DeviceStatus& devStatus, jthrowable& outEx);
static WEAVE_ERROR N2J_WdmClientFlushUpdateError(JNIEnv *env, WEAVE_ERROR inErr, jobject& outEx, const char * path, TraitDataSink * dataSink);
static WEAVE_ERROR N2J_WdmClientFlushUpdateDeviceStatus(JNIEnv *env, DeviceStatus& devStatus, jobject& outEx, const char * path, TraitDataSink * dataSink);
static void EngineEventCallback(void * const aAppState,
                                SubscriptionEngine::EventID aEvent,
                                const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam);
static void BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
                                  const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam);
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL

#if CURRENTLY_UNUSED
static WEAVE_ERROR J2N_EnumVal(JNIEnv *env, jobject enumObj, int& outVal);
static WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal);
#endif

jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveMessageLayer::InitContext initContext;
    JNIEnv *env;
    int pthreadErr = 0;
    pthread_mutexattr_t stackLockAttrs;

    WeaveLogProgress(DeviceManager, "JNI_OnLoad() called");

    // Save a reference to the JVM.  The IO thread will need this to call back into Java.
    sJVM = jvm;

    // Get a JNI environment object.
    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    WeaveLogProgress(DeviceManager, "Loading Java class references.");

    // Get various class references need by the API.
    err = GetClassRef(env, "nl/Weave/DeviceManager/NetworkInfo", sNetworkInfoCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WeaveDeviceException", sWeaveDeviceExceptionCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WeaveDeviceManagerException", sWeaveDeviceManagerExceptionCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WeaveDeviceDescriptor", sWeaveDeviceDescriptorCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WirelessRegulatoryConfig", sWirelessRegulatoryConfigCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WeaveDeviceManager", sWeaveDeviceManagerCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DeviceManager/WeaveStack", sWeaveStackCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DataManagement/WdmClientFlushUpdateDeviceException", sWdmClientFlushUpdateDeviceExceptionCls);
    SuccessOrExit(err);
    err = GetClassRef(env, "nl/Weave/DataManagement/WdmClientFlushUpdateException", sWdmClientFlushUpdateExceptionCls);
    SuccessOrExit(err);
    WeaveLogProgress(DeviceManager, "Java class references loaded.");

    // Initialize the lock that will be used to protect the stack.  Note that this needs to allow recursive acquisition.
    pthread_mutexattr_init(&stackLockAttrs);
    pthread_mutexattr_settype(&stackLockAttrs, PTHREAD_MUTEX_RECURSIVE);
    pthreadErr = pthread_mutex_init(&sStackLock, &stackLockAttrs);
    VerifyOrExit(pthreadErr == 0, err = System::MapErrorPOSIX(pthreadErr));

    // Initialize the underlying platform secure random source.
    err = nl::Weave::Platform::Security::InitSecureRandomDataSource(NULL, 64, NULL, 0);
    SuccessOrExit(err);

    // Initialize the Weave System Layer object
    err = sSystemLayer.Init(NULL);
    SuccessOrExit(err);

    // Initialize the InetLayer object.
    err = sInet.Init(sSystemLayer, NULL);
    SuccessOrExit(err);

#if CONFIG_NETWORK_LAYER_BLE
    // Initialize the ApplicationDelegate
    sBleApplicationDelegate.SetNotifyWeaveConnectionClosedCallback(HandleNotifyWeaveConnectionClosed);
    // Initialize the PlatformDelegate
    sBlePlatformDelegate.SetSendWriteRequestCallback(HandleSendCharacteristic);
    sBlePlatformDelegate.SetSubscribeCharacteristicCallback(HandleSubscribeCharacteristic);
    sBlePlatformDelegate.SetUnsubscribeCharacteristicCallback(HandleUnsubscribeCharacteristic);
    sBlePlatformDelegate.SetCloseConnectionCallback(HandleCloseConnection);
    sBlePlatformDelegate.SetGetMTUCallback(HandleGetMTU);
    // Initialize the BleLayer object.
    err = sBle.Init(&sBlePlatformDelegate, &sBleApplicationDelegate, &sSystemLayer);
    SuccessOrExit(err);
#endif
    // Initialize the FabricState object.
    err = sFabricState.Init();
    SuccessOrExit(err);

    // Not a member of a fabric.
    sFabricState.FabricId = 0;

    // Generate a unique node id for local Weave stack.
    err = GenerateWeaveNodeId(sFabricState.LocalNodeId);
    SuccessOrExit(err);

    // Initialize the WeaveMessageLayer object.
    initContext.systemLayer = &sSystemLayer;
    initContext.inet = &sInet;
    initContext.fabricState = &sFabricState;
    initContext.listenTCP = false;
#if WEAVE_CONFIG_DEVICE_MGR_DEMAND_ENABLE_UDP
    initContext.listenUDP = false;
#else
    initContext.listenUDP = true;
#endif
#if CONFIG_NETWORK_LAYER_BLE
    initContext.ble = &sBle;
    initContext.listenBLE = true;
#endif
#if WEAVE_CONFIG_ENABLE_EPHEMERAL_UDP_PORT
    initContext.enableEphemeralUDPPort = true;
#endif
    err = sMessageLayer.Init(&initContext);
    SuccessOrExit(err);

    // Initialize the Exchange Manager object.
    err = sExchangeMgr.Init(&sMessageLayer);
    SuccessOrExit(err);

    // Initialize the Security Manager object.
    err = sSecurityMgr.Init(sExchangeMgr, sSystemLayer);
    SuccessOrExit(err);

    // Create and start the IO thread.
    sShutdown = false;
    pthreadErr = pthread_create(&sIOThread, NULL, IOThreadMain, NULL);
    VerifyOrExit(pthreadErr == 0, err = System::MapErrorPOSIX(pthreadErr));

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ThrowError(env, err);
        JNI_OnUnload(jvm, reserved);
    }

    return (err == WEAVE_NO_ERROR) ? JNI_VERSION_1_2 : JNI_ERR;
}

void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    JNIEnv *env;

    WeaveLogProgress(DeviceManager, "JNI_OnUnload() called");

    // Get a JNI environment object.
    jvm->GetEnv((void **)&env, JNI_VERSION_1_2);

    // If the IO thread has been started, tell it to shutdown and wait for it to exit.
    if (sIOThread != PTHREAD_NULL)
    {
        sShutdown = true;
        sSystemLayer.WakeSelect();
        pthread_join(sIOThread, NULL);
    }

    sSecurityMgr.Shutdown();
    sExchangeMgr.Shutdown();
    sMessageLayer.Shutdown();
    sFabricState.Shutdown();
#if CONFIG_NETWORK_LAYER_BLE
    sBle.Shutdown();
#endif
    sInet.Shutdown();
    sSystemLayer.Shutdown();

    pthread_mutex_destroy(&sStackLock);

    sJVM = NULL;
}

jlong Java_nl_Weave_DeviceManager_WeaveDeviceManager_newDeviceManager(JNIEnv *env, jobject self)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = NULL;
    long res = 0;

    WeaveLogProgress(DeviceManager, "newDeviceManager() called");

    deviceMgr = new WeaveDeviceManager();
    VerifyOrExit(deviceMgr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = deviceMgr->Init(&sExchangeMgr, &sSecurityMgr);
    SuccessOrExit(err);

    deviceMgr->AppState = (void *)env->NewGlobalRef(self);

    res = (long)deviceMgr;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (deviceMgr != NULL)
        {
            if (deviceMgr->AppState != NULL)
                env->DeleteGlobalRef((jobject)deviceMgr->AppState);
            deviceMgr->Shutdown();
            delete deviceMgr;
        }

        if (err != WDM_JNI_ERROR_EXCEPTION_THROWN)
            ThrowError(env, err);
    }

    return res;
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_deleteDeviceManager(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "deleteDeviceManager() called");

    if (deviceMgr != NULL)
    {
        if (deviceMgr->AppState != NULL)
            env->DeleteGlobalRef((jobject)deviceMgr->AppState);

        deviceMgr->Shutdown();

        delete deviceMgr;
    }
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBleNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "connectBle() called");


    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectBle(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), (void *)"ConnectBle", HandleSimpleOperationComplete,
                HandleError, (bool)autoClose);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBlePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose, jstring pairingCodeObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const char *pairingCodeStr = NULL;

    WeaveLogProgress(DeviceManager, "connectBle() called with pairing code");

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectBle(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), pairingCodeStr, (void *)"ConnectBle", HandleSimpleOperationComplete, HandleError, (bool)autoClose);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (pairingCodeStr != NULL)
        env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectBleAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint connObj, jboolean autoClose, jbyteArray accessTokenObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    uint8_t *accessToken = NULL;
    uint32_t accessTokenLen;

    WeaveLogProgress(DeviceManager, "connectBle() called with access token");

    err = J2N_ByteArray(env, accessTokenObj, accessToken, accessTokenLen);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectBle(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), accessToken, accessTokenLen, (void *)"ConnectBle", HandleSimpleOperationComplete, HandleError, (bool)autoClose);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (accessToken != NULL)
        free(accessToken);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDeviceNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IPAddress deviceAddr;

    WeaveLogProgress(DeviceManager, "beginConnectDevice() called with no auth");

    if (deviceAddrObj != NULL)
    {
        const char *deviceAddrStr = env->GetStringUTFChars(deviceAddrObj, 0);
        bool res = IPAddress::FromString(deviceAddrStr, deviceAddr);
        env->ReleaseStringUTFChars(deviceAddrObj, deviceAddrStr);

        if (!res)
            ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);
    }
    else
        deviceAddr = IPAddress::Any;

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectDevice((uint64_t)deviceId, deviceAddr, (void *)"ConnectDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDevicePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj, jstring pairingCodeObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IPAddress deviceAddr;
    const char *pairingCodeStr = NULL;

    WeaveLogProgress(DeviceManager, "beginConnectDevice() called with pairing code");

    if (deviceAddrObj != NULL)
    {
        const char *deviceAddrStr = env->GetStringUTFChars(deviceAddrObj, 0);
        bool res = IPAddress::FromString(deviceAddrStr, deviceAddr);
        env->ReleaseStringUTFChars(deviceAddrObj, deviceAddrStr);

        if (!res)
            ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);
    }
    else
        deviceAddr = IPAddress::Any;

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectDevice((uint64_t)deviceId, deviceAddr, pairingCodeStr, (void *)"ConnectDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (pairingCodeStr != NULL)
        env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginConnectDeviceAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong deviceId, jstring deviceAddrObj, jbyteArray accessTokenObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IPAddress deviceAddr;
    uint8_t *accessToken = NULL;
    uint32_t accessTokenLen;

    WeaveLogProgress(DeviceManager, "beginConnectDevice() called with access token");

    if (deviceAddrObj != NULL)
    {
        const char *deviceAddrStr = env->GetStringUTFChars(deviceAddrObj, 0);
        bool res = IPAddress::FromString(deviceAddrStr, deviceAddr);
        env->ReleaseStringUTFChars(deviceAddrObj, deviceAddrStr);

        if (!res)
            ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);
    }
    else
        deviceAddr = IPAddress::Any;

    err = J2N_ByteArray(env, accessTokenObj, accessToken, accessTokenLen);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ConnectDevice((uint64_t)deviceId, deviceAddr, accessToken, accessTokenLen, (void *)"ConnectDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (accessToken != NULL)
        free(accessToken);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDeviceNoAuth(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject deviceCriteriaObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IdentifyDeviceCriteria deviceCriteria;

    WeaveLogProgress(DeviceManager, "beginRendezvousDevice() called with no auth");

    err = J2N_IdentifyDeviceCriteria(env, deviceCriteriaObj, deviceCriteria);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RendezvousDevice(deviceCriteria, (void *)"RendezvousDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDevicePairingCode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring pairingCodeObj, jobject deviceCriteriaObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IdentifyDeviceCriteria deviceCriteria;
    const char *pairingCodeStr = NULL;

    WeaveLogProgress(DeviceManager, "beginRendezvousDevice() called with pairing code");

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    err = J2N_IdentifyDeviceCriteria(env, deviceCriteriaObj, deviceCriteria);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RendezvousDevice(pairingCodeStr, deviceCriteria, (void *)"RendezvousDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (pairingCodeStr != NULL)
        env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRendezvousDeviceAccessToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray accessTokenObj, jobject deviceCriteriaObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IdentifyDeviceCriteria deviceCriteria;
    uint8_t *accessToken = NULL;
    uint32_t accessTokenLen;

    WeaveLogProgress(DeviceManager, "beginRendezvousDevice() called with access token");

    err = J2N_ByteArray(env, accessTokenObj, accessToken, accessTokenLen);
    SuccessOrExit(err);

    err = J2N_IdentifyDeviceCriteria(env, deviceCriteriaObj, deviceCriteria);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RendezvousDevice(accessToken, accessTokenLen, deviceCriteria, (void *)"RendezvousDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (accessToken != NULL)
        free(accessToken);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousNoAuth(
        JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring rendezvousAddrObj,
        jint rendezvousTimeoutSec, jint inactivityTimeoutSec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const char *rendezvousAddrStr = NULL;
    IPAddress rendezvousAddr;

    WeaveLogProgress(DeviceManager, "beginRemotePassiveRendezvous() called with no auth");

    rendezvousAddrStr = env->GetStringUTFChars(rendezvousAddrObj, 0);
    VerifyOrExit(rendezvousAddrStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!IPAddress::FromString(rendezvousAddrStr, rendezvousAddr))
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RemotePassiveRendezvous(rendezvousAddr, rendezvousTimeoutSec,
                                             inactivityTimeoutSec,
                                             (void *)"RemotePassiveRendezvous",
                                             HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
    if (rendezvousAddrStr != NULL)
        env->ReleaseStringUTFChars(rendezvousAddrObj, rendezvousAddrStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousPairingCode(
        JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring pairingCodeObj,
        jstring rendezvousAddrObj, jint rendezvousTimeoutSec, jint inactivityTimeoutSec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const char *pairingCodeStr = NULL;
    const char *rendezvousAddrStr = NULL;
    IPAddress rendezvousAddr;

    WeaveLogProgress(DeviceManager, "beginRemotePassiveRendezvous() called with pairing code");

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);
    VerifyOrExit(pairingCodeStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    rendezvousAddrStr = env->GetStringUTFChars(rendezvousAddrObj, 0);
    VerifyOrExit(rendezvousAddrStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!IPAddress::FromString(rendezvousAddrStr, rendezvousAddr))
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RemotePassiveRendezvous(rendezvousAddr, pairingCodeStr, rendezvousTimeoutSec,
                                             inactivityTimeoutSec, (void *)"RemotePassiveRendezvous",
                                             HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (pairingCodeStr != NULL)
        env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    if (rendezvousAddrStr != NULL)
        env->ReleaseStringUTFChars(rendezvousAddrObj, rendezvousAddrStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemotePassiveRendezvousAccessToken(
        JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray accessTokenObj,
        jstring rendezvousAddrObj, jint rendezvousTimeoutSec, jint inactivityTimeoutSec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    uint8_t *accessToken = NULL;
    uint32_t accessTokenLen;
    const char *rendezvousAddrStr = NULL;
    IPAddress rendezvousAddr;

    WeaveLogProgress(DeviceManager, "beginRemotePassiveRendezvous() called with access token");

    err = J2N_ByteArray(env, accessTokenObj, accessToken, accessTokenLen);
    SuccessOrExit(err);

    rendezvousAddrStr = env->GetStringUTFChars(rendezvousAddrObj, 0);
    VerifyOrExit(rendezvousAddrStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!IPAddress::FromString(rendezvousAddrStr, rendezvousAddr))
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RemotePassiveRendezvous(rendezvousAddr, accessToken, accessTokenLen,
                                             rendezvousTimeoutSec, inactivityTimeoutSec,
                                             (void *)"RemotePassiveRendezvous",
                                             HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    if (accessToken != NULL)
        free(accessToken);

    if (rendezvousAddrStr != NULL)
        env->ReleaseStringUTFChars(rendezvousAddrObj, rendezvousAddrStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginReconnectDevice(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginReconnectDevice() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ReconnectDevice((void *)"ReconnectDevice", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginIdentifyDevice(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginIdentifyDevice() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->IdentifyDevice((void *)"IdentifyDevice", HandleIdentifyDeviceComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginScanNetworks(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint networkType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginScanNetworks() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ScanNetworks((NetworkType)networkType, (void *)"ScanNetworks", HandleNetworkScanComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginAddNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject networkInfoObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    NetworkInfo networkInfo;

    WeaveLogProgress(DeviceManager, "beginAddNetwork() called");

    err = J2N_NetworkInfo(env, networkInfoObj, networkInfo);
    WeaveLogProgress(DeviceManager, "beginAddNetwork() J2N_NetworkInfo returned %s", nl::ErrorStr(err));
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->AddNetwork(&networkInfo, (void *)"AddNetwork", HandleAddNetworkComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUpdateNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject networkInfoObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    NetworkInfo networkInfo;

    WeaveLogProgress(DeviceManager, "beginUpdateNetwork() called");

    err = J2N_NetworkInfo(env, networkInfoObj, networkInfo);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->UpdateNetwork(&networkInfo, (void *)"UpdateNetwork", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRemoveNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginRemoveNetwork() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RemoveNetwork((uint32_t)networkId, (void *)"RemoveNetwork", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetNetworks(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint getFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginGetNetworks() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetNetworks(getFlags, (void *)"GetNetworks", HandleGetNetworksComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetCameraAuthData(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring nonce)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const char *nonceStr = NULL;

    WeaveLogProgress(DeviceManager, "beginGetCameraAuthData() called");

    nonceStr = env->GetStringUTFChars(nonce, 0);
    VerifyOrExit(nonceStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetCameraAuthData(nonceStr, (void *)"GetCameraAuthData", HandleGetCameraAuthDataComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (nonceStr != NULL)
        env->ReleaseStringUTFChars(nonce, nonceStr);
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginEnableNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginEnableNetwork() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->EnableNetwork((uint32_t)networkId, (void *)"EnableNetwork", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisableNetwork(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginDisableNetwork() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->DisableNetwork((uint32_t)networkId, (void *)"DisableNetwork", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginTestNetworkConnectivity(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginTestNetworkConnectivity() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->TestNetworkConnectivity((uint32_t)networkId, (void *)"TestNetworkConnectivity", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetRendezvousMode(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginGetRendezvousMode() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetRendezvousMode((void *)"GetRendezvousMode", HandleGetRendezvousModeComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginSetRendezvousMode(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint rendezvousFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginSetRendezvousMode() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetRendezvousMode((uint16_t)rendezvousFlags, (void *)"SetRendezvousMode", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginRegisterServicePairAccount(JNIEnv *env, jobject self, jlong deviceMgrPtr,
                                                                                    jlong serviceId, jstring accountId, jbyteArray serviceConfig,
                                                                                    jbyteArray pairingToken, jbyteArray pairingInitData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const uint8_t *serviceConfigBuf;
    const uint8_t *pairingTokenBuf;
    const uint8_t *pairingInitDataBuf;
    const char *accountIdStr = NULL;

    WeaveLogProgress(DeviceManager, "beginRegisterServicePairAccount() called");

    uint32_t serviceConfigLen;
    uint32_t pairingTokenLen;
    uint32_t pairingInitDataLen;

    serviceConfigLen = env->GetArrayLength(serviceConfig);
    serviceConfigBuf = (const uint8_t *)env->GetByteArrayElements(serviceConfig, NULL);

    pairingTokenLen = env->GetArrayLength(pairingToken);
    pairingTokenBuf = (const uint8_t *)env->GetByteArrayElements(pairingToken, NULL);

    pairingInitDataLen = env->GetArrayLength(pairingInitData);
    pairingInitDataBuf = (const uint8_t *)env->GetByteArrayElements(pairingInitData, NULL);

    accountIdStr = env->GetStringUTFChars(accountId, 0);
    VerifyOrExit(accountIdStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->RegisterServicePairAccount((uint64_t) serviceId, accountIdStr,
                                                serviceConfigBuf, serviceConfigLen,
                                                pairingTokenBuf, pairingTokenLen,
                                                pairingInitDataBuf, pairingInitDataLen,
                                                (void *)"RegisterServicePairAccount", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (accountIdStr != NULL)
        env->ReleaseStringUTFChars(accountId, accountIdStr);
    if (serviceConfigBuf != NULL)
        env->ReleaseByteArrayElements(serviceConfig, (jbyte *)serviceConfigBuf, JNI_ABORT);
    if (pairingTokenBuf != NULL)
        env->ReleaseByteArrayElements(pairingToken, (jbyte *)pairingTokenBuf, JNI_ABORT);
    if (pairingInitDataBuf != NULL)
        env->ReleaseByteArrayElements(pairingInitData, (jbyte *)pairingInitDataBuf, JNI_ABORT);
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUnregisterService(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong serviceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginUnregisterService() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->UnregisterService((uint64_t) serviceId, (void *)"UnregisterService", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetLastNetworkProvisioningResult(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginGetLastNetworkProvisioningResult() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetLastNetworkProvisioningResult((void *)"GetLastNetworkProvisioningResult", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPing(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPingWithSize(env, self, deviceMgrPtr, 0);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPingWithSize(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint payloadSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginPingWithSize() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->Ping((void *)"Ping", (int32_t)payloadSize, HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setRendezvousAddress(JNIEnv *env, jobject self, jlong deviceMgrPtr, jstring rendezvousAddrObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const char *rendezvousAddrStr = NULL;
    IPAddress rendezvousAddr;

    WeaveLogProgress(DeviceManager, "setRendezvousAddress() called");

    rendezvousAddrStr = env->GetStringUTFChars(rendezvousAddrObj, 0);
    VerifyOrExit(rendezvousAddrStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!IPAddress::FromString(rendezvousAddrStr, rendezvousAddr))
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetWiFiRendezvousAddress(rendezvousAddr);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
    if (rendezvousAddrStr != NULL)
        env->ReleaseStringUTFChars(rendezvousAddrObj, rendezvousAddrStr);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setAutoReconnect(JNIEnv *env, jobject self, jlong deviceMgrPtr, jboolean autoReconnect)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "setAutoReconnect() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetAutoReconnect(autoReconnect != JNI_FALSE);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setRendezvousLinkLocal(JNIEnv *env, jobject self, jlong deviceMgrPtr, jboolean rendezvousLinkLocal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "setRendezvousLinkLocal() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetRendezvousLinkLocal(rendezvousLinkLocal != JNI_FALSE);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setConnectTimeout(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint timeoutMS)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "setConnectTimeout() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetConnectTimeout((uint32_t)timeoutMS);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginCreateFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginCreateFabric() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->CreateFabric((void *)"CreateFabric", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginLeaveFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginLeaveFabric() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->LeaveFabric((void *)"LeaveFabric", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetFabricConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginGetFabricConfig() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetFabricConfig((void *)"GetFabricConfig", HandleGetFabricConfigComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginJoinExistingFabric(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray fabricConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const uint8_t *fabricConfigBuf;
    uint32_t fabricConfigLen;

    WeaveLogProgress(DeviceManager, "beginJoinExistingFabric() called");

    fabricConfigLen = env->GetArrayLength(fabricConfig);
    fabricConfigBuf = (const uint8_t *)env->GetByteArrayElements(fabricConfig, NULL);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->JoinExistingFabric(fabricConfigBuf, fabricConfigLen, (void *)"JoinExistingFabric", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (fabricConfigBuf != NULL)
        env->ReleaseByteArrayElements(fabricConfig, (jbyte *)fabricConfigBuf, JNI_ABORT);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginArmFailSafe(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint armMode, jint failSafeToken)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginArmFailSafe() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ArmFailSafe((uint8_t) armMode, (uint32_t)failSafeToken, (void *)"ArmFailSafe", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisarmFailSafe(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginDisarmFailSafe() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->DisarmFailSafe((void *)"DisarmFailSafe", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginStartSystemTest(JNIEnv *env, jobject self, jlong deviceMgrPtr, jlong profileId, jlong testId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginStartSystemTest() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->StartSystemTest((void *)"StartSystemTest", static_cast<uint32_t>(profileId), static_cast<uint32_t>(testId), HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginStopSystemTest(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginStopSystemTest() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->StopSystemTest((void *)"StopSystemTest", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginResetConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint resetFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginResetConfig() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->ResetConfig((uint16_t)resetFlags, (void *)"ResetConfig", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginEnableConnectionMonitor(JNIEnv *env, jobject self, jlong deviceMgrPtr, jint interval, jint timeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginEnableConnectionMonitor() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->EnableConnectionMonitor((uint16_t)interval, (uint16_t)timeout, (void *)"EnableConnectionMonitor", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginDisableConnectionMonitor(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginDisableConnectionMonitor() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->DisableConnectionMonitor((void *)"DisableConnectionMonitor", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

jboolean Java_nl_Weave_DeviceManager_WeaveDeviceManager_isValidPairingCode(JNIEnv *env, jclass cls, jstring pairingCodeObj)
{
    const char *pairingCodeStr = NULL;
    bool res;

    WeaveLogProgress(DeviceManager, "isValidPairingCode() called");

    pairingCodeStr = env->GetStringUTFChars(pairingCodeObj, 0);

    res = WeaveDeviceManager::IsValidPairingCode(pairingCodeStr);

    env->ReleaseStringUTFChars(pairingCodeObj, pairingCodeStr);

    return res ? JNI_TRUE : JNI_FALSE;
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginPairToken(JNIEnv *env, jobject self, jlong deviceMgrPtr, jbyteArray pairingToken)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    const uint8_t *pairingTokenBuf;
    uint32_t pairingTokenLen;

    WeaveLogProgress(DeviceManager, "beginPairToken() called");

    pairingTokenLen = env->GetArrayLength(pairingToken);
    pairingTokenBuf = (const uint8_t *)env->GetByteArrayElements(pairingToken, NULL);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->PairToken(pairingTokenBuf, static_cast<uint16_t>(pairingTokenLen), (void *)"PairToken", HandlePairTokenComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (pairingTokenBuf != NULL)
        env->ReleaseByteArrayElements(pairingToken, (jbyte *)pairingTokenBuf, JNI_ABORT);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginUnpairToken(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginUnpairToken() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->UnpairToken((void *)"UnpairToken", HandleUnpairTokenComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginGetWirelessRegulatoryConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "beginGetWirelessRegulatoryConfig() called");

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->GetWirelessRegulatoryConfig((void *)"GetWirelessRegulatoryConfig", HandleGetWirelessRegulatoryConfigComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_beginSetWirelessRegulatoryConfig(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject regConfigObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    WirelessRegConfig regConfig;

    WeaveLogProgress(DeviceManager, "beginSetWirelessRegulatoryConfig() called");

    regConfig.Init();

    err = J2N_WirelessRegulatoryConfig(env, regConfigObj, regConfig);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->SetWirelessRegulatoryConfig(&regConfig, (void *)"SetWirelessRegulatoryConfig", HandleSimpleOperationComplete, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_closeEndpoints(JNIEnv *env, jclass cls)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "closeEndpoints() called");

    pthread_mutex_lock(&sStackLock);

    err = sMessageLayer.CloseEndpoints();

    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_setLogFilter(JNIEnv *env, jclass cls, jint logLevel)
{
    nl::Weave::Logging::SetLogFilter(logLevel);
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_startDeviceEnumeration(JNIEnv *env, jobject self, jlong deviceMgrPtr, jobject deviceCriteriaObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IdentifyDeviceCriteria deviceCriteria;

    WeaveLogProgress(DeviceManager, "startDeviceEnumeration()");

    err = J2N_IdentifyDeviceCriteria(env, deviceCriteriaObj, deviceCriteria);
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    err = deviceMgr->StartDeviceEnumeration((void *)"StartDeviceEnumeration", deviceCriteria, HandleDeviceEnumerationResponse, HandleError);

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_stopDeviceEnumeration(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "stopDeviceEnumeration()");

    pthread_mutex_lock(&sStackLock);

    deviceMgr->StopDeviceEnumeration();

    pthread_mutex_unlock(&sStackLock);
}

void Java_nl_Weave_DeviceManager_WeaveStack_handleWriteConfirmation(JNIEnv *env, jobject self, jint connObj,
                                        jbyteArray svcIdObj, jbyteArray charIdObj,
                                        jboolean success)
{
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "handleWriteConfirmation() called");

    WeaveBleUUID svcId;
    err = J2N_ByteArrayInPlace(env, svcIdObj, svcId.bytes, sizeof(svcId.bytes));
    SuccessOrExit(err);

    WeaveBleUUID charId;
    err = J2N_ByteArrayInPlace(env, charIdObj, charId.bytes, sizeof(charId.bytes));
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    if (success)
    {
        if (!sBle.HandleWriteConfirmation(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), &svcId, &charId))
        {
            err = BLE_ERROR_WOBLE_PROTOCOL_ABORT;
        }
    }
    else
    {
        sBle.HandleConnectionError(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), BLE_ERROR_GATT_WRITE_FAILED);
    }

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
#endif // CONFIG_NETWORK_LAYER_BLE
}

void Java_nl_Weave_DeviceManager_WeaveStack_handleIndicationReceived(JNIEnv *env, jobject self, jint connObj,
                                         jbyteArray svcIdObj, jbyteArray charIdObj,
                                         jbyteArray dataObj)
{
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "handleIndicationReceived() called");

    WeaveBleUUID svcId;
    WeaveBleUUID charId;
    PacketBuffer *msgBuf = NULL;

    err = J2N_ByteArrayInPlace(env, svcIdObj, svcId.bytes, sizeof(svcId.bytes));
    SuccessOrExit(err);

    err = J2N_ByteArrayInPlace(env, charIdObj, charId.bytes, sizeof(charId.bytes));
    SuccessOrExit(err);

    msgBuf = PacketBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);


    err = J2N_ByteArrayInPlace(env, dataObj, msgBuf->Start(), msgBuf->AvailableDataLength());
    if (err != WEAVE_NO_ERROR)
    {
        PacketBuffer::Free(msgBuf);
        ExitNow();
    }
    msgBuf->SetDataLength(env->GetArrayLength(dataObj));

    pthread_mutex_lock(&sStackLock);

    if (!sBle.HandleIndicationReceived(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), &svcId, &charId, msgBuf)) {
        err = BLE_ERROR_WOBLE_PROTOCOL_ABORT;
        PacketBuffer::Free(msgBuf);
        msgBuf = NULL;
    }

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
#endif // CONFIG_NETWORK_LAYER_BLE
}

void Java_nl_Weave_DeviceManager_WeaveStack_handleSubscribeComplete(JNIEnv *env, jobject self, jint connObj,
                                                                            jbyteArray svcIdObj, jbyteArray charIdObj,
                                                                            jboolean success)
{
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "handleSubscribeComplete() called");

    WeaveBleUUID svcId;
    err = J2N_ByteArrayInPlace(env, svcIdObj, svcId.bytes, sizeof(svcId.bytes));
    SuccessOrExit(err);

    WeaveBleUUID charId;
    err = J2N_ByteArrayInPlace(env, charIdObj, charId.bytes, sizeof(charId.bytes));
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    if (success)
    {
        if (!sBle.HandleSubscribeComplete(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), &svcId, &charId))
        {
        err = BLE_ERROR_WOBLE_PROTOCOL_ABORT;
        }
    }
    else
    {
        sBle.HandleConnectionError(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), BLE_ERROR_GATT_SUBSCRIBE_FAILED);
    }

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
#endif // CONFIG_NETWORK_LAYER_BLE
}

void Java_nl_Weave_DeviceManager_WeaveStack_handleUnsubscribeComplete(JNIEnv *env, jobject self, jint connObj,
                                          jbyteArray svcIdObj, jbyteArray charIdObj,
                                          jboolean success)
{
#if CONFIG_NETWORK_LAYER_BLE
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceManager, "handleUnsubscribeComplete() called");

    WeaveBleUUID svcId;
    err = J2N_ByteArrayInPlace(env, svcIdObj, svcId.bytes, sizeof(svcId.bytes));
    SuccessOrExit(err);

    WeaveBleUUID charId;
    err = J2N_ByteArrayInPlace(env, charIdObj, charId.bytes, sizeof(charId.bytes));
    SuccessOrExit(err);

    pthread_mutex_lock(&sStackLock);

    if (success)
    {
        if (!sBle.HandleUnsubscribeComplete(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), &svcId, &charId))
        {
        err = BLE_ERROR_WOBLE_PROTOCOL_ABORT;
        }
    }
    else
    {
        sBle.HandleConnectionError(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), BLE_ERROR_GATT_UNSUBSCRIBE_FAILED);
    }

    pthread_mutex_unlock(&sStackLock);

exit:
    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);
#endif // CONFIG_NETWORK_LAYER_BLE
}

void Java_nl_Weave_DeviceManager_WeaveStack_handleConnectionError(JNIEnv *env, jobject self, jint connObj)
{
#if CONFIG_NETWORK_LAYER_BLE
    WeaveLogProgress(DeviceManager, "handleConnectionError() called");

    pthread_mutex_lock(&sStackLock);

    sBle.HandleConnectionError(reinterpret_cast<BLE_CONNECTION_OBJECT>(connObj), BLE_ERROR_REMOTE_DEVICE_DISCONNECTED);

    pthread_mutex_unlock(&sStackLock);
#endif
}

void Java_nl_Weave_DeviceManager_WeaveDeviceManager_close(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "close() called");

    if (deviceMgr != NULL)
        deviceMgr->Close();
}

jboolean Java_nl_Weave_DeviceManager_WeaveDeviceManager_isConnected(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "isConnected() called");

    return deviceMgr->IsConnected() ? JNI_TRUE : JNI_FALSE;
}

jlong Java_nl_Weave_DeviceManager_WeaveDeviceManager_deviceId(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    uint64_t deviceId;

    WeaveLogProgress(DeviceManager, "deviceId() called");

    err = deviceMgr->GetDeviceId(deviceId);
    if (err == WEAVE_ERROR_INCORRECT_STATE)
    {
        err = WEAVE_NO_ERROR; // No exception, just return 0.
        deviceId = 0;
    }

    if (err != WEAVE_NO_ERROR)
        ThrowError(env, err);

    return (jlong)deviceId;
}

jstring Java_nl_Weave_DeviceManager_WeaveDeviceManager_deviceAddress(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;
    IPAddress devAddr;
    jstring devAddrStr = NULL;
    char devAddrStrBuf[32];

    WeaveLogProgress(DeviceManager, "deviceAddress() called");

    err = deviceMgr->GetDeviceAddress(devAddr);
    if (err == WEAVE_NO_ERROR)
    {
        devAddr.ToString(devAddrStrBuf, sizeof(devAddrStrBuf));
        devAddrStr = env->NewStringUTF(devAddrStrBuf);
    }
    else if (err == WEAVE_ERROR_INCORRECT_STATE)
        err = WEAVE_NO_ERROR; // No exception, just return null.

    if (err != WEAVE_NO_ERROR)
        ThrowError(env, err);

    return devAddrStr;
}

jobject Java_nl_Weave_DeviceManager_WeaveDeviceDescriptor_decode(JNIEnv *env, jclass cls, jbyteArray encodedDesc)
{
    WEAVE_ERROR err;
    const uint8_t *encodedBuf = NULL;
    uint32_t encodedLen;
    WeaveDeviceDescriptor deviceDesc;
    jobject deviceDescObj = NULL;

    WeaveLogProgress(DeviceManager, "WeaveDeviceDescriptor.decode() called");

    encodedLen = env->GetArrayLength(encodedDesc);
    encodedBuf = (const uint8_t *)env->GetByteArrayElements(encodedDesc, NULL);

    err = WeaveDeviceDescriptor::Decode(encodedBuf, encodedLen, deviceDesc);
    SuccessOrExit(err);

    err = N2J_DeviceDescriptor(env, deviceDesc, deviceDescObj);
    SuccessOrExit(err);

exit:
    if (encodedBuf != NULL)
        env->ReleaseByteArrayElements(encodedDesc, (jbyte *)encodedBuf, JNI_ABORT);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
        ThrowError(env, err);

    return deviceDescObj;
}

void *IOThreadMain(void *arg)
{
    JNIEnv *env;
    JavaVMAttachArgs attachArgs;
    struct timeval sleepTime;
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;

    // Attach the IO thread to the JVM as a daemon thread.  This allows the JVM to shutdown
    // without waiting for this thread to exit.
    attachArgs.version = JNI_VERSION_1_2;
    attachArgs.name = (char *)"Weave Device Manager IO Thread";
    attachArgs.group = NULL;
#ifdef __ANDROID__
    sJVM->AttachCurrentThreadAsDaemon(&env, (void *)&attachArgs);
#else
    sJVM->AttachCurrentThreadAsDaemon((void **)&env, (void *)&attachArgs);
#endif

    WeaveLogProgress(DeviceManager, "IO thread starting starting");

    // Lock the stack to prevent collisions with Java threads.
    pthread_mutex_lock(&sStackLock);

    // Loop until we told to exit.
    while (true)
    {
        numFDs = 0;
        FD_ZERO(&readFDs);
        FD_ZERO(&writeFDs);
        FD_ZERO(&exceptFDs);

        sleepTime.tv_sec = 10;
        sleepTime.tv_usec = 0;

        // Collect the currently active file descriptors.
        sSystemLayer.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, sleepTime);
        sInet.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs, sleepTime);

        // Unlock the stack so that Java threads can make API calls.
        pthread_mutex_unlock(&sStackLock);

        // Wait for for I/O or for the next timer to expire.
        int selectRes = select(numFDs, &readFDs, &writeFDs, &exceptFDs, &sleepTime);

        // Break the loop if requested to shutdown.
        if (sShutdown)
            break;

        // Re-lock the stack.
        pthread_mutex_lock(&sStackLock);

        // Perform I/O and/or dispatch timers.
        sSystemLayer.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
        sInet.HandleSelectResult(selectRes, &readFDs, &writeFDs, &exceptFDs);
    }

    // Detach the thread from the JVM.
    sJVM->DetachCurrentThread();

    return NULL;
}

void HandleIdentifyDeviceComplete(WeaveDeviceManager *deviceMgr, void *reqState, const WeaveDeviceDescriptor *deviceDesc)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jobject deviceDescObj;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to IdentifyDevice request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_DeviceDescriptor(env, *deviceDesc, deviceDescObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onIdentifyDeviceComplete", "(Lnl/Weave/DeviceManager/WeaveDeviceDescriptor;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onIdentifyDeviceComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, deviceDescObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleNetworkScanComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t netCount, const NetworkInfo *netInfoList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jobjectArray netInfoArrayObj;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to ScanNetworks request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_NetworkInfoArray(env, netInfoList, netCount, netInfoArrayObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onScanNetworksComplete", "([Lnl/Weave/DeviceManager/NetworkInfo;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onScanNetworksComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, netInfoArrayObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleGetNetworksComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t netCount, const NetworkInfo *netInfoList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jobjectArray netInfoArrayObj;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to GetNetworks request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_NetworkInfoArray(env, netInfoList, netCount, netInfoArrayObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onGetNetworksComplete", "([Lnl/Weave/DeviceManager/NetworkInfo;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetNetworksComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, netInfoArrayObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleGetCameraAuthDataComplete(WeaveDeviceManager *deviceMgr, void *reqState, const char *macAddress, const char *signedPayload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jmethodID method;
    jstring macAddressStr = NULL;
    jstring signedPayloadStr = NULL;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to GetCameraAuthData request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
    {
        localFramePushed = true;
    }
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    macAddressStr = env->NewStringUTF(macAddress);
    signedPayloadStr = env->NewStringUTF(signedPayload);

    method = env->GetMethodID(deviceMgrCls, "onGetCameraAuthDataComplete", "(Ljava/lang/String;Ljava/lang/String;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetCameraAuthDataComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, macAddressStr, signedPayloadStr);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (NULL != macAddressStr)
    {
        env->DeleteLocalRef(macAddressStr);
    }

    if (NULL != signedPayloadStr)
    {
        env->DeleteLocalRef(signedPayloadStr);
    }

    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
    }
    env->ExceptionClear();
    if (localFramePushed)
    {
        env->PopLocalFrame(NULL);
    }
}

void HandleAddNetworkComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to AddNetwork request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onAddNetworkComplete", "(J)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onAddNetworkComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, (jlong)networkId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleGetRendezvousModeComplete(WeaveDeviceManager *deviceMgr, void *reqState, uint16_t modeFlags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to GetRendezvousMode request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onGetRendezvousModeComplete", "(I)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetRendezvousModeComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, (jint)modeFlags);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleGetFabricConfigComplete(WeaveDeviceManager *deviceMgr, void *reqState, const uint8_t *fabricConfig, uint32_t fabricConfigLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jbyteArray fabricConfigObj;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to GetFabricConfig request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_ByteArray(env, fabricConfig, fabricConfigLen, fabricConfigObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onGetFabricConfigComplete", "([B)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetFabricConfigComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, fabricConfigObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleDeviceEnumerationResponse(WeaveDeviceManager *deviceMgr, void *reqState, const WeaveDeviceDescriptor *deviceDesc,
                                     IPAddress deviceAddr, InterfaceId deviceIntf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jobject deviceDescObj;
    jstring deviceAddrStr = NULL;
    jmethodID method;
    char deviceAddrCStr[INET6_ADDRSTRLEN + IF_NAMESIZE + 2]; // Include space for "<addr>%<interface name>" and NULL terminator
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received device enumeration response");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
    {
        localFramePushed = true;
    }

    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    deviceAddr.ToString(deviceAddrCStr, sizeof(deviceAddrCStr));

    // Add "%" separator character, with NULL terminator, per IETF RFC 4007: https://tools.ietf.org/html/rfc4007
    VerifyOrExit(snprintf(&(deviceAddrCStr[strlen(deviceAddrCStr)]), 2, "%%") > 0, err = System::MapErrorPOSIX(errno));

    // Concatenate zone index (aka interface name) to IP address, with NULL terminator, per IETF RFC 4007: https://tools.ietf.org/html/rfc4007
    err = nl::Inet::GetInterfaceName(deviceIntf, &(deviceAddrCStr[strlen(deviceAddrCStr)]), IF_NAMESIZE + 1);
    SuccessOrExit(err);

    deviceAddrStr = env->NewStringUTF(deviceAddrCStr);

    err = N2J_DeviceDescriptor(env, *deviceDesc, deviceDescObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onDeviceEnumerationResponse", "(Lnl/Weave/DeviceManager/WeaveDeviceDescriptor;Ljava/lang/String;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onDeviceEnumerationResponse method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, deviceDescObj, deviceAddrStr);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
    }

    env->ExceptionClear();

    if (localFramePushed)
    {
        env->DeleteLocalRef(deviceAddrStr);
        env->PopLocalFrame(NULL);
    }
}

void HandleGenericOperationComplete(jobject obj, void *reqState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = obj;
    jclass cls;
    jmethodID method;
    char methodName[128];
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to %s request", (const char *)reqState);

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    cls = env->GetObjectClass(self);
    VerifyOrExit(cls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    snprintf(methodName, sizeof(methodName) - 1, "on%sComplete", (const char *)reqState);
    methodName[sizeof(methodName) - 1] = 0;
    method = env->GetMethodID(cls, methodName, "()V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java %s method", methodName);

    env->ExceptionClear();
    env->CallVoidMethod(self, method);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleSimpleOperationComplete(WeaveDeviceManager *deviceMgr, void *reqState)
{
    jobject self = (jobject)deviceMgr->AppState;
    HandleGenericOperationComplete(self, reqState);
}
#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
void HandleWdmClientComplete(void *context, void *reqState)
{
    WdmClient * wdmClient = reinterpret_cast<WdmClient *>(context);
    jobject self = (jobject)wdmClient->mpAppState;
    HandleGenericOperationComplete(self, reqState);
}

void HandleGenericTraitUpdatableDataSinkComplete(void *context, void *reqState)
{
    GenericTraitUpdatableDataSink * sink = reinterpret_cast<GenericTraitUpdatableDataSink *>(context);
    jobject self = (jobject)sink->mpAppState;
    HandleGenericOperationComplete(self, reqState);
}

void HandleWdmClientError(void *context, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus)
{
    WdmClient * wdmClient = reinterpret_cast<WdmClient *>(context);
    jobject self = (jobject)wdmClient->mpAppState;
    HandleGenericError(self, reqState, deviceMgrErr, devStatus);
}

void HandleGenericTraitUpdatableDataSinkError(void *context, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus)
{
    GenericTraitUpdatableDataSink * sink = reinterpret_cast<GenericTraitUpdatableDataSink *>(context);
    jobject self = (jobject)sink->mpAppState;
    HandleGenericError(self, reqState, deviceMgrErr, devStatus);
}

void HandleWdmClientFlushUpdateComplete(void *context, void *reqState, uint16_t pathCount, WdmClientFlushUpdateStatus *statusResults)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    WdmClient * wdmClient = reinterpret_cast<WdmClient *>(context);
    jobject self = (jobject)wdmClient->mpAppState;
    jclass wdmClientCls;
    jclass java_lang_Throwable = NULL;
    jmethodID method;
    jobjectArray exceptionArrayObj = NULL;
    bool localFramePushed = false;

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);
    WeaveLogProgress(DeviceManager, "Received response to FlushUpdate request, number of failed updated path is %d", pathCount);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
    {
        localFramePushed = true;
    }

    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    java_lang_Throwable = env->FindClass("java/lang/Throwable");
    VerifyOrExit(java_lang_Throwable != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    err = N2J_GenericArray(env, pathCount, java_lang_Throwable, exceptionArrayObj,
    [&statusResults](JNIEnv *_env, uint32_t index, jobject& ex) -> WEAVE_ERROR
    {
        WEAVE_ERROR _err = WEAVE_NO_ERROR;
        if (statusResults[index].mErrorCode == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            _err = N2J_WdmClientFlushUpdateDeviceStatus(_env, statusResults[index].mDevStatus, ex, statusResults->mpPath, statusResults->mpDataSink);
        }
        else
        {
            _err = N2J_WdmClientFlushUpdateError(_env, statusResults[index].mErrorCode, ex, statusResults->mpPath, statusResults->mpDataSink);
        }
        return _err;
    });
    SuccessOrExit(err);

    wdmClientCls = env->GetObjectClass(self);
    VerifyOrExit(wdmClientCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(wdmClientCls, "onFlushUpdateComplete", "([Ljava/lang/Throwable;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onFlushUpdateComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, exceptionArrayObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
void HandleNotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;

    WeaveLogProgress(DeviceManager, "Received NotifyWeaveConnectionClosed");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    method = env->GetStaticMethodID(sWeaveStackCls, "onNotifyWeaveConnectionClosed", "(I)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java NotifyWeaveConnectionClosed");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    env->CallStaticVoidMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj));
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

bool HandleSendCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId, const uint8_t *characteristicData, uint32_t characteristicDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jbyteArray svcIdObj;
    jbyteArray charIdObj;
    jbyteArray characteristicDataObj;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;
    bool rc = false;

    WeaveLogProgress(DeviceManager, "Received SendCharacteristic");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_ByteArray(env, svcId, 16, svcIdObj);
    SuccessOrExit(err);

    err = N2J_ByteArray(env, charId, 16, charIdObj);
    SuccessOrExit(err);

    err = N2J_ByteArray(env, characteristicData, characteristicDataLen, characteristicDataObj);
    SuccessOrExit(err);

    method = env->GetStaticMethodID(sWeaveStackCls, "onSendCharacteristic", "(I[B[B[B)Z");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java SendCharacteristic");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    rc = (bool) env->CallStaticBooleanMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj), svcIdObj, charIdObj, characteristicDataObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
        rc = false;
    }
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);

    return rc;
}

bool HandleSubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jbyteArray svcIdObj;
    jbyteArray charIdObj;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;
    bool rc = false;

    WeaveLogProgress(DeviceManager, "Received SubscribeCharacteristic");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_ByteArray(env, svcId, 16, svcIdObj);
    SuccessOrExit(err);

    err = N2J_ByteArray(env, charId, 16, charIdObj);
    SuccessOrExit(err);

    method = env->GetStaticMethodID(sWeaveStackCls, "onSubscribeCharacteristic", "(I[B[B)Z");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java SubscribeCharacteristic");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    rc = (bool) env->CallStaticBooleanMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj), svcIdObj, charIdObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
        rc = false;
    }
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);

    return rc;
}

bool HandleUnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const uint8_t *svcId, const uint8_t *charId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jbyteArray svcIdObj;
    jbyteArray charIdObj;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;
    bool rc = false;

    WeaveLogProgress(DeviceManager, "Received UnsubscribeCharacteristic");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_ByteArray(env, svcId, 16, svcIdObj);
    SuccessOrExit(err);

    err = N2J_ByteArray(env, charId, 16, charIdObj);
    SuccessOrExit(err);

    method = env->GetStaticMethodID(sWeaveStackCls, "onUnsubscribeCharacteristic", "(I[B[B)Z");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java UnsubscribeCharacteristic");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    rc = (bool) env->CallStaticBooleanMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj), svcIdObj, charIdObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
        rc = false;
    }
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);

    return rc;
}

void HandlePairTokenComplete(WeaveDeviceManager *deviceMgr, void *reqState, const uint8_t *pairingTokenBundle, uint32_t pairingTokenBundleLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jbyteArray pairingTokenBundleObj;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to PairToken request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_ByteArray(env, pairingTokenBundle, pairingTokenBundleLen, pairingTokenBundleObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onPairTokenComplete", "([B)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onPairTokenComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, pairingTokenBundleObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleUnpairTokenComplete(WeaveDeviceManager *deviceMgr, void *reqState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to UnpairToken request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onUnpairTokenComplete", "()V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onUnpairTokenComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleGetWirelessRegulatoryConfigComplete(WeaveDeviceManager *deviceMgr, void *reqState, const WirelessRegConfig * regConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = (jobject)deviceMgr->AppState;
    jclass deviceMgrCls;
    jobject regConfigObj = NULL;
    jmethodID method;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received response to GetWirelessRegulatoryConfig request");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    err = N2J_WirelessRegulatoryConfig(env, *regConfig, regConfigObj);
    SuccessOrExit(err);

    deviceMgrCls = env->GetObjectClass(self);
    VerifyOrExit(deviceMgrCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(deviceMgrCls, "onGetWirelessRegulatoryConfigComplete", "(Lnl/Weave/DeviceManager/WirelessRegulatoryConfig;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetWirelessRegulatoryConfigComplete method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, regConfigObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

bool HandleCloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;
    bool rc = false;

    WeaveLogProgress(DeviceManager, "Received CloseConnection");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    method = env->GetStaticMethodID(sWeaveStackCls, "onCloseConnection", "(I)Z");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java CloseConnection");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    rc = (bool) env->CallStaticBooleanMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj));
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
        rc = false;
    }
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
    return rc;
}

uint16_t HandleGetMTU(BLE_CONNECTION_OBJECT connObj)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jmethodID method;
    bool localFramePushed = false;
    intptr_t tmpConnObj;
    uint16_t mtu = 0;

    WeaveLogProgress(DeviceManager, "Received GetMTU");

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    method = env->GetStaticMethodID(sWeaveStackCls, "onGetMTU", "(I)I");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onGetMTU");

    env->ExceptionClear();
    tmpConnObj = reinterpret_cast<intptr_t>(connObj);
    mtu = (int16_t) env->CallStaticIntMethod(sWeaveStackCls, method, static_cast<jint>(tmpConnObj));
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ReportError(env, err, __FUNCTION__);
        mtu = 0;
    }
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
    return mtu;
}

void HandleGenericError(jobject obj, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    JNIEnv *env;
    jobject self = obj;
    jclass cls;
    jmethodID method;
    jthrowable ex;
    bool localFramePushed = false;

    WeaveLogProgress(DeviceManager, "Received error response to %s request", (const char *)reqState);

    sJVM->GetEnv((void **)&env, JNI_VERSION_1_2);

    if (env->PushLocalFrame(WDM_JNI_CALLBACK_LOCAL_REF_COUNT) == 0)
        localFramePushed = true;
    VerifyOrExit(localFramePushed, err = WEAVE_ERROR_NO_MEMORY);

    if (deviceMgrErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED && devStatus != NULL)
    {
        err = N2J_DeviceStatus(env, *devStatus, ex);
        SuccessOrExit(err);
    }
    else
    {
        err = N2J_Error(env, deviceMgrErr, ex);
        SuccessOrExit(err);
    }

    cls = env->GetObjectClass(self);
    VerifyOrExit(cls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    method = env->GetMethodID(cls, "onError", "(Ljava/lang/Throwable;)V");
    VerifyOrExit(method != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    WeaveLogProgress(DeviceManager, "Calling Java onError method");

    env->ExceptionClear();
    env->CallVoidMethod(self, method, ex);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    if (err != WEAVE_NO_ERROR)
        ReportError(env, err, __FUNCTION__);
    env->ExceptionClear();
    if (localFramePushed)
        env->PopLocalFrame(NULL);
}

void HandleError(WeaveDeviceManager *deviceMgr, void *reqState, WEAVE_ERROR deviceMgrErr, DeviceStatus *devStatus)
{
    jobject self = (jobject)deviceMgr->AppState;
    HandleGenericError(self, reqState, deviceMgrErr, devStatus);
}

void ThrowError(JNIEnv *env, WEAVE_ERROR errToThrow)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jthrowable ex;

    err = N2J_Error(env, errToThrow, ex);
    if (err == WEAVE_NO_ERROR)
        env->Throw(ex);
}

void ReportError(JNIEnv *env, WEAVE_ERROR cbErr, const char *functName)
{
    if (cbErr == WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        WeaveLogError(DeviceManager, "Java exception thrown in %s", functName);
        env->ExceptionDescribe();
    }
    else
    {
        const char *errStr;
        switch (cbErr)
        {
        case WDM_JNI_ERROR_TYPE_NOT_FOUND            : errStr = "JNI type not found"; break;
        case WDM_JNI_ERROR_METHOD_NOT_FOUND          : errStr = "JNI method not found"; break;
        case WDM_JNI_ERROR_FIELD_NOT_FOUND           : errStr = "JNI field not found"; break;
        default:                                     ; errStr = nl::ErrorStr(cbErr); break;
        }
        WeaveLogError(DeviceManager, "Error in %s : %s", functName, errStr);
    }
}

template<typename GetElementFunct>
WEAVE_ERROR N2J_GenericArray(JNIEnv *env, uint32_t arrayLen, jclass arrayElemCls, jobjectArray& outArray, GetElementFunct getElem)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jobject elem = NULL;

    outArray = env->NewObjectArray(arrayLen, arrayElemCls, NULL);
    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_NO_MEMORY);

    for (uint32_t i = 0; i < arrayLen; i++)
    {
        err = getElem(env, i, elem);
        SuccessOrExit(err);

        env->ExceptionClear();
        env->SetObjectArrayElement(outArray, i, elem);
        VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

        env->DeleteLocalRef(elem);
        elem = NULL;
    }

exit:
    env->DeleteLocalRef(elem);
    return err;
}

WEAVE_ERROR J2N_ByteArray(JNIEnv *env, jbyteArray inArray, uint8_t *& outArray, uint32_t& outArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    outArrayLen = env->GetArrayLength(inArray);

    outArray = (uint8_t *)malloc(outArrayLen);
    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (outArrayLen != 0)
    {
        env->ExceptionClear();
        env->GetByteArrayRegion(inArray, 0, outArrayLen, (jbyte *)outArray);
        VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (outArray != NULL)
        {
            free(outArray);
            outArray = NULL;
        }
    }

    return err;
}

// A version of J2N_ByteArray that copies into an existing buffer, rather than allocating memory.
WEAVE_ERROR J2N_ByteArrayInPlace(JNIEnv *env, jbyteArray inArray, uint8_t * outArray, uint32_t maxArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t arrayLen;

    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    arrayLen = env->GetArrayLength(inArray);
    VerifyOrExit(arrayLen <= maxArrayLen, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (arrayLen != 0)
    {
        env->ExceptionClear();
        env->GetByteArrayRegion(inArray, 0, arrayLen, (jbyte *)outArray);
        VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);
    }

exit:
    return err;
}

WEAVE_ERROR J2N_StdString(JNIEnv *env, jstring inString, std::string& outString)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass java_lang_String = NULL;
    jmethodID ctor = NULL;
    jbyteArray strJbytes = NULL;
    const uint8_t *pDataBuf = NULL;
    size_t length = 0;

    VerifyOrExit(inString != NULL, err = WEAVE_ERROR_NO_MEMORY);

    java_lang_String = env->FindClass("java/lang/String");
    VerifyOrExit(java_lang_String != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    ctor = env->GetMethodID(java_lang_String, "getBytes", "(Ljava/lang/String;)[B");
    VerifyOrExit(ctor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    strJbytes = (jbyteArray) env->CallObjectMethod(inString, ctor, env->NewStringUTF("UTF-8"));
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

    length = (size_t) env->GetArrayLength(strJbytes);
    pDataBuf = (const uint8_t *)env->GetByteArrayElements(strJbytes, NULL);

    outString.append((char *)pDataBuf, length);

exit:
    if (strJbytes != NULL)
    {
        env->ReleaseByteArrayElements(strJbytes, (jbyte* )pDataBuf, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    env->DeleteLocalRef(strJbytes);
    env->DeleteLocalRef(java_lang_String);
    return err;
}

WEAVE_ERROR N2J_ByteArray(JNIEnv *env, const uint8_t *inArray, uint32_t inArrayLen, jbyteArray& outArray)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    outArray = env->NewByteArray((int)inArrayLen);
    VerifyOrExit(outArray != NULL, err = WEAVE_ERROR_NO_MEMORY);

    env->ExceptionClear();
    env->SetByteArrayRegion(outArray, 0, inArrayLen, (jbyte *)inArray);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    return err;
}

WEAVE_ERROR N2J_NewStringUTF(JNIEnv *env, const char *inStr, jstring& outString)
{
    return N2J_NewStringUTF(env, inStr, strlen(inStr), outString);
}

WEAVE_ERROR N2J_NewStringUTF(JNIEnv *env, const char *inStr, size_t inStrLen, jstring& outString)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jbyteArray charArray = NULL;
    jstring utf8Encoding = NULL;
    jclass java_lang_String = NULL;
    jmethodID ctor = NULL;

    err = N2J_ByteArray(env, reinterpret_cast<const uint8_t *>(inStr), inStrLen, charArray);
    SuccessOrExit(err);

    utf8Encoding = env->NewStringUTF("UTF-8");
    VerifyOrExit(utf8Encoding != NULL, err = WEAVE_ERROR_NO_MEMORY);

    java_lang_String = env->FindClass("java/lang/String");
    VerifyOrExit(java_lang_String != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    ctor = env->GetMethodID(java_lang_String, "<init>", "([BLjava/lang/String;)V");
    VerifyOrExit(ctor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    outString = (jstring) env->NewObject(java_lang_String, ctor, charArray, utf8Encoding);
    VerifyOrExit(outString != NULL, err = WEAVE_ERROR_NO_MEMORY);

exit:
    // error code propagated from here, so clear any possible
    // exceptions that arose here
    env->ExceptionClear();

    if (utf8Encoding != NULL)
        env->DeleteLocalRef(utf8Encoding);
    if (charArray != NULL)
        env->DeleteLocalRef(charArray);

    return err;
}

#if CURRENTLY_UNUSED

WEAVE_ERROR J2N_EnumVal(JNIEnv *env, jobject enumObj, int& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass enumCls = NULL;
    jfieldID fieldId;

    enumCls = env->GetObjectClass(enumObj);
    VerifyOrExit(enumCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(enumCls, "val", "I");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetIntField(enumObj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(enumCls);
    return err;
}

WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "I");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetIntField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

#endif // CURRENTLY_UNUSED

WEAVE_ERROR J2N_EnumFieldVal(JNIEnv *env, jobject obj, const char *fieldName, const char *fieldType, int& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL, enumCls = NULL;
    jfieldID fieldId;
    jobject enumObj = NULL;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, fieldType);
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    enumObj = env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

    if (enumObj != NULL)
    {
        enumCls = env->GetObjectClass(enumObj);
        VerifyOrExit(enumCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

        fieldId = env->GetFieldID(enumCls, "val", "I");
        VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

        env->ExceptionClear();
        outVal = env->GetIntField(enumObj, fieldId);
        VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);
    }
    else
        outVal = -1;

exit:
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(enumObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_ShortFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jshort& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "S");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetShortField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_IntFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jint& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "I");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetIntField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_LongFieldVal(JNIEnv *env, jobject obj, const char *fieldName, jlong& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "J");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outVal = env->GetLongField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_StringFieldVal(JNIEnv *env, jobject obj, const char *fieldName, char *& outVal)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;
    jstring strObj = NULL;
    const char *nativeStr;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "Ljava/lang/String;");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    strObj = (jstring)env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

    if (strObj != NULL)
    {
        nativeStr = env->GetStringUTFChars(strObj, 0);
        outVal = strdup(nativeStr);
        env->ReleaseStringUTFChars(strObj, nativeStr);
    }
    else
        outVal = NULL;

exit:
    env->DeleteLocalRef(strObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_ByteArrayFieldVal(JNIEnv *env, jobject obj, const char *fieldName, uint8_t *& outArray, uint32_t& outArrayLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass objCls = NULL;
    jfieldID fieldId;
    jbyteArray byteArrayObj = NULL;

    objCls = env->GetObjectClass(obj);
    VerifyOrExit(objCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    fieldId = env->GetFieldID(objCls, fieldName, "[B");
    VerifyOrExit(fieldId != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    byteArrayObj = (jbyteArray)env->GetObjectField(obj, fieldId);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

    if (byteArrayObj != NULL)
    {
        err = J2N_ByteArray(env, byteArrayObj, outArray, outArrayLen);
        SuccessOrExit(err);
    }
    else
    {
        outArray = NULL;
        outArrayLen = 0;
    }

exit:
    env->DeleteLocalRef(byteArrayObj);
    env->DeleteLocalRef(objCls);
    return err;
}

WEAVE_ERROR J2N_IdentifyDeviceCriteria(JNIEnv *env, jobject inCriteria, IdentifyDeviceCriteria& outCriteria)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jlong longVal;
    jint intVal;
    int enumVal;

    err = J2N_LongFieldVal(env, inCriteria, "TargetFabricId", longVal);
    SuccessOrExit(err);
    outCriteria.TargetFabricId = (uint64_t)longVal;

    err = J2N_EnumFieldVal(env, inCriteria, "TargetModes", "Lnl/Weave/DeviceManager/TargetDeviceModes;", enumVal);
    SuccessOrExit(err);
    outCriteria.TargetModes = (uint32_t)enumVal;

    err = J2N_IntFieldVal(env, inCriteria, "TargetVendorId", intVal);
    SuccessOrExit(err);
    outCriteria.TargetVendorId = (uint16_t)intVal;

    err = J2N_IntFieldVal(env, inCriteria, "TargetProductId", intVal);
    SuccessOrExit(err);
    outCriteria.TargetProductId = (uint16_t)intVal;

    err = J2N_LongFieldVal(env, inCriteria, "TargetDeviceId", longVal);
    SuccessOrExit(err);
    outCriteria.TargetDeviceId = (uint64_t)longVal;

exit:
    return err;
}

WEAVE_ERROR J2N_NetworkInfo(JNIEnv *env, jobject inNetworkInfo, NetworkInfo& outNetworkInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jlong longVal;
    jint intVal;
    jshort shortVal;
    uint32_t len;
    int enumVal;

    err = J2N_EnumFieldVal(env, inNetworkInfo, "NetworkType", "Lnl/Weave/DeviceManager/NetworkType;", enumVal);
    SuccessOrExit(err);
    outNetworkInfo.NetworkType = (NetworkProvisioning::NetworkType)enumVal;

    err = J2N_LongFieldVal(env, inNetworkInfo, "NetworkId", longVal);
    SuccessOrExit(err);
    outNetworkInfo.NetworkId = longVal;

    err = J2N_StringFieldVal(env, inNetworkInfo, "WiFiSSID", outNetworkInfo.WiFiSSID);
    SuccessOrExit(err);

    err = J2N_EnumFieldVal(env, inNetworkInfo, "WiFiMode", "Lnl/Weave/DeviceManager/WiFiMode;", enumVal);
    SuccessOrExit(err);
    outNetworkInfo.WiFiMode = (NetworkProvisioning::WiFiMode)enumVal;

    err = J2N_EnumFieldVal(env, inNetworkInfo, "WiFiRole", "Lnl/Weave/DeviceManager/WiFiRole;", enumVal);
    SuccessOrExit(err);
    outNetworkInfo.WiFiRole = (NetworkProvisioning::WiFiRole)enumVal;

    err = J2N_EnumFieldVal(env, inNetworkInfo, "WiFiSecurityType", "Lnl/Weave/DeviceManager/WiFiSecurityType;", enumVal);
    SuccessOrExit(err);
    outNetworkInfo.WiFiSecurityType = (NetworkProvisioning::WiFiSecurityType)enumVal;

    err = J2N_ByteArrayFieldVal(env, inNetworkInfo, "WiFiKey", outNetworkInfo.WiFiKey, outNetworkInfo.WiFiKeyLen);
    SuccessOrExit(err);

    err = J2N_StringFieldVal(env, inNetworkInfo, "ThreadNetworkName", outNetworkInfo.ThreadNetworkName);
    SuccessOrExit(err);

    err = J2N_ByteArrayFieldVal(env, inNetworkInfo, "ThreadExtendedPANId", outNetworkInfo.ThreadExtendedPANId, len);
    SuccessOrExit(err);
    if (outNetworkInfo.ThreadExtendedPANId != NULL && len != NetworkInfo::kThreadExtendedPANIdLength)
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = J2N_ByteArrayFieldVal(env, inNetworkInfo, "ThreadNetworkKey", outNetworkInfo.ThreadNetworkKey, len);
    SuccessOrExit(err);
    if (outNetworkInfo.ThreadNetworkKey != NULL && len != NetworkInfo::kThreadNetworkKeyLength)
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = J2N_ByteArrayFieldVal(env, inNetworkInfo, "ThreadPSKc", outNetworkInfo.ThreadPSKc, len);
    SuccessOrExit(err);
    if (outNetworkInfo.ThreadPSKc!= NULL && len != NetworkInfo::kThreadPSKcLength)
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = J2N_IntFieldVal(env, inNetworkInfo, "ThreadPANId", intVal);
    SuccessOrExit(err);
    outNetworkInfo.ThreadPANId = intVal;

    err = J2N_IntFieldVal(env, inNetworkInfo, "ThreadChannel", intVal);
    SuccessOrExit(err);
    outNetworkInfo.ThreadChannel = intVal;

    err = J2N_ShortFieldVal(env, inNetworkInfo, "WirelessSignalStrength", shortVal);
    SuccessOrExit(err);
    outNetworkInfo.WirelessSignalStrength = shortVal;

exit:
    return err;
}

WEAVE_ERROR N2J_NetworkInfo(JNIEnv *env, const NetworkInfo& inNetworkInfo, jobject& outNetworkInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID makeMethod;
    jstring wifiSSID = NULL;
    jbyteArray wifiKey = NULL;
    jstring threadNetName = NULL;
    jbyteArray threadExtPANId = NULL;
    jbyteArray threadKey = NULL;
    jbyteArray threadPSKc = NULL;

    if (inNetworkInfo.WiFiSSID != NULL)
    {
        err = N2J_NewStringUTF(env, inNetworkInfo.WiFiSSID, wifiSSID);
        SuccessOrExit(err);
    }

    if (inNetworkInfo.WiFiKey != NULL)
    {
        err = N2J_ByteArray(env, inNetworkInfo.WiFiKey, inNetworkInfo.WiFiKeyLen, wifiKey);
        SuccessOrExit(err);
    }

    if (inNetworkInfo.ThreadNetworkName != NULL)
    {
        err = N2J_NewStringUTF(env, inNetworkInfo.ThreadNetworkName, threadNetName);
        SuccessOrExit(err);
    }

    if (inNetworkInfo.ThreadExtendedPANId != NULL)
    {
        err = N2J_ByteArray(env, inNetworkInfo.ThreadExtendedPANId, NetworkInfo::kThreadExtendedPANIdLength, threadExtPANId);
        SuccessOrExit(err);
    }

    if (inNetworkInfo.ThreadNetworkKey != NULL)
    {
        err = N2J_ByteArray(env, inNetworkInfo.ThreadNetworkKey, NetworkInfo::kThreadNetworkKeyLength, threadKey);
        SuccessOrExit(err);
    }

    if (inNetworkInfo.ThreadPSKc != NULL)
    {
        err = N2J_ByteArray(env, inNetworkInfo.ThreadPSKc, NetworkInfo::kThreadPSKcLength, threadPSKc);
        SuccessOrExit(err);
    }

    makeMethod = env->GetStaticMethodID(sNetworkInfoCls, "Make", "(IJLjava/lang/String;III[BLjava/lang/String;[B[B[BSII)Lnl/Weave/DeviceManager/NetworkInfo;");
    VerifyOrExit(makeMethod != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outNetworkInfo = env->CallStaticObjectMethod(sNetworkInfoCls, makeMethod,
                                                 (jint)inNetworkInfo.NetworkType,
                                                 (jlong)inNetworkInfo.NetworkId,
                                                 wifiSSID,
                                                 (jint)inNetworkInfo.WiFiMode,
                                                 (jint)inNetworkInfo.WiFiRole,
                                                 (jint)inNetworkInfo.WiFiSecurityType,
                                                 wifiKey,
                                                 threadNetName,
                                                 threadExtPANId,
                                                 threadKey,
                                                 threadPSKc,
                                                 (jshort)inNetworkInfo.WirelessSignalStrength,
                                                 (jint)inNetworkInfo.ThreadPANId,
                                                 (jint)inNetworkInfo.ThreadChannel);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(threadKey);
    env->DeleteLocalRef(threadExtPANId);
    env->DeleteLocalRef(threadNetName);
    env->DeleteLocalRef(wifiKey);
    env->DeleteLocalRef(wifiSSID);
    return err;
}

WEAVE_ERROR N2J_NetworkInfoArray(JNIEnv *env, const NetworkInfo *inArray, uint32_t inArrayLen, jobjectArray& outArray)
{
    return N2J_GenericArray(env, inArrayLen, sNetworkInfoCls, outArray,
            [&](JNIEnv * _env, uint32_t index, jobject& _outElem) -> WEAVE_ERROR
            {
                return N2J_NetworkInfo(_env, inArray[index], _outElem);
            });
}

WEAVE_ERROR N2J_DeviceDescriptor(JNIEnv *env, const WeaveDeviceDescriptor& inDeviceDesc, jobject& outDeviceDesc)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    jbyteArray primary802154MACAddress = NULL;
    jbyteArray primaryWiFiMACAddress = NULL;
    jstring serialNumber = NULL;
    jstring rendezvousWiFiESSID = NULL;
    jstring pairingCode = NULL;
    jstring softwareVersion = NULL;

    if (!WeaveDeviceDescriptor::IsZeroBytes(inDeviceDesc.Primary802154MACAddress, sizeof(inDeviceDesc.Primary802154MACAddress)))
    {
        err = N2J_ByteArray(env, inDeviceDesc.Primary802154MACAddress, sizeof(inDeviceDesc.Primary802154MACAddress), primary802154MACAddress);
        SuccessOrExit(err);
    }

    if (!WeaveDeviceDescriptor::IsZeroBytes(inDeviceDesc.PrimaryWiFiMACAddress, sizeof(inDeviceDesc.PrimaryWiFiMACAddress)))
    {
        err = N2J_ByteArray(env, inDeviceDesc.PrimaryWiFiMACAddress, sizeof(inDeviceDesc.PrimaryWiFiMACAddress), primaryWiFiMACAddress);
        SuccessOrExit(err);
    }

    if (inDeviceDesc.SerialNumber[0] != 0)
    {
        err = N2J_NewStringUTF(env, inDeviceDesc.SerialNumber, serialNumber);
        SuccessOrExit(err);
    }

    if (inDeviceDesc.RendezvousWiFiESSID[0] != 0)
    {
        err = N2J_NewStringUTF(env, inDeviceDesc.RendezvousWiFiESSID, rendezvousWiFiESSID);
        SuccessOrExit(err);
    }

    if (inDeviceDesc.PairingCode[0] != 0)
    {
        err = N2J_NewStringUTF(env, inDeviceDesc.PairingCode, pairingCode);
        SuccessOrExit(err);
    }

    if (inDeviceDesc.SoftwareVersion[0] != 0)
    {
        err = N2J_NewStringUTF(env, inDeviceDesc.SoftwareVersion, softwareVersion);
        SuccessOrExit(err);
    }

    constructor = env->GetMethodID(sWeaveDeviceDescriptorCls, "<init>", "(IIIIII[B[BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;IIII)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outDeviceDesc = (jthrowable)env->NewObject(sWeaveDeviceDescriptorCls, constructor,
                                               inDeviceDesc.VendorId, inDeviceDesc.ProductId, inDeviceDesc.ProductRevision,
                                               inDeviceDesc.ManufacturingDate.Year, inDeviceDesc.ManufacturingDate.Month, inDeviceDesc.ManufacturingDate.Day,
                                               primary802154MACAddress, primaryWiFiMACAddress,
                                               serialNumber, rendezvousWiFiESSID, pairingCode,
                                               (jlong)inDeviceDesc.DeviceId, (jlong)inDeviceDesc.FabricId, softwareVersion,
                                               (jint)inDeviceDesc.PairingCompatibilityVersionMajor,
                                               (jint)inDeviceDesc.PairingCompatibilityVersionMinor,
                                               (jint)inDeviceDesc.DeviceFeatures, (jint)inDeviceDesc.Flags);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(softwareVersion);
    env->DeleteLocalRef(pairingCode);
    env->DeleteLocalRef(rendezvousWiFiESSID);
    env->DeleteLocalRef(serialNumber);
    env->DeleteLocalRef(primaryWiFiMACAddress);
    env->DeleteLocalRef(primary802154MACAddress);
    return err;
}

WEAVE_ERROR J2N_WirelessRegulatoryConfig(JNIEnv *env, jobject inRegConfig, WirelessRegConfig & outRegConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int enumVal;
    char * strVal = NULL;

    outRegConfig.Init();

    err = J2N_StringFieldVal(env, inRegConfig, "RegDomain", strVal);
    SuccessOrExit(err);

    // If present, RegDomain must be exactly 2 characters long.
    if (strVal != NULL)
    {
        VerifyOrExit(strlen(strVal) == sizeof(outRegConfig.RegDomain.Code), err = WEAVE_ERROR_INVALID_ARGUMENT);

        memcpy(outRegConfig.RegDomain.Code, strVal, sizeof(outRegConfig.RegDomain.Code));
    }

    err = J2N_EnumFieldVal(env, inRegConfig, "OpLocation", "Lnl/Weave/DeviceManager/WirelessOperatingLocation;", enumVal);
    SuccessOrExit(err);
    outRegConfig.OpLocation = (uint8_t)enumVal;

    // NOTE: The SupportRegulatoryDomains field is never sent *to* a device.  Thus we ignore the field value here.

exit:
    if (strVal != NULL)
    {
        free(strVal);
    }
    return err;
}

WEAVE_ERROR N2J_WirelessRegulatoryConfig(JNIEnv *env, const WirelessRegConfig& inRegConfig, jobject& outRegConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    jstring regDomain = NULL;
    jobjectArray supportedRegDomains = NULL;
    jclass java_lang_String = NULL;

    if (inRegConfig.IsRegDomainPresent())
    {
        err = N2J_NewStringUTF(env, inRegConfig.RegDomain.Code, sizeof(inRegConfig.RegDomain.Code), regDomain);
        SuccessOrExit(err);
    }

    java_lang_String = env->FindClass("java/lang/String");
    VerifyOrExit(java_lang_String != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    err = N2J_GenericArray(env, inRegConfig.NumSupportedRegDomains, java_lang_String, supportedRegDomains,
        [&inRegConfig](JNIEnv *_env, uint32_t index, jobject& outElem) -> WEAVE_ERROR
        {
            jstring strElem = NULL;
            WEAVE_ERROR _err = N2J_NewStringUTF(_env, inRegConfig.SupportedRegDomains[index].Code, sizeof(WirelessRegDomain::Code), strElem);
            outElem = strElem;
            return _err;
        });
    SuccessOrExit(err);

    constructor = env->GetMethodID(sWirelessRegulatoryConfigCls, "<init>", "(Ljava/lang/String;I[Ljava/lang/String;)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    env->ExceptionClear();
    outRegConfig = (jthrowable)env->NewObject(sWirelessRegulatoryConfigCls, constructor,
            regDomain, (jint)inRegConfig.OpLocation, supportedRegDomains);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(regDomain);
    env->DeleteLocalRef(supportedRegDomains);
    return err;
}

WEAVE_ERROR N2J_Error(JNIEnv *env, WEAVE_ERROR inErr, jthrowable& outEx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    const char *errStr = NULL;
    jstring errStrObj = NULL;

    constructor = env->GetMethodID(sWeaveDeviceManagerExceptionCls, "<init>", "(ILjava/lang/String;)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    switch (inErr)
    {
    case WDM_JNI_ERROR_TYPE_NOT_FOUND            : errStr = "Weave Device Manager Error: JNI type not found"; break;
    case WDM_JNI_ERROR_METHOD_NOT_FOUND          : errStr = "Weave Device Manager Error: JNI method not found"; break;
    case WDM_JNI_ERROR_FIELD_NOT_FOUND           : errStr = "Weave Device Manager Error: JNI field not found"; break;
    default:                                     ; errStr = nl::ErrorStr(inErr); break;
    }
    errStrObj = (errStr != NULL) ? env->NewStringUTF(errStr) : NULL;

    env->ExceptionClear();
    outEx = (jthrowable)env->NewObject(sWeaveDeviceManagerExceptionCls, constructor, (jint)inErr, errStrObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(errStrObj);
    return err;
}

WEAVE_ERROR N2J_DeviceStatus(JNIEnv *env, DeviceStatus& devStatus, jthrowable& outEx)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    const char *errStr = NULL;
    jstring errStrObj = NULL;

    constructor = env->GetMethodID(sWeaveDeviceExceptionCls, "<init>", "(IIILjava/lang/String;)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    errStr = nl::StatusReportStr(devStatus.StatusProfileId, devStatus.StatusCode);
    errStrObj = (errStr != NULL) ? env->NewStringUTF(errStr) : NULL;

    env->ExceptionClear();
    outEx = (jthrowable)env->NewObject(sWeaveDeviceExceptionCls, constructor, (jint)devStatus.StatusCode, (jint)devStatus.StatusProfileId, (jint)devStatus.SystemErrorCode, errStrObj);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(errStrObj);
    return err;
}

WEAVE_ERROR N2J_WdmClientFlushUpdateError(JNIEnv *env, WEAVE_ERROR inErr, jobject& outEx, const char * path, TraitDataSink * dataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    const char *errStr = NULL;
    jstring errStrObj = NULL;
    jstring pathStrObj = NULL;

    constructor = env->GetMethodID(sWdmClientFlushUpdateExceptionCls, "<init>", "(ILjava/lang/String;Ljava/lang/String;J)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);

    switch (inErr)
    {
        case WDM_JNI_ERROR_TYPE_NOT_FOUND            : errStr = "WdmClient Error: JNI type not found"; break;
        case WDM_JNI_ERROR_METHOD_NOT_FOUND          : errStr = "WdmClient Error: JNI method not found"; break;
        case WDM_JNI_ERROR_FIELD_NOT_FOUND           : errStr = "WdmClient Error: JNI field not found"; break;
        default:                                     ; errStr = nl::ErrorStr(inErr); break;
    }

    errStrObj = (errStr != NULL) ? env->NewStringUTF(errStr) : NULL;
    pathStrObj = (path != NULL) ? env->NewStringUTF(path) : NULL;

    env->ExceptionClear();
    outEx = env->NewObject(sWdmClientFlushUpdateExceptionCls, constructor, (jint)inErr, errStrObj, pathStrObj, (jlong)dataSink);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(errStrObj);
    env->DeleteLocalRef(pathStrObj);
    return err;
}

WEAVE_ERROR N2J_WdmClientFlushUpdateDeviceStatus(JNIEnv *env, DeviceStatus& devStatus, jobject& outEx, const char * path, TraitDataSink * dataSink)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jmethodID constructor;
    const char *errStr = NULL;
    jstring errStrObj = NULL;
    jstring pathStrObj = NULL;

    constructor = env->GetMethodID(sWdmClientFlushUpdateDeviceExceptionCls, "<init>", "(IIILjava/lang/String;Ljava/lang/String;J)V");
    VerifyOrExit(constructor != NULL, err = WDM_JNI_ERROR_METHOD_NOT_FOUND);
    errStr = nl::StatusReportStr(devStatus.StatusProfileId, devStatus.StatusCode);
    errStrObj = (errStr != NULL) ? env->NewStringUTF(errStr) : NULL;
    pathStrObj = (path != NULL) ? env->NewStringUTF(path) : NULL;

    env->ExceptionClear();
    outEx = (jthrowable)env->NewObject(sWdmClientFlushUpdateDeviceExceptionCls, constructor, (jint)devStatus.StatusCode,
            (jint)devStatus.StatusProfileId, (jint)devStatus.SystemErrorCode, errStrObj, pathStrObj, (jlong)dataSink);
    VerifyOrExit(!env->ExceptionCheck(), err = WDM_JNI_ERROR_EXCEPTION_THROWN);

exit:
    env->DeleteLocalRef(errStrObj);
    env->DeleteLocalRef(pathStrObj);
    return err;
}

WEAVE_ERROR J2N_ResourceIdentifier(JNIEnv *env, jobject inResourceIdentifier, ResourceIdentifier & outResourceIdentifier)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jlong longVal = 0;
    int intVal;

    err = J2N_IntFieldVal(env, inResourceIdentifier, "resourceType", intVal);
    SuccessOrExit(err);

    err = J2N_LongFieldVal(env, inResourceIdentifier, "resourceId", longVal);
    SuccessOrExit(err);

    outResourceIdentifier = ResourceIdentifier(intVal, (uint64_t) longVal);
exit:
    return err;
}

WEAVE_ERROR GetClassRef(JNIEnv *env, const char *clsType, jclass& outCls)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jclass cls = NULL;

    cls = env->FindClass(clsType);
    VerifyOrExit(cls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    outCls = (jclass)env->NewGlobalRef((jobject)cls);
    VerifyOrExit(outCls != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

exit:
    env->DeleteLocalRef(cls);
    return err;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
void EngineEventCallback(void * const aAppState,
                                SubscriptionEngine::EventID aEvent,
                                const SubscriptionEngine::InEventParam & aInParam, SubscriptionEngine::OutEventParam & aOutParam)
{
    switch (aEvent)
    {
        default:
            SubscriptionEngine::DefaultEventHandler(aEvent, aInParam, aOutParam);
            break;
    }
}

void Java_nl_Weave_DataManagement_WdmClientImpl_init(JNIEnv *env, jobject self)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = SubscriptionEngine::GetInstance()->Init(&sExchangeMgr, NULL, EngineEventCallback);
    SuccessOrExit(err);

exit:
    return;
}

void BindingEventCallback (void * const apAppState, const nl::Weave::Binding::EventType aEvent,
                                  const nl::Weave::Binding::InEventParam & aInParam, nl::Weave::Binding::OutEventParam & aOutParam)
{
    WeaveLogDetail(DeviceManager, "%s: Event(%d)", __func__, aEvent);
    switch (aEvent)
    {
        case nl::Weave::Binding::kEvent_PrepareRequested:
            WeaveLogDetail(DeviceManager, "kEvent_PrepareRequested");
            break;

        case nl::Weave::Binding::kEvent_PrepareFailed:
            WeaveLogDetail(DeviceManager, "kEvent_PrepareFailed: reason %s", ::nl::ErrorStr(aInParam.PrepareFailed.Reason));
            break;

        case nl::Weave::Binding::kEvent_BindingFailed:
            WeaveLogDetail(DeviceManager, "kEvent_BindingFailed: reason %s", ::nl::ErrorStr(aInParam.PrepareFailed.Reason));
            break;

        case nl::Weave::Binding::kEvent_BindingReady:
            WeaveLogDetail(DeviceManager, "kEvent_BindingReady");
            break;

        case nl::Weave::Binding::kEvent_DefaultCheck:
            WeaveLogDetail(DeviceManager, "kEvent_DefaultCheck");
            // fall through
        default:
            nl::Weave::Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
    }
}

jlong Java_nl_Weave_DataManagement_WdmClientImpl_newWdmClient(JNIEnv *env, jobject self, jlong deviceMgrPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    long res = 0;
    WdmClient *wdmClient = NULL;
    Binding * pBinding = NULL;
    WeaveDeviceManager *deviceMgr = (WeaveDeviceManager *)deviceMgrPtr;

    WeaveLogProgress(DeviceManager, "NewWdmClient() called");

    pBinding = sExchangeMgr.NewBinding(BindingEventCallback, deviceMgr);
    VerifyOrExit(NULL != pBinding, err = WEAVE_ERROR_NO_MEMORY);

    err = deviceMgr->ConfigureBinding(pBinding);
    SuccessOrExit(err);

    wdmClient = new WdmClient();
    VerifyOrExit(wdmClient != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = wdmClient->Init(&MessageLayer, pBinding);
    SuccessOrExit(err);

    wdmClient->mpAppState = (void *)env->NewGlobalRef(self);

    res = (long)wdmClient;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (wdmClient != NULL)
        {
            if (wdmClient->mpAppState != NULL)
            {
                env->DeleteGlobalRef((jobject)wdmClient->mpAppState);
            }
            wdmClient->Close();
            delete wdmClient;
        }
    }

    if (NULL != pBinding)
    {
        pBinding->Release();
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return res;
}

void Java_nl_Weave_DataManagement_WdmClientImpl_deleteWdmClient(JNIEnv *env, jobject self, jlong wdmClientPtr)
{
    WdmClient *wdmClient = (WdmClient *)wdmClientPtr;

    WeaveLogProgress(DeviceManager, "DeleteWdmClient() called");

    if (wdmClient != NULL)
    {
        if (wdmClient->mpAppState != NULL)
        {
            env->DeleteGlobalRef((jobject)wdmClient->mpAppState);
        }
        wdmClient->Close();
        delete wdmClient;
    }
}

void Java_nl_Weave_DataManagement_WdmClientImpl_setNodeId(JNIEnv *env, jobject self, jlong wdmClientPtr, jlong nodeId)
{
    WdmClient *wdmClient = (WdmClient *)wdmClientPtr;
    WeaveLogProgress(DeviceManager, "setNodeId() called");
    wdmClient->SetNodeId((uint64_t)nodeId);
}

jlong Java_nl_Weave_DataManagement_WdmClientImpl_newDataSink(JNIEnv *env, jobject self, jlong wdmClientPtr, jobject resourceIdentifierObj, jlong profileId, jlong instanceId, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    long res = 0;
    const char *pPathStr = NULL;
    ResourceIdentifier resourceIdentifier;
    GenericTraitUpdatableDataSink * dataSink = NULL;
    WdmClient *wdmClient = (WdmClient *)wdmClientPtr;

    WeaveLogProgress(DeviceManager, "newDataSink() called");

    err = J2N_ResourceIdentifier(env, resourceIdentifierObj, resourceIdentifier);
    SuccessOrExit(err);

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = wdmClient->NewDataSink(resourceIdentifier, (uint32_t)profileId, (uint64_t)instanceId, pPathStr, dataSink);

exit:
    if (err == WEAVE_NO_ERROR && dataSink != NULL)
    {
        res = (long) dataSink;
    }

    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return res;
}

void Java_nl_Weave_DataManagement_WdmClientImpl_beginFlushUpdate(JNIEnv *env, jobject self, jlong wdmClientPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WdmClient *wdmClient = (WdmClient *)wdmClientPtr;

    WeaveLogProgress(DeviceManager, "beginFlushUpdate() called");

    pthread_mutex_lock(&sStackLock);
    err = wdmClient->FlushUpdate((void *)"FlushUpdate", HandleWdmClientFlushUpdateComplete, HandleWdmClientError);
    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_WdmClientImpl_beginRefreshData(JNIEnv *env, jobject self, jlong wdmClientPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WdmClient *wdmClient = (WdmClient *)wdmClientPtr;

    WeaveLogProgress(DeviceManager, "beginRefreshData() called");

    pthread_mutex_lock(&sStackLock);
    err = wdmClient->RefreshData((void *)"RefreshData", HandleWdmClientComplete, HandleWdmClientError, NULL);
    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_init(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr)
{
    GenericTraitUpdatableDataSink *pDataSink = NULL;
    pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "Init() called");

    pDataSink->mpAppState = (void *)env->NewGlobalRef(self);

}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_shutdown(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr)
{
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "shutdown() called");

    if (pDataSink != NULL)
    {
        if (pDataSink->mpAppState != NULL)
        {
            env->DeleteGlobalRef((jobject)pDataSink->mpAppState);
        }
        pDataSink->Clear();
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_clear(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr)
{
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "clear() called");

    if (pDataSink != NULL)
    {
        pDataSink->Clear();
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_beginRefreshData(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "beginRefreshData() called");

    pthread_mutex_lock(&sStackLock);
    err = pDataSink->RefreshData((void *)"RefreshData", HandleGenericTraitUpdatableDataSinkComplete, HandleGenericTraitUpdatableDataSinkError);
    pthread_mutex_unlock(&sStackLock);

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setInt(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jlong value, jboolean isConditional, jboolean isSigned)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;

    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setInt() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (isSigned == JNI_TRUE)
    {
        int64_t signedValue = (int64_t)value;
        err = pDataSink->SetData(pPathStr, signedValue, isConditional == JNI_TRUE);
    }
    else
    {
        uint64_t unsignedValue = (uint64_t)value;
        err = pDataSink->SetData(pPathStr, unsignedValue, isConditional == JNI_TRUE);
    }

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setDouble(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jdouble value, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setDouble() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->SetData(pPathStr, value, isConditional == JNI_TRUE);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setBoolean(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jboolean value, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setBoolean() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->SetBoolean(pPathStr, value == JNI_TRUE, isConditional == JNI_TRUE);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setString(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jstring value, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    const char *pValue = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setString() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    pValue = env->GetStringUTFChars(value, 0);
    VerifyOrExit(pValue != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->SetString(pPathStr, pValue, isConditional == JNI_TRUE);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (pValue != NULL)
    {
        env->ReleaseStringUTFChars(value, pValue);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setNull(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setNull() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->SetNull(pPathStr, isConditional == JNI_TRUE);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setBytes(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jbyteArray value, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    const uint8_t *pDataBuf = NULL;
    uint32_t dataLen;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setBytes() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    dataLen = env->GetArrayLength(value);
    pDataBuf = (const uint8_t *)env->GetByteArrayElements(value, NULL);

    err = pDataSink->SetBytes(pPathStr, pDataBuf, dataLen, isConditional == JNI_TRUE);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (pDataBuf != NULL)
    {
        env->ReleaseByteArrayElements(value, (jbyte *)pDataBuf, JNI_ABORT);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_setStringArray(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path, jobjectArray stringArray, jboolean isConditional)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    jstring jst = NULL;
    std::vector<std::string> stringVector;
    int stringCount = env->GetArrayLength(stringArray);
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "setStringArray() called");
    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    for (int i=0; i < stringCount; i++)
    {
        std::string val;
        jst = (jstring) (env->GetObjectArrayElement(stringArray, i));
        err = J2N_StdString(env, jst, val);
        SuccessOrExit(err);
        stringVector.push_back(val);
    }

    err = pDataSink->SetStringArray(pPathStr, stringVector, isConditional == JNI_TRUE);

exit:
    env->DeleteLocalRef(jst);

    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

jlong Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getLong(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t value = 0;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getLong() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetData(pPathStr, value);
    SuccessOrExit(err);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return (int64_t)value;

}

jdouble Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getDouble(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    double value = 0;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getDouble() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetData(pPathStr, value);
    SuccessOrExit(err);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return value;
}

jboolean Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getBoolean(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool value = true;
    jboolean jValue = JNI_TRUE;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getBoolean() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetBoolean(pPathStr, value);
    SuccessOrExit(err);

    if (!value)
    {
        jValue = JNI_FALSE;
    }

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return jValue;
}

jstring Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getString(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    BytesData bytesData;
    std::string valueStr;

    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getString() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetString(pPathStr, &bytesData);
    SuccessOrExit(err);

    valueStr = std::string((char *)(bytesData.mpDataBuf), bytesData.mDataLen);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return env->NewStringUTF(valueStr.c_str());
}

jbyteArray Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getBytes(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    jbyteArray value = NULL;
    const char *pPathStr = NULL;
    BytesData bytesData;

    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getByte() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetBytes(pPathStr, &bytesData);
    SuccessOrExit(err);

    err = N2J_ByteArray(env, bytesData.mpDataBuf, bytesData.mDataLen, value);
    SuccessOrExit(err);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return value;
}

jobjectArray Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getStringArray(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    jobjectArray array = NULL;
    jclass java_lang_String = NULL;
    std::vector<std::string> stringVector;

    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "getStringArray() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->GetStringArray(pPathStr, stringVector);
    SuccessOrExit(err);

    java_lang_String = env->FindClass("java/lang/String");
    VerifyOrExit(java_lang_String != NULL, err = WDM_JNI_ERROR_TYPE_NOT_FOUND);

    array = env->NewObjectArray(stringVector.size(), java_lang_String, NULL);

    for (size_t i = 0; i < stringVector.size(); ++i)
    {
        jstring obj = env->NewStringUTF(stringVector[i].c_str());
        env->SetObjectArrayElement(array, i, obj);
        env->DeleteLocalRef(obj);
        obj = NULL;
    }

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return array;
}

jboolean Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_isNull(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;
    bool isNull = false;
    const char *pPathStr = NULL;

    WeaveLogProgress(DeviceManager, "isNull() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->IsNull(pPathStr, isNull);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }

    return isNull ? JNI_TRUE : JNI_FALSE;
}

NL_DLL_EXPORT jlong Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_getVersion(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr)
{
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;
    uint64_t version = 0;

    WeaveLogProgress(DeviceManager, "getVersion() called");

    version = pDataSink->GetVersion();

    return (jlong)version;
}

NL_DLL_EXPORT void Java_nl_Weave_DataManagement_GenericTraitUpdatableDataSinkImpl_deleteData(JNIEnv *env, jobject self, jlong genericTraitUpdatableDataSinkPtr, jstring path)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char *pPathStr = NULL;
    GenericTraitUpdatableDataSink *pDataSink = (GenericTraitUpdatableDataSink *)genericTraitUpdatableDataSinkPtr;

    WeaveLogProgress(DeviceManager, "deleteData() called");

    pPathStr = env->GetStringUTFChars(path, 0);
    VerifyOrExit(pPathStr != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = pDataSink->DeleteData(pPathStr);

exit:
    if (pPathStr != NULL)
    {
        env->ReleaseStringUTFChars(path, pPathStr);
    }

    if (err != WEAVE_NO_ERROR && err != WDM_JNI_ERROR_EXCEPTION_THROWN)
    {
        ThrowError(env, err);
    }
}

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    static nl::Weave::Profiles::DataManagement::SubscriptionEngine sWdmSubscriptionEngine;
    return &sWdmSubscriptionEngine;
}

namespace Platform {
void CriticalSectionEnter()
{
    return;
}

void CriticalSectionExit()
{
    return;
}

} // Platform

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

/*
* Dummy implementations of PersistedStorage platform methods. These aren't
* used in the context of the Java DeviceManager, but are required to satisfy
* the linker.
*/

WEAVE_ERROR Read(const char *aKey, uint32_t &aValue)
{
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR Write(const char *aKey, uint32_t aValue)
{
    return WEAVE_NO_ERROR;
}

} // PersistentStorage
} // Platform
} // Weave
} // nl

/*

    Copyright (c) 2013-2018 Nest Labs, Inc.
    Copyright (c) 2019 Google, LLC.
    All rights reserved.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */
package nl.Weave.DeviceManager;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.os.Build;
import android.util.Log;

import java.util.EnumSet;
import java.util.Random;

public class WeaveDeviceManager
{
    // Values taken from WeaveLogging::LogCategory
    private static final int LOG_CATEGORY_NONE = 0;
    private static final int LOG_CATEGORY_MAX = 3;

    /**
     * Enable or disable logging output
     *
     * @param enabled true if logs should be printed; false for no output
     */
    public static void setLoggingEnabled(boolean enabled) {
        setLogFilter(enabled ? LOG_CATEGORY_MAX : LOG_CATEGORY_NONE);
    }

    public WeaveDeviceManager()
    {
        mDeviceMgrPtr = newDeviceManager();
        mCompHandler = null;
    }

    public BluetoothGattCallback getCallback()
    {
        return WeaveStack.getInstance().getCallback();
    }

    public BluetoothGatt getBluetoothGatt()
    {
        return mBluetoothGatt;
    }

    public void beginConnectBle(BluetoothGatt server, boolean autoClose)
    {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			if (mConnId == 0)
			{
				mBluetoothGatt = server;
				mConnId = WeaveStack.getInstance().addConnection(this);
				if (mConnId == 0)
				{
					Log.e(TAG, "Unable to add BLE connection.");
					mCompHandler.onError(new Exception("Unable to add BLE connection."));
					return;
				}
				Log.d(TAG, "BLE connection assigned id " + mConnId);
				beginConnectBleNoAuth(mDeviceMgrPtr, mConnId, autoClose);
			}
			else
			{
				Log.e(TAG, "BLE connection already in use");
				mCompHandler.onError(new Exception("BLE connection already in use"));
				return;
			}
		}
		else
		{
			Log.e(TAG, "BLE not supported on device.");
			mCompHandler.onError(new Exception("BLE not supported on device."));
			return;
		}
    }

    public void beginConnectBle(BluetoothGatt server, boolean autoClose, String pairingCode)
    {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			if (mConnId == 0)
			{
				mBluetoothGatt = server;
				mConnId = WeaveStack.getInstance().addConnection(this);
				if (mConnId == 0)
				{
					Log.e(TAG, "Unable to add BLE connection.");
					mCompHandler.onError(new Exception("Unable to add BLE connection."));
					return;
				}
				Log.d(TAG, "BLE connection assigned id " + mConnId);
				beginConnectBlePairingCode(mDeviceMgrPtr, mConnId, autoClose, pairingCode);
			}
			else
			{
				Log.e(TAG, "BLE connection already in use");
				mCompHandler.onError(new Exception("BLE connection already in use"));
				return;
			}
		}
		else
		{
			Log.e(TAG, "BLE not supported on device.");
			mCompHandler.onError(new Exception("BLE not supported on device."));
			return;
		}
    }

    public void beginConnectBle(BluetoothGatt server, boolean autoClose, byte[] accessToken)
    {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
		{
			if (mConnId == 0)
			{
				mBluetoothGatt = server;
				mConnId = WeaveStack.getInstance().addConnection(this);
				if (mConnId == 0)
				{
					Log.e(TAG, "Unable to add BLE connection.");
					mCompHandler.onError(new Exception("Unable to add BLE connection."));
					return;
				}
				Log.d(TAG, "BLE connection assigned id " + mConnId);
				beginConnectBleAccessToken(mDeviceMgrPtr, mConnId, autoClose, accessToken);
			}
			else
			{
				Log.e(TAG, "BLE connection already in use");
				mCompHandler.onError(new Exception("BLE connection already in use"));
				return;
			}
		}
		else
		{
			Log.e(TAG, "BLE not supported on device.");
			mCompHandler.onError(new Exception("BLE not supported on device."));
			return;
		}
    }

    public void close()
    {
        close(mDeviceMgrPtr);
        releaseBluetoothGatt(mConnId);
    }

    public boolean isConnected()
    {
        return isConnected(mDeviceMgrPtr);
    }

    public long deviceId()
    {
        return deviceId(mDeviceMgrPtr);
    }

    public String deviceAddress()
    {
        return deviceAddress(mDeviceMgrPtr);
    }

    public CompletionHandler getCompletionHandler()
    {
        return mCompHandler;
    }

    public void setCompletionHandler(CompletionHandler compHandler)
    {
        mCompHandler = compHandler;
    }

    public void beginConnectDevice(long deviceId, String ipAddr)
    {
        beginConnectDeviceNoAuth(mDeviceMgrPtr, deviceId, ipAddr);
    }

    public void beginConnectDevice(long deviceId, String ipAddr, String pairingCode)
    {
        beginConnectDevicePairingCode(mDeviceMgrPtr, deviceId, ipAddr, pairingCode);
    }

    public void beginConnectDevice(long deviceId, String ipAddr, byte[] accessToken)
    {
        beginConnectDeviceAccessToken(mDeviceMgrPtr, deviceId, ipAddr, accessToken);
    }

    public void startDeviceEnumeration(IdentifyDeviceCriteria deviceCriteria)
    {
		startDeviceEnumeration(mDeviceMgrPtr, deviceCriteria);
    }

    public void stopDeviceEnumeration()
    {
        stopDeviceEnumeration(mDeviceMgrPtr);
    }

    public void beginRendezvousDevice(IdentifyDeviceCriteria deviceCriteria)
    {
        beginRendezvousDeviceNoAuth(mDeviceMgrPtr, deviceCriteria);
    }

    public void beginRendezvousDevice(String pairingCode, IdentifyDeviceCriteria deviceCriteria)
    {
        beginRendezvousDevicePairingCode(mDeviceMgrPtr, pairingCode, deviceCriteria);
    }

    public void beginRendezvousDevice(byte[] accessToken, IdentifyDeviceCriteria deviceCriteria)
    {
        beginRendezvousDeviceAccessToken(mDeviceMgrPtr, accessToken, deviceCriteria);
    }

    public void beginRemotePassiveRendezvous(String rendezvousAddress,
            int rendezvousTimeoutSec, int inactivityTimeoutSec)
    {
        beginRemotePassiveRendezvousNoAuth(mDeviceMgrPtr, rendezvousAddress,
                rendezvousTimeoutSec, inactivityTimeoutSec);
    }

    public void beginRemotePassiveRendezvous(String pairingCode, String rendezvousAddress,
            int rendezvousTimeoutSec, int inactivityTimeoutSec)
    {
        beginRemotePassiveRendezvousPairingCode(mDeviceMgrPtr, pairingCode, rendezvousAddress,
                rendezvousTimeoutSec, inactivityTimeoutSec);
    }

    public void beginRemotePassiveRendezvous(byte[] accessToken, String rendezvousAddress,
            int rendezvousTimeoutSec, int inactivityTimeoutSec)
    {
        beginRemotePassiveRendezvousAccessToken(mDeviceMgrPtr, accessToken, rendezvousAddress,
                rendezvousTimeoutSec, inactivityTimeoutSec);
    }

    public void beginReconnectDevice()
    {
        beginReconnectDevice(mDeviceMgrPtr);
    }

    public void beginIdentifyDevice()
    {
        beginIdentifyDevice(mDeviceMgrPtr);
    }

    public void beginScanNetworks(NetworkType netType)
    {
        beginScanNetworks(mDeviceMgrPtr, netType.val);
    }

    public void beginGetNetworks(GetNetworkFlags getFlags)
    {
        beginGetNetworks(mDeviceMgrPtr, getFlags.val);
    }

    public void beginGetCameraAuthData(String nonce)
    {
        beginGetCameraAuthData(mDeviceMgrPtr, nonce);
    }

    public void beginAddNetwork(NetworkInfo netInfo)
    {
        beginAddNetwork(mDeviceMgrPtr, netInfo);
    }

    public void beginUpdateNetwork(NetworkInfo netInfo)
    {
        beginUpdateNetwork(mDeviceMgrPtr, netInfo);
    }

    public void beginRemoveNetwork(long networkId)
    {
        beginRemoveNetwork(mDeviceMgrPtr, networkId);
    }

    public void beginEnableNetwork(long networkId)
    {
        beginEnableNetwork(mDeviceMgrPtr, networkId);
    }

    public void beginDisableNetwork(long networkId)
    {
        beginDisableNetwork(mDeviceMgrPtr, networkId);
    }

    public void beginTestNetworkConnectivity(long networkId)
    {
        beginTestNetworkConnectivity(mDeviceMgrPtr, networkId);
    }

    public void beginGetRendezvousMode()
    {
        beginGetRendezvousMode(mDeviceMgrPtr);
    }

    public void beginSetRendezvousMode(EnumSet<RendezvousMode> modeSet)
    {
        beginSetRendezvousMode(mDeviceMgrPtr, RendezvousMode.toFlags(modeSet));
    }

    public void beginSetRendezvousMode(int modeFlags)
    {
        beginSetRendezvousMode(mDeviceMgrPtr, modeFlags);
    }

    public void beginGetLastNetworkProvisioningResult()
    {
        beginGetLastNetworkProvisioningResult(mDeviceMgrPtr);
    }

    public void beginRegisterServicePairAccount(long serviceId, String accountId, byte[] serviceConfig, String pairingToken, String pairingInitData)
    {
        beginRegisterServicePairAccount(mDeviceMgrPtr, serviceId, accountId, serviceConfig, pairingToken.getBytes(), pairingInitData.getBytes());
    }

    public void beginUnregisterService(long serviceId)
    {
        beginUnregisterService(mDeviceMgrPtr, serviceId);
    }

    public void beginCreateFabric()
    {
        beginCreateFabric(mDeviceMgrPtr);
    }

    public void beginLeaveFabric()
    {
        beginLeaveFabric(mDeviceMgrPtr);
    }

    public void beginGetFabricConfig()
    {
        beginGetFabricConfig(mDeviceMgrPtr);
    }

    public void beginJoinExistingFabric(byte[] fabricConfig)
    {
        beginJoinExistingFabric(mDeviceMgrPtr, fabricConfig);
    }

    public void beginArmFailSafe(FailSafeArmMode armMode, int failSafeToken)
    {
        beginArmFailSafe(mDeviceMgrPtr, armMode.val, failSafeToken);
    }

    public int beginArmFailSafe(FailSafeArmMode armMode)
    {
        int failSafeToken = new Random().nextInt();
        beginArmFailSafe(mDeviceMgrPtr, armMode.val, failSafeToken);
        return failSafeToken;
    }

    public void beginDisarmFailSafe()
    {
        beginDisarmFailSafe(mDeviceMgrPtr);
    }

    public void beginStartSystemTest(long profileId, long testId)
    {
        beginStartSystemTest(mDeviceMgrPtr, profileId, testId);
    }

    public void beginStopSystemTest()
    {
        beginStopSystemTest(mDeviceMgrPtr);
    }

    public void beginResetConfig(ResetFlags resetFlags)
    {
        beginResetConfig(mDeviceMgrPtr, resetFlags.val);
    }

    public void beginPing()
    {
        beginPing(mDeviceMgrPtr);
    }

    public void beginPing(int payloadSize)
    {
        beginPingWithSize(mDeviceMgrPtr, payloadSize);
    }

    public void beginEnableConnectionMonitor(int intervalMS, int timeoutMS)
    {
        beginEnableConnectionMonitor(mDeviceMgrPtr, intervalMS, timeoutMS);
    }

    public void beginDisableConnectionMonitor()
    {
        beginDisableConnectionMonitor(mDeviceMgrPtr);
    }

    public void beginPairToken(byte[] pairingToken)
    {
        beginPairToken(mDeviceMgrPtr, pairingToken);
    }

    public void beginUnpairToken()
    {
        beginUnpairToken(mDeviceMgrPtr);
    }

    public void setRendezvousAddress(String rendezvousAddr)
    {
        setRendezvousAddress(mDeviceMgrPtr, rendezvousAddr);
    }

    public void setAutoReconnect(boolean autoReconnect)
    {
        setAutoReconnect(mDeviceMgrPtr, autoReconnect);
    }

    public void setRendezvousLinkLocal(boolean rendezvousLinkLocal)
    {
        setRendezvousLinkLocal(mDeviceMgrPtr, rendezvousLinkLocal);
    }

    public void setConnectTimeout(int timeoutMS)
    {
        setConnectTimeout(mDeviceMgrPtr, timeoutMS);
    }

    public WeaveDeviceDescriptor decodeDeviceDescriptor(byte[] encodedDeviceDesc)
    {
        return WeaveDeviceDescriptor.decode(encodedDeviceDesc);
    }

    public void onConnectDeviceComplete()
    {
        mCompHandler.onConnectDeviceComplete();
    }

    public void onRendezvousDeviceComplete()
    {
        mCompHandler.onRendezvousDeviceComplete();
    }

    public void onRemotePassiveRendezvousComplete()
    {
        mCompHandler.onRemotePassiveRendezvousComplete();
    }

    public void onReconnectDeviceComplete()
    {
        mCompHandler.onReconnectDeviceComplete();
    }

    public void onIdentifyDeviceComplete(WeaveDeviceDescriptor deviceDesc)
    {
        mCompHandler.onIdentifyDeviceComplete(deviceDesc);
    }

    public void onScanNetworksComplete(NetworkInfo[] networks)
    {
        mCompHandler.onScanNetworksComplete(networks);
    }

    public void onGetNetworksComplete(NetworkInfo[] networks)
    {
        mCompHandler.onGetNetworksComplete(networks);
    }

    public void onGetCameraAuthDataComplete(String macAddress, String authData)
    {
        mCompHandler.onGetCameraAuthDataComplete(macAddress, authData);
    }

    public void onAddNetworkComplete(long networkId)
    {
        mCompHandler.onAddNetworkComplete(networkId);
    }

    public void onUpdateNetworkComplete()
    {
        mCompHandler.onUpdateNetworkComplete();
    }

    public void onRemoveNetworkComplete()
    {
        mCompHandler.onRemoveNetworkComplete();
    }

    public void onEnableNetworkComplete()
    {
        mCompHandler.onEnableNetworkComplete();
    }

    public void onDisableNetworkComplete()
    {
        mCompHandler.onDisableNetworkComplete();
    }

    public void onTestNetworkConnectivityComplete()
    {
        mCompHandler.onTestNetworkConnectivityComplete();
    }

    public void onGetRendezvousModeComplete(EnumSet<RendezvousMode> rendezvousModes)
    {
        mCompHandler.onGetRendezvousModeComplete(rendezvousModes);
    }

    public void onSetRendezvousModeComplete()
    {
        mCompHandler.onSetRendezvousModeComplete();
    }

    public void onGetLastNetworkProvisioningResultComplete()
    {
        mCompHandler.onGetLastNetworkProvisioningResultComplete();
    }

    public void onRegisterServicePairAccountComplete()
    {
        mCompHandler.onRegisterServicePairAccountComplete();
    }

    public void onUnregisterServiceComplete()
    {
        mCompHandler.onUnregisterServiceComplete();
    }

    public void onCreateFabricComplete()
    {
        mCompHandler.onCreateFabricComplete();
    }

    public void onLeaveFabricComplete()
    {
        mCompHandler.onLeaveFabricComplete();
    }

    public void onGetFabricConfigComplete(byte[] fabricConfig)
    {
        mCompHandler.onGetFabricConfigComplete(fabricConfig);
    }

    public void onJoinExistingFabricComplete()
    {
        mCompHandler.onJoinExistingFabricComplete();
    }

    public void onArmFailSafeComplete()
    {
        mCompHandler.onArmFailSafeComplete();
    }

    public void onDisarmFailSafeComplete()
    {
        mCompHandler.onDisarmFailSafeComplete();
    }

    public void onStartSystemTestComplete()
    {
        mCompHandler.onStartSystemTestComplete();
    }

    public void onStopSystemTestComplete()
    {
        mCompHandler.onStopSystemTestComplete();
    }

    public void onResetConfigComplete()
    {
        mCompHandler.onResetConfigComplete();
    }

    public void onPingComplete()
    {
        mCompHandler.onPingComplete();
    }

    public void onEnableConnectionMonitorComplete()
    {
        mCompHandler.onEnableConnectionMonitorComplete();
    }

    public void onDisableConnectionMonitorComplete()
    {
        mCompHandler.onDisableConnectionMonitorComplete();
    }

    public void onPairTokenComplete(byte[] pairingTokenBundle)
    {
        mCompHandler.onPairTokenComplete(pairingTokenBundle);
    }

    public void onUnpairTokenComplete()
    {
        mCompHandler.onUnpairTokenComplete();
    }

    public void onConnectBleComplete()
    {
        mCompHandler.onConnectBleComplete();
    }

    public void onNotifyWeaveConnectionClosed(int connId)
    {
        // clear connection state.
        WeaveStack.getInstance().removeConnection(connId);
        mConnId = 0;
        mBluetoothGatt = null;

        Log.d(TAG, "calling onNotifyWeaveConnectionClosed()");
        mCompHandler.onNotifyWeaveConnectionClosed();
    }

    public void onCloseBleComplete(int connId)
    {
        if (releaseBluetoothGatt(connId)) {
            Log.d(TAG, "calling onCloseBleComplete()");
            mCompHandler.onCloseBleComplete();
        } else {
            Log.d(TAG, "not calling onCloseBleComplete() as the connection has already been closed.");
        }
    }

    public void onError(Throwable err)
    {
        mCompHandler.onError(err);
    }

    public void onDeviceEnumerationResponse(WeaveDeviceDescriptor deviceDesc, String deviceAddr)
    {
        mCompHandler.onDeviceEnumerationResponse(deviceDesc, deviceAddr);
    }

    public long getDeviceMgrPtr()
    {
        return mDeviceMgrPtr;
    }

    public interface CompletionHandler
    {
        void onConnectDeviceComplete();
        void onRendezvousDeviceComplete();
        void onRemotePassiveRendezvousComplete();
        void onReconnectDeviceComplete();
        void onIdentifyDeviceComplete(WeaveDeviceDescriptor deviceDesc);
        void onScanNetworksComplete(NetworkInfo[] networks);
        void onGetNetworksComplete(NetworkInfo[] networks);
        void onGetCameraAuthDataComplete(String macAddress, String authData);
        void onAddNetworkComplete(long networkId);
        void onUpdateNetworkComplete();
        void onRemoveNetworkComplete();
        void onEnableNetworkComplete();
        void onDisableNetworkComplete();
        void onTestNetworkConnectivityComplete();
        void onGetRendezvousModeComplete(EnumSet<RendezvousMode> rendezvousModes);
        void onSetRendezvousModeComplete();
        void onGetLastNetworkProvisioningResultComplete();
        void onRegisterServicePairAccountComplete();
        void onUnregisterServiceComplete();
        void onCreateFabricComplete();
        void onLeaveFabricComplete();
        void onGetFabricConfigComplete(byte[] fabricConfig);
        void onJoinExistingFabricComplete();
        void onArmFailSafeComplete();
        void onDisarmFailSafeComplete();
        void onResetConfigComplete();
        void onPingComplete();
        void onEnableConnectionMonitorComplete();
        void onDisableConnectionMonitorComplete();
        void onConnectBleComplete();
        void onPairTokenComplete(byte[] pairingTokenBundle);
        void onUnpairTokenComplete();
        void onCloseBleComplete();
        void onNotifyWeaveConnectionClosed();
        void onStartSystemTestComplete();
        void onStopSystemTestComplete();
        void onError(Throwable err);
        void onDeviceEnumerationResponse(WeaveDeviceDescriptor deviceDesc, String deviceAddr);
    }

    public static native boolean isValidPairingCode(String pairingCode);

    public static native void closeEndpoints();

    private static native void setLogFilter(int logLevel);


    // ----- Protected Members -----

    private CompletionHandler mCompHandler;

    protected void finalize() throws Throwable
    {
        super.finalize();
        if (mDeviceMgrPtr != 0)
        {
            deleteDeviceManager(mDeviceMgrPtr);
            mDeviceMgrPtr = 0;
        }
    }

    // ----- Private Members -----

    private long mDeviceMgrPtr;
    private final static String TAG = WeaveDeviceManager.class.getSimpleName();

    private BluetoothGatt mBluetoothGatt;
    private int mConnId = 0;

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private boolean releaseBluetoothGatt(int connId) {
        if (mConnId != 0) {
            Log.d(TAG, "Closing GATT and removing connection for " + connId);

            // Close gatt
            mBluetoothGatt.close();

            // clear connection state.
            WeaveStack.getInstance().removeConnection(connId);
            mConnId = 0;
            mBluetoothGatt = null;
            return true;
        } else {
            return false;
        }
    }

    private native long newDeviceManager();
    private native void deleteDeviceManager(long deviceMgrPtr);
    private native void beginConnectBleNoAuth(long deviceMgrPtr, int connId, boolean autoClose);
    private native void beginConnectBlePairingCode(long deviceMgrPtr, int connId, boolean autoClose, String pairingCode);
    private native void beginConnectBleAccessToken(long deviceMgrPtr, int connId, boolean autoClose, byte[] accessToken);
    private native void beginConnectDeviceNoAuth(long deviceMgrPtr, long deviceId, String ipAddr);
    private native void beginConnectDevicePairingCode(long deviceMgrPtr, long deviceId, String ipAddr, String pairingCode);
    private native void beginConnectDeviceAccessToken(long deviceMgrPtr, long deviceId, String ipAddr, byte[] accessToken);
    private native void startDeviceEnumeration(long deviceMgrPtr, IdentifyDeviceCriteria deviceCriteria);
    private native void stopDeviceEnumeration(long deviceMgrPtr);
    private native void beginRendezvousDeviceNoAuth(long deviceMgrPtr, IdentifyDeviceCriteria deviceCriteria);
    private native void beginRendezvousDevicePairingCode(long deviceMgrPtr, String pairingCode, IdentifyDeviceCriteria deviceCriteria);
    private native void beginRendezvousDeviceAccessToken(long deviceMgrPtr, byte[] accessToken, IdentifyDeviceCriteria deviceCriteria);
    private native void beginRemotePassiveRendezvousNoAuth(long deviceMgrPtr, String rendezvousAddress, int rendezvousTimeoutSec, int inactivityTimeoutSec);
    private native void beginRemotePassiveRendezvousPairingCode(long deviceMgrPtr, String pairingCode, String rendezvousAddress, int rendezvousTimeoutSec, int inactivityTimeoutSec);
    private native void beginRemotePassiveRendezvousAccessToken(long deviceMgrPtr, byte[] accessToken, String rendezvousAddress, int rendezvousTimeoutSec, int inactivityTimeoutSec);
    private native void beginReconnectDevice(long deviceMgrPtr);
    private native void beginIdentifyDevice(long deviceMgrPtr);
    private native void beginScanNetworks(long deviceMgrPtr, int netType);
    private native void beginGetNetworks(long deviceMgrPtr, int getFlags);
    private native void beginGetCameraAuthData(long deviceMgrPtr, String nonce);
    private native void beginAddNetwork(long deviceMgrPtr, NetworkInfo netInfo);
    private native void beginUpdateNetwork(long deviceMgrPtr, NetworkInfo netInfo);
    private native void beginRemoveNetwork(long deviceMgrPtr, long networkId);
    private native void beginEnableNetwork(long deviceMgrPtr, long networkId);
    private native void beginDisableNetwork(long deviceMgrPtr, long networkId);
    private native void beginTestNetworkConnectivity(long deviceMgrPtr, long networkId);
    private native void beginGetRendezvousMode(long deviceMgrPtr);
    private native void beginSetRendezvousMode(long deviceMgrPtr, int modeFlags);
    private native void beginGetLastNetworkProvisioningResult(long deviceMgrPtr);
    private native void beginRegisterServicePairAccount(long deviceMgrPtr, long serviceId, String accountId, byte[] serviceConfig, byte[] pairingToken, byte[] pairingInitData);
    private native void beginUnregisterService(long deviceMgrPtr, long serviceId);
    private native void beginCreateFabric(long deviceMgrPtr);
    private native void beginLeaveFabric(long deviceMgrPtr);
    private native void beginGetFabricConfig(long deviceMgrPtr);
    private native void beginJoinExistingFabric(long deviceMgrPtr, byte[] fabricConfig);
    private native void beginArmFailSafe(long deviceMgrPtr, int armMode, int failSafeToken);
    private native void beginDisarmFailSafe(long deviceMgrPtr);
    private native void beginStartSystemTest(long deviceMgrPtr, long profileId, long testId);
    private native void beginStopSystemTest(long deviceMgrPtr);
    private native void beginResetConfig(long deviceMgrPtr, int resetFlags);
    private native void beginPing(long deviceMgrPtr);
    private native void beginPingWithSize(long deviceMgrPtr, int payloadSize);
    private native void beginEnableConnectionMonitor(long deviceMgrPtr, int intervalMS, int timeoutMS);
    private native void beginDisableConnectionMonitor(long deviceMgrPtr);
    private native void beginPairToken(long deviceMgrPtr, byte[] pairingToken);
    private native void beginUnpairToken(long deviceMgrPtr);
    private native void close(long deviceMgrPtr);
    private native boolean isConnected(long deviceMgrPtr);
    private native long deviceId(long deviceMgrPtr);
    private native String deviceAddress(long deviceMgrPtr);
    private native void setRendezvousAddress(long deviceMgrPtr, String rendezvousAddr);
    private native void setAutoReconnect(long deviceMgrPtr, boolean autoReconnect);
    private native void setRendezvousLinkLocal(long deviceMgrPtr, boolean rendezvousLinkLocal);
    private native void setConnectTimeout(long deviceMgrPtr, int timeoutMS);


    private void onGetRendezvousModeComplete(int modeFlags)
    {
        onGetRendezvousModeComplete(RendezvousMode.fromFlags(modeFlags));
    }
};

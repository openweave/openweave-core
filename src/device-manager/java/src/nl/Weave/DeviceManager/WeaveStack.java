/*
 *
 *    Copyright (c) 013-2017 Nest Labs, Inc.
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
package nl.Weave.DeviceManager;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.util.Log;

import java.util.ArrayList;
import java.util.UUID;

/*
 * WeaveStack - a singleton object shared between all instances of
 * WeaveDeviceManager.
 *
 * Holds list of active connections with mapping to the associated
 * connection object.
 */
public final class WeaveStack
{
    private static final String TAG = WeaveStack.class.getSimpleName();
    public static final int INITIAL_CONNECTIONS = 4;

    private static class BleMtuBlacklist
    {
	/**
	 * Will be set at initialization to indicate whether the device on which this code is being run
	 * is known to indicate unreliable MTU values for Bluetooth LE connections.
	 */
	static final boolean BLE_MTU_BLACKLISTED;

	/**
	 * If {@link #BLE_MTU_BLACKLISTED} is true, then this is the fallback MTU to use for this device
	 */
	static final int BLE_MTU_FALLBACK = 23;

	static
	{
	    if ("OnePlus".equals(android.os.Build.MANUFACTURER))
	    {
		if ("ONE A2005".equals(android.os.Build.MODEL))
		{
		    BLE_MTU_BLACKLISTED = true;
		}
		else
		{
		    BLE_MTU_BLACKLISTED = false;
		}
	    }
	    else if ("motorola".equals(android.os.Build.MANUFACTURER))
	    {
		if ("XT1575".equals(android.os.Build.MODEL) ||
		    "XT1585".equals(android.os.Build.MODEL))
		{
		    BLE_MTU_BLACKLISTED = true;
                }
		else
		{
		    BLE_MTU_BLACKLISTED = false;
		}
	    }
	    else
	    {
		BLE_MTU_BLACKLISTED = false;
	    }
	}
    }

    /*
     * Singleton instance of this class
     */
    private static final WeaveStack sInstance = new WeaveStack();

    /*
     * Mapping of connections to connection objects
     */
    private final ArrayList<WeaveDeviceManager> mConnections;

    private BluetoothGattCallback mGattCallback;

    private WeaveStack()
    {
	mConnections = new ArrayList<WeaveDeviceManager>(INITIAL_CONNECTIONS);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
        {
            mGattCallback = new BluetoothGattCallback()
            {
		@Override
		public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState)
		{
		    int connId = 0;

		    if (newState == BluetoothProfile.STATE_DISCONNECTED)
		    {
			connId = getConnId(gatt);
			if (connId > 0)
			{
			    Log.d(TAG, "onConnectionStateChange Disconnected");
			    handleConnectionError(connId);
			}
			else
			{
			    Log.e(TAG, "onConnectionStateChange disconnected: no active connection");
			}
		    }
		}

		@Override
		public void onServicesDiscovered(BluetoothGatt gatt, int status)
		{
		}

		@Override
		public void onCharacteristicRead(BluetoothGatt gatt,
						 BluetoothGattCharacteristic characteristic,
						 int status)
		{
		}

		@Override
		public void onCharacteristicWrite(BluetoothGatt gatt,
						  BluetoothGattCharacteristic characteristic,
						  int status)
		{
		    byte[] svcIdBytes = convertUUIDToBytes(characteristic.getService().getUuid());
		    byte[] charIdBytes = convertUUIDToBytes(characteristic.getUuid());

		    if (status != BluetoothGatt.GATT_SUCCESS)
		    {
			Log.e(TAG, "onCharacteristicWrite for " + characteristic.getUuid().toString() + " failed with status: " + status);
			return;
		    }
		    int connId = getConnId(gatt);
		    if (connId > 0)
		    {
			handleWriteConfirmation(connId, svcIdBytes, charIdBytes, status == BluetoothGatt.GATT_SUCCESS);
		    }
		    else
		    {
			Log.e(TAG, "onCharacteristicWrite no active connection");
			return;
		    }
		}

		@Override
		public void onCharacteristicChanged(BluetoothGatt gatt,
						    BluetoothGattCharacteristic characteristic)
		{
		    byte[] svcIdBytes = convertUUIDToBytes(characteristic.getService().getUuid());
		    byte[] charIdBytes = convertUUIDToBytes(characteristic.getUuid());
		    int connId = getConnId(gatt);
		    if (connId > 0)
		    {
                        handleIndicationReceived(connId, svcIdBytes, charIdBytes, characteristic.getValue());
		    }
		    else
		    {
			Log.e(TAG, "onCharacteristicChanged no active connection");
			return;
		    }
		}

		@Override
		public void onDescriptorWrite(BluetoothGatt gatt,
					      BluetoothGattDescriptor descriptor,
					      int status)
		{
		    BluetoothGattCharacteristic characteristic = descriptor.getCharacteristic();

		    byte[] svcIdBytes = convertUUIDToBytes(characteristic.getService().getUuid());
		    byte[] charIdBytes = convertUUIDToBytes(characteristic.getUuid());

		    if (status != BluetoothGatt.GATT_SUCCESS)
		    {
			Log.e(TAG, "onDescriptorWrite for " + descriptor.getUuid().toString() + " failed with status: " + status);
		    }
		    int connId = getConnId(gatt);
		    if (connId == 0)
		    {
			Log.e(TAG, "onDescriptorWrite no active connection");
			return;
		    }
		    if (descriptor.getValue() == BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)
		    {
			handleSubscribeComplete(connId, svcIdBytes, charIdBytes, status == BluetoothGatt.GATT_SUCCESS);
		    }
		    else if (descriptor.getValue() == BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE)
		    {
			handleUnsubscribeComplete(connId, svcIdBytes, charIdBytes, status == BluetoothGatt.GATT_SUCCESS);
		    }
		    else
		    {
			Log.d(TAG, "Unexpected onDescriptorWrite().");
		    }
		}

		@Override
		public void onDescriptorRead(BluetoothGatt gatt,
					     BluetoothGattDescriptor descriptor,
					     int status)
		{
		}
	    };
        }

    }

    public static WeaveStack getInstance()
    {
	return sInstance;
    }

    public BluetoothGattCallback getCallback()
    {
        return mGattCallback;
    }

    public synchronized WeaveDeviceManager getConnection(int connId)
    {
        int connIndex = connId - 1;
        if (connIndex >= 0 && connIndex < mConnections.size())
        {
            return mConnections.get(connIndex);
        }
        else
        {
            Log.e(TAG, "Unknown connId " + connId);
            return null;
        }
    }

    public synchronized int getConnId(BluetoothGatt gatt)
    {
       // Find callback given gatt
       int connIndex = 0;
       while  (connIndex < mConnections.size())
       {
           WeaveDeviceManager devMgr = mConnections.get(connIndex);
           if (devMgr != null)
           {
               if (gatt == devMgr.getBluetoothGatt())
               {
                   return connIndex + 1;
               }
           }
	   connIndex++;
       }
       return 0;
    }

    // Returns connId, a 1's based version of the index.
    public synchronized int addConnection(WeaveDeviceManager connObj)
    {
	int connIndex = 0;
	while (connIndex < mConnections.size())
	{
	    if (mConnections.get(connIndex) == null)
	    {
	        mConnections.set(connIndex, connObj);
	        return connIndex + 1;
	    }
	    connIndex++;
	}
	mConnections.add(connIndex, connObj);
	return connIndex + 1;
    }

    public synchronized WeaveDeviceManager removeConnection(int connId)
    {
        int connIndex = connId - 1;
        if (connIndex >= 0 && connIndex < mConnections.size())
        {
            // Set to null, rather than remove, so that other indexes are unchanged.
            return mConnections.set(connIndex, null);
        }
        else
        {
            Log.e(TAG, "Trying to remove unknown connId " + connId);
            return null;
        }
    }

    public static void onNotifyWeaveConnectionClosed(int connId)
    {
	WeaveDeviceManager deviceManager = WeaveStack.getInstance().getConnection(connId);
	deviceManager.onNotifyWeaveConnectionClosed(connId);
    }

    public static boolean onSendCharacteristic(int connId, byte[] svcId, byte[] charId, byte[] characteristicData)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
	{
	    WeaveDeviceManager deviceManager = WeaveStack.getInstance().getConnection(connId);
	    BluetoothGatt bluetoothGatt = deviceManager.getBluetoothGatt();
	    if (bluetoothGatt == null)
		return false;

	    UUID svcUUID = convertBytesToUUID(svcId);
	    BluetoothGattService sendSvc = bluetoothGatt.getService(svcUUID);
	    if (sendSvc == null)
	    {
		Log.e(TAG, "Bad service");
		return false;
	    }

	    UUID charUUID = convertBytesToUUID(charId);
	    BluetoothGattCharacteristic sendChar = sendSvc.getCharacteristic(charUUID);
	    if (!sendChar.setValue(characteristicData))
	    {
		Log.e(TAG, "Failed to set characteristic");
		return false;
	    }

	    if (!bluetoothGatt.writeCharacteristic(sendChar))
	    {
		Log.e(TAG, "Failed writing char");
		return false;
	    }
	    return true;
	}
	else
	{
	    Log.e(TAG, "BLE not supported on device.");
	}
	return false;
    }

    public static boolean onSubscribeCharacteristic(int connId, byte[] svcId, byte[] charId)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
        {
	    WeaveDeviceManager deviceManager = WeaveStack.getInstance().getConnection(connId);
	    BluetoothGatt bluetoothGatt = deviceManager.getBluetoothGatt();
	    if (bluetoothGatt == null)
		return false;

	    UUID svcUUID = convertBytesToUUID(svcId);
	    BluetoothGattService subscribeSvc = bluetoothGatt.getService(svcUUID);
	    if (subscribeSvc == null)
	    {
		Log.e(TAG, "Bad service");
		return false;
	    }

	    UUID charUUID = convertBytesToUUID(charId);
	    BluetoothGattCharacteristic subscribeChar = subscribeSvc.getCharacteristic(charUUID);
	    if (subscribeChar == null)
	    {
		Log.e(TAG, "Bad characteristic");
		return false;
	    }

	    if (!bluetoothGatt.setCharacteristicNotification(subscribeChar, true))
	    {
		Log.e(TAG, "Failed to subscribe to characteristic.");
		return false;
	    }

	    BluetoothGattDescriptor descriptor =
		subscribeChar.getDescriptor(UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
	    descriptor.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
	    if (!bluetoothGatt.writeDescriptor(descriptor))
	    {
		Log.e(TAG, "writeDescriptor failed");
		return false;
	    }
	    return true;
	}
	else
	{
	    Log.e(TAG, "BLE not supported on device.");
	}
	return false;
    }

    public static boolean onUnsubscribeCharacteristic(int connId, byte[] svcId, byte[] charId)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
        {
	    WeaveDeviceManager deviceManager = WeaveStack.getInstance().getConnection(connId);
	    BluetoothGatt bluetoothGatt = deviceManager.getBluetoothGatt();
	    if (bluetoothGatt == null)
		return false;

	    UUID svcUUID = convertBytesToUUID(svcId);
	    BluetoothGattService subscribeSvc = bluetoothGatt.getService(svcUUID);
	    if (subscribeSvc == null)
	    {
		Log.e(TAG, "Bad service");
		return false;
	    }

	    UUID charUUID = convertBytesToUUID(charId);
	    BluetoothGattCharacteristic subscribeChar = subscribeSvc.getCharacteristic(charUUID);
	    if (subscribeChar == null)
	    {
		Log.e(TAG, "Bad characteristic");
		return false;
	    }

	    if (!bluetoothGatt.setCharacteristicNotification(subscribeChar, false))
	    {
		Log.e(TAG, "Failed to unsubscribe to characteristic.");
		return false;
	    }

	    BluetoothGattDescriptor descriptor =
		subscribeChar.getDescriptor(UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
	    descriptor.setValue(BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
	    if (!bluetoothGatt.writeDescriptor(descriptor))
	    {
		Log.e(TAG, "writeDescriptor failed");
		return false;
	    }
	    return true;
	}
	else
	{
	    Log.e(TAG, "BLE not supported on device.");
	}
	return false;
    }

    public static boolean onCloseConnection(int connId)
    {
	WeaveDeviceManager deviceManager = WeaveStack.getInstance().getConnection(connId);
	deviceManager.onCloseBleComplete(connId);
	return true;
    }

    // onGetMTU returns the desired MTU for the BLE connection.
    // In most cases, a value of 0 is used to indicate no preference.
    // On some devices, we override to use the minimum MTU to work around device bugs.
    public static int onGetMTU(int connId)
    {
	int mtu = 0;

	Log.d(TAG, "Android Manufacturer: (" + android.os.Build.MANUFACTURER + ")");
	Log.d(TAG, "Android Model: (" + android.os.Build.MODEL + ")");

	if (BleMtuBlacklist.BLE_MTU_BLACKLISTED)
	{
	    mtu = BleMtuBlacklist.BLE_MTU_FALLBACK;
	    Log.e(TAG, "Detected Android Manufacturer/Model with MTU compatibiility issues. Reporting mtu of " + mtu);
	}
	return mtu;
    }

    // ----- Private Members -----

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void handleWriteConfirmation(int connId, byte[] svcId, byte[] charId, boolean success);
    private native void handleIndicationReceived(int connId, byte[] svcId, byte[] charId, byte[] data);
    private native void handleSubscribeComplete(int connId, byte[] svcId, byte[] charId, boolean success);
    private native void handleUnsubscribeComplete(int connId, byte[] svcId, byte[] charId, boolean success);
    private native void handleConnectionError(int connId);

    // CLIENT_CHARACTERISTIC_CONFIG is the well-known UUID of the client characteristic descriptor that has the
    // flags for enabling and disabling notifications and indications.
    // c.f. https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-attribute-profile
    private static String CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";

    private static byte[] convertUUIDToBytes(UUID uuid)
    {
	byte[] idBytes = new byte[16];
	long idBits;
	idBits = uuid.getLeastSignificantBits();
	for (int i = 0; i < 8; i++)
        {
	    idBytes[15 - i] = (byte) (idBits & 0xff);
	    idBits = idBits >> 8;
	}
	idBits = uuid.getMostSignificantBits();
	for (int i = 0; i < 8; i++)
        {
	    idBytes[7 - i] = (byte) (idBits & 0xff);
	    idBits = idBits >> 8;
        }
	return idBytes;
    }

    private static UUID convertBytesToUUID(byte[] id)
    {
        long mostSigBits = 0;
        long leastSigBits = 0;

	if (id.length == 16)
        {
	    for (int i = 0; i < 8; i++)
	    {
		mostSigBits = (mostSigBits << 8) | (0xff & id[i]);
	    }
	    for (int i = 0; i < 8; i++)
	    {
		leastSigBits = (leastSigBits << 8) | (0xff & id[i + 8]);
	    }
	}
        return new UUID(mostSigBits, leastSigBits);
    }
}

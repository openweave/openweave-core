/*

    Copyright (c) 013-2017 Nest Labs, Inc.
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

import java.util.Calendar;
import java.util.EnumSet;

public class WeaveDeviceDescriptor
{
    /** Weave device id (0 = not present).
        */
    public long deviceId;

    /** Device vendor code (0 = not present).
        */
    public int vendorCode;

    /** Device product code (0 = not present).
        */
    public int productCode;

    /** Device product revision (0 = not present).
        */
    public int productRevision;

    /** Date of device manufacture (null = not present).
        */
    public Calendar manufacturingDate;

    /** MAC address for primary 802.15.4 interface (big-endian, null = not present).
        */
    public byte[] primary802154MACAddress;

    /** MAC address for primary WiFi interface (big-endian, null = not present).
        */
    public byte[] primaryWiFiMACAddress;

    /** Device serial number (null = not present).
        */
    public String serialNumber;

    /** Installed software version (null = not present).
        */
    public String softwareVersion;

    /** ESSID or ESSID suffix for device's rendezvous WiFi network (null = not present).
        */
    public String rendezvousWiFiESSID;
    
    /** True if the rendezvousWiFiESSID field contains a suffix string.
        */
    public boolean isRendezvousWiFiESSIDSuffix;

    /** Device pairing code (null = not present).
        */
    public String pairingCode;

    /** Weave fabric to which the device belongs (0 = not present).
        */
    public long fabricId;

    /** Set of flags identifying features supported by the device.
        */
    public EnumSet<DeviceFeatures> deviceFeatures;

    public WeaveDeviceDescriptor()
    {
    }

    public WeaveDeviceDescriptor(int vendorCode, int productCode, int productRevision,
                                 int manufacturingYear, int manufacturingMonth, int manufacturingDay,
                                 byte[] primary802154MACAddress, byte[] primaryWiFiMACAddress,
                                 String serialNumber, String rendezvousWiFiESSID, String pairingCode,
                                 long deviceId, long fabricId, String softwareVersion, int deviceFeatures,
                                 int flags)
    {
        this.vendorCode = vendorCode;
        this.productCode = productCode;
        this.productRevision = productRevision;
        if (manufacturingYear != 0 || manufacturingMonth != 0 || manufacturingDay != 0) {
            this.manufacturingDate = Calendar.getInstance();
            if (manufacturingYear != 0)
                this.manufacturingDate.set(Calendar.YEAR, manufacturingYear);
            if (manufacturingMonth != 0)
                this.manufacturingDate.set(Calendar.MONTH, manufacturingMonth - 1);
            if (manufacturingDay != 0)
                this.manufacturingDate.set(Calendar.DAY_OF_MONTH, manufacturingDay);
        }
        this.primary802154MACAddress = primary802154MACAddress;
        this.primaryWiFiMACAddress = primaryWiFiMACAddress;
        this.serialNumber = serialNumber;
        this.rendezvousWiFiESSID = rendezvousWiFiESSID;
        this.pairingCode = pairingCode;
        this.deviceId = deviceId;
        this.fabricId = fabricId;
        this.softwareVersion = softwareVersion;
        this.deviceFeatures = DeviceFeatures.fromFlags(deviceFeatures);
        this.isRendezvousWiFiESSIDSuffix = ((flags & FLAG_IS_RENDEZVOUS_WIFI_ESSID_SUFFIX) != 0);
    }

    public static native WeaveDeviceDescriptor decode(byte[] encodedDeviceDesc);

    static {
        System.loadLibrary("WeaveDeviceManager");
    }
    
    static final int FLAG_IS_RENDEZVOUS_WIFI_ESSID_SUFFIX = 0x01;
}

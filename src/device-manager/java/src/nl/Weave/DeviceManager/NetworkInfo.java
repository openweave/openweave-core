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

/**
 *  Represents information about a network configured on Weave device, or known to it
 *  via a network scan.
 */
public class NetworkInfo
{
    public NetworkInfo()
    {
        NetworkType = nl.Weave.DeviceManager.NetworkType.NotSpecified;
        NetworkId = -1;
        WiFiSSID = null;
        WiFiMode = nl.Weave.DeviceManager.WiFiMode.NotSpecified;
        WiFiRole = nl.Weave.DeviceManager.WiFiRole.NotSpecified;
        WiFiSecurityType = nl.Weave.DeviceManager.WiFiSecurityType.NotSpecified;
        WiFiKey = null;
        ThreadNetworkName = null;
        ThreadExtendedPANId = null;
        ThreadNetworkKey = null;
        WirelessSignalStrength = Short.MIN_VALUE;
    }

    /** The type of network (WiFi, Thread, etc.)
          */
    public NetworkType NetworkType;

    /** The network id assigned to the network by the device, -1 if not specified.
          */
    public long NetworkId;

    /** The WiFi SSID, or NULL if not a WiFi network.
         */
    public String WiFiSSID;

    /** The operating mode of the WiFi network.
         */
    public WiFiMode WiFiMode;

    /** The role played by the device on the WiFi network.
         */
    public WiFiRole WiFiRole;

    /** The WiFi security type.
         */
    public WiFiSecurityType WiFiSecurityType;

    /** The WiFi key, or NULL if not specified.
         */
    public byte[] WiFiKey;

    /** The Thread network name, or NULL if not a Thread network.
         */
    public String ThreadNetworkName;

    /** The Thread extended PAN id, or NULL if not specified.
         * Must be exactly 8 bytes in length.
         */
    public byte[] ThreadExtendedPANId;

    /** The Thread network key, or NULL if not specified.
         */
    public byte[] ThreadNetworkKey;

    /** The signal strength of the network, in dBm, or Short.MIN_VALUE if not available/applicable.
         */
    public short WirelessSignalStrength;

    public static NetworkInfo MakeWiFi(String wifiSSID, WiFiMode wifiMode, WiFiRole wifiRole,
                                       WiFiSecurityType wifiSecurityType, byte[] wifiKey)
    {
        NetworkInfo netInfo = new NetworkInfo();
        netInfo.NetworkType = nl.Weave.DeviceManager.NetworkType.WiFi;
        netInfo.WiFiSSID = wifiSSID;
        netInfo.WiFiMode = wifiMode;
        netInfo.WiFiRole = wifiRole;
        netInfo.WiFiSecurityType = wifiSecurityType;
        netInfo.WiFiKey = wifiKey;
        return netInfo;
    }

    public static NetworkInfo MakeThread(String threadNetworkName, byte[] threadExtendedPANId, byte[] threadNetworkKey)
    {
        NetworkInfo netInfo = new NetworkInfo();
        netInfo.NetworkType = nl.Weave.DeviceManager.NetworkType.Thread;
        netInfo.ThreadNetworkName = threadNetworkName;
        netInfo.ThreadExtendedPANId = threadExtendedPANId;
        netInfo.ThreadNetworkKey = threadNetworkKey;
        return netInfo;
    }

    public static NetworkInfo Make(int networkType, long networkId,
                                   String wifiSSID, int wifiMode, int wifiRole, int wifiSecurityType, byte[] wifiKey,
                                   String threadNetworkName, byte[] threadExtendedPANId, byte[] threadNetworkKey,
                                   short wirelessSignalStrength)
    {
        NetworkInfo netInfo = new NetworkInfo();
        netInfo.NetworkType = nl.Weave.DeviceManager.NetworkType.fromVal(networkType);
        netInfo.NetworkId = networkId;
        netInfo.WiFiSSID = wifiSSID;
        netInfo.WiFiMode = nl.Weave.DeviceManager.WiFiMode.fromVal(wifiMode);
        netInfo.WiFiRole = nl.Weave.DeviceManager.WiFiRole.fromVal(wifiRole);
        netInfo.WiFiSecurityType = nl.Weave.DeviceManager.WiFiSecurityType.fromVal(wifiSecurityType);
        netInfo.WiFiKey = wifiKey;
        netInfo.ThreadNetworkName = threadNetworkName;
        netInfo.ThreadExtendedPANId = threadExtendedPANId;
        netInfo.ThreadNetworkKey = threadNetworkKey;
        netInfo.WirelessSignalStrength = wirelessSignalStrength;
        return netInfo;
    }
}

/*

    Copyright (c) 2019 Google LLC
    Copyright (c) 2013-2017 Nest Labs, Inc.
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

import java.text.SimpleDateFormat;
import java.util.EnumSet;
import javax.xml.bind.DatatypeConverter;
import java.math.BigInteger;

public class TestMain implements WeaveDeviceManager.CompletionHandler
{
    public class TestFailedException extends RuntimeException
    {
        public TestFailedException(String testName)
        {
            super(testName);
        }
    }

    public static void main(String[] args)
    {
        TestMain mainObj = new TestMain();

        try {
            mainObj.RunUnitTests();
        }
        catch (TestFailedException ex) {
            System.exit(-1);
        }
    }

    String TestResult = null;
    WeaveDeviceManager DeviceMgr;
    long AddNetworkId = -1;
    byte[] TestDeviceDescriptor = DatatypeConverter.parseHexBinary(
        "95010024002A24010124020125033245" +
        "2C041030354241303141433033313330" +
        "30334730050818B43000000A91B33006" +
        "0618B43001D1832C070A746F70617A2D" +
        "393162332C080631323334353618"
    );
    byte[] FabricConfig = null;
    int ExpectedNetworkCount;

    void RunUnitTests()
    {
        DeviceMgr = new WeaveDeviceManager();
        DeviceMgr.setCompletionHandler(this);

        System.out.println("isConnected Test");
        TestResult = String.valueOf(DeviceMgr.isConnected());
        ExpectResult("isConnected", "false");
        System.out.println("isConnected Test Succeeded");

        TestResult = null;
        System.out.println("ConnectDevice Test");
        System.out.println("    Connecting to test device at 127.0.0.1");
        DeviceMgr.beginConnectDevice(1, "127.0.0.1");
        ExpectSuccess("ConnectDevice");
        System.out.println("ConnectDevice Test Succeeded");

        System.out.println("isConnected Test");
        TestResult = String.valueOf(DeviceMgr.isConnected());
        ExpectResult("isConnected", "true");
        System.out.println("isConnected Test Succeeded");

        System.out.println("Closing WeaveDeviceManager");
        DeviceMgr.close();

        System.out.println("Setting Rendezvous Address");
        DeviceMgr.setRendezvousAddress("127.0.0.1");

        TestResult = null;
        System.out.println("RendezvousDevice Test");
        System.out.println("    Rendezvous with device at 127.0.0.1");
        DeviceMgr.beginRendezvousDevice("TEST", generateDefaultCriteria());
        ExpectSuccess("RendezvousDevice");
        System.out.println("RendezvousDevice Test Succeeded");

        System.out.println("deviceId Test");
        System.out.format("  Device Id: %d%n", DeviceMgr.deviceId());
        System.out.println("deviceId Test Complete");

        System.out.println("deviceAddress Test");
        System.out.format("  Device Address: %s%n", DeviceMgr.deviceAddress());
        System.out.println("deviceAddress Test Complete");

        TestResult = null;
        System.out.println("Ping Test");
        System.out.println("    Pinging device...");
        DeviceMgr.beginPing();
        ExpectSuccess("Ping");
        System.out.println("Ping Test Succeeded");

        System.out.println("Closing WeaveDeviceManager");
        DeviceMgr.close();

        TestResult = null;
        System.out.println("ReconnectDevice Test");
        System.out.println("    Reconnect with device");
        DeviceMgr.beginReconnectDevice();
        ExpectSuccess("ReconnectDevice");
        System.out.println("ReconnectDevice Test Succeeded");

        System.out.println("Closing WeaveDeviceManager");
        DeviceMgr.close();

        System.out.println("Closing Endpoints");
        WeaveDeviceManager.closeEndpoints();

        System.out.println("Enabling auto-reconnect");
        DeviceMgr.setAutoReconnect(true);

        TestResult = null;
        System.out.println("ResetConfig Test");
        System.out.println("    Resetting device configuration...");
        DeviceMgr.beginResetConfig(ResetFlags.All);
        ExpectSuccess("ResetConfig");
        System.out.println("ResetConfig Test Succeeded");

        TestResult = null;
        System.out.println("IdentifyDevice Test");
        System.out.println("    Identifying device...");
        DeviceMgr.beginIdentifyDevice();
        ExpectSuccess("IdentifyDevice");
        System.out.println("IdentifyDevice Test Succeeded");

        TestResult = null;
        System.out.println("ScanNetworks Test");
        System.out.println("    Scanning for WiFi networks...");
        ExpectedNetworkCount = 3;
        DeviceMgr.beginScanNetworks(NetworkType.WiFi);
        ExpectSuccess("ScanNetworks");
        System.out.println("ScanNetworks Test Succeeded");

        TestResult = null;
        System.out.println("ScanNetworks Test");
        System.out.println("    Scanning for Thread networks...");
        ExpectedNetworkCount = 1;
        DeviceMgr.beginScanNetworks(NetworkType.Thread);
        ExpectSuccess("ScanNetworks");
        System.out.println("ScanNetworks Test Succeeded");

        TestResult = null;
        System.out.println("ArmFailSafe Test");
        System.out.println("    Arming config fail-safe mechanism...");
        int failSafeToken = DeviceMgr.beginArmFailSafe(FailSafeArmMode.New);
        ExpectSuccess("ArmFailSafe");
        System.out.format("    Fail-safe token = %d%n", failSafeToken);
        System.out.println("ArmFailSafe Test Succeeded");

        TestResult = null;
        System.out.println("AddNetwork Test");
        System.out.println("    Adding new Wifi network...");
        NetworkInfo networkInfo = NetworkInfo.MakeWiFi("Wireless-Test", WiFiMode.Managed,
                WiFiRole.Station, WiFiSecurityType.WEP, "apassword".getBytes());
        DeviceMgr.beginAddNetwork(networkInfo);
        ExpectSuccess("AddNetwork");
        System.out.println("AddNetwork Test Succeeded");

        TestResult = null;
        System.out.println("AddNetwork Test");
        System.out.println("    Adding new Thread network...");
        networkInfo = NetworkInfo.MakeThread("Thread-Test",
                                        DatatypeConverter.parseHexBinary("0102030405060708"),
                                        "akey".getBytes(),
                                        0x1234,
                                        (byte)21);
        DeviceMgr.beginAddNetwork(networkInfo);
        ExpectSuccess("AddNetwork");
        System.out.println("AddNetwork Test Succeeded");

        TestResult = null;
        System.out.println("GetNetworks Test");
        System.out.println("    Getting configured networks...");
        ExpectedNetworkCount = 2;
        DeviceMgr.beginGetNetworks(GetNetworkFlags.None);
        ExpectSuccess("GetNetworks");
        System.out.println("GetNetworks Test Succeeded");

        TestResult = null;
        System.out.println("RemoveNetwork Test");
        System.out.format("    Removing WiFi network %d...%n", AddNetworkId);
        DeviceMgr.beginRemoveNetwork(AddNetworkId);
        ExpectSuccess("RemoveNetwork");
        System.out.println("RemoveNetwork Test Succeeded");

        TestResult = null;
        System.out.println("GetNetworks Test");
        System.out.println("    Getting configured networks...");
        ExpectedNetworkCount = 1;
        DeviceMgr.beginGetNetworks(GetNetworkFlags.None);
        ExpectSuccess("GetNetworks");
        System.out.println("GetNetworks Test Succeeded");

        TestResult = null;
        System.out.println("GetCameraAuthData Test");
        System.out.println("    Getting camera auth data...");
        DeviceMgr.beginGetCameraAuthData("Ceci n'est pas un nonce.012345670123456789ABCDEF0123456789ABCDEF");
        ExpectSuccess("GetCameraAuthData");
        System.out.println("GetCameraAuthData Test Succeeded");

        TestResult = null;
        System.out.println("CreateFabric Test");
        System.out.format("    Creating fabric...%n");
        DeviceMgr.beginCreateFabric();
        ExpectSuccess("CreateFabric");
        System.out.println("CreateFabric Test Succeeded");

        TestResult = null;
        System.out.println("GetFabricConfig Test");
        System.out.format("    Getting fabric configuration...%n");
        DeviceMgr.beginGetFabricConfig();
        ExpectSuccess("GetFabricConfig");
        System.out.println("GetFabricConfig Test Succeeded");

        TestResult = null;
        System.out.println("LeaveFabric Test");
        System.out.format("    Leaving fabric...%n");
        DeviceMgr.beginLeaveFabric();
        ExpectSuccess("LeaveFabric");
        System.out.println("LeaveFabric Test Succeeded");

        TestResult = null;
        System.out.println("JoinExistingFabric Test");
        System.out.format("    Joinging existing fabric...%n");
        DeviceMgr.beginJoinExistingFabric(FabricConfig);
        ExpectSuccess("JoinExistingFabric");
        System.out.println("JoinExistingFabric Test Succeeded");

        TestResult = null;
        System.out.println("DisrmFailSafe Test");
        System.out.println("    Disarming config fail-safe mechanism...");
        DeviceMgr.beginDisarmFailSafe();
        ExpectSuccess("DisrmFailSafe");
        System.out.println("DisrmFailSafe Test Succeeded");

        System.out.println("Closing WeaveDeviceManager");
        DeviceMgr.close();

        System.out.println("Decode Device Descriptor Test");
        WeaveDeviceDescriptor deviceDesc = DeviceMgr.decodeDeviceDescriptor(TestDeviceDescriptor);
        print(deviceDesc, "  ");
        System.out.println("Decode Device Descriptor Test Succeeded");

        System.out.println("ValidatePairingCode Test");
        if (!DeviceMgr.isValidPairingCode("3Y0DN8"))
            throw new TestFailedException("ValidatePairingCode");
        if (DeviceMgr.isValidPairingCode("3Y0D8N"))
            throw new TestFailedException("ValidatePairingCode");
        if (DeviceMgr.isValidPairingCode("ABCDEFGHI"))
            throw new TestFailedException("ValidatePairingCode");
        System.out.println("ValidatePairingCode Test Succeeded");

        System.out.println("Forcing GC/finalization of WeaveDeviceManager object");
        DeviceMgr = null;
        for (int i = 0; i < 3; i++) {
            System.gc();
            System.runFinalization();
            try { Thread.sleep(100); }
            catch (Exception ex) { }
        }

        System.out.println("All tests succeeded.");
    }

    private static IdentifyDeviceCriteria generateDefaultCriteria() {
        final IdentifyDeviceCriteria criteria = new IdentifyDeviceCriteria();
        criteria.TargetVendorId = 0x235A;
        criteria.TargetProductId = IdentifyDeviceCriteria.PRODUCT_WILDCARD_ID_NEST_PROTECT;
        criteria.TargetModes = TargetDeviceModes.UserSelectedMode;
        return criteria;
    }

    void ExpectSuccess(String testName)
    {
        ExpectResult(testName, "Success");
    }

    void ExpectResult(String testName, String expectedResult)
    {
        while (TestResult == null)
            try { Thread.sleep(100); }
            catch (Exception ex) { }

        if (TestResult != expectedResult) {
            if (expectedResult != "Success")
                System.out.format("%s test failed:%n    Expected: %s%n    Got: %s%n", testName, expectedResult, TestResult);
            else
                System.out.format("%s test failed: %s%n", testName, TestResult);
            throw new TestFailedException(testName);
        }
    }

    public void onConnectDeviceComplete()
    {
        System.out.println("    Connected to device");
        TestResult = "Success";
    }

    public void onRendezvousDeviceComplete()
    {
        System.out.println("    Rendezvous device complete");
        TestResult = "Success";
    }

    public void onReconnectDeviceComplete()
    {
        System.out.println("    Reconnect device complete");
        TestResult = "Success";
    }

    public void onIdentifyDeviceComplete(WeaveDeviceDescriptor deviceDesc)
    {
        System.out.format("    Identify device complete:%n");
        print(deviceDesc, "      ");
        TestResult = "Success";
    }

    public void onScanNetworksComplete(NetworkInfo[] networks)
    {
        System.out.format("    Network scan complete: %d network(s) found%n", networks.length);
        print(networks, "      ");
        if (networks.length == ExpectedNetworkCount)
            TestResult = "Success";
        else
            TestResult = "Incorrect number of networks";
    }

    public void onGetNetworksComplete(NetworkInfo[] networks)
    {
        System.out.format("    Get networks complete: %d network(s) found%n", networks.length);
        print(networks, "      ");
        if (networks.length == ExpectedNetworkCount)
            TestResult = "Success";
        else
            TestResult = "Incorrect number of networks";
    }

    public void onGetCameraAuthDataComplete(String macAddress, String authData)
    {
        System.out.format("     Get camera auth data complete: mac = %s, auth_data = %s%n", macAddress, authData);
        TestResult = "Success";
    }

    public void onAddNetworkComplete(long networkId)
    {
        System.out.format("    Add network complete: new network id %d%n", networkId);
        TestResult = "Success";
        if (AddNetworkId == -1)
            AddNetworkId = networkId;
    }

    public void onUpdateNetworkComplete()
    {
    }

    public void onRemoveNetworkComplete()
    {
        System.out.format("    Remove network complete%n");
        TestResult = "Success";
    }

    public void onEnableNetworkComplete()
    {
    }

    public void onDisableNetworkComplete()
    {
    }

    public void onTestNetworkConnectivityComplete()
    {
    }

    public void onGetRendezvousModeComplete(EnumSet<RendezvousMode> rendezvousModes)
    {
    }

    public void onSetRendezvousModeComplete()
    {
    }
    
    public void onGetLastNetworkProvisioningResultComplete()
    {
    }

    public void onRegisterServicePairAccountComplete()
    {
    }
    
    public void onUnregisterServiceComplete()
    {
    }

    public void onCreateFabricComplete()
    {
        System.out.println("    Create fabric complete");
        TestResult = "Success";
    }

    public void onLeaveFabricComplete()
    {
        System.out.println("    Leave fabric complete");
        TestResult = "Success";
    }

    public void onGetFabricConfigComplete(byte[] fabricConfig)
    {
        System.out.println("    Get fabric config complete");
        TestResult = "Success";
        FabricConfig = fabricConfig;
    }

    public void onJoinExistingFabricComplete()
    {
        System.out.println("    Join existing fabric complete");
        TestResult = "Success";
    }

    public void onArmFailSafeComplete()
    {
        System.out.println("    Arm fail-safe complete");
        TestResult = "Success";
    }

    public void onDisarmFailSafeComplete()
    {
        System.out.println("    Disarm fail-safe complete");
        TestResult = "Success";
    }

    public void onResetConfigComplete()
    {
        System.out.println("    Reset config complete");
        TestResult = "Success";
    }

    public void onPingComplete()
    {
        System.out.format("    Ping complete%n");
        TestResult = "Success";
    }

    public void onEnableConnectionMonitorComplete()
    {
    }

    public void onDisableConnectionMonitorComplete()
    {
    }

    public void onPairTokenComplete(byte[] pairingTokenBundle)
    {
        System.out.println("    Pair token complete");
        TestResult = "Success";
    }

    public void onUnpairTokenComplete()
    {
        System.out.println("    Unpair token complete");
        TestResult = "Success";
    }

    public void onError(Throwable err)
    {
        TestResult = err.toString();
    }

    public void onDeviceEnumerationResponse(WeaveDeviceDescriptor deviceDesc, String deviceAddr)
    {
        System.out.format("    Device enumeration response received, IP addr = %s%n", deviceAddr);
        print(deviceDesc, "     ");
    }

    public void onConnectBleComplete() {}

    public void onCloseBleComplete() {}

    public void onNotifyWeaveConnectionClosed() {}

    public void onRemotePassiveRendezvousComplete() {}

    public void onStartSystemTestComplete()
    {
    }

    public void onStopSystemTestComplete()
    {
    }

    public void print(WeaveDeviceDescriptor deviceDesc, String prefix)
    {
        BigInteger twoToThe64 = BigInteger.ONE.shiftLeft(64);

        if (deviceDesc.deviceId != 0)
            System.out.format("%sDevice Id: %s%n", prefix, BigInteger.valueOf(deviceDesc.deviceId).add(twoToThe64).mod(twoToThe64).toString(16));
        if (deviceDesc.vendorCode != 0)
            System.out.format("%sVendor Code: %d%n", prefix, deviceDesc.vendorCode);
        if (deviceDesc.productCode != 0)
            System.out.format("%sProduct Code: %d%n", prefix, deviceDesc.productCode);
        if (deviceDesc.productRevision != 0)
            System.out.format("%sProduct Revision: %d%n", prefix, deviceDesc.productRevision);
        if (deviceDesc.serialNumber != null)
            System.out.format("%sSerial Number: %s%n", prefix, deviceDesc.serialNumber);
        if (deviceDesc.softwareVersion != null)
            System.out.format("%sSoftware Version: %s%n", prefix, deviceDesc.softwareVersion);
        if (deviceDesc.manufacturingDate != null)
            System.out.format("%sManufacturing Date: %s%n", prefix, new SimpleDateFormat("yyyy/MM/dd").format(deviceDesc.manufacturingDate.getTime()));
        if (deviceDesc.primary802154MACAddress != null)
            System.out.format("%sPrimary 802.15.4 MAC Address: %s%n", prefix, DatatypeConverter.printHexBinary(deviceDesc.primary802154MACAddress));
        if (deviceDesc.primaryWiFiMACAddress != null)
            System.out.format("%sPrimary WiFi MAC Address: %s%n", prefix, DatatypeConverter.printHexBinary(deviceDesc.primaryWiFiMACAddress));
        if (deviceDesc.rendezvousWiFiESSID != null)
            System.out.format("%sRendezvous WiFi ESSID: %s%n", prefix, deviceDesc.rendezvousWiFiESSID);
        if (deviceDesc.pairingCode != null)
            System.out.format("%sPairing Code: %s%n", prefix, deviceDesc.pairingCode);
        if (deviceDesc.fabricId != 0)
            System.out.format("%sFabric Id: %s%n", prefix, BigInteger.valueOf(deviceDesc.fabricId).add(twoToThe64).mod(twoToThe64).toString(16));
        System.out.format("%sDevice Features: ", prefix);
        for (DeviceFeatures enumVal : deviceDesc.deviceFeatures)
            System.out.format("%s ", enumVal);
        System.out.format("%n");
    }

    public void print(NetworkInfo[] networks, String indent)
    {
        int i = 0;
        for (NetworkInfo n : networks) {
            System.out.format("%sNetwork %d%n", indent, i);
            if (n.NetworkType != NetworkType.NotSpecified)
                System.out.format("%s  Network Type: %s%n", indent, n.NetworkType.name());
            if (n.NetworkId != -1)
                System.out.format("%s  Network Id: %d%n", indent, n.NetworkId);
            if (n.WiFiSSID != null)
                System.out.format("%s  WiFi SSID: \"%s\"%n", indent, n.WiFiSSID);
            if (n.WiFiMode != WiFiMode.NotSpecified)
                System.out.format("%s  WiFi Mode: %s%n", indent, n.WiFiMode.name());
            if (n.WiFiRole != WiFiRole.NotSpecified)
                System.out.format("%s  WiFi Role: %s%n", indent, n.WiFiRole.name());
            if (n.WiFiSecurityType != WiFiSecurityType.NotSpecified)
                System.out.format("%s  WiFi Security Type: %s%n", indent, n.WiFiSecurityType.name());
            if (n.WiFiKey != null)
                System.out.format("%s  WiFi Key: %s%n", indent, DatatypeConverter.printHexBinary(n.WiFiKey));
            if (n.ThreadNetworkName != null)
                System.out.format("%s  Thread Network Name: \"%s\"%n", indent, n.ThreadNetworkName);
            if (n.ThreadExtendedPANId != null)
                System.out.format("%s  Thread Extended PAN Id: %s%n", indent, DatatypeConverter.printHexBinary(n.ThreadExtendedPANId));
            if (n.ThreadNetworkKey != null)
                System.out.format("%s  Thread Network Key: %s%n", indent, DatatypeConverter.printHexBinary(n.ThreadNetworkKey));
            if (n.WirelessSignalStrength != Short.MIN_VALUE)
                System.out.format("%s  Wireless Signal Strength: %d%n", indent, n.WirelessSignalStrength);
            i++;
        }
    }

}

#
#    Copyright (c) 2013-2018 Nest Labs, Inc.
#    Copyright (c) 2019-2020 Google, LLC.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    @file
#      Python interface for Weave Device Manager
#

"""Weave Device Manager interface
"""

from __future__ import absolute_import
from __future__ import print_function
import functools
import sys
import os
import re
import copy
import binascii
import datetime
import time
import glob
import platform
import ast
from threading import Thread, Lock, Event
from ctypes import *
import six
from six.moves import range
from .WeaveUtility import WeaveUtility
from .WeaveStack import *


__all__ = [ 'WeaveDeviceManager', 'NetworkInfo', 'DeviceDescriptor' ]

NetworkType_WiFi                            = 1
NetworkType_Thread                          = 2
NetworkType_SystemManaged                   = 3

WiFiMode_AdHoc                              = 1
WiFiMode_Managed                            = 2

WiFiRole_Station                            = 1
WiFiRole_AccessPoint                        = 2

WiFiSecurityType_None                       = 1
WiFiSecurityType_WEP                        = 2
WiFiSecurityType_WPAPersonal                = 3
WiFiSecurityType_WPA2Personal               = 4
WiFiSecurityType_WPA2MixedPersonal          = 5
WiFiSecurityType_WPAEnterprise              = 6
WiFiSecurityType_WPA2Enterprise             = 7
WiFiSecurityType_WPA2MixedEnterprise        = 8
WiFiSecurityType_WPA3Personal               = 9
WiFiSecurityType_WPA3MixedPersonal          = 10
WiFiSecurityType_WPA3Enterprise             = 11
WiFiSecurityType_WPA3MixedEnterprise        = 12
ThreadPANId_NotSpecified                    = 0xFFFFFFFF

ThreadChannel_NotSpecified                  = 0xFF

RendezvousMode_EnableWiFiRendezvousNetwork   = 0x0001
RendezvousMode_Enable802154RendezvousNetwork = 0x0002
RendezvousMode_EnableFabricRendezvousAddress = 0x0004

TargetFabricId_AnyFabric                    = 0xFFFFFFFFFFFFFFFF
TargetFabricId_NotInFabric                  = 0

TargetDeviceMode_Any                        = 0x00000000    # Locate all devices regardless of mode.
TargetDeviceMode_UserSelectedMode           = 0x00000001    # Locate all devices in 'user-selected' mode -- i.e. where the device has
                                                            #   has been directly identified by a user, e.g. by pressing a button.

TargetVendorId_Any                          = 0xFFFF

TargetProductId_Any                         = 0xFFFF

TargetDeviceId_Any                          = 0xFFFFFFFFFFFFFFFF

DeviceFeature_HomeAlarmLinkCapable          = 0x00000001    # Indicates a Nest Protect that supports connection to a home alarm panel
DeviceFeature_LinePowered                   = 0x00000002    # Indicates a device that requires line power

SystemTest_ProductList                      = { 'thermostat'    : 0x235A000A,
                                                'topaz'         : 0x235A0003}

DeviceDescriptorFlag_IsRendezvousWiFiESSIDSuffix = 0x01

class NetworkInfo:
    def __init__(self, networkType=None, networkId=None, wifiSSID=None, wifiMode=None, wifiRole=None,
                 wifiSecurityType=None, wifiKey=None,
                 threadNetworkName=None, threadExtendedPANId=None, threadNetworkKey=None, threadPSKc=None,
                 wirelessSignalStrength=None, threadPANId=None, threadChannel=None):
        self.NetworkType = networkType
        self.NetworkId = networkId
        self.WiFiSSID = wifiSSID
        self.WiFiMode = wifiMode
        self.WiFiRole = wifiRole
        self.WiFiSecurityType = wifiSecurityType
        self.WiFiKey = wifiKey
        self.ThreadNetworkName = threadNetworkName
        self.ThreadExtendedPANId = threadExtendedPANId
        self.ThreadNetworkKey = threadNetworkKey
        self.ThreadPSKc = threadPSKc
        self.ThreadPANId = threadPANId
        self.ThreadChannel = threadChannel
        self.WirelessSignalStrength = wirelessSignalStrength

    def Print(self, prefix=""):
        print("%sNetwork Type: %s" % (prefix, NetworkTypeToString(self.NetworkType)))
        if self.NetworkId != None:
            print("%sNetwork Id: %d" % (prefix, self.NetworkId))
        if self.WiFiSSID != None:
            print("%sWiFi SSID: \"%s\"" % (prefix, self.WiFiSSID))
        if self.WiFiMode != None:
            print("%sWiFi Mode: %s" % (prefix, WiFiModeToString(self.WiFiMode)))
        if self.WiFiRole != None:
            print("%sWiFi Role: %s" % (prefix, WiFiRoleToString(self.WiFiRole)))
        if self.WiFiSecurityType != None:
            print("%sWiFi Security Type: %s" % (prefix, WiFiSecurityTypeToString(self.WiFiSecurityType)))
        if self.WiFiKey != None:
            print("%sWiFi Key: %s" % (prefix, self.WiFiKey))
        if self.ThreadNetworkName != None:
            print("%sThread Network Name: \"%s\"" % (prefix, self.ThreadNetworkName))
        if self.ThreadExtendedPANId != None:
            print("%sThread Extended PAN Id: %s" % (prefix, WeaveUtility.ByteArrayToHex(self.ThreadExtendedPANId)))
        if self.ThreadNetworkKey != None:
            print("%sThread Network Key: %s" % (prefix, WeaveUtility.ByteArrayToHex(self.ThreadNetworkKey)))
        if self.ThreadPSKc != None:
            print("%sThread Network PSKc: %s" % (prefix, WeaveUtility.ByteArrayToHex(self.ThreadPSKc)))
        if self.ThreadPANId != None:
            print("%sThread PAN Id: %04x" % (prefix, self.ThreadPANId))
        if self.ThreadChannel != None:
            print("%sThread Channel: %d" % (prefix, self.ThreadChannel))
        if self.WirelessSignalStrength != None:
            print("%sWireless Signal Strength: %s" % (prefix, self.WirelessSignalStrength))

    def SetField(self, name, val):
        name = name.lower();
        if (name == 'networktype' or name == 'network-type' or name == 'type'):
            self.NetworkType = ParseNetworkType(val)
        elif (name == 'networkid' or name == 'network-id' or name == 'id'):
            self.NetworkId = int(val)
        elif (name == 'wifissid' or name == 'wifi-ssid' or name == 'ssid'):
            self.WiFiSSID = val
        elif (name == 'wifimode' or name == 'wifi-mode'):
            self.WiFiMode = ParseWiFiMode(val)
        elif (name == 'wifirole' or name == 'wifi-role'):
            self.WiFiRole = ParseWiFiRole(val)
        elif (name == 'wifisecuritytype' or name == 'wifi-security-type' or name == 'securitytype' or name == 'security-type' or name == 'wifi-security' or name == 'security'):
            self.WiFiSecurityType = ParseSecurityType(val)
        elif (name == 'wifikey' or name == 'wifi-key' or name == 'key'):
            self.WiFiKey = val
        elif (name == 'threadnetworkname' or name == 'thread-network-name' or name == 'thread-name'):
            self.ThreadNetworkName = val
        elif (name == 'threadextendedpanid' or name == 'thread-extended-pan-id'):
            self.ThreadExtendedPANId = val
        elif (name == 'threadnetworkkey' or name == 'thread-network-key' or name == 'thread-key'):
            self.ThreadNetworkKey = val
        elif (name == 'threadpskc' or name == 'thread-pskc' or name == 'pskc'):
            self.ThreadPSKc = val
        elif (name == 'threadpanid' or name == 'thread-pan-id' or name == 'pan-id'):
            self.ThreadPANId = val
        elif (name == 'threadchannel' or name == 'thread-channel'):
            self.ThreadChannel = val
        elif (name == 'wirelesssignalstrength' or name == 'wireless-signal-strength'):
            self.WirelessSignalStrength = val
        else:
            raise Exception("Invalid NetworkInfo field: " + str(name))

class DeviceDescriptor:
    def __init__(self, deviceId=None, fabricId=None, vendorId=None, productId=None, productRevision=None,
                 manufacturingYear=None, manufacturingMonth=None, manufacturingDay=None,
                 primary802154MACAddress=None, primaryWiFiMACAddress=None,
                 serialNumber=None, softwareVersion=None, rendezvousWiFiESSID=None, pairingCode=None,
                 pairingCompatibilityVersionMajor=None, pairingCompatibilityVersionMinor=None,
                 deviceFeatures=None, flags=None):
        self.DeviceId = deviceId
        self.FabricId = fabricId
        self.VendorId = vendorId
        self.ProductId = productId
        self.ProductRevision = productRevision
        self.ManufacturingYear = manufacturingYear
        self.ManufacturingMonth = manufacturingMonth
        self.ManufacturingDay = manufacturingDay
        self.Primary802154MACAddress = primary802154MACAddress
        self.PrimaryWiFiMACAddress = primaryWiFiMACAddress
        self.SerialNumber = serialNumber
        self.SoftwareVersion = softwareVersion
        self.RendezvousWiFiESSID = rendezvousWiFiESSID
        self.PairingCode = pairingCode
        self.PairingCompatibilityVersionMajor = pairingCompatibilityVersionMajor
        self.PairingCompatibilityVersionMinor = pairingCompatibilityVersionMinor
        self.DeviceFeatures = [ ]
        if deviceFeatures != None:
            featureVal = 1
            while featureVal != 0x80000000:
                if (deviceFeatures & featureVal) == featureVal:
                    self.DeviceFeatures.append(featureVal)
                featureVal <<= 1
        self.Flags = flags if flags != None else 0

    def Print(self, prefix=""):
        if self.DeviceId != None:
            print("%sDevice Id: %016X" % (prefix, self.DeviceId))
        if self.FabricId != None:
            print("%sFabrid Id: %016X" % (prefix, self.FabricId))
        if self.VendorId != None:
            print("%sVendor Id: %X" % (prefix, self.VendorId))
        if self.ProductId != None:
            print("%sProduct Id: %X" % (prefix, self.ProductId))
        if self.ProductRevision != None:
            print("%sProduct Revision: %X" % (prefix, self.ProductRevision))
        if self.SerialNumber != None:
            print("%sSerial Number: %s" % (prefix, self.SerialNumber))
        if self.SoftwareVersion != None:
            print("%sSoftware Version: %s" % (prefix, self.SoftwareVersion))
        if self.ManufacturingYear != None and self.ManufacturingMonth != None:
            if self.ManufacturingDay != None:
                print("%sManufacturing Date: %04d/%02d/%02d" % (prefix, self.ManufacturingYear, self.ManufacturingMonth, self.ManufacturingDay))
            else:
                print("%sManufacturing Date: %04d/%02d" % (prefix, self.ManufacturingYear, self.ManufacturingMonth))
        if self.Primary802154MACAddress != None:
            print("%sPrimary 802.15.4 MAC Address: %s" % (prefix, WeaveUtility.ByteArrayToHex(self.Primary802154MACAddress)))
        if self.PrimaryWiFiMACAddress != None:
            print("%sPrimary WiFi MAC Address: %s" % (prefix, WeaveUtility.ByteArrayToHex(self.PrimaryWiFiMACAddress)))
        if self.RendezvousWiFiESSID != None:
            print("%sRendezvous WiFi ESSID%s: %s" % (prefix, " Suffix" if self.IsRendezvousWiFiESSIDSuffix else "", self.RendezvousWiFiESSID))
        if self.PairingCode != None:
            print("%sPairing Code: %s" % (prefix, self.PairingCode))
        if self.PairingCompatibilityVersionMajor != None:
            print("%sPairing Compatibility Major Id: %X" % (prefix, self.PairingCompatibilityVersionMajor))
        if self.PairingCompatibilityVersionMinor != None:
            print("%sPairing Compatibility Minor Id: %X" % (prefix, self.PairingCompatibilityVersionMinor))
        if self.DeviceFeatures != None:
            print("%sDevice Features: %s" % (prefix, " ".join([DeviceFeatureToString(val) for val in self.DeviceFeatures])))

    @property
    def IsRendezvousWiFiESSIDSuffix(self):
        return (self.Flags & DeviceDescriptorFlag_IsRendezvousWiFiESSIDSuffix) != 0

class WirelessRegConfig:
    def __init__(self, regDomain=None, opLocation=None, supportedRegDomains=None):
        self.RegDomain = regDomain
        self.OpLocation = opLocation
        self.SupportedRegDomains = supportedRegDomains

    def Print(self, prefix=""):
        if self.RegDomain != None:
            print("%sRegulatory Domain: %s%s" % (prefix, self.RegDomain, ' (world wide)' if self.RegDomain == '00' else ''))
        if self.OpLocation != None:
            print("%sOperating Location: %s" % (prefix, OperatingLocationToString(self.OpLocation)))
        if self.SupportedRegDomains != None:
            print("%sSupported Regulatory Domains: %s" % (prefix, ','.join(self.SupportedRegDomains)))

class _IdentifyDeviceCriteriaStruct(Structure):
    _fields_ = [
        ("TargetFabricId",  c_uint64),
        ("TargetModes",     c_uint32),
        ("TargetVendorId",  c_uint16),
        ("TargetProductId", c_uint16),
        ("TargetDeviceId",  c_uint64)
    ]

class _NetworkInfoStruct(Structure):
    _fields_ = [
        ('NetworkType', c_int32),           # The type of network.
        ('NetworkId', c_int64),             # network id assigned to the network by the device, -1 if not specified.
        ('WiFiSSID', c_char_p),             # The WiFi SSID.
        ('WiFiMode', c_int32),              # The operating mode of the WiFi network.
        ('WiFiRole', c_int32),              # The role played by the device on the WiFi network.
        ('WiFiSecurityType', c_int32),      # The WiFi security type.
        ('WiFiKey', c_void_p),              # The WiFi key, or NULL if not specified.
        ('WiFiKeyLen', c_uint32),           # The length in bytes of the WiFi key.
        ('ThreadNetworkName', c_char_p),    # The name of the Thread network.
        ('ThreadExtendedPANId', c_void_p),  # The Thread extended PAN id (8 bytes).
        ('ThreadNetworkKey', c_void_p),     # The Thread master network key.
        ('ThreadPSKc', c_void_p),           # The Thread pre-shared key for commissioner
        ('ThreadPANId', c_uint32),          # The 16-bit Thread PAN ID, or kThreadPANId_NotSpecified
        ('ThreadChannel', c_uint8),         # The current channel on which the Thread network operates, or kThreadChannel_NotSpecified
        ('WirelessSignalStrength', c_int16),# The signal strength of the network, or INT16_MIN if not available/applicable.
        ('Hidden', c_bool)                  # Whether or not the network is hidden.
    ]

    def toNetworkInfo(self):
        return NetworkInfo(
            networkType = self.NetworkType if self.NetworkType != -1 else None,
            networkId = self.NetworkId if self.NetworkId != -1 else None,
            wifiSSID = WeaveUtility.CStringToString(self.WiFiSSID),
            wifiMode = self.WiFiMode if self.WiFiMode != -1 else None,
            wifiRole = self.WiFiRole if self.WiFiRole != -1 else None,
            wifiSecurityType = self.WiFiSecurityType if self.WiFiSecurityType != -1 else None,
            wifiKey = WeaveUtility.VoidPtrToByteArray(self.WiFiKey, self.WiFiKeyLen),
            threadNetworkName = WeaveUtility.CStringToString(self.ThreadNetworkName),
            threadExtendedPANId = WeaveUtility.VoidPtrToByteArray(self.ThreadExtendedPANId, 8),
            threadNetworkKey = WeaveUtility.VoidPtrToByteArray(self.ThreadNetworkKey, 16),
            threadPSKc = WeaveUtility.VoidPtrToByteArray(self.ThreadPSKc, 16),
            threadPANId = self.ThreadPANId if self.ThreadPANId != ThreadPANId_NotSpecified else None,
            threadChannel = self.ThreadChannel if self.ThreadChannel != ThreadChannel_NotSpecified else None,
            wirelessSignalStrength = self.WirelessSignalStrength if self.WirelessSignalStrength != -32768 else None
        )

    @classmethod
    def fromNetworkInfo(cls, networkInfo):
        networkInfoStruct = cls()
        networkInfoStruct.NetworkType = networkInfo.NetworkType if networkInfo.NetworkType != None else -1
        networkInfoStruct.NetworkId = networkInfo.NetworkId if networkInfo.NetworkId != None else -1
        networkInfoStruct.WiFiSSID = WeaveUtility.StringToCString(networkInfo.WiFiSSID)
        networkInfoStruct.WiFiMode = networkInfo.WiFiMode if networkInfo.WiFiMode != None else -1
        networkInfoStruct.WiFiRole = networkInfo.WiFiRole if networkInfo.WiFiRole != None else -1
        networkInfoStruct.WiFiSecurityType = networkInfo.WiFiSecurityType if networkInfo.WiFiSecurityType != None else -1
        networkInfoStruct.WiFiKey = WeaveUtility.ByteArrayToVoidPtr(networkInfo.WiFiKey)
        networkInfoStruct.WiFiKeyLen = len(networkInfo.WiFiKey) if (networkInfo.WiFiKey != None) else 0
        networkInfoStruct.ThreadNetworkName = WeaveUtility.StringToCString(networkInfo.ThreadNetworkName)
        networkInfoStruct.ThreadExtendedPANId = WeaveUtility.ByteArrayToVoidPtr(networkInfo.ThreadExtendedPANId)
        networkInfoStruct.ThreadNetworkKey = WeaveUtility.ByteArrayToVoidPtr(networkInfo.ThreadNetworkKey)
        networkInfoStruct.ThreadPSKc = WeaveUtility.ByteArrayToVoidPtr(networkInfo.ThreadPSKc)
        networkInfoStruct.ThreadPANId = networkInfo.ThreadPANId if networkInfo.ThreadPANId != None else ThreadPANId_NotSpecified
        networkInfoStruct.ThreadChannel = networkInfo.ThreadChannel if networkInfo.ThreadChannel != None else ThreadChannel_NotSpecified
        networkInfoStruct.WirelessSignalStrength = networkInfo.WirelessSignalStrength if networkInfo.WirelessSignalStrength != None else -32768
        return networkInfoStruct

class _DeviceDescriptorStruct(Structure):
    _fields_ = [
        ('DeviceId', c_uint64),                     # Weave device id (0 = not present)
        ('FabricId', c_uint64),                     # Id of Weave fabric to which the device belongs (0 = not present)
        ('DeviceFeatures', c_uint32),               # Bit field indicating support for specific device features.
        ('VendorId', c_uint16),                     # Device vendor id  (0 = not present)
        ('ProductId', c_uint16),                    # Device product id (0 = not present)
        ('ProductRevision', c_uint16),              # Device product revision (0 = not present)
        ('ManufacturingYear', c_uint16),            # Year of device manufacture (valid range 2001 - 2099, 0 = not present)
        ('ManufacturingMonth', c_ubyte),            # Month of device manufacture (1 = January, 0 = not present)
        ('ManufacturingDay', c_ubyte),              # Day of device manufacture (0 = not present)
        ('Primary802154MACAddress', c_ubyte * 8),   # MAC address for primary 802.15.4 interface (big-endian, all zeros = not present)
        ('PrimaryWiFiMACAddress', c_ubyte * 6),     # MAC address for primary WiFi interface (big-endian, all zeros = not present)
        ('SerialNumber', c_char * 33),              # Serial number of device (nul terminated, 0 length = not present)
        ('SoftwareVersion', c_char * 129),           # Version of software running on the device (nul terminated, 0 length = not present)
        ('RendezvousWiFiESSID', c_char * 33),       # ESSID for pairing WiFi network (nul terminated, 0 length = not present)
        ('PairingCode', c_char * 17),               # Device pairing code (nul terminated, 0 length = not present)
        ('PairingCompatibilityVersionMajor', c_uint16), # Pairing software compatibility major version
        ('PairingCompatibilityVersionMinor', c_uint16), # Pairing software compatibility minor version
        ('Flags', c_ubyte),                         # Flags
    ]

    def toDeviceDescriptor(self):
        return DeviceDescriptor(
            deviceId = self.DeviceId if self.DeviceId != 0 else None,
            fabricId = self.FabricId if self.FabricId != 0 else None,
            vendorId = self.VendorId if self.VendorId != 0 else None,
            productId = self.ProductId if self.ProductId != 0 else None,
            productRevision = self.ProductRevision if self.ProductRevision != 0 else None,
            manufacturingYear = self.ManufacturingYear if self.ManufacturingYear != 0 else None,
            manufacturingMonth = self.ManufacturingMonth if self.ManufacturingMonth != 0 else None,
            manufacturingDay = self.ManufacturingDay if self.ManufacturingDay != 0 else None,
            primary802154MACAddress = bytearray(self.Primary802154MACAddress) if not WeaveUtility.IsByteArrayAllZeros(self.Primary802154MACAddress) else None,
            primaryWiFiMACAddress = bytearray(self.PrimaryWiFiMACAddress) if not WeaveUtility.IsByteArrayAllZeros(self.PrimaryWiFiMACAddress) else None,
            serialNumber = WeaveUtility.CStringToString(self.SerialNumber) if len(self.SerialNumber) != 0 else None,
            softwareVersion = WeaveUtility.CStringToString(self.SoftwareVersion) if len(self.SoftwareVersion) != 0 else None,
            rendezvousWiFiESSID = WeaveUtility.CStringToString(self.RendezvousWiFiESSID) if len(self.RendezvousWiFiESSID) != 0 else None,
            pairingCode = WeaveUtility.CStringToString(self.PairingCode) if len(self.PairingCode) != 0 else None,
            pairingCompatibilityVersionMajor = self.PairingCompatibilityVersionMajor,
            pairingCompatibilityVersionMinor = self.PairingCompatibilityVersionMinor,
            deviceFeatures = self.DeviceFeatures,
            flags = self.Flags)

class _WirelessRegDomain(Structure):
    _fields_ = [
        ('Code', c_char * 2),                        # Wireless regulatory domain code (exactly 2 characters, non-null terminated)
    ]
    
    def __str__(self, *args, **kwargs):
        return ''.join(WeaveUtility.CStringToString(self.Code))
    
    @classmethod
    def fromStr(cls, val):
        regDomainStruct = cls()
        if val != None:
            if len(val) != 2:
                raise ValueError('Invalid wireless regulatory domain code: ' + val)
            regDomainStruct.Code = WeaveUtility.StringToCString(val)
        else:
            regDomainStruct.Code = b'\0\0'
        return regDomainStruct

class _WirelessRegConfigStruct(Structure):
    _fields_ = [
        ('SupportedRegDomains', POINTER(_WirelessRegDomain)),   # Array of _WirelessRegDomain structures
        ('NumSupportedRegDomains', c_uint16),                   # Length of SupportedRegDomains array
        ('RegDomain', _WirelessRegDomain),                      # Selected wireless regulatory domain
        ('OpLocation', c_ubyte),                                # Selected operating location
    ]

    def toWirelessRegConfig(self):
        return WirelessRegConfig(
            regDomain = str(self.RegDomain) if self.RegDomain.Code[0] != 0 else None,
            opLocation = self.OpLocation if self.OpLocation != 0xFF else None,
            supportedRegDomains = [ str(self.SupportedRegDomains[i]) for i in range(0, self.NumSupportedRegDomains) ]
        )

    @classmethod
    def fromWirelessRegConfig(cls, regConfig):
        regConfigStruct = cls()
        regConfigStruct.SupportedRegDomains = POINTER(_WirelessRegDomain)()
        regConfigStruct.NumSupportedRegDomains = 0
        regConfigStruct.RegDomain = _WirelessRegDomain.fromStr(regConfig.RegDomain)
        regConfigStruct.OpLocation = regConfig.OpLocation if regConfig.OpLocation != None else 0
        return regConfigStruct

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_IdentifyDeviceCompleteFunct                = CFUNCTYPE(None, c_void_p, c_void_p, POINTER(_DeviceDescriptorStruct))
_PairTokenCompleteFunct                     = CFUNCTYPE(None, c_void_p, c_void_p, c_void_p, c_uint32)
_UnpairTokenCompleteFunct                   = CFUNCTYPE(None, c_void_p, c_void_p)
_NetworkScanCompleteFunct                   = CFUNCTYPE(None, c_void_p, c_void_p, c_uint16, POINTER(_NetworkInfoStruct))
_AddNetworkCompleteFunct                    = CFUNCTYPE(None, c_void_p, c_void_p, c_uint32)
_GetNetworksCompleteFunct                   = CFUNCTYPE(None, c_void_p, c_void_p, c_uint16, POINTER(_NetworkInfoStruct))
_GetCameraAuthDataCompleteFunct             = CFUNCTYPE(None, c_void_p, c_void_p, c_char_p, c_char_p)
_GetRendezvousModeCompleteFunct             = CFUNCTYPE(None, c_void_p, c_void_p, c_uint16)
_GetFabricConfigCompleteFunct               = CFUNCTYPE(None, c_void_p, c_void_p, c_void_p, c_uint32)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_GetBleEventFunct                           = CFUNCTYPE(c_void_p)
_WriteBleCharacteristicFunct                = CFUNCTYPE(c_bool, c_void_p, c_void_p, c_void_p, c_void_p, c_uint16)
_SubscribeBleCharacteristicFunct            = CFUNCTYPE(c_bool, c_void_p, c_void_p, c_void_p, c_bool)
_CloseBleFunct                              = CFUNCTYPE(c_bool, c_void_p)
_DeviceEnumerationResponseFunct             = CFUNCTYPE(None, c_void_p, POINTER(_DeviceDescriptorStruct), c_char_p)
_GetWirelessRegulatoryConfigCompleteFunct   = CFUNCTYPE(None, c_void_p, c_void_p, POINTER(_WirelessRegConfigStruct))

# This is a fix for WEAV-429. Jay Logue recommends revisiting this at a later
# date to allow for truely multiple instances so this is temporary.
def _singleton(cls):
    instance = [None]

    def wrapper(*args, **kwargs):
        if instance[0] is None:
            instance[0] = cls(*args, **kwargs)
        return instance[0]

    return wrapper

@_singleton
class WeaveDeviceManager(object):
    def __init__(self, startNetworkThread=True):
        self.devMgr = None
        self.networkThread = None
        self.networkThreadRunable = False
        self._weaveStack = WeaveStack()
        self._dmLib = None

        self._InitLib()

        devMgr = c_void_p(None)
        res = self._dmLib.nl_Weave_DeviceManager_NewDeviceManager(pointer(devMgr))
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        self.devMgr = devMgr
        self._weaveStack.devMgr = devMgr

        def HandleDeviceEnumerationResponse(devMgr, deviceDescPtr, deviceAddrStr):
            print("    Enumerated device IP: %s" % (WeaveUtility.CStringToString(deviceAddrStr)))
            deviceDescPtr.contents.toDeviceDescriptor().Print("    ")

        self.cbHandleDeviceEnumerationResponse = _DeviceEnumerationResponseFunct(HandleDeviceEnumerationResponse)
        self.blockingCB = None # set by other modules(BLE) that require service by thread while thread blocks.
        self.cbHandleBleEvent = None # set by other modules (BLE) that provide event callback to Weave.
        self.cbHandleBleWriteChar = None
        self.cbHandleBleSubscribeChar = None
        self.cbHandleBleClose = None

        if (startNetworkThread):
            self.StartNetworkThread()

    def __del__(self):
        if (self.devMgr != None):
            self._dmLib.nl_Weave_DeviceManager_DeleteDeviceManager(self.devMgr)
            self.devMgr = None

        self.StopNetworkThread()

    def DriveBleIO(self):
        # perform asynchronous write to pipe in IO thread's select() to wake for BLE input
        res = self._dmLib.nl_Weave_DeviceManager_WakeForBleIO()
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def SetBleEventCB(self, bleEventCB):
        if (self.devMgr != None):
            self.cbHandleBleEvent = _GetBleEventFunct(bleEventCB)
            self._dmLib.nl_Weave_DeviceManager_SetBleEventCB(self.cbHandleBleEvent)

    def SetBleWriteCharCB(self, bleWriteCharCB):
        if (self.devMgr != None):
            self.cbHandleBleWriteChar = _WriteBleCharacteristicFunct(bleWriteCharCB)
            self._dmLib.nl_Weave_DeviceManager_SetBleWriteCharacteristic(self.cbHandleBleWriteChar)

    def SetBleSubscribeCharCB(self, bleSubscribeCharCB):
        if (self.devMgr != None):
            self.cbHandleBleSubscribeChar = _SubscribeBleCharacteristicFunct(bleSubscribeCharCB)
            self._dmLib.nl_Weave_DeviceManager_SetBleSubscribeCharacteristic(self.cbHandleBleSubscribeChar)

    def SetBleCloseCB(self, bleCloseCB):
        if (self.devMgr != None):
            self.cbHandleBleClose = _CloseBleFunct(bleCloseCB)
            self._dmLib.nl_Weave_DeviceManager_SetBleClose(self.cbHandleBleClose)

    def StartNetworkThread(self):
        if (self.networkThread != None):
            return

        def RunNetworkThread():
            while (self.networkThreadRunable):
                self._weaveStack.networkLock.acquire()
                self._dmLib.nl_Weave_DeviceManager_DriveIO(50)
                self._weaveStack.networkLock.release()
                time.sleep(0.005)

        self.networkThread = Thread(target=RunNetworkThread, name="WeaveNetworkThread")
        self.networkThread.daemon = True
        self.networkThreadRunable = True
        self.networkThread.start()

    def StopNetworkThread(self):
        if (self.networkThread != None):
            self.networkThreadRunable = False
            self.networkThread.join()
            self.networkThread = None

    def IsConnected(self):
        return self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_IsConnected(self.devMgr)
        )

    def DeviceId(self):
        return self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_DeviceId(self.devMgr)
        )

    def DeviceAddress(self):
        return self._weaveStack.Call(
            lambda: WeaveUtility.CStringToString(self._dmLib.nl_Weave_DeviceManager_DeviceAddress(self.devMgr))
        )

    def SetRendezvousAddress(self, addr, intf = None):
        if addr is not None and "\x00" in addr:
            raise ValueError("Unexpected NUL character in addr");

        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetRendezvousAddress(self.devMgr, WeaveUtility.StringToCString(addr), WeaveUtility.StringToCString(intf))
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def SetConnectTimeout(self, timeoutMS):
        if timeoutMS < 0 or timeoutMS > pow(2,32):
            raise ValueError("timeoutMS must be an unsigned 32-bit integer")

        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetConnectTimeout(self.devMgr, timeoutMS)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def SetAutoReconnect(self, autoReconnect):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetAutoReconnect(self.devMgr, autoReconnect)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def SetRendezvousLinkLocal(self, RendezvousLinkLocal):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetRendezvousLinkLocal(self.devMgr, RendezvousLinkLocal)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def StartDeviceEnumeration(self, targetFabricId=TargetFabricId_AnyFabric,
                         targetModes=TargetDeviceMode_Any,
                         targetVendorId=TargetVendorId_Any,
                         targetProductId=TargetProductId_Any,
                         targetDeviceId=TargetDeviceId_Any):

        deviceCriteria = _IdentifyDeviceCriteriaStruct()
        deviceCriteria.TargetFabricId = targetFabricId
        deviceCriteria.TargetModes = targetModes
        deviceCriteria.TargetVendorId = targetVendorId
        deviceCriteria.TargetProductId = targetProductId
        deviceCriteria.TargetDeviceId = targetDeviceId

        self._weaveStack.Call(
                lambda: self._dmLib.nl_Weave_DeviceManager_StartDeviceEnumeration(self.devMgr, deviceCriteria, self.cbHandleDeviceEnumerationResponse, self._weaveStack.cbHandleError)
            )

    def StopDeviceEnumeration(self):
        res = self._weaveStack.Call(
                lambda: self._dmLib.nl_Weave_DeviceManager_StopDeviceEnumeration(self.devMgr)
            )

    def ConnectDevice(self, deviceId, deviceAddr=None,
                      pairingCode=None, accessToken=None):
        if deviceAddr is not None and '\x00' in deviceAddr:
            raise ValueError("Unexpected NUL character in deviceAddr")

        if pairingCode is not None and '\x00' in pairingCode:
            raise ValueError("Unexpected NUL character in pairingCode")

        if (pairingCode != None and accessToken != None):
            raise ValueError('Must specify only one of pairingCode or accessToken when calling WeaveDeviceManager.ConnectDevice')

        if (pairingCode == None and accessToken == None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectDevice_NoAuth(self.devMgr, deviceId, WeaveUtility.StringToCString(deviceAddr), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        elif (pairingCode != None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectDevice_PairingCode(self.devMgr, deviceId, WeaveUtility.StringToCString(deviceAddr), WeaveUtility.StringToCString(pairingCode), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        else:
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectDevice_AccessToken(self.devMgr, deviceId, WeaveUtility.StringToCString(deviceAddr), WeaveUtility.ByteArrayToVoidPtr(accessToken), len(accessToken), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )

    def RendezvousDevice(self, pairingCode=None, accessToken=None,
                         targetFabricId=TargetFabricId_AnyFabric,
                         targetModes=TargetDeviceMode_Any,
                         targetVendorId=TargetVendorId_Any,
                         targetProductId=TargetProductId_Any,
                         targetDeviceId=TargetDeviceId_Any):

        if pairingCode is not None and '\x00' in pairingCode:
            raise ValueError("Unexpected NUL character in pairingCode")

        if (pairingCode != None and accessToken != None):
            raise ValueError('Must specify only one of pairingCode or accessToken when calling WeaveDeviceManager.RendezvousDevice')

        deviceCriteria = _IdentifyDeviceCriteriaStruct()
        deviceCriteria.TargetFabricId = targetFabricId
        deviceCriteria.TargetModes = targetModes
        deviceCriteria.TargetVendorId = targetVendorId
        deviceCriteria.TargetProductId = targetProductId
        deviceCriteria.TargetDeviceId = targetDeviceId

        if (pairingCode == None and accessToken == None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_NoAuth(self.devMgr, deviceCriteria, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        elif (pairingCode != None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_PairingCode(self.devMgr, WeaveUtility.StringToCString(pairingCode), deviceCriteria, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        else:
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_AccessToken(self.devMgr, WeaveUtility.ByteArrayToVoidPtr(accessToken), len(accessToken), deviceCriteria, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )

    # methods for testing BLE performance are not a part of the Weave Device Manager API, but rather are considered internal.
    def TestBle(self, connObj, count, duration, delay, ack, size, rx):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_TestBle(self.devMgr, connObj, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError, count, duration, delay, ack, size, rx)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def TestResultBle(self, connObj, local):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_TestResultBle(self.devMgr, connObj, local)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def TestAbortBle(self, connObj):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_TestAbortBle(self.devMgr, connObj)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def TxTimingBle(self, connObj, enabled, remote):
        res = self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_TxTimingBle(self.devMgr, connObj, enabled, remote)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    # end of BLE testing methods

    def ConnectBle(self, bleConnection, pairingCode=None, accessToken=None):
        if pairingCode is not None and '\x00' in pairingCode:
            raise ValueError("Unexpected NUL character in pairingCode")

        if (pairingCode != None and accessToken != None):
            raise ValueError('Must specify only one of pairingCode or accessToken when calling WeaveDeviceManager.ConnectBle')

        if (pairingCode == None and accessToken == None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectBle_NoAuth(self.devMgr, bleConnection, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        elif (pairingCode != None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectBle_PairingCode(self.devMgr, bleConnection, WeaveUtility.StringToCString(pairingCode), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        else:
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_ConnectBle_AccessToken(self.devMgr, bleConnection, WeaveUtility.ByteArrayToVoidPtr(accessToken), len(accessToken), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )

    def PassiveRendezvousDevice(self, pairingCode=None, accessToken=None):
        if pairingCode is not None and '\x00' in pairingCode:
            raise ValueError("Unexpected NUL character in pairingCode")

        if (pairingCode != None and accessToken != None):
            raise ValueError('Must specify only one of pairingCode or accessToken when calling WeaveDeviceManager.PassiveRendezvousDevice')

        if (pairingCode == None and accessToken == None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_NoAuth(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        elif (pairingCode != None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_PairingCode(self.devMgr, WeaveUtility.StringToCString(pairingCode), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        else:
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_AccessToken(self.devMgr, WeaveUtility.ByteArrayToVoidPtr(accessToken), len(accessToken), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )

    def RemotePassiveRendezvous(self, rendezvousDeviceAddr=None, pairingCode=None, accessToken=None, rendezvousTimeout=None, inactivityTimeout=None):
        if rendezvousDeviceAddr == None:
            rendezvousDeviceAddr = "::"

        if '\x00' in rendezvousDeviceAddr:
            raise ValueError("Unexpected NUL character in rendezvousDeviceAddr")

        if pairingCode is not None and '\x00' in pairingCode:
            raise ValueError("Unexpected NUL character in pairingCode")

        if (pairingCode == None and accessToken == None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_NoAuth(self.devMgr, WeaveUtility.StringToCString(rendezvousDeviceAddr), rendezvousTimeout, inactivityTimeout, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        elif (pairingCode != None):
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_PASEAuth(self.devMgr, WeaveUtility.StringToCString(rendezvousDeviceAddr), WeaveUtility.StringToCString(pairingCode), rendezvousTimeout, inactivityTimeout, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )
        else:
            self._weaveStack.CallAsync(
                lambda: self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_CASEAuth(self.devMgr, WeaveUtility.StringToCString(rendezvousDeviceAddr), WeaveUtility.ByteArrayToVoidPtr(accessToken), len(accessToken), rendezvousTimeout, inactivityTimeout, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
            )

    def ReconnectDevice(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_ReconnectDevice(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def Close(self):
        self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_Close(self.devMgr)
        )

    def EnableConnectionMonitor(self, interval, timeout):
        if interval < 0 or interval > pow(2,16):
            raise ValueError("interval must be an unsigned 16-bit unsigned value")

        if timeout < 0 or timeout > pow(2,16):
            raise ValueError("timeout must be an unsigned 16-bit unsigned value")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_EnableConnectionMonitor(self.devMgr, interval, timeout, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def DisableConnectionMonitor(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_DisableConnectionMonitor(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def IdentifyDevice(self):
        def HandleIdentifyDeviceComplete(devMgr, reqState, deviceDescPtr):
            self._weaveStack.callbackRes = deviceDescPtr.contents.toDeviceDescriptor()
            self._weaveStack.completeEvent.set()

        cbHandleIdentifyDeviceComplete = _IdentifyDeviceCompleteFunct(HandleIdentifyDeviceComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_IdentifyDevice(self.devMgr, cbHandleIdentifyDeviceComplete, self._weaveStack.cbHandleError)
        )

    def PairToken(self, pairingToken):
        def HandlePairTokenComplete(devMgr, reqState, tokenPairingBundlePtr, tokenPairingBundleLen):
            self._weaveStack.callbackRes = WeaveUtility.VoidPtrToByteArray(tokenPairingBundlePtr, tokenPairingBundleLen)
            self._weaveStack.completeEvent.set()

        cbHandlePairTokenComplete = _PairTokenCompleteFunct(HandlePairTokenComplete)

        if pairingToken is not None and isinstance(pairingToken, str):
            pairingToken = WeaveUtility.StringToCString(pairingToken)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_PairToken(self.devMgr, WeaveUtility.ByteArrayToVoidPtr(pairingToken), len(pairingToken), cbHandlePairTokenComplete, self._weaveStack.cbHandleError)
        )

    def UnpairToken(self):
        def HandleUnpairTokenComplete(devMgr, reqState):
            self._weaveStack.callbackRes = True
            self._weaveStack.completeEvent.set()

        cbHandleUnpairTokenComplete = _UnpairTokenCompleteFunct(HandleUnpairTokenComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_UnpairToken(self.devMgr, cbHandleUnpairTokenComplete, self._weaveStack.cbHandleError)
        )

    def ScanNetworks(self, networkType):
        def HandleScanNetworksComplete(devMgr, reqState, netCount, netInfoPtr):
            self._weaveStack.callbackRes = [ netInfoPtr[i].toNetworkInfo() for i in range(netCount) ]
            self._weaveStack.completeEvent.set()

        cbHandleScanNetworksComplete = _NetworkScanCompleteFunct(HandleScanNetworksComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_ScanNetworks(self.devMgr, networkType, cbHandleScanNetworksComplete, self._weaveStack.cbHandleError)
        )

    def GetNetworks(self, getFlags):
        def HandleGetNetworksComplete(devMgr, reqState, netCount, netInfoPtr):
            self._weaveStack.callbackRes = [ netInfoPtr[i].toNetworkInfo() for i in range(netCount) ]
            self._weaveStack.completeEvent.set()

        cbHandleGetNetworksComplete = _GetNetworksCompleteFunct(HandleGetNetworksComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetNetworks(self.devMgr, getFlags, cbHandleGetNetworksComplete, self._weaveStack.cbHandleError)
        )

    def GetCameraAuthData(self, nonce):
        if nonce is not None and '\x00' in nonce:
            raise ValueError("Unexpected NUL character in nonce")

        def HandleGetCameraAuthDataComplete(devMgr, reqState, macAddress, signedCameraPayload):
            self.callbackRes = [ WeaveUtility.CStringToString(macAddress), WeaveUtility.CStringToString(signedCameraPayload) ]
            self.completeEvent.set()

        cbHandleGetCameraAuthDataComplete = _GetCameraAuthDataCompleteFunct(HandleGetCameraAuthDataComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetCameraAuthData(self.devMgr, WeaveUtility.StringToCString(nonce), cbHandleGetCameraAuthDataComplete, self._weaveStack.cbHandleError)
        )

    def AddNetwork(self, networkInfo):
        def HandleAddNetworkComplete(devMgr, reqState, networkId):
            self._weaveStack.callbackRes = networkId
            self._weaveStack.completeEvent.set()

        cbHandleAddNetworkComplete = _AddNetworkCompleteFunct(HandleAddNetworkComplete)

        networkInfoStruct = _NetworkInfoStruct.fromNetworkInfo(networkInfo)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_AddNetwork(self.devMgr, networkInfoStruct, cbHandleAddNetworkComplete, self._weaveStack.cbHandleError)
        )

    def UpdateNetwork(self, networkInfo):
        networkInfoStruct = _NetworkInfoStruct.fromNetworkInfo(networkInfo)

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_UpdateNetwork(self.devMgr, networkInfoStruct, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def RemoveNetwork(self, networkId):
        if networkId < 0 or networkId > pow(2,32):
            raise ValueError("networkId must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_RemoveNetwork(self.devMgr, networkId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def EnableNetwork(self, networkId):

        if networkId < 0 or networkId > pow(2,32):
            raise ValueError("networkId must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_EnableNetwork(self.devMgr, networkId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def DisableNetwork(self, networkId):
        if networkId < 0 or networkId > pow(2,32):
            raise ValueError("networkId must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_DisableNetwork(self.devMgr, networkId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def TestNetworkConnectivity(self, networkId):
        if networkId < 0 or networkId > pow(2,32):
            raise ValueError("networkId must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_TestNetworkConnectivity(self.devMgr, networkId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def GetRendezvousMode(self):
        def HandleGetRendezvousModeComplete(devMgr, reqState, modeFlags):
            self._weaveStack.callbackRes = modeFlags
            self._weaveStack.completeEvent.set()

        cbHandleGetRendezvousModeComplete = _GetRendezvousModeCompleteFunct(HandleGetRendezvousModeComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetRendezvousMode(self.devMgr, cbHandleGetRendezvousModeComplete, self._weaveStack.cbHandleError)
        )

    def SetRendezvousMode(self, modeFlags):
        if modeFlags < 0 or modeFlags > pow(2,16):
            raise ValueError("modeFlags must be an unsigned 16-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetRendezvousMode(self.devMgr, modeFlags, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def GetWirelessRegulatoryConfig(self):
        def HandleComplete(devMgr, reqState, regConfigPtr):
            self._weaveStack.callbackRes = regConfigPtr[0].toWirelessRegConfig()
            self._weaveStack.completeEvent.set()

        cbHandleComplete = _GetWirelessRegulatoryConfigCompleteFunct(HandleComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetWirelessRegulatoryConfig(self.devMgr, cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def SetWirelessRegulatoryConfig(self, regConfig):
        regConfigStruct = _WirelessRegConfigStruct.fromWirelessRegConfig(regConfig)
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetWirelessRegulatoryConfig(self.devMgr, regConfigStruct, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def GetLastNetworkProvisioningResult(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetLastNetworkProvisioningResult(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def CreateFabric(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_CreateFabric(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def LeaveFabric(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_LeaveFabric(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def GetFabricConfig(self):
        def HandleGetFabricConfigComplete(devMgr, reqState, fabricConfigPtr, fabricConfigLen):
            self._weaveStack.callbackRes = WeaveUtility.VoidPtrToByteArray(fabricConfigPtr, fabricConfigLen)
            self._weaveStack.completeEvent.set()

        cbHandleGetFabricConfigComplete = _GetFabricConfigCompleteFunct(HandleGetFabricConfigComplete)

        return self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetFabricConfig(self.devMgr, cbHandleGetFabricConfigComplete, self._weaveStack.cbHandleError)
        )

    def JoinExistingFabric(self, fabricConfig):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_JoinExistingFabric(self.devMgr, WeaveUtility.ByteArrayToVoidPtr(fabricConfig), len(fabricConfig),
                                              self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def Ping(self):
        WeaveUtility.StringToCString("test")
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_Ping(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def RegisterServicePairAccount(self, serviceId, accountId, serviceConfig, pairingToken, pairingInitData):
        if accountId is not None and '\x00' in accountId:
            raise ValueError("Unexpected NUL character in accountId")

        if pairingToken is not None and isinstance(pairingToken, str):
            pairingToken = WeaveUtility.StringToCString(pairingToken)

        if pairingInitData is not None and isinstance(pairingInitData, str):
            pairingInitData = WeaveUtility.StringToCString(pairingInitData)

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_RegisterServicePairAccount(self.devMgr, serviceId, WeaveUtility.StringToCString(accountId),
                                                                                  WeaveUtility.ByteArrayToVoidPtr(serviceConfig), len(serviceConfig),
                                                                                  WeaveUtility.ByteArrayToVoidPtr(pairingToken), len(pairingToken),
                                                                                  WeaveUtility.ByteArrayToVoidPtr(pairingInitData), len(pairingInitData),
                                                                                  self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def UpdateService(self, serviceId, serviceConfig):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_UpdateService(self.devMgr, serviceId, WeaveUtility.ByteArrayToVoidPtr(serviceConfig),
                                         len(serviceConfig), self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def UnregisterService(self, serviceId):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_UnregisterService(self.devMgr, serviceId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def ArmFailSafe(self, armMode, failSafeToken):
        if armMode < 0 or armMode > pow(2, 8):
            raise ValueError("armMode must be an unsigned 8-bit integer")

        if failSafeToken < 0 or failSafeToken > pow(2, 32):
            raise ValueError("failSafeToken must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_ArmFailSafe(self.devMgr, armMode, failSafeToken, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def DisarmFailSafe(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_DisarmFailSafe(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def ResetConfig(self, resetFlags):
        if resetFlags < 0 or resetFlags > pow(2, 16):
            raise ValueError("resetFlags must be an unsigned 16-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_ResetConfig(self.devMgr, resetFlags, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def CloseEndpoints(self):
        self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_CloseEndpoints()
        )

    def SetLogFilter(self, category):
        if category < 0 or category > pow(2, 8):
            raise ValueError("category must be an unsigned 8-bit integer")

        self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_SetLogFilter(category)
        )

    def GetLogFilter(self):
        self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_DeviceManager_GetLogFilter()
        )

    def SetBlockingCB(self, blockingCB):
        self._weaveStack.blockingCB = blockingCB

    def StartSystemTest(self, profileId, testId):
        if profileId < 0 or profileId > pow(2, 32):
            raise ValueError("profileId must be an unsigned 32-bit integer")

        if testId < 0 or testId > pow(2, 32):
            raise ValueError("testId must be an unsigned 32-bit integer")

        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_StartSystemTest(self.devMgr, profileId, testId, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def StopSystemTest(self):
        self._weaveStack.CallAsync(
            lambda: self._dmLib.nl_Weave_DeviceManager_StopSystemTest(self.devMgr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    # ----- Private Members -----
    def _InitLib(self):
        if (self._dmLib == None):
            self._dmLib = CDLL(self._weaveStack.LocateWeaveDLL())

            self._dmLib.nl_Weave_DeviceManager_NewDeviceManager.argtypes = [ POINTER(c_void_p) ]
            self._dmLib.nl_Weave_DeviceManager_NewDeviceManager.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_DeleteDeviceManager.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_DeleteDeviceManager.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_Close.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_Close.restype = None

            self._dmLib.nl_Weave_DeviceManager_DriveIO.argtypes = [ c_uint32 ]
            self._dmLib.nl_Weave_DeviceManager_DriveIO.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_WakeForBleIO.argtypes = [ ]
            self._dmLib.nl_Weave_DeviceManager_WakeForBleIO.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetBleEventCB.argtypes = [ _GetBleEventFunct ]
            self._dmLib.nl_Weave_DeviceManager_SetBleEventCB.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetBleWriteCharacteristic.argtypes = [ _WriteBleCharacteristicFunct ]
            self._dmLib.nl_Weave_DeviceManager_SetBleWriteCharacteristic.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetBleSubscribeCharacteristic.argtypes = [ _SubscribeBleCharacteristicFunct ]
            self._dmLib.nl_Weave_DeviceManager_SetBleSubscribeCharacteristic.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetBleClose.argtypes = [ _CloseBleFunct ]
            self._dmLib.nl_Weave_DeviceManager_SetBleClose.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_IsConnected.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_IsConnected.restype = c_bool

            self._dmLib.nl_Weave_DeviceManager_DeviceId.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_DeviceId.restype = c_uint64

            self._dmLib.nl_Weave_DeviceManager_DeviceAddress.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_DeviceAddress.restype = c_char_p

            self._dmLib.nl_Weave_DeviceManager_StartDeviceEnumeration.argtypes = [ c_void_p, POINTER(_IdentifyDeviceCriteriaStruct), _DeviceEnumerationResponseFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_StartDeviceEnumeration.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_StopDeviceEnumeration.argtypes = [ c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_StopDeviceEnumeration.restype = None

            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_NoAuth.argtypes = [ c_void_p, c_uint64, c_char_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_NoAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_PairingCode.argtypes = [ c_void_p, c_uint64, c_char_p, c_char_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_PairingCode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_AccessToken.argtypes = [ c_void_p, c_uint64, c_char_p, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectDevice_AccessToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_NoAuth.argtypes = [ c_void_p, POINTER(_IdentifyDeviceCriteriaStruct), _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_NoAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_PairingCode.argtypes = [ c_void_p, c_char_p, POINTER(_IdentifyDeviceCriteriaStruct), _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_PairingCode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_AccessToken.argtypes = [ c_void_p, c_void_p, c_uint32, POINTER(_IdentifyDeviceCriteriaStruct), _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RendezvousDevice_AccessToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_NoAuth.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_NoAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_PairingCode.argtypes = [ c_void_p, c_char_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_PairingCode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_AccessToken.argtypes = [ c_void_p, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_PassiveRendezvousDevice_AccessToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_TestBle.argtypes = [ c_void_p, c_void_p, _CompleteFunct, _ErrorFunct, c_uint32, c_uint32, c_uint16, c_uint8, c_uint16, c_bool ]
            self._dmLib.nl_Weave_DeviceManager_TestBle.restype = c_uint32
            self._dmLib.nl_Weave_DeviceManager_TestResultBle.argtypes = [ c_void_p, c_void_p, c_bool ]
            self._dmLib.nl_Weave_DeviceManager_TestResultBle.restype = c_uint32
            self._dmLib.nl_Weave_DeviceManager_TestAbortBle.argtypes = [ c_void_p, c_void_p ]
            self._dmLib.nl_Weave_DeviceManager_TestAbortBle.restype = c_uint32
            self._dmLib.nl_Weave_DeviceManager_TxTimingBle.argtypes = [ c_void_p, c_void_p, c_bool, c_bool ]
            self._dmLib.nl_Weave_DeviceManager_TxTimingBle.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ConnectBle_NoAuth.argtypes = [ c_void_p, c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectBle_NoAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ConnectBle_PairingCode.argtypes = [ c_void_p, c_void_p, c_char_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectBle_PairingCode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ConnectBle_AccessToken.argtypes = [ c_void_p, c_void_p, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ConnectBle_AccessToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_CASEAuth.argtypes = [ c_void_p, c_char_p, c_char_p, c_uint32, c_uint16, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_CASEAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_PASEAuth.argtypes = [ c_void_p, c_char_p, c_char_p, c_uint16, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_PASEAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_NoAuth.argtypes = [ c_void_p, c_char_p, c_uint16, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RemotePassiveRendezvous_NoAuth.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ReconnectDevice.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ReconnectDevice.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_EnableConnectionMonitor.argtypes = [ c_void_p, c_uint16, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_EnableConnectionMonitor.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_DisableConnectionMonitor.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_DisableConnectionMonitor.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_IdentifyDevice.argtypes = [ c_void_p, _IdentifyDeviceCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_IdentifyDevice.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_PairToken.argtypes = [ c_void_p, c_void_p, c_uint32, _PairTokenCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_PairToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_UnpairToken.argtypes = [ c_void_p, _UnpairTokenCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_UnpairToken.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ScanNetworks.argtypes = [ c_void_p, c_int, _NetworkScanCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ScanNetworks.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetNetworks.argtypes = [ c_void_p, c_int, _GetNetworksCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetNetworks.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetCameraAuthData.argtypes = [ c_void_p, c_char_p, _GetCameraAuthDataCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetCameraAuthData.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_AddNetwork.argtypes = [ c_void_p, POINTER(_NetworkInfoStruct), _AddNetworkCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_AddNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_UpdateNetwork.argtypes = [ c_void_p, POINTER(_NetworkInfoStruct), _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_UpdateNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RemoveNetwork.argtypes = [ c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RemoveNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_EnableNetwork.argtypes = [ c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_EnableNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_DisableNetwork.argtypes = [ c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_DisableNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_TestNetworkConnectivity.argtypes = [ c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_TestNetworkConnectivity.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetRendezvousMode.argtypes = [ c_void_p, _GetRendezvousModeCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetRendezvousMode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetRendezvousMode.argtypes = [ c_void_p, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_SetRendezvousMode.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetLastNetworkProvisioningResult.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetLastNetworkProvisioningResult.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetWirelessRegulatoryConfig.argtypes = [ c_void_p, _GetWirelessRegulatoryConfigCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_AddNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetWirelessRegulatoryConfig.argtypes = [ c_void_p, POINTER(_WirelessRegConfigStruct), _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_AddNetwork.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetLastNetworkProvisioningResult.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetLastNetworkProvisioningResult.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_LeaveFabric.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_LeaveFabric.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetFabricConfig.argtypes = [ c_void_p, _GetFabricConfigCompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_GetFabricConfig.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetRendezvousAddress.argtypes = [ c_void_p, c_char_p, c_char_p ]
            self._dmLib.nl_Weave_DeviceManager_SetRendezvousAddress.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_JoinExistingFabric.argtypes = [ c_void_p, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_JoinExistingFabric.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_Ping.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_Ping.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetRendezvousAddress.argtypes = [ c_void_p, c_char_p ]
            self._dmLib.nl_Weave_DeviceManager_SetRendezvousAddress.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetConnectTimeout.argtypes = [ c_void_p, c_uint32 ]
            self._dmLib.nl_Weave_DeviceManager_SetConnectTimeout.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetAutoReconnect.argtypes = [ c_void_p, c_bool ]
            self._dmLib.nl_Weave_DeviceManager_SetAutoReconnect.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_SetRendezvousLinkLocal.argtypes = [ c_void_p, c_bool ]
            self._dmLib.nl_Weave_DeviceManager_SetRendezvousLinkLocal.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_RegisterServicePairAccount.argtypes = [ c_void_p, c_uint64, c_char_p, c_void_p, c_uint32, c_void_p, c_uint32, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_RegisterServicePairAccount.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_UpdateService.argtypes = [ c_void_p, c_uint64, c_void_p, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_UpdateService.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_UnregisterService.argtypes = [ c_void_p, c_uint64, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_UnregisterService.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ArmFailSafe.argtypes = [ c_void_p, c_uint8, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ArmFailSafe.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_DisarmFailSafe.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_DisarmFailSafe.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_ResetConfig.argtypes = [ c_void_p, c_uint16, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_ResetConfig.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_GetLogFilter.argtypes = [ ]
            self._dmLib.nl_Weave_DeviceManager_GetLogFilter.restype = c_uint8

            self._dmLib.nl_Weave_DeviceManager_SetLogFilter.argtypes = [ c_uint8 ]
            self._dmLib.nl_Weave_DeviceManager_SetLogFilter.restype = None

            self._dmLib.nl_Weave_DeviceManager_CloseEndpoints.argtypes = [ ]
            self._dmLib.nl_Weave_DeviceManager_CloseEndpoints.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_StartSystemTest.argtypes = [ c_void_p, c_uint32, c_uint32, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_StartSystemTest.restype = c_uint32

            self._dmLib.nl_Weave_DeviceManager_StopSystemTest.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._dmLib.nl_Weave_DeviceManager_StopSystemTest.restype = c_uint32

def NetworkTypeToString(val):
    if (val == NetworkType_WiFi):
        return "WiFi"
    if (val == NetworkType_Thread):
        return "Thread"
    if (val == NetworkType_SystemManaged):
        return "SystemManaged"
    if (val != None):
        return "UNKNOWN (" + str(val)+ ")"
    return None

def ParseNetworkType(val):
    if isinstance(val, six.integer_types):
        return val
    val = val.lower()
    if (val == "wifi"):
        return NetworkType_WiFi
    if (val == "thread"):
        return NetworkType_Thread
    if (val == "system"):
        return NetworkType_SystemManaged
    raise Exception("Invalid network type: " + str(val))

def WiFiModeToString(val):
    if (val == WiFiMode_AdHoc):
        return "AdHoc"
    if (val == WiFiMode_Managed):
        return "Managed"
    if (val != None):
        return "Unknown (" + str(val)+ ")"
    return None

def ParseWiFiMode(val):
    if isinstance(val, six.integer_types):
        return val
    val = val.lower()
    if (val == "adhoc" or val == "ad-hoc"):
        return WiFiMode_AdHoc
    if (val == "managed"):
        return WiFiMode_Managed
    raise Exception("Invalid Wifi mode: " + str(val))

def WiFiRoleToString(val):
    if (val == WiFiRole_Station):
        return "Station"
    if (val == WiFiRole_AccessPoint):
        return "AccessPoint"
    if (val != None):
        return "Unknown (" + str(val)+ ")"
    return None

def ParseWiFiRole(val):
    if isinstance(val, six.integer_types):
        return val
    val = val.lower()
    if (val == "station"):
        return WiFiRole_Station
    if (val == "accesspoint" or val == "access-point"):
        return WiFiRole_AccessPoint
    raise Exception("Invalid Wifi role: " + str(val))

def WiFiSecurityTypeToString(val):
    if (val == WiFiSecurityType_None):
        return "None"
    if (val == WiFiSecurityType_WEP):
        return "WEP"
    if (val == WiFiSecurityType_WPAPersonal):
        return "WPA"
    if (val == WiFiSecurityType_WPA2Personal):
        return "WPA2"
    if (val == WiFiSecurityType_WPA2MixedPersonal):
        return "WPA2Mixed"
    if (val == WiFiSecurityType_WPAEnterprise):
        return "WPAEnterprise"
    if (val == WiFiSecurityType_WPA2Enterprise):
        return "WPA2Enterprise"
    if (val == WiFiSecurityType_WPA2MixedEnterprise):
        return "WPA2MixedEnterprise"
    if (val == WiFiSecurityType_WPA3Personal):
        return "WPA3"
    if (val == WiFiSecurityType_WPA3MixedPersonal):
        return "WPA3Mixed"
    if (val == WiFiSecurityType_WPA3Enterprise):
        return "WPA3Enterprise"
    if (val == WiFiSecurityType_WPA3MixedEnterprise):
        return "WPA3MixedEnterprise"
    if (val != None):
        return "Unknown (" + str(val)+ ")"
    return None

def ParseSecurityType(val):
    val = val.lower()
    if (val == 'none'):
        return WiFiSecurityType_None
    if (val == 'wep'):
        return WiFiSecurityType_WEP
    if (val == 'wpa' or val == 'wpapersonal' or val == 'wpa-personal'):
        return WiFiSecurityType_WPAPersonal
    if (val == 'wpa2' or val == 'wpa2personal' or val == 'wpa2-personal'):
        return WiFiSecurityType_WPA2Personal
    if (val == 'wpa3' or val == 'wpa3personal' or val == 'wpa3-personal'):
        return WiFiSecurityType_WPA3Personal
    if (val == 'wpa2mixed' or val == 'wpa2-mixed' or val == 'wpa2mixedpersonal' or val == 'wpa2-mixed-personal'):
        return WiFiSecurityType_WPA2MixedPersonal
    if (val == 'wpa3mixed' or val == 'wpa3-mixed' or val == 'wpa3mixedpersonal' or val == 'wpa3-mixed-personal'):
        return WiFiSecurityType_WPA3MixedPersonal
    if (val == 'wpaenterprise' or val == 'wpa-enterprise'):
        return WiFiSecurityType_WPAEnterprise
    if (val == 'wpa2enterprise' or val == 'wpa2-enterprise'):
        return WiFiSecurityType_WPA2Enterprise
    if (val == 'wpa3enterprise' or val == 'wpa3-enterprise'):
        return WiFiSecurityType_WPA3Enterprise
    if (val == 'wpa2mixedenterprise' or val == 'wpa2-mixed-enterprise'):
        return WiFiSecurityType_WPA2MixedEnterprise
    if (val == 'wpa3mixedenterprise' or val == 'wpa3-mixed-enterprise'):
        return WiFiSecurityType_WPA3MixedEnterprise
    raise Exception("Invalid Wifi security type: " + str(val))

def DeviceFeatureToString(val):
    if (val == DeviceFeature_HomeAlarmLinkCapable):
        return "HomeAlarmLinkCapable"
    if (val == DeviceFeature_LinePowered):
        return "LinePowered"
    return "0x%08X" % (val)

def OperatingLocationToString(val):
    if val == 1:
        return 'unknown'
    if val == 2:
        return 'indoors'
    if val == 3:
        return 'outdoors'
    raise Exception("Invalid operating location: " + str(val))

def ParseOperatingLocation(val):
    val = val.lower()
    if val == 'unknown':
        return 1
    if val == 'indoors':
        return 2
    if val == 'outdoors':
        return 3
    raise Exception("Invalid operating location: " + str(val))


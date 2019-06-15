#!/usr/bin/env python3

#
#   Copyright (c) 2019 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   @file
#         Tool for factory provisioning of devices running OpenWeave firmware.
#

import os
import sys
import re
import datetime
import binascii 
import base64
import struct
import tempfile
import subprocess
import urllib.request
import ssl
import argparse
import functools
import WeaveTLV
import hashlib
import csv
import plistlib
from functools import reduce

_verbosity = 0

class _UsageError(Exception):
    pass

def _applyDefault(val, defaultVal):
    return val if val is not None else defaultVal

def _prefixLines(str, prefix):
    return re.sub(r'.*', prefix + '\g<0>', str)

def _hexDumpFile(file, prefix):
    any(map(print, (prefix + (' '.join("0x%02X," % b for b in c)) for c in iter(lambda: file.read(16), b""))))
    
def _parseIntArg(val, min=None, max=None, argDesc='integer argument value', base=0):
    try:
        res = int(val, base=base)
        if min != None and res < min:
            res = None
        elif max != None and res > max:
            res = None
    except ValueError:
        res = None
    if res == None:
        raise _UsageError('Invalid %s: %s' % (argDesc, val))
    return res

def _readDataOrFileArg(argName, argValue, isBase64Funct=None):
    if os.path.exists(argValue):
        try:
            with open(argValue, mode='rb') as f:
                argValue = f.read()
            if isBase64Funct != None and isBase64Funct(argValue):
                argValue = base64.b64decode(argValue, validate=True)
        except binascii.Error as ex:
            raise _UsageError('Contents of %s file invalid: %s\nFile name: %s' % (argName, str(ex), deviceCert))
        except IOError as ex:
            raise _UsageError('Unable to read %s file: %s\nFile name: %s' % (argName, str(ex), deviceCert))
    else:
        try:
            argValue = base64.b64decode(deviceCert, validate=True)
        except binascii.Error as ex:
            raise _UsageError('Invalid value specified for %s: %s' % (argName, str(ex)))
    return argValue

class _WipedNamedTemporaryFile(object):
    def __init__(self, mode='w+b', suffix="", prefix=tempfile.template, dir=None):
        if dir is None and os.path.isdir('/dev/shm'):
            dir = '/dev/shm'
        self.file = tempfile.NamedTemporaryFile(mode=mode, suffix=suffix, prefix=prefix, dir=dir, delete=True)
        
    def __getattr__(self, name):
        file = self.__dict__['file']
        return getattr(file, name)

    def __enter__(self):
        self.file.__enter__()
        return self

    def close(self):
        self._wipe()
        self.file.close()

    def __del__(self):
        self._wipe()
        self.file.__del__()

    def __exit__(self, exc, value, tb):
        self._wipe()
        return self.file.__exit__(exc, value, tb)
    
    def _wipe(self):
        if os.path.isfile(self.file.name):
            fileLen = os.stat(self.file.name).st_size
            with open(self.file.name, mode='wb') as f:
                lenWiped = 0
                while lenWiped < fileLen:
                    f.write(b'\0' * 16)
                    lenWiped += 16
                f.truncate()

def isBase64WeaveCert(s):
    return s.startswith(b"1QAABAAB")

def isBase64WeavePrivateKey(s):
    return s.startswith(b"1QAABAAC") or s.startswith(b"1QAABAAD")

def encodeESP32FirmwareImage(segments, entryPoint=0, flashMode=0, flashFreq=0, flashSize=0):
    '''Constructs an ESP32 firmware image.
    
       This format is used by the ESP32 ROM bootloader for flashing firmware image
       and for loading code and data into RAM.
        
       The image format is described here: https://github.com/espressif/esptool/wiki/Firmware-Image-Format.
    '''

    ESP_IMAGE_MAGIC = 0xe9
    
    ESP_CHECKSUM_MAGIC = 0xef
    
    # Encode the image header
    # (magic, # segments, flash mode, flash size/freq, entry point)
    encodedImage = struct.pack('<BBBBI', ESP_IMAGE_MAGIC, len(segments), flashMode, flashSize<<4|flashFreq, entryPoint)
    
    # Encode all zeros for the ESP32 extended header
    encodedImage += b'\0' * 16

    # Encode each of the segments, with their offsets and sizes.
    for (segmentOffset, segmentData) in segments:    
        encodedImage += struct.pack('<II', segmentOffset, len(segmentData))
        encodedImage += segmentData

    # Compute a checksum over all the segment data.
    checksumData = (b for (segmentOffset, segmentData) in segments for b in segmentData)
    checksum = reduce(lambda a,b: a^b, checksumData, ESP_CHECKSUM_MAGIC)

    # Encode trailing padding and checksum byte
    encodedImage += b'\0' * (15 - (len(encodedImage) % 16))
    encodedImage += struct.pack('B', checksum)
    
    return encodedImage

class ProvisioningData(object):
    '''Container for factory provisioning data
    
       Supports reading provisioning data from command line arguments, a provisioning data
       CSV file and/or an HTTP-based provisioning server.
       
       Provides method for encoding provisioning data in a self-identifying format that
       can be detected and consumed by firmware running on the device.
    '''
    
    _marker = '^OW-PROV-DATA^'

    _tag_SerialNumber           = 0    # Serial Number (string)
    _tag_DeviceId               = 1    # Manufacturer-assigned Device Id (unsigned int)
    _tag_DeviceCert             = 2    # Manufacturer-assigned Device Cert (byte string)
    _tag_DevicePrivateKey       = 3    # Manufacturer-assigned Device Key (byte string)
    _tag_PairingCode            = 4    # Pairing Code (string)
    _tag_ProductRev             = 5    # Product Revision (unsigned int)
    _tag_MfgDate                = 6    # Manufacturing Date (string)

    def __init__(self):
        self._fields = { }
        
    @property
    def serialNum(self):
        return self._fields.get(ProvisioningData._tag_SerialNumber, None)
    
    @serialNum.setter
    def serialNum(self, value):
        if value != None and not isinstance(value, str):
            raise ValueError('Invalid value specified for serial number')
        self._setField(ProvisioningData._tag_SerialNumber, value)

    @property
    def deviceId(self):
        return self._fields.get(ProvisioningData._tag_DeviceId, None)
    
    @deviceId.setter
    def deviceId(self, value):
        if value != None and (not isinstance(value, int) or value < 0 or value > 0xFFFFFFFFFFFFFFFF):
            raise ValueError('Invalid value specified for manufacturer-assigned device id')
        self._setField(ProvisioningData._tag_DeviceId, value)

    @property
    def deviceCert(self):
        return self._fields.get(ProvisioningData._tag_DeviceCert, None)
    
    @deviceCert.setter
    def deviceCert(self, value):
        if value != None and not isinstance(value, bytes) and not isinstance(value, bytearray):
            raise ValueError('Invalid value specified for manufacturer-assigned device certificate')
        self._setField(ProvisioningData._tag_DeviceCert, value)

    @property
    def devicePrivateKey(self):
        return self._fields.get(ProvisioningData._tag_DevicePrivateKey, None)
    
    @devicePrivateKey.setter
    def devicePrivateKey(self, value):
        if value != None and not isinstance(value, bytes) and not isinstance(value, bytearray):
            raise ValueError('Invalid value specified for manufacturer-assigned device private key')
        self._setField(ProvisioningData._tag_DevicePrivateKey, value)

    @property
    def pairingCode(self):
        return self._fields.get(ProvisioningData._tag_PairingCode, None)
    
    @pairingCode.setter
    def pairingCode(self, value):
        if value != None and not isinstance(value, str):
            raise ValueError('Invalid value specified for pairing code')
        self._setField(ProvisioningData._tag_PairingCode, value)

    @property
    def productRev(self):
        return self._fields.get(ProvisioningData._tag_ProductRev, None)
    
    @productRev.setter
    def productRev(self, value):
        if value != None and (not isinstance(value, int) or value < 0 or value > 0xFFFF):
            raise ValueError('Invalid value specified for product revision')
        self._setField(ProvisioningData._tag_ProductRev, value)

    @property
    def mfgDate(self):
        return self._fields.get(ProvisioningData._tag_MfgDate, None)
    
    @mfgDate.setter
    def mfgDate(self, value):
        if value != None and not isinstance(value, str):
            raise ValueError('Invalid value specified for manufacturing date')
        try:
            datetime.datetime.strptime(value, '%Y/%m/%d')
        except ValueError:
            raise ValueError('Invalid value specified for manufacturing date: %s' % value)
        self._setField(ProvisioningData._tag_MfgDate, value)

    def encode(self):
        
        encodedProvData = bytearray()
        
        # Pre-encode the data fields in Weave TLV format
        tlvWriter = WeaveTLV.TLVWriter()
        tlvWriter.put(None, self._fields)
        encodedDataFields = tlvWriter.encoding
        
        # Encode the provData marker
        encodedProvData.extend(ProvisioningData._marker.encode('utf-8'))
        
        # Encode the length of the data fields in 32-bit little-endian format.
        encodedProvData.extend(struct.pack('<L', len(encodedDataFields)))
        
        # Append the encoded data fields.
        encodedProvData.extend(encodedDataFields)
        
        # Compute a SHA256 hash of the marker, length and data fields and append
        # it to the encoded provisioning data.
        encodedProvData.extend(hashlib.sha256(encodedProvData).digest())
        
        return encodedProvData
    
    def __str__(self):
        return self.encode()

    @classmethod
    def addArguments(cls, argParser):
        argParser.add_argument('--serial-num', metavar='<string>', help='Set the device serial number.')
        argParser.add_argument('--device-id', metavar='<hex-digits>', help='Set the manufacturer-assigned device id.',
                               type=functools.partial(_parseIntArg, min=0, max=0xFFFFFFFFFFFFFFFF, argDesc='device id', base=16))
        argParser.add_argument('--device-cert', metavar='<base-64>|<file-name>', help='Set the manufacturer-assigned Weave device certificate.')
        argParser.add_argument('--device-key', metavar='<base-64>|<file-name>', help='Set the manufacturer-assigned Weave device private key.')
        argParser.add_argument('--pairing-code', metavar='<string>', help='Set the device pairing code.')
        argParser.add_argument('--product-rev', metavar='<int>', help='Set the product revision for the device.',
                               type=functools.partial(_parseIntArg, min=0, max=0xFFFF, argDesc='product revision'))
        argParser.add_argument('--mfg-date', metavar='<YYYY/MM/DD>|today|now', 
                               help='Set the device\'s manufacturing date.')

    def configFromArguments(self, argValues):
        self.serialNum   = argValues.serial_num
        self.deviceId    = argValues.device_id
        self.pairingCode = argValues.pairing_code
        self.productRev  = argValues.product_rev
        
        mfgDate = argValues.mfg_date
        if mfgDate != None:
            mfgDate = mfgDate.lower()
            if mfgDate == 'today' or mfgDate == 'now':
                mfgDate = datetime.datetime.now().strftime('%Y/%m/%d')
            else:
                mfgDate = mfgDate.replace('-', '/')
                mfgDate = mfgDate.replace('.', '/')
            try:
                self.mfgDate = mfgDate
            except ValueError as ex:
                raise _UsageError(str(ex))

        # Accept device certificate as base-64 string or name of file
        # containing raw or base-64 encoded TLV.
        deviceCert = argValues.device_cert
        if deviceCert != None:
            self.deviceCert = _readDataOrFileArg('device certificate', deviceCert, isBase64WeaveCert)
        
        # Accept device private key as base-64 string or name of file
        # containing raw or base-64 encoded TLV.
        devicePrivateKey = argValues.device_key
        if devicePrivateKey != None:
            self.devicePrivateKey = _readDataOrFileArg('device private key', devicePrivateKey, isBase64WeaveCert)
        
    def configFromProvisioningCSVFile(self, deviceId, fileName):
        deviceIdStr = "%016X" % deviceId
        with open(fileName) as file:
            reader = csv.DictReader(file, dialect=csv.unix_dialect, skipinitialspace=True)
            for row in reader:
                curDeviceId = row.get('MAC', None)
                if curDeviceId == deviceIdStr:
                    try:
                        self.deviceId = deviceId
                        self._configFromProvisioningCSVRow(row)
                    except binascii.Error as ex:
                        raise binascii.Error('Invalid base-64 data in provisioning data CSV file: %s\nFile %s, line %d' % (str(ex), fileName, reader.line_num))
                    return True
        return False

    def configFromProvisioningServer(self, serverBaseURL, deviceId, disableServerValidation=False):
        if disableServerValidation:
            ssl._create_default_https_context = ssl._create_unverified_context
        url = serverBaseURL + '/GetWeaveProvisioningInfo?macAddr=%016X' % deviceId 
        with urllib.request.urlopen(url) as resp:
            respData = resp.read()
        respPlist = plistlib.loads(respData, fmt=plistlib.FMT_XML)
        try:
            if self.deviceCert == None:
                self.deviceCert = base64.b64decode(respPlist['nlWeaveCertificate'], validate=True)
            if self.devicePrivateKey == None:
                self.devicePrivateKey = base64.b64decode(respPlist['nlWeavePrivateKey'], validate=True)
            if self.pairingCode == None:
                self.pairingCode = respPlist['nlWeavePairingCode']
        except binascii.Error as ex:
            raise binascii.Error('Invalid base-64 data returned by provisioning server: %s' % (str(ex)))
                        
    def _configFromProvisioningCSVRow(self, row):
        if self.deviceCert == None:
            deviceCert = row.get('Certificate', None)
            if deviceCert == None:
                raise Exception('"Certificate" column not found in provisioning csv file')
            self.deviceCert = base64.b64decode(deviceCert, validate=True)
        if self.devicePrivateKey == None:
            devicePrivateKey = row.get('Private Key', None)
            if devicePrivateKey == None:
                raise Exception('"Private Key" column not found in provisioning csv file')
            self.devicePrivateKey = base64.b64decode(devicePrivateKey, validate=True)
        if self.pairingCode == None:
            pairingCode = row.get('Pairing Code', None)
            if pairingCode == None:
                raise Exception('"Pairing Code" column not found in provisioning csv file')
            self.pairingCode = pairingCode

    def _setField(self, tag, value):
        if value != None:
            self._fields[tag] = value
        else:
            self._fields.pop(tag, None)
    

class ESPSerialDeviceControler(object):
    '''Supports provisioning ESP32 devices using the ESP32 serial bootloader protocol and
       Espressif's esptool.py command.'''
    
    def __init__(self, target, loadAddr=None):
        self._target = target
        self._esptoolCmd = 'esptool.py'
        self._comPort = target.comPort
        self._comSpeed = target.comSpeed
        self._loadAddr = _applyDefault(loadAddr, target.defaultLoadAddr) 

    def provisionDevice(self, provData):
        
        # Encode the provisioning data
        encodedProvData = provData.encode()
        
        # Verify that the load address is valid and that no part of the provisioning data will
        # fall outside the valid address range for the target device.
        self._target.validateLoadAddress(self._loadAddr, len(encodedProvData))
        
        # Create an ESP32 firmware image consisting of two segments:
        #    - a data segment containing the provisioning data.
        #    - an executable segment containing a small program to reboot the device.
        provImage = encodeESP32FirmwareImage(
                            entryPoint=ESPSerialDeviceControler._rebootProgEntryPoint,
                            segments=[
                                (ESPSerialDeviceControler._rebootProgLoadAddr, ESPSerialDeviceControler._rebootProg),
                                (self._loadAddr, encodedProvData)
                            ]) 

        print('Writing provisioning data to device');            

        with _WipedNamedTemporaryFile(suffix='.bin') as imageFile:
            
            # Write the provisioning image into a temporary file 
            imageFile.write(provImage)
            imageFile.flush()
            
            # Construct a command to invoke the 'load_ram' operation of the esptool, passing the name of
            # of the temporary file as the input.
            esptoolCmd = [
                self._esptoolCmd,
                '--no-stub',
                '--chip', 'esp32',
                '--port', self._comPort,
                '--baud', self._comSpeed,
                'load_ram', imageFile.name
            ]

            if _verbosity > 0:
                print('  Running "%s"' % ' '.join(esptoolCmd))            
            if _verbosity > 1:
                print('  ESP32 Image file (%s):' % imageFile.name)
                imageFile.seek(0)
                _hexDumpFile(imageFile, '    ')

            # Invoke the esptool command and capture its output.
            esptoolProc = subprocess.Popen(esptoolCmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (esptoolOut, esptoolErr) = esptoolProc.communicate()
            esptoolOut = esptoolOut.decode('utf-8')
            esptoolOut = esptoolOut.strip()
            
        if esptoolProc.returncode != 0 or _verbosity > 0:
            print('  esptool command exited with %d' % esptoolProc.returncode)
            print(_prefixLines(esptoolOut, '  | '))
            
        return esptoolProc.returncode == 0

    @classmethod
    def addArguments(cls, argParser):
        argParser.add_argument('--esptool-cmd', metavar='<path-name>', help='Path to esptool command. Defaults to \'esptool.py\'.')
        argParser.add_argument('--port', metavar='<path-name>', help='COM port device name for ESP32. Defaults to /tty/USB0.')
        argParser.add_argument('--speed', metavar='<int>', help='Baud rate for COM port. Defaults to 115200.')
    
    def configFromArguments(self, argValues):
        self._esptoolCmd = _applyDefault(argValues.esptool_cmd, self._esptoolCmd)
        self._comPort    = _applyDefault(argValues.port, self._comPort)
        self._comSpeed   = _applyDefault(argValues.speed, self._comSpeed)

    # Small ESP32 program to initiate a reboot of the device.
    _rebootProg = bytearray([
        0x70, 0x80, 0xF4, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F, 0xA4, 0x80, 0xF4, 0x3F, 0xA1, 0x3A, 0xD8, 0x50,
        0x8C, 0x80, 0xF4, 0x3F, 0x00, 0x0C, 0x00, 0xC0, 0x90, 0x80, 0xF4, 0x3F, 0xF0, 0x49, 0x02, 0x00,
        0xA0, 0x80, 0xF4, 0x3F, 0x00, 0x00, 0x00, 0x80, 0x36, 0x41, 0x00, 0x91, 0xF5, 0xFF, 0x81, 0xF5,
        0xFF, 0xC0, 0x20, 0x00, 0xA8, 0x09, 0x80, 0x8A, 0x10, 0xC0, 0x20, 0x00, 0x89, 0x09, 0x91, 0xF3,
        0xFF, 0x81, 0xF1, 0xFF, 0xC0, 0x20, 0x00, 0x99, 0x08, 0x91, 0xF2, 0xFF, 0x81, 0xF1, 0xFF, 0xC0,
        0x20, 0x00, 0x99, 0x08, 0x91, 0xF2, 0xFF, 0x81, 0xF0, 0xFF, 0xC0, 0x20, 0x00, 0x99, 0x08, 0x91,
        0xF1, 0xFF, 0x81, 0xEF, 0xFF, 0xC0, 0x20, 0x00, 0x99, 0x08, 0x06, 0xFF, 0xFF,
    ])
    _rebootProgLoadAddr = 0x40080000
    _rebootProgEntryPoint = 0x40080028


class JLinkDeviceControler(object):
    '''Supports provisioning devices using a SEGGER J-Link debug probe and SEGGER's JLinkExe command.'''
    
    def __init__(self, target, loadAddr=None):
        self._target = target
        self._jlinkCmd = 'JLinkExe'
        self._jLinkDeviceName = target.jlinkDeviceName
        self._jlinkInterface = target.jlinkInterface
        self._jlinkSpeed = target.jlinkSpeed
        self._jlinkSN = None
        self._loadAddr = _applyDefault(loadAddr, target.defaultLoadAddr) 

    def provisionDevice(self, provData):

        # Encode the provisioning data
        encodedProvData = provData.encode()
        
        # Verify that the load address is valid and that no part of the provisioning data will
        # fall outside the valid address range for the target device.
        self._target.validateLoadAddress(self._loadAddr, len(encodedProvData))
        
        print('Writing provisioning data to device');            

        with tempfile.NamedTemporaryFile(mode='w+') as scriptFile:
            with _WipedNamedTemporaryFile(suffix='.bin') as provDataFile:

                # Write the provisioning data set into a temporary file 
                provDataFile.write(provData.encode())
                provDataFile.flush()

                # Write a command script containing the commands:
                #    r
                #    loadfile <data-set-file> <data-set-address>
                #    go
                #    q
                scriptFile.write('r\nloadfile %s 0x%08X\ngo\nq\n' % (provDataFile.name, self._loadAddr))
                scriptFile.flush()

                jlinkCmd = self._FormJLinkCommand(scriptFile.name)

                if _verbosity > 0:
                    print('  Running "%s"' % ' '.join(jlinkCmd))            
                if _verbosity > 1:
                    print('  Script file (%s):' % scriptFile.name)
                    scriptFile.seek(0)
                    print(_prefixLines(scriptFile.read().strip(), '  | '))
                    print('  Provisioning data file (%s):' % provDataFile.name)
                    provDataFile.seek(0)
                    _hexDumpFile(provDataFile, '    ')

                # Run the jlink command and collect its output.
                jlinkProc = subprocess.Popen(jlinkCmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                (jlinkOut, jlinkErr) = jlinkProc.communicate()
                jlinkOut = jlinkOut.decode('utf-8')
                jlinkOut = jlinkOut.strip()

        if jlinkProc.returncode != 0 or _verbosity > 0:
            print('  J-Link command exited with %d' % jlinkProc.returncode)
            print(_prefixLines(jlinkOut, '  | '))
            
        return jlinkProc.returncode == 0

    def _FormJLinkCommand(self, scriptFileName):
        cmd = [ 
            self._jlinkCmd,
            '-Device', self._jLinkDeviceName,
            '-If', self._jlinkInterface,
            '-Speed', self._jlinkSpeed,
            '-ExitOnError', '1',
            '-AutoConnect', '1'
        ]
        if self._jlinkSN != None:
            cmd += [ '-SelectEmuBySn', self._jlinkSN ]
        cmd += [ '-CommandFile', scriptFileName ]
        return cmd

    @classmethod
    def addArguments(cls, argParser):
        argParser.add_argument('--jlink-cmd', metavar='<path-name>', help='Path to JLink command. Defaults to \'JLinkExe\'.')
        argParser.add_argument('--jlink-if', metavar='SWD|JTAG', help='J-Link interface type. Defaults to SWD.',
                               choices=[ 'SWD', 'JTAG'])
        argParser.add_argument('--jlink-speed', metavar='<int>|adaptive|auto', help='J-Link interface speed.')
        argParser.add_argument('--jlink-sn', metavar='<string>', help='J-Link probe serial number.')

    def configFromArguments(self, argValues):
        self._jlinkCmd       = _applyDefault(argValues.jlink_cmd, self._jlinkCmd)
        self._jlinkInterface = _applyDefault(argValues.jlink_if, self._jlinkInterface)
        self._jlinkSpeed     = _applyDefault(argValues.jlink_speed, self._jlinkSpeed)
        self._jlinkSN        = _applyDefault(argValues.jlink_sn, self._jlinkSN)

class Target(object):
    '''Describes a class of target devices and the parameters needed to control or communicate with them.'''

    def __init__(self, controllerClass, 
                 jlinkDeviceName=None, jlinkInterface=None, jlinkSpeed=None,
                 comPort = None, comSpeed=None, 
                 defaultLoadAddr=None, validAddrRanges=None):
        self.controllerClass = controllerClass
        self.jlinkDeviceName = jlinkDeviceName
        self.jlinkInterface = jlinkInterface
        self.jlinkSpeed = jlinkSpeed
        self.comPort = comPort
        self.comSpeed = comSpeed
        self.defaultLoadAddr = defaultLoadAddr
        self.validAddrRanges = validAddrRanges
        
    def isValidLoadAddress(self, addr, dataLen):
        if self.validAddrRanges != None:
            addrEnd = addr + dataLen
            for (addrRangeStart, addrRangeEnd) in self.validAddrRanges:
                if addr >= addrRangeStart and addr <= addrRangeEnd and addrEnd >= addrRangeStart and addrEnd <= addrRangeEnd:
                    return True
        return False
    
    def validateLoadAddress(self, addr, dataLen):
        if not self.isValidLoadAddress(addr, dataLen):
            raise _UsageError('ERROR: Invalid provisioning data load address\nSome or all of the data would fall outside of the valid memory ranges for the target device');

Target_nrf52840 = Target(controllerClass=JLinkDeviceControler,
                         jlinkDeviceName='NRF52840_XXAA', 
                         jlinkInterface='SWD', 
                         jlinkSpeed='8000',
                         defaultLoadAddr=0x2003E000,
                         validAddrRanges=[
                             (0x20000000, 0x2003FFFF)  # Data RAM region
                         ])

Target_esp32 =    Target(controllerClass=ESPSerialDeviceControler,
                         comPort='/dev/ttyUSB0',
                         comSpeed='115200',
                         defaultLoadAddr=(0x40000000 - 0x400),
                         validAddrRanges=[
                             (0x3FFAE000, 0x3FFFFFFF)  # Internal SRAM 1 + Internal SRAM 2
                         ])

Targets = {
    'nrf52840' : Target_nrf52840,
    'esp32'    : Target_esp32
}

def main():

    try:
        
        class CustomArgumentParser(argparse.ArgumentParser):
            def error(self, message):
                raise _UsageError(message)
        
        argParser = CustomArgumentParser(description='Tool for factory provisioning of devices running OpenWeave firmware.')
        argParser.add_argument('--target', metavar='<string>', help='Target device type.  Choices are: %s' % (', '.join(Targets.keys())))
        argParser.add_argument('--load-addr', metavar='<hex-string>', help='Address in device memory at which provisioning data will be written.',
                               type=functools.partial(_parseIntArg, min=0, max=0xFFFFFFFF, argDesc='provisioning data address'))
        argParser.add_argument('--verbose', '-v', action='count', help='Adjust output verbosity level. Use multiple arguments to increase verbosity.')
        ProvisioningData.addArguments(argParser)
        JLinkDeviceControler.addArguments(argParser)
        ESPSerialDeviceControler.addArguments(argParser)
        argParser.add_argument('--prov-csv-file', metavar='<file-name>', help='Read device provisioning data from a provisioning CSV file.')
        argParser.add_argument('--prov-server', metavar='<url>', help='Read device provisioning data from a provisioning server.')
        argParser.add_argument('--disable-server-validation', action='store_true', help='When using HTTPS, disable validation of the certificate presented by the provisioning server.')
        
        argValues = argParser.parse_args()
        
        if argValues.target == None:
            raise _UsageError('Please specify a target device type using the --target argument.')
        if argValues.target not in Targets:
            raise _UsageError('Unrecognized target device type: %s\nValid device types are: %s' % (argValues.target, ', '.join(Targets.keys())))
        
        if argValues.verbose != None:
            global _verbosity
            _verbosity = argValues.verbose

        # Lookup the target definition object    
        target = Targets[argValues.target]
        
        # Create a ProvisioningData object and initialize it from the command line arguments.
        provData = ProvisioningData()
        provData.configFromArguments(argValues)
        
        # If a provisioning CSV file was given, search the file for an entry matching the given
        # device id.  Set the device certificate, private key and pairing code from the data in
        # the CSV file *unless* any of these values were set on the command line.
        if argValues.prov_csv_file != None:
            if provData.deviceId is None:
                raise _UsageError('Please specify the id for the device')
            try:
                print("Reading provisioning data from CSV file: %s" % argValues.prov_csv_file)
                if not provData.configFromProvisioningCSVFile(provData.deviceId, argValues.prov_csv_file):
                    raise _UsageError('Unable to find provisioning data for device id %016X' % provData.deviceId)
            except IOError as ex:
                raise _UsageError('Unable to read provisioning data CSV file\n%s' % (str(ex)))
            except binascii.Error as ex:
                raise _UsageError(str(ex))

        # If a provisioning server was given, connect to the server and request the device certificate,
        # private key and pairing code.  Use these value unless any of the values were set on the
        # command line. 
        if argValues.prov_server != None:
            if provData.deviceId is None:
                raise _UsageError('Please specify the id for the device')
            print("Fetching provisioning data from provisioning server: %s" % argValues.prov_server)
            provData.configFromProvisioningServer(argValues.prov_server, provData.deviceId, 
                                                  disableServerValidation=argValues.disable_server_validation)
        
        # Instantiate the appropriate type of device controller based on the selected target. 
        devController = target.controllerClass(target=target, loadAddr=argValues.load_addr)
        devController.configFromArguments(argValues)
        
        # Invoke the device controller to provision the device with the given provisioning data.
        if not devController.provisionDevice(provData=provData):
            print('Failed to provision device.', file=sys.stderr)
            return -1
        
        return 0

    except _UsageError as ex:
        print(str(ex), file=sys.stderr)
        return -1

if __name__ == '__main__':
    sys.exit(main())

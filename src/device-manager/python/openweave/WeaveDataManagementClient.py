#
#    Copyright (c) 2019 Google, LLC.
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
#      Python interface for WeaveDataManagementClient
#

"""Weave Data Management Client interface
"""

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
import struct
from threading import Thread, Lock, Event
from ctypes import *
from WeaveStack import *

__all__ = [ 'GenericTraitDataSink', 'WDMClient' ]

TLVType_SignedInteger                   = 0x00
TLVType_UnsignedInteger                 = 0x04
TLVType_Boolean                         = 0x08
TLVType_FloatingPointNumber             = 0x0A
TLVType_UTF8String                      = 0x0C
TLVType_ByteString                      = 0x10
TLVType_Null                            = 0x14
TLVType_Structure                       = 0x15
TLVType_Array                           = 0x16
TLVType_Path                            = 0x17

TLVTagControl_Anonymous                 = 0x00
TLVTagControl_ContextSpecific           = 0x20
TLVTagControl_CommonProfile_2Bytes      = 0x40
TLVTagControl_CommonProfile_4Bytes      = 0x60
TLVTagControl_ImplicitProfile_2Bytes    = 0x80
TLVTagControl_ImplicitProfile_4Bytes    = 0xA0
TLVTagControl_FullyQualified_6Bytes     = 0xC0
TLVTagControl_FullyQualified_8Bytes     = 0xE0

TLVBoolean_False                        = TLVType_Boolean
TLVBoolean_True                         = TLVType_Boolean + 1

TLVEndOfContainer                       = 0x18

INT8_MIN                                = -128
INT16_MIN                               = -32768
INT32_MIN                               = -2147483648
INT64_MIN                               = -9223372036854775808

INT8_MAX                                = 127
INT16_MAX                               = 32767
INT32_MAX                               = 2147483647
INT64_MAX                               = 9223372036854775807

UINT8_MAX                               = 255
UINT16_MAX                              = 65535
UINT32_MAX                              = 4294967295
UINT64_MAX                              = 18446744073709551615

TagControlTypes = {
    0x0:"Anonymous",
    0x1:"Context 1-byte",
    0x2:"Core Profile 2-byte",
    0x3:"Core Profile 4-byte",
    0x4:"Implicit Profile 2-byte",
    0x5:"Implicit Profile 4-byte",
    0x6:"Fully Qualified 6-byte",
    0x7:"Fully Qualified 8-byte",
    "Anonymous":0x0,
    "Context 1-byte":0x1,
    "Core Profile 2-byte":0x2,
    "Core Profile 4-byte":0x3,
    "Implicit Profile 2-byte":0x4,
    "Implicit Profile 4-byte":0x5,
    "Fully Qualified 6-byte":0x6,
    "Fully Qualified 8-byte":0x7,
}

# TLV Types
ElementTypes = {
    0x00:"Signed Integer 1-byte value",
    0x01:"Signed Integer 2-byte value",
    0x02:"Signed Integer 4-byte value",
    0x03:"Signed Integer 8-byte value",
    0x04:"Unsigned Integer 1-byte value",
    0x05:"Unsigned Integer 2-byte value",
    0x06:"Unsigned Integer 4-byte value",
    0x07:"Unsigned Integer 8-byte value",
    0x08:"Boolean False",
    0x09:"Boolean True",
    0x0A:"Floating Point 4-byte value",
    0x0B:"Floating Point 8-byte value",
    0x0C:"UTF-8 String 1-byte length",
    0x0D:"UTF-8 String 2-byte length",
    0x0E:"UTF-8 String 4-byte length",
    0x0F:"UTF-8 String 8-byte length",
    0x10:"Byte String 1-byte length",
    0x11:"Byte String 2-byte length",
    0x12:"Byte String 4-byte length",
    0x13:"Byte String 8-byte length",
    0x14:"Null",
    0x15:"Structure",
    0x16:"Array",
    0x17:"Path",
    0x18:"End of Collection",
    "Signed Integer 1-byte value":0x00,
    "Signed Integer 2-byte value":0x01,
    "Signed Integer 4-byte value":0x02,
    "Signed Integer 8-byte value":0x03,
    "Unsigned Integer 1-byte value":0x04,
    "Unsigned Integer 2-byte value":0x05,
    "Unsigned Integer 4-byte value":0x06,
    "Unsigned Integer 8-byte value":0x07,
    "Boolean True":0x08,
    "Boolean False":0x09,
    "Floating Point 4-byte value":0x0A,
    "Floating Point 8-byte value":0x0B,
    "UTF-8 String 1-byte length":0x0C,
    "UTF-8 String 2-byte length":0x0D,
    "UTF-8 String 4-byte length":0x0E,
    "UTF-8 String 8-byte length":0x0F,
    "Byte String 1-byte length":0x10,
    "Byte String 2-byte length":0x11,
    "Byte String 4-byte length":0x12,
    "Byte String 8-byte length":0x13,
    "Null":0x14,
    "Structure":0x15,
    "Array":0x16,
    "Path":0x17,
    "End of Collection":0x18,
}

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_ConstructBytesArrayFunct                   = CFUNCTYPE(None, c_void_p, c_uint32)

class GenericTraitDataSink:
    def __init__(self, traitInstance, wdmClient):
        self.traitInstance = traitInstance
        self._wdmClient = wdmClient
        self._weaveStack = WeaveStack()
        self._generictraitdatasinkLib = None
        self._containerStack = []
        self._implicitProfile = None
        self._InitLib()

    def __del__(self):
        self.close()

    def refreshData(self):
        res = self._weaveStack.Call(
            lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_RefreshData(self.traitInstance, self._wdmClient, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def close(self):
        if self.traitInstance != None:
            res = self._weaveStack.Call(
                lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_Close(self.traitInstance)
            )
            if (res != 0):
                raise self._weaveStack.ErrorToException(res)
            self.traitInstance = None
            self._weaveStack = None

    def setData(self, path, val, isConditional=False):
        '''Write a value in TLV format with the specified TLV tag.

           val can be a Python object which will be encoded as follows:
           - Python bools, floats and strings are encoded as their respective TLV types.
           - Python ints are encoded as unsigned TLV integers if zero or positive; signed TLV
             integers if negative.
           - None is encoded as a TLV Null.
           - bytes and bytearray objects are encoded as TVL byte strings.
        '''
        self._encoding = bytearray()
        dataBuf = None
        tag = 1
        if val == None:
            self._setNull(tag)
        elif isinstance(val, bool):
            self._setBool(tag, val)
        elif isinstance(val, int):
            if val < 0:
                self._setSignedInt(tag, val)
            else:
                self._setUnsignedInt(tag, val)
        elif isinstance(val, float):
            self._setFloat(tag, val)
        elif isinstance(val, str):
            self._setString(tag, val)
        elif isinstance(val, bytes) or isinstance(val, bytearray):
            self._setLeafBytes(tag, val)
        else:
            raise ValueError('Attempt to TLV encode unsupported value')

        dataBuf = WeaveStack.ByteArrayToVoidPtr(self._encoding)
        dataLen = len(self._encoding) if (self._encoding != None) else 0

        res = self._weaveStack.Call(
            lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetLeafBytes(self.traitInstance, path, dataBuf, dataLen, isConditional)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def _setSignedInt(self, tag, val):
        '''Write a value as a TLV signed integer with the specified TLV tag.'''
        if val >= INT8_MIN and val <= INT8_MAX:
            format = '<b'
        elif val >= INT16_MIN and val <= INT16_MAX:
            format = '<h'
        elif val >= INT32_MIN and val <= INT32_MAX:
            format = '<l'
        elif val >= INT64_MIN and val <= INT64_MAX:
            format = '<q'
        else:
            raise ValueError('Integer value out of range')
        val = struct.pack(format, val)
        controlAndTag = self._encodeControlAndTag(TLVType_SignedInteger, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def _setUnsignedInt(self, tag, val):
        '''Write a value as a TLV unsigned integer with the specified TLV tag.'''
        val = self._encodeUnsignedInt(val)
        controlAndTag = self._encodeControlAndTag(TLVType_UnsignedInteger, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def _setFloat(self, tag, val):
        '''Write a value as a TLV float with the specified TLV tag.'''
        val = struct.pack('d', val)
        controlAndTag = self._encodeControlAndTag(TLVType_FloatingPointNumber, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def _setString(self, tag, val):
        '''Write a value as a TLV string with the specified TLV tag.'''
        val = val.encode('utf-8')
        valLen = self._encodeUnsignedInt(len(val))
        controlAndTag = self._encodeControlAndTag(TLVType_UTF8String, tag, lenOfLenOrVal=len(valLen))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(valLen)
        self._encoding.extend(val)

    def _setLeafBytes(self, tag, val):
        '''Write a value as a TLV byte string with the specified TLV tag.'''
        valLen = self._encodeUnsignedInt(len(val))
        controlAndTag = self._encodeControlAndTag(TLVType_ByteString, tag, lenOfLenOrVal=len(valLen))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(valLen)
        self._encoding.extend(val)

    def _setBool(self, tag, val):
        '''Write a value as a TLV boolean with the specified TLV tag.'''
        if val:
            type = TLVBoolean_True
        else:
            type = TLVBoolean_False
        controlAndTag = self._encodeControlAndTag(type, tag)
        self._encoding.extend(controlAndTag)

    def _setNull(self, tag):
        '''Write a TLV null with the specified TLV tag.'''
        controlAndTag = self._encodeControlAndTag(TLVType_Null, tag)
        self._encoding.extend(controlAndTag)

    def _encodeControlAndTag(self, type, tag, lenOfLenOrVal=0):
        controlByte = type
        if lenOfLenOrVal == 2:
            controlByte |= 1
        elif lenOfLenOrVal == 4:
            controlByte |= 2
        elif lenOfLenOrVal == 8:
            controlByte |= 3
        if tag == None:
            if type != TLVEndOfContainer and len(self._containerStack) != 0 and self._containerStack[0] == TLVType_Structure:
                raise ValueError('Attempt to encode anonymous tag within TLV structure')
            controlByte |= TLVTagControl_Anonymous
            return struct.pack('<B', controlByte)
        if isinstance(tag, int):
            if tag < 0 or tag > UINT8_MAX:
                ValueError('Context-specific TLV tag number out of range')
            controlByte |= TLVTagControl_ContextSpecific
            return struct.pack('<BB', controlByte, tag)
        if isinstance(tag, tuple):
            (profile, tagNum) = tag
            if not isinstance(tagNum, int):
                raise ValueError('Invalid object given for TLV tag')
            if tagNum < 0 or tagNum > UINT32_MAX:
                raise ValueError('TLV tag number out of range')
            if profile != None:
                if not isinstance(profile, int):
                    raise ValueError('Invalid object given for TLV profile id')
                if profile < 0 or profile > UINT32_MAX:
                    raise ValueError('TLV profile id value out of range')
            if len(self._containerStack) != 0 and self._containerStack[0] == TLVType_Array:
                raise ValueError('Attempt to encode profile-specific tag within TLV array')
            if profile == None or profile == self._implicitProfile:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_ImplicitProfile_2Bytes
                    return struct.pack('<BH', controlByte, tagNum)
                else:
                    controlByte |= TLVTagControl_ImplicitProfile_4Bytes
                    return struct.pack('<BL', controlByte, tagNum)
            elif profile == 0:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_CommonProfile_2Bytes
                    return struct.pack('<BH', controlByte, tagNum)
                else:
                    controlByte |= TLVTagControl_CommonProfile_4Bytes
                    return struct.pack('<BL', controlByte, tagNum)
            else:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_FullyQualified_6Bytes
                    return struct.pack('<BLH', controlByte, profile, tagNum)
                else:
                    controlByte |= TLVTagControl_FullyQualified_8Bytes
                    return struct.pack('<BLL', controlByte, profile, tagNum)
        raise ValueError('Invalid object given for TLV tag')

    @staticmethod
    def _encodeUnsignedInt(val):
        if val < 0:
            raise ValueError('Integer value out of range')
        if val <= UINT8_MAX:
            format = '<B'
        elif val <= UINT16_MAX:
            format = '<H'
        elif val <= UINT32_MAX:
            format = '<L'
        elif val <= UINT64_MAX:
            format = '<Q'
        else:
            raise ValueError('Integer value out of range')
        return struct.pack(format, val)

    def setBytes(self, path, value, isConditional=False):
        dataBuf = WeaveStack.ByteArrayToVoidPtr(value)
        dataLen = len(value) if (value != None) else 0

        res = self._weaveStack.Call(
            lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetBytes(self.traitInstance, path, dataBuf, dataLen, isConditional)
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def getData(self, path):
        """
        Unpack weave tlv data from bytestring
        """
        buffer = self.getLeafBytes(path)

        try:
            end_of_collection = False
            item = {}
            while len(buffer) > 0 and not end_of_collection:
                ### Grab control byte and parse it
                control_byte, = struct.unpack("<B", buffer[:1])
                item["tagcontrolkey"] = control_byte >> 5
                item["tagcontrol"] = TagControlTypes[item["tagcontrolkey"]]
                item["typekey"] = control_byte & 0x1f
                item["type"] = ElementTypes[item["typekey"]]
                buffer = buffer[1:]
                ### if we've hit an 'End of Collection' stop reading data
                if item["type"] == "End of Collection":
                    end_of_collection = True

                ### Grab the next few bytes as the tag
                if "1-byte" in item["tagcontrol"]:
                    item["tag"], = struct.unpack("<B", buffer[:1])
                    item["taglength"] = 1
                    buffer = buffer[1:]
                elif "2-byte" in item["tagcontrol"]:
                    item["tag"], = struct.unpack("<H", buffer[:2])
                    item["taglength"] = 2
                    buffer = buffer[2:]
                elif "4-byte" in item["tagcontrol"]:
                    item["tag"], = struct.unpack("<L", buffer[:4])
                    item["taglength"] = 4
                    buffer = buffer[4:]
                elif "8-byte" in item["tagcontrol"]:
                    item["tag"], = struct.unpack("<Q", buffer[:8])
                    item["taglength"] = 8
                    buffer = buffer[8:]
                else:
                    item["tag"] = None
                    item["taglength"] = 0

                ### If the element type needs a length field grab the next bytes as length
                if "length" in item["type"]:
                    if "1-byte" in item["type"]:
                        item["datalength"], = struct.unpack("<B", buffer[:1])
                        item["lenlength"] = 1
                        buffer = buffer[1:]
                    elif "2-byte" in item["type"]:
                        item["datalength"], = struct.unpack("<H", buffer[:2])
                        item["lenlength"] = 2
                        buffer = buffer[2:]
                    elif "4-byte" in item["type"]:
                        item["datalength"], = struct.unpack("<L", buffer[:4])
                        item["lenlength"] = 4
                        buffer = buffer[4:]
                    elif "8-byte" in item["type"]:
                        item["datalength"], = struct.unpack("<Q", buffer[:8])
                        item["lenlength"] = 8
                        buffer = buffer[8:]
                else:
                    item["lenlength"] = 0

                ### Grab the data
                if item["type"] == "Null" or item["type"] == "End of Collection":
                    item["value"] = None
                elif item["type"] == "Boolean True":
                    item["value"] = True
                elif item["type"] == "Boolean False":
                    item["value"] = False
                elif item["type"] == "Unsigned Integer 1-byte value":
                    item["value"], = struct.unpack("<B", buffer[:1])
                    buffer = buffer[1:]
                elif item["type"] == "Signed Integer 1-byte value":
                    item["value"], = struct.unpack("<b", buffer[:1])
                    buffer = buffer[1:]
                elif item["type"] == "Unsigned Integer 2-byte value":
                    item["value"], = struct.unpack("<H", buffer[:2])
                    buffer = buffer[2:]
                elif item["type"] == "Signed Integer 2-byte value":
                    item["value"], = struct.unpack("<h", buffer[:2])
                    buffer = buffer[2:]
                elif item["type"] == "Unsigned Integer 4-byte value":
                    item["value"], = struct.unpack("<L", buffer[:4])
                    buffer = buffer[4:]
                elif item["type"] == "Signed Integer 4-byte value":
                    item["value"], = struct.unpack("<l", buffer[:4])
                    buffer = buffer[4:]
                elif item["type"] == "Unsigned Integer 8-byte value":
                    item["value"], = struct.unpack("<Q", buffer[:8])
                    buffer = buffer[8:]
                elif item["type"] == "Signed Integer 8-byte value":
                    item["value"], = struct.unpack("<q", buffer[:8])
                    buffer = buffer[8:]
                elif item["type"] == "Floating Point 4-byte value":
                    item["value"], = struct.unpack("<f", buffer[:4])
                    buffer = buffer[4:]
                elif item["type"] == "Floating Point 8-byte value":
                    item["value"], = struct.unpack("<d", buffer[:8])
                    buffer = buffer[8:]
                elif "String" in item["type"]:
                    item["value"], = struct.unpack("<%ds" % item["datalength"], buffer[:item["datalength"]])
                    buffer = buffer[item["datalength"]:]

        except Exception:
            ValueError('Invalid object given')
        print item["value"]
        return item["value"]

    def getLeafBytes(self, path):
        def HandleConstructBytesArray(dataBuf, dataLen):
            self._weaveStack.callbackRes = WeaveStack.VoidPtrToByteArray(dataBuf, dataLen)

        cbHandleConstructBytesArray = _ConstructBytesArrayFunct(HandleConstructBytesArray)
        if self.traitInstance != None:
            return self._weaveStack.Call(
                lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetLeafBytes(self.traitInstance, path, cbHandleConstructBytesArray)
            )

    def getBytes(self, path):
        def HandleConstructBytesArray(dataBuf, dataLen):
            self._weaveStack.callbackRes = WeaveStack.VoidPtrToByteArray(dataBuf, dataLen)

        cbHandleConstructBytesArray = _ConstructBytesArrayFunct(HandleConstructBytesArray)
        if self.traitInstance != None:
            return self._weaveStack.Call(
                lambda: self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetBytes(self.traitInstance, path, cbHandleConstructBytesArray)
            )
    # ----- Private Members -----
    def _InitLib(self):
        if (self._generictraitdatasinkLib == None):
            self._generictraitdatasinkLib = CDLL(self._weaveStack.LocateWeaveDLL())
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_RefreshData.argtypes = [ c_void_p, c_void_p, _CompleteFunct, _ErrorFunct ]
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_RefreshData.restype = c_uint32

            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetLeafBytes.argtypes = [ c_void_p, c_char_p, c_void_p, c_uint32, c_bool ]
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetLeafBytes.restype = c_uint32
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetBytes.argtypes = [ c_void_p, c_char_p, c_void_p, c_uint32, c_bool ]
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_SetBytes.restype = c_uint32

            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetLeafBytes.argtypes = [ c_void_p, c_char_p, _ConstructBytesArrayFunct ]
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetLeafBytes.restype = c_uint32
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetBytes.argtypes = [ c_void_p, c_char_p, _ConstructBytesArrayFunct ]
            self._generictraitdatasinkLib.nl_Weave_GenericTraitDataSink_GetBytes.restype = c_uint32

class WDMClient():
    def __init__(self):
        self._wdmClient = None
        self._weaveStack = WeaveStack()
        self._datamanagmentLib = None
        self._traitCatalogue = {}
        self._InitLib()

        binding = c_void_p(None)
        res = self._datamanagmentLib.nl_Weave_DataManagementClient_CreateBinding(pointer(binding), self._weaveStack.devMgr)
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        wdmClient = c_void_p(None)
        res = self._datamanagmentLib.nl_Weave_DataManagementClient_NewWDMClient(pointer(wdmClient), binding)
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        self._datamanagmentLib.nl_Weave_DataManagementClient_ReleaseBinding(binding)

        self._wdmClient = wdmClient

    def __del__(self):
        self.close()

    def close(self):
        if (self._wdmClient != None):
            for item in self._traitCatalogue.items():
                del item
            self._traitCatalogue.clear()

            self._weaveStack.Call(
                lambda: self._datamanagmentLib.nl_Weave_DataManagementClient_DeleteWDMClient(self._wdmClient)
            )
            self._wdmClient = None
            self._weaveStack = None

    def newDataSink(self, resourceType, resourceId, resourceIdLen, profileId, instanceId, path):
        handle = (profileId, instanceId)
        traitInstance = c_void_p(None)
        if (self._wdmClient == None):
            raise ValueError('wdmClient is None')

        res = self._weaveStack.Call(
            lambda: self._datamanagmentLib.nl_Weave_DataManagementClient_NewDataSink(self._wdmClient, resourceType, resourceId, resourceIdLen, profileId, instanceId, path, pointer(traitInstance))
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        if handle not in self._traitCatalogue.keys():
            self._traitCatalogue[handle] =  GenericTraitDataSink(traitInstance, self._wdmClient)

        return self._traitCatalogue[handle]

    def flushUpdate(self):
        if (self._wdmClient == None):
            raise ValueError('wdmClient is None')

        self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_DataManagementClient_FlushUpdate(self._wdmClient, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def refreshData(self):
        if (self._wdmClient == None):
            raise ValueError('wdmClient is None')

        self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_DataManagementClient_RefreshData(self._wdmClient, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    # ----- Private Members -----
    def _InitLib(self):
        if (self._datamanagmentLib == None):
            self._datamanagmentLib = CDLL(self._weaveStack.LocateWeaveDLL())

            self._datamanagmentLib.nl_Weave_DataManagementClient_Init.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_Init.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_Shutdown.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_Shutdown.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_CreateBinding.argtypes = [ POINTER(c_void_p), c_void_p]
            self._datamanagmentLib.nl_Weave_DataManagementClient_CreateBinding.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_ReleaseBinding.argtypes = [ c_void_p]
            self._datamanagmentLib.nl_Weave_DataManagementClient_ReleaseBinding.restype = None

            self._datamanagmentLib.nl_Weave_DataManagementClient_NewWDMClient.argtypes = [ POINTER(c_void_p), c_void_p]
            self._datamanagmentLib.nl_Weave_DataManagementClient_NewWDMClient.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_DeleteWDMClient.argtypes = [ c_void_p ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_DeleteWDMClient.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_NewDataSink.argtypes = [ c_void_p, c_uint16, c_void_p, c_uint32, c_uint32, c_uint64, c_char_p, c_void_p ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_NewDataSink.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_FlushUpdate.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_FlushUpdate.restype = c_uint32

            self._datamanagmentLib.nl_Weave_DataManagementClient_RefreshData.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_DataManagementClient_RefreshData.restype = c_uint32

        res = self._datamanagmentLib.nl_Weave_DataManagementClient_Init()
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

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
#      Python interface for GenericTraitUpdatableDataSink
#


import struct
from ctypes import *
from WeaveStack import *
from WeaveTLV import *


__all__ = [ 'GenericTraitUpdatableDataSink' ]

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
    0x18:"End of Collection"
}

TagControlTypes = {
    0x00:"Anonymous",
    0x20:"Context 1-byte",
    0x40:"Core Profile 2-byte",
    0x60:"Core Profile 4-byte",
    0x80:"Implicit Profile 2-byte",
    0xA0:"Implicit Profile 4-byte",
    0xC0:"Fully Qualified 6-byte",
    0xE0:"Fully Qualified 8-byte"
}

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_ConstructBytesArrayFunct                   = CFUNCTYPE(None, c_void_p, c_uint32)

class GenericTraitUpdatableDataSink:
    def __init__(self, traitInstance, wdmClient):
        self._traitInstance = traitInstance
        self._wdmClient = wdmClient
        self._weaveStack = WeaveStack()
        self._generictraitupdatabledatasinkLib = None
        self._InitLib()

    def __del__(self):
        self.close()
        self._weaveStack = None
        self._generictraitupdatabledatasinkLib = None

    def close(self):
        if self._wdmClient != None:
            self._wdmClient.removeDataSinkRef(self._traitInstance)
            self._wdmClient = None

        if self._traitInstance != None:
            res = self._weaveStack.Call(
                lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_Clear(self._traitInstance)
            )
            if (res != 0):
                raise self._weaveStack.ErrorToException(res)
            self._traitInstance = None

    def clear(self):
        if self._wdmClient != None and self._traitInstance != None:
            res = self._weaveStack.Call(
                lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_Clear(self._traitInstance)
            )
            if (res != 0):
                raise self._weaveStack.ErrorToException(res)

    def refreshData(self):
        if self._traitInstance == None:
            print "traitInstance is not ready"
            return

        self._weaveStack.CallAsync(
            lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_RefreshData(self._traitInstance, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def setData(self, path, val, isConditional=False):
        if self._traitInstance == None:
            print "traitInstance is not ready"
            return
        if (isConditional):
            print "not support conditional update"
            return
        writer = TLVWriter()
        writer.put(None, val)

        dataBuf = WeaveStack.ByteArrayToVoidPtr(writer.encoding)
        dataLen = len(writer.encoding) if (writer.encoding != None) else 0

        res = self._weaveStack.Call(
            lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_SetTLVBytes(self._traitInstance, path, dataBuf, dataLen, isConditional)
        )

        del writer
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def getData(self, path):
        if self._traitInstance == None:
            print "traitInstance is not ready"
            return

        data = self._getTLVBytes(path)
        try :
            return self._getDataHelper(data)
        except:
            raise ValueError('fail to get data')

    def _getDataHelper(self, data):
        """
        Unpack weave tlv data from bytestring
        Todo: create new class tlvreader to process tlv bytes
        """
        # skip 1 byte anonymous structure
        buffer = data[1:]
        item = {}

        ### Grab control byte and parse it
        control_byte, = struct.unpack("<B", buffer[:1])
        controltypeIndex = control_byte & 0x20
        item["tagcontrol"] = TagControlTypes[controltypeIndex]
        elementtypeIndex = control_byte & 0x1f
        item["type"] = ElementTypes[elementtypeIndex]
        buffer = buffer[1:]

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
        else:
            raise ValueError('Attempt to TLV decode unsupported value')

        return item["value"]

    def _getTLVBytes(self, path):
        def HandleConstructBytesArray(dataBuf, dataLen):
            self._weaveStack.callbackRes = WeaveStack.VoidPtrToByteArray(dataBuf, dataLen)

        cbHandleConstructBytesArray = _ConstructBytesArrayFunct(HandleConstructBytesArray)
        if self._traitInstance != None:
            return self._weaveStack.Call(
                lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetTLVBytes(self._traitInstance, path, cbHandleConstructBytesArray)
            )

    def getVersion(self):
        if self._traitInstance != None:
            return self._weaveStack.Call(
                lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetVersion(self._traitInstance)
            )

    # ----- Private Members -----
    def _InitLib(self):
        if (self._generictraitupdatabledatasinkLib == None):
            self._generictraitupdatabledatasinkLib = CDLL(self._weaveStack.LocateWeaveDLL())
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_Clear.argtypes = [ c_void_p ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_Clear.restype = c_uint32
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_RefreshData.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_RefreshData.restype = c_uint32
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_SetTLVBytes.argtypes = [ c_void_p, c_char_p, c_void_p, c_uint32, c_bool ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_SetTLVBytes.restype = c_uint32
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetTLVBytes.argtypes = [ c_void_p, c_char_p, _ConstructBytesArrayFunct ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetTLVBytes.restype = c_uint32
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetVersion.argtypes = [ c_void_p ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetVersion.restype = c_uint64
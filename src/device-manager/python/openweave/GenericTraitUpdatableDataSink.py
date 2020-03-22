#
#    Copyright (c) 2020 Google LLC.
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

from __future__ import absolute_import
from __future__ import print_function
from ctypes import *
from .WeaveUtility import WeaveUtility
from .WeaveStack import *
from .WeaveTLV import *


__all__ = [ 'GenericTraitUpdatableDataSink' ]

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_ConstructBytesArrayFunct                   = CFUNCTYPE(None, c_void_p, c_uint32)

class GenericTraitUpdatableDataSink:
    def __init__(self, resourceIdentifier, profileId, instanceId, path, traitInstance, wdmClient):
        self._resourceIdentifier = resourceIdentifier
        self._profileId = profileId
        self._instanceId = instanceId
        self._path = path
        self._traitInstance = traitInstance
        self._wdmClient = wdmClient
        self._weaveStack = WeaveStack()
        self._generictraitupdatabledatasinkLib = None
        self._InitLib()

    def __del__(self):
        self.close()
        self._weaveStack = None
        self._generictraitupdatabledatasinkLib = None

    @property
    def resourceIdentifier(self):
        return self._resourceIdentifier

    @property
    def profileId(self):
        return self._profileId

    @property
    def instanceId(self):
        return self._instanceId

    @property
    def path(self):
        return self._path

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
        self._ensureNotClosed()
        self._weaveStack.CallAsync(lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_RefreshData(self._traitInstance, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError))

    def setData(self, path, val, isConditional=False):
        self._ensureNotClosed()
        writer = TLVWriter()
        writer.put(None, val)

        dataBuf = WeaveUtility.ByteArrayToVoidPtr(writer.encoding)
        dataLen = len(writer.encoding) if (writer.encoding != None) else 0

        res = self._weaveStack.Call(
            lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_SetTLVBytes(self._traitInstance, path, dataBuf, dataLen, isConditional)
        )

        del writer
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    def getData(self, path):
        self._ensureNotClosed()
        res = self._getTLVBytes(path)

        if isinstance(res, int):
            raise self._weaveStack.ErrorToException(res)

        reader = TLVReader(res)
        out = reader.get()
        try :
            return out["Any"]
        except:
            raise ValueError("fail to get data")

    def _getTLVBytes(self, path):
        def HandleConstructBytesArray(dataBuf, dataLen):
            self._weaveStack.callbackRes = WeaveUtility.VoidPtrToByteArray(dataBuf, dataLen)

        cbHandleConstructBytesArray = _ConstructBytesArrayFunct(HandleConstructBytesArray)
        if self._traitInstance != None:
            return self._weaveStack.Call(
                lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetTLVBytes(self._traitInstance, path, cbHandleConstructBytesArray)
            )

    def getVersion(self):
        self._ensureNotClosed()
        return self._weaveStack.Call(
            lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_GetVersion(self._traitInstance)
        )

    def deleteData(self, path):
        self._ensureNotClosed()
        return self._weaveStack.Call(
            lambda: self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_DeleteData(self._traitInstance, path)
        )

    def _ensureNotClosed(self):
        if self._traitInstance == None:
            raise ValueError("traitInstance is not ready")

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
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_DeleteData.argtypes = [ c_void_p, c_char_p ]
            self._generictraitupdatabledatasinkLib.nl_Weave_GenericTraitUpdatableDataSink_DeleteData.restype = c_uint32

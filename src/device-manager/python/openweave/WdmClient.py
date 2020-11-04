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
#      Python interface for WeaveDataManagementClient
#

from __future__ import absolute_import
from __future__ import print_function
from ctypes import *
from .WeaveStack import *
from .GenericTraitUpdatableDataSink import *
from .ResourceIdentifier import *
from .WeaveUtility import WeaveUtility

__all__ = [ 'WdmClient', 'WdmFlushUpdateStatusStruct', 'WdmClientFlushUpdateError', 'WdmClientFlushUpdateDeviceError' ]
_ConstructBytesArrayFunct                   = CFUNCTYPE(None, c_void_p, c_uint32)
WEAVE_ERROR_STATUS_REPORT = 4044

class WdmFlushUpdateStatusStruct(Structure):
    _fields_ = [
        ("errorCode",       c_uint32),
        ("DeviceStatus",    DeviceStatusStruct),
        ("path",            c_void_p),
        ("pathLen",         c_uint32),
        ("dataSink",        c_void_p),
    ]

class WdmClientFlushUpdateError(WeaveStackError):
    def __init__(self, err, path, dataSink, msg=None):
        super(WdmClientFlushUpdateError, self).__init__(err, msg)
        self.path = path
        self.dataSink = dataSink

    def __str__(self):
        return "WdmClientFlushUpdateError path:{}, trait profileId:{}, trait InstanceId:{}, msg:{} ".format(self.path, self.dataSink.profileId, self.dataSink.instanceId, self.msg)

class WdmClientFlushUpdateDeviceError(DeviceError):
    def __init__(self, profileId, statusCode, systemErrorCode, path, dataSink, msg=None):
        super(WdmClientFlushUpdateDeviceError, self).__init__(profileId, statusCode, systemErrorCode, msg)
        self.path = path
        self.dataSink = dataSink

    def __str__(self):
        return "WdmClientFlushUpdateDeviceError path:{}, trait profileId:{}, trait InstanceId:{}, msg:{} ".format(self.path, self.dataSink.profileId, self.dataSink.instanceId, self.msg)

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_FlushUpdateCompleteFunct                   = CFUNCTYPE(None, c_void_p, c_void_p, c_uint16, POINTER(WdmFlushUpdateStatusStruct))

class WdmClient():
    def __init__(self):
        self._wdmClientPtr = None
        self._weaveStack = WeaveStack()
        self._datamanagmentLib = None
        self._traitTable = {}
        self._InitLib()

        wdmClient = c_void_p(None)
        res = self._datamanagmentLib.nl_Weave_WdmClient_NewWdmClient(pointer(wdmClient), self._weaveStack.devMgr)
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        self._wdmClientPtr = wdmClient

    def __del__(self):
        self.close()
        self._weaveStack = None
        self._datamanagmentLib = None

    def close(self):
        if (self._traitTable != None):
            for key in self._traitTable.keys():
                if self._traitTable[key] != None:
                    val = self._traitTable[key]
                    val.close()
            self._traitTable.clear()
            self._traitTable = None

        if (self._wdmClientPtr != None):
            self._weaveStack.Call(
                lambda: self._datamanagmentLib.nl_Weave_WdmClient_DeleteWdmClient(self._wdmClientPtr)
            )
            self._wdmClientPtr = None

    def setNodeId(self, nodeId):
        self._ensureNotClosed()
        self._weaveStack.Call(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_SetNodeId(self._wdmClientPtr, nodeId)
        )

    def newDataSink(self, resourceIdentifier, profileId, instanceId, path):
        self._ensureNotClosed()
        traitInstance = c_void_p(None)
        _resourceIdentifier = ResourceIdentifierStruct.fromResourceIdentifier(resourceIdentifier)

        res = self._weaveStack.Call(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_NewDataSink(self._wdmClientPtr, _resourceIdentifier, profileId, instanceId, path, pointer(traitInstance))
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        addr = traitInstance.value

        if addr not in self._traitTable.keys():
            self._traitTable[addr] = GenericTraitUpdatableDataSink(resourceIdentifier, profileId, instanceId, path, traitInstance, self)

        return self._traitTable[addr]

    def removeDataSinkRef(self, traitInstance):
        addr = id(traitInstance)
        if addr in self._traitTable.keys():
            self._traitTable[addr] = None

    def flushUpdate(self):
        self._ensureNotClosed()

        def HandleFlushUpdateComplete(appState, reqState, pathCount, statusResultsPtr):
            statusResults = []
            for i in range(pathCount):
                path = WeaveUtility.VoidPtrToByteArray(statusResultsPtr[i].path, statusResultsPtr[i].pathLen).decode()
                addr = statusResultsPtr[i].dataSink
                if addr in self._traitTable.keys():
                    dataSink = self._traitTable[addr]
                else:
                    print("unexpected, trait %d does not exist in traitTable", i)
                    dataSink = None

                if statusResultsPtr[i].errorCode == WEAVE_ERROR_STATUS_REPORT:
                    statusResults.append(WdmClientFlushUpdateDeviceError(statusResultsPtr[i].DeviceStatus.ProfileId, statusResultsPtr[i].DeviceStatus.StatusCode, statusResultsPtr[i].DeviceStatus.SysErrorCode, path, dataSink))
                else:
                    statusResults.append(WdmClientFlushUpdateError(statusResultsPtr[i].errorCode, path, dataSink))
                print(statusResults[-1])

            self._weaveStack.callbackRes = statusResults
            self._weaveStack.completeEvent.set()

        cbHandleComplete = _FlushUpdateCompleteFunct(HandleFlushUpdateComplete)

        return self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_FlushUpdate(self._wdmClientPtr, cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def refreshData(self):
        self._ensureNotClosed()

        return self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_RefreshData(self._wdmClientPtr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def fetchEvents(self, timeoutSec):
        self._ensureNotClosed()

        return self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_FetchEvents(self._wdmClientPtr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError, timeoutSec)
        )

    def getEvents(self):
        self._ensureNotClosed()
        res = self._getEvents()

        if isinstance(res, int):
            raise self._weaveStack.ErrorToException(res)

        return res
    
    def _getEvents(self):
        self._ensureNotClosed()

        def HandleConstructBytesArray(dataBuf, dataLen):
            self._weaveStack.callbackRes = WeaveUtility.VoidPtrToByteArray(dataBuf, dataLen)

        cbHandleConstructBytesArray = _ConstructBytesArrayFunct(HandleConstructBytesArray)
        return self._weaveStack.Call(
            lambda: self._datamanagmentLib.nl_Weave_WdmClient_GetEvents(self._wdmClientPtr, cbHandleConstructBytesArray)
        )

    def _ensureNotClosed(self):
        if (self._wdmClientPtr == None):
            raise ValueError("wdmClient is not ready")

    # ----- Private Members -----
    def _InitLib(self):
        if (self._datamanagmentLib == None):
            self._datamanagmentLib = CDLL(self._weaveStack.LocateWeaveDLL())

            self._datamanagmentLib.nl_Weave_WdmClient_Init.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_WdmClient_Init.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_Shutdown.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_WdmClient_Shutdown.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_SetNodeId.argtypes = [ c_void_p, c_uint64]
            self._datamanagmentLib.nl_Weave_WdmClient_SetNodeId.restype = None

            self._datamanagmentLib.nl_Weave_WdmClient_NewWdmClient.argtypes = [ POINTER(c_void_p), c_void_p]
            self._datamanagmentLib.nl_Weave_WdmClient_NewWdmClient.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_DeleteWdmClient.argtypes = [ c_void_p ]
            self._datamanagmentLib.nl_Weave_WdmClient_DeleteWdmClient.restype = None

            self._datamanagmentLib.nl_Weave_WdmClient_NewDataSink.argtypes = [ c_void_p, POINTER(ResourceIdentifierStruct), c_uint32, c_uint64, c_char_p, c_void_p ]
            self._datamanagmentLib.nl_Weave_WdmClient_NewDataSink.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_FlushUpdate.argtypes = [ c_void_p, _FlushUpdateCompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_WdmClient_FlushUpdate.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_RefreshData.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_WdmClient_RefreshData.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_FetchEvents.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct, c_uint32 ]
            self._datamanagmentLib.nl_Weave_WdmClient_FetchEvents.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WdmClient_GetEvents.argtypes = [ c_void_p, _ConstructBytesArrayFunct ]
            self._datamanagmentLib.nl_Weave_WdmClient_GetEvents.restype = c_uint32

        res = self._datamanagmentLib.nl_Weave_WdmClient_Init()
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

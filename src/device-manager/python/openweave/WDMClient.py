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

from ctypes import *
from WeaveStack import *
from GenericTraitUpdatableDataSink import *
from ResourceIdentifier import *

__all__ = [ 'WDMClient' ]

_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))

class WDMClient():
    def __init__(self):
        self._wdmClientPtr = None
        self._weaveStack = WeaveStack()
        self._datamanagmentLib = None
        self._traitTable = {}
        self._InitLib()

        wdmClient = c_void_p(None)
        res = self._datamanagmentLib.nl_Weave_WDMClient_NewWDMClient(pointer(wdmClient), self._weaveStack.devMgr)
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
                lambda: self._datamanagmentLib.nl_Weave_WDMClient_DeleteWDMClient(self._wdmClientPtr)
            )
            self._wdmClientPtr = None

    def newDataSink(self, resourceIdentifier, profileId, instanceId, path):
        traitInstance = c_void_p(None)
        if (self._wdmClientPtr == None):
            print "wdmClient is not ready"
            return

        _resourceIdentifier = ResourceIdentifierStruct.fromResourceIdentifier(resourceIdentifier)

        res = self._weaveStack.Call(
            lambda: self._datamanagmentLib.nl_Weave_WDMClient_NewDataSink(self._wdmClientPtr, _resourceIdentifier, profileId, instanceId, path, pointer(traitInstance))
        )
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

        addr = id(traitInstance)
        if addr not in self._traitTable.keys():
            self._traitTable[addr] = GenericTraitUpdatableDataSink(traitInstance, self)

        return self._traitTable[addr]

    def removeDataSinkRef(self, traitInstance):
        addr = id(traitInstance)
        if addr in self._traitTable.keys():
            self._traitTable[addr] = None

    def flushUpdate(self):
        if (self._wdmClientPtr == None):
            print "wdmClient is not ready"
            return

        self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_WDMClient_FlushUpdate(self._wdmClientPtr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    def refreshData(self):
        if (self._wdmClientPtr == None):
            print "wdmClient is not ready"
            return

        self._weaveStack.CallAsync(
            lambda: self._datamanagmentLib.nl_Weave_WDMClient_RefreshData(self._wdmClientPtr, self._weaveStack.cbHandleComplete, self._weaveStack.cbHandleError)
        )

    # ----- Private Members -----
    def _InitLib(self):
        if (self._datamanagmentLib == None):
            self._datamanagmentLib = CDLL(self._weaveStack.LocateWeaveDLL())

            self._datamanagmentLib.nl_Weave_WDMClient_Init.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_WDMClient_Init.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_Shutdown.argtypes = [ ]
            self._datamanagmentLib.nl_Weave_WDMClient_Shutdown.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_NewWDMClient.argtypes = [ POINTER(c_void_p), c_void_p]
            self._datamanagmentLib.nl_Weave_WDMClient_NewWDMClient.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_DeleteWDMClient.argtypes = [ c_void_p ]
            self._datamanagmentLib.nl_Weave_WDMClient_DeleteWDMClient.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_NewDataSink.argtypes = [ c_void_p, POINTER(ResourceIdentifierStruct), c_uint32, c_uint64, c_char_p, c_void_p ]
            self._datamanagmentLib.nl_Weave_WDMClient_NewDataSink.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_FlushUpdate.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_WDMClient_FlushUpdate.restype = c_uint32

            self._datamanagmentLib.nl_Weave_WDMClient_RefreshData.argtypes = [ c_void_p, _CompleteFunct, _ErrorFunct ]
            self._datamanagmentLib.nl_Weave_WDMClient_RefreshData.restype = c_uint32

        res = self._datamanagmentLib.nl_Weave_WDMClient_Init()
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

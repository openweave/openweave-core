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
#      Python interface for Weave Stack
#

"""Weave Stack interface
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
import logging
from threading import Thread, Lock, Event
from ctypes import *
from .WeaveUtility import WeaveUtility

__all__ = [ 'DeviceStatusStruct', 'WeaveStackException', 'DeviceError', 'WeaveStackError', 'WeaveStack']

WeaveStackDLLBaseName = '_WeaveDeviceMgr.so'

def _singleton(cls):
    instance = [None]

    def wrapper(*args, **kwargs):
        if instance[0] is None:
            instance[0] = cls(*args, **kwargs)
        return instance[0]

    return wrapper

class DeviceStatusStruct(Structure):
    _fields_ = [
        ("ProfileId",       c_uint32),
        ("StatusCode",      c_uint16),
        ("SysErrorCode",    c_uint32)
    ]

class WeaveStackException(Exception):
    pass

class WeaveStackError(WeaveStackException):
    def __init__(self, err, msg=None):
        self.err = err
        if msg != None:
            self.msg = msg
        else:
            self.msg = "Weave Stack Error %ld" % (err)

    def __str__(self):
        return self.msg

class DeviceError(WeaveStackException):
    def __init__(self, profileId, statusCode, systemErrorCode, msg=None):
        self.profileId = profileId
        self.statusCode = statusCode
        self.systemErrorCode = systemErrorCode
        if (msg == None):
            if (systemErrorCode):
                self.msg = "[ %08X:%d ] (system err %d)" % (profileId, statusCode, systemErrorCode)
            else:
                self.msg = "[ %08X:%d ]" % (profileId, statusCode)
        else:
            self.msg = msg

    def __str__(self):
        return "Device Error: " + self.msg

class LogCategory(object):
    '''Debug logging categories used by openweave.'''
    
    # NOTE: These values must correspond to those used in the openweave C++ code.
    Disabled    = 0
    Error       = 1
    Progress    = 2
    Detail      = 3
    Retain      = 4

    @staticmethod
    def categoryToLogLevel(cat):
        if cat == LogCategory.Error:
            return logging.ERROR
        elif cat == LogCategory.Progress:
            return logging.INFO
        elif cat == LogCategory.Detail:
            return logging.DEBUG
        elif cat == LogCategory.Retain:
            return logging.CRITICAL
        else:
            return logging.NOTSET

class WeaveLogFormatter(logging.Formatter):
    '''A custom logging.Formatter for logging openweave library messages.'''
    def __init__(self, datefmt=None, logModulePrefix=False, logLevel=False, logTimestamp=False, logMSecs=True):
        fmt = '%(message)s'
        if logModulePrefix:
            fmt = 'WEAVE:%(weave-module)s: ' + fmt
        if logLevel:
            fmt = '%(levelname)s:' + fmt
        if datefmt is not None or logTimestamp:
            fmt = '%(asctime)s ' + fmt
        super(WeaveLogFormatter, self).__init__(fmt=fmt, datefmt=datefmt)
        self.logMSecs = logMSecs
        
    def formatTime(self, record, datefmt=None):
        timestamp = record.__dict__.get('timestamp', time.time())
        timestamp = time.localtime(timestamp)
        if datefmt is None:
            datefmt = '%Y-%m-%d %H:%M:%S%z'
        timestampStr = time.strftime('%Y-%m-%d %H:%M:%S%z')
        if self.logMSecs:
            timestampUS = record.__dict__.get('timestamp-usec', 0)
            timestampStr = '%s.%03ld' % (timestampStr, timestampUS / 1000)
        return timestampStr


_CompleteFunct                              = CFUNCTYPE(None, c_void_p, c_void_p)
_ErrorFunct                                 = CFUNCTYPE(None, c_void_p, c_void_p, c_ulong, POINTER(DeviceStatusStruct))
_LogMessageFunct                            = CFUNCTYPE(None, c_int64, c_int64, c_char_p, c_uint8, c_char_p)

@_singleton
class WeaveStack(object):
    def __init__(self, installDefaultLogHandler=True):
        self.networkLock = Lock()
        self.completeEvent = Event()
        self._weaveStackLib = None
        self._weaveDLLPath = None
        self.devMgr = None
        self.callbackRes = None
        self._activeLogFunct = None
        self.addModulePrefixToLogMessage = True

        # Locate and load the openweave shared library.
        self._loadLib()

        # Arrange to log output from the openweave library to a python logger object with the
        # name 'openweave.WeaveStack'.  If desired, applications can override this behavior by
        # setting self.logger to a different python logger object, or by calling setLogFunct()
        # with their own logging function.
        self.logger = logging.getLogger(__name__)
        self.setLogFunct(self.defaultLogFunct)

        # Determine if there are already handlers installed for the logger.  Python 3.5+
        # has a method for this; on older versions the check has to be done manually.  
        if hasattr(self.logger, 'hasHandlers'):
            hasHandlers = self.logger.hasHandlers()
        else:
            hasHandlers = False
            logger = self.logger
            while logger is not None:
                if len(logger.handlers) > 0:
                    hasHandlers = True
                    break
                if not logger.propagate:
                    break
                logger = logger.parent

        # If a logging handler has not already been initialized for 'openweave.WeaveStack',
        # or any one of its parent loggers, automatically configure a handler to log to
        # stdout.  This maintains compatibility with a number of applications which expect
        # openweave log output to go to stdout by default.
        #
        # This behavior can be overridden in a variety of ways:
        #     - Initialize a different log handler before WeaveStack is initialized.
        #     - Pass installDefaultLogHandler=False when initializing WeaveStack.
        #     - Replace the StreamHandler on self.logger with a different handler object.
        #     - Set a different Formatter object on the existing StreamHandler object.
        #     - Reconfigure the existing WeaveLogFormatter object.
        #     - Configure openweave to call an application-specific logging function by
        #       calling self.setLogFunct().
        #     - Call self.setLogFunct(None), which will configure the openweave library
        #       to log directly to stdout, bypassing python altogether.
        #
        if installDefaultLogHandler and not hasHandlers:
            logHandler = logging.StreamHandler(stream=sys.stdout)
            logHandler.setFormatter(WeaveLogFormatter())
            self.logger.addHandler(logHandler)
            self.logger.setLevel(logging.DEBUG)

        def HandleComplete(appState, reqState):
            self.callbackRes = True
            self.completeEvent.set()

        def HandleError(appState, reqState, err, devStatusPtr):
            self.callbackRes = self.ErrorToException(err, devStatusPtr)
            self.completeEvent.set()

        self.cbHandleComplete = _CompleteFunct(HandleComplete)
        self.cbHandleError = _ErrorFunct(HandleError)
        self.blockingCB = None # set by other modules(BLE) that require service by thread while thread blocks.

        # Initialize the openweave library
        res = self._weaveStackLib.nl_Weave_Stack_Init()
        if (res != 0):
            raise self._weaveStack.ErrorToException(res)

    @property
    def defaultLogFunct(self):
        '''Returns a python callable which, when called, logs a message to the python logger object
           currently associated with the WeaveStack object.
           The returned function is suitable for passing to the setLogFunct() method.'''
        def logFunct(timestamp, timestampUSec, moduleName, logCat, message):
            moduleName = WeaveUtility.CStringToString(moduleName)
            message = WeaveUtility.CStringToString(message)
            if self.addModulePrefixToLogMessage:
                message = 'WEAVE:%s: %s' % (moduleName, message) 
            logLevel = LogCategory.categoryToLogLevel(logCat)
            msgAttrs = { 
                'weave-module' : moduleName,
                'timestamp' : timestamp,
                'timestamp-usec' : timestampUSec
            }
            self.logger.log(logLevel, message, extra=msgAttrs)
        return logFunct

    def setLogFunct(self, logFunct):
        '''Set the function used by the openweave library to log messages.
           The supplied object must be a python callable that accepts the following
           arguments:
              timestamp (integer)
              timestampUS (integer)
              module name (encoded UTF-8 string)
              log category (integer)
              message (encoded UTF-8 string)
           Specifying None configures the openweave library to log directly to stdout.'''
        if logFunct is None:
            logFunct = 0
        if not isinstance(logFunct, _LogMessageFunct):
            logFunct = _LogMessageFunct(logFunct)
        with self.networkLock:
            # NOTE: WeaveStack must hold a reference to the CFUNCTYPE object while it is
            # set. Otherwise it may get garbage collected, and logging calls from the
            # openweave library will fail.
            self._activeLogFunct = logFunct
            self._weaveStackLib.nl_Weave_Stack_SetLogFunct(logFunct)

    def Shutdown(self):
        self._weaveStack.Call(
            lambda: self._dmLib.nl_Weave_Stack_Shutdown()
        )
        self.networkLock = None
        self.completeEvent = None
        self._weaveStackLib = None
        self._weaveDLLPath = None
        self.devMgr = None
        self.callbackRes = None

    def Call(self, callFunct):
        # throw error if op in progress
        self.callbackRes = None
        self.completeEvent.clear()
        with self.networkLock:
            res = callFunct()
        self.completeEvent.set()
        if res == 0 and self.callbackRes != None:
            return self.callbackRes
        return res

    def CallAsync(self, callFunct):
        # throw error if op in progress
        self.callbackRes = None
        self.completeEvent.clear()
        with self.networkLock:
            res = callFunct()
    
        if (res != 0):
            self.completeEvent.set()
            raise self.ErrorToException(res)
        while (not self.completeEvent.isSet()):
            if self.blockingCB:
                self.blockingCB()
    
            self.completeEvent.wait(0.05)
        if (isinstance(self.callbackRes, WeaveStackException)):
            raise self.callbackRes
        return self.callbackRes
    
    def ErrorToException(self, err, devStatusPtr=None):
        if (err == 4044 and devStatusPtr):
            devStatus = devStatusPtr.contents
            msg = WeaveUtility.CStringToString((self._weaveStackLib.nl_Weave_Stack_StatusReportToString(devStatus.ProfileId, devStatus.StatusCode)))
            sysErrorCode = devStatus.SysErrorCode if (devStatus.SysErrorCode != 0) else None
            if (sysErrorCode != None):
                msg = msg + " (system err %d)" % (sysErrorCode)
            return DeviceError(devStatus.ProfileId, devStatus.StatusCode, sysErrorCode, msg)
        else:
            return WeaveStackError(err, WeaveUtility.CStringToString((self._weaveStackLib.nl_Weave_Stack_ErrorToString(err))))

    def LocateWeaveDLL(self):
        if self._weaveDLLPath:
            return self._weaveDLLPath

        scriptDir = os.path.dirname(os.path.abspath(__file__))

        # When properly installed in the weave package, the Weave Device Manager DLL will
        # be located in the package root directory, along side the package's
        # modules.
        dmDLLPath = os.path.join(scriptDir, WeaveStackDLLBaseName)
        if os.path.exists(dmDLLPath):
            self._weaveDLLPath = dmDLLPath
            return self._weaveDLLPath

        # For the convenience of developers, search the list of parent paths relative to the
        # running script looking for an OpenWeave build directory containing the Weave Device
        # Manager DLL. This makes it possible to import and use the WeaveDeviceMgr module
        # directly from a built copy of the OpenWeave source tree.
        buildMachineGlob = '%s-*-%s*' % (platform.machine(), platform.system().lower())
        relDMDLLPathGlob = os.path.join('build', buildMachineGlob, 'src/device-manager/python/.libs', WeaveStackDLLBaseName)
        for dir in self._AllDirsToRoot(scriptDir):
            dmDLLPathGlob = os.path.join(dir, relDMDLLPathGlob)
            for dmDLLPath in glob.glob(dmDLLPathGlob):
                if os.path.exists(dmDLLPath):
                    self._weaveDLLPath = dmDLLPath
                    return self._weaveDLLPath

        raise Exception("Unable to locate Weave Device Manager DLL (%s); expected location: %s" % (WeaveStackDLLBaseName, scriptDir))

    # ----- Private Members -----
    def _AllDirsToRoot(self, dir):
        dir = os.path.abspath(dir)
        while True:
            yield dir
            parent = os.path.dirname(dir)
            if parent == '' or parent == dir:
                break
            dir = parent

    def _loadLib(self):
        if (self._weaveStackLib == None):
            self._weaveStackLib = CDLL(self.LocateWeaveDLL())
            self._weaveStackLib.nl_Weave_Stack_Init.argtypes = [ ]
            self._weaveStackLib.nl_Weave_Stack_Init.restype = c_uint32
            self._weaveStackLib.nl_Weave_Stack_Shutdown.argtypes = [ ]
            self._weaveStackLib.nl_Weave_Stack_Shutdown.restype = c_uint32
            self._weaveStackLib.nl_Weave_Stack_StatusReportToString.argtypes = [ c_uint32, c_uint16 ]
            self._weaveStackLib.nl_Weave_Stack_StatusReportToString.restype = c_char_p
            self._weaveStackLib.nl_Weave_Stack_ErrorToString.argtypes = [ c_uint32 ]
            self._weaveStackLib.nl_Weave_Stack_ErrorToString.restype = c_char_p
            self._weaveStackLib.nl_Weave_Stack_SetLogFunct.argtypes = [ _LogMessageFunct ]
            self._weaveStackLib.nl_Weave_Stack_SetLogFunct.restype = c_uint32

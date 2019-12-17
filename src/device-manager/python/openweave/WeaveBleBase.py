#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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
#      This file is Weave BLE Base class file
#

from __future__ import absolute_import
import abc
import shlex
import optparse
from optparse import OptionParser, Option, OptionValueError
import six

class WeaveBleBase(six.with_metaclass(abc.ABCMeta, object)):
    @abc.abstractmethod
    def scan(self, line):
        """ API to initiatae BLE scanning for -t user_timeout seconds."""
        return

    @abc.abstractmethod
    def SubscribeBleCharacteristic(self, connObj, svcId, charId, subscribe):
        """ Called by Weave to (un-)subscribe to a characteristic of a service."""
        return

    @abc.abstractmethod
    def WriteBleCharacteristic(self, connObj, svcId, charId, buffer, length):
        """ Called by WeaveDeviceMgr.py to satisfy a request by Weave to transmit a packet over BLE."""
        return

    @abc.abstractmethod
    def scan_connect(self, line):
        """ API to perform both scan and connect operations in one call."""
        return

    @abc.abstractmethod
    def disconnect(self):
        """ API to initiate BLE disconnect procedure."""
        return

    @abc.abstractmethod
    def connect(self, identifier):
        """ API to initiate BLE connection to peripheral device whose identifier == identifier."""
        return

    @abc.abstractmethod
    def CloseBle(self, connObj):
        """ Called by Weave to close the BLE connection."""
        return

    @abc.abstractmethod
    def GetBleEvent(self):
        """ Called by WeaveDeviceMgr.py on behalf of Weave to retrieve a queued message."""
        return

    @abc.abstractmethod
    def devMgrCB(self):
        """ A callback used by WeaveDeviceMgr.py to drive the runloop while the
            main thread waits for the Weave thread to complete its operation."""
        return

    @abc.abstractmethod
    def readlineCB(self):
        """ A callback used by readline to drive the runloop while the main thread
            waits for commandline input from the user."""
        return

    @abc.abstractmethod
    def setInputHook(self, hookFunc):
        """Set the PyOS_InputHook to call the specific function."""
        return

    @abc.abstractmethod
    def runLoopUntil(self, should_tuple):
        """ helper function to drive runloop until an expected event is received or
            the timeout expires."""
        return

    def Usage(self, cmd):
        if cmd == None:
            return

        line = "USAGE: "

        if cmd == "scan":
            line += "ble-scan [-t <timeout>] [<name>|<identifier>] [-q <quiet>]"
        elif cmd == "scan-connect":
            line += "ble-scan-connect [-t <timeout>] <name> [-q <quiet>]"

        self.logger.info(line)

    def ParseInputLine(self, line, cmd=None):
        args = shlex.split(line)
        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE)

        if cmd == "scan" or cmd == "scan-connect":
            optParser.add_option("-t", "--timeout", action="store", dest="timeout", type="float", default=10.0)
            optParser.add_option("-q", "--quiet", action="store_true", dest="quiet")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            self.Usage(cmd)
            return None

        if cmd == None:
            return remainingArgs

        if len(remainingArgs) > 1:
            self.Usage(cmd)
            return None

        name = None

        if len(remainingArgs):
            name = str(remainingArgs[0])
        elif cmd == "scan-connect":
            self.Usage(cmd)
            return None

        return (options.timeout, options.quiet, name)
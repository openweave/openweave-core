#!/usr/bin/env python


#
#    Copyright (c) 2018 Nest Labs, Inc.
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
#       Implements WeaveBleCentral class that tests Weave Device manager among Weave Nodes.
#

import os
import pexpect
import sys
import time

chatTimeout = 40


class WeaveDeviceManagerConsole(object):
    """
    This class is the python interface to the weave device manager
    """

    def __init__(self, logfd=sys.stdout):
        weaveDeviceMgrPath = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..", "build", "x86_64-unknown-linux-gnu", "src", "device-manager", "python"))
        self.app_prompt = 'weave-device-mgr.*? >'
        app = "sudo " + weaveDeviceMgrPath + "/weave-device-mgr"
        environment = os.environ.copy()
        self.app = pexpect.spawn(app, logfile=logfd, env=environment)
        self.app.delaybeforesend = 0
        self.app.expect([self.app_prompt, pexpect.TIMEOUT], timeout=-1)

    def close(self):
        if hasattr(self, 'app'):
            self.app.sendline('exit')
            try:
                self.app.expect(pexpect.EOF, timeout=10)
            except pexpect.TIMEOUT:
                self.app.close(force=True)

    def __del__(self):
        self.close()

    def chat(self, cmd, timeout=chatTimeout, interrupt_on_timeout=False, force=True, expect=None):
        self.app.send(cmd)
        if not force:
            self.app.expect_exact(cmd, timeout=2)
        self.app.send('\n')

        if expect:
            index = self.app.expect(
                [expect, pexpect.TIMEOUT],
                timeout=timeout
            )
        else:
            index = self.app.expect(
                [self.app_prompt, pexpect.TIMEOUT],
                timeout=timeout
            )
        buf = self.app.before
        dummy, rest = buf.split('\r\n', 1)
        if index == 0:
            return rest
        elif interrupt_on_timeout:
            print "Timeout, sending Ctrl-C"
            self.app.sendintr()
            self.app.expect(self.app_prompt, timeout=1)
            return rest
        else:
            raise pexpect.TIMEOUT(
                "Timeout waiting for response to command '%s'" % cmd
            )


class WeaveBleCentral(object):
    def __init__(self, src, dst):
        self.src = src
        self.dst = dst
        self.devMgr = WeaveDeviceManagerConsole()
        output = self.devMgr.chat("ble-adapter-print")

    def startBleConnection(self):
        # TODO: In WeaveBluezMgr.py, sometimes bluez would stuck in find_characteristic. Need fix later in bluez.
        attempts = 0
        while attempts < 3:
            try:
                self.devMgr.chat("ble-adapter-select " + self.src)
                self.devMgr.chat("ble-scan-connect -t 20 " + self.dst , expect="connect success")
                break
            except:
                attempts += 1
                if attempts == 3:
                    raise

    def testWoblePase(self):
        self.startBleConnection()
        self.devMgr.chat("connect -b -p TEST", expect="Secure session established")
        self.devMgr.chat("arm-fail-safe reset", expect="Arm fail-safe complete")
        self.devMgr.chat("ping", expect="Ping complete")
        self.devMgr.chat("identify", expect="HomeAlarmLinkCapable LinePowered")
        self.devMgr.chat("disarm-fail-safe", expect="Disarm fail-safe complete")
        self.devMgr.chat("close", expect="Closing endpoints")
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        time.sleep(5)

    def testWoblePaseFailure(self):
        self.startBleConnection()
        self.devMgr.chat("connect -b -p TEST1", expect="Secure session failed")
        self.devMgr.chat("close", expect="Closing endpoints")
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        time.sleep(5)

    def testWobleNoEncrption(self):
        self.startBleConnection()
        self.devMgr.chat("connect -b", expect="Connected to device")
        self.devMgr.chat("ping", expect="Ping complete")
        self.devMgr.chat("close", expect="BLE connection")
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        time.sleep(5)

    def testWobleNoEncrptionWithAuthFailure(self):
        self.startBleConnection()
        self.devMgr.chat("connect -b", expect="Connected to device")
        self.devMgr.chat("arm-fail-safe reset", expect="Authentication required")
        self.devMgr.chat("close", expect="BLE connection")
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        time.sleep(5)

    def testWobleConnectionEstablishError(self):
        self.startBleConnection()
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        self.devMgr.chat("connect -b", expect="Ble Error 6007: GATT write characteristic operation failed")
        self.devMgr.chat("close", expect="Closing endpoints")
        time.sleep(5)

    def testWobleConnectionError(self):
        self.startBleConnection()
        self.devMgr.chat("connect -b", expect="Connected to device")
        self.devMgr.chat("ble-disconnect", expect="disconnected")
        self.devMgr.chat("ping", expect="WeaveDie WeaveDie WeaveDie")
        time.sleep(5)

    def startwobleTest(self):
        self.testWoblePase()
        self.testWoblePaseFailure()
        self.testWobleNoEncrption()
        self.testWobleNoEncrptionWithAuthFailure()
        self.testWobleConnectionEstablishError()
        self.testWobleConnectionError()

if __name__ == "__main__":
    total = len(sys.argv)
    cmdargs = str(sys.argv)
    print ("The total numbers of args passed to the WeaveBleCentral: %d " % total)
    print ("Args list: %s " % cmdargs)
    print ("src argument: %s" % str(sys.argv[1]))
    print ("dst argument: %s" % str(sys.argv[2]))
    bleCentral = WeaveBleCentral(str(sys.argv[1]), str(sys.argv[2]))
    bleCentral.startwobleTest()
    time.sleep(10)
    print "WoBLE central is good to go"

#!/usr/bin/env python

#
#    Copyright (c) 2013-2018 Nest Labs, Inc.
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
#      This file implements a CLI for the Python-based Weave Device Manager
#

import sys
import os
import platform
import optparse
from optparse import OptionParser, Option, OptionValueError
import binascii
import shlex
import base64
import random
import textwrap
import string
from copy import copy
from cmd import Cmd
from string import lower

# Attempt to import the Weave Device Manager Python module. We might
# be lucky and find the module sitting in the same directory. More
# than likely, however, Python will need to look for it relative to
# the directory this script is in or relative to the WEAVE_HOME
# environment variable.
#
# The relative path will either likely be in:
#
#     lib/python/PACKAGE
#
# or:
#
#     lib/pythonPYTHON_VERSION/dist-packages/PACKAGE
#
# or:
#
#     lib/pythonPYTHON_VERSION/site-packages/PACKAGE

try:
    import WeaveDeviceMgr
except Exception:
    pyversion = sys.version[:3]

    pkgpythondirs = [ "python",
                      "lib/python/weave",
                      "lib/python" + pyversion + "/dist-packages/weave",
                      "lib/python" + pyversion + "/site-packages/weave" ]

    for pkgpythondir in pkgpythondirs:
        if os.environ.has_key("WEAVE_HOME"):
            pyweavepath = os.path.normpath(os.path.join(os.environ["WEAVE_HOME"], pkgpythondir))
        else:
            pyweavepath = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", pkgpythondir))

        if os.path.exists(pyweavepath):
            # The path exists in the current SDK, go ahead and append it to
            # the Python module search path.

            sys.path.append(pyweavepath)

    # At this point, we've added any possible paths to search for
    # Python modules. Give it one last try.

    try:
        import WeaveDeviceMgr
    except Exception:
        print "Could not find the WeaveDeviceMgr module!"

        sys.exit(1)

# After solving the path problem for WeaveDeviceMgr above now try and import WeaveBleMgr.
if platform.system() == 'Darwin':
    from WeaveCoreBluetoothMgr import CoreBluetoothManager as BleManager
    from WeaveBleUtility import FAKE_CONN_OBJ_VALUE

if sys.platform.startswith('linux'):
    from WeaveBluezMgr import BluezManager as BleManager
    from WeaveBleUtility import FAKE_CONN_OBJ_VALUE


# Dummy Service Configuration
#
# The dummy service configuration object below contains the following information:
#
#    Trusted Certificates:
#        The Nest Development Root Certificate
#        A dummy "account" certificate with a common name of "DUMMY-ACCOUNT-ID" (see below)
#
#    Directory End Point:
#        Endpoint Id: 18B4300200000001 (the service directory endpoint)
#        Endpoint Host Name: frontdoor.integration.nestlabs.com
#        Endpoint Port: 11095 (the Weave default port)
#
# The dummy account certificate is:
#
#    1QAABAABADABCE4vMktB1zrbJAIENwMsgRBEVU1NWS1BQ0NPVU5ULUlEGCYEy6j6GyYFSzVPQjcG
#    LIEQRFVNTVktQUNDT1VOVC1JRBgkBwImCCUAWiMwCjkEK9nbWmLvurFTKg+ZY7eKMMWKQSmlGU5L
#    C/N+2sXpszXwdRhtSV2GxEQlB0G006nv7rQq1gpdneA1gykBGDWCKQEkAgUYNYQpATYCBAIEARgY
#    NYEwAghCPJVfRh5S2xg1gDACCEI8lV9GHlLbGDUMMAEdAIphhmI9F7LSz9JtOT3kJWngkeoFanXO
#    3UXrg88wAhx0tCukbRRlt7dxmlqvZNKIYG6zsaAxypJvyvJDGBg=
#
# The corresponding private key is:
#
#    1QAABAACACYBJQBaIzACHLr840+Gv3w4EnAr+aMQv0+b8+8wD6VETUI6Z2owAzkEK9nbWmLvurFT
#    Kg+ZY7eKMMWKQSmlGU5LC/N+2sXpszXwdRhtSV2GxEQlB0G006nv7rQq1gpdneAY
#

dummyServiceConfig = (
    "1QAADwABADYBFTABCE4vMktB1zrbJAIENwMsgRBEVU1NWS1BQ0NPVU5ULUlEGCYEy6j6GyYFSzVP" +
    "QjcGLIEQRFVNTVktQUNDT1VOVC1JRBgkBwImCCUAWiMwCjkEK9nbWmLvurFTKg+ZY7eKMMWKQSml" +
    "GU5LC/N+2sXpszXwdRhtSV2GxEQlB0G006nv7rQq1gpdneA1gykBGDWCKQEkAgUYNYQpATYCBAIE" +
    "ARgYNYEwAghCPJVfRh5S2xg1gDACCEI8lV9GHlLbGDUMMAEdAIphhmI9F7LSz9JtOT3kJWngkeoF" +
    "anXO3UXrg88wAhx0tCukbRRlt7dxmlqvZNKIYG6zsaAxypJvyvJDGBgVMAEJAKg0IunZdeRVJAIE" +
    "VwMAJxMBAADu7jC0GBgmBJUjqRkmBRXB0ixXBgAnEwEAAO7uMLQYGCQHAiQIFTAKMQR4UuKckrpw" +
    "GVhGba4Yckr7Qw32BykzDWFV5WVGjroNpT+1F8BHZEQCGE+oESRQ1Hs1gykBKQIYNYIpASQCYBg1" +
    "gTACCEIMrPa0ZHHmGDWAMAIIQgys9rRkceYYNQwwARkAvg7aoWNajvFSF0WAvdyUEtTMHCwzTinc" +
    "MAIZAIvn7i4RFxSuktorO20v112eX824ui9ldhgYGDUCJwEBAAAAAjC0GDYCFSwBImZyb250ZG9v" +
    "ci5pbnRlZ3JhdGlvbi5uZXN0bGFicy5jb20YGBgY"
)

# Dummy Access Token
#
# The following fabric access token contains the dummy account certificate and
# private key described above. This can be used to authenticate to the mock device
# when it has been configured to use the dummy service config.
#
dummyAccessToken = (
    "1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C" +
    "NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ" +
    "TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB" +
    "GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq" +
    "dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/" +
    "fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz" +
    "NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY"
)



def DecodeBase64Option(option, opt, value):
    try:
        return base64.standard_b64decode(value)
    except TypeError:
        raise OptionValueError(
            "option %s: invalid base64 value: %r" % (opt, value))

def DecodeHexIntOption(option, opt, value):
    try:
        return int(value, 16)
    except ValueError:
        raise OptionValueError(
            "option %s: invalid value: %r" % (opt, value))

class ExtendedOption (Option):
    TYPES = Option.TYPES + ("base64", "hexint", )
    TYPE_CHECKER = copy(Option.TYPE_CHECKER)
    TYPE_CHECKER["base64"] = DecodeBase64Option
    TYPE_CHECKER["hexint"] = DecodeHexIntOption


class DeviceMgrCmd(Cmd):

    def __init__(self, rendezvousAddr=None):
        self.lastNetworkId = None

        Cmd.__init__(self)

        Cmd.identchars = string.ascii_letters + string.digits + '-'

        if (sys.stdin.isatty()):
            self.prompt = "weave-device-mgr > "
        else:
            self.use_rawinput = 0
            self.prompt = ""

        DeviceMgrCmd.command_names.sort()

        self.bleMgr = None

        self.devMgr = WeaveDeviceMgr.WeaveDeviceManager()

        if (rendezvousAddr):
            try:
                self.devMgr.SetRendezvousAddress(rendezvousAddr)
            except WeaveDeviceMgr.DeviceManagerException, ex:
                print str(ex)
                return

        self.historyFileName = os.path.expanduser("~/.weave-device-mgr-history")

        try:
            import readline
            if 'libedit' in readline.__doc__:
                readline.parse_and_bind("bind ^I rl_complete")
            readline.set_completer_delims(' ')
            try:
                readline.read_history_file(self.historyFileName)
            except IOError:
                pass
        except ImportError:
            pass


    command_names = [
        'close',
        'start-device-enumeration',
        'stop-device-enumeration',
        'connect',
        'rendezvous',
        'passive-rendezvous',
        'remote-passive-rendezvous',
        'reconnect',
        'scan-networks',
        'add-wifi-network',
        'add-thread-network',
        'update-network',
        'remove-network',
        'get-networks',
        'get-camera-auth-data',
        'enable-network',
        'disable-network',
        'test-network',
        'set-rendezvous-mode',
        'ping',
        'identify',
        'create-fabric',
        'create-thread-network',
        'leave-fabric',
        'get-fabric-config',
        'join-existing-fabric',
        'register-service',
        'update-service',
        'unregister-service',
        'arm-fail-safe',
        'disarm-fail-safe',
        'reset-config',
        'set-rendezvous-addr',
        'quit',
        'history',
        'help',
        'enable-connection-monitor',
        'disable-connection-monitor',
        'get-last-network-provisioning-result',
        'set-auto-reconnect',
        'set-log-output',
        'set-rendezvous-link-local',
        'set-connect-timeout',
        'ble-scan',
        'ble-connect',
        'ble-disconnect',
        'ble-read',
        'get-active-locale',
        'set-active-locale',
        'get-available-locales',
        'thermostat-get-entry-key',
        'thermostat-system-test-status',
        'start-system-test',
        'stop-system-test',
        'ble-scan-connect',
        'pair-token',
        'unpair-token',
        'ble-adapter-select',
        'ble-adapter-print',
        'ble-debug-log',
        'ble-diag-test',
        'ble-diag-test-result',
        'ble-diag-test-abort',
        'ble-diag-test-timing'
    ]

    def parseline(self, line):
        cmd, arg, line = Cmd.parseline(self, line)
        if (cmd):
            cmd = self.shortCommandName(cmd)
            line = cmd + ' ' + arg
        return cmd, arg, line

    def completenames(self, text, *ignored):
        return [ name + ' ' for name in DeviceMgrCmd.command_names if name.startswith(text) or self.shortCommandName(name).startswith(text) ]

    def shortCommandName(self, cmd):
        return cmd.replace('-', '')

    def precmd(self, line):
        if (not self.use_rawinput and line != 'EOF' and line != ''):
            print '>>> ' + line
        return line

    def postcmd(self, stop, line):
        if (not stop and self.use_rawinput):
            if (self.devMgr.IsConnected()):
                self.prompt = "weave-device-mgr (%X @ %s) > " % (self.devMgr.DeviceId(), self.devMgr.DeviceAddress())
            else:
                self.prompt = "weave-device-mgr > "
        return stop

    def postloop(self):
        try:
            import readline
            try:
                readline.write_history_file(self.historyFileName)
            except IOError:
                pass
        except ImportError:
            pass

    def do_help(self, line):
        if (line):
            cmd, arg, unused = self.parseline(line)
            try:
                doc = getattr(self, 'do_' + cmd).__doc__
            except AttributeError:
                doc = None
            if doc:
                self.stdout.write("%s\n" % textwrap.dedent(doc))
            else:
                self.stdout.write("No help on %s\n" % (line))
        else:
            self.print_topics("\nAvailable commands (type help <name> for more information):", DeviceMgrCmd.command_names, 15,80)

    def do_startdeviceenumeration(self, line):
        """
        start-device-enumeration [ <options> ]

        Start enumerating Weave devices.

        Options:

          --target-fabric <fabric-id>

            Specifies that only devices that are members of the specified Weave fabric
            should respond. A value of 0 specifies that only devices that are not a member
            of a fabric should respond. A value of FFFFFFFFFFFFFFF (the default)
            specifies that all devices should respond regardless of fabric membership.

          --target-modes <modes>

            Specifies that only devices that are currently in the specified modes
            should respond. <modes> is a bit-field with the following values:

              0 - Locate all devices regardless of mode.
              1 - Locate all devices in 'user-selected' mode -- i.e., where the device
                  has has been directly identified by a user, e.g., by pressing a button.

          --target-vendor <vendor-id>

            Specifies that only devices manufactured by the given vendor should
            respond to the identify request. A value of FFFF specifies any vendor.

          --target-product <product-id>

            Specifies that only devices with the given product id should respond.
            A value of 0xFFFF specifies any product. If the --target-product is specified,
            then the --target-vendor must also be specified.

          --target-device <node-id>

            Specifies that only the device with the given Weave node id should respond.
            A value of 0xFFFFFFFFFFFFFFFF (the default) specifies that all devices should
            respond.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("", "--target-fabric", action="store", dest="targetFabricId", type="hexint", default=WeaveDeviceMgr.TargetFabricId_AnyFabric)
        optParser.add_option("", "--target-modes", action="store", dest="targetModes", type="hexint", default=WeaveDeviceMgr.TargetDeviceMode_Any)
        optParser.add_option("", "--target-vendor", action="store", dest="targetVendorId", type="hexint", default=WeaveDeviceMgr.TargetVendorId_Any)
        optParser.add_option("", "--target-product", action="store", dest="targetProductId", type="hexint", default=WeaveDeviceMgr.TargetProductId_Any)
        optParser.add_option("", "--target-device", action="store", dest="targetDeviceId", type="hexint", default=WeaveDeviceMgr.TargetDeviceId_Any)

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 1):
            print "Unexpected argument: " + remainingArgs[1]
            return

        try:
            self.devMgr.StartDeviceEnumeration(targetFabricId=options.targetFabricId,
                                         targetModes=options.targetModes,
                                         targetVendorId=options.targetVendorId,
                                         targetProductId=options.targetProductId,
                                         targetDeviceId=options.targetDeviceId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Started device enumeration"

    def do_stopdeviceenumeration(self, line):
        """
        stop-device-enumeration

        Stop enumerating Weave devices.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[2]
            return

        try:
            self.devMgr.StopDeviceEnumeration()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Stopped device enumeration"

    def do_connect(self, line):
        """
        connect [ <options> ] <ip-address> [ <node-id> ]

        Connect to a device at the specified IP address. If <node-id> is not specified, a
        node id of 1 will be used.

        Options:

          --pairing-code <pairing-code>

            Specifies a pairing code string to be used to authenticate the device.

          --access-token <access-token>

            Specifies a base-64 encoded access token to be used to authenticate the device.

          --use-dummy-access-token

            Authenticate using the built-in dummy access token.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-p", "--pairing-code", action="store", dest="pairingCode", type="string")
        optParser.add_option("-a", "--access-token", action="store", dest="accessToken", type="base64")
        optParser.add_option("-d", "--use-dummy-access-token", action="store_true", dest="useDummyAccessToken")
        optParser.add_option("-b", "--ble", action="store_true", dest="useBle")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) == 0 and not options.useBle):
            print "Usage:"
            self.do_help('connect')
            return

        if (len(remainingArgs) > 2):
            print "Unexpected argument: " + remainingArgs[2]
            return

        if not options.useBle:
            addr = remainingArgs[0]
            remainingArgs.pop(0)

        if (len(remainingArgs)):
            nodeId = int(remainingArgs[0], 16)
            remainingArgs.pop(0)
        else:
            nodeId = 1

        if (options.useDummyAccessToken and not options.accessToken):
            options.accessToken = base64.standard_b64decode(dummyAccessToken)
        if (options.pairingCode and options.accessToken):
            print "Cannot specify both pairing code and access token"
            return
        try:
            if options.useBle:
                self.devMgr.ConnectBle(bleConnection=FAKE_CONN_OBJ_VALUE,
                                       pairingCode=options.pairingCode,
                                       accessToken=options.accessToken)
            else:
                self.devMgr.ConnectDevice(deviceId=nodeId, deviceAddr=addr,
                                          pairingCode=options.pairingCode,
                                          accessToken=options.accessToken)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Connected to device."
    def do_blediagtest(self, line):
        """
        ble-diag-test [ <options> ]

        Perform BLE throughput test.

        Options:

          --packet-count # | -c #

            Specifies total packet count for the test.

          --duration # | -t #

            Specifies a test duration (unit: second).

          --payload-size # | -s #

            Specifies the payload size for each tx packet (unit: byte, default = 100, max 2048).

          --tx-gap # | -d #

            Specifies a delay for each tx packet (unit: ms, default = 100, min = 1).

          --ack | -a

            Ack on every received packet.

          --reversed | -r

            Perform a throughput test in the reverse direction.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-c", "--packet-count", action="store", dest="packetCount", type="int")
        optParser.add_option("-t", "--duration", action="store", dest="duration", type="int")
        optParser.add_option("-d", "--tx-gap", action="store", dest="delay", type="int")
        optParser.add_option("-s", "--payload-size", action="store", dest="size", type="int")
        optParser.add_option("-a", "--ack", action="store_true", dest="ack")
        optParser.add_option("-r", "--reversed", action="store_true", dest="reversed")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 0):
            print "Unexpected argument: " + remainingArgs[0]
            return

        if options.packetCount:
            count = int(options.packetCount)
        else:
            count = 0

        if options.duration:
            duration = int(options.duration)
            if (duration > 2000000):    # 2GB in ms
                print "Invalid duration %d seconds" % duration
                return
            duration = duration * 1000
        else:
            duration = 0

        if options.delay:
            delay = int(options.delay)
        else:
            delay = 100

        if options.size:
            size = int(options.size)
        else:
            size = 100

        if options.ack:
            ack = 1
        else:
            ack = 0

        if options.reversed:
            rx = 1
        else:
            rx = 0

        if (count == 0 and duration == 0):
            print "Packet Count or Duration has to be specified"
            return

        if (size < 0 or size > 2048):
            print "Invalid Payload Size (0~2048): %d" % size
            return

        if (delay < 1 or delay > 60000):
            print "Invalid Tx Gap (1~60000ms): %d" % delay
            return

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        if not self.bleMgr.isConnected():
            print "BLE not connected"
            return

        print "PacketCount = %d" % count
        print "Duration = %d ms" % duration
        print "Tx Gap = %d ms" % delay
        print "Ack = %d" % ack
        print "Payload Size = %d bytes" % size
        print "Rx = %d" % rx

        try:
            self.devMgr.TestBle(FAKE_CONN_OBJ_VALUE, count, duration, delay, ack, size, rx)
        except WoBleTestMgr.DeviceManagerException, ex:
            print str(ex)
        return

    def do_blediagtestresult(self, line):
        """
        ble-diag-test-result [ <options> ]

        Get the test result of last BLE throughput test (default: remote).

        Options:

          --local | -l

            Get the local throughput test result.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-l", "--local", action="store_true", dest="local")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 0):
            print "Unexpected argument: " + remainingArgs[0]
            return

        if options.local:
            local = 1
        else:
            local = 0

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        if not self.bleMgr.isConnected():
            print "BLE not connected"
            return

        try:
            self.devMgr.TestResultBle(FAKE_CONN_OBJ_VALUE, local)
        except WoBleTestMgr.DeviceManagerException, ex:
            print str(ex)
        return

    def do_blediagtestabort(self, line):
        """
        ble-diag-test-abort

        Stop the BLE throughput test.

        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 0):
            print "Unexpected argument: " + remainingArgs[0]
            return

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        if not self.bleMgr.isConnected():
            print "BLE not connected"
            return

        try:
            self.devMgr.TestAbortBle(FAKE_CONN_OBJ_VALUE)
        except WoBleTestMgr.DeviceManagerException, ex:
            print str(ex)
        return

    def do_blediagtesttiming(self, line):
        """
        ble-diag-test-timing [ <options> ] [ on | off ]

        Set Tx timing mode.

        Options:

          --remote | -r

            Set the remote Tx timing mode.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('ble-diag-test-timing')
            return

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-r", "--remote", action="store_true", dest="remote")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 1):
            print "Usage:"
            self.do_help('ble-diag-test-timing')
            return

        enabled = self.parseBoolean(remainingArgs[0])
        if (enabled == None):
            print "Invalid argument: " + remainingArgs[0]
            return

        if options.remote:
            remote = 1
        else:
            remote = 0

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        if not self.bleMgr.isConnected():
            print "BLE not connected"
            return

        try:
            self.devMgr.TxTimingBle(FAKE_CONN_OBJ_VALUE, enabled, remote)
        except WoBleTestMgr.DeviceManagerException, ex:
            print str(ex)
            return

    def do_rendezvous(self, line):
        """
        rendezvous [ <options> ]
        rendezvous <pairing-code>

        Options:

          --pairing-code <pairing-code>

            Specifies a pairing code string to be used to authenticate the device.

          --access-token <access-token>

            Specifies a base-64 encoded access token to be used to authenticate the device.

          --use-dummy-access-token

            Authenticate using the built-in dummy access token.

          --target-fabric <fabric-id>

            Specifies that only devices that are members of the specified Weave fabric
            should respond. A value of 0 specifies that only devices that are not a member
            of a fabric should respond. A value of FFFFFFFFFFFFFFF (the default)
            specifies that all devices should respond regardless of fabric membership.

          --target-modes <modes>

            Specifies that only devices that are currently in the specified modes
            should respond. <modes> is a bit-field with the following values:

              0 - Locate all devices regardless of mode.
              1 - Locate all devices in 'user-selected' mode -- i.e., where the device
                  has has been directly identified by a user, e.g., by pressing a button.

          --target-vendor <vendor-id>

            Specifies that only devices manufactured by the given vendor should
            respond to the identify request. A value of FFFF specifies any vendor.

          --target-product <product-id>

            Specifies that only devices with the given product id should respond.
            A value of 0xFFFF specifies any product. If the --target-product is specified,
            then the --target-vendor must also be specified.

          --target-device <node-id>

            Specifies that only the device with the given Weave node id should respond.
            A value of 0xFFFFFFFFFFFFFFFF (the default) specifies that all devices should
            respond.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-p", "--pairing-code", action="store", dest="pairingCode", type="string")
        optParser.add_option("-a", "--access-token", action="store", dest="accessToken", type="base64")
        optParser.add_option("-d", "--use-dummy-access-token", action="store_true", dest="useDummyAccessToken")
        optParser.add_option("", "--target-fabric", action="store", dest="targetFabricId", type="hexint", default=WeaveDeviceMgr.TargetFabricId_AnyFabric)
        optParser.add_option("", "--target-modes", action="store", dest="targetModes", type="hexint", default=WeaveDeviceMgr.TargetDeviceMode_Any)
        optParser.add_option("", "--target-vendor", action="store", dest="targetVendorId", type="hexint", default=WeaveDeviceMgr.TargetVendorId_Any)
        optParser.add_option("", "--target-product", action="store", dest="targetProductId", type="hexint", default=WeaveDeviceMgr.TargetProductId_Any)
        optParser.add_option("", "--target-device", action="store", dest="targetDeviceId", type="hexint", default=WeaveDeviceMgr.TargetDeviceId_Any)

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 1):
            print "Unexpected argument: " + remainingArgs[1]
            return

        if (len(remainingArgs) == 1):
            options.pairingCode = remainingArgs[0]
        if (options.useDummyAccessToken and not options.accessToken):
            options.accessToken = base64.standard_b64decode(dummyAccessToken)
        if (options.pairingCode and options.accessToken):
            print "Cannot specify both pairing code and access token"
            return

        try:
            self.devMgr.RendezvousDevice(pairingCode=options.pairingCode,
                                         accessToken=options.accessToken,
                                         targetFabricId=options.targetFabricId,
                                         targetModes=options.targetModes,
                                         targetVendorId=options.targetVendorId,
                                         targetProductId=options.targetProductId,
                                         targetDeviceId=options.targetDeviceId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Connected to device %X at %s" % (self.devMgr.DeviceId(), self.devMgr.DeviceAddress())

    def do_passiverendezvous(self, line):
        """
        passive-rendezvous [ <options> ]
        passive-rendezvous <pairing-code>

        Options:

          --pairing-code <pairing-code>

            Specifies a pairing code string to be used to authenticate the device.

          --access-token <access-token>

            Specifies a base-64 encoded access token to be used to authenticate the device.

          --use-dummy-access-token

            Authenticate using the built-in dummy access token.

        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-p", "--pairing-code", action="store", dest="pairingCode", type="string")
        optParser.add_option("-a", "--access-token", action="store", dest="accessToken", type="base64")
        optParser.add_option("-d", "--use-dummy-access-token", action="store_true", dest="useDummyAccessToken")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 1):
            print "Unexpected argument: " + remainingArgs[1]
            return

        if (len(remainingArgs) == 1):
            options.pairingCode = remainingArgs[0]
        if (options.useDummyAccessToken and not options.accessToken):
            options.accessToken = base64.standard_b64decode(dummyAccessToken)
        if (options.pairingCode and options.accessToken):
            print "Cannot specify both pairing code and access token"
            return

        try:
            self.devMgr.PassiveRendezvousDevice(pairingCode=options.pairingCode,
                                                accessToken=options.accessToken)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Connected to device %X at %s" % (self.devMgr.DeviceId(), self.devMgr.DeviceAddress())
        print "done"

    def do_remotepassiverendezvous(self, line):
        """
        remote-passive-rendezvous [ <options> ]
        passive-rendezvous <pairing-code>

        Options:

          --pairing-code <pairing-code>

            Specifies a pairing code string to be used to authenticate the device.

          --access-token <access-token>

            Specifies a base-64 encoded access token to be used to authenticate the device.

          --use-dummy-access-token

            Authenticate using the built-in dummy access token.

          --joiner-address <joiner-address>

            Specifies the expected IPv6 address of the joiner. The assisting device will not connect a remote device
            to the client unless the remote device's address matches this value.

          --rendezvous-timeout <rendezous-timeout>

            Specifies a timeout in seconds for the completion of the Remote Passive Rendezvous. Both the Device
            Manager and assisting device will close their end of the RPR connection if no remote host rendezvouses
            with the assisting device prior to this timeout's expiration.

          --inactivity-timeout <inactivity-timeout>

            Specifies an inactivity timeout in seconds for use by the assisting device after the Remote Passive
            Rendezvous completes. If the assisting device does not observe traffic across the tunnel from both
            sides within a period of time equal to or great than this timeout, it will close the tunnel.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-p", "--pairing-code", action="store", dest="pairingCode", type="string")
        optParser.add_option("-t", "--access-token", action="store", dest="accessToken", type="base64")
        optParser.add_option("-d", "--use-dummy-access-token", action="store_true", dest="useDummyAccessToken")
        optParser.add_option("-j", "--joiner-address", action="store", dest="joinerAddr", type="string")
        optParser.add_option("-r", "--rendezvous-timeout", action="store", dest="rendezvousTimeout", type="int")
        optParser.add_option("-i", "--inactivity-timeout", action="store", dest="inactivityTimeout", type="int")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 1):
            print "Unexpected argument: " + remainingArgs[1]
            return

        if (len(remainingArgs) == 1):
            options.pairingCode = remainingArgs[0]
        if (options.useDummyAccessToken and not options.accessToken):
            options.accessToken = base64.standard_b64decode(dummyAccessToken)
        if (options.pairingCode and options.accessToken):
            print "Cannot specify both pairing code and access token"
            return

        try:
            self.devMgr.RemotePassiveRendezvous(rendezvousDeviceAddr=options.joinerAddr,
                    pairingCode=options.pairingCode, accessToken=options.accessToken,
                    rendezvousTimeout=options.rendezvousTimeout, inactivityTimeout=options.inactivityTimeout)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Successfully connected to remote device %X" % (self.devMgr.DeviceId())

    def do_reconnect(self, line):
        """
        reconnect

        Reconnect to the device using the previously supplied connect arguments.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('reconnect')
            return

        try:
            self.devMgr.ReconnectDevice()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

    def do_close(self, line):
        """
        close

        Close the connection to the device.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('close')
            return

        try:
            self.devMgr.Close()
            self.devMgr.CloseEndpoints()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)

    def do_enableconnectionmonitor(self, line):
        """
        enable-connection-monitor [ <interval> <timeout> ]

        Instruct the device to enable Weave connection monitoring.

        <interval>
            -- Interval at which to send EchoRequest messages (in ms).
               Defaults to 500. Max is 65535 ms.

        <timeout>
            -- Amount of time after which the lack of a response to an
               EchoRequest will cause the device to terminate the
               connection (in ms). Defaults to 2000. Max is 65535 ms.

        """

        args = shlex.split(line)

        if (len(args) > 2):
            print "Unexpected argument: " + args[2]
            return

        if (len(args) == 0):
            interval = 500
            timeout = 2000
        elif (len(args) == 2):
            interval = int(args[0])
            if (interval < 0 or interval > 65535):
                print "Invalid value specified for interval: " + args[0]
                return
            timeout = int(args[1])
            if (timeout < 0 or timeout > 65535):
                print "Invalid value specified for interval: " + args[1]
                return
        else:
            print "Usage:"
            self.do_help('rendezvous')
            return

        try:
            self.devMgr.EnableConnectionMonitor(interval, timeout)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Connection monitor enabled"

    def do_disableconnectionmonitor(self, line):
        """
        disable-connection-monitor

        Instruct the device to disable Weave connection monitoring.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[2]
            return

        try:
            self.devMgr.DisableConnectionMonitor()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Connection monitor disabled"

    def do_scannetworks(self, line):
        """
          scan-networks

          Scan for remote WiFi networks.
        """

        args = shlex.split(line)

        networkType = WeaveDeviceMgr.NetworkType_WiFi

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        if (len(args) == 1):
            try:
                networkType = WeaveDeviceMgr.ParseNetworkType(args[0])
            except Exception, ex:
                print "Invalid network type: " + args[0]
                return

        try:
            scanResult = self.devMgr.ScanNetworks(networkType)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "ScanNetworks complete, %d network(s) found" % (len(scanResult))
        i = 1
        for net in scanResult:
            print "  Network %d" % (i)
            net.Print("    ")
            i = i + 1

    def do_addnetwork(self, line):
        self.do_addwifinetwork(line)

    def do_addwifinetwork(self, line):
        """
          add-wifi-network <ssid> <security-type> [ <key> ]

            Provision a new WiFi network.

            <security-type>:
              none
              wep
              wpa
              wpa2
              wpa2-mixed-personal
              wpa-enterprise
              wpa2-enterprise
              wpa2-mixed-enterprise
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('add-wifi-network')
            return

        if (len(args) < 2):
            print "Please specify WiFI security type"
            return

        securityType = WeaveDeviceMgr.ParseSecurityType(args[1])
        if (securityType == None):
            print "Unrecognized security type: " + args[1]
            return

        networkInfo = WeaveDeviceMgr.NetworkInfo(
            networkType = WeaveDeviceMgr.NetworkType_WiFi,
            wifiSSID = args[0],
            wifiMode = WeaveDeviceMgr.WiFiMode_Managed,
            wifiRole = WeaveDeviceMgr.WiFiRole_Station,
            wifiSecurityType = securityType)

        if (securityType != WeaveDeviceMgr.WiFiSecurityType_None):
            if (len(args) < 3):
                print "Must supply WiFi key"
                return
            if (len(args) > 3):
                print "Unexpected argument: " + args[3]
                return
            networkInfo.WiFiKey = args[2]
        elif (len(args) > 2):
            print "Unexpected argument: " + args[2]
            return

        try:
            addResult = self.devMgr.AddNetwork(networkInfo)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        self.lastNetworkId = addResult

        print "Add wifi network complete (network id = " + str(addResult) + ")"

    def do_addthreadnetwork(self, line):
        """
          add-thread-network <name> <extended-pan-id> [ <key> ] [ <field>=<value>... ]

          Provision a new Thread network.

            <name>: string name of network
            <extended-pan-id>: hex string (8 bytes)
            <key>: hex string (any length)

            <field>:
              thread-key or key
              thread-pan-id or pan-id
              thread-channel or channel
              ...
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('add-thread-network')
            return
        if (len(args) < 2):
            print "Please specify the Network Name and Extended PAN Identifier"
            return

        networkInfo = WeaveDeviceMgr.NetworkInfo()
        networkInfo.NetworkType = WeaveDeviceMgr.NetworkType_Thread
        networkInfo.ThreadNetworkName = args[0]

        try:
            networkInfo.ThreadExtendedPANId = bytearray(binascii.unhexlify(args[1]))
            if len(networkInfo.ThreadExtendedPANId) != 8:
                print "Thread extended PAN id must be 8 bytes in hex"
                return
        except ValueError:
            print "Invalid value specified for thread extended PAN id: " + args[1]
            return

        kvstart = 3 if (len(args) > 2 and len(args[2].split('=', 1)) == 1) else 2

        if (kvstart > 2):
            try:
                networkInfo.ThreadNetworkKey = bytearray(binascii.unhexlify(args[2]))
            except ValueError:
                print "Invalid value for Thread Network Key"
                return

        for addedVal in args[kvstart:]:
            pair = addedVal.split('=', 1)
            if (len(pair) < 2):
                print "Invalid argument: must be key=value format <" + addedVal + ">"
                return

            name = pair[0]
            val = pair[1]

            if name == 'key':
                name = 'threadnetworkkey'
            elif name == 'channel':
                name = 'threadchannel'
            elif name == 'extended-pan-id':
                name = 'threadextendedpanid'

            try:
                if (name == 'threadchannel' or name == 'thread-channel'):
                    val = int(val, 10)
                elif (name == 'threadnetworkkey' or name == 'thread-network-key' or name == 'thread-key'):
                    val = bytearray(binascii.unhexlify(val))
                elif (name == 'threadextendedpanid' or name == 'thread-extended-pan-id'):
                    val = bytearray(binascii.unhexlify(val))
                elif (name == 'threadpanid' or name == 'thread-pan-id' or name == 'pan-id'):
                    val = int(val, 16)
            except ValueError:
                print "Invalid value specified for <" + name + "> field"
                return

            try:
                networkInfo.SetField(name, val)
            except Exception, ex:
                print str(ex)
                return

        if networkInfo.ThreadPANId  != None:
            panId=networkInfo.ThreadPANId
            if panId < 1 or panId > 0xffff:
                print "Thread PAN Id must be non-zero and 2 bytes in hex"
                return

        try:
            addResult = self.devMgr.AddNetwork(networkInfo)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        self.lastNetworkId = addResult

        print "Add thread network complete (network id = " + str(addResult) + ")"

    def do_createthreadnetwork(self, line):
        """
          create-thread-network [ <options> ]

          Send a request to device to create a new Thread network and wait for a reply.

          Options:

            --name <name>

              Thread network name (string).

            --key <key>

              Thread network key (hex string of any length).

            --panid <panid>

              Thread network PAN id (16-bit hex int).

            --channel <channel>

              Thread network channel number (int). Valid supported range is [11 - 26].

          All above parameters are optional and if not specified the value will be created by device.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("-n", "--name", action="store", dest="threadNetworkName", type="string")
        optParser.add_option("-k", "--key", action="store", dest="threadNetworkKey", type="string")
        optParser.add_option("-p", "--panid", action="store", dest="threadPANId", type="hexint")
        optParser.add_option("-c", "--channel", action="store", dest="threadChannel", type="int")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 0):
            print "Unexpected argument: " + remainingArgs[0]
            return

        networkInfo = WeaveDeviceMgr.NetworkInfo()
        networkInfo.NetworkType = WeaveDeviceMgr.NetworkType_Thread

        if (options.threadNetworkName):
            networkInfo.ThreadNetworkName = options.threadNetworkName
        if (options.threadNetworkKey):
            networkInfo.ThreadNetworkKey = bytearray(binascii.unhexlify(options.threadNetworkKey))
        if (options.threadPANId):
            networkInfo.ThreadPANId = options.threadPANId
            if (networkInfo.ThreadPANId > 0xffff):
                print "Thread PAN Id must be 16-bit hex value."
                return
        if (options.threadChannel):
            networkInfo.ThreadChannel = options.threadChannel
            if (networkInfo.ThreadChannel < 11 or networkInfo.ThreadChannel > 26):
                print "Thread Channel value must be in a range [11 - 26]."
                return

        try:
            addResult = self.devMgr.AddNetwork(networkInfo)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        self.lastNetworkId = addResult

        print "Create Thread network complete (network id = " + str(addResult) + ")"

    def do_updatenetwork(self, line):
        """
          update-network <network-id> [ <field>=<value>... ]

          Update an existing provisioned network.

            <field>:
              wifi-ssid or ssid
              wifi-mode
              wifi-role
              wifi-security or security
              wifi-key or key
              thread-network-name or thread-name
              thread-extended-pan-id or pan-id
              network-key or thread-key
        """

        args = shlex.split(line)

        print args

        if (len(args) == 0):
            print "Usage:"
            self.do_help('update-network')
            return

        if (len(args) < 1):
            print "Please specify the network id"
            return

        networkId = self.parseNetworkId(args[0])
        if (networkId == None):
            return

        self.lastNetworkId = networkId

        networkInfo = WeaveDeviceMgr.NetworkInfo(networkId=networkId)
        for updatedVal in args[1:]:
            nameVal = updatedVal.split('=', 1)
            if (len(nameVal) < 2):
                print "Invalid argument: updatedVal"
                return
            try:
                networkInfo.SetField(nameVal[0], nameVal[1])
            except Exception, ex:
                print str(ex)
                return

        try:
            self.devMgr.UpdateNetwork(networkInfo)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Update network complete"

    def do_removenetwork(self, line):
        """
          remove-network <network-id>

          Remove a provisioned network.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('remove-network')
            return

        if (len(args) < 1):
            print "Please specify a network id"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        networkId = self.parseNetworkId(args[0])
        if (networkId == None):
            return

        self.lastNetworkId = networkId

        try:
            self.devMgr.RemoveNetwork(networkId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Remove network complete"

    def do_getnetworks(self, line):
        """
          get-networks [ <flags> ]

          Get the list of currently provisioned networks.

            <flags> (integer bit field):
                1 -- Include network credentials.
        """

        args = shlex.split(line)

        flags = 1 # 1 = Include credentials

        if (len(args) == 1):
            flags = int(args[0])

        elif (len(args) > 1):
            print "Unexpected argument: " + args[2]
            return

        try:
            getResult = self.devMgr.GetNetworks(flags)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Get networks complete, %d network(s) returned" % (len(getResult))
        i = 1
        for net in getResult:
            print "  Network %d" % (i)
            net.Print("    ")
            i = i + 1

    def do_getcameraauthdata(self, line):
        """
          get-camera-auth-data <nonce>

          Get the signed Oculus pairing payload from the Nest Cam.
        """

        args = shlex.split(line)

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        if (len(args) < 1):
            print "Please specify a nonce"
            return

        try:
            getResult = self.devMgr.GetCameraAuthData(args[0])
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Get camera auth data returned:"
        print "     camera MAC address  = %s" % getResult[0]
        print "     camera auth_data    = %s" % getResult[1]

    def do_enablenetwork(self, line):
        """
          enable-network <network-id>

          Enable a provisioned network.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('enable-network')
            return

        if (len(args) < 1):
            print "Please specify a network id"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        networkId = self.parseNetworkId(args[0])
        if (networkId == None):
            return

        self.lastNetworkId = networkId

        try:
            self.devMgr.EnableNetwork(networkId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Enable network complete"

    def do_disablenetwork(self, line):
        """
          disable-network <network-id>

          Disable a provisioned network.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('disable-network')
            return

        if (len(args) < 1):
            print "Please specify a network id"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        networkId = self.parseNetworkId(args[0])
        if (networkId == None):
            return

        self.lastNetworkId = networkId

        try:
            self.devMgr.DisableNetwork(networkId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Disable network complete"

    def do_testnetwork(self, line):
        """
          test-network <network-id>

          Perform a network connectivity test for a particular provisioned network.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('test-network')
            return

        if (len(args) < 1):
            print "Please specify a network id"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        networkId = self.parseNetworkId(args[0])
        if (networkId == None):
            return

        self.lastNetworkId = networkId

        try:
            self.devMgr.TestNetworkConnectivity(networkId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Network test complete"

    def do_setrendezvousmode(self, line):
        """
          set-rendezvous-mode <mode-flags>

          Send an SetRendezvousMode request to the device and wait for a reply.
        """

        args = shlex.split(line)

        if (len(args) < 1):
            print "Please specify the rendezvous mode flags"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.SetRendezvousMode(int(args[0]))
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Set rendezvous mode complete"

    def do_getlastnetworkprovisioningresult(self, line):
        """
        get-last-network-provisioning-result

        Request the device to return the result of the last network provisioning request.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.GetLastNetworkProvisioningResult()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Last network provisioning request was successful"

    def do_ping(self, line):
        """
          ping

          Send an EchoRequest to the device and wait for a reply.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.Ping()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Ping complete"

    def do_identify(self, line):
        """
          identify

          Send an IdentifyDevice request to the connected device.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            deviceDesc = self.devMgr.IdentifyDevice()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Identify device complete"
        deviceDesc.Print("  ")

    def do_pairtoken(self, line):
        """
          pair-token <pairing-token>

          Send an PairToken request to the connected device.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('pair-token')
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        try:
            tokenPairingBundle = self.devMgr.PairToken(args[0])
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Pair token complete"
        print "TokenPairingBundle: " + base64.b64encode(buffer(tokenPairingBundle))

    def do_unpairtoken(self, line):
        """
          unpair-token

          Send an UnpairToken request to the connected device.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Usage:"
            self.do_help('unpair-token')
            return

        try:
            result = self.devMgr.UnpairToken()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Unpair token complete"

    def do_createfabric(self, line):
        """
          create-fabric

          Send a CreateFabric request to the device and wait for a reply.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.CreateFabric()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Create fabric complete"

    def do_leavefabric(self, line):
        """
          leave-fabric

          Send a LeaveFabric request to the device and wait for a reply.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.LeaveFabric()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Leave fabric complete"

    def do_getfabricconfig(self, line):
        """
          get-fabric-config

          Send a GetFabricConfig request to the device and wait for a reply.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[1]
            return

        try:
            fabricConfig = self.devMgr.GetFabricConfig()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Get fabric config complete"
        print "Fabric configuration: " + base64.b64encode(buffer(fabricConfig))

    def do_joinexistingfabric(self, line):
        """
          join-existing-fabric <fabric-config>

          Send a JoinExistingFabric request to the device and wait for a reply.

            <fabric-config>: Base-64 encoded fabric configuration value, as returned
            by the getfabricconfig command.
        """

        args = shlex.split(line)

        if (len(args) < 1):
            print "Please specify the fabric configuration value"
            return

        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        try:
            fabricConfig = base64.b64decode(args[0])
        except TypeError, ex:
            print "Invalid fabric configuration value: " + str(ex)
            return

        try:
            self.devMgr.JoinExistingFabric(fabricConfig)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Join existing fabric complete"

    def do_registerservice(self, line):
        """
          register-service [ <options> ]

          Send a RegisterServicePairAccount request to the device and wait for a reply.

            Valid options are:

                --service-id <hex-string>

                --account-id <string>

                --service-config <base64-string>

                --pairing-token <string>

                --init-data <string>

            Default values are automatically supplied for any missing options.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("", "--service-id", action="store", dest="serviceId", type="hexint", default=0x18B4300100000001)
        optParser.add_option("", "--account-id", action="store", dest="accountId", default="DUMMY-ACCOUNT-ID")
        optParser.add_option("", "--service-config", action="store", dest="serviceConfig", type="base64", default=dummyServiceConfig)
        optParser.add_option("", "--pairing-token", action="store", dest="pairingToken", default="dummy-pairing-token")
        optParser.add_option("", "--init-data", action="store", dest="pairingInitData", default="dummy-pairing-init-data")

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        if (len(remainingArgs) > 0):
            print "Unexpected argument: " + remainingArgs[0]
            return

        try:
            self.devMgr.RegisterServicePairAccount(options.serviceId, options.accountId, options.serviceConfig, options.pairingToken, options.pairingInitData)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Register service complete"

    def do_updateservice(self, line):
        """
          update-service [ <options> ]

          Send an UpdateService request to the device and wait for a reply.

            Valid options are:

                --service-id <hex-string>

                --service-config <base64-string>

            Default values are automatically supplied for any missing options.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("", "--service-id", action="store", dest="serviceId", type="hexint", default=0x18B4300100000001)
        optParser.add_option("", "--service-config", action="store", dest="serviceConfig", type="base64", default=dummyServiceConfig)

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        try:
            self.devMgr.UpdateService(options.serviceId, options.serviceConfig)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Update service complete"

    def do_unregisterservice(self, line):
        """
          unregister-service [ <options> ]

          Send an UnregisterService request to the device and wait for a reply.

            Valid options are:

                --service-id <hex-string>

            Default values are automatically supplied for any missing options.
        """

        args = shlex.split(line)

        optParser = OptionParser(usage=optparse.SUPPRESS_USAGE, option_class=ExtendedOption)
        optParser.add_option("", "--service-id", action="store", dest="serviceId", type="hexint", default=0x18B4300100000001)

        try:
            (options, remainingArgs) = optParser.parse_args(args)
        except SystemExit:
            return

        try:
            self.devMgr.UnregisterService(options.serviceId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Unregister service complete"

    def do_armfailsafe(self, line):
        """
          arm-fail-safe [ <arm-mode> [ <fail-safe-token> ] ]

          Arm the configuration fail-safe mechanism on the device.

            <arm-mode>:
                new    -- Arm a new fail-safe; return an error if one is already active.
                          This is the default.
                reset  -- Reset the device's configuration and arm a new fail-safe.
                resume -- Resume a fail-safe already in progress; return an error if
                          there is no fail-safe in progress, or if the fail-safe token
                          does not match the active fail-safe.

            <fail-safe-token>:
                          Unique unsigned integer identifying a particular in-progress
                          fail-safe. Defaults to a random number.
        """

        args = shlex.split(line)

        failSafeToken = random.randint(0, 2**32)

        if (len(args) < 1):
            armMode = 1
        else:
            armMode = args[0].lower()
            if (armMode == "new"):
                armMode = 1
            elif (armMode == "reset"):
                armMode = 2
            elif (armMode == "resume"):
                armMode = 3
            else:
                print "Invalid fail-safe arm mode: " + args[0]
                return
            if (len(args) == 2):
                failSafeToken = int(args[1])

        if (len(args) > 2):
            print "Unexpected argument: " + args[2]
            return

        try:
            self.devMgr.ArmFailSafe(armMode, failSafeToken)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Arm fail-safe complete, fail-safe token = " + str(failSafeToken)

    def do_disarmfailsafe(self, line):
        """
          disarm-fail-safe

          Disarm the currently active configuration fail-safe on the device.
        """

        args = shlex.split(line)

        if (len(args) > 0):
            print "Unexpected argument: " + args[0]
            return

        try:
            self.devMgr.DisarmFailSafe()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Disarm fail-safe complete"

    def do_resetconfig(self, line):
        """
          reset-config [ <reset-flags> ]

          Reset the device's configuration. <reset-flags> is an optional hex value
          specifying the type of reset to be performed.
        """

        args = shlex.split(line)

        if (len(args) < 1):
            resetFlags = 0xFF
        elif (len(args) == 1):
            try:
                resetFlags = int(args[0], 16)
            except ValueError:
                raise OptionValueError(
                    "Invalid value specified for reset-flags: %r" % (args[0]))
        else:
            print "Unexpected argument: " + args[1]
            return

        try:
            self.devMgr.ResetConfig(resetFlags)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Reset config complete"

    def do_setrendezvousaddr(self, line):
        """
          set-rendezvous-addr <addr>

          Set the device IP address to be used when rendezvousing with a device.
          <addr> can be an IPv4 or IPv6 address. Specify the IPv6 link-local, all
          nodes multicast address (ff02::1) to use multicast rendezvous.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('set-rendezvous-addr')
            return

        addr = args[0]

        try:
            self.devMgr.SetRendezvousAddress(addr)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Done."

    def do_setautoreconnect(self, line):
        """
          set-auto-reconnect [ on | off ]

          Enable/disable the auto-reconnect feature.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('set-auto-reconnect')
            return
        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        autoReconnect = self.parseBoolean(args[0])
        if (autoReconnect == None):
            return

        try:
            self.devMgr.SetAutoReconnect(autoReconnect)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Done."

    def do_setlogoutput(self, line):
        """
          set-log-output [ none | error | progress | detail ]

          Set the level of Weave logging output.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('set-log-output')
            return
        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        category = args[0].lower()
        if (category == 'none'):
            category = 0
        elif (category == 'error'):
            category = 1
        elif (category == 'progress'):
            category = 2
        elif (category == 'detail'):
            category = 3
        else:
            print "Invalid argument: " + args[0]
            return

        try:
            self.devMgr.SetLogFilter(category)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Done."


    def do_setrendezvouslinklocal(self, line):
        """
          set-rendezvous-link-local [ on | off ]

          Enable/disable rendezvousing with a device using its link-local address.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('set-rendezvous-link-local')
            return
        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        rendezvousLinkLocal = self.parseBoolean(args[0])
        if (rendezvousLinkLocal == None):
            return

        try:
            self.devMgr.SetRendezvousLinkLocal(rendezvousLinkLocal)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Done."


    def do_setconnecttimeout(self, line):
        """
          set-connect-timeout <int>

          Set the connection timeout in ms.
        """

        args = shlex.split(line)

        if (len(args) == 0):
            print "Usage:"
            self.do_help('set-connect-timeout')
            return
        if (len(args) > 1):
            print "Unexpected argument: " + args[1]
            return

        timeoutMS = self.parseInt(args[0])
        if (timeoutMS == None):
            return

        try:
            self.devMgr.SetConnectTimeout(timeoutMS)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Done."

    def do_bleadapterselect(self, line):
        """
          ble-adapter-select

          Start BLE adapter select.
        """
        if sys.platform.startswith('linux'):
            if not self.bleMgr:
                self.bleMgr = BleManager(self.devMgr)

            self.bleMgr.ble_adapter_select(line)
        else:
            print "ble-adapter-select only works in Linux, ble-adapter-select mac_address"

        return

    def do_bleadapterprint(self, line):
        """
          ble-adapter-print

          Print attached BLE adapter.
        """
        if sys.platform.startswith('linux'):
            if not self.bleMgr:
                self.bleMgr = BleManager(self.devMgr)

            self.bleMgr.ble_adapter_print()
        else:
            print "ble-adapter-print only works in Linux"

        return

    def do_bledebuglog(self, line):
        """
          ble-debug-log 0:1
            0: disable BLE debug log
            1: enable BLE debug log
        """
        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        self.bleMgr.ble_debug_log(line)

        return

    def do_blescan(self, line):
        """
          ble-scan

          Start BLE scanning operations.
        """

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        self.bleMgr.scan(line)

        return

    def do_bleconnect(self, line):
        """
          ble-connect

          Connect to a BLE peripheral identified by line.
        """

        if not self.bleMgr:
           self.bleMgr = BleManager(self.devMgr)
        self.bleMgr.connect(line)

        return

    def do_blescanconnect(self, line):
        """
          ble-scan-connect

          Scan and connect to a BLE peripheral identified by line.
        """

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        self.bleMgr.scan_connect(line)

        return

    def do_bledisconnect(self, line):
        """
        ble-disconnect

        Disconnect from a BLE peripheral.
        """

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        self.bleMgr.disconnect()

        return

    def do_blesend(self, line):
        """
            ble-send

            Send a string of bytes to the connected peripheral.
        """

        if not self.bleMgr:
            self.bleMgr = BleManager(self.devMgr)

        self.bleMgr.send(line)

    def do_getactivelocale(self, line):
        """
          get-active-locale

          Get the device's active locale
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('get-active-locale')
            return

        try:
            getResult = self.devMgr.GetActiveLocale()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Get active locale complete: %s" % getResult

    def do_getavailablelocales(self, line):
        """
          get-available-locales

          Get the device's available locales.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('get-available-locales')
            return

        try:
            getResult = self.devMgr.GetAvailableLocales()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Get active locales complete: %s" % getResult

    def do_setactivelocale(self, line):
        """
          set-active-locale <locale>

          Set the device's active locale.
        """

        args = shlex.split(line)

        if (len(args) != 1):
            print "Usage:"
            self.do_help('set-active-locale')
            return

        try:
            self.devMgr.SetActiveLocale(args[0])
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Set active locale complete"

    def do_startsystemtest(self, line):
        """
          start-system-test <product-name> <test-id>

          Start system test on a device.

            <product-name>: thermostat
            <product-name>: topaz
        """

        args = shlex.split(line)

        if (len(args) != 2):
            print "Usage:"
            self.do_help('start-system-test')
            return

        productName = args[0]
        testId = int(args[1])

        if productName not in WeaveDeviceMgr.SystemTest_ProductList.keys():
            print "Unknown product for system tests: %s" % productName
            print "Usage:"
            self.do_help('start-system-test')
            return

        try:
            self.devMgr.StartSystemTest(WeaveDeviceMgr.SystemTest_ProductList[productName], testId)
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Start system test complete"

    def do_stopsystemtest(self, line):
        """
          stop-system-test

          Stop system test on a device.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('stop-system-test')
            return

        try:
            self.devMgr.StopSystemTest()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Stop system test complete"

    def do_thermostatgetentrykey(self, line):
        """
          thermostat-get-entry-key

          Get the thermostat's 6-character entry key.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('thermostat-get-entry-key')
            return

        try:
            getResult = self.devMgr.ThermostatGetEntryKey()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Thermostat get entry key complete: %s" % getResult

    def do_thermostatsystemteststatus(self, line):
        """
          thermostat-system-test-status

          Get the thermostat system test status.
        """

        args = shlex.split(line)

        if (len(args) != 0):
            print "Usage:"
            self.do_help('thermostat-system-test-status')
            return

        try:
            getResult = self.devMgr.ThermostatSystemTestStatus()
        except WeaveDeviceMgr.DeviceManagerException, ex:
            print str(ex)
            return

        print "Thermostat system test status complete: %s" % getResult

    def do_history(self, line):
        """
        history

        Show previously executed commands.
        """

        try:
            import readline
            h = readline.get_current_history_length()
            for n in range(1,h+1):
                print readline.get_history_item(n)
        except ImportError:
            pass

    def do_h(self, line):
        self.do_history(line)

    def do_exit(self, line):
        return True

    def do_quit(self, line):
        return True

    def do_q(self, line):
        return True

    def do_EOF(self, line):
        print
        return True

    def emptyline(self):
        pass

    def parseNetworkId(self, value):

        if (value == '$last-network-id' or value == '$last'):
            if (self.lastNetworkId != None):
                return self.lastNetworkId
            else:
                print "No last network id"
                return None

        try:
            return int(value)
        except ValueError:
            print "Invalid network id"
            return None

    def parseBoolean(self, val):
        val = val.lower()
        if (val == 'on' or val == 'true' or val == 'yes' or val == '1'):
            return True
        elif (val == 'off' or val == 'false' or val == 'no' or val == '0'):
            return False
        else:
            print "Invalid argument: " + val
            return None

    def parseInt(self, val):
        try:
            return int(val)
        except ValueError:
            print "Invalid argument: " + val
            return None



optParser = OptionParser()
optParser.add_option("-r", "--rendezvous-addr", action="store", dest="rendezvousAddr", help="Device rendezvous address", metavar='<ip-address>')
(options, remainingArgs) = optParser.parse_args(sys.argv[1:])

if len(remainingArgs) != 0:
    print 'Unexpected argument: %s' % remainingArgs[0]
    sys.exit(-1)

devMgrCmd = DeviceMgrCmd(rendezvousAddr=options.rendezvousAddr)
print "Weave Device Manager Shell"
if options.rendezvousAddr:
    print "Rendezvous address set to %s" % options.rendezvousAddr
print

try:
    devMgrCmd.cmdloop()
except KeyboardInterrupt:
    print '\nQuitting'

sys.exit(0)

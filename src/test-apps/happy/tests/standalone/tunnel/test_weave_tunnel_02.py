#!/usr/bin/env python
#
#       Copyright (c) 2015-2017  Nest Labs, Inc.
#       All rights reserved.
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
#       Calls Weave Tunnel between nodes.
#

import itertools
import os
import unittest
import set_test_path
import time

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeavePing
import WeaveTunnelStart
import WeaveTunnelStop
import WeaveUtilities
import subprocess

class test_weave_tunnel_02(unittest.TestCase):
    def setUp(self):
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.use_lwip = True
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_on_tap_ap_service.sh"
        else:
            self.use_lwip = False
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_ap_service.sh"
        output = subprocess.call([topology_shell_script])

        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        time.sleep(2)

    def tearDown(self):
        # cleaning up
        subprocess.call(["happy-state-delete"])


    def test_weave_tunnel(self):
        # topology has nodes: ThreadNode, BorderRouter, onhub and cloud
        # we run tunnel between BorderRouter and cloud

        # Start tunnel
        value, data = self.__start_tunnel_between("BorderRouter", "cloud")

        time.sleep(1)

        value, data = self.__run_ping_test_between("ThreadNode", "cloud")
        self.__process_result("ThreadNode", "cloud", value, data)

        # Stop tunnel
        value, data = self.__stop_tunnel_between("BorderRouter", "cloud")


    def __start_tunnel_between(self, gateway, service):
        options = WeaveTunnelStart.option()
        options["quiet"] = True
        options["border_gateway"] = gateway
        options["service"] = service
        options["use_lwip"] = self.use_lwip
        if self.use_lwip:
            options["client_tap_wlan"] = "wlan0"
            options["client_tap_wpan"] = "wpan0"
            options["service_tap"] = "eth0"

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __run_ping_test_between(self, nodeA, nodeB):
        options = WeavePing.option()
        options["quiet"] = False
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["udp"] = True
        options["endpoint"] = "Tunnel"
        options["count"] = "10"
        options["use_lwip"] = self.use_lwip
        if self.use_lwip:
            options["client_tap"] = "wpan0"
            options["service_tap"] = "wlan0"
            options["border_gateway"] = "BorderRouter"

        weave_ping = WeavePing.WeavePing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __process_result(self, nodeA, nodeB, value, data):
        print "ping from " + nodeA + " to " + nodeB + " ",

        data = data[0]

        if value > 11:
            print hred("Failed")
        else:
            print hgreen("Passed")

        try:
            self.assertTrue(value < 11, "%s < 11 %%" % (str(value)))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            print "Client Output: "
            for line in data["client_output"].split("\n"):
               print "\t" + line

            print "Server Output: "
            for line in data["server_output"].split("\n"):
                print "\t" + line

            if self.show_strace == True:
                print "Server Strace: "
                for line in data["server_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data["client_strace"].split("\n"):
                    print "\t" + line

        if value > 11:
            raise ValueError("Weave Ping over Weave Tunnel Failed")


    def __stop_tunnel_between(self, gateway, service):
        options = WeaveTunnelStop.option()
        options["quiet"] = True
        options["border_gateway"] = gateway
        options["service"] = service

        weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



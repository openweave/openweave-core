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
#       Runs the following tunnel tests:
#       1. ping test: ThreadNode -> BorderRouter (border gw) -> tunnel + case -> cloud
#

import itertools
import os
import unittest
import set_test_path
import time

from happy.Utils import *
import happy.HappyNodeList
import WeavePing
import WeaveStateLoad
import WeaveStateUnload
import WeaveTunnelStart
import WeaveTunnelStop
import WeaveUtilities

class test_weave_tunnel_04(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_on_tap_ap_service.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_ap_service.json"

        self.show_strace = False

        # setting Mesh for thread test
        options = WeaveStateLoad.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        ret = setup_network.run()

        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        time.sleep(2)


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_tunnel(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")
            return

        # topology has nodes: ThreadNode, BorderRouter, onhub and cloud
        # we run tunnel between BorderRouter and cloud

        # Start tunnel
        value, data = self.__start_tunnel_between("BorderRouter", "cloud", use_case=True)

        time.sleep(1)

        value, data = self.__run_ping_test_between("ThreadNode", "cloud", use_case=False)
        self.__process_result("ThreadNode", "cloud", value, data)

        # Stop tunnel
        value, data = self.__stop_tunnel_between("BorderRouter", "cloud")


    def __start_tunnel_between(self, gateway, service, use_case=False):
        options = WeaveTunnelStart.option()
        options["quiet"] = True
        options["border_gateway"] = gateway
        options["service"] = service
        options["tap"] = self.tap
        options["case"] = use_case
        options["case_cert_path"] = 'default'

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __run_ping_test_between(self, nodeA, nodeB, use_case=False):
        options = WeavePing.option()
        options["quiet"] = False
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["udp"] = True
        options["endpoint"] = "Tunnel"
        options["count"] = "10"
        options["tap"] = self.tap
        options["case"] = use_case
        options["case_cert_path"] = 'default'

        weave_ping = WeavePing.WeavePing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __process_result(self, nodeA, nodeB, value, data):
        print "ping from " + nodeA + " to " + nodeB + " ",

        if value > 11:
            print hred("Failed")
        else:
            print hgreen("Passed")

        try:
            self.assertTrue(value < 11, "%s < 11 %%" % (str(value)))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            data = data[0]

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



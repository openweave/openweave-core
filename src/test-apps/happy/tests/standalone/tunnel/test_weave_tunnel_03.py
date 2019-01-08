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
#       Executes Weave Tunnel tests between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveTunnelTest
import WeaveUtilities

class test_weave_tunnel_03(unittest.TestCase):
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


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_tunnel(self):
        # TODO: Once LwIP bugs are fixed, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")        
            return

        # topology has nodes: ThreadNode, BorderRouter, onhub and cloud
        # we run tunnel between BorderRouter and cloud

        # Start tunnel
        value, data = self.__run_tunnel_tests_between("BorderRouter", "cloud")
        self.__process_result(value, data)

    def __process_result(self, value, data):

        if value == True:
            print hgreen("Passed")
        else:
            print hred("Failed")

        try:
            self.assertTrue(value == True, "%s == True %%" % (str(value)))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            data = data[0]

            print "Border gateway Output: "
            for line in data["gateway_output"].split("\n"):
                print "\t" + line

            print "Service Output: "
            for line in data["service_output"].split("\n"):
                print "\t" + line

            if self.show_strace:
                print "Service Strace: "
                for line in data["service_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data["gateway_strace"].split("\n"):
                    print "\t" + line

        if value != True:
            raise ValueError("Failure in Weave Tunnel Tests")

    def __run_tunnel_tests_between(self, gateway, service):
        options = WeaveTunnelTest.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["service"] = service
        options["tap"] = self.tap

        weave_tunnel = WeaveTunnelTest.WeaveTunnelTest(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



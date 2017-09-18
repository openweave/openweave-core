#!/usr/bin/env python


#
#    Copyright (c) 2016-2017 Nest Labs, Inc.
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
#       Calls Weave Echo between nodes, using authentication and encryption
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveSecurityPing as WeaveSecurityPing
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_echo_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if os.environ.get("WEAVE_SYSTEM_CONFIG_USE_LWIP") == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_wifi_weave.json"
            self.tap = "wlan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_wifi_weave.json"

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


    def test_weave_echo(self):
        # udp + CASE                               nodeA,    nodeB,    udp,  tcp,   wrmp,  pase, case, groupkey, test_tag
        # Note that CASE will use WRMP if it is compiled. 
        # Only the echo request will use pure UDP
        print "start UDP ping with CASE:"
        value, data = self.__run_ping_test_between("node01", "node02", True, False, False, False, True, False, "_CASE_UDP")
        self.__process_result("node01", "node02", value, data)

        # TCP + CASE
        print "start TCP ping with CASE:"
        value, data = self.__run_ping_test_between("node01", "node02", False, True, False, False, True, False, "_CASE_TCP")
        self.__process_result("node01", "node02", value, data)

        # TCP + PASE
        print "start TCP ping with PASE:"
        value, data = self.__run_ping_test_between("node01", "node02", False, True, False, True, False, False, "_PASE_TCP")
        self.__process_result("node01", "node02", value, data)

        # WRMP + group key
        print "start WRMP ping with group key encryption:"
        value, data = self.__run_ping_test_between("node01", "node02", True, False, True,  False, False, True, "_WRMP_GROUPKEY")
        self.__process_result("node01", "node02", value, data)

        # UDP + group key
        print "start UDP ping with group key encryption:"
        value, data = self.__run_ping_test_between("node01", "node02", True, False, False,  False, False, True, "_UDP_GROUPKEY")
        self.__process_result("node01", "node02", value, data)



    def __process_result(self, nodeA, nodeB, value, data):
        print "ping from " + nodeA + " to " + nodeB + " ",

        if value > 0:
            print hred("Failed")
        else:
            print hgreen("Passed")

        try:
            self.assertTrue(value == 0, "%s > 0 %%" % (str(value)))
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

        if value > 0:
            raise ValueError("Weave Ping Failed")


    def __run_ping_test_between(self, nodeA, nodeB, udp, tcp, wrmp, pase, case, groupkey, test_tag):
        options = WeaveSecurityPing.option()
        options["plaid"] = "auto"
        options["quiet"] = False
        options["client"] = nodeA
        options["server"] = nodeB
        options["udp"] = udp
        options["tcp"] = tcp
        options["wrmp"] = wrmp
        options["PASE"] = pase
        options["CASE"] = case
        options["group_key"] = groupkey
        options["count"] = "10"
        options["tap"] = self.tap
        options["test_tag"] = test_tag
        if udp and not wrmp and groupkey:
            # The first request is lost, and so we can never get 100% success on the first iteration
            options["iterations"] = 2

        weave_ping = WeaveSecurityPing.WeaveSecurityPing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



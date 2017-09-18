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
#       Calls Weave Echo between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeavePing as WeavePing
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_echo_03(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_thread_weave.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_thread_weave.json"

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
        print "start WRMP ping:"
        value, data = self.__run_ping_test_between("node01", "node02")
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
            for line in data[0]["client_output"].split("\n"):
               print "\t" + line

            print "Server Output: "
            for line in data[0]["server_output"].split("\n"):
                print "\t" + line

            if self.show_strace == True:
                print "Server Strace: "
                for line in data[0]["server_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data[0]["client_strace"].split("\n"):
                    print "\t" + line

        if value > 0:
            raise ValueError("Weave Ping Failed")


    def __run_ping_test_between(self, nodeA, nodeB):
        options = WeavePing.option()
        options["quiet"] = False
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["wrmp"] = True
        options["count"] = "10"
        options["tap"] = self.tap

        weave_ping = WeavePing.WeavePing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



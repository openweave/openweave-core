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
#       Run Weave WRMP test #1 between two Thread nodes.
#

import itertools
import os
import sys
import time
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveWRMP
import WeaveUtilities


class test_weave_wrmp(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/three_nodes_on_tap_thread_weave.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/three_nodes_on_thread_weave.json"

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


    def test_weave_wrmp_among_all_nodes(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")            
            return

        options = happy.HappyNodeList.option()
        options["quiet"] = True

        nodes_list = happy.HappyNodeList.HappyNodeList(options)
        ret = nodes_list.run()

        node_ids = ret.Data()
        pairs_of_nodes = [(node_ids[0], node_ids[1]), (node_ids[1], node_ids[0])]

        for pair in pairs_of_nodes:
            if pair[0] == pair[1]:
                print "Skip WRMP test on client and server running on the same node."
                continue

            for t in range(1,17):
                value, data = self.__run_wrmp_test_between(pair[0], pair[1], t)
                self.__process_result(pair[0], pair[1], value, data, t)


    def __process_result(self, nodeA, nodeB, value, data, test_num):
        print "wrmp from " + nodeA + " to " + nodeB + " (Test #" + str(test_num) + ") ",

        if value == True:
            print hgreen("Passed")
        else:
            print hred("Failed")

        try:
            self.assertTrue(value == True, "%s == True %%" % (str(value)))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            print "Client Output: "
            for line in data["client_output"].split("\n"):
                print "\t" + line

            print "Server Output: "
            for line in data["server_output"].split("\n"):
                print "\t" + line

            if self.show_strace:
                print "Server Strace: "
                for line in data["server_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data["client_strace"].split("\n"):
                    print "\t" + line

        if value != True:
            raise ValueError("WRMP #" + str(test_num) + " Failed")


    def __run_wrmp_test_between(self, nodeA, nodeB, test_num):
        options = WeaveWRMP.option()
        options["quiet"] = False
        options["client"] = nodeA
        options["server"] = nodeB
        options["test"] = test_num
        options["tap"] = self.tap
        options["strace"] = False
        weave_wrmp = WeaveWRMP.WeaveWRMP(options)
        ret = weave_wrmp.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()


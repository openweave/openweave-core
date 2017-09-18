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
#       Calls Weave Heartbeat between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveHeartbeat as WeaveHeartbeat
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_heartbeat_01(unittest.TestCase):
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


    def test_weave_heartbeat(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        nodes_list = happy.HappyNodeList.HappyNodeList(options)
        ret = nodes_list.run()

        node_ids = ret.Data()
        pairs_of_nodes = list(itertools.product(node_ids, node_ids))

        for pair in pairs_of_nodes:
            if pair[0] == pair[1]:
                print "Skip Heartbeat test on client and server running on the same node."
                continue

            value, data = self.__run_heartbeat_test_between(pair[0], pair[1])
            self.__process_result(pair[0], pair[1], value, data)


    def __process_result(self, nodeA, nodeB, value, data):
        print "heartbeat from " + nodeA + " to " + nodeB + " ",

        if value == self.count:
            print hgreen("Passed")
        else:
            print hred("Failed")

        try:
            self.assertTrue(value == self.count, "%d != %d" % (value, self.count))
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

        if value != self.count:
            raise ValueError("Weave Heartbeat Failed")


    def __run_heartbeat_test_between(self, nodeA, nodeB):
        options = WeaveHeartbeat.option()
        options["quiet"] = False
        options["client"] = nodeA
        options["server"] = nodeB
        options["tap"] = self.tap
        self.count = 10
        options["count"] = str(self.count)

        weave_heartbeat = WeaveHeartbeat.WeaveHeartbeat(options)
        ret = weave_heartbeat.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



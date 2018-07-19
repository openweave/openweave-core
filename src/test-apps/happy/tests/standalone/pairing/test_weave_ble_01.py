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
#       Calls Weave BLE between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveBle
import WeaveUtilities

class test_weave_ble_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if os.environ.get("WEAVE_SYSTEM_CONFIG_USE_LWIP") == "1":
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


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_ble(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        nodes_list = happy.HappyNodeList.HappyNodeList(options)
        ret = nodes_list.run()

        node_ids = ret.Data()
        result_list, data = self.__run_woble_test_between(node_ids[0], node_ids[1])
        client_output = data[0]["client_output"]
        server_output = data[0]["server_output"]

        passed = result_list[0]

        if not passed:
            print "Captured experiment result:"

            print "Client Output: "
            for line in client_output.split("\n"):
                print "\t" + line

            print "Server Output: "
            for line in server_output.split("\n"):
                print "\t" + line

            raise ValueError("The test failed")


    def __run_woble_test_between(self, nodeA, nodeB):
        options = WeaveBle.option()
        options["quiet"] = False
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["tap"] = self.tap

        weaveBle = WeaveBle.WeaveBle(options)
        ret = weaveBle.run()
        value = ret.Value()
        data = ret.Data()

        return value, data

if __name__ == "__main__":
    WeaveUtilities.run_unittest()



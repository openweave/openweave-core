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
#       Calls Weave WDMv0 Test between nodes.
#

import itertools
import os
import unittest
import set_test_path
import time
import random
import shutil
import string

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveWDMv0 as WeaveWDMv0
import plugin.WeaveTunnelStart as WeaveTunnelStart
import plugin.WeaveTunnelStop as WeaveTunnelStop
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_wdmv0_01(unittest.TestCase):
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


    def test_weave_wdmv0(self):
        # topology has nodes: node01(publisher), node02(client) and node03

        # WDMv0
        value, data = self.__weave_wdmv0_between("node01", "node02")
        self.__process_result("node01", "node02", value, data)


    def __process_result(self, nodeA, nodeB, value, data):

        if value:
            print hgreen("Passed")
        else:
            print hred("Failed")
            raise ValueError("Weave WDMv0 Test Failed")


    def __weave_wdmv0_between(self, nodeA, nodeB):
        options = WeaveWDMv0.option()
        options["quiet"] = False
        options["server"] = nodeA
        options["client"] = nodeB
        options["tap"] = self.tap

        weave_wdmv0 = WeaveWDMv0.WeaveWDMv0(options)
        ret = weave_wdmv0.run()

        value = ret.Value()
        data = ret.Data()

        return value, data

if __name__ == "__main__":
    WeaveUtilities.run_unittest()



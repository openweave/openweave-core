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
#       Calls Weave pairing between the mobile and device.
#

import os
import unittest
import set_test_path

from happy.Utils import *
import WeaveStateLoad
import WeaveStateUnload
import WeavePairing
import WeaveUtilities


class test_weave_pairing_01(unittest.TestCase):
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


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_pairing(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")            
            return

        # topology has nodes: node01(mobile), node02(device) and node03

        # Pairing
        result_list, data = self.__run_weave_pairing_between("node01", "node02", "node03")

        mobiles_output = data["mobiles_output"]
        devices_output = data["devices_output"]

        passed = result_list[0]

        if not passed:
            print "Captured experiment result:"

            print "Mobile Output: "
            for line in mobiles_output[0].split("\n"):
               print "\t" + line

            print "Device Output: "
            for line in devices_output[0].split("\n"):
                print "\t" + line

            raise ValueError("The test failed")


    def __run_weave_pairing_between(self, nodeA, nodeB, nodeC):

        options = WeavePairing.option()
        options["quiet"] = False
        options["mobile"] = nodeA
        options["devices"] = [nodeB]
        options["server"] = nodeC
        options["tap"] = self.tap

        weave_pairing = WeavePairing.WeavePairing(options)
        ret = weave_pairing.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



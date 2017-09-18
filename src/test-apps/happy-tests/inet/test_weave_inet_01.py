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
#       Calls Weave Inet test between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveInet as WeaveInet
import plugin.WeaveUtilities as WeaveUtilities

count = 2
length = 500

class test_weave_inet_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/thread_wifi_on_tap_ap_service.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/thread_wifi_ap_service.json"

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


    def test_weave_inet(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            return

        options = happy.HappyNodeList.option()
        options["quiet"] = True

        value, data = self.__run_inet_test_between("BorderRouter", "ThreadNode", "udp")
        self.__process_result("BorderRouter", "ThreadNode", value, data, "udp")

        value, data = self.__run_inet_test_between("BorderRouter", "ThreadNode", "tcp")
        self.__process_result("BorderRouter", "ThreadNode", value, data, "tcp")

        value, data = self.__run_inet_test_between("BorderRouter", "ThreadNode", "raw6")
        self.__process_result("BorderRouter", "ThreadNode", value, data, "raw6")

        value, data = self.__run_inet_test_between("onhub", "cloud", "raw4")
        self.__process_result("onhub", "cloud", value, data, "raw4")


    def __process_result(self, nodeA, nodeB, value, data, protocol_type):
        print "inet %s test from " % protocol_type + nodeA + " to " + nodeB + " ",

        if value:
            print hgreen("Passed")
        else:
            print hred("Failed")
            raise ValueError("Weave Inet Test Failed")


    def __run_inet_test_between(self, nodeA, nodeB, protocol_type):
        options = WeaveInet.option()
        options["quiet"] = False
        options["client"] = nodeA
        options["server"] = nodeB
        options["type"] = protocol_type
        options["count"] = str(count)
        options["length"] = str(length)
        options["tap"] = self.tap

        weave_inet = WeaveInet.WeaveInet(options)
        ret = weave_inet.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



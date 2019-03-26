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
import WeaveInetDNS
import WeaveUtilities
import subprocess

count = 2
length = 500

class test_inet_dns(unittest.TestCase):
    def setUp(self):
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.using_lwip = True
        else:
            self.using_lwip = False

        topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
            "/topology/two_nodes_on_tap_wifi.sh"
        output = subprocess.call([topology_shell_script])

    def tearDown(self):
        # cleaning up
        subprocess.call(["happy-state-delete"])

    def test_inet_dns(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        value, data = self.__run_inet_dns_test("node01")
        self.__process_result(value, data)

    def __process_result(self, value, data):
        """
        process result returned from inet dns test template
        """
        if value:
            print hgreen("PASSED")
        else:
            print hred("FAILED")
            raise ValueError("Weave Inet DNS Test Failed")

    def __run_inet_dns_test(self, node):
        options = WeaveInetDNS.option()
        options["quiet"] = False
        options["node_id"] = node
        options["tap_if"] = "wlan0"
        options["prefix"] = "10.0.1"
        options["ipv4_gateway"] = "10.0.1.2"
        options["dns"] = "8.8.8.8"
        options["use_lwip"] = self.using_lwip

        weave_inet_dns = WeaveInetDNS.WeaveInetDNS(options)
        ret = weave_inet_dns.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



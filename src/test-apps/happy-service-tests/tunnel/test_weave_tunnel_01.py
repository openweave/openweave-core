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
#       A01: Calls Weave Tunnel between a simulated BorderRouter and NestService
#

import itertools
import os
import re
import sys
import unittest
import set_test_path

from happy.Utils import *
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveTunnelStart as WeaveTunnelStart
import plugin.WeaveTunnelStop as WeaveTunnelStop
import plugins.testrail.TestrailResultOutput

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from topologies.thread_wifi_ap_internet_configurable_topology import thread_wifi_ap_internet_configurable_topology

TEST_OPTION_QUIET = True
DEFAULT_FABRIC_SEED = "00000"
TESTRAIL_SECTION_NAME = "Weave tunnel between Mock-Client and Real-Service"
TESTRAIL_SUFFIX = "_TESTRAIL.json"


class test_weave_tunnel_01(unittest.TestCase):
    def setUp(self):
        self.tap = None
        self.tap_id = None
        self.quiet = TEST_OPTION_QUIET
        self.options = None

        self.topology_setup_required = int(os.environ.get("TOPOLOGY", "1")) == 1

        self.count = os.environ.get("ECHO_COUNT", "400")

        fabric_seed = os.environ.get("FABRIC_SEED", DEFAULT_FABRIC_SEED)

        if "FABRIC_OFFSET" in os.environ.keys():
            self.fabric_id = format(int(fabric_seed, 16) + int(os.environ["FABRIC_OFFSET"]), 'x').zfill(5)
        else:
            self.fabric_id = fabric_seed

        self.eui64_prefix = os.environ.get("EUI64_PREFIX", '18:B4:30:AA:00:')

        self.customized_eui64_seed = self.eui64_prefix + self.fabric_id[0:2] + ':' + self.fabric_id[2:4] + ':' + self.fabric_id[4:]

        # TODO: Once LwIP bugs for tunnel are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.tap = True
            self.tap_id = "wpan0"
            return
        else:
            self.tap = False

        self.case = int(os.environ.get("CASE", "0")) == 1

        self.use_service_dir = int(os.environ.get("USE_SERVICE_DIR", "0")) == 1

        if self.topology_setup_required:
            self.topology = thread_wifi_ap_internet_configurable_topology(quiet=self.quiet,
                                                                          fabric_id=self.fabric_id,
                                                                          customized_eui64_seed=self.customized_eui64_seed,
                                                                          tap=self.tap,
                                                                          dns=None)
            self.topology.createTopology()
        else:
            print "topology set up not required"

        self.show_strace = False

        self.weave_wdm = None
        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        delayExecution(2)


    def tearDown(self):
        if self.topology_setup_required:
            self.topology.destroyTopology()
            self.topology = None


    def test_weave_tunnel(self):

        # topology has nodes: ThreadNode, BorderRouter, onhub and NestService instance
        # we run tunnel between BorderRouter and NestService

        # Start tunnel
        value, data = self.__start_tunnel_from("BorderRouter")

        delayExecution(1)

        # Stop tunnel
        value, data = self.__stop_tunnel_from("BorderRouter")

        self.__process_result("BorderRouter", "service", value, data)

    def __start_tunnel_from(self, gateway):
        options = WeaveTunnelStart.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["tap"] = self.tap_id
        options["case"] = self.case
        options["service_dir"] = self.use_service_dir

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __stop_tunnel_from(self, gateway):
        options = WeaveTunnelStop.option()
        options["quiet"] = False
        options["border_gateway"] = gateway

        options["service_dir"] = self.use_service_dir

        self.weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = self.weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data

    def __process_result(self, nodeA, nodeB, value, data):
        condition = 'Connection established to node 18b4300200000011'

        found = re.search(condition, data["gateway_output"])
        if found:
            success = True
            print hgreen("Passed")
        else:
            success = False
            print hred("Failed")
            print "Captured experiment result:"

            print "Gateway Output: "
            for line in data["gateway_output"].split("\n"):
                print "\t" + line

        test_results = []

        test_results.append({
            'testName': 'Tunnel-NestService-A01: Calls Weave Tunnel between a simulated BorderRouter and NestService',
            'testStatus': 'success' if success else 'failure',
            'testDescription': 'check if we can find %s' % condition,
            'success_iterations_count': 1 if success else 0,
            'total_iterations_count': 1
        })

        # output the test result to a json file
        output_data = {
            'sectionName': TESTRAIL_SECTION_NAME,
            'testResults': test_results
        }

        output_file_name = self.weave_tunnel.process_log_prefix + self.__class__.__name__ + TESTRAIL_SUFFIX

        self.__output_test_result(output_file_name, output_data)

        if not success:
            raise ValueError('test failure')


    def __output_test_result(self, file_path, output_data):
        options = plugins.testrail.TestrailResultOutput.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["file_path"] = file_path
        options["output_data"] = output_data

        output_test = plugins.testrail.TestrailResultOutput.TestrailResultOutput(options)
        output_test.run()

if __name__ == "__main__":
    unittest.main()



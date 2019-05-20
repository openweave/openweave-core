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
#       Calls Weave Tunnel between a simulated BorderRouter and NestService.
#       After the tunnel is establish, weave-pairing  interaction will be executed between the Mobile to ThreadNode,
#       and ThreadNode register to NestService.
#

import os
import re
import sys
import unittest
import set_test_path

from happy.Utils import *

import WeavePairing
import WeaveTunnelStart
import WeaveTunnelStop
import plugins.testrail.TestrailResultOutput

from topologies.dynamic.thread_wifi_ap_internet_configurable_topology import thread_wifi_ap_internet_configurable_topology

TEST_OPTION_QUIET = True
TESTRAIL_SECTION_NAME = "Weave pairing between Mock-Client and Real-Service"
TESTRAIL_SUFFIX = "_TESTRAIL.json"
DEFAULT_FABRIC_SEED = "00000"


class test_weave_pairing_01(unittest.TestCase):
    def setUp(self):
        self.tap = None
        self.tap_id = None
        self.quiet = TEST_OPTION_QUIET
        self.options = None

        if "weave_service_address" in os.environ.keys():
            if "unstable" not in os.environ["weave_service_address"]:
                found = re.search('.(\w+).nestlabs.com', os.environ["weave_service_address"])
                self.tier = found.group(1)
                self.customized_tunnel_port = None
            else:
                self.customized_tunnel_port = 9204

        self.topology_setup_required = int(os.environ.get("TOPOLOGY", "1")) == 1

        fabric_seed = os.environ.get("FABRIC_SEED", DEFAULT_FABRIC_SEED)

        if "FABRIC_OFFSET" in os.environ.keys():
            self.fabric_id = format(int(fabric_seed, 16) + int(os.environ["FABRIC_OFFSET"]), 'x').zfill(5)
        else:
            self.fabric_id = fabric_seed

        self.eui64_prefix = os.environ.get("EUI64_PREFIX", '18:B4:30:AA:00:')

        self.customized_eui64_seed = self.eui64_prefix + self.fabric_id[0:2] + ':' + self.fabric_id[2:4] + ':' + self.fabric_id[4:]

        self.device_numbers = int(os.environ.get("DEVICE_NUMBERS", 1))

        self.test_timeout = int(os.environ.get("TEST_TIMEOUT", 60 * 30))

        self.username = os.environ.get("WEAVE_USERNAME", "test-it+weave_happy_fabric%s@nestlabs.com" % self.fabric_id)

        self.password = os.environ.get("WEAVE_PASSWORD", "nest-egg")

        self.initial_device_index = int(os.environ.get("INITIAL_DEVICE_INDEX", "8"))

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
                                                                          dns=None,
                                                                          device_numbers=self.device_numbers,
                                                                          mobile=True,
                                                                          initial_device_index=self.initial_device_index)
            self.topology.createTopology()
        else:
            print "topology set up not required"

        self.show_strace = False

        self.weave_wdm = None
        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        delayExecution(2)

        # topology has nodes: ThreadNode, BorderRouter, onhub and NestService instance
        # we run tunnel between BorderRouter and NestService

        # Start tunnel
        value, data = self.__start_tunnel_from("BorderRouter")
        delayExecution(1)

    def tearDown(self):
        delayExecution(1)
        # Stop tunnel
        value, data = self.__stop_tunnel_from("BorderRouter")
        if self.topology_setup_required:
            self.topology.destroyTopology()
            self.topology = None


    def test_weave_pairing(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")            
            return

        # topology has nodes: ThreadNode, BorderRouter, onhub and NestService instance
        # we run pairing between BorderRouter and NestService depends on mobile
        value, data = self.__run_pairing_test_between("ThreadNode", "mobile")
        self.__process_result("BorderRouter", "service", value, data)

        delayExecution(1)


    def __start_tunnel_from(self, gateway):
        options = WeaveTunnelStart.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["tap"] = self.tap_id

        options["case"] = self.case
        options["service_dir"] = self.use_service_dir

        if self.customized_tunnel_port:
            options["customized_tunnel_port"] = self.customized_tunnel_port

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __run_pairing_test_between(self, nodeA, nodeB):
        options = WeavePairing.option()
        options["quiet"] = False
        options["mobile"] = nodeB
        options["devices"] = [nodeA + str(index + self.initial_device_index).zfill(2) for index in range(self.device_numbers)]
        options["server"] = "service"
        options["tap"] = self.tap_id
        options["username"] = self.username
        options["password"] = self.password
        options["tier"] = self.tier
        self.weave_pairing = WeavePairing.WeavePairing(options)
        ret = self.weave_pairing.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __process_result(self, nodeA, nodeB, value, data):
        success = True
        for result, device_output, device_strace, device_info in zip(value, data['devices_output'], data['devices_strace'], data['devices_info']):
            print "Service provision from " + nodeA + " to real NestService using " + nodeB + " ",

            if result is True:
                print hgreen("Passed")
            else:
                success = False
                print hred("Failed")

            try:
                self.assertTrue(value, "result %s == True %%" % (str(result)))
            except AssertionError, e:
                print str(e)
                print "Captured experiment result:"

                print "Device Output: "
                for line in device_output.split("\n"):
                   print "\t" + line

                if self.show_strace == True:
                    print " Device Strace: "
                    for line in device_strace.split("\n"):
                        print "\t" + line

            test_results = []

            test_results.append({
                'testName': 'Pairing-NestService-A01: Weave pairing between Mock-Client and Real-Service',
                'testStatus': 'success' if result else 'failure',
                'testDescription': "Do weave pairing between Mock-Client %s and Real-Service" % device_info['device_weave_id'],
                'success_iterations_count': 1 if result else 0,
                'total_iterations_count': 1
            })

            # output the test result to a json file
            output_data = {
                'sectionName': TESTRAIL_SECTION_NAME,
                'testResults': test_results
            }

            output_file_name = self.weave_pairing.process_log_prefix + device_info['device'] +  self.__class__.__name__ + TESTRAIL_SUFFIX

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


    def __stop_tunnel_from(self, gateway):
        options = WeaveTunnelStop.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["service_dir"] = self.use_service_dir
        weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    unittest.main()



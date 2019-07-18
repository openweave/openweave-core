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
#       Run Weave Service Directory Profile between a client and service.
#

import itertools
import os
import unittest
import set_test_path
import getopt
import sys

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveServiceDir
import WeaveUtilities
import subprocess

gFaultopts = WeaveUtilities.FaultInjectionOptions(nodes=["client", "service"])
gOptions = { "fault-injection": False }

class test_service_directory_01(unittest.TestCase):
    def setUp(self):
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.use_lwip = True
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_on_tap_ap_service.sh"
            # tap interface, ipv4 gateway and node addr should be provided if device is tap device
            # both BorderRouter and cloud node are tap devices here
            self.BR_tap = "wlan0"
            self.BR_ipv4_gateway = "10.0.1.2"
            self.BR_node_addr = "10.0.1.3"
            self.cloud_tap = "eth0"
            self.cloud_ipv4_gateway = "192.168.100.2"
            self.cloud_node_addr = "192.168.100.3"
        else:
            self.use_lwip = False
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_ap_service.sh"
        output = subprocess.call([topology_shell_script])

    def tearDown(self):
        # cleaning up
        subprocess.call(["happy-state-delete"])

    def test_service_directory(self):

        num_tests = 0
        num_failed_tests = 0
        failed_tests = []

        test_tag = "happy_sequence"

        value, data = self.__run_service_directory_test_between("BorderRouter", "cloud", test_tag = test_tag)
        if not value:
            num_failed_tests += 1
            failed_tests.append(test_tag)
        num_tests += 1

        output_logs = {}
        output_logs['client'] = data['client_output']
        output_logs['service'] = data['service_output']

        if gOptions["fault-injection"]:
            for node in gFaultopts.nodes:
                restart = True

                fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node], restart)

                for fault_config in fault_configs:
                    test_tag = "_" + str(num_tests) + "_" + node + "_" + fault_config
                    print "tag: " + test_tag
                    value, data = self.__run_service_directory_test_between("BorderRouter", "cloud", num_iterations = 3,
                                                                            faults = {node: fault_config}, test_tag = test_tag)
                    if not value:
                        num_failed_tests += 1
                        failed_tests.append(test_tag)
                        self.assertTrue(value);
                    num_tests += 1

        print "executed %d cases" % num_tests
        print "failed %d cases:" % num_failed_tests
        if num_failed_tests > 0:
            for failed in failed_tests:
                print "    " + failed
        self.assertEqual(num_failed_tests, 0, "Something failed")


    def __run_service_directory_test_between(self, nodeA, nodeB, num_iterations=1, faults = {}, test_tag = ""):
        options = WeaveServiceDir.option()
        options["quiet"] = False
        options["client"] = nodeA
        options["service"] = nodeB
        options["use_lwip"] = self.use_lwip
        if self.use_lwip:
            options["client_tap"] = self.BR_tap
            options["client_ipv4_gateway"] = self.BR_ipv4_gateway
            options["client_node_addr"] = self.BR_node_addr
            options["service_tap"] = self.cloud_tap
            options["service_ipv4_gateway"] = self.cloud_ipv4_gateway
            options["service_node_addr"] = self.cloud_node_addr
        options["client_faults"] = faults.get("client")
        options["service_faults"] = faults.get("service")
        options["iterations"] = num_iterations
        options["test_tag"] = test_tag
        options["plaid"] = "auto"

        service_dir = WeaveServiceDir.WeaveServiceDir(options)
        ret = service_dir.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    help_str = """usage:
    --help  Print this usage info and exit
    --fault-injection  Enable the fault-injection loop\n"""

    help_str += gFaultopts.help_string

    longopts = ["help", "fault-injection"]
    longopts.extend(gFaultopts.getopt_config)

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h", longopts)

    except getopt.GetoptError as err:
        print help_str
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    opts = gFaultopts.process_opts(opts)

    for o, a in opts:
        if o in ("-h", "--help"):
            print help_str
            sys.exit(0)

        elif o in ("--fault-injection"):
            gOptions["fault-injection"] = True

    sys.argv = [sys.argv[0]]

    WeaveUtilities.run_unittest()



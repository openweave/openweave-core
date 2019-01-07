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

gFaultopts = WeaveUtilities.FaultInjectionOptions(nodes=["client", "service"])
gOptions = { "fault-injection": False }

class test_service_directory_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_on_tap_ap_service.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_ap_service.json"

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


    def test_service_directory(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")            
            return

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
        options["tap"] = self.tap
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



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
#       Calls Weave Echo between nodes, with case and fault-injection
#

import itertools
import os
import getopt
import sys
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveSecurityPing as WeaveSecurityPing
import plugin.WeaveUtilities as WeaveUtilities

gFaultopts = WeaveUtilities.FaultInjectionOptions()
gOptions = { 'wrmp': False, 'pase': False, 'group_key': False }

class test_weave_echo_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if os.environ.get("WEAVE_SYSTEM_CONFIG_USE_LWIP") == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_wifi_weave.json"
            self.tap = "wlan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_wifi_weave.json"

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


    def test_weave_echo(self):

        wrmp = gOptions['wrmp']
        transport = "WRMP" if wrmp else "TCP"
        pase = gOptions['pase']
        auth = "PASE" if pase else "CASE"
        groupkey = gOptions['group_key']
        if groupkey:
            auth = "group_key" 

        print "start " + transport + " ping with " + auth + ":"
        value, data = self.__run_ping_test_between("node01", "node02", wrmp, auth)
        self.__process_result("node01", "node02", value, data)

        output_logs = {}
        output_logs['client'] = data['client_output']
        output_logs['server'] = data['server_output']

        num_tests = 0
        num_failed_tests = 0
        failed_tests = []

        for node in gFaultopts.nodes:
            restart = True

            fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node], restart)

            for fault_config in fault_configs:
                test_tag = "_" + auth + "_" + transport + "_" + str(num_tests) + "_" + node + "_" + fault_config
                print "tag: " + test_tag
                # We need to run this with 3 iterations, since when the server restarts at the end of the first one
                # sometimes the client fails to reconnect and therefore the second iteration fails as well
                value, data = self.__run_ping_test_between("node01", "node02", wrmp, auth, num_iterations = 3, faults = {node: fault_config}, test_tag = test_tag)
                if self.__process_result("node01", "node02", value, data):
                    num_failed_tests += 1
                    failed_tests.append(test_tag)
                num_tests += 1

        print "executed %d cases" % num_tests
        print "failed %d cases:" % num_failed_tests
        if num_failed_tests > 0:
            for failed in failed_tests:
                print "    " + failed
        self.assertEqual(num_failed_tests, 0, "Something failed")


    def __process_result(self, nodeA, nodeB, value, data):
        print "ping from " + nodeA + " to " + nodeB + " ",

        if value > 0:
            print hred("Failed")
        else:
            print hgreen("Passed")

        if data["other_failure"] > 0:
            print hred("other failure detected")

        failed = False
        if value > 0 or data["other_failure"]:
            failed = True

        return failed


    def __run_ping_test_between(self, nodeA, nodeB, wrmp, auth, num_iterations = None, faults = {}, test_tag = "" ):
        options = WeaveSecurityPing.option()
        options["plaid"] = "auto"
        options["quiet"] = False
        options["client"] = nodeA
        options["server"] = nodeB
        options["udp"] = wrmp
        options["wrmp"] = wrmp
        options["tcp"] = not wrmp
        options["CASE"] = False
        options["PASE"] = False
        options["group_key"] = False
        if auth == "CASE":
            options["CASE"] = True
        elif auth == "PASE":
            options["PASE"] = True
        elif auth == "group_key":
            options["group_key"] = True
        options["count"] = "1"
        options["tap"] = self.tap
        options["client_faults"] = faults.get('client')
        options["server_faults"] = faults.get('server')
        options["iterations"] = num_iterations
        options["test_tag"] = test_tag

        weave_ping = WeaveSecurityPing.WeaveSecurityPing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    help_str = """usage:
    --help  Print this usage info and exit
    --wrmp  Use WRMP over UDP instead of TCP
    --tcp   Use TCP (on by default)
    --pase  Use PASE instead of CASE
    --case  Use CASE (on by default)
    --group-key Use a group key instead of CASE\n"""

    help_str += gFaultopts.help_string

    longopts = ["help", "wrmp", "tcp", "pase", "case", "group-key"]
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

        elif o in ("--wrmp"):
            gOptions["wrmp"] = True
        elif o in ("--pase"):
            gOptions["pase"] = True
        elif o in ("--case"):
            gOptions["case"] = True
        elif o in ("--tcp"):
            gOptions["tcp"] = True
        elif o in ("--group-key"):
            gOptions["group_key"] = True

    if gOptions["group_key"] and not gOptions["wrmp"]:
        msg = "--group-key requires --wrmp"
        print hred(msg)
        sys.exit(-1)

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()



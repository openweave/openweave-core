#!/usr/bin/env python3


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
#       WeaveTimeSync test with support for the following sync modes:
#           - local
#           - service
#           - auto
#       Also available: fault-injection (enabled via command-line option)
#

from __future__ import absolute_import
from __future__ import print_function
import itertools
import os
import getopt
import sys
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveTime
import WeaveUtilities
import plugins.plaid.Plaid as Plaid

gFaultOpts = WeaveUtilities.FaultInjectionOptions()
gOptions = { 'enableFaults': False, 'mode': "local" }

class test_weave_time_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in list(os.environ.keys()) and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
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

        # set up Plaid for faster execution
        plaid_opts = Plaid.default_options()
        plaid_opts['num_clients'] = 3
        if gOptions["mode"] == "service":
            plaid_opts['num_clients'] = 2
            plaid_opts["max_time_at_high_speed_secs"] = 40
        if gOptions["mode"] == "local":
            plaid_opts["max_time_at_high_speed_secs"] = 80
        if gOptions["mode"] == "auto":
            plaid_opts["max_time_at_high_speed_secs"] = 40
        plaid_opts['strace'] = self.show_strace
        self.plaid = Plaid.Plaid(plaid_opts)
        self.use_plaid = self.plaid.isPlaidConfigured()


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_time(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        mode = gOptions['mode']

        # The following no-fault run is used to compute the fault counters
        # that are used to generate a set of tests with faults injected.
        # Note that the nodes are allowed to run at plaid speed only for the
        # duration of the sequence; otherwise, the nodes would race ahead in
        # time while the python script is trying to terminate them; this would
        # cause very large event counters that then cause the generation of
        # useless tests.

        if self.use_plaid:
            # print "starting plaid server"
            self.__start_plaid_server()

        value, data = self.__run_time_test_between("node01", "node02",
                                                   "node03", mode, use_plaid=self.use_plaid)

        if self.use_plaid:
            # print "stopping plaid server"
            self.__stop_plaid_server()

        err = self.__process_result("node01", "node02", "node03", mode, value)
        # err=True => failure, err=False => success
        self.assertEqual(err, False, "failed to run success case")


        if not gOptions['enableFaults']:
            return

        output_logs = {}
        output_logs['client'] = data['client_output']
        output_logs['coordinator'] = data['coordinator_output']
        output_logs['server'] = data['server_output']

        num_tests = 0
        num_failed_tests = 0
        failed_tests = []

        # During the fault injection tests, we don't need to limit the time executed at plaid speed.
        self.plaid.max_time_at_high_speed_secs = 0

        for node in gFaultOpts.nodes:
            restart = True

            fault_configs = gFaultOpts.generate_fault_config_list(node, output_logs[node], restart) 

            for fault_config in fault_configs:
                test_tag = "_" + "_".join([str(x) for x in (num_tests, node, fault_config)])
                print("tag: ", test_tag)

                if self.use_plaid:
                    # print "starting plaid server"
                    self.__start_plaid_server()

                value, data = self.__run_time_test_between("node01", "node02",
                                "node03", mode, num_iterations=3,
                                faults = {node: fault_config}, test_tag=test_tag, use_plaid=self.use_plaid)

                if self.__process_result("node01", "node02", "node03", mode, value):
                    # returns 'True' if test failed
                    num_failed_tests += 1
                    failed_tests.append(test_tag)

                if self.use_plaid:
                    # print "stopping plaid server"
                    self.__stop_plaid_server()

                num_tests += 1

        print("executed %d cases" % num_tests)
        print("failed %d cases:" % num_failed_tests)
        if num_failed_tests > 0:
            for failed in failed_tests:
                print("    " + failed)
        self.assertEqual(num_failed_tests, 0, "Something failed")


    def __process_result(self, nodeA, nodeB, nodeC, mode, value):
        print("time sync test among client:" + nodeA + \
              ", coordinator:" + nodeB + ", server:" + nodeC + \
              ", sync mode:" + mode)

        if value:
            print(hgreen("Passed"))
            failed = False
        else:
            print(hred("Failed"))
            failed = True

        return failed


    def __run_time_test_between(self, nodeA, nodeB, nodeC, mode, num_iterations=None, faults = {}, test_tag = "", use_plaid=False):
        options = WeaveTime.option()
        options["quiet"] = False
        options["client"] = nodeA
        options["coordinator"] = nodeB
        options["server"] = nodeC
        options["mode"] = mode
        options["tap"] = self.tap
        options["client_faults"] = faults.get('client')
        options["coordinator_faults"] = faults.get('coordinator')
        options["server_faults"] = faults.get('server')
        options["iterations"] = num_iterations
        options["test_tag"] = test_tag

        if use_plaid:
            options["plaid_client_env"] = self.plaid.getPlaidClientLibEnv(nodeA)
            options["plaid_coordinator_env"] = self.plaid.getPlaidClientLibEnv(nodeB)
            options["plaid_server_env"] = self.plaid.getPlaidClientLibEnv(nodeC)

        weave_time = WeaveTime.WeaveTime(options)
        ret = weave_time.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __start_plaid_server(self):
        self.plaid.startPlaidServerProcess()


    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


if __name__ == "__main__":
    help_str = """usage:
    --help           Print this usage info and exit
    --syncmode       Time sync mode (local, service, auto)
    --enable-faults  Run fault injection tests\n"""

    help_str += gFaultOpts.help_string

    longopts = ["help", "enable-faults", "syncmode="]
    longopts.extend(gFaultOpts.getopt_config)

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hfm:", longopts)

    except getopt.GetoptError as err:
        print(help_str)
        print(hred(str(err)))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    opts = gFaultOpts.process_opts(opts)

    for o, a in opts:
        if o in ("-h", "--help"):
            print(help_str)
            sys.exit(0)

        elif o in ("-m", "--syncmode"):
            gOptions["mode"] = a

        elif o in ("-f", "--enable-faults"):
            gOptions["enableFaults"] = True

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()


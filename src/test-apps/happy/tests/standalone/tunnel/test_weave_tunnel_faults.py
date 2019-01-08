#!/usr/bin/env python

#
#    Copyright (c) 2017 Nest Labs, Inc.
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
#       Tests the following scenarios:
#       1. Fault injection tests between the Border Gateway and Tunnel
#          Front End with faults limited to tunnel establishment and
#          teardown interactions. weave-ping is used to test tunnel
#          connectivity while faults are injected.
#

import itertools
import os
import unittest
import set_test_path
import time
import json
import getopt
import sys

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeavePing
import WeaveTunnelStart
import WeaveTunnelStop
import WeaveUtilities
from WeaveTest import WeaveTest
import plugins.plaid.Plaid as Plaid


gFaultopts = WeaveUtilities.FaultInjectionOptions(nodes=["gateway", "service"])
gOptions = { 'case': True, "num_pings": "3", "restart": True }

class test_weave_tunnel_faults(unittest.TestCase):
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
        options["json_file"] = self.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        ret = setup_network.run()

        # set up Plaid for faster execution
        # The tunnel tests need the amount time run at plaid speed to be
        # limited to avoid the gateway to race ahead and process more events
        # while the test script is sending it a SIGUSR1 to trigger a cleanup.
        # Without a limit on the amount of time spent at plaid-speed, the
        # gateway has time to process extra keepalive messages that will make the
        # test not consistent with an execution at realtime speed.
        # The number below is determined by looking at the logs of a real-time run.
        plaid_opts = Plaid.default_options()
        plaid_opts["max_time_at_high_speed_secs"] = 10
        plaid_opts['num_clients'] = 3
        plaid_opts['strace'] = self.show_strace
        self.plaid = Plaid.Plaid(plaid_opts)
        self.use_plaid = self.plaid.isPlaidConfigured()


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_tunnel(self):
        # TODO: Once LwIP bugs are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print hred("WARNING: Test skipped due to LwIP-based network cofiguration!")
            return

        # topology has nodes: ThreadNode, BorderRouter, onhub and cloud
        # we run tunnel between BorderRouter and cloud

        use_case = gOptions["case"]

        if self.use_plaid:
            self.__start_plaid_server()


        test_tag = "_HAPPY_SEQUENCE"

        # Start tunnel
        value_start, start_tunnel_data = self.__start_tunnel_between("BorderRouter", "cloud", use_case=use_case, test_tag=test_tag, use_plaid=self.use_plaid)

        # Run ping test
        for i in range(10):
            print "  running weave-ping between 'ThreadNode' and 'cloud' (iteration:%s/10)" %(str(i+1))
            value_ping, data_ping = self.__run_ping_test_between("ThreadNode", "cloud", use_case=False, test_tag = test_tag, use_plaid=self.use_plaid)
            if not value_ping: # 0 == SUCCESS:
                # we're done, no need for more iterations.
                break

        # Stop tunnel
        value_stop, stop_tunnel_data = self.__stop_tunnel_between("BorderRouter", "cloud", test_tag=test_tag)

        if self.use_plaid:
            self.__stop_plaid_server()

        output_logs = {}

        output_logs['gateway'] = stop_tunnel_data['gateway_output']
        output_logs['service'] = stop_tunnel_data['service_output']

        num_tests = 0
        num_failed_tests = 0
        failed_tests = []

        for node in gFaultopts.nodes:
            restart = gOptions["restart"]

            if node == "service":
                # TODO: due to a bug in mock-tunnel-service, tunnel data messages are not processed
                # after restart. Hence, all restart cases fail. Re-enable after fixing
                # mock-tunnel-service.
                restart = False

            fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node], restart)

            for fault_config in fault_configs:
                auth = "CASE" if use_case else "None"
                test_tag = "_" + auth + "_" + str(num_tests) + "_" + node + "_" + fault_config
                print "tag: " + test_tag

                if self.use_plaid:
                    self.__start_plaid_server()

                # Start tunnel
                print "  starting tunnel between 'BorderRouter' and 'cloud'"
                value_start, data_start_tunnel = self.__start_tunnel_between("BorderRouter", "cloud", use_case=use_case, num_iterations = 3, faults = {node: fault_config}, test_tag = test_tag, use_plaid=self.use_plaid)

                for i in range(15):
                    # Note that when running on plaid this loop only executes ~3 iterations in what is a minute of virtual time
                    # Without plaid, the number of iterations required is much more
                    print "  running weave-ping between 'ThreadNode' and 'cloud' (iteration:%s/15)" %(str(i+1))
                    value_ping, data_ping = self.__run_ping_test_between("ThreadNode", "cloud", use_case=False, test_tag = test_tag, use_plaid=self.use_plaid)
                    if not value_ping: # 0 == SUCCESS:
                        # we're done, no need for more iterations.
                        break

                # if tunnel goes down after ping succeeds, give it up to 15s to recover. 
                wt = WeaveTest()
                for i in range(15):
                    gw_value, gw_output = wt.get_test_output("BorderRouter", "WEAVE-GATEWAY-TUNNEL" + test_tag, quiet=True)
                    if self.__is_tunnel_up(gw_output):
                        break
                    print "  *** waiting for tunnel to recover *** (%s/15 seconds)" %(str(i+1))
                    time.sleep(1)

                # Stop tunnel
                print "  stopping tunnel between 'BorderRouter' and 'cloud'"
                value_stop, data_stop_tunnel = self.__stop_tunnel_between("BorderRouter", "cloud", test_tag = test_tag)

                if self.use_plaid:
                    self.__stop_plaid_server()

                results = self.__process_tunnel_results("BorderRouter", value_stop, data_stop_tunnel, value_ping, data_ping, test_tag=test_tag)

                success = reduce(lambda x, y: x and y, results.values())
                if not success:
                    print hred("  Failed + [" + test_tag + "]")
                    num_failed_tests += 1
                    failed_tests.append(test_tag)
                else:
                    print hgreen("  Passed [" + test_tag + "]")
                num_tests += 1

        print "  executed %d cases" % num_tests
        print "  failed %d cases:" % num_failed_tests
        if num_failed_tests > 0:
            for failed in failed_tests:
                print "    " + failed
        self.assertEqual(num_failed_tests, 0, "The above tests failed")


    def __start_tunnel_between(self, gateway, service, use_case=False, num_iterations=1, faults={}, test_tag="", block_until_tunnel_up=False, use_plaid=False):
        options = WeaveTunnelStart.option()
        options["border_gateway"] = gateway
        options["service"] = service
        options["case"] = use_case
        options["case_cert_path"] = "default"
        options["tap"] = self.tap
        options["gateway_faults"] = faults.get("gateway")
        options["service_faults"] = faults.get("service")
        options["iterations"] = num_iterations
        options["test_tag"] = test_tag

        if use_plaid:
            options["plaid_gateway_env"] = self.plaid.getPlaidClientLibEnv(gateway)
            options["plaid_service_env"] = self.plaid.getPlaidClientLibEnv(service)

        if block_until_tunnel_up:
            # wait for tunnel to be established before saying you're done
            options["sync_on_gateway_output"] = "ToState:PrimaryTunnelEstablished"

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __run_ping_test_between(self, nodeA, nodeB, use_case=False, test_tag="", use_plaid=False):
        options = WeavePing.option()
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["udp"] = True
        options["endpoint"] = "Tunnel"
        options["count"] = gOptions["num_pings"]
        options["tap"] = self.tap
        options["case"] = use_case
        options["case_cert_path"] = "default"
        options["test_tag"] = test_tag

        if use_plaid:
            options["plaid_client_env"] = self.plaid.getPlaidClientLibEnv(nodeA)
            options["plaid_server_env"] = self.plaid.getPlaidClientLibEnv(nodeB)

        weave_ping = WeavePing.WeavePing(options)
        ret = weave_ping.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __process_ping_result(self, nodeA, nodeB, value, data):
        num_pings = int(gOptions["num_pings"])

        print "ping from " + nodeA + " to " + nodeB + " ",

        data = data[0]

        if value > num_pings + 1:
            print hred("Failed")
        else:
            print hgreen("Passed")

        try:
            self.assertTrue(value < num_pings + 1, "%s < %s %%" % (str(value), num_pings))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            print "Client Output: "
            for line in data["client_output"].split("\n"):
               print "\t" + line

            print "Server Output: "
            for line in data["server_output"].split("\n"):
                print "\t" + line

            if self.show_strace == True:
                print "Server Strace: "
                for line in data["server_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data["client_strace"].split("\n"):
                    print "\t" + line

        if value > num_pings + 1:
            raise ValueError("Weave Ping over Weave Tunnel Failed")


    def __process_tunnel_results(self, gw, gw_value, gw_data, ping_value, ping_data, test_tag):

        gw_output = gw_data.get("gateway_output")
        service_output = gw_data.get("service_output")

        tunnel_up = self.__is_tunnel_up(gw_output)
        gw_parser_error, gw_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(gw_output)
        service_parser_error, service_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(service_output)

        # at the end of the test, 'tunnel_up' should be True
        if not tunnel_up:
            print hred("  Tunnel start: Failed   [" + test_tag + "]")

        if ping_value != 0:  # loss_percentage == 0
            print hred("  Ping test: Failed [" + test_tag + "]")

        if gw_parser_error is True:
            print hred("  gateway parser error  [" + test_tag + "]")

        if gw_leak_detected is True:
            print hred("  gateway leak detected [" + test_tag + "]+")

        if service_parser_error is True:
            print hred("service parser error")
        if service_leak_detected is True:
            print hred("service resource leak detected")

        result = {}
        result["tunnel_up"] = tunnel_up
        result["ping_successful"] = ping_value == 0   #loss_percentage == 0
        result["no_gw_parser_error"] = not gw_parser_error
        result["no_gw_leak_detected"] = not gw_leak_detected
        result["no_service_parser_error"] = not service_parser_error
        result["no_service_leak_detected"] = not service_leak_detected

        return result


    def __is_tunnel_up(self, gw_output):
        tunnel_up = False
        post_fault_section = False

        if not gw_output:
            return False

        gw_output = gw_output.split('\n')

        for line in gw_output:
            if "SIGUSR1 received: proceed to exit gracefully" in line:
                break

            if "***** Injecting fault" in line:
                post_fault_section = True

            if "ToState:PrimaryTunnelEstablished" in line:
                tunnel_up = True

            if "FromState:PrimaryTunnelEstablished" in line and "ToState:PrimaryTunnelEstablished" not in line:
                tunnel_up = False

            if "WEAVE:WT: Connection ABORTED" in line:
                tunnel_up = False

            if "WEAVE:WT: Tunnel connection not up" in line:
                tunnel_up = False

        return tunnel_up


    def __stop_tunnel_between(self, gateway, service, num_iterations=1, faults={}, test_tag=""):
        options = WeaveTunnelStop.option()
        options["border_gateway"] = gateway
        options["service"] = service
        options["gateway_faults"] = faults.get("gateway")
        options["service_faults"] = faults.get("service")
        options["iterations"] = num_iterations
        options["test_tag"] = test_tag

        weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data

    def __start_plaid_server(self):
        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        print "test_weave_tunnel_faults: %s" % (emsg)

    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()

        emsg = "plaid-server should not be running any longer."
        print "test_weave_tunnel_faults: %s" % (emsg)

if __name__ == "__main__":
    help_str = """usage:
    --help  Print this usage info and exit
    --num_pings N  Number of echo packets sent during ping test (default: 3)
    --disable-case  Turn off CASE (CASE is enabled by default)\n"""

    help_str += gFaultopts.help_string
    longopts = ["help", "num_pings=","disable-case", "no-restart"]
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

        elif o in ("--disable-case"):
            gOptions["case"] = False

        elif o in ("--num_pings"):
            gOptions["num_pings"] = a

        elif o in ("--no-restart"):
            gOptions["restart"] = False

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

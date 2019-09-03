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
#       Calls Weave WDM between mock client and NestService Wdm Services via Tunnel
#

import json
import os
import re
import set_test_path
import sys
import time
import unittest


from happy.Utils import *
from topologies.dynamic.thread_wifi_ap_internet_configurable_topology \
    import thread_wifi_ap_internet_configurable_topology
import plugins.testrail.TestrailResultOutput
import Weave
import WeaveTunnelStart
import WeaveTunnelStop
import WeaveWdmNext
import WeaveWdmNextOptions as wwno


TEST_OPTION_QUIET = True
DEFAULT_FABRIC_SEED = "00001"
TESTRAIL_SECTION_NAME = 'WDM-Next test suite between Mock-Client and Real-Service'
TESTRAIL_SUFFIX = "_TESTRAIL.json"


class weave_wdm_next_test_service_base(unittest.TestCase):

    def setUp(self):
        """
        Set up the toplogy/tunnel and load default test parameters.
        """
        self.quiet = TEST_OPTION_QUIET
        self.options = WeaveWdmNext.option()

        self.topology_setup_required = int(os.environ.get("TOPOLOGY", "1")) == 1

        self.device_numbers = int(os.environ.get("DEVICE_NUMBERS", 1))

        self.success_threshold = float(os.environ.get("SUCCESS_THRESHOLD", 0.9))

        fabric_seed = os.environ.get("FABRIC_SEED", DEFAULT_FABRIC_SEED)

        if "FABRIC_OFFSET" in os.environ.keys():
            self.fabric_id = format(int(fabric_seed, 16) +
                                    int(os.environ["FABRIC_OFFSET"]), 'x').zfill(5)
        else:
            self.fabric_id = fabric_seed

        self.eui64_prefix = os.environ.get("EUI64_PREFIX", '18:B4:30:AA:00:')

        self.customized_eui64_seed = self.eui64_prefix + \
            self.fabric_id[0:2] + ':' + self.fabric_id[2:4] + ':' + self.fabric_id[4:]

        self.use_service_dir = int(os.environ.get("USE_SERVICE_DIR", "0")) == 1

        self.enable_random_fabric = int(os.environ.get("ENABLE_RANDOM_FABRIC", "0")) == 1

        # Set default parameter options
        self.options[wwno.CLIENT][wwno.TOTAL_COUNT] = int(os.environ.get("TOTAL_CLIENT_COUNT", 4))

        self.options[wwno.CLIENT][wwno.TEST_DELAY] = int(
            os.environ.get("TEST_CLIENT_DELAY", 30000))

        self.options[wwno.CLIENT][wwno.TIMER_PERIOD] = int(
            os.environ.get("TIMER_CLIENT_PERIOD", 15000))

        self.options[wwno.TEST][wwno.TIMEOUT] = int(os.environ.get("TEST_TIMEOUT", 60 * 30))

        self.options[wwno.CLIENT][wwno.CASE] = int(os.environ.get("CASE", "0")) == 1

        self.options[wwno.CLIENT][wwno.LIVENESS_CHECK_PERIOD] = int(
            os.environ.get("WDM_CLIENT_LIVENESS_CHECK_PERIOD", 30))

        # TODO: Once LwIP bugs for tunnel are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ[
                "WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.tap = True
            self.options[wwno.CLIENT][wwno.TAP] = "wpan0"
            self.options[wwno.SERVER][wwno.TAP] = "wpan0"
            return
        else:
            self.tap = False

        if self.topology_setup_required:
            print "SETTING UP TOPLOGY INTERNET!!!!"
            self.topology = thread_wifi_ap_internet_configurable_topology(quiet=self.quiet,
                                                                          fabric_id=self.fabric_id,
                                                                          customized_eui64_seed=self.customized_eui64_seed,
                                                                          tap=self.tap,
                                                                          dns=None,
                                                                          device_numbers=self.device_numbers,
                                                                          enable_random_fabric=self.enable_random_fabric)
            self.topology.createTopology()
        else:
            print "topology set up not required"

        self.weave_wdm = None
        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        delayExecution(2)

        # topology has nodes: ThreadNode, BorderRouter, onhub and NestService instance
        # we run tunnel between BorderRouter and NestService

        # Start tunnel
        value, data = self.__start_tunnel_from("BorderRouter")

        try:
            print "sleeping..."
            # time.sleep(60*60)
        except:
            print "sleep interrupted"

    def tearDown(self):
        """
        Tear down the tunnel/toplogy.
        """
        value, data = self.__stop_tunnel_from("BorderRouter")
        if self.topology_setup_required:
            self.topology.destroyTopology()
            self.topology = None

    @staticmethod
    def get_test_param_json(test_name):
        """
        Retrieve the test params for a particular test.

        Args:
            test_name (str): Name of the test.

        Returns:
            dict: Dictionary containing test, client, and server test params.
        """
        curr_file_dir = os.path.dirname(os.path.abspath(__file__))
        test_param_json_path = "{}/weave_wdm_next_test_service_params.json".format(curr_file_dir)
        with open(test_param_json_path) as json_file:
            data = json.load(json_file)
            return data[test_name]

    def weave_wdm_next_test_service_base(self, wdm_next_args):
        """
        Initiate wdm next test between client and service given test parameters.

        Args:
            wdm_next_args (dict): Test parameters.
        """
        success, data = self.__run_wdm_test_between("ThreadNode", "service", wdm_next_args)
        self.__process_result("ThreadNode", "service", success, data)
        self.result_data = data

        delayExecution(1)

    def __start_tunnel_from(self, gateway):
        """
        Start the weave tunnel.

        Args:
            gateway (str): Node to start tunnel from.

        Returns:
            (bool, list): Whether weave tunnel started successfully and logs
        """
        options = WeaveTunnelStart.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["border_gateway"] = gateway
        options["tap"] = self.options[wwno.CLIENT][wwno.TAP]
        options["case"] = self.options[wwno.CLIENT][wwno.CASE]
        options["service_dir"] = self.use_service_dir

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()
        value = ret.Value()
        data = ret.Data()

        return value, data

    def __run_wdm_test_between(self, nodeA, nodeB, wdm_next_args):
        """
        Execute the test between two nodes given test parameters.

        Args:
            nodeA (str): First node identifier.
            nodeB (str): Second node identifier.
            wdm_next_args (dict): Custom test parameters.

        Returns:
            (bool, list): Whether test executed without parser/leak errors and
                          test results/logs
        """
        # Update default options with test parameters
        for category, test_param_args in wdm_next_args.items():
            self.options[category].update(test_param_args)

        self.options[wwno.TEST][wwno.CLIENT] = [nodeA + str(index + 1).zfill(2)
                                                for index in range(self.device_numbers)]
        self.options[wwno.TEST][wwno.SERVER] = nodeB
        self.options[wwno.CLIENT][wwno.ENABLE_DICTIONARY_TEST] = False

        self.weave_wdm = WeaveWdmNext.WeaveWdmNext(self.options)

        ret = self.weave_wdm.run_client_test()
        success = ret.Value()
        data = ret.Data()

        return success, data

    def __process_result(self, nodeA, nodeB, success, all_data):
        """
        Process the weave wdm next test results.

        Args:
            nodeA (str): First node identifier.
            nodeB (str): Second node identifier.
            success (bool): Whether test executed successfully without parser/leak errors.
            all_data (list): Results from weave wdm test including node logs.
        """
        success = True
        client_event_dic = None
        client_stress_event_dic = None
        tc_name = self.options[wwno.TEST][wwno.TEST_CASE_NAME]

        exception_dic = {}
        for data in all_data:
            test_results = []
            wdm_sanity_check = True
            wdm_stress_check = success
            wdm_sanity_diag_list = []
            wdm_stress_diag_list = []
            wdm_sanity_diag_list.append('test_file: {}\n client_weave_id: {}\n parameters: {}\n'
                                        .format(self.options[wwno.TEST][wwno.TEST_TAG],
                                                data['client_weave_id'], self.options))
            wdm_stress_diag_list.append('test_file: {}\n client_weave_id: {}\n parameters: {}\n'
                                        .format(self.options[wwno.TEST][wwno.TEST_TAG],
                                                data['client_weave_id'], self.options))
            wdm_stress_diag_list.append(str(data['success_dic']))
            print "weave-wdm-next {} from {} to {}".format(
                self.options[wwno.TEST][wwno.WDM_OPTION], data['client'], nodeB)
            if len(tc_name) == 2:
                client_delimiter = 'Current completed test iteration is'
                client_completed_iter_list = [
                    i + client_delimiter for i in data["client_output"].split(client_delimiter)]

                for i in self.options[wwno.CLIENT][wwno.LOG_CHECK]:
                    item_output_cnt = len(re.findall(i[0], client_completed_iter_list[0]))
                    if item_output_cnt == 0 and i[1] != 0:
                        wdm_sanity_check = False
                        wdm_sanity_diag_list.append("missing '%s' in client_output" % i[0])
                    elif item_output_cnt != 0 and i[1] == 0:
                        wdm_sanity_check = False
                        wdm_sanity_diag_list.append("unexpected '%s' in client_output" % i[0])

                if len(re.findall("Good Iteration", client_completed_iter_list[0])) != 1:
                    wdm_sanity_check = False
                    wdm_sanity_diag_list.append("missing good iteration")

                if self.options[wwno.CLIENT][wwno.EVENT_GENERATOR]:
                    client_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(
                        client_completed_iter_list[0])
                    if not client_event_dic['success']:
                        wdm_sanity_check = False
                        wdm_sanity_diag_list.append("initiator missing events:")
                    else:
                        wdm_sanity_diag_list.append("initiator good events:")
                    wdm_sanity_diag_list.append(str(client_event_dic))

                client_sanity_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(
                    client_completed_iter_list[0])
                wdm_sanity_diag_list.append("initiator sanity trait checksum:")
                wdm_sanity_diag_list.append(str(client_sanity_checksum_list))

            success_iterations_count = len(re.findall("Good Iteration", data["client_output"]))
            success_rate = round(success_iterations_count /
                                 float(self.options[wwno.CLIENT][wwno.TEST_ITERATIONS]), 3)
            success_iterations_rate_string = "success rate(success iterations/total iterations) "\
                                             "{}/{} = {}".format(
                                                 success_iterations_count,
                                                 self.options[wwno.CLIENT][wwno.TEST_ITERATIONS],
                                                 success_rate)

            wdm_stress_diag_list.append(success_iterations_rate_string)

            for i in self.options[wwno.CLIENT][wwno.LOG_CHECK]:
                item_output_cnt = len(re.findall(i[0], data["client_output"]))
                if item_output_cnt != i[1]:
                    wdm_stress_check = False
                    wdm_stress_diag_list.append(
                        "wrong number of '%s' in client_output, expected %d, actual %d" %
                        (i[0], i[1], item_output_cnt))

            if self.options[wwno.CLIENT][wwno.EVENT_GENERATOR]:
                client_stress_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(data[
                                                                                                "client_output"])
                if not client_stress_event_dic['success']:
                    wdm_stress_check = False
                    wdm_stress_diag_list.append("missing events:")
                else:
                    wdm_stress_diag_list.append("good events:")
                wdm_stress_diag_list.append(str(client_stress_event_dic))

            client_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(
                data["client_output"])
            wdm_stress_diag_list.append("initiator trait checksum:")
            wdm_stress_diag_list.append(str(client_checksum_list))

            if len(tc_name) == 2:
                test_results.append({
                    'testName': tc_name[0],
                    'testStatus': 'success' if wdm_sanity_check else 'failure',
                    'testDescription': ('Sanity test passed!' if wdm_sanity_check else 'Sanity test failed!')
                    + "\n" + ''.join(wdm_sanity_diag_list),
                    'success_iterations_count': (1 if wdm_sanity_check else 0),
                    'initiator_event_list': str(client_event_dic['client_event_list']) if client_event_dic else [],
                    'initiator_trait_checksum_list': str(client_sanity_checksum_list) if client_sanity_checksum_list else [],
                    'total_iterations_count': 1
                })
            test_results.append({
                'testName': tc_name[1] if len(tc_name) == 2 else tc_name[0],
                'testStatus': 'success' if wdm_stress_check else 'failure',
                'testDescription': ('Stress test passed!\n' if wdm_stress_check
                                    else 'Stress test failed!\n') + ''.join(wdm_stress_diag_list),
                'success_iterations_count': success_iterations_count,
                'initiator_event_list': str(client_stress_event_dic['client_event_list']) if client_stress_event_dic else [],
                'initiator_trait_checksum_list': str(client_checksum_list) if client_checksum_list else [],
                'total_iterations_count': self.options[wwno.CLIENT][wwno.TEST_ITERATIONS]
            })

            # output the test result to a json file
            output_data = {
                'sectionName': TESTRAIL_SECTION_NAME,
                'testResults': test_results
            }

            output_file_name = self.weave_wdm.process_log_prefix + data['client'] + \
                self.options[wwno.TEST][wwno.TEST_TAG][1:].upper() + TESTRAIL_SUFFIX

            self.__output_test_result(output_file_name, output_data)

            if not wdm_sanity_check:
                print "wdm_sanity_check is False\n\n" + "\n\t".join(wdm_sanity_diag_list)
            if not wdm_stress_check:
                print "wdm_stress_check is False\n\n" + "\n\t".join(wdm_stress_diag_list)

            try:
                self.assertTrue(
                    wdm_sanity_check,
                    "wdm_sanity_check %s == True %%" %
                    (str(wdm_sanity_check)))
                self.assertTrue(
                    wdm_stress_check,
                    "wdm_stress_check %s == True %%" %
                    (str(wdm_stress_check)))
            except AssertionError:
                success = False
                exception_dic[data['client']] = test_results
            if success_rate < self.success_threshold:
                print exception_dic
                success = False
        if not success:
            print hred("Failed")
            raise ValueError('test failure')
        else:
            print hgreen("Passed")

    def __output_test_result(self, file_path, output_data):
        """
        Output the test results in Testrail.

        Args:
            file_path (str): Path to testrail logs.
            output_data (dict): Holds the test results.
        """
        options = plugins.testrail.TestrailResultOutput.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["file_path"] = file_path
        options["output_data"] = output_data

        output_test = plugins.testrail.TestrailResultOutput.TestrailResultOutput(options)
        output_test.run()

    def __stop_tunnel_from(self, gateway):
        """
        Stop the weave tunnel.

        Args:
            gateway (str): Node to start tunnel from.

        Returns:
            (bool, list): Whether weave tunnel started successfully and logs
        """
        options = WeaveTunnelStop.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["border_gateway"] = gateway
        options["service_dir"] = self.use_service_dir

        weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = weave_tunnel.run()
        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    unittest.main()

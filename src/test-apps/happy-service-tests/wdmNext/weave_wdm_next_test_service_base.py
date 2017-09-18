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

import os
import re
import sys
import unittest
import set_test_path

from happy.Utils import *
import plugin.WeaveTunnelStart as WeaveTunnelStart
import plugin.WeaveTunnelStop as WeaveTunnelStop
import plugin.WeaveWdmNext as WeaveWdmNext
import plugins.weave.Weave as Weave
import plugins.testrail.TestrailResultOutput


sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from topologies.thread_wifi_ap_internet_configurable_topology import thread_wifi_ap_internet_configurable_topology

TEST_OPTION_QUIET = True
DEFAULT_FABRIC_SEED = "00000"
TESTRAIL_SECTION_NAME = 'WDM-Next test suite between Mock-Client and Real-Service'
TESTRAIL_SUFFIX = "_TESTRAIL.json"


class weave_wdm_next_test_service_base(unittest.TestCase):
    def setUp(self):
        self.tap = None
        self.tap_id = None
        self.quiet = TEST_OPTION_QUIET
        self.options = None

        self.topology_setup_required = int(os.environ.get("TOPOLOGY", "1")) == 1

        self.device_numbers = int(os.environ.get("DEVICE_NUMBERS", 1))

        self.success_threshold = float(os.environ.get("SUCCESS_THRESHOLD", 0.9))

        self.wdm_client_liveness_check_period = int(os.environ.get("WDM_CLIENT_LIVENESS_CHECK_PERIOD", 30))

        fabric_seed = os.environ.get("FABRIC_SEED", DEFAULT_FABRIC_SEED)

        if "FABRIC_OFFSET" in os.environ.keys():
            self.fabric_id = format(int(fabric_seed, 16) + int(os.environ["FABRIC_OFFSET"]), 'x').zfill(5)
        else:
            self.fabric_id = fabric_seed

        self.eui64_prefix = os.environ.get("EUI64_PREFIX", '18:B4:30:AA:00:')

        self.customized_eui64_seed = self.eui64_prefix + self.fabric_id[0:2] + ':' + self.fabric_id[2:4] + ':' + self.fabric_id[4:]

        self.total_client_count = int(os.environ.get("TOTAL_CLIENT_COUNT", 4))

        self.test_client_delay = int(os.environ.get("TEST_CLIENT_DELAY", 30000))

        self.timer_client_period = int(os.environ.get("TIMER_CLIENT_PERIOD", 15000))

        self.test_timeout = int(os.environ.get("TEST_TIMEOUT", 60 * 30))

        self.case = int(os.environ.get("CASE", "0")) == 1

        self.use_service_dir = int(os.environ.get("USE_SERVICE_DIR", "0")) == 1

        self.enable_random_fabric = int(os.environ.get("ENABLE_RANDOM_FABRIC", "0")) == 1

        # TODO: Once LwIP bugs for tunnel are fix, enable this test on LwIP
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.tap = True
            self.tap_id = "wpan0"
            return
        else:
            self.tap = False

        if self.topology_setup_required:
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

        self.show_strace = False

        self.weave_wdm = None
        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        delayExecution(2)

        # topology has nodes: ThreadNode, BorderRouter, onhub and NestService instance
        # we run tunnel between BorderRouter and NestService

        # Start tunnel
        value, data = self.__start_tunnel_from("BorderRouter")


    def tearDown(self):

        # Stop tunnel
        value, data = self.__stop_tunnel_from("BorderRouter")
        if self.topology_setup_required:
            self.topology.destroyTopology()
            self.topology = None


    def weave_wdm_next_test_service_base(self, wdm_next_args):
        default_options = {'test_client_case' : 1,
                           'save_client_perf' : False,
                           'total_client_count': self.total_client_count,
                           'timer_client_period': self.timer_client_period,
                           'enable_client_stop' : True,
                           'timeout' : self.test_timeout,
                           'test_client_delay' : self.test_client_delay,
                           'client_event_generator' : None,
                           'client_inter_event_period' : None,
                           'client_faults' : None,
                           'server_faults' : None,
                           'test_case_name' : [],
                           'enable_client_dictionary_test' : False,
                           'wdm_client_liveness_check_period': self.wdm_client_liveness_check_period,
                           'enable_mock_event_timestamp_initial_counter': True
                           }

        default_options.update(wdm_next_args)
        self.__dict__.update(default_options)
        self.default_options = default_options

        value, data = self.__run_wdm_test_between("ThreadNode", "service")
        self.__process_result("ThreadNode", "service", value, data)

        delayExecution(1)


    def __start_tunnel_from(self, gateway):
        options = WeaveTunnelStart.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["border_gateway"] = gateway
        options["tap"] = self.tap_id

        options["case"] = self.case

        options["service_dir"] = self.use_service_dir

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __run_wdm_test_between(self, nodeA, nodeB):
        options = WeaveWdmNext.option()

        options.update(self.default_options)
        options["clients"] = [nodeA + str(index + 1).zfill(2) for index in range(self.device_numbers)]
        options["server"] = nodeB
        options["tap"] = self.tap
        options["test_tag"] = options["test_tag"][19:].upper()
        options["device_numbers"] = self.device_numbers
        options["quiet"] = TEST_OPTION_QUIET
        options["case"] = self.case
        self.options = options
        self.weave_wdm = WeaveWdmNext.WeaveWdmNext(options)

        ret = self.weave_wdm.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


    def __process_result(self, nodeA, nodeB, value, all_data):
        success = True
        client_event_dic = None
        client_stress_event_dic = None

        exception_dic = {}
        for data in all_data:
            test_results = []
            wdm_sanity_check = True
            wdm_stress_check = value
            wdm_sanity_diag_list = []
            wdm_stress_diag_list = []
            wdm_sanity_diag_list.append('test_file: %s\n client_weave_id: %s\n parameters: %s\n'
                                        % (self.test_tag, data['client_weave_id'], str(self.options)))
            wdm_stress_diag_list.append('test_file: %s\n client_weave_id: %s\n parameters: %s\n'
                                        % (self.test_tag, data['client_weave_id'], str(self.options)))
            wdm_stress_diag_list.append(str(data['success_dic']))
            print "weave-wdm-next %s from " % self.wdm_option + data['client'] + " to " + nodeB + " "
            if len(self.test_case_name) == 2:
                client_delimiter = 'Current completed test iteration is'
                client_completed_iter_list = [i + client_delimiter for i in data["client_output"].split(client_delimiter)]

                for i in self.client_log_check:
                    item_output_cnt = len(re.findall(i[0], client_completed_iter_list[0]))
                    if item_output_cnt == 0:
                        wdm_sanity_check = False
                        wdm_sanity_diag_list.append("missing '%s' in client_output" % i[0])

                if len(re.findall("Good Iteration", client_completed_iter_list[0])) != 1:
                    wdm_sanity_check = False
                    wdm_sanity_diag_list.append("missing good iteration")

                if self.client_event_generator is not None:
                    client_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(client_completed_iter_list[0])
                    if not ((client_event_dic['success'] is True)):
                        wdm_sanity_check = False
                        wdm_sanity_diag_list.append("initiator missing events:")
                    else:
                        wdm_sanity_diag_list.append("initiator good events:")
                    wdm_sanity_diag_list.append(str(client_event_dic))

                client_sanity_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(client_completed_iter_list[0])
                wdm_sanity_diag_list.append("initiator sanity trait checksum:")
                wdm_sanity_diag_list.append(str(client_sanity_checksum_list))

            success_iterations_count = len(re.findall("Good Iteration", data["client_output"]))
            success_rate = success_iterations_count/float(self.test_client_iterations)
            success_iterations_rate_string = "success rate(success iterations/total iterations) %d/%d = %0.3f" \
                                             % (success_iterations_count, self.test_client_iterations, success_rate)
            wdm_stress_diag_list.append(success_iterations_rate_string)

            for i in self.client_log_check:
                item_output_cnt = len(re.findall(i[0], data["client_output"]))
                if item_output_cnt != i[1]:
                    wdm_stress_check = False
                    wdm_stress_diag_list.append("missing '%s' in client_output, need %d, actual %d" % (i[0], i[1], item_output_cnt))

            if self.client_event_generator is not None:
                client_stress_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(data["client_output"])
                if not ((client_stress_event_dic['success']) is True):
                    wdm_stress_check = False
                    wdm_stress_diag_list.append("missing events:")
                else:
                    wdm_stress_diag_list.append("good events:")
                wdm_stress_diag_list.append(str(client_stress_event_dic))

            client_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(data["client_output"])
            wdm_stress_diag_list.append("initiator trait checksum:")
            wdm_stress_diag_list.append(str(client_checksum_list))

            if len(self.test_case_name) == 2:
                test_results.append({
                    'testName': self.test_case_name[0],
                    'testStatus': 'success' if wdm_sanity_check else 'failure',
                    'testDescription': ('Sanity test passed!' if wdm_sanity_check else 'Sanity test failed!')
                                       + "\n" + ''.join(wdm_sanity_diag_list),
                    'success_iterations_count': (1 if wdm_sanity_check else 0),
                    'initiator_event_list': str(client_event_dic['client_event_list']) if client_event_dic else [],
                    'initiator_trait_checksum_list': str(client_sanity_checksum_list) if client_sanity_checksum_list else [],
                    'total_iterations_count': 1
                })
            test_results.append({
                'testName': self.test_case_name[1] if len(self.test_case_name) == 2 else self.test_case_name[0],
                'testStatus': 'success' if wdm_stress_check else 'failure',
                'testDescription': ('Stress test passed!' if wdm_stress_check else 'Stress test failed!')
                                   + "\n" + ''.join(wdm_stress_diag_list),
                'success_iterations_count': success_iterations_count,
                'initiator_event_list': str(client_stress_event_dic['client_event_list']) if client_stress_event_dic else [],
                'initiator_trait_checksum_list': str(client_checksum_list) if client_checksum_list else [],
                'total_iterations_count': self.test_client_iterations
            })

            # output the test result to a json file
            output_data = {
                'sectionName': TESTRAIL_SECTION_NAME,
                'testResults': test_results
            }

            output_file_name = self.weave_wdm.process_log_prefix + data['client'] + self.test_tag[20:].upper() \
                                    + TESTRAIL_SUFFIX

            self.__output_test_result(output_file_name, output_data)

            try:
                self.assertTrue(wdm_sanity_check, "wdm_sanity_check %s == True %%" % (str(wdm_sanity_check)))
                self.assertTrue(wdm_stress_check, "wdm_stress_check %s == True %%" % (str(wdm_stress_check)))
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
        options = plugins.testrail.TestrailResultOutput.option()
        options["quiet"] = TEST_OPTION_QUIET
        options["file_path"] = file_path
        options["output_data"] = output_data

        output_test = plugins.testrail.TestrailResultOutput.TestrailResultOutput(options)
        output_test.run()


    def __stop_tunnel_from(self, gateway):
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



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
#       Calls Weave WDM base test class between nodes.
#

from __future__ import absolute_import
from __future__ import print_function
import itertools
import os
import re
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveWdmNext
import plugins.testrail.TestrailResultOutput

test_option_quiet = True
test_option_all_nodes = False

TESTRAIL_SECTION_NAME = 'WDM-Next test suite between Mock-Client and Mock-Service'
TESTRAIL_SUFFIX = "_TESTRAIL.json"

class weave_wdm_next_test_base(unittest.TestCase):
    def setUp(self):
        self.tap = None
        self.wdm_option = None
        self.options = None

        if "WDM_CLIENT_LIVENESS_CHECK_PERIOD" in list(os.environ.keys()):
            self.wdm_client_liveness_check_period = int(os.environ["WDM_CLIENT_LIVENESS_CHECK_PERIOD"])
        else:
            self.wdm_client_liveness_check_period = None

        if "WDM_SERVER_LIVENESS_CHECK_PERIOD" in list(os.environ.keys()):
            self.wdm_server_liveness_check_period = int(os.environ["WDM_SERVER_LIVENESS_CHECK_PERIOD"])
        else:
            self.wdm_server_liveness_check_period = None

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


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def weave_wdm_next_test_base(self, wdm_next_args):
        options = happy.HappyNodeList.option()
        default_options = {'test_client_case' : 1,
                           'test_server_case' : 1,
                           'save_client_perf' : False,
                           'save_server_perf' : False,
                           'test_server_delay' : 0,
                           'timeout' : 60*15,
                           'client_event_generator' : None,
                           'client_inter_event_period' : None,
                           'client_faults' : None,
                           'server_event_generator' : None,
                           'server_inter_event_period' : None,
                           'server_faults' : None,
                           'test_case_name' : [],
                           'strace': False,
                           'enable_client_dictionary_test' : True,
                           'wdm_client_liveness_check_period': self.wdm_client_liveness_check_period,
                           'wdm_server_liveness_check_period': self.wdm_server_liveness_check_period,
                           'enable_retry': False,
                           'enable_mock_event_timestamp_initial_counter': True
                           }

        default_options.update(wdm_next_args)

        self.__dict__.update(default_options)
        self.default_options = default_options

        self.test_results = []

        if test_option_all_nodes is True:
            nodes_list = happy.HappyNodeList.HappyNodeList(options)
            ret = nodes_list.run()

            node_ids = ret.Data()
            pairs_of_nodes = list(itertools.product(node_ids, node_ids))

            for pair in pairs_of_nodes:
                if pair[0] == pair[1]:
                    print("Skip weave-wdm test on client and server running on the same node.")
                    continue
                value, data = self.__run_wdm_test_between(pair[0], pair[1])
                self.__process_result(pair[0], pair[1], value, data)
        else:
            value, data = self.__run_wdm_test_between("node01", "node02")
            self.__process_result("node01", "node02", value, data)
            self.result_data = data

    def __validate_log_line(self, data, node_type, wdm_stress_check, stress_failures):
        """
        check expected log lines showed up in node logs with expected count
        """
        if node_type == "client":
            node_log = data["client_output"]
            log_check_list = self.client_log_check
        else:
            node_log = data["server_output"]
            log_check_list = self.server_log_check
        for expect_line_count in log_check_list:
            item_output_cnt = len(re.findall(expect_line_count[0], node_log))
            if item_output_cnt != expect_line_count[1]:
                wdm_stress_check = False
                error = "missing '%s' in %s_output, need %d, actual %d" % (expect_line_count[0], node_type, expect_line_count[1], item_output_cnt)
                stress_failures.append(error)
        return (wdm_stress_check, stress_failures)

    def __validate_node_events(self, wdm_stress_check, stress_failures, data, client_gen):
        """
        validate events from client and publisher, event count should be equal from client and publisher
        client: send events
        publisher: publish events
        """
        if client_gen: # client send events, server publish events
            client_output = data["client_output"]
            publisher_output = data["server_output"]
            node_role = {"client":"client",
                         "publisher":"server"
                        }
        else: # server send events, client publish events
            client_output = data["server_output"]
            publisher_output = data["client_output"]
            node_role = {"client":"server",
                         "publisher":"client"
                        }
        client_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(client_output)
        publisher_event_dic = self.weave_wdm.wdm_next_publisher_event_sequence_process(publisher_output)
        if not ((client_event_dic['success']) is True and (publisher_event_dic['success'] is True) and (client_event_dic['client_event_list'] == publisher_event_dic['publisher_event_list'])):
            wdm_stress_check = False
            error = "The events sent from the client to the server don't match"
            stress_failures.append(error)
            stress_failures.append("\tevents sent by the " + node_role["client"] + ": " + str(client_event_dic))
            stress_failures.append("\tevents received by the "+ node_role["publisher"]+": " + str(publisher_event_dic))
        return (wdm_stress_check, stress_failures)

    def __process_result(self, nodeA, nodeB, value, data):
        print("weave-wdm-next %s from " % self.wdm_option + nodeA + " to " + nodeB + " ")
        data = data[0]
        test_results = []
        wdm_stress_check = value
        wdm_stress_diag_list = []
        tag_and_paramenters = 'test_file: %s\nparameters: %s\n' % (self.test_tag, str(self.options))

        # collect all problems in a list of strings to pass to the assert at the end of the test
        wdm_stress_diag_list.append(tag_and_paramenters)
        wdm_stress_diag_list.append(str(data['success_dic']))

        stress_failures = ["There is a failure in the overall test; log file tag: " + self.test_tag]
        if not value:
            # This means something failed and it was caught in the WeaveWdmNext plugin
            stress_failures.append(str(data['success_dic']))

        success_iterations_count = len(re.findall("Good Iteration", data["client_output"]))
        success_iterations_rate_string = "success rate(success iterations/total iterations)" \
                                         + str(success_iterations_count) \
                                         + "/" + str(self.test_client_iterations) \
                                         + " = " + str(success_iterations_count/float(self.test_client_iterations))
        wdm_stress_diag_list.append(success_iterations_rate_string)

        if success_iterations_count != self.test_client_iterations and not (self.client_faults or self.server_faults):
            stress_failures.append(success_iterations_rate_string)

        # check both client and server log for expected log filters with expected counts
        wdm_stress_check, stress_failures = self.__validate_log_line(data, "client", wdm_stress_check, stress_failures)
        wdm_stress_check, stress_failures = self.__validate_log_line(data, "server", wdm_stress_check, stress_failures)

        if not (self.client_faults or self.server_faults):
            if self.client_event_generator is not None:
                wdm_stress_check, stress_failures = self.__validate_node_events(wdm_stress_check, stress_failures, data, True)

            if self.server_event_generator is not None:
                wdm_stress_check, stress_failures = self.__validate_node_events(wdm_stress_check, stress_failures, data, False)

            if self.server_event_generator is None and self.client_event_generator is None:
                client_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(data["client_output"])

                server_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(data["server_output"])

                if client_checksum_list != server_checksum_list:
                    wdm_stress_check = False
                    wdm_stress_diag_list.append("mismatched trait checksum:")
                    error = "Mismatch in the trait data checksums:"
                    stress_failures.append(error)
                    stress_failures.append("\tchecksums on the client: " + str(client_checksum_list))
                    stress_failures.append("\tchecksums on the server: " + str(server_checksum_list))
                wdm_stress_diag_list.append('initiator:' + str(client_checksum_list))
                wdm_stress_diag_list.append('responder:' + str(server_checksum_list))

        test_results.append({
            'testName': self.test_case_name[0],
            'testStatus': 'success' if wdm_stress_check else 'failure',
            'testDescription': ('Stress test passed!' if wdm_stress_check else 'Stress test failed!')
                               + "\n" + ''.join(wdm_stress_diag_list)
        })

        # output the test result to a json file
        output_data = {
            'sectionName': TESTRAIL_SECTION_NAME,
            'testResults': test_results
        }

        output_file_name = self.weave_wdm.process_log_prefix + self.test_tag[1:] + TESTRAIL_SUFFIX
        self.__output_test_result(output_file_name, output_data)

        self.assertTrue(wdm_stress_check, "\nwdm_stress_check is False\n\n" + "\n\t".join(stress_failures))
        print(hgreen("Passed"))

    def __output_test_result(self, file_path, output_data):
        options = plugins.testrail.TestrailResultOutput.option()
        options["quiet"] = test_option_quiet
        options["file_path"] = file_path
        options["output_data"] = output_data

        output_test = plugins.testrail.TestrailResultOutput.TestrailResultOutput(options)
        output_test.run()


    def __run_wdm_test_between(self, nodeA, nodeB):
        options = WeaveWdmNext.option()
        options.update(self.default_options)

        options["quiet"] = test_option_quiet
        options["clients"] = [nodeA]
        options["server"] = nodeB
        options["tap"] = self.tap
        options["wdm_option"] = self.wdm_option

        self.options = options

        self.weave_wdm = WeaveWdmNext.WeaveWdmNext(options)

        ret = self.weave_wdm.run()

        value = ret.Value()
        data = ret.Data()

        return value, data

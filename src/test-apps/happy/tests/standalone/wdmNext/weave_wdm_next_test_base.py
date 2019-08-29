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
#       Calls Weave WDM base test class between nodes.
#

import itertools
import json
import os
import re
import set_test_path
import unittest

from happy.Utils import *
import happy.HappyNodeList
import plugins.testrail.TestrailResultOutput
import WeaveStateLoad
import WeaveStateUnload
import WeaveWdmNext
import WeaveWdmNextOptions as wwno

TEST_OPTION_ALL_NODES = False
TESTRAIL_SECTION_NAME = 'WDM-Next test suite between Mock-Client and Mock-Service'
TESTRAIL_SUFFIX = "_TESTRAIL.json"


class weave_wdm_next_test_base(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """
        Set up the weave toplogy.
        """
        cls.tap = None
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ[
                "WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            cls.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/three_nodes_on_tap_thread_weave.json"
            cls.tap = "wpan0"
        else:
            cls.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/three_nodes_on_thread_weave.json"

        # setting Mesh for thread test
        options = WeaveStateLoad.option()
        options["quiet"] = True
        options["json_file"] = cls.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        setup_network.run()

    def setUp(self):
        """
        Load the default test parameters.
        """
        self.wdm_option = None
        self.options = WeaveWdmNext.option()

        if "WDM_CLIENT_LIVENESS_CHECK_PERIOD" in os.environ.keys():
            self.options[wwno.CLIENT][wwno.LIVENESS_CHECK_PERIOD] = int(
                os.environ["WDM_CLIENT_LIVENESS_CHECK_PERIOD"])
        else:
            self.options[wwno.CLIENT][wwno.LIVENESS_CHECK_PERIOD] = None

        if "WDM_SERVER_LIVENESS_CHECK_PERIOD" in os.environ.keys():
            self.options[wwno.SERVER][wwno.LIVENESS_CHECK_PERIOD] = int(
                os.environ["WDM_SERVER_LIVENESS_CHECK_PERIOD"])
        else:
            self.options[wwno.SERVER][wwno.LIVENESS_CHECK_PERIOD] = None

        self.show_strace = False
        self.options[wwno.CLIENT][wwno.TAP] = self.tap
        self.options[wwno.SERVER][wwno.TAP] = self.tap

    @classmethod
    def tearDownClass(cls):
        """
        Tear down the toplogy.
        """
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = cls.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()

    @staticmethod
    def generate_test(wdm_next_args):
        """
        Generate individual test cases for a group of tests.

        Args:
            wdm_next_args (dict): Test parameters.

        Returns:
            func: Function that runs a test given wdm arguments.
        """

        def test(self):
            self.weave_wdm_next_test_base(wdm_next_args)
        return test

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
        test_param_json_path = "{}/weave_wdm_next_test_params.json".format(curr_file_dir)
        with open(test_param_json_path) as json_file:
            data = json.load(json_file)
            return data[test_name]

    def weave_wdm_next_test_base(self, wdm_next_args):
        """
        Initiate wdm next test between nodes given test parameters.

        Args:
            wdm_next_args (dict): Test parameters.
        """
        options = happy.HappyNodeList.option()
        self.test_results = []

        if TEST_OPTION_ALL_NODES:
            nodes_list = happy.HappyNodeList.HappyNodeList(options)
            ret = nodes_list.run()

            node_ids = ret.Data()
            pairs_of_nodes = list(itertools.product(node_ids, node_ids))

            for pair in pairs_of_nodes:
                if pair[0] == pair[1]:
                    print "Skip weave-wdm test on client and server running on the same node."
                    continue
                success, data = self.__run_wdm_test_between(pair[0], pair[1], wdm_next_args)
                self.__process_result(pair[0], pair[1], success, data)
        else:
            success, data = self.__run_wdm_test_between("node01", "node02", wdm_next_args)
            self.__process_result("node01", "node02", success, data)
            self.result_data = data

    def __validate_log_line(self, data, node_type, wdm_stress_check, stress_failures):
        """
        Validate log lines for client/server nodes based on expected count.

        Args:
            data (dict): Holds client/server log lines.
            node_type (str): Denote client/server.
            wdm_stress_check (bool): Whether stress test is passing (so far).
            stress_failures (list): List of failure messages.

        Returns:
            (bool, list): Whether stress test passed so far and list of failure messages.
        """
        if node_type == "client":
            node_log = data["client_output"]
            log_check_list = self.options[wwno.CLIENT][wwno.LOG_CHECK]
        else:
            node_log = data["server_output"]
            log_check_list = self.options[wwno.SERVER][wwno.LOG_CHECK]
        for expect_line_count in log_check_list:
            item_output_cnt = len(re.findall(expect_line_count[0], node_log))
            if item_output_cnt != expect_line_count[1]:
                wdm_stress_check = False
                error = "missing '%s' in %s_output, need %d, actual %d" % (
                    expect_line_count[0], node_type, expect_line_count[1], item_output_cnt)
                stress_failures.append(error)
        return (wdm_stress_check, stress_failures)

    def __validate_node_events(self, wdm_stress_check, stress_failures, data, client_gen):
        """
        Validate the event counts for client/publisher.

        Args:
            wdm_stress_check (bool): Whether stress test is passing (so far).
            stress_failures (list): List of failure messages.
            data (dict): Dict holding client/server events.
            client_gen (bool): Whether client or server sends the events.

        Returns:
            (bool, list): Whether stress test passed so far and list of failure messages.
        """
        if client_gen:  # client send events, server publish events
            client_output = data["client_output"]
            publisher_output = data["server_output"]
            node_role = {
                "client": "client",
                "publisher": "server"
            }
        else:  # server send events, client publish events
            client_output = data["server_output"]
            publisher_output = data["client_output"]
            node_role = {
                "client": "server",
                "publisher": "client"
            }
        client_event_dic = self.weave_wdm.wdm_next_client_event_sequence_process(client_output)
        publisher_event_dic = self.weave_wdm.wdm_next_publisher_event_sequence_process(
            publisher_output)
        if not (client_event_dic['success'] and publisher_event_dic['success'] and (
                client_event_dic['client_event_list'] == publisher_event_dic['publisher_event_list'])):
            wdm_stress_check = False
            error = "The events sent from the client to the server don't match"
            stress_failures.append(error)
            stress_failures.append(
                "\tevents sent by the " +
                node_role["client"] +
                ": " +
                str(client_event_dic))
            stress_failures.append(
                "\tevents received by the " +
                node_role["publisher"] +
                ": " +
                str(publisher_event_dic))
        return (wdm_stress_check, stress_failures)

    def __process_result(self, nodeA, nodeB, success, data):
        """
        Process the weave wdm next test results.

        Args:
            nodeA (str): First node identifier.
            nodeB (str): Second node identifier.
            success (bool): Whether test executed successfully without parser/leak errors.
            data (list): Results from weave wdm test including node logs.
        """
        print "weave-wdm-next {}\n {} from {} to {}".format(
            self.options[wwno.TEST][wwno.TEST_CASE_NAME][0],
            self.options[wwno.TEST][wwno.WDM_OPTION], nodeA, nodeB)
        data = data[0]
        test_results = []
        wdm_stress_check = success
        wdm_stress_diag_list = []
        tag_and_paramenters = 'test_file: {}\nparameters: {}\n'.format(
            self.options[wwno.TEST][wwno.TEST_TAG], self.options)

        # collect all problems in a list of strings to pass to the assert at the end of the test
        wdm_stress_diag_list.append(tag_and_paramenters)
        wdm_stress_diag_list.append(str(data['success_dic']))

        stress_failures = [
            "There is a failure in the overall test; log file tag: {}".format(
                self.options[wwno.TEST][wwno.TEST_TAG])]
        if not success:
            # This means something failed and it was caught in the WeaveWdmNext plugin
            stress_failures.append(str(data['success_dic']))

        success_iterations_count = len(re.findall("Good Iteration", data["client_output"]))
        success_rate = round(success_iterations_count /
                             float(self.options[wwno.CLIENT][wwno.TEST_ITERATIONS]), 2)
        success_iterations_rate_string = "success rate(success iterations/total iterations) " \
                                         "{}/{} = {}".format(
                                             success_iterations_count,
                                             self.options[wwno.CLIENT][wwno.TEST_ITERATIONS],
                                             success_rate)

        wdm_stress_diag_list.append(success_iterations_rate_string)

        if success_iterations_count != self.options[wwno.CLIENT][wwno.TEST_ITERATIONS] and not \
                (self.options[wwno.CLIENT][wwno.FAULTS] or self.options[wwno.SERVER][wwno.FAULTS]):
            # There are failures but no faults configured
            stress_failures.append(success_iterations_rate_string)

        # check both client and server log for expected log filters with expected counts
        wdm_stress_check, stress_failures = self.__validate_log_line(
            data, "client", wdm_stress_check, stress_failures)
        wdm_stress_check, stress_failures = self.__validate_log_line(
            data, "server", wdm_stress_check, stress_failures)

        if not (self.options[wwno.CLIENT][wwno.FAULTS] or self.options[wwno.SERVER][wwno.FAULTS]):
            if self.options[wwno.CLIENT][wwno.EVENT_GENERATOR]:
                wdm_stress_check, stress_failures = self.__validate_node_events(
                    wdm_stress_check, stress_failures, data, True)

            if self.options[wwno.SERVER][wwno.EVENT_GENERATOR]:
                wdm_stress_check, stress_failures = self.__validate_node_events(
                    wdm_stress_check, stress_failures, data, False)

            if not self.options[wwno.CLIENT][wwno.EVENT_GENERATOR] and not self.options[
                    wwno.SERVER][wwno.EVENT_GENERATOR]:
                client_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(data[
                                                                                      "client_output"])

                server_checksum_list = self.weave_wdm.wdm_next_checksum_check_process(data[
                                                                                      "server_output"])

                if client_checksum_list != server_checksum_list:
                    wdm_stress_check = False
                    wdm_stress_diag_list.append("mismatched trait checksum:")
                    error = "Mismatch in the trait data checksums:"
                    stress_failures.append(error)
                    stress_failures.append(
                        "\tchecksums on the client: " +
                        str(client_checksum_list))
                    stress_failures.append(
                        "\tchecksums on the server: " +
                        str(server_checksum_list))
                wdm_stress_diag_list.append('initiator:' + str(client_checksum_list))
                wdm_stress_diag_list.append('responder:' + str(server_checksum_list))

        test_results.append({
            'testName': self.options[wwno.TEST][wwno.TEST_CASE_NAME][0],
            'testStatus': 'success' if wdm_stress_check else 'failure',
            'testDescription': ('Stress test passed!' if wdm_stress_check else 'Stress test failed!')
            + "\n" + ''.join(wdm_stress_diag_list)
        })

        # output the test result to a json file
        output_data = {
            'sectionName': TESTRAIL_SECTION_NAME,
            'testResults': test_results
        }

        output_file_name = self.weave_wdm.process_log_prefix + \
            self.options[wwno.TEST][wwno.TEST_TAG][1:] + TESTRAIL_SUFFIX
        self.__output_test_result(output_file_name, output_data)

        self.assertTrue(
            wdm_stress_check,
            "\nwdm_stress_check is False\n\n" +
            "\n\t".join(stress_failures))
        print hgreen("Passed")

    def __output_test_result(self, file_path, output_data):
        """
        Output the test results in Testrail.

        Args:
            file_path (str): Path to testrail logs.
            output_data (dict): Holds the test results.
        """
        options = plugins.testrail.TestrailResultOutput.option()
        options["quiet"] = self.options[wwno.TEST][wwno.QUIET]
        options["file_path"] = file_path
        options["output_data"] = output_data

        output_test = plugins.testrail.TestrailResultOutput.TestrailResultOutput(options)
        output_test.run()

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
        # Update default options with test parameters=
        for category, test_param_args in wdm_next_args.items():
            self.options[category].update(test_param_args)

        self.options[wwno.TEST][wwno.CLIENT] = [nodeA]
        self.options[wwno.TEST][wwno.SERVER] = nodeB

        self.weave_wdm = WeaveWdmNext.WeaveWdmNext(self.options)

        ret = self.weave_wdm.run_client_test()
        success = ret.Value()
        data = ret.Data()

        return success, data

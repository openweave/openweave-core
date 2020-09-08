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
#       Sends Weave WDM subscriptionless notification between nodes.
#

from __future__ import absolute_import
from __future__ import print_function
import unittest
import set_test_path
import os
import getopt
import sys
import traceback
import happy.HappyStateDelete
from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


gFaultOpts = WeaveUtilities.FaultInjectionOptions()
gOptions = { 'enableFaults': False, 'mode': "local" }

class test_weave_wdm_next_subless_notify_01(weave_wdm_next_test_base):

    def test_weave_wdm_next_subless_notify_01(self):
        wdm_next_args = {}

        wdm_next_args['wdm_option'] = "subless_notify"

        wdm_next_args['total_client_count'] = 0
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 0
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_server_iterations'] = 1

        wdm_next_args['total_server_count'] = 0
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 0
        wdm_next_args['enable_server_flip'] = 0

        wdm_next_args['total_server_count'] = 0
        wdm_next_args['test_server_delay'] = 1000
        wdm_next_args['client_log_check'] = []
        wdm_next_args['server_log_check'] = []

        wdm_next_args['timeout'] = 10

        wdm_next_args['test_tag'] = self.__class__.__name__[19:].upper()
        wdm_next_args['test_case_name'] = ['O01: Subscriptionless Notify: Publisher sends subscriptionless notify to receiver, and receiver notifies application after processing trait data'
        ]
        print('test file: ' + self.__class__.__name__)
        print("weave-wdm-next test O01")
        super(test_weave_wdm_next_subless_notify_01, self).weave_wdm_next_test_base(wdm_next_args)

        if not gOptions['enableFaults']:
            return

        base_test_tag = "_SUBLESS_NOTIFY_FAULTS"
        wdm_next_args['test_tag'] = base_test_tag
        wdm_next_args['test_case_name'] = ['WDM Subscriptionless Notification for fault injection']

        wdm_next_args['client_faults'] = None
        wdm_next_args['server_faults'] = None

        output_logs = {}
        output_logs['client'] = self.result_data[0]['client_output']
        output_logs['server'] = self.result_data[0]['server_output']


        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_server_iterations'] = 1

        # empty the arrays of strings to look for in the logs; rely on the default check for "Good Iteration"
        wdm_next_args['client_log_check'] = []
        wdm_next_args['server_log_check'] = []
        num_tests = 0

        for node in gFaultOpts.nodes:
            fault_configs = gFaultOpts.generate_fault_config_list(node, output_logs[node])

            for fault_config in fault_configs:
                wdm_next_args['test_tag'] = base_test_tag + "_" + str(num_tests) + "_" + node + "_" + fault_config
                print(wdm_next_args['test_tag'])
                if node == 'client':
                    wdm_next_args['client_faults'] = fault_config
                    wdm_next_args['server_faults'] = None
                else:
                    wdm_next_args['client_faults'] = None
                    wdm_next_args['server_faults'] = fault_config

                super(test_weave_wdm_next_subless_notify_01, self).weave_wdm_next_test_base(wdm_next_args)
                num_tests += 1

if __name__ == "__main__":
    help_str = """usage:
    --help           Print this usage info and exit
    --enable-faults  Run fault injection tests\n"""

    help_str += gFaultOpts.help_string

    longopts = ["help", "enable-faults"]
    longopts.extend(gFaultOpts.getopt_config)

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:", longopts)

    except getopt.GetoptError as err:
        print(help_str)
        print(hred(str(err)))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    opts = gFaultOpts.process_opts(opts)

    for o in opts:
        if o in ("-h", "--help"):
            print(help_str)
            sys.exit(0)

        elif o in ("-f", "--enable-faults"):
            gOptions["enableFaults"] = True

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

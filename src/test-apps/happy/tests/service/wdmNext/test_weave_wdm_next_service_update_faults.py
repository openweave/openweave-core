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
#       Calls Weave WDM mutual subscribe with updates between mock device and real service.
#       Runs the test multiple times with different fault-injection configurations.
#
# This is WIP

import getopt
import sys
import unittest
from happy.Utils import *
from weave_wdm_next_test_service_base import weave_wdm_next_test_service_base

import WeaveUtilities

gFaultopts = WeaveUtilities.FaultInjectionOptions()

class test_weave_wdm_next_service_update_faults(weave_wdm_next_test_service_base):

    def setup_log_checks(self):

        # By default, empty the arrays of strings to look for in the logs; rely on the default check for "Good Iteration"
        self.wdm_next_args['client_log_check'] = []
        self.wdm_next_args['server_log_check'] = []

        # For some of the exceptions, enforce they are handled in a specific way
        #if self.wdm_next_args['server_faults']:

        #    #
        #    # Test the handling of the ExpiryTime and MustBeVersion fields of custom commands
        #    #
        #    if "Weave_WDMSendCommandExpired" in self.wdm_next_args['server_faults']:
        #        self.wdm_next_args['client_log_check'].append(["Command\[0\] \[.*\] SendError profile: 11, code: 36, err No Error", 1])
        #        self.wdm_next_args['server_log_check'].append(["Received Status Report 0xB : 0x24", 1])

        #    elif "Weave_WDMSendCommandBadVersion" in self.wdm_next_args['server_faults']:
        #        self.wdm_next_args['client_log_check'].append(["Command\[0\] \[.*\] SendError profile: 11, code: 37, err No Error", 1])
        #        self.wdm_next_args['server_log_check'].append(["Received Status Report 0xB : 0x25", 1])

        #if self.wdm_next_args['client_faults']:
        #    pass

    def test_weave_wdm_next_service_update_faults(self):

        # run the sequence once without faults to read the counters

        wdm_next_args = {}

        self.wdm_next_args = wdm_next_args

        wdm_next_args['wdm_option'] = "mutual_subscribe"
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['enable_client_flip'] = 1
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_client_delay'] = 4000
        wdm_next_args['timer_client_period'] = 4000
        wdm_next_args['client_clear_state_between_iterations'] = False
        wdm_next_args['test_client_case'] = 10 # kTestCase_TestUpdatableTraits
        wdm_next_args['total_client_count'] = 1 

        wdm_next_args['enable_retry'] = True 

        wdm_next_args['client_update_mutation'] = "OneLeaf" 
        wdm_next_args['client_update_num_traits'] = 2
        wdm_next_args['client_update_num_mutations'] = 1

        wdm_next_args['client_log_check'] = [('Mutual: Good Iteration', 1),
                                             ('Update: path result: success', 2),
                                             ('Update: no more pending updates', 1),
                                             ('Update: path failed', 0),
                                             ('Need to resubscribe', 0)]

        base_test_tag = 'test_weave_wdm_next_service_update_faults'
        wdm_next_args['test_tag'] = base_test_tag
        wdm_next_args['test_case_name'] = ['WDM Update sequence for fault-injection']

        self.wdm_next_args['client_faults'] = None

        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test update with faults"
        super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)

        output_logs = {}
        self.output_logs = output_logs
        output_logs['client'] = self.result_data[0]['client_output']

        # Make sure all important paths are going to be tested by checking the counters
        # self.validate_happy_sequence()

        # Usually we run 3 iterations; the first one or two can fail because of the fault being injected.
        # The third one should always succeed; the second one fails only because a fault that
        # was supposed to be triggered at the end of the first one ends up hitting the beginning
        # of the second one instead.
        # But since running tests against the service is very slow, just run 2 iterations for now.
        wdm_next_args['test_client_iterations'] = 2

        num_tests = 0

        node = 'client'

        fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node])

        # Again, this is very slow, so just run a few interesting cases for now.

        fault_configs = []

        # Drop the update response, which is usually the 11th message received (so skip 10):
        fault_configs += [
                "Weave_DropIncomingUDPMsg_s8_f1",
                "Weave_DropIncomingUDPMsg_s9_f1",
                "Weave_DropIncomingUDPMsg_s10_f1",
                "Weave_DropIncomingUDPMsg_s11_f1",
                "Weave_DropIncomingUDPMsg_s12_f1"]

        fault_configs += [
                "Weave_WDMSendUpdateBadVersion_s0_f1"]

        for fault_config in fault_configs:
            wdm_next_args['test_tag'] = base_test_tag + "_" + str(num_tests) + "_" + node + "_" + fault_config
            print wdm_next_args['test_tag']
            wdm_next_args['client_faults'] = fault_config
            wdm_next_args['server_faults'] = None

            self.setup_log_checks() 

            super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)
            num_tests += 1



if __name__ == "__main__":

    help_str = """usage:
    --help  Print this usage info and exit\n"""

    help_str += gFaultopts.help_string

    longopts = []
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

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

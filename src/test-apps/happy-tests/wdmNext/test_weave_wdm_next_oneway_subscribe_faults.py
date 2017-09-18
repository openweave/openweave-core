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
#       Tests a wdm one-way subscription with fault injection.
#       The client subscribes once and then cancels the subscription after 3 seconds.
#       The server in the mean time modifies its trait instance once.
#

import getopt
import sys
import unittest
import set_test_path
import plugin.WeaveUtilities as WeaveUtilities


from weave_wdm_next_test_base import weave_wdm_next_test_base

gFaultopts = WeaveUtilities.FaultInjectionOptions()


class test_weave_wdm_next_oneway_subscribe_faults(weave_wdm_next_test_base):

    def test_weave_wdm_next_oneway_subscribe_faults(self):

        # run the sequence once without faults to read the counters

        wdm_next_args = {}

        wdm_next_args['wdm_option'] = "one_way_subscribe"

        # Total_client_count is set to 1 even if the responder does not subscribe back.
        # This is done so that the client waits 3 seconds before canceling the subscription,
        # and the responder in the mean time gets a chance to modify the trait instance.
        wdm_next_args['total_client_count'] = 1
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 3000
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_client_delay'] = 2000
        wdm_next_args['enable_client_flip'] = 1

        wdm_next_args['total_server_count'] = 1
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 2000 
        wdm_next_args['test_server_delay'] = 0
        wdm_next_args['enable_server_flip'] = 1

        wdm_next_args['client_log_check'] = [('Client\[0\] \[(ALIVE|CONFM)\] EndSubscription Ref\(\d+\)', wdm_next_args['test_client_iterations']),
                                             ('Client\[0\] \[CANCL\] AbortSubscription Ref\(\d+\)', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', wdm_next_args['test_client_iterations'] * (wdm_next_args['total_server_count']) +1),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]
        wdm_next_args['server_log_check'] = [('Handler\[0\] \[(ALIVE|CONFM)\] CancelRequestHandler', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]

        base_test_tag = "_ONEWAY_SUB_FAULTS"
        wdm_next_args['test_tag'] = base_test_tag
        wdm_next_args['test_case_name'] = ['One way subscription happy sequence for fault injection']

        wdm_next_args['client_faults'] = None
        wdm_next_args['server_faults'] = None

        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test one way with faults"

        super(test_weave_wdm_next_oneway_subscribe_faults, self).weave_wdm_next_test_base(wdm_next_args)

        output_logs = {}
        output_logs['client'] = self.result_data[0]['client_output']
        output_logs['server'] = self.result_data[0]['server_output']

        # Run 3 iterations; the first one or two can fail because of the fault being injected.
        # The third one should always succeed; the second one fails only because a fault that
        # was supposed to be triggered at the end of the first one ends up hitting the beginning
        # of the second one instead.
        wdm_next_args['test_client_iterations'] = 3
        # empty the arrays of strings to look for in the logs; rely on the default check for "Good Iteration"
        wdm_next_args['client_log_check'] = []
        wdm_next_args['server_log_check'] = []

        num_tests = 0

        for node in gFaultopts.nodes:
            fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node])
        
            for fault_config in fault_configs:
                wdm_next_args['test_tag'] = base_test_tag + "_" + str(num_tests) + "_" + node + "_" + fault_config
                print wdm_next_args['test_tag']
                if node == 'client':
                    wdm_next_args['client_faults'] = fault_config
                    wdm_next_args['server_faults'] = None
                else:
                    wdm_next_args['client_faults'] = None
                    wdm_next_args['server_faults'] = fault_config

                super(test_weave_wdm_next_oneway_subscribe_faults, self).weave_wdm_next_test_base(wdm_next_args)
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

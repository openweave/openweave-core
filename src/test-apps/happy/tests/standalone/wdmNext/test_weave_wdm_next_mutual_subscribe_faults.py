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
#       Calls Weave WDM mutual subscribe between nodes with fault-injection.
#       Both initiator and responder mutate the trait instances.
#       The subscriptions are terminated by the initiator.
#       The initiator generates events as well.
#

import getopt
import sys
import unittest
import set_test_path
import re
import WeaveUtilities
from happy.Utils import *


from weave_wdm_next_test_base import weave_wdm_next_test_base

gFaultopts = WeaveUtilities.FaultInjectionOptions()


class test_weave_wdm_next_mutual_subscribe_faults(weave_wdm_next_test_base):

    def validate_happy_sequence(self):

        output_logs = self.output_logs
        counters = {}
        counters['client'] = WeaveUtilities.parse_fault_injection_counters(output_logs['client'])
        counters['server'] = WeaveUtilities.parse_fault_injection_counters(output_logs['server'])

        # As of today, all fault points should be executed on the server, but not the WDM Update ones
        wdm_udpate_faults = [ "Weave_WDMSendUpdateBadVersion",
                              "Weave_WDMDelayUpdateResponse",
                              "Weave_WDMUpdateRequestTimeout",
                              "Weave_WDMUpdateRequestSendErrorInline",
                              "Weave_WDMUpdateRequestSendErrorAsync",
                              "Weave_WDMUpdateRequestBadProfile",
                              "Weave_WDMUpdateResponseBusy",
                              "Weave_WDMUpdateRequestDropMessage",
                              "Weave_WDMPathStoreFull"]
        not_required_server_faults = wdm_udpate_faults

        required_server_faults = [ key for key in counters["server"].keys() if "Weave_WDM" in key and key not in not_required_server_faults]

        # This will raise a ValueError
        WeaveUtilities.validate_counters(required_server_faults, counters["server"], "server")

        # As of today, the client does not send commands nor updates
        not_required_client_faults = [ "Weave_WDMSendCommandBadVersion",
                                       "Weave_WDMSendCommandExpired"]
        not_required_client_faults += wdm_udpate_faults

        required_client_faults = [ key for key in counters["client"].keys() if "Weave_WDM" in key and key not in not_required_client_faults]

        # This will raise a ValueError
        WeaveUtilities.validate_counters(required_client_faults, counters["client"], "client")


    def setup_log_checks(self):

        # By default, empty the arrays of strings to look for in the logs; rely on the default check for "Good Iteration"
        self.wdm_next_args['client_log_check'] = []
        self.wdm_next_args['server_log_check'] = []

        # For some of the exceptions, enforce they are handled in a specific way
        if self.wdm_next_args['server_faults']:

            #
            # Test the handling of the ExpiryTime and MustBeVersion fields of custom commands
            #
            if "Weave_WDMSendCommandExpired" in self.wdm_next_args['server_faults']:
                self.wdm_next_args['client_log_check'].append(["Command\[0\] \[.*\] SendError profile: 11, code: 36, err No Error", 1])
                self.wdm_next_args['server_log_check'].append(["Received Status Report 0xB : 0x24", 1])

            elif "Weave_WDMSendCommandBadVersion" in self.wdm_next_args['server_faults']:
                self.wdm_next_args['client_log_check'].append(["Command\[0\] \[.*\] SendError profile: 11, code: 37, err No Error", 1])
                self.wdm_next_args['server_log_check'].append(["Received Status Report 0xB : 0x25", 1])

        if self.wdm_next_args['client_faults']:
            pass

    def test_weave_wdm_next_mutual_subscribe_faults(self):

        # run the sequence once without faults to read the counters

        wdm_next_args = {}

        self.wdm_next_args = wdm_next_args

        wdm_next_args['wdm_option'] = "mutual_subscribe"

        wdm_next_args['total_client_count'] = 2
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 5000
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_client_delay'] = 2000
        wdm_next_args['enable_client_flip'] = 1

        wdm_next_args['total_server_count'] = 2
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 4000
        wdm_next_args['enable_server_flip'] = 1

        wdm_next_args['client_event_generator'] = 'Liveness'

        wdm_next_args['client_inter_event_period'] = 9000

        wdm_next_args['client_log_check'] = [('Handler\[0\] \[(ALIVE|CONFM)\] bound mutual subscription is going away', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', wdm_next_args['test_client_iterations'] * (wdm_next_args['total_server_count'] + 1)),
                                             ('Client\[0\] \[(ALIVE|CONFM)\] EndSubscription Ref\(\d+\)', wdm_next_args['test_client_iterations']),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]
        wdm_next_args['server_log_check'] = [('bound mutual subscription is going away', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', 4),
                                             ('Client\[0\] \[(ALIVE|CONFM)\] CancelRequestHandler', wdm_next_args['test_client_iterations']),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]

        base_test_tag = "_MUTUAL_SUB_FAULTS"
        wdm_next_args['test_tag'] = base_test_tag
        wdm_next_args['test_case_name'] = ['Mutual subscription happy sequence for fault-injection']

        self.wdm_next_args['client_faults'] = None
        self.wdm_next_args['server_faults'] = None

        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test mutual with faults"

        super(test_weave_wdm_next_mutual_subscribe_faults, self).weave_wdm_next_test_base(wdm_next_args)

        output_logs = {}
        self.output_logs = output_logs
        output_logs['client'] = self.result_data[0]['client_output']
        output_logs['server'] = self.result_data[0]['server_output']

        # Make sure all important paths are going to be tested by checking the counters
        self.validate_happy_sequence()

        # Run 3 iterations; the first one or two can fail because of the fault being injected.
        # The third one should always succeed; the second one fails only because a fault that
        # was supposed to be triggered at the end of the first one ends up hitting the beginning
        # of the second one instead.
        wdm_next_args['test_client_iterations'] = 3

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

                self.setup_log_checks() 

                super(test_weave_wdm_next_mutual_subscribe_faults, self).weave_wdm_next_test_base(wdm_next_args)
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

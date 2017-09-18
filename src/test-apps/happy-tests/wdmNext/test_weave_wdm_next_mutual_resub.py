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
#       Injects a series of faults into the SubscriptionClient and
#       SubscriptionHandler, confirming the retry mechanism for a
#       mutual subscription is still successfully established
#       after some number of resubscribes.
#

import getopt
import sys
import unittest
import set_test_path
import plugin.WeaveUtilities as WeaveUtilities


from weave_wdm_next_test_base import weave_wdm_next_test_base

gFaultopts = WeaveUtilities.FaultInjectionOptions()


class test_weave_wdm_next_mutual_resub(weave_wdm_next_test_base):

    def test_weave_wdm_next_mutual_resub(self):
        wdm_next_args = {}

        wdm_next_args['wdm_option'] = "mutual_subscribe"

        wdm_next_args['total_client_count'] = 2
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 5000
        wdm_next_args['enable_client_stop'] = True
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_client_delay'] = 2000
        wdm_next_args['enable_client_flip'] = 1

        wdm_next_args['total_server_count'] = 2
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 4000
        wdm_next_args['enable_server_stop'] = False
        wdm_next_args['test_server_iterations'] = 1
        wdm_next_args['enable_server_flip'] = 1
        wdm_next_args['enable_retry'] = True

        base_test_tag = "_MUTUAL_RESUB"
        wdm_next_args['test_tag'] = base_test_tag
        wdm_next_args['test_case_name'] = ['N04: Mutual Subscribe: Root path, Null Version, Resubscribe On Error']

        # We're going to run once, but expect a successful subscription since
        # retries are enabled.
        NUM_ITERATIONS = 1
        wdm_next_args['test_client_iterations'] = NUM_ITERATIONS
        wdm_next_args['server_log_check'] = []

        fault_list = {}
        fault_list['server'] = [
                # fail to create the first subscription handler on the
                # server side. this sends a StatusReport to the client
                # in the subscribing stage, which triggers another
                # subscribe request to be sent.
                ('Weave_WDMSubscriptionHandlerNew_s0_f1', 2),

                # on counter subscription, responder sends an invalid
                # message type, which triggers a StatusReport from the
                # initiator. this causes the responder to tear down the
                # mutual subscription (which is in confirming state),
                # then a retry is set up.
                ('Weave_WDMSendUnsupportedReqMsgType_s0_f1', 2),

                # reject the NotificationRequest coming from the initiator,
                # sending StatusReport to the client after the mutual
                # subscription is established (initiator will be in the alive
                # state).
                ('Weave_WDMBadSubscriptionId_s0_f1', 2),
                ]
        fault_list['client'] = [
                # fail 3 instances of a send error after allowing the first.
                # this will cause a failure after the subscription has been
                # established. fail a few instances to confirm the retries
                # continue. this tests the OnSendError and OnResponseTimeout
                # callbacks in WDM.
                ('Weave_WRMSendError_s1_f3', 4),
                ]

        for node in gFaultopts.nodes:
            for fault_config, expected_retries in fault_list[node]:
                wdm_next_args['client_log_check'] = [("Good Iteration", NUM_ITERATIONS),
                                                     ('SendSubscribeRequest', NUM_ITERATIONS * expected_retries)]
                wdm_next_args['test_tag'] = "%s_%s_FAULT_%s" % (base_test_tag, node.upper(), fault_config)
                print wdm_next_args['test_tag']
                if node == 'client':
                    wdm_next_args['client_faults'] = fault_config
                    wdm_next_args['server_faults'] = None
                else:
                    wdm_next_args['client_faults'] = None
                    wdm_next_args['server_faults'] = fault_config

                super(test_weave_wdm_next_mutual_resub, self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":

    help_str = """usage:
    --help  Print this usage info and exit\n"""

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

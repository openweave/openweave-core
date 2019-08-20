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
import WeaveUtilities
import WeaveWdmNextOptions as wwno

from weave_wdm_next_test_base import weave_wdm_next_test_base

gFaultopts = WeaveUtilities.FaultInjectionOptions()


class test_weave_wdm_next_mutual_resub(weave_wdm_next_test_base):

    def test_weave_wdm_next_mutual_resub(self):
        wdm_next_args = self.get_test_param_json(self.__class__.__name__)
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

        base_test_tag = wdm_next_args[wwno.TEST][wwno.TEST_TAG]

        for node in gFaultopts.nodes:
            for fault_config, expected_retries in fault_list[node]:
                wdm_next_args[wwno.CLIENT][wwno.LOG_CHECK] = [
                    ("Good Iteration", wdm_next_args[wwno.CLIENT][wwno.TEST_ITERATIONS]),
                    ('SendSubscribeRequest', wdm_next_args[wwno.CLIENT][wwno.TEST_ITERATIONS] * expected_retries)]
                wdm_next_args[wwno.TEST][wwno.TEST_TAG] = "%s_%s_FAULT_%s" % (
                    base_test_tag, node.upper(), fault_config)
                print wdm_next_args[wwno.TEST][wwno.TEST_TAG]
                if node == 'client':
                    wdm_next_args[wwno.CLIENT][wwno.FAULTS] = fault_config
                    wdm_next_args[wwno.SERVER][wwno.FAULTS] = None
                else:
                    wdm_next_args[wwno.CLIENT][wwno.FAULTS] = None
                    wdm_next_args[wwno.SERVER][wwno.FAULTS] = fault_config

                super(
                    test_weave_wdm_next_mutual_resub,
                    self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":

    help_str = """usage:
    --help  Print this usage info and exit\n"""

    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

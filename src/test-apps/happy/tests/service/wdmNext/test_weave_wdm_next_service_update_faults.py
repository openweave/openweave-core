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
import itertools
from happy.Utils import *
from weave_wdm_next_test_service_base import weave_wdm_next_test_service_base

import WeaveUtilities

gTestCases = [ "UpdateResponseDelay", "UpdateResponseTimeout", "CondUpdateBadVersion",
               "UpdateRequestSendError", "UpdateRequestBadProfile", "UpdateRequestBadProfileBeforeSub",
               "PartialUpdateRequestSendError"]
gConditionalities = [ "Conditional", "Unconditional", "Mixed" ]
gFaultopts = WeaveUtilities.FaultInjectionOptions()
gOpts = { "conditionality" : None,
          "testcase"       : None
          }

class test_weave_wdm_next_service_update_faults(weave_wdm_next_test_service_base):

    def configure_test(self, testcase, conditionality):

        wdm_next_args = self.wdm_next_args

        # By default, empty the arrays of strings to look for in the logs; rely on the default check for "Good Iteration"
        wdm_next_args['client_log_check'] = []
        wdm_next_args['server_log_check'] = []

        # Usually we run 3 iterations; the first one or two can fail because of the fault being injected.
        # The third one should always succeed; the second one fails only because a fault that
        # was supposed to be triggered at the end of the first one ends up hitting the beginning
        # of the second one instead.
        # But since running tests against the service is very slow, just run 1 iteration for now and check
        # that the failure is handled; to properly test the success-on-retry we need to improve the base class so
        # we can specify a different log_check per iteration.
        wdm_next_args['test_client_iterations'] = 1

        fault_config = None

        if testcase == "UpdateResponseDelay":
            fault_config = "Weave_WDMDelayUpdateResponse_s0_f1"

            # Check that everything succeeds as usual
            client_log_check = self.happy_path_log_check

            # Check that the notifications received before the StatusReport cause the PotentialDataLoss flag to be set;
            # it is then cleared by the StatusReport.
            client_log_check += [("Potential data loss set for traitDataHandle", 2),
                                 ("Potential data loss cleared for traitDataHandle", 2)]

        if testcase == "UpdateResponseTimeout":
            fault_config = "WeaveSys_AsyncEvent_s0_f1"

            if conditionality == "Conditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # But the service has received it and processed it;
                        # There is no PotentialDataLoss because we retry the update before the subscription
                        ("Potential data loss set for traitDataHandle", 0),
                        ("MarkFailedPendingPaths", 0),
                        ("Need to resubscribe for potential data loss in TDH", 0),
                        # The second attempt fails for a version mismatch:
                        ("Update: path failed: Weave Error 4044:.*:37", 2),
                        # The update never succeeds:
                        ('Update: path result: success', 0)]
            elif conditionality == "Unconditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # No data loss because we retry the update before the subscription
                        ("Potential data loss set for traitDataHandle", 0),
                        ("MarkFailedPendingPaths", 0),
                        ("The conditional update of a WDM path failed for a version mismatch", 0),
                        # Finally the update succeeds:
                        ('Update: path result: success', 2)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # But the service has received it and processed it;
                        # We retry the update before the sub, so no potential data loss.
                        ("Potential data loss set for traitDataHandle", 0),
                        ("MarkFailedPendingPaths", 0),
                        # The update succeeds only for the unconditional trait; the conditional one fails with version mismatch
                        ("Update: path failed: Weave Error 4044:.*:37", 1),
                        ('Update: path result: success', 1)]

        if testcase == "UpdateRequestSendError":

            fault_config = "Weave_WDMUpdateRequestSendError_s0_f1"

            # In this case, there is no significant difference between Conditional and Unconditional

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Weave Error 4099: Message not acknowledged", 2),
                    # The update succeeds after retrying:
                    ('Update: path result: success', 2),
                    # when resubscribing, the notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    # After the notification, no pending paths are purged
                    ("MarkFailedPendingPaths", 0),
                    ("The conditional update of a WDM path failed for a version mismatch", 0)]

        if testcase == "PartialUpdateRequestSendError":
            fault_config = "Weave_WDMUpdateRequestSendError_s0_f1"

            wdm_next_args['client_update_mutation'] = "WholeLargeDictionary"
            wdm_next_args['client_update_num_traits'] = 1

            # In this case, there is no significant difference between Conditional and Unconditional
            # One trait with two paths: a leaf and a huge dictionary.

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Weave Error 4099: Message not acknowledged", 2),
                    # The update succeeds after retrying:
                    ('Update: path result: success', 2),
                    # The subscription goes down and is brought up after the update retries.
                    # The notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    ("MarkFailedPendingPaths", 0),
                    ("The conditional update of a WDM path failed for a version mismatch", 0)]

        if testcase == "CondUpdateBadVersion":
            fault_config = "Weave_WDMSendUpdateBadVersion_s0_f1"
            if conditionality == "Conditional":
                client_log_check = [
                        # Status report "Multiple failures"
                        ("Received Status Report 0xB : 0x2B", 1),
                        # Both paths fail and the application is notified:
                        ("Update: path failed: Weave Error 4044: Status Report received from peer, \[ WDM\(0000000B\):37 \]", 2),
                        # The UpdateResponse handler decides to resubscribe
                        ("UpdateResponse: triggering resubscription", 1),
                        # In this case there are no notification while paths are pending/in-progress:
                        ("Potential data loss set for traitDataHandle", 0)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # Status report "Multiple failures"
                        ("Received Status Report 0xB : 0x2B", 1),
                        # The conditional path fails and the application is notified:
                        ("Update: path failed: Weave Error 4044: Status Report received from peer, \[ WDM\(0000000B\):37 \]", 1),
                        # The unconditional path succeeds:
                        ('Update: path result: success', 1),
                        # The UpdateResponse handler decides to resubscribe because of the failed conditional update:
                        ("UpdateResponse: triggering resubscription", 1),
                        # In this case there are no notification while paths are pending/in-progress:
                        ("Potential data loss set for traitDataHandle", 0)]
            else:
                # Can't inject a bad version in a scenarion without conditional updates
                return False

        if testcase == "UpdateRequestBadProfile":
            fault_config = "Weave_WDMUpdateRequestBadProfile_s0_f1"
            # This fault makes the service reply with an InternalError status.
            # Weave holds on to the metadata and retries the update.
            client_log_check = [
                    # Status report "Internal error"
                    ("Received Status Report 0x0 : 0x50", 1),
                    # The paths fail and the application is notified:
                    ("Update: path failed: Weave Error 4044: Status Report received from peer, .*Internal error", 2),
                    # The update is tried again and it succeeds
                    ('Update: path result: success', 2),
                    # There are no notification while paths are pending/in-progress:
                    ("Potential data loss set for traitDataHandle", 0)
                    ]

        if testcase == "UpdateRequestBadProfileBeforeSub":
            fault_config = "Weave_WDMUpdateRequestBadProfile_s0_f1"
            # This fault makes the service reply with an InternalError status.
            # Weave holds on to the metadata and retries the update.
            wdm_next_args['client_update_timing'] = "BeforeSub"
            if conditionality == "Unconditional":
                client_log_check = [
                        ('Update: path result: success', 2),
                        ]
            else:
                return False

        wdm_next_args['test_tag'] = self.base_test_tag + "_" + str(self.num_tests) + "_" + testcase + "_" + conditionality + "_" + fault_config
        print wdm_next_args['test_tag']
        wdm_next_args['client_faults'] = fault_config
        wdm_next_args['client_update_conditionality'] = conditionality
        wdm_next_args['client_log_check'] = client_log_check

        return True


    def test_weave_wdm_next_service_update_faults(self):

        # Run the sequence once; usually we do this to profile it and know which faults
        # to inject. In this case it's just to know that the happy path works: we only
        # execute a few specific fault handling cases, for performance reasons.

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
        wdm_next_args['client_update_conditionality'] = gOpts["conditionality"]
        wdm_next_args['client_update_num_traits'] = 2
        wdm_next_args['client_update_num_mutations'] = 1

        self.happy_path_log_check = [('Mutual: Good Iteration', 1),
                                     ('Update: path result: success', 2),
                                     ('Update: no more pending updates', 1),
                                     ('Update: path failed', 0),
                                     ('Need to resubscribe', 0)]

        wdm_next_args['client_log_check'] = self.happy_path_log_check

        self.base_test_tag = 'test_weave_wdm_next_service_update_faults'
        wdm_next_args['test_tag'] = self.base_test_tag
        wdm_next_args['test_case_name'] = ['WDM Update sequence for fault-injection']

        wdm_next_args['client_faults'] = None
        wdm_next_args['server_faults'] = None

        node = 'client'

        if False:
            print 'test file: ' + self.__class__.__name__
            print "weave-wdm-next test update with faults"
            super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)

            output_logs = {}
            self.output_logs = output_logs
            output_logs['client'] = self.result_data[0]['client_output']

            fault_configs = gFaultopts.generate_fault_config_list(node, output_logs[node])

        self.num_tests = 0

        fault_configs = []

        testcases = [ gOpts["testcase"] ]
        conditionalities = [ gOpts["conditionality"] ]
        if (conditionalities[0] == None):
            conditionalities = gConditionalities

        if (testcases[0] == None):
            testcases = gTestCases

        print "testcases: " + str(testcases)
        print "conditionalities: " + str(conditionalities)

        for (testcase, conditionality) in itertools.product(testcases, conditionalities):

            # restore defaults, and then cofigure for this particular test
            wdm_next_args['client_update_mutation'] = "OneLeaf"
            wdm_next_args['client_update_num_traits'] = 2
            wdm_next_args['client_update_num_mutations'] = 1

            if not self.configure_test(testcase, conditionality):
                continue

            print "Testing: " + testcase + " " + conditionality

            self.assertTrue(len(self.wdm_next_args["client_log_check"]) > 0, "will not run a test without log checks")

            super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)
            self.num_tests += 1

        print "Executed " + str(self.num_tests) + " tests"



if __name__ == "__main__":

    help_str = """usage:
    --help  Print this usage info and exit
    --testcase { """ + """, """.join(gTestCases) + """ } (default: all of them)
    --conditionality { """ + """, """.join(gConditionalities) + """ } (default: all of them)
    """

    help_str += gFaultopts.help_string

    longopts = ["help", "conditionality=", "testcase="]
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
        if o in ("--conditionality"):
            gOpts["conditionality"] = a
        if o in ("--testcase"):
            if not (a in gTestCases):
                print help_str
                sys.exit(0)
            gOpts["testcase"] = a


    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

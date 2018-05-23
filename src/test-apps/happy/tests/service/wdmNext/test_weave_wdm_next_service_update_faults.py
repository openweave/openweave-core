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

gTestCases = [ "UpdateResponseDelay", "UpdateResponseTimeout", "CondUpdateBadVersion" ] 
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
                        # when resubscribing, the notification triggers PotentialDataLoss:
                        ("Potential data loss set for traitDataHandle", 2),
                        # After the notification, the pending paths are purged:
                        ("MarkFailedPendingPaths", 2),
                        # The application is notified again because now the two paths have been purged:
                        ("The conditional update of a WDM path failed for a version mismatch", 2),
                        # Data loss at the SubscribeResponse triggers another subscription:
                        ("Need to resubscribe for potential data loss in TDH", 2),
                        # This last subscription request clears the flag:
                        ("Potential data loss cleared for traitDataHandle", 2),
                        # The update never succeeds:
                        ('Update: path result: success', 0)]
            elif conditionality == "Unconditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # When resubscribing, the notification triggers PotentialDataLoss:
                        ("Potential data loss set for traitDataHandle", 2),
                        # Resubscribing a second time clears the potential data loss:
                        ("Potential data loss cleared for traitDataHandle", 2),
                        # Finally the update succeeds:
                        ('Update: path result: success', 2)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # But the service has received it and processed it; 
                        # when resubscribing, the notification triggers PotentialDataLoss:
                        ("Potential data loss set for traitDataHandle", 2),
                        # After the notification, the conditional pending paths are purged:
                        ("MarkFailedPendingPaths", 1),
                        # The application is notified again for the path that has been purged:
                        ("The conditional update of a WDM path failed for a version mismatch", 1),
                        # Resubscribing a second time clears the potential data loss on the conditional trait;
                        # the unconditional one is cleared by the successful update at the end.
                        ("Potential data loss cleared for traitDataHandle", 2),
                        # The update succeeds only for the unconditional trait:
                        ('Update: path result: success', 1)]

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


            # Make sure all important paths are going to be tested by checking the counters
            # self.validate_happy_sequence()

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

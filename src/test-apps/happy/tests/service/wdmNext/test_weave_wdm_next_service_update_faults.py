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
import copy
from happy.Utils import *
from weave_wdm_next_test_service_base import weave_wdm_next_test_service_base

import WeaveUtilities

gScenarios = [ "UpdateResponseDelay", "UpdateResponseTimeout", "UpdateResponseBusyAndFailed", "CondUpdateBadVersion",
               "UpdateRequestSendError", "UpdateRequestSendErrorInline", "UpdateRequestBadProfile", "UpdateRequestBadProfileBeforeSub",
               "PartialUpdateRequestSendError", "PartialUpdateRequestSendErrorInline", "DiscardUpdatesOnNoResponse", "DiscardUpdatesOnStatusReport",
               "UpdateResponseMultipleTimeout", "UpdateResponseMultipleTimeoutBeforeSub",
               "FailBindingBeforeSub", "PathStoreFullOnSendError", "PathStoreFullOnSendErrorInline", "UpdateRequestMutateDictAndFailSendBeforeSub"]
gConditionalities = [ "Conditional", "Unconditional", "Mixed" ]
gFaultopts = WeaveUtilities.FaultInjectionOptions()
gOpts = { "conditionality" : None,
          "scenario"       : None
          }

class test_weave_wdm_next_service_update_faults(weave_wdm_next_test_service_base):

    def configure_test(self, scenario, conditionality):

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

        if scenario == "UpdateResponseDelay":
            fault_config = "Weave_WDMDelayUpdateResponse_s0_f1"

            # Check that everything succeeds as usual
            client_log_check = copy.copy(self.happy_path_log_check)

            # Check that the notifications received before the StatusReport cause the PotentialDataLoss flag to be set;
            # it is then cleared by the StatusReport.
            client_log_check += [("Potential data loss set for traitDataHandle", 2),
                                 ("Potential data loss cleared for traitDataHandle", 2)]

        if scenario == "UpdateResponseTimeout":
            fault_config = "Weave_WDMUpdateRequestTimeout_s0_f1"

            if conditionality == "Conditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # But the service has received it and processed it;
                        # The first notification causes a PotentialDataLoss, and purges the udpate on the trait
                        ("Potential data loss set for traitDataHandle", 1),
                        ("MarkFailedPendingPaths", 1),
                        ("Update: path failed: Weave Error 4176: The conditional update of a WDM path failed for a version mismatch", 1),
                        # The other path is retried, but it fails with VersionMismatch in the StatusReport
                        ("Update: path failed: Weave Error 4044:.*:37", 1)]
            elif conditionality == "Unconditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # The notifications are received and they mark data loss
                        ("Potential data loss set for traitDataHandle", 2),
                        # There is no purging
                        ("MarkFailedPendingPaths", 0),
                        # Finally the update succeeds:
                        ('Update: path result: success', 2)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2),
                        # But the service has received it and processed it;
                        # The notification on the conditional trait causes its update to be purged and the subscription to go down
                        ("Potential data loss set for traitDataHandle", 1),
                        ("MarkFailedPendingPaths", 1),
                        # The unconditional update succeeds on retry
                        ('Update: path result: success', 1)]

        if scenario == "UpdateResponseMultipleTimeout":
            num_failures = 2
            fault_config = "Weave_WDMUpdateRequestTimeout_s0_f" + str(num_failures)

            if conditionality == "Conditional":
                client_log_check = [
                        # The UpdateRequest timeout: there are 3 because it retries 2 times, but one
                        # of the two paths gets purged at the first notification
                        ("Update: path failed: Weave Error 4050: Timeout", 2*num_failures -1),
                        # The service receives the message, and the first notification purges one path and causes the subscription
                        # to go down.
                        # Everything else depends on how quickly the subscription is re-established (service will send kStatus_Busy a couple of times)
                        # and how the notifications interleave with the update attempts.
                        ('Update: path result: success', 0)]
            elif conditionality == "Unconditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2*num_failures),
                        # We receive notifications back because the the requests receive the service:
                        ("Potential data loss set for traitDataHandle", 2),
                        ("MarkFailedPendingPaths", 0),
                        ("The conditional update of a WDM path failed for a version mismatch", 0),
                        # Finally the update succeeds:
                        ('Update: path result: success', 2)]
            else:
                client_log_check = [
                        # only the unconditional one goes through eventually
                        ('Update: path result: success', 1)]

        if scenario == "UpdateResponseMultipleTimeoutBeforeSub":
            num_failures = 2
            fault_config = "Weave_WDMUpdateRequestTimeout_s0_f" + str(num_failures)
            wdm_next_args['client_update_timing'] = "BeforeSub"

            if conditionality == "Unconditional":
                client_log_check = [
                        # The UpdateRequest timeout:
                        ("Update: path failed: Weave Error 4050: Timeout", 2*num_failures),
                        # It's hard to guess what happens with multiple timeouts and the
                        # subscription being established.
                        # Finally the update succeeds:
                        ('Update: path result: success', 2)]
            else:
                return False

        if scenario == "UpdateRequestSendError":

            fault_config = "Weave_WDMUpdateRequestSendErrorAsync_s0_f1"

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

        if scenario == "PathStoreFullOnSendError":

            # Inject a SendError, and a PathStoreFull late enough that it hits when the SendError
            # is processed.
            fault_config = "Weave_WDMUpdateRequestSendErrorAsync_s0_f1:Weave_WDMPathStoreFull_s5_f1"

            # In this case, there is no significant difference between Conditional and Unconditional

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Weave Error 4099: Message not acknowledged.*will retry", 2),
                    # The PATH_STORE_FULL errors
                    ("Update: path failed: Weave Error 4181: A WDM TraitPath store is full.*will not retry", 2),
                    # The update never succeeds
                    ('Update: path result: success', 0)]

        if scenario == "PathStoreFullOnSendErrorInline":

            # Inject a SendError inline, and a PathStoreFull late enough that it hits when the SendError
            # is processed.
            fault_config = "Weave_WDMUpdateRequestSendErrorInline_s0_f1:Weave_WDMPathStoreFull_s5_f1"

            # In this case, there is no significant difference between Conditional and Unconditional

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Inet Error.*will retry", 2),
                    # The PATH_STORE_FULL errors
                    ("Update: path failed: Weave Error 4181: A WDM TraitPath store is full.*will not retry", 2),
                    # The update never succeeds
                    ('Update: path result: success', 0)]

        if scenario == "UpdateRequestSendErrorInline":

            fault_config = "Weave_WDMUpdateRequestSendErrorInline_s0_f1"

            # In this case, there is no significant difference between Conditional and Unconditional

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Inet Error", 2),
                    # The update succeeds after retrying:
                    ('Update: path result: success', 2),
                    # when resubscribing, the notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    # After the notification, no pending paths are purged
                    ("MarkFailedPendingPaths", 0),
                    ("The conditional update of a WDM path failed for a version mismatch", 0)]

        if scenario == "DiscardUpdatesOnNoResponse":

            # Inject a SendError and make the initiator discard the updates
            wdm_next_args['client_update_mutation'] = "FewDictionaryItems"
            wdm_next_args['client_update_discard_on_error'] = True

            fault_config = "Weave_WDMUpdateRequestSendErrorAsync_s0_f1"

            # In this case, there is no significant difference between Conditional and WholeDictionary

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    # Note that there is only one of these instead of 5 (4 paths in one trait and 1 in the other)
                    # because the application discards the paths at the first error.
                    ("Update: path failed: Weave Error 4099: Message not acknowledged", 1),
                    # Also, AbortUpdates sees all 5 paths to discard because the one triggering the notification
                    # would have been retried.
                    ("Discarded 0 pending  and 5 inProgress paths", 1),
                    # The update is not retried
                    ('Update: path result: success', 0),
                    # when resubscribing, the notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    # After the notification, no pending paths are purged
                    ("MarkFailedPendingPaths", 0)]

        if scenario == "DiscardUpdatesOnStatusReport":

            # Inject a SendError and make the initiator discard the updates
            wdm_next_args['client_update_mutation'] = "FewDictionaryItems"
            wdm_next_args['client_update_discard_on_error'] = True

            fault_config = "Weave_WDMSendUpdateBadVersion_s0_f1"
            if conditionality == "Conditional":
                client_log_check = [
                        # Note that there is one of these instead of 5 (4 paths in one trait and 1 in the other)
                        # because the application discards the paths at the first error.
                        ("Update: path failed: Weave Error 4044: Status Report received from peer, \[ WDM\(0000000B\):37 \]", 1),
                        # Also, AbortUpdates only sees 4 paths to discard because the one triggering the notification
                        # has been deleted already (it was not going to be retried).
                        ("Discarded 0 pending  and 4 inProgress paths", 1),
                        # The update is not retried
                        ('Update: path result: success', 0),
                        # when resubscribing, the notification does not trigger PotentialDataLoss:
                        ("Potential data loss set for traitDataHandle", 0),
                        # No resubscription from the UpdateResponse handler
                        ("UpdateResponse: triggering resubscription", 0),
                        # After the notification, no pending paths are purged
                        ("MarkFailedPendingPaths", 0)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # Like above, only the first trait is conditional and fails...
                        ("Update: path failed: Weave Error 4044: Status Report received from peer, \[ WDM\(0000000B\):37 \]", 1),
                        # Also, AbortUpdates only sees 4 paths to discard because the one triggering the notification
                        # has been deleted already (it was not going to be retried).
                        ("Discarded 0 pending  and 4 inProgress paths", 1),
                        # ... but the application calls DiscardUpdates, and so it does not get notified of the success either.
                        ('Update: path result: success', 0),
                        # No resubscription from the UpdateResponse handler
                        ("UpdateResponse: triggering resubscription", 0),
                        # when resubscribing, the notification does not trigger PotentialDataLoss:
                        ("Potential data loss set for traitDataHandle", 0),
                        # After the notification, no pending paths are purged
                        ("MarkFailedPendingPaths", 0)]
            else:
                # Can't inject a bad version in a scenarion without conditional updates
                return False

        if scenario == "PartialUpdateRequestSendError":
            fault_config = "Weave_WDMUpdateRequestSendErrorAsync_s0_f1"

            wdm_next_args['client_update_mutation'] = "WholeLargeDictionary"
            wdm_next_args['client_update_num_traits'] = 1

            # In this case, there is no significant difference between Conditional and Unconditional
            # One trait with two paths: a leaf and a huge dictionary.

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Weave Error 4099: Message not acknowledged", 2),
                    # The update succeeds after retrying:
                    ('Update: path result: success', 2),
                    # The notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    ("MarkFailedPendingPaths", 0),
                    ("The conditional update of a WDM path failed for a version mismatch", 0)]

        if scenario == "PartialUpdateRequestSendErrorInline":
            fault_config = "Weave_WDMUpdateRequestSendErrorInline_s0_f1"

            wdm_next_args['client_update_mutation'] = "WholeLargeDictionary"
            wdm_next_args['client_update_num_traits'] = 1

            # In this case, there is no significant difference between Conditional and Unconditional
            # One trait with two paths: a leaf and a huge dictionary.

            client_log_check = [
                    # The UpdateRequest SendError (note: the responder does not receive the request):
                    ("Update: path failed: Inet Error", 2),
                    # The update succeeds after retrying:
                    ('Update: path result: success', 2),
                    # The notification does not trigger PotentialDataLoss:
                    ("Potential data loss set for traitDataHandle", 0),
                    ("MarkFailedPendingPaths", 0),
                    ("The conditional update of a WDM path failed for a version mismatch", 0)]

        if scenario == "CondUpdateBadVersion":
            fault_config = "Weave_WDMSendUpdateBadVersion_s0_f1"
            if conditionality == "Conditional":
                client_log_check = [
                        # Status report "Multiple failures"
                        ("Received StatusReport .*WDM.0000000B.:43", 1),
                        # Both paths fail (even if the second trait has a good version - the service should really treat them separately) 
                        # and the application is notified:
                        ("Update: path failed: Weave Error 4044: Status Report received from peer.*WDM.0000000B.:37", 2),
                        # The UpdateResponse handler decides to resubscribe
                        ("UpdateResponse: triggering resubscription", 1),
                        # In this case there are no notification while paths are pending/in-progress:
                        ("Potential data loss set for traitDataHandle", 0)]
            elif conditionality == "Mixed":
                client_log_check = [
                        # Status report "Multiple failures"
                        ("Received StatusReport .*WDM.0000000B.:43", 1),
                        # The conditional path fails and the application is notified:
                        ("Update: path failed: Weave Error 4044: Status Report received from peer.*WDM.0000000B.:37", 1),
                        # The unconditional path succeeds:
                        ('Update: path result: success', 1),
                        # The UpdateResponse handler decides to resubscribe because of the failed conditional update:
                        ("UpdateResponse: triggering resubscription", 1),
                        # In this case there are no notification while paths are pending/in-progress:
                        ("Potential data loss set for traitDataHandle", 0)]
            else:
                # Can't inject a bad version in a scenarion without conditional updates
                return False

        if scenario == "UpdateResponseBusyAndFailed":
            # The first fault makes the service reply with an InternalError status.
            # Then, the second fault overrides the second StatusElement with a Busy code
            fault_config = "Weave_WDMUpdateRequestBadProfile_s0_f1:Weave_WDMUpdateResponseBusy_s1_f1"
            # Weave holds on to the metadata of the busy one and retries the update.
            client_log_check = [
                    # Status report "Internal error"
                    ("Received StatusReport.*Common.*Internal error", 1),
                    # The paths fail and the application is notified:
                    ("Update: path failed: Weave Error 4044: Status Report received from peer, .*Internal error", 1),
                    ("Update: path failed: Weave Error 4044: Status Report received from peer, .*Sender busy", 1),
                    # The update that failed with BUSY is tried again and it succeeds
                    ('Update: path result: success', 1),
                    # There are no notification while paths are pending/in-progress:
                    ("Potential data loss set for traitDataHandle", 0)
                    ]

        if scenario == "UpdateRequestBadProfile":
            fault_config = "Weave_WDMUpdateRequestBadProfile_s0_f1"
            # This fault makes the service reply with an InternalError status.
            # Internal Error means the update should not be retried, and so there is no success.
            client_log_check = [
                    # Status report "Internal error"
                    ("Received StatusReport.*Common.*Internal error", 1),
                    # The paths fail and the application is notified:
                    ("Update: path failed: Weave Error 4044: Status Report received from peer, .*Internal error", 2),
                    # The update is not tried again because Internal Error is fatal
                    ('Update: path result: success', 0),
                    # There are no notification while paths are pending/in-progress:
                    ("Potential data loss set for traitDataHandle", 0)
                    ]

        if scenario == "UpdateRequestBadProfileBeforeSub":
            fault_config = "Weave_WDMUpdateRequestBadProfile_s0_f1"
            # This fault makes the service reply with an InternalError status.
            # Internal Error means the update should not be retried, and so there is no success.
            wdm_next_args['client_update_timing'] = "BeforeSub"
            if conditionality == "Unconditional":
                client_log_check = [
                        ("Received StatusReport.*Common.*Internal error", 1),
                        ('Update: path result: success', 0),
                        ]
            else:
                return False

        if scenario == "UpdateRequestMutateDictAndFailSendBeforeSub":
            fault_config = "Weave_WDMUpdateRequestDropMessage_s0_f2"

            wdm_next_args['client_update_mutation'] = "WholeDictionary"
            wdm_next_args['client_update_num_traits'] = 1

            # This fault makes the service reply with an InternalError status.
            # Internal Error means the update should not be retried, and so there is no success.
            wdm_next_args['client_update_timing'] = "BeforeSub"
            if conditionality == "Unconditional":
                client_log_check = [
                        ('Update: path result: success', 1),
                        ("Potential data loss set for traitDataHandle", 1),
                        ]
            else:
                return False

        if scenario == "FailBindingBeforeSub":
            fault_config = "Weave_CASEKeyConfirm_s0_f2"
            wdm_next_args['client_update_timing'] = "BeforeSub"
            if conditionality == "Unconditional":
                client_log_check = [
                        ('Update: path result: success', 2),
                        ]
            else:
                return False

        wdm_next_args['test_tag'] = self.base_test_tag + "_" + str(self.num_tests) + "_" + scenario + "_" + conditionality + "_" + fault_config
        print wdm_next_args['test_tag']
        wdm_next_args['client_faults'] = fault_config
        wdm_next_args['client_update_conditionality'] = conditionality

        # In all cases, at the end the application should be notified that there are
        # no more pending updates.
        client_log_check.append(('Update: no more pending updates', 1))

        wdm_next_args['client_log_check'] = client_log_check

        return True

    def get_default_options(self):
        wdm_next_args = {}

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

        wdm_next_args['test_case_name'] = ['WDM Update sequence for fault-injection']

        self.base_test_tag = 'test_weave_wdm_next_service_update_faults'
        wdm_next_args['test_tag'] = self.base_test_tag

        wdm_next_args['client_faults'] = None
        wdm_next_args['server_faults'] = None


        return wdm_next_args

    def test_weave_wdm_next_service_update_faults(self):

        self.happy_path_log_check = [('Mutual: Good Iteration', 1),
                ('Update: path result: success', 2),
                ('Update: no more pending updates', 1),
                ('Update: path failed', 0),
                ('Need to resubscribe', 0)]


        if not (gOpts["conditionality"] or gOpts["scenario"]):

            # Run a clean sequence once; usually we do this to profile it and know which faults
            # to inject. In this case it's just to know that the happy path works: we only
            # execute a few specific fault handling cases, for performance reasons.
            # Also, send a "WholeDictionary" update so we reset the test traits to something
            # normal.

            self.wdm_next_args = self.get_default_options()

            wdm_next_args = self.wdm_next_args

            wdm_next_args['client_log_check'] = self.happy_path_log_check
            wdm_next_args['client_update_mutation'] = "WholeDictionary"

            node = 'client'

            print 'test file: ' + self.__class__.__name__
            print "weave-wdm-next test update with faults"
            super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)

        self.num_tests = 0

        fault_configs = []

        scenarios = [ gOpts["scenario"] ]
        conditionalities = [ gOpts["conditionality"] ]
        if (conditionalities[0] == None):
            conditionalities = gConditionalities

        if (scenarios[0] == None):
            scenarios = gScenarios

        print "scenarios: " + str(scenarios)
        print "conditionalities: " + str(conditionalities)

        results = []

        num_failures = 0

        for (scenario, conditionality) in itertools.product(scenarios, conditionalities):

            # restore defaults, and then cofigure for this particular test
            wdm_next_args = self.get_default_options()
            self.wdm_next_args = wdm_next_args

            if not self.configure_test(scenario, conditionality):
                continue

            print "Testing: " + scenario + " " + conditionality

            self.assertTrue(len(self.wdm_next_args["client_log_check"]) > 0, "will not run a test without log checks")

            result = "Success"

            try:
                super(test_weave_wdm_next_service_update_faults, self).weave_wdm_next_test_service_base(wdm_next_args)
            except:
                result = "Failure"
                num_failures = num_failures + 1
                pass
            results.append(result + " " + scenario + " " + conditionality)
            print results[-1]
            self.num_tests += 1

        print "Executed " + str(self.num_tests) + " tests; " + str(num_failures) + " failures"
        if (results):
            print "\n".join(results)



if __name__ == "__main__":

    help_str = """usage:
    --help  Print this usage info and exit
    --scenario { """ + """, """.join(gScenarios) + """ } (default: all of them)
    --conditionality { """ + """, """.join(gConditionalities) + """ } (default: all of them)
    """

    longopts = ["help", "conditionality=", "scenario="]

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h", longopts)

    except getopt.GetoptError as err:
        print help_str
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print help_str
            sys.exit(0)
        if o in ("--conditionality"):
            if not (a in gConditionalities):
                print help_str
                sys.exit(0)
            gOpts["conditionality"] = a
        if o in ("--scenario"):
            if not (a in gScenarios):
                print help_str
                sys.exit(0)
            gOpts["scenario"] = a


    sys.argv = [sys.argv[0]]
    WeaveUtilities.run_unittest()

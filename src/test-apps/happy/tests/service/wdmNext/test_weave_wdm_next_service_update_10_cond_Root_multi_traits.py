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
#       Calls Weave WDM mutual subscribe between mock device and real service.
#       Test Wdm-NestService-O07: Client creates a mutual subscription, sends one
#           UpdateRequest to the publisher with 2 traits, and receives a
#           StatusReport
#

import unittest
from weave_wdm_next_test_service_base import weave_wdm_next_test_service_base


class test_weave_wdm_next_service_update_10_cond_Root_multi_traits(weave_wdm_next_test_service_base):
    def test_weave_wdm_next_service_update_10_cond_Root_multi_traits(self):
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

        wdm_next_args['client_update_mutation'] = "Root" 
        wdm_next_args['client_update_num_traits'] = 4
        wdm_next_args['client_update_num_mutations'] = 1

        wdm_next_args['client_log_check'] = [('Mutual: Good Iteration', 1),
                                             ('Update: path result: success', 4),
                                             ('Update: no more pending updates', 1),
                                             ('Suppressing error .*; will try again later', 1), # make sure to test failing to add a DataElement...
                                             ('Msg sent 0000000B:44', 1),                       # ... which will cause a PartialUpdateReq + UpdateReq, instead of two separate UpdateRequests.
                                             ('Msg sent 0000000B:34', 1),
                                             ('Not inserting duplicate path', 1),  # processing the same root twice won't cause a duplicate private path for the dictionary
                                             ('Path already pending', 2),          # TestATrait's root mutation re-adds a path after adding root (there are two instances of TestATrait)
                                             ('replace dictionary', 0),      # All DataElements for dictionaries are "merge", not "replace" 
                                             #('DataElementPath = <Resource = {ProfileId = 0x235afe00,}/0xb = null>', 1),
                                             #('DataElementPath = <Resource = {ProfileId = 0x235afe00,InstanceId = 0x1,}/0xb = null>', 1),
                                             ('Update: path failed', 0),
                                             ('Need to resubscribe', 0)]

        wdm_next_args['test_tag'] = self.__class__.__name__
        wdm_next_args['test_case_name'] = ['Wdm-NestService-O10: Client creates a mutual subscription, sends one UpdateRequest to the publisher with 4 full traits, and receives a StatusReport']
        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test O10"
        super(test_weave_wdm_next_service_update_10_cond_Root_multi_traits, self).weave_wdm_next_test_service_base(wdm_next_args)


if __name__ == "__main__":
    unittest.main()

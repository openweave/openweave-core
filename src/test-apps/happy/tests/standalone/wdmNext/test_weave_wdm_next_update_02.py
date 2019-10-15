#!/usr/bin/env python


#
#    Copyright (c) 2019 Google, LLC.
#    Copyright (c) 2016-2018 Nest Labs, Inc.
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
#       Calls Weave WDM Update between nodes.
#       Update 02: Client creates mutual subscription, sends unconditional update request to publisher,
#       and receives notification and status report
#

import unittest
import set_test_path
from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_update_02(weave_wdm_next_test_base):

    def test_weave_wdm_next_mutual_subscribe_02(self):
        wdm_next_args = {}
        wdm_next_args['wdm_option'] = "mutual_subscribe"

        wdm_next_args['total_client_count'] = 1
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 10000
        wdm_next_args['test_client_iterations'] = 1
        wdm_next_args['test_client_delay'] = 2000
        wdm_next_args['enable_client_flip'] = 1
        wdm_next_args['test_client_case'] = 10 # kTestCase_TestUpdatableTrait

        wdm_next_args['enable_retry'] = True

        wdm_next_args['total_server_count'] = 0
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 0
        wdm_next_args['enable_server_flip'] = 0
        wdm_next_args['test_server_case'] = 10

        wdm_next_args['client_clear_state_between_iterations'] = False
        wdm_next_args['server_clear_state_between_iterations'] = False

        wdm_next_args['client_update_mutation'] = "SameLevelLeaves"
        wdm_next_args['client_update_conditionality'] = "Unconditional"
        wdm_next_args['client_update_num_mutations'] = 2
        wdm_next_args['client_update_num_traits'] = 2
        wdm_next_args['client_update_timing'] = "AfterSub"

        wdm_next_args['client_log_check'] = [('UpdateComplete event: 1', wdm_next_args['test_client_iterations'] * wdm_next_args['client_update_num_mutations'])]
        wdm_next_args['server_log_check'] = []
        wdm_next_args['test_tag'] = self.__class__.__name__[19:].upper()
        wdm_next_args['test_case_name'] = ['Update 02: Client creates mutual subscription, sends unconditional update request to publisher, and receives notification and status report']
        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next update test 02"
        super(test_weave_wdm_next_update_02, self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":
    WeaveUtilities.run_unittest()

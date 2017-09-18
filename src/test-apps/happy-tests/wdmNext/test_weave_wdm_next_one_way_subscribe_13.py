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
#       Calls Weave WDM one way subscribe between nodes.
#       D01: One way Subscribe: Multiple Iterations. Mutate data in publisher. Client aborts. Version is kept
#

import unittest
import set_test_path
from weave_wdm_next_test_base import weave_wdm_next_test_base
import plugin.WeaveUtilities as WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_13(weave_wdm_next_test_base):

    def test_weave_wdm_next_one_way_subscribe_13(self):
        wdm_next_args = {}

        wdm_next_args['wdm_option'] = "one_way_subscribe"

        wdm_next_args['total_client_count'] = 2
        wdm_next_args['final_client_status'] = 2
        wdm_next_args['timer_client_period'] = 5000
        wdm_next_args['test_client_iterations'] = 5
        wdm_next_args['test_client_delay'] = 35000
        wdm_next_args['enable_client_flip'] = 0

        wdm_next_args['total_server_count'] = 2
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 4000
        wdm_next_args['enable_server_flip'] = 1

        wdm_next_args['client_log_check'] = [('Client\[0\] \[(ALIVE|CONFM)\] AbortSubscription Ref\(\d+\)', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', wdm_next_args['test_client_iterations'] * wdm_next_args['total_server_count'] + 1),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations']),
                                             ('0 ==> 100 ==> 101 ==> 102 ==> 103 ==> 104 ==> 105 ==> 106 ==> 107 ==> 108 ==> 109 ==> 110', 2),
                                             ('0 ==> 200 ==> 201 ==> 202 ==> 203 ==> 204 ==> 205 ==> 206 ==> 207 ==> 208 ==> 209 ==> 210', 1),
                                             ('0 ==> 300 ==> 301 ==> 302 ==> 303 ==> 304 ==> 305 ==> 306 ==> 307 ==> 308 ==> 309 ==> 310', 1)]
        wdm_next_args['server_log_check'] = [('Handler\[0\] \[(ALIVE|CONFM)\] TimerEventHandler Ref\(\d+\) Timeout', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] \[(ALIVE|CONFM)\] AbortSubscription Ref\(\d+\)', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]

        wdm_next_args['test_tag'] = self.__class__.__name__[19:].upper()
        wdm_next_args['test_case_name'] = ['D01: One way Subscribe: Multiple Iterations. Mutate data in publisher. Client aborts. Version is kept']
        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test D01"
        super(test_weave_wdm_next_one_way_subscribe_13, self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":
    WeaveUtilities.run_unittest()


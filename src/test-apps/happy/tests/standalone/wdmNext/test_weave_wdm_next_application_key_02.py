#!/usr/bin/env python

#
#    Copyright (c) 2017 Nest Labs, Inc.
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
#       Calls Weave WDM mutual subscribe between nodes.
#       It uses default group key id 0x00005536 to encrypt the mutual subscription.
#       A02: Mutual Subscribe: Application key: Group key
#       B02: Stress Mutual Subscribe: Application key: Group key
#

import os
import unittest
from weave_wdm_next_test_base import weave_wdm_next_test_base


class test_weave_wdm_next_application_key_02(weave_wdm_next_test_base):

    def test_weave_wdm_next_application_key_02(self):
        wdm_next_args = {}
        wdm_next_args['wdm_option'] = "mutual_subscribe"
        wdm_next_args['total_client_count'] = 2
        wdm_next_args['final_client_status'] = 0
        wdm_next_args['timer_client_period'] = 5000
        wdm_next_args['test_client_iterations'] = 5
        wdm_next_args['test_client_delay'] = 2000
        wdm_next_args['enable_client_flip'] = 1

        wdm_next_args['total_server_count'] = 2
        wdm_next_args['final_server_status'] = 4
        wdm_next_args['timer_server_period'] = 4000
        wdm_next_args['enable_server_flip'] = 1

        wdm_next_args['group_enc'] = True
        wdm_next_args['group_enc_key_id'] = "0x00005536"

        wdm_next_args['client_clear_state_between_iterations'] = True
        wdm_next_args['server_clear_state_between_iterations'] = True

        wdm_next_args['client_log_check'] = [('Message Encryption Key', 1),
                                             ('Handler\[0\] \[(ALIVE|CONFM)\] bound mutual subscription is going away', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', wdm_next_args['test_client_iterations'] * (wdm_next_args['total_server_count'] + 1)),
                                             ('Client\[0\] \[(ALIVE|CONFM)\] EndSubscription Ref\(1\)', wdm_next_args['test_client_iterations']),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]
        wdm_next_args['server_log_check'] = [('Handler\[0\] \[(ALIVE|CONFM)\] bound mutual subscription is going away', wdm_next_args['test_client_iterations']),
                                             ('Client->kEvent_OnNotificationProcessed', wdm_next_args['test_client_iterations'] * (wdm_next_args['total_client_count'] + 1)),
                                             ('Client\[0\] \[(ALIVE|CONFM)\] CancelRequestHandler Ref\(1\)', wdm_next_args['test_client_iterations']),
                                             ('Client\[0\] moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations']),
                                             ('Handler\[0\] Moving to \[ FREE\] Ref\(0\)', wdm_next_args['test_client_iterations'])]
        wdm_next_args['test_tag'] = self.__class__.__name__[35:].upper()
        wdm_next_args['test_case_name'] = ['B02: Stress Mutual Subscribe: Application key: Group key']
        print 'test file: ' + self.__class__.__name__
        print "test_weave_wdm_next_application_key test A02 and B02"
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print "it does not support wdm with group key encryption under Lwip:"
            return
        super(test_weave_wdm_next_application_key_02, self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":
    unittest.main()

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
import WeaveWdmNextOptions as wwno


class test_weave_wdm_next_application_key_02(weave_wdm_next_test_base):

    def test_weave_wdm_next_application_key_02(self):
        print 'test file: ' + self.__class__.__name__
        print "test_weave_wdm_next_application_key test A02 and B02"
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ[
                "WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            print "it does not support wdm with group key encryption under Lwip:"
            return
        wdm_next_args = self.get_test_param_json(self.__class__.__name__)
        super(test_weave_wdm_next_application_key_02, self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":
    unittest.main()

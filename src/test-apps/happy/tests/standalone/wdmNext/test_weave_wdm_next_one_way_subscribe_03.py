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
#       C01: One way Subscribe: Root path. Null Version. Idle. Client cancels
#       L03: Stress One way Subscribe: Root path. Null Version. Idle. Client cancels
#

import unittest
import set_test_path
from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_03(weave_wdm_next_test_base):

    def test_weave_wdm_next_one_way_subscribe_03(self):
        print 'test file: ' + self.__class__.__name__
        print "weave-wdm-next test C01 and L03"
        wdm_next_args = self.get_test_param_json(self.__class__.__name__)
        super(test_weave_wdm_next_one_way_subscribe_03,
              self).weave_wdm_next_test_base(wdm_next_args)


if __name__ == "__main__":
    WeaveUtilities.run_unittest()

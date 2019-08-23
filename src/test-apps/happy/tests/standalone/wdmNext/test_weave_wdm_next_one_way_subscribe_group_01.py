#!/usr/bin/env python

"""
Runs wdm one way subscribe root path null version client action.

1. L01: Stress One way Subscribe: Root path. Null Version. Client cancels
2. L02: Stress One way Subscribe: Root path. Null Version. Client aborts
3. L03: Stress One way Subscribe: Root path. Null Version. Idle. Client cancels
4. L04: Stress One way Subscribe: Root path. Null Version. Idle. Client aborts
5. L05: Stress One way Subscribe: Root path, Null Version. Mutate data in Publisher. Client cancels
6. L06: Stress One way Subscribe: Root path. Null Version. Mutate data in Publisher. Client aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_01(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_one_way_subscribe_01",
            "test_weave_wdm_next_one_way_subscribe_02",
            "test_weave_wdm_next_one_way_subscribe_03",
            "test_weave_wdm_next_one_way_subscribe_04",
            "test_weave_wdm_next_one_way_subscribe_05",
            "test_weave_wdm_next_one_way_subscribe_06"]:
        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_01, test_name, test)

    WeaveUtilities.run_unittest()

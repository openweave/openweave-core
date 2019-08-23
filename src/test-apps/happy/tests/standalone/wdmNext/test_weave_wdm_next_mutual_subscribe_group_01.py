#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version, initiator action.

1. M01: Stress Mutual Subscribe: Root path. Null Version. Client in initiator cancels
2. M02: Stress Mutual Subscribe: Root path. Null Version. Publisher in initiator cancels
3. M03: Stress Mutual Subscribe: Root path. Null Version. Client in initiator aborts
4. M04: Stress Mutual Subscribe: Root path. Null Version. Publisher in initiator aborts
5. M05: Stress Mutual Subscribe: Root path. Null Version. Idle, Client in initiator cancels
6. M06: Stress Mutual Subscribe: Root path. Null Version. Idle. Publisher in initiator cancels
7. M07: Stress Mutual Susbscribe: Root path. Null Version. Idle. Client in initiator aborts
8. M08: Stress Mutual Subscribe: Root path. Null Version. Idle. Publisher in initiator aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_01(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_01",
            "test_weave_wdm_next_mutual_subscribe_02",
            "test_weave_wdm_next_mutual_subscribe_03",
            "test_weave_wdm_next_mutual_subscribe_04",
            "test_weave_wdm_next_mutual_subscribe_05",
            "test_weave_wdm_next_mutual_subscribe_06",
            "test_weave_wdm_next_mutual_subscribe_07",
            "test_weave_wdm_next_mutual_subscribe_08"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_01, test_name, test)

    WeaveUtilities.run_unittest()

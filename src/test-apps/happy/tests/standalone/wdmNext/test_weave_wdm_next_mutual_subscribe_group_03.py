#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version mutate data in initiator AND responder,
initator action.

17. M17: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Client in initiator cancels
18. M18: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Publisher in initiator cancels
19. M19: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Client in initiator aborts
20. M20: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Publisher in initiator aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_03(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_17",
            "test_weave_wdm_next_mutual_subscribe_18",
            "test_weave_wdm_next_mutual_subscribe_19",
            "test_weave_wdm_next_mutual_subscribe_20"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_03, test_name, test)

    WeaveUtilities.run_unittest()

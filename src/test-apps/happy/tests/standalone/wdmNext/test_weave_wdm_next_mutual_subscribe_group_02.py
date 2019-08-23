#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version mutate data in initiator or responder,
initator action.

9. M09: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Client in initiator cancels
10. M10: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Publisher in initiator cancels
11. M11: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Client in initiator aborts
12. M12: Stress Mutual Subscribe: Root path. Null Version. Mutate data in initiator.Publisher in initiator aborts
13. M13: Stress Mutual Subscribe: Root path. Null Version. Mutate data in responder. Client in initiator cancels
14. M14: Stress Mutual Subscribe: Root path, Null Version, Notification in responder, Publisher in initiator Cancel
15. M15: Stress Mutual Subscribe: Root path. Null Version. Notification in responder. Client in initiator aborts
16. M16: Stress Mutual Subscribe: Root path. Null Version. Mutate data in responder. Publisher in initiator aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_02(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_09",
            "test_weave_wdm_next_mutual_subscribe_10",
            "test_weave_wdm_next_mutual_subscribe_11",
            "test_weave_wdm_next_mutual_subscribe_12",
            "test_weave_wdm_next_mutual_subscribe_13",
            "test_weave_wdm_next_mutual_subscribe_14",
            "test_weave_wdm_next_mutual_subscribe_15",
            "test_weave_wdm_next_mutual_subscribe_16"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_02, test_name, test)

    WeaveUtilities.run_unittest()

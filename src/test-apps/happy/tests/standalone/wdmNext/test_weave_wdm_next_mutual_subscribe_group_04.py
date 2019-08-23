#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version, responder action.

21. E05: Mutual Subscribe: Root path. Null Version. Client in responder cancels
22. E06: Mutual Subscribe: Root path. Null Version. Publisher in responder cancels
23. E07: Mutual Subscribe: Root path. Null Version. Client in responder aborts
24. E08: Mutual Subscribe: Root path. Null Version. Publisher in responder aborts
25. F17: Mutual Subscribe: Root path. Null Version. Idle. Client in responder cancels
26. F18: Mutual Subscribe: Root path. Null Version. Idle. Publisher in responder cancels
27. F19: Mutual Subscribe: Root path. Null Version. Idle. Client in responder aborts
28. F20: Mutual Subscribe: Root path. Null Version. Idle. Publisher in responder aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_04(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_21",
            "test_weave_wdm_next_mutual_subscribe_22",
            "test_weave_wdm_next_mutual_subscribe_23",
            "test_weave_wdm_next_mutual_subscribe_24",
            "test_weave_wdm_next_mutual_subscribe_25",
            "test_weave_wdm_next_mutual_subscribe_26",
            "test_weave_wdm_next_mutual_subscribe_27",
            "test_weave_wdm_next_mutual_subscribe_28"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_04, test_name, test)

    WeaveUtilities.run_unittest()

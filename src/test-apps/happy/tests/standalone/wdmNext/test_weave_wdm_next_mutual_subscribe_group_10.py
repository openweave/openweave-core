#!/usr/bin/env python

"""
Tests wdm mutual subscribe initiator and responder continuous events, mutate data in initiator and responder,
initiator action.

60. M41: Initiator and Responder Continuous Events. Mutate data in initiator responder. Client in initiator cancels
61. M42: Stress Mutual Subscribe: Initiator and Responder Continuous Events. Mutate data in initiator and responder. Publisher in initiator cancels
62. M43: Stress Mutual Subscribe: Initiator and Responder Continuous Events. Mutate data in initiator and responder. Client in initiator aborts
63. M44: Stress Mutual Subscribe: Initiator and Responder Continuous Events. Mutate data in initiator and responder. Publisher in initiator abort
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_10(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_60",
            "test_weave_wdm_next_mutual_subscribe_61",
            "test_weave_wdm_next_mutual_subscribe_62",
            "test_weave_wdm_next_mutual_subscribe_63"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_10, test_name, test)

    WeaveUtilities.run_unittest()

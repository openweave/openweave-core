#!/usr/bin/env python

"""
Tests wdm mutual subscribe initiator or responder continuous events, initiator action.

52. M33: Stress Mutual Subscribe: Initiator Continuous Events. Client in initiator cancels
53. M34: Stress Mutual Subscribe: Initiator Continuous Events. Publisher in Initiator cancels
54. M35: Stress Mutual Subscribe: Initiator Continuous Events. Client in initiator aborts
55. M36: Stress Mutual Subscribe: Initiator Continuous Events. Publisher in initiator aborts
56. M37: Stress Mutual Subscribe: Responder Continuous Events. Client in initiator cancels
57. M38: Stress Mutual Subscribe: Responder Continuous Events. Publisher in Initiator cancels
58. M39: Stress Mutual Subscribe: Responder Continuous Events. Client in initiator aborts
59. M40: Stress Mutual Subscribe: Responder Continuous Events. Publisher in initiator aborts

"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_09(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_52",
            "test_weave_wdm_next_mutual_subscribe_53",
            "test_weave_wdm_next_mutual_subscribe_54",
            "test_weave_wdm_next_mutual_subscribe_55",
            "test_weave_wdm_next_mutual_subscribe_56",
            "test_weave_wdm_next_mutual_subscribe_57",
            "test_weave_wdm_next_mutual_subscribe_58",
            "test_weave_wdm_next_mutual_subscribe_59"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_09, test_name, test)

    WeaveUtilities.run_unittest()

#!/usr/bin/env python

"""
Tests wdm mutual subscribe initiator or respoonder continuous events mutate data in initiator or responder,
initiator action.

44. M25: Stress Mutual Subscribe: Initiator Continuous Events. Mutate data in initiator. Client in initiator cancels
45. M26: Stress Mutual Subscribe: Initiator Continuous Events. Mutate data in initiator. Publisher in Initiator cancels
46. M27: Stress Mutual Subscribe: Initiator Continuous Events. Mutate data in initiator. Client in initiator aborts
47. M28: Stress Mutual Subscribe: Initiator Continuous Events. Mutate data in initiator. Publisher in initiator aborts
48. M29: Stress Mutual Subscribe: Responder Continuous Events. Mutate data in responder. Client in initiator cancels
49. M30: Stress Mutual Subscribe: Responder Continuous Events. Mutate data in responder. Publisher in Initiator cancels
50. M31: Stress Mutual Subscribe: Responder Continuous Events. Mutate data in responder. Client in initiator aborts
51. M32: Stress Mutual Subscribe: Responder Continuous Events. Mutate data in responder. Publisher in initiator aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_08(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_44",
            "test_weave_wdm_next_mutual_subscribe_45",
            "test_weave_wdm_next_mutual_subscribe_46",
            "test_weave_wdm_next_mutual_subscribe_47",
            "test_weave_wdm_next_mutual_subscribe_48",
            "test_weave_wdm_next_mutual_subscribe_49",
            "test_weave_wdm_next_mutual_subscribe_50",
            "test_weave_wdm_next_mutual_subscribe_51"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_08, test_name, test)

    WeaveUtilities.run_unittest()

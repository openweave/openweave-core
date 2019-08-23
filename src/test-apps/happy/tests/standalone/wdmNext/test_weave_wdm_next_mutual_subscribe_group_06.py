#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version mutate data in initiator AND responder,
responder action.

37. F29: Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Client in responder cancels
38. F30: Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Publisher in responder cancels
39. F31: Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Client in responder aborts
40. F32: Mutual Subscribe: Root path. Null Version. Mutate data in initiator and responder. Publisher in responder aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_06(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_37",
            "test_weave_wdm_next_mutual_subscribe_38",
            "test_weave_wdm_next_mutual_subscribe_39",
            "test_weave_wdm_next_mutual_subscribe_40"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_06, test_name, test)

    WeaveUtilities.run_unittest()

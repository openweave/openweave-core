#!/usr/bin/env python

"""
Tests wdm mutual subscribe root path null version mutate data in initiator or responder,
responder action.

29. F21: Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Client in responder cancels"
30. F22: Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Publisher in responder cancels
31. F23: Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Client in responder aborts
32. "F24: Mutual Subscribe: Root path. Null Version. Mutate data in initiator. Publisher in responder Abort
33. F25: Mutual Subscribe: Root path. Null Version. Mutate data in responder. Client in responder cancels
34. F26: Mutual Subscribe: Root path. Null Version. Mutate data in responder. Publisher in responder cancels"
35. F27: Mutual Subscribe: Root path. Null Version. Mutate data in responder. Client in responder aborts"
36. F28: Mutual Subscribe: Root path. Null Version. Mutate data in responder. Publisher in responder aborts"
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_05(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_29",
            "test_weave_wdm_next_mutual_subscribe_30",
            "test_weave_wdm_next_mutual_subscribe_31",
            "test_weave_wdm_next_mutual_subscribe_32",
            "test_weave_wdm_next_mutual_subscribe_33",
            "test_weave_wdm_next_mutual_subscribe_34",
            "test_weave_wdm_next_mutual_subscribe_35",
            "test_weave_wdm_next_mutual_subscribe_36"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_05, test_name, test)

    WeaveUtilities.run_unittest()

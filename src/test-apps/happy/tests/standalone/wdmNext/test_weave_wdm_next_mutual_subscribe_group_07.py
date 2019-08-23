#!/usr/bin/env python

"""
Tests wdm mutual subscribe multiple iterations root path null version mutate data in initiator or responder,
initiator action, version is kept.

41. G01: Mutual Subscribe: Multiple Iterations. Mutate data in responder. Client in initiator aborts. Version is kept.
42. G02: Mutual Subscribe: Multiple Iterations. Mutate data in initiator. Client in initiator aborts. Version is kept."
43. G03: Mutual Subscribe: Multiple Iterations. Mutate data in initiator and responder. Client in initiator aborts. Version is kept
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_07(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_41",
            "test_weave_wdm_next_mutual_subscribe_42",
            "test_weave_wdm_next_mutual_subscribe_43"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_07, test_name, test)

    WeaveUtilities.run_unittest()

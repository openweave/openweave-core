#!/usr/bin/env python

"""
Runs tests wdm one way subscribe continuous events.

13. D01: One way Subscribe: Multiple Iterations. Mutate data in publisher. Client aborts. Version is kept
14. L07: Stress One way Subscribe: Publisher Continuous Events. Publisher mutates trait data. Client cancels
15. L08: Stress One way Subscribe: Publisher Continuous Events. Publisher mutates trait data. Client aborts
16. L09: Stress One way Subscribe: Publisher Continuous Events. Client cancels
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_03(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_one_way_subscribe_13",
            "test_weave_wdm_next_one_way_subscribe_14",
            "test_weave_wdm_next_one_way_subscribe_15",
            "test_weave_wdm_next_one_way_subscribe_16"]:
        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_03, test_name, test)

    WeaveUtilities.run_unittest()

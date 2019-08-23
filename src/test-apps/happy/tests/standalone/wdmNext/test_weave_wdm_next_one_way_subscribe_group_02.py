#!/usr/bin/env python

"""
Runs wdm one way subscribe root path null version publisher action.

7. B03: One way Subscribe: Root path. Null Version. Publisher cancels
8. B04: One way Subscribe: Root path. Null Version. Publisher aborts
9. C05: One way Subscribe: Root path. Null Version. Idle. Publisher cancels
10. C06: One way Subscribe: Root path, Null Version, Idle, Publisher Abort"
11. C07: One way Subscribe: Root path. Null Version. Mutate data in publisher. Publisher cancels
12. C08: One way Subscribe: Root path. Null Version. Mutate data in publisher. Publisher aborts
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_02(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_one_way_subscribe_07",
            "test_weave_wdm_next_one_way_subscribe_08",
            "test_weave_wdm_next_one_way_subscribe_09",
            "test_weave_wdm_next_one_way_subscribe_10",
            "test_weave_wdm_next_one_way_subscribe_11",
            "test_weave_wdm_next_one_way_subscribe_12"]:
        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_02, test_name, test)

    WeaveUtilities.run_unittest()

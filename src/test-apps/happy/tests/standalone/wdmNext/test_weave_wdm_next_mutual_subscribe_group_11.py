#!/usr/bin/env python

"""
Tests wdm mutual subscribe oversize traits.

64. N01: Stress Mutual Subscribe: Oversize trait exists in the end of source trait list at initiator and in the beginning of source trait list at responder.
65. N02: Stress Mutual Subscribe: Oversize trait exists in the end of source trait list at initiator and in the middle of source trait list at responder."
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_one_way_subscribe_group_11(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_mutual_subscribe_64",
            "test_weave_wdm_next_mutual_subscribe_65"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_one_way_subscribe_group_11, test_name, test)

    WeaveUtilities.run_unittest()

#!/usr/bin/env python

"""
Tests wdm resubscribe on error.

1. N03: One way Subscribe: Root path, Null Version, Resubscribe On Error
2. N04: Mutual Subscribe: Root path, Null Version, Resubscribe On Error"
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_resub_group(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_oneway_resub",
            "test_weave_wdm_next_mutual_resub"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_resub_group, test_name, test)

    WeaveUtilities.run_unittest()

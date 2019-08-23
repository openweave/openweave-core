#!/usr/bin/env python

"""
Tests wdm client updates.

1. Update 01: Client creates mutual subscription, sends conditional update request to publisher, and receives notification and status report
2. Update 02: Client creates mutual subscription, sends unconditional update request to publisher, and receives notification and status report
3. Update 03: Client sends standalone unconditional update request to publisher, and receives status report
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_update_group(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_update_01",
            "test_weave_wdm_next_update_02",
            "test_weave_wdm_next_update_03"]:
        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_update_group, test_name, test)

    WeaveUtilities.run_unittest()

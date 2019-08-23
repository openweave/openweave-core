#!/usr/bin/env python

"""
Tests wdm application key

1. B01: Stress Mutual Subscribe: Application key: Key distribution
2. B02: Stress Mutual Subscribe: Application key: Group key
"""

from weave_wdm_next_test_base import weave_wdm_next_test_base
import WeaveUtilities


class test_weave_wdm_next_application_key_group(weave_wdm_next_test_base):
    pass

if __name__ == "__main__":
    for test_name in [
            "test_weave_wdm_next_application_key_01",
            "test_weave_wdm_next_application_key_02"]:

        test = weave_wdm_next_test_base.generate_test(
            weave_wdm_next_test_base.get_test_param_json(test_name))
        setattr(test_weave_wdm_next_application_key_group, test_name, test)

    WeaveUtilities.run_unittest()

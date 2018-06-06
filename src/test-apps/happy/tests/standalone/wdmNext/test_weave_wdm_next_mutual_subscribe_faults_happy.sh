#!/bin/bash

# The fault injection script for mutual subscription has some extra checks on the happy sequence;
# it enforces that the happy sequence indeed covers all the fault points that it is meant to exercise.
# This test runs that script without injecting faults, so we can catch if the happy sequence changes,
# or if any new faults need to be excluded from it (without running the full fault-injection test suite).
${abs_srcdir}/happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults.py --nofaults
exit $?

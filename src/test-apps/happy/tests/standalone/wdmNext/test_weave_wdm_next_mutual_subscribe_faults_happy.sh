#!/bin/bash

#
#    Copyright (c) 2018-2019 Google, LLC.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

# The fault injection script for mutual subscription has some extra checks on the happy sequence;
# it enforces that the happy sequence indeed covers all the fault points that it is meant to exercise.
# This test runs that script without injecting faults, so we can catch if the happy sequence changes,
# or if any new faults need to be excluded from it (without running the full fault-injection test suite).
${abs_srcdir}/happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults.py --nofaults
exit $?

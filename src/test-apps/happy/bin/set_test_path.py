#!/usr/bin/env python

#
#    Copyright (c) 2017 Nest Labs, Inc.
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


import os
import sys

#
# This module is imported by test scripts in the following directories:
#   - happy-tests
#   - happy-service-tests
#
# os.path.abspath(__file__) will return the path of the file that imports
# this module, so the path to "src/test-apps/happy" is derived relative
# to the above test scripts.
#

wrapper_home = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "happy"))

wrapper_dirs = [ ".",
                 wrapper_home,
                 wrapper_home + "/plugin",
                 wrapper_home + "/bin" ]

for wrapper_dir in wrapper_dirs:
    if os.path.exists(wrapper_dir):
        sys.path.append(wrapper_dir)

#!/bin/sh

#
#    Copyright 2019 Google LLC All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      Travis CI build script for nRF52840 integration builds.
#

# Export GNU_VERSION variable.
export GNU_VERSION=7.3.1

# Add Nordic nRF jprog tool to the path.
export PATH=${PATH}:${NRF5_TOOLS_ROOT}/nrfjprog

# Set OPENWEAVE_ROOT to the Travis build directory. This will result in the example app being
# built using the target OpenWeave commit.
export OPENWEAVE_ROOT=${TRAVIS_BUILD_DIR}

# Build the example application.
make -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example || exit 1

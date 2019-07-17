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
#      Travis CI build preparation script for the nRF52840 target on Linux.
#

TMPDIR=${TMPDIR-/tmp}

set -x

# Clone the openweave-nrf52840-lock-example application.  This code will be used to
# test the ability to build OpenWeave for the nRF52840.
#
git -C ${TRAVIS_BUILD_DIR} clone https://github.com/openweave/openweave-nrf52840-lock-example.git || exit 1

# Test if a parallel branch exists in the example repo with the same name
# as the OpenWeave branch being built.  If so, build using that branch of
# the example app.
#
SOURCE_BRANCH=${TRAVIS_PULL_REQUEST_BRANCH}
if [ -z "${SOURCE_BRANCH}" ]; then
    SOURCE_BRANCH=${TRAVIS_BRANCH}
fi
if git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example rev-parse --verify origin/${SOURCE_BRANCH} >/dev/null 2>&1; then
    git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example checkout ${SOURCE_BRANCH}
fi
EXAMPLE_APP_BRANCH=`git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example rev-parse --abbrev-ref HEAD`

# Call the prepare script in the lock example repo to install related
# dependencies.
source ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example/.travis/prepare.sh

# Initialize and update all submodules within the example app EXCEPT the
# OpenWeave submodule.
#
git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example submodule init || exit 1
git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example submodule deinit third_party/openweave-core || exit 1
git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example submodule update || exit 1

set +x

# Log relevant build information.
#
echo '---------------------------------------------------------------------------'
echo 'nRF52840 Build Preparation Complete'
echo ''
echo "openweave-core branch: ${TRAVIS_BRANCH}"
echo "openweave-nrf52840-lock-example branch: ${EXAMPLE_APP_BRANCH}"
echo "Nordic SDK for Thread and Zigbee: ${NORDIC_SDK_URL}"
echo "Nordic nRF5x Command Line Tools: ${NORDIC_COMMAND_LINE_TOOLS_URL}"
echo "ARM GCC Toolchain: ${ARM_GCC_TOOLCHAIN_URL}"
echo 'Commit Hashes'
echo '  openweave-core: '`git -C ${TRAVIS_BUILD_DIR} rev-parse --short HEAD`
echo '  openweave-nrf52840-lock-example: '`git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example rev-parse --short HEAD`
git -C ${TRAVIS_BUILD_DIR}/openweave-nrf52840-lock-example submodule --quiet foreach 'echo "  openweave-nrf52840-lock-example/$path: "`git rev-parse --short HEAD`'
echo '---------------------------------------------------------------------------'

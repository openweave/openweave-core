#!/bin/sh

#
#    Copyright 2018 Google LLC All Rights Reserved.
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
#      Travis CI build preparation script for the ESP32 target on Linux.
#

TMPDIR=${TMPDIR-/tmp}

# Set tools download links
#
XTENSA_TOOL_CHAIN_URL=https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
ESP_IDF_VERSION=v3.0.4

# --------------------------------------------------------------------------------

set -x

# Install dependent packages needed by the ESP32 development environment.
#
sudo apt-get update
sudo apt-get -y install libncurses-dev flex bison gperf

# Install ESP32 toolchain for linux
#
wget -O ${TMPDIR}/xtensa-tool-chain.tar.gz -nv ${XTENSA_TOOL_CHAIN_URL} || exit 1
tar -C ${TRAVIS_BUILD_DIR} -xzf ${TMPDIR}/xtensa-tool-chain.tar.gz || exit 1
rm ${TMPDIR}/xtensa-tool-chain.tar.gz

# Install the ESP32 development environment.
#
git -C ${TRAVIS_BUILD_DIR} clone --depth 1 https://github.com/espressif/esp-idf.git -b ${ESP_IDF_VERSION} || exit 1
git -C ${TRAVIS_BUILD_DIR}/esp-idf checkout ${ESP_IDF_VERSION} || exit 1
git -C ${TRAVIS_BUILD_DIR}/esp-idf submodule update --init --recursive || exit 1

# Clone the openweave-esp32-demo application.  This code will be used to
# test the ability to build OpenWeave for the ESP32.
#
git -C ${TRAVIS_BUILD_DIR} clone https://github.com/openweave/openweave-esp32-demo.git || exit 1

# Test if a parallel branch exists in the demo app repo with the same name
# as the OpenWeave branch being built.  If so, build using that branch of
# the demo app.
#
SOURCE_BRANCH=${TRAVIS_PULL_REQUEST_BRANCH}
if [ -z "${SOURCE_BRANCH}" ]; then
    SOURCE_BRANCH=${TRAVIS_BRANCH}
fi
if git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo rev-parse --verify origin/${SOURCE_BRANCH} >/dev/null 2>&1; then
    git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo checkout ${SOURCE_BRANCH}
fi
DEMO_APP_BRANCH=`git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo rev-parse --abbrev-ref HEAD`

# Initialize and update all submodules within the demo app EXCEPT the
# OpenWeave submodule.
#
git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo submodule init || exit 1
git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo submodule deinit third_party/openweave || exit 1
git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo submodule update || exit 1

# Create a symbolic link from openweave-esp32-demo/third_party/openweave to
# the OpenWeave source directory.  This will result in the demo app being
# built using the target OpenWeave commit.
#
rmdir ${TRAVIS_BUILD_DIR}/openweave-esp32-demo/third_party/openweave || exit 1
ln -s ${TRAVIS_BUILD_DIR} ${TRAVIS_BUILD_DIR}/openweave-esp32-demo/third_party/openweave || exit 1

set +x

# Log relevant build information.
#
echo '---------------------------------------------------------------------------'
echo 'ESP32 Build Preparation Complete'
echo ''
echo "openweave-core branch: ${TRAVIS_BRANCH}"
echo "openweave-esp32-demo branch: ${DEMO_APP_BRANCH}"
echo "ESP-IDF version: ${ESP_IDF_VERSION}"
echo "Xtensa Tool Chain: ${XTENSA_TOOL_CHAIN_URL}"
echo 'Commit Hashes'
echo '  openweave-core: '`git -C ${TRAVIS_BUILD_DIR} rev-parse --short HEAD`
echo '  openweave-esp32-demo: '`git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo rev-parse --short HEAD`
git -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo submodule --quiet foreach 'echo "  openweave-esp32-demo/$path: "`git rev-parse --short HEAD`'
echo '---------------------------------------------------------------------------'

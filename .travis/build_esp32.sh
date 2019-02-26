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
#      Travis CI build script for ESP32 integration builds.
#

# Add the xtensa tool chain to the path.
export PATH=$PATH:${TRAVIS_BUILD_DIR}/xtensa-esp32-elf/bin

# Export IDF_PATH variable pointing to the ESP32 development environment.  
export IDF_PATH=${TRAVIS_BUILD_DIR}/esp-idf

# Set defaults for all configuration options in the demo application.
make -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo defconfig || exit 1

# Build the demo application.
make -C ${TRAVIS_BUILD_DIR}/openweave-esp32-demo || exit 1

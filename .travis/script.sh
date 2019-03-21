#!/bin/sh

#
#    Copyright 2018-2019 Google LLC All Rights Reserved.
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
#      This file is the script for Travis CI hosted, distributed continuous 
#      integration 'script' step.
#

die()
{
    echo " *** ERROR: " ${*}
    exit 1
}

###############################################################
# Function gcc_check_happy()
# Build for linux-auto or linux-lwip, and run tests using happy
# Arguments:
#     ${BUILD_TARGET}
# Returns:
#     None
###############################################################
gcc_check_happy()
{
    cmd_start="sudo make -f Makefile-Standalone DEBUG=1 TIMESTAMP=1 COVERAGE=1 "
    cmd_end="BuildJobs=24 check"

    if [ "lwip" in "${BUILD_TARGET}" ];then
        build_cmd="$cmd_start USE_LWIP=1 $cmd_end"
        build_folder="x86_64-unknown-linux-gnu-lwip"
    else
        build_cmd="$cmd_start $cmd_end"
        build_folder="x86_64-unknown-linux-gnu"
    fi

    mkdir -p $TRAVIS_BUILD_DIR/happy-test-logs/$1/from-tmp
    eval $build_cmd
    cp $TRAVIS_BUILD_DIR/build/$build_folder/src/test-apps/happy $TRAVIS_BUILD_DIR/happy-test-logs/$1 -rf
    cp /tmp/happy* $TRAVIS_BUILD_DIR/happy-test-logs/$1/from-tmp
    echo "please check happy-test-log/<UTC time> under link: https://storage.cloud.google.com/openweave"
}

case "${BUILD_TARGET}" in

    linux-auto-clang)
        ./configure && make && make check
        ;;

    linux-auto-gcc-check)
        ./configure --enable-coverage && make && make check
        ;;

    linux-auto-gcc-check-happy)
        gcc_check_happy ${BUILD_TARGET}
        ;;

    linux-lwip-gcc-check-happy)
        gcc_check_happy ${BUILD_TARGET}
        ;;

    linux-lwip-clang)
        ./configure --with-target-network=lwip --with-lwip=internal --disable-java && make
        ;;

    linux-lwip-gcc-check)
        # Note, LwIP requires sudo prior to running 'make check' to ensure the appropriate TUN and bridge interfaces
        # may be created.
        ./configure --enable-coverage --with-target-network=lwip --with-lwip=internal --disable-java && make && sudo make check
        ;;

    osx-auto-clang)
        ./configure && make
        ;;

    osx-lwip-clang)
        ./configure --with-target-network=lwip --with-lwip=internal --disable-java && make
        ;;

    esp32)
        .travis/build_esp32.sh
        ;;

    linux-auto-*-distcheck)
        ./configure && make distcheck
        ;;

    linux-auto-*-lint)
        ./configure && make pretty-check
        ;;

    *)
        die "Unknown build target \"${BUILD_TARGET}\"."
        ;;
        
esac

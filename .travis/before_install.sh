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
#      This file is the script for Travis CI hosted, distributed continuous 
#      integration 'before_install' trigger of the 'install' step.
#

die()
{
    echo " *** ERROR: " ${*}
    exit 1
}

echo "echo before install"
echo $HOME
echo $TRAVIS_BUILD_DIR
pwd

# Package build machine OS-specific configuration and setup

case "${TRAVIS_OS_NAME}" in

    linux)
        # By default, Travis CI does not have IPv6 enabled on
        # Linux. Ensure that IPv6 is enabled since Weave, and its unit
        # and functional tests, depend on working IPv6 support.

        sudo sysctl -w net.ipv6.conf.all.disable_ipv6=0

        ;;

esac

# Package target-specific configuration and setup

case "${BUILD_TARGET}" in

    linux-auto-*)
        # By default, the BLE layer is enabled in OpenWeave Core and, by
        # default on Linux, the BLE layer is implemented by BlueZ. On Linux,
        # BlueZ requires:
        #
        #   * libdbus-1-dev
        #   * libical-dev
        #   * libudev-dev
        #   * systemd

        sudo apt-get update
        sudo apt-get install libdbus-1-dev libudev-dev libical-dev systemd

        # By default, OpenWeave Core uses OpenSSL for cryptography on
        # Linux and the OpenSSL version included in package depends
        # on the perl Text::Template mmodule.

        curl -L https://cpanmin.us | sudo perl - --sudo App::cpanminus
        sudo cpanm "Text::Template"

        ;;

    esp32)
        .travis/prepare_esp32.sh

        ;;

    osx-auto-clang)
        # By default, OpenWeave Core uses OpenSSL for cryptography on
        # OS X and the OpenSSL version included in package depends
        # on the perl Text::Template mmodule.

        curl -L https://cpanmin.us | sudo perl - --sudo App::cpanminus
        sudo cpanm "Text::Template"

        ;;

    happy_test)
        # prepare for happy test run 
        sudo apt-get update
        sudo apt-get install python-setuptools
        sudo apt-get install bridge-utils
        sudo apt-get install swig
        sudo apt-get install lcov

        cd $HOME
        #git clone ssh://git@stash.nestlabs.com:7999/platform/happy.git # or wget
        git clone https://github.com/openweave/happy.git

        mkdir -p ve
        cd ve
        virtualenv happy
        ls ./happy/bin/activate
        . ./happy/bin/activate
        cd ${HOME}/happy
        python pip_packages.py
        python setup.py develop

        # configure happy, $HOME: /home/travis
        # $TRAVIS_BUILD_DIR: /home/travis/build/jenniexie/openweave-core
        cat << EOF >~/.happy_conf.json
        {
            "weave_path": "$TRAVIS_BUILD_DIR/build/x86_64-unknown-linux-gnu/src/test-apps/"
        }
EOF

        sudo ln -s ${HOME}/.happy_conf.json /root/.happy_conf.json
        
    ;;
    *)
        die "Unknown build target \"${BUILD_TARGET}\"."
        ;;
    
esac














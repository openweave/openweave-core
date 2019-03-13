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
#      integration 'before_install' trigger of the 'install' step.
#

die()
{
    echo " *** ERROR: " ${*}
    exit 1
}

#
# installdeps <dependency tag>
#
# Abstraction for handling common dependency fulfillment across
# different, but related, test targets.
#
installdeps()
{
    case "${1}" in

        bluez-deps)
            sudo apt-get update
            sudo apt-get -y install libdbus-1-dev libudev-dev libical-dev systemd
            sudo apt-get -y install libusb-dev libglib2.0-dev libreadline-dev libdbus-glib-1-dev libtool
            sudo pip install dbus-python==1.2.4

            ;;

        happy-deps)
            sudo apt-get update
            sudo apt-get -y install bridge-utils
            sudo apt-get -y install lcov
            sudo apt-get -y install python-lockfile
            sudo apt-get -y install python-psutil
            sudo apt-get -y install python-setuptools
            sudo apt-get -y install swig

            ;;

        openssl-deps)
            # OpenWeave Core may use OpenSSL for cryptography. The
            # OpenSSL version included in package depends on the
            # perl Text::Template mmodule.

            curl -L https://cpanmin.us | sudo perl - --sudo App::cpanminus
            sudo cpanm "Text::Template"

            ;;

    esac
}

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

    esp32)
        .travis/prepare_esp32.sh

        ;;

    osx-auto-clang|osx-lwip-clang)
        # By default, OpenWeave Core uses OpenSSL for cryptography on
        # OS X and the OpenSSL version included in package depends
        # on the perl Text::Template mmodule.
        
        installdeps "openssl-deps"

        ;;

    linux-auto-gcc-check-happy|linux-lwip-gcc-check-happy)
        # By default, the BLE layer is enabled in OpenWeave Core and,
        # by default on Linux, the BLE layer is implemented by
        # BlueZ. Satisfy its dependencies.

        installdeps "bluez-deps"

        # By default, OpenWeave Core uses OpenSSL for cryptography on
        # Linux and the OpenSSL version included in package depends
        # on the perl Text::Template mmodule.

        installdeps "openssl-deps"

        installdeps "happy-deps"

        cd $HOME
        git clone https://github.com/openweave/happy.git
        cd ${HOME}/happy
        make install
        python pip_packages.py
        pip install pexpect
        sudo apt install python-gobject
        sudo apt install python-dbus

        # build bluez
        cd $TRAVIS_BUILD_DIR
        wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.48.tar.gz
        tar xfz $TRAVIS_BUILD_DIR/bluez-5.48.tar.gz
        cd $TRAVIS_BUILD_DIR/bluez-5.48/
        ./configure  --prefix=/usr --mandir=/usr/share/man --sysconfdir=/etc --localstatedir=/var --enable-tools --enable-testing --enable-experimental --with-systemdsystemunitdir=/lib/systemd/system --with-systemduserunitdir=/usr/lib/systemd --enable-deprecated
        make
        sudo make install
        cd $TRAVIS_BUILD_DIR

        # configure happy, $HOME: /home/travis
        # $TRAVIS_BUILD_DIR: /home/travis/build/jenniexie/openweave-core
        cat << EOF >~/.happy_conf.json
        {
            "weave_path": "$TRAVIS_BUILD_DIR/build/x86_64-unknown-linux-gnu/src/test-apps/"
            "happy_log_path": "/tmp/happy_test_log/"
        }
EOF

        sudo ln -s ${HOME}/.happy_conf.json /root/.happy_conf.json
        ;;

    linux-auto-*|linux-lwip-*)
        # By default, the BLE layer is enabled in OpenWeave Core and,
        # by default on Linux, the BLE layer is implemented by
        # BlueZ. Satisfy its dependencies.

        installdeps "bluez-deps"

        # By default, OpenWeave Core uses OpenSSL for cryptography on
        # Linux and the OpenSSL version included in package depends
        # on the perl Text::Template mmodule.

        installdeps "openssl-deps"

        ;;

    *)
        die "Unknown build target \"${BUILD_TARGET}\"."

        ;;
esac

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

NORDIC_SDK_FOR_THREAD_URL=https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5-SDK-for-Thread/nRF5-SDK-for-Thread-and-Zigbee/nRF5SDKforThreadandZigbee20029775ac.zip

NORDIC_COMMAND_LINE_TOOLS_URL=https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF5-command-line-tools/sw/nRF-Command-Line-Tools_9_8_1_Linux-x86_64.tar

ARM_GCC_TOOLCHAIN_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2

TMPDIR=${TMPDIR-/tmp}

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

        nrf5-sdk)
            # Install Nordic nRF52840 SDK for Thread and Zigbee
            wget -O ${TMPDIR}/nordic_sdk_for_thread.zip -nv ${NORDIC_SDK_FOR_THREAD_URL} || exit 1
            unzip -d ${TRAVIS_BUILD_DIR}/nRF5x-SDK-for-Thread-and-Zigbee -q ${TMPDIR}/nordic_sdk_for_thread.zip || exit 1
            rm ${TMPDIR}/nordic_sdk_for_thread.zip
            
            ;;
        
        nrf5-tools)
            # Install Nordic nRF5x Command Line Tools
            wget -O ${TMPDIR}/nordic_command_line_tools.tar -nv ${NORDIC_COMMAND_LINE_TOOLS_URL} || exit 1
            mkdir ${TRAVIS_BUILD_DIR}/nRF5x-Command-Line-Tools
            tar -C ${TRAVIS_BUILD_DIR}/nRF5x-Command-Line-Tools -xf ${TMPDIR}/nordic_command_line_tools.tar || exit 1
            rm ${TMPDIR}/nordic_command_line_tools.tar

            ;;

        arm-gcc)
            # Install ARM GCC Toolchain
            wget -O ${TMPDIR}/arm_gcc_toolchain.tar.bz2 -nv ${ARM_GCC_TOOLCHAIN_URL} || exit 1
            mkdir ${TRAVIS_BUILD_DIR}/arm
            tar -jxf ${TMPDIR}/arm_gcc_toolchain.tar.bz2 --directory ${TRAVIS_BUILD_DIR}/arm || exit 1
            rm ${TMPDIR}/arm_gcc_toolchain.tar.bz2

            ;;

        osx-autotools)
            HOMEBREW_NO_AUTO_UPDATE=1 brew install automake libtool
            ;;
            
        osx-openssl)
            HOMEBREW_NO_AUTO_UPDATE=1 brew install openssl
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

    nrf52840)
        .travis/prepare_nrf52840.sh

        ;;

    osx-*)
        installdeps "osx-autotools"
        installdeps "osx-openssl"
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

        # install lcov
        sudo apt-get update
        sudo apt-get install lcov

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

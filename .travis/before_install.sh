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

    osx-auto-clang)
        ;;

    *)
        die "Unknown build target \"${BUILD_TARGET}\"."
        ;;
	
esac

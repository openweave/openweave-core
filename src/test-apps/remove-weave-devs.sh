#!/bin/bash


#
#    Copyright (c) 2013-2017 Nest Labs, Inc.
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



SCRIPT_NAME=remove-weave-devs.sh
SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd ${SCRIPT_DIR}; pwd`
SYS_NAME=`uname -s`
USER_NAME=`id -nu`
DEV_COUNT=3
DEV_PREFIX="weave-"

function usage() 
{
    cat <<END
Usage:

    ${SCRIPT_NAME} <options>

    -c <num>
    --count <num>
    
        Number of tap devices to remove. Defaults to 3.

    -p <string>
    --prefix <string>
        
        Device name prefix.  Defaults to 'weave-'.

END
}

# Parse options
#
while [ -n "$1" ]; do
    case $1 in
        -c | --count)
            DEV_COUNT=$2
            shift 2
            ;;
        -p | --prefix)
            DEV_PREFIX=$2
            shift 2
            ;;
        -h | --help)
            usage
            exit 1
            ;;
        *)
            echo "${SCRIPT_NAME} : Unrecognized option: $1"
            usage
            exit 1
            ;;
    esac
done

# Force prompt for root password before we begin.
sudo true || exit -1

if [ "${SYS_NAME}" = "Linux" ]; then
    sudo ifconfig lo del fd00:0:1:1::1/64
    sudo ifconfig lo del fd00:0:1:1::2/64
    sudo ifconfig lo del fd00:0:1:1::3/64

    for ((i = 1; i <= ${DEV_COUNT}; i++)); do
        sudo ifconfig ${DEV_PREFIX}dev-${i} down
    done

    sudo ifconfig ${DEV_PREFIX}bridge down

    for ((i = 1; i <= ${DEV_COUNT}; i++)); do
        sudo brctl delif ${DEV_PREFIX}bridge weave-dev-${i}
    done

    sudo brctl delbr ${DEV_PREFIX}bridge

    for ((i = 1; i <= ${DEV_COUNT}; i++)); do
        sudo tunctl -d ${DEV_PREFIX}dev-${i} > /dev/null
    done

    sudo ebtables -F

elif [ "${SYS_NAME}" = "Darwin" ]; then
    sudo ifconfig lo0 inet6 delete fd00:0:1:1::1/64
    sudo ifconfig lo0 inet6 delete fd00:0:1:1::2/64
    sudo ifconfig lo0 inet6 delete fd00:0:1:1::3/64
fi



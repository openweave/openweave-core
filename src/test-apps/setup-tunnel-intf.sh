#!/bin/bash


#
#    Copyright (c) 2014-2017 Nest Labs, Inc.
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



SCRIPT_NAME=setup-tunnel-intf.sh
SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd ${SCRIPT_DIR}; pwd`
SYS_NAME=`uname -s`
USER_NAME=`id -nu`
GRP_NAME=`id -ng`
DEV_PREFIX="weave-tun"
BORDER_GW_LL_ADDR="fe80::1"
ASSIGN_ADDRESSES=true

function usage() 
{
    cat <<END
Usage:

    ${SCRIPT_NAME} <options>
    
    -p <string>
    --prefix <string>
        
        Device name prefix.  Defaults to 'weave-tun'.

    -n
    --no-addresses
    
        Don't assign ip addresses to interfaces.

END
}

# Parse options
#
while [ -n "$1" ]; do
    case $1 in
        -p | --prefix)
            DEV_PREFIX=$2
            shift 2
            ;;
        -n | --no-addresses)
            ASSIGN_ADDRESSES=false
            shift 1
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

# The following works on Linux only...
#
if [ "${SYS_NAME}" = "Linux" ]; then

    # Setup weave tun interfaces.
    #
    # Fabric Id = 1
    # Subnet Id = 1
    echo "Creating tun interface"
    echo "    ${DEV_PREFIX}"
    sudo ip tuntap add dev ${DEV_PREFIX}0 mode tun user ${USER_NAME} group ${GRP_NAME} > /dev/null
    sudo ip -6 addr add ${BORDER_GW_LL_ADDR}/64 dev ${DEV_PREFIX}0 > /dev/null
    echo "Bringing up tun interface"
    echo "    ${DEV_PREFIX}0"
    sudo ip link set ${DEV_PREFIX}0 up 
    
    if ${ASSIGN_ADDRESSES}; then
        #Add a ULA address for it to be selected as source address
        sudo ifconfig lo add fd00:0:1:1::101/64 > /dev/null
    fi

    #Set ipv6 forwarding to TRUE
    sudo sysctl -w net.ipv6.conf.all.forwarding=1 > /dev/null
fi


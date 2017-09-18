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



SCRIPT_NAME=setup-weave-devs.sh
SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd ${SCRIPT_DIR}; pwd`
SYS_NAME=`uname -s`
USER_NAME=`id -nu`
GRP_NAME=`id -ng`
DEV_COUNT=3
DEV_PREFIX="weave-"
RESTRICT=false
ASSIGN_ADDRESSES=true
ADDITIONAL_INTERFACES=

function usage() 
{
    cat <<END
Usage:

    ${SCRIPT_NAME} <options>

    -c <num>
    --count <num>
    
        Number of tap devices to create. Defaults to 3.

    -p <string>
    --prefix <string>
        
        Device name prefix.  Defaults to 'weave-'.

    -i <iface-name>
    --add-interface <iface-name>
        
        Add an existing interface to the bridge.

        WARNING: This will remove any existing IPv4 addresses on the
        specified interface.

    -r
    --restrict
    
        Restrict packet flow between devices such that devices can only
        talk to immediate neighbors.

    -n
    --no-addresses
    
        Don't assign ip addresses to interfaces.

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
        -i | --add-interface)
            ADDITIONAL_INTERFACES="${ADDITIONAL_INTERFACES} $2"
            shift 2
            ;;
        -r | --restrict)
            RESTRICT=true
            shift 1
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

# Add 3 fabric addresses to the loopback device. This allows the host and 2 weave
# devices to talk using sockets.  By convention the host uses node id 3.
#
# Fabric Id = 1
# Subnet = 1
# Node Ids = 1, 2, 3
#
if ${ASSIGN_ADDRESSES}; then
    echo "Adding fabric addresses for sockets-based fabric (fabric id 1, subnet 1)"
    if [ "${SYS_NAME}" = "Linux" ]; then
        sudo ifconfig lo add fd00:0:1:1::1/64
        sudo ifconfig lo add fd00:0:1:1::2/64
        sudo ifconfig lo add fd00:0:1:1::3/64
    elif [ "${SYS_NAME}" = "Darwin" ]; then
        sudo ifconfig lo0 inet6 fd00:0:1:1::1/64
        sudo ifconfig lo0 inet6 fd00:0:1:1::2/64
        sudo ifconfig lo0 inet6 fd00:0:1:1::3/64
    fi
fi

# The following works on Linux only...
#
if [ "${SYS_NAME}" = "Linux" ]; then

    # Setup weave tap devices and bridge.
    #
    echo "Creating tap interfaces"
    for ((i = 1; i <= ${DEV_COUNT}; i++)); do
        echo "    ${DEV_PREFIX}dev-${i}"
        sudo tunctl -u ${USER_NAME} -g ${GRP_NAME} -t ${DEV_PREFIX}dev-${i} > /dev/null
    done

    echo "Creating ${DEV_PREFIX}bridge"
    sudo brctl addbr ${DEV_PREFIX}bridge

    echo "Bringing up tap interfaces"
    for ((i=1; i<=${DEV_COUNT}; i++)); do
        echo "    ${DEV_PREFIX}dev-${i}"
        sudo brctl addif ${DEV_PREFIX}bridge ${DEV_PREFIX}dev-${i}
        sudo ifconfig ${DEV_PREFIX}dev-${i} up
    done

    if [ -n "${ADDITIONAL_INTERFACES}" ]; then
        echo "Bringing up additional interfaces"
        for i in ${ADDITIONAL_INTERFACES}; do
            echo "    ${i}"
            sudo brctl addif ${DEV_PREFIX}bridge ${i}
            sudo ifconfig ${i} 0.0.0.0 promisc up
        done
    fi

    echo "Bringing up ${DEV_PREFIX}bridge"
    sudo ifconfig ${DEV_PREFIX}bridge up

    if ${RESTRICT}; then
        echo "Setting up restricted connectivity"
        j=${DEV_COUNT}
        sudo ebtables -F FORWARD 
        for ((i=1; i<= ${DEV_COUNT}; i++)); do
            echo "    ${DEV_PREFIX}dev-${j} <-> ${DEV_PREFIX}dev-${i}"
            sudo ebtables -A FORWARD -i ${DEV_PREFIX}dev-${j} -o ${DEV_PREFIX}dev-${i} -j ACCEPT
            sudo ebtables -A FORWARD -i ${DEV_PREFIX}dev-${i} -o ${DEV_PREFIX}dev-${j} -j ACCEPT
            j=${i}
        done
        sudo ebtables -A FORWARD -o ${DEV_PREFIX}bridge -j ACCEPT
        sudo ebtables -A FORWARD -i ${DEV_PREFIX}bridge -j ACCEPT
        sudo ebtables -A FORWARD -j DROP
    fi
    
    if ${ASSIGN_ADDRESSES}; then

        # Add a set of fabric addresses to bridge interface. This allows the host to talk IPv6 to devices
        # that are using LwIP on the weave bridge.
        #
        # Fabric Id = 2
        # Subnet Id = 1
        # Node Id = 3,4,5
        #
        echo "Adding fabric address for host machine in LwIP fabric (fabric id 2, subnet 1)"
        sudo ifconfig ${DEV_PREFIX}bridge add FD00:0:2:1::3/64
        sudo ifconfig ${DEV_PREFIX}bridge add FD00:0:2:1::4/64
        sudo ifconfig ${DEV_PREFIX}bridge add FD00:0:2:1::5/64

        # Add a private IPv4 address to bridge interface. This allows the host to talk IPv4 to devices
        # that are using LwIP on the bridge.
        #
        echo "Adding private IPv4 address to ${DEV_PREFIX}bridge interface"
        sudo ifconfig ${DEV_PREFIX}bridge 192.168.234.3/24
    fi

    # Disable firewall for bridge.
    echo "Disabling iptables filewall for ${DEV_PREFIX}bridge interface"
    sudo iptables -I FIREWALL -i ${DEV_PREFIX}bridge -j ACCEPT
fi


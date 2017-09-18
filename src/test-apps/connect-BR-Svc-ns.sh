#!/bin/bash


#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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



SCRIPT_NAME=connect-BR-Svc-ns.sh
SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd ${SCRIPT_DIR}; pwd`
SYS_NAME=`uname -s`
USER_NAME=`id -nu`
NS_BORDER_GW="node0"
NS_SERVICE="node-svc"
NS_THREAD="node2"
NS_THREAD_WPAN="wpan2"
INTFNAME="ns_if"
REMOVEFLAG=false
MOB_REMOTE=true
function usage() 
{
    cat <<END
Usage:

    ${SCRIPT_NAME} <options>

    -b <bg-node-name>
    --bgw <bg-node-name>
        
        border gateway network namespace name.
    -s <svc-node-name>
    --svc <svc-node-name>

        service network namespace name.
    -e <end-node-name>
    --endNode <end-node-name>

        end node network namespace name.
    -w <end-node-wpan-intf>
    --ewpan <end-node-wpan-intf>

        end node wpan interface name(e.g., wpan2).
    -D

        Delete all network namespaces.

END
}

# Parse options
#
while [ -n "$1" ]; do
    case $1 in
        -b | --bgw)
            NS_BORDER_GW=$2
            shift 2
            ;;
        -s | --svc)
            NS_SERVICE=$2
            shift 2
            ;;
        -e | --endNode)
            NS_THREAD=$2
            shift 2
            ;;
        -w | --ewpan)
            NS_THREAD_WPAN=$2
            shift 2
            ;;
        -D | --delete)
            REMOVEFLAG=true
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

    # Setup network namespace and add a virtual interface pair.
    #
    if ${REMOVEFLAG}; then
      for item in $(ip netns list)
        do
          #echo "Removing network namespace ${item}"
          sudo ip netns del ${item}
        done

      sudo killall mock-tunnel-service
      sudo killall mock-weave-bg

    else
        echo "Creating network namespaces ${NS_SERVICE}"
        sudo ip netns add ${NS_SERVICE}

        echo "Creating virtual interface pairs and assigning them to namespaces"
        sudo ip link add name ${INTFNAME}-bg0 type veth peer name ${INTFNAME}-svc0
        sudo ip link set ${INTFNAME}-bg0 netns ${NS_BORDER_GW}
        sudo ip link set ${INTFNAME}-svc0 netns ${NS_SERVICE}
 
        echo "Bring up virtual interfaces within network namespaces"
        sudo ip netns exec ${NS_BORDER_GW} ifconfig ${INTFNAME}-bg0 up
        sudo ip netns exec ${NS_SERVICE} ifconfig ${INTFNAME}-svc0 up

        sudo ip netns exec ${NS_BORDER_GW} ifconfig lo up
        sudo ip netns exec ${NS_SERVICE} ifconfig lo up

        echo "Adding IPv4 addresses to virtual interfaces"
        sudo ip netns exec ${NS_BORDER_GW} ip addr add 10.1.1.101/24 dev ${INTFNAME}-bg0
        sudo ip netns exec ${NS_SERVICE} ip addr add 10.1.1.102/24 dev ${INTFNAME}-svc0

        echo "Adding IPv6 addresses to virtual interfaces"
        sudo ip netns exec ${NS_BORDER_GW} ip -6 addr add fd00:0:1:1::101/64 dev ${INTFNAME}-bg0
        sudo ip netns exec ${NS_SERVICE} ip -6 addr add fd00:0:1:5::102/64 dev ${INTFNAME}-svc0

        echo "Add a fabric default route in ${NS_THREAD} to go to ${NS_BORDER_GW}"
        sudo ip netns exec ${NS_THREAD} ip -6 route add fd00:0:1::/48 dev ${NS_THREAD_WPAN}
        
        echo "Enable forwarding in ${NS_BORDER_GW}"
        sudo ip netns exec ${NS_BORDER_GW} sysctl -w net.ipv6.conf.all.forwarding=1
        #sudo ip netns exec ${NS_SERVICE} sysctl -w net.ipv6.conf.all.forwarding=1
        #sudo ip netns exec ${NS_TH} sysctl -w net.ipv6.conf.all.forwarding=1

        echo "Starting mock-tunnel-service in ${NS_SERVICE}"
        sudo ip netns exec ${NS_SERVICE} /home/${USER_NAME}/weave/build/x86_64-unknown-linux-gnu/src/test-apps/mock-tunnel-service --node-id 102 > /dev/null &

        echo "Starting mock-weave-bg in ${NS_BORDER_GW}"
        sudo ip netns exec ${NS_BORDER_GW} /home/${USER_NAME}/weave/build/x86_64-unknown-linux-gnu/src/test-apps/mock-weave-bg --node-id 101 --connect-to 10.1.1.102 102 > /dev/null &

    fi
fi


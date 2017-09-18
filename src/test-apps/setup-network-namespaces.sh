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



SCRIPT_NAME=setup-network-namespaces.sh
SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd ${SCRIPT_DIR}; pwd`
SYS_NAME=`uname -s`
USER_NAME=`id -nu`
GRP_NAME=`id -ng`
NS_BORDER_GW="NS-bg"
NS_SERVICE="NS-svc"
NS_MOB="NS-mob"
NS_APP="NS-app"
INTFNAME="ns_if"
REMOVEFLAG=false
MOB_REMOTE=true
function usage() 
{
    cat <<END
Usage:

    ${SCRIPT_NAME} <options>

    -i <iface-name>
    --intfName <iface-name>
        
        Interface name prefix.
    -l
    --local Mobile is local

    -r
    --remote Mobile is remote

    -D

        Delete all network namespaces.

END
}

# Parse options
#
while [ -n "$1" ]; do
    case $1 in
        -i | --intfName)
            INTFNAME=$2
            shift 2
            ;;
        -D | --delete)
            REMOVEFLAG=true
            shift 1
            ;;
        -l | --local)
            MOB_REMOTE=false
            shift 1
            ;;
        -r | --remote)
            MOB_REMOTE=true
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

      sudo ebtables -D FORWARD -d 33:33:00:00:00:01 -j DROP > /dev/null 2>&1

      sudo ifconfig NS-bridge down
      sudo brctl delif NS-bridge ${INTFNAME}-bg0
      sudo brctl delif NS-bridge ${INTFNAME}-svc0
      sudo brctl delif NS-bridge ${INTFNAME}-mob0
      sudo brctl delbr NS-bridge
    else
        echo "Creating network namespaces ${NS_BORDER_GW}, ${NS_SERVICE}, ${NS_MOB} and ${NS_APP}"
        sudo ip netns add ${NS_BORDER_GW}
        sudo ip netns add ${NS_SERVICE}
        sudo ip netns add ${NS_MOB}
        sudo ip netns add ${NS_APP}

        echo "Creating virtual interface pairs and assigning them to namespaces"
        sudo ip link add name ${INTFNAME}-bg0 type veth peer name ${INTFNAME}-bg1
        sudo ip link add name ${INTFNAME}-mob0 type veth peer name ${INTFNAME}-mob1
        sudo ip link add name ${INTFNAME}-svc0 type veth peer name ${INTFNAME}-svc1
        sudo ip link add name ${INTFNAME}-app0 type veth peer name ${INTFNAME}-app1
        sudo ip link set ${INTFNAME}-app0 netns ${NS_APP}
        sudo ip link set ${INTFNAME}-app1 netns ${NS_BORDER_GW}
        sudo ip link set ${INTFNAME}-bg1 netns ${NS_BORDER_GW}
        sudo ip link set ${INTFNAME}-svc1 netns ${NS_SERVICE}
        sudo ip link set ${INTFNAME}-mob1 netns ${NS_MOB}
 
        echo "Creating Namespace bridge"
        sudo brctl addbr NS-bridge

        echo "Adding interfaces to bridge"
        sudo brctl addif NS-bridge ${INTFNAME}-bg0
        sudo brctl addif NS-bridge ${INTFNAME}-mob0
        sudo brctl addif NS-bridge ${INTFNAME}-svc0


        echo "Bringing up NS-bridge"
        sudo ifconfig NS-bridge up

        echo "Bring up virtual interfaces within network namespaces"
        sudo ip netns exec ${NS_BORDER_GW} ifconfig ${INTFNAME}-bg1 up
        sudo ip netns exec ${NS_APP} ifconfig ${INTFNAME}-app0 up
        sudo ip netns exec ${NS_BORDER_GW} ifconfig ${INTFNAME}-app1 up
        sudo ip netns exec ${NS_SERVICE} ifconfig ${INTFNAME}-svc1 up
        sudo ip netns exec ${NS_MOB} ifconfig ${INTFNAME}-mob1 up

        sudo ip netns exec ${NS_APP} ifconfig lo up
        sudo ip netns exec ${NS_BORDER_GW} ifconfig lo up
        sudo ip netns exec ${NS_SERVICE} ifconfig lo up
        sudo ip netns exec ${NS_MOB} ifconfig lo up

        echo "Adding IPv4 addresses to virtual interfaces"
        sudo ip netns exec ${NS_APP} ip addr add 10.1.1.100/24 dev ${INTFNAME}-app0
        sudo ip netns exec ${NS_BORDER_GW} ip addr add 10.1.1.101/24 dev ${INTFNAME}-app1
        sudo ip netns exec ${NS_BORDER_GW} ip addr add 10.1.2.101/24 dev ${INTFNAME}-bg1
        sudo ip netns exec ${NS_SERVICE} ip addr add 10.1.2.102/24 dev ${INTFNAME}-svc1
        sudo ip netns exec ${NS_MOB} ip addr add 10.1.2.103/24 dev ${INTFNAME}-mob1

        sudo ip addr add 10.1.2.104/24 dev NS-bridge
        sudo ip -6 addr add fd00:0:1:1::104/64 dev NS-bridge

        sudo ifconfig ${INTFNAME}-bg0 up
        sudo ifconfig ${INTFNAME}-svc0 up
        sudo ifconfig ${INTFNAME}-mob0 up

        echo "Adding IPv6 addresses to virtual interfaces"
        sudo ip netns exec ${NS_APP} ip -6 addr add fd00:0:1:6::100/64 dev ${INTFNAME}-app0
        sudo ip netns exec ${NS_BORDER_GW} ip -6 addr add fd00:0:1:6::101/64 dev ${INTFNAME}-app1
        sudo ip netns exec ${NS_BORDER_GW} ip -6 addr add fd00:0:1:1::101/64 dev ${INTFNAME}-bg1
        sudo ip netns exec ${NS_SERVICE} ip -6 addr add fd00:0:1:5::102/64 dev ${INTFNAME}-svc1
        sudo ip netns exec ${NS_MOB} ip -6 addr add fd00:0:1:4::103/64 dev ${INTFNAME}-mob1

        echo "Add a fabric default route in ${NS_APP} to go to ${NS_BORDER_GW}"
        sudo ip netns exec ${NS_APP} ip -6 route add fd00:0:1::/48 via fd00:0:1:6::101 dev ${INTFNAME}-app0

        echo "Enable forwarding in ${NS_BORDER_GW}"
        sudo ip netns exec ${NS_BORDER_GW} sysctl -w net.ipv6.conf.all.forwarding=1

        if ${MOB_REMOTE}; then
            echo "Disable multicast traffic within bridge"
            sudo ebtables -A FORWARD -d 33:33:00:00:00:01 -j DROP
        fi
    fi
fi


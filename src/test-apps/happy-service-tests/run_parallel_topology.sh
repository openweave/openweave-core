#!/bin/bash

#
#    Copyright (c) 2017 Nest Labs, Inc.
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

#
#    @file
#       Manually run happy topology
#       M of Thead, WiFi, and WAN networks, which have:
#       - N thread-only devices
#       - one thread-wifi border_gateway devices
#       - one access-point router
#       - the onhub router is connected to the Internet
#       - fabric seed id 0x0701
#       - one mobile phone
#       - border_gateway has cellular backup connection
#      run_parallel_topology.sh create M N 0x00000 cellular mobile
#

if [ -z "$EUI64_PREFIX" ]; then
    #18:B4:30:AA Simulated / Test Weave Device Identifiers
    EUI64_PREFIX='18:B4:30:AA:00:'
fi

if [ "$1" != "" ]; then
    if [ "$1" == "create" ]; then
        echo "create the topology"
    elif [ "$1" == "destroy" ]; then
        echo "destroy the topology"
    else
        echo "need parameter create or destroy"
        exit
    fi
    sudo echo "parallel running start"
else
    echo "need parameter create or destroy"
    exit
fi

total_count=`printf "%d" $2`
if (($?)); then
    echo "error: total_count must be a number" >&2; exit 1
fi
echo "process $total_count fabrics"

device_numbers=`printf "%d" $3`
if (($?)); then
    echo "error: device_numbers must be a number, need to specify the number of devices within one fabric" >&2; exit 1
fi
echo "process $device_numbers devices within one fabric"

fabric_seed=`printf "%d" $4`
if (($?)); then
    echo "error: Fabric seed must be a number" >&2; exit 1
fi
echo "process initial fabric_seed $4"

if [ "$5" == "cellular" ]; then
    cellular="--cellular"
fi

if [ "$6" == "mobile" ]; then
    mobile="--mobile"
fi

if [ "$7" == "randomFabric" ]; then
    enable_random_fabric="--enable_random_fabric"
fi

pids=""
flag=true
count=0
declare -A pid_array
TEST_DIR=$(dirname $(readlink -f $0))

for ((i=0; i <$total_count; i++)); do
    action="$1"
    x=`printf "%03d" ${i}`
    cindex=$((fabric_seed+i))
    y=`printf "%05X" ${cindex}`
    z=`printf "%s%s:%s:%s" ${EUI64_PREFIX} ${y:0:2} ${y:2:2} ${y:4:1}`
    HAPPY_STATE_ID=${x} python $TEST_DIR/topologies/thread_wifi_ap_internet_configurable_topology.py --action=${action} --fabric_id=${y} --customized_eui64_seed=${z} --device_numbers=${device_numbers} ${cellular} ${mobile} ${enable_random_fabric}&
    pids+="$! "
    pid_array["$!"]=${x}
done

echo "all bullets are on the fly"

for pid in $pids; do
    wait $pid
    if [ $? -eq 0 ]; then
        echo -e "\033[32m SUCCESS - Job $pid, HAPPY_STATE_ID ${pid_array["$pid"]} exited with a status of $?"
        echo -e "\033[0m"
    else
        echo -e "\033[31m FAILED - Job $pid, HAPPY_STATE_ID ${pid_array["$pid"]} exited with a status of $?"
        echo -e "\033[0m"
        flag=false
        count=$(($count+1))
    fi
done

printf " success rate for $total_count runs is %%"
echo "scale=2; $((total_count-count))*100/$total_count" | bc
if $flag; then
    echo -e "\033[32m parallel run succeeds"
    echo -e "\033[0m"
    exit 0
else
    echo -e "\033[31m parallel running fails"
    echo -e "\033[0m"
    exit 1
fi

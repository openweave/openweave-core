#!/bin/bash


#
#    Copyright (c) 2016-2017 Nest Labs, Inc.
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
#       Manually run happy WdmNext tests in parallel
#       for example, create topology with two fabrics where each fabric has three devices and fabric seed is 0701, tier is tunnel01.weave01.iad02.loadtest3.nestlabs.com
#       time sh run_parallel_weave_service_test.sh 1 3 0x0701 1 tunnel01.weave01.iad02.loadtest3.nestlabs.com wdmNext/test_weave_wdm_next_service_mutual_subscribe_01.py
#

sudo echo "parallel running start"

pids=""

declare -A pid_array
flag=true
count=0

if [ -z "$BUILD_JOBS" ]; then
    export BUILD_JOBS=50
fi

total_count=`printf "%d" $1`
if (($?)); then
    echo "error: total_count must be a number" >&2; exit 1
fi
echo "process $total_count fabrics"

device_numbers=`printf "%d" $2`
if (($?)); then
    echo "error: device_numbers must be a number, need to specify the number of devices within one fabric" >&2; exit 1
fi
echo "process $device_numbers devices within one fabric"

fabric_seed=`printf "%04X" $3`
if (($?)); then
    echo "error: Fabric seed must be a number" >&2; exit 1
fi
echo "process initial fabric_seed $3"

topology=$4
if [[ "$4" -eq 1 ]] ; then
    echo "create topology"
else
    echo "not required to create topology"
fi

if [ -z "$happy_dns" ]; then
    export happy_dns="8.8.8.8 172.16.255.1 172.16.255.153 172.16.255.53"
fi

weave_service_address=$5
export weave_service_address
echo "service address is $weave_service_address"

test_file=$6
echo "test file is $test_file"

for ((i=0; i<$total_count; i++)); do
    while [ $(jobs | wc -l) -ge $BUILD_JOBS ] ; do sleep 1 ; done
    action="$1"
    x=`printf "%03d" ${i}`
    HAPPY_STATE_ID=${x} FABRIC_OFFSET=${i} DEVICE_NUMBERS=${device_numbers} TOPOLOGY=${topology} FABRIC_SEED=${fabric_seed} python $6  &
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
        count=$(($count+1))
        flag=false
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


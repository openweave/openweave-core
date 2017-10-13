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
#    run weave perf test suite

if [ -z "$happy_dns" ]; then
    export happy_dns="8.8.8.8 172.16.255.1 172.16.255.153 172.16.255.53"
fi

if [ -z "$weave_service_address" ]; then
    export weave_service_address="tunnel01.weave01.iad02.integration.nestlabs.com"
fi

if [ -z "$FABRIC_SEED" ]; then
    export FABRIC_SEED="0x00000"
fi

if [ -z "$NUM_TUNNELS" ]; then
    export NUM_TUNNELS=50
fi

if [ -z "$NUM_DEVICES" ]; then
    export NUM_DEVICES=3
fi

if [ -z "$ENABLE_RANDOM_FABRIC" ]; then
    export randomFabric=""
else
    export randomFabric="randomFabric"
fi

VAR_TEST=true

if [ -z "$TESTCASES" ]; then
    TESTCASES=(echo/test_weave_echo_02.py
               tunnel/test_weave_tunnel_02.py
               time/test_weave_time_01.py
               wdmNext/test_weave_wdm_next_service_mutual_subscribe_05.py
               wdmNext/test_weave_wdm_next_service_mutual_subscribe_09.py
               wdmNext/test_weave_wdm_next_service_mutual_subscribe_17.py
              )
else
    TESTCASES=($(echo $TESTCASES | tr ',' "\n"))
fi

TEST_DIR=$(dirname $(readlink -f $0))
(source $TEST_DIR/run_parallel_topology.sh create ${NUM_TUNNELS} ${NUM_DEVICES} ${FABRIC_SEED} cellular mobile ${randomFabric})
VAR_TOPOLOGY_CREATE=$?

for i in ${TESTCASES[@]}; do
    (source $TEST_DIR/run_parallel_weave_service_test.sh ${NUM_TUNNELS} ${NUM_DEVICES} ${FABRIC_SEED} 0 ${weave_service_address} $TEST_DIR/${i})
    if [ $? -eq 0 ]; then
        echo -e "\033[32m SUCCESS - ${i} exited with a status of $?"
        echo -e "\033[0m"
    else
        echo -e "\033[31m FAILED - ${i} exited with a status of $?"
        echo -e "\033[0m"
        VAR_TEST=false
    fi
done

(source $TEST_DIR/run_parallel_topology.sh destroy ${NUM_TUNNELS} ${NUM_DEVICES} ${FABRIC_SEED})
VAR_TOPOLOGY_DESTROY=$?

if $VAR_TEST; then
    echo -e "\033[32m test is ok"
    echo -e "\033[0m"
    exit 0
else
    echo -e "\033[31m test is not ok"
    echo -e "\033[0m"
    exit 1
fi

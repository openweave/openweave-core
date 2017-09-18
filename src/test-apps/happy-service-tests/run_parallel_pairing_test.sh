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


export PARALLEL_PAIRING=1

if [ -z "$NUM_DEVICES" ]; then
    export NUM_DEVICES=1
fi

if [ -z "$NUM_TUNNELS" ]; then
    export NUM_TUNNELS=10
fi

if [ -z "$weave_service_tier" ];then
    export weave_service_tier="gopal2.unstable"
fi

./weave_service_perf_run.sh

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
# Manually run happy tests
#

heartbeat/test_weave_heartbeat_01.py
echo/test_weave_echo_01.py
bdx/test_weave_bdx_01.py
bdx/test_weave_bdx_02.py
bdx/test_weave_bdx_03.py
bdx/test_weave_bdx_04.py
tunnel/test_weave_tunnel_01.py
wrmp/test_weave_wrmp_01.py
time/test_weave_time_01.py

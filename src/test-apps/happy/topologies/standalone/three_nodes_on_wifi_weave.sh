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

# Steps to manually build three_nodes_on_wifi_weave.json

happy-node-add node01
happy-node-add node02
happy-node-add node03
happy-network-add Home wifi
happy-network-address Home 2001:db8:1:2::
happy-node-join node01 Home
happy-node-join node02 Home
happy-node-join node03 Home

weave-fabric-add fab1
weave-node-configure

#happy-state -s three_nodes_on_wifi_weave.json

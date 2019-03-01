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

# Steps to manually build 2 nodes happy topology for dns resolution test
IFACE=$(route | grep '^default' | grep -o '[^ ]*$')
happy-state-delete
happy-network-add Home wifi
happy-network-address --id Home --add 10.0.1.0
happy-node-add node01
happy-node-add --ap onhub
happy-node-join onhub Home
happy-node-join --tap node01 Home
happy-network-route --prefix 10.0.1.0 Home onhub
happy-internet --node onhub --interface ${IFACE} --isp ${IFACE//[0-9]/} --seed 249

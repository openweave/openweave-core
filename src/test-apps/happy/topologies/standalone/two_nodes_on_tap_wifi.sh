#!/bin/bash

#
#    Copyright (c) 2019 Google LLC.
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

# Steps to manually build two_nodes_on_tap_wifi.json

happy-node-add 00-WifiNode-Tx
happy-node-add 01-WifiNode-Rx

happy-network-add WifiNetwork wifi
happy-network-address WifiNetwork 2001:db8:1:2::
happy-network-address WifiNetwork fd00:0000:fab1:0001::
happy-network-address WifiNetwork fe80::
happy-network-address WifiNetwork 192.168.1.0

happy-node-join --tap 00-WifiNode-Tx WifiNetwork
happy-node-join --tap 01-WifiNode-Rx WifiNetwork

# happy-state -s two_nodes_on_tap_wifi.json

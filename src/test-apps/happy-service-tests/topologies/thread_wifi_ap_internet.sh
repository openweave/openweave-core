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

# Example of Thead, WiFi, and WAN networks, which have:
# - one thread-only device ThreadNode
# - one cellular-only mobile phone
# - one thread-wifi-cellular border_gateway device BorderRouter
# - one access-point router onhub
# - the onhub router is connected to the Internet

happy-network-add HomeThread thread
happy-network-address HomeThread 2001:db8:111:1::

happy-network-add HomeWiFi wifi
happy-network-address HomeWiFi 2001:db8:222:2::
happy-network-address HomeWiFi 10.0.1.0

happy-network-add TMobile cellular
happy-network-address TMobile 2001:db8:333:3::

happy-node-add ThreadNode01
happy-node-join ThreadNode01 HomeThread

happy-node-add mobile
happy-node-join mobile TMobile

happy-node-add BorderRouter
happy-node-join BorderRouter HomeThread
happy-node-join BorderRouter HomeWiFi
happy-node-join BorderRouter TMobile

happy-node-add --ap onhub
happy-node-join onhub HomeWiFi

happy-network-route HomeThread BorderRouter
happy-network-route TMobile BorderRouter
happy-network-route --prefix 10.0.1.0 HomeWiFi onhub
happy-network-route --prefix 2001:db8:222:2:: HomeWiFi onhub

weave-fabric-add fab1
weave-node-configure
weave-network-gateway HomeThread BorderRouter

# To run the manual test against an entity on the internet two additional steps are required:
# 1. configure the internet access on an interface on one of the nodes in the happy network:
# happy-internet --node onhub --interface eth0 --isp eth --seed 249
# 2. provide DNS configuration for Happy:
# happy-dns 172.16.255.1 172.16.255.153 172.16.255.53

# To delete topology, three steps are required:
# 1. clear dns configuration:
# happy-dns -d 172.16.255.1 172.16.255.153 172.16.255.53
# 2. clear the internet access configuration on an interface:
# happy-internet -d --interface eth0 --isp eth --seed 249
# 3. clear network topology:
# happy-state-delete


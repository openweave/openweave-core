#
# Simnet Network Layout: Two HANs with shared gateways implemented on host
#
# This simnet configuration defines two HANs, each with its own WiFi and Thread networks.
# Both HANs contain a single Weave device connected to the respective WiFi/Thread networks.
# The HANs also contain separate Gateway nodes that are implemented, in unison, by the
# host (i.e. the host acts as both Gateways simultaneously). The gateways use the host's
# default interface (typically eth0) as their outside interface allowing the HANs to
# access the internet if the host has internet access.
#
# Note: In order for this configuration to work, the two Gateway nodes must use distinct
# IPv4 subnets on their inside interfaces, even though in a real scenario they could use
# the same subnet.
#

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

#===============================================================================
# HAN-1
#===============================================================================

WiFiNetwork(
    name = 'han-1-wifi',
)

ThreadNetwork(
    name = 'han-1-thread',
    meshLocalPrefix = 'fd24:2424:2424::/64'
)

# Gateway in HAN-1 implemented on host, with outside access via host's default interface. 
Gateway(
    name = 'han-1-gw',
    outsideNetwork = None,
    outsideInterface = 'host-default',
    useHost = True,
    insideNetwork = 'han-1-wifi',
    insideIP4Subnet = '192.168.168.0/24',
    isIP4DefaultGateway = True
)

# Weave device in HAN-1 connected to HAN-1 WiFi and Thread networks
WeaveDevice(
    name = 'han-1-dev',
    weaveNodeId = 1,
    weaveFabricId = 1,
    wifiNetwork = 'han-1-wifi',
    threadNetwork = 'han-1-thread'
)

#===============================================================================
# HAN-2
#===============================================================================

WiFiNetwork(
    name = 'han-2-wifi',
)

ThreadNetwork(
    name = 'han-2-thread',
    meshLocalPrefix = 'fd42:4242:4242::/64'
)

# Gateway in HAN-2 implemented on host, with outside access via host's default interface. 
Gateway(
    name = 'han-2-gw',
    outsideNetwork = None,
    outsideInterface = 'host-default',
    useHost = True,
    insideNetwork = 'han-2-wifi',
    insideIP4Subnet = '192.168.167.0/24',
    isIP4DefaultGateway = True
)

# Weave device in HAN-2 connected to HAN-2 WiFi and Thread networks
WeaveDevice(
    name = 'han-2-dev',
    weaveNodeId = 2,
    weaveFabricId = 2,
    wifiNetwork = 'han-2-wifi',
    threadNetwork = 'han-2-thread'
)

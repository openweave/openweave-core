# 
# Simnet Network Layout: Two Weave devices
#
# This simnet configuration defines a single HAN with WiFi, Thread and Legacy
# Thread networks. The HAN contains two Weave devices, one WiFi-enabled (dev1)
# and one Thread-only (dev2).  A Gateway node (home-gw) connects the HAN's WiFi
# network to a simulated Internet. 
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
# Networks
#===============================================================================

Internet()

WiFiNetwork(
    name = 'wifi',
)

ThreadNetwork(
    name = 'thread',
    meshLocalPrefix = 'fd42:4242:4242::/64'
)

LegacyThreadNetwork(
    name = 'legacy'
)

#===============================================================================
# Devices
#===============================================================================

# Home WiFi gateway with connection to the Internet
Gateway(
    name = 'home-gw',
    outsideNetwork = 'inet',
    insideNetwork = 'wifi',
    insideIP4Subnet = '192.168.168.0/24',
    isIP4DefaultGateway = True
)

# Weave device with WiFi, Thread and Legacy Thread networks
WeaveDevice(
    name = 'dev1',
    weaveNodeId = 1,
    weaveFabricId = 1,
    wifiNetwork = 'wifi',
    threadNetwork = 'thread',
    legacyNetwork = 'legacy',
)

# Weave device with Thread and Legacy Thread networks only
WeaveDevice(
    name = 'dev2',
    weaveFabricId = 1,
    weaveNodeId = 2,
    threadNetwork = 'thread',
    legacyNetwork = 'legacy'
)

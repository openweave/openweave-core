#===============================================================================
# Networks
#===============================================================================

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


Internet()

WiFiNetwork(
    name = 'wifi'
)

Gateway(
    name = 'home-gw',
    outsideNetwork = 'inet',
    insideNetwork = 'wifi',
    insideIP4Subnet = '192.168.1.0/24',
    isIP4DefaultGateway = True
)

WiFiNetwork(
    name = 'cell-network'
)

Gateway(
    name = 'cell-gw',
    outsideNetwork = 'inet',
    insideNetwork = 'cell-network',
    insideIP4Subnet = '192.168.2.0/24',
    isIP4DefaultGateway = True
)


WeaveDevice(
    name = 'dev',
    wifiNetwork = 'wifi',
    cellularNetwork = 'cell-network'
)

SimpleNode(
    name = 'service',
    network = 'inet'
)

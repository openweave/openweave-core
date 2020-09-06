#!/usr/bin/env python

#
#    Copyright (c) 2019 Google, LLC.
#    Copyright (c) 2015-2018 Nest Labs, Inc.
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

##
#    @file
#       Implements WeaveState class that implements methods to retrieve
#       parts of state setup that relate to Weave plugin.
#

import json
from happy.State import State
from happy.utils.IP import IP

options = {}
options["quiet"] = False

def option():
    return options.copy()

class WeaveState(State):

    """
    Displays Weave-related parameters for Weave nodes in a Happy network
    topology.

    weave-state [-h --help] [-q --quiet]

    Examples:
    $ weave-state
        Displays Weave-related parameters for all Weave nodes in the
        current Happy topology.

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        State.__init__(self)

        self.quiet = opts["quiet"]

    def getWeaveRecord(self, state=None):
        state = self.getState(state)
        if "weave" not in state.keys():
            state["weave"] = {}
        return state["weave"]

    def getFabricRecord(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "fabric" not in weave_record.keys():
            weave_record["fabric"] = {}
        return weave_record["fabric"]

    def getWeaveNodeRecord(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "node" not in weave_record.keys():
            weave_record["node"] = {}
        return weave_record["node"]

    def getTunnelRecord(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "tunnel" not in weave_record.keys():
            weave_record["tunnel"] = {}
        return weave_record["tunnel"]

    def getWeaveNetworks(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "network" not in weave_record.keys():
            weave_record["network"] = {}
        return weave_record["network"]

    def getFabricId(self, state=None):
        fabric_record = self.getFabricRecord(state)
        if "id" not in fabric_record.keys():
            return None
        fabid = fabric_record["id"]
        return "%x" % (int(fabid, 16))

    def getTunnelGatewayNodeId(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "tunnel" not in weave_record.keys():
            return None
        if "gateway" not in weave_record["tunnel"].keys():
            return None
        return weave_record["tunnel"]["gateway"]

    def getTunnelServiceNodeId(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "tunnel" not in weave_record.keys():
            return None
        if "service" not in weave_record["tunnel"].keys():
            return None
        return weave_record["tunnel"]["service"]

    def getTunnelServiceDir(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "tunnel" not in weave_record.keys():
            return None
        if "service_dir" not in weave_record["tunnel"].keys():
            return None
        return weave_record["tunnel"]["service_dir"]

    def getFabricGlobalPrefix(self, state=None):
        fabric_record = self.getFabricRecord(state)
        if "global_prefix" not in fabric_record.keys():
            return None
        return fabric_record["global_prefix"]

    def getWeaveNodes(self, state=None):
        nodes_record = self.getWeaveNodeRecord(state)
        return nodes_record.keys()

    def getWeaveNodeIds(self, state=None):
        nodes_record = self.getWeaveNodeRecord(state)
        return [nodes_record[node_name]['weave_node_id'] for node_name in nodes_record]

    def getNodeWeaveRecord(self, node_id=None, state=None):
        nodes_record = self.getWeaveNodeRecord(state)
        if node_id not in nodes_record.keys():
            return {}
        return nodes_record[node_id]

    def getWeaveNodeID(self, node_id=None, state=None):
        node_record = self.getNodeWeaveRecord(node_id, state)
        if "weave_node_id" not in node_record.keys():
            return None
        return node_record["weave_node_id"]

    def getWeaveNodeIID(self, node_id=None, state=None):
        node_record = self.getNodeWeaveRecord(node_id, state)
        if "iid" not in node_record.keys():
            return None
        return node_record["iid"]

    def getWeaveNodeEUI64(self, node_id=None, state=None):
        node_record = self.getNodeWeaveRecord(node_id, state)
        if "eui64" not in node_record.keys():
            return None
        return node_record["eui64"]

    def getNodeWeaveIPAddress(self, node_id=None, state=None):
        weave_global_prefix = self.getFabricGlobalPrefix(state)
        if weave_global_prefix is None:
            return None
        node_addresses = self.getNodeAddresses(node_id, state)
        if node_addresses == []:
            return None
        for addr in node_addresses:
            if IP.prefixMatchAddress(weave_global_prefix, addr):
                return addr
        return None

    def getNodeInterfaceWeaveIPAddress(self, interface_id, node_id, state=None):
        """This function attempts to get weave IP address of a paticular node's interface.

        Args:
            interface_id(str): A string containing a node interface id, example: "wlan0" or "wpan0".
            node_id(str): A string containing a node id, example: "BorderGateway", "ThreadNode".
            state(json data): node's json data, default is None.

        Returns:
           str. The return value:
                None:  not found weave address
                addr:  found weave address
        """
        weave_global_prefix = self.getFabricGlobalPrefix(state)
        if weave_global_prefix is None:
            return None
        node_interface_addresses = self.getNodeInterfaceAddresses(interface_id, node_id, state)
        if node_interface_addresses == []:
            return None
        for addr in node_interface_addresses:
            if IP.prefixMatchAddress(weave_global_prefix, addr):
                return addr
        return None

    def getWeaveNetworkIds(self, state=None):
        networks_record = self.getWeaveNetworks(state)
        return networks_record.keys()

    def getWeaveNetworkRecord(self, network_id=None, state=None):
        networks_record = self.getWeaveNetworks(state)
        if network_id not in networks_record.keys():
            return {}
        return networks_record[network_id]

    def getWeaveNetworkGatewayIds(self, network_id=None, state=None):
        network_record = self.getWeaveNetworkRecord(network_id, state)
        if "gateway" not in network_record.keys():
            return []
        return network_record["gateway"].keys()

    def getServiceWeaveID(self, service, node_id=None, state=None):
        endpoints = self.getServiceEndpoints()
        if service not in endpoints.keys():
            return None
        return endpoints[service]["id"]

    def getServiceWeaveIPAddress(self, service, node_id=None, state=None):
        weave_global_prefix = self.getFabricGlobalPrefix(state)

        global_prefix_addr, _ = weave_global_prefix.split("::")

        if weave_global_prefix is None:
            return None

        weave_id = self.getServiceWeaveID(service, node_id=None, state=None)

        if weave_id is None:
            return None

        subnet = 5
        subnet_hex = "%04x" % (subnet)

        eui = self.WeaveIdtoEUI64(weave_id)
        iid = IP.EUI64toIID(eui)

        addr = global_prefix_addr + ":" + subnet_hex + ":" + iid
        return addr

    def getWeaveGlobalPrefix(self, fabric_id):
        ula_prefix = "fd"

        global_prefix = ""

        lower_40_bits = int(str(fabric_id), 16)

        lower_40_bits = lower_40_bits % (1 << 40)
        lower_40_bits = str(format(lower_40_bits, 'x'))

        for i in range(3):
            left = len(lower_40_bits)
            if left > 3:
                global_prefix = lower_40_bits[-4:] + ":" + global_prefix
                lower_40_bits = lower_40_bits[:-4]
            elif left > 0:
                global_prefix = "0" * (4 - left) + lower_40_bits + ":" + global_prefix
                lower_40_bits = ""
            else:
                global_prefix = "0000:" + global_prefix

        global_prefix = ula_prefix + global_prefix[2:]
        global_prefix += ":/48"

        return global_prefix

    def typeToWeaveSubnet(self, interface_type):
        """This function attempts to match node interface type to a matching weave subnet.

        Args:
           interface_type (str): A string containing a node interface type, example: "wifi" or "wan".

        Returns:
           integer. The return code:

              1  -- weave subnet is 1 which matches a wifi interface.
              5  -- weave subnet is 5 which matches a wan interface.
              4  -- weave subnet is 4 which matches a mobile interface.
              6  -- weave subnet is 6 which matches a thread interface.
              0  -- weave subnet is 0 which matches all other interfaces.
        """

        if interface_type == "wifi":
            return 1

        if interface_type == "wan":
            return 5

        if interface_type == "mobile":
            return 4

        if interface_type == "thread":
            return 6

        return 0

    def IPv6toWeaveId(self, addr):
        iid = IP.getIPv6IID(addr)
        eui64 = IP.IIDtoEUI64(iid)
        weave_id = self.EUI64toWeaveId(eui64)
        return weave_id

    def EUI64toWeaveId(self, addr):
        id = addr.split("-")
        id = "".join(id)
        return id

    def WeaveIdtoEUI64(self, addr):
        zeros = 16 - len(addr)
        eui = '0' * zeros
        eui += addr

        eui = [eui[i:i+2] for i in range(0, len(eui), 2)]
        eui = "-".join(eui)
        return str(eui)

    def getServiceEndpoints(self):
        endpoint = {}

        endpoint["Directory"] = {}
        endpoint["Directory"]["id"] = '18B4300200000001'

        endpoint["SoftwareUpdate"] = {}
        endpoint["SoftwareUpdate"]["id"] = '18B4300200000002'

        endpoint["DataManagement"] = {}
        endpoint["DataManagement"]["id"] = '18B4300200000003'

        endpoint["LogUpload"] = {}
        endpoint["LogUpload"]["id"] = '18B4300200000004'

        endpoint["Time"] = {}
        endpoint["Time"]["id"] = '18B4300200000005'

        endpoint["ServiceProvisioning"] = {}
        endpoint["ServiceProvisioning"]["id"] = '18B4300200000010'

        endpoint["Tunnel"] = {}
        endpoint["Tunnel"]["id"] = '18B4300200000011'

        endpoint["ServiceRouter"] = {}
        endpoint["ServiceRouter"]["id"] = '18B4300200000012'

        endpoint["FileDownload"] = {}
        endpoint["FileDownload"]["id"] = '18B4300200000013'

        endpoint["BastionService"] = {}
        endpoint["BastionService"]["id"] = '18B4300200000014'

        endpoint["Echo"] = {}
        endpoint["Echo"]["id"] = '18B4300200000012'    # Reuse ServiceRouter Endpoint ID

        endpoint["KeyExport"] = {}
        endpoint["KeyExport"]["id"] = '18B4300200000014'    # Use Bastion Service Endpoint ID

        return endpoint

    def getSerialNum(self, node_id=None, state=None):
        node_record = self.getNodeWeaveRecord(node_id, state)
        if "serial_num" not in node_record.keys():
            return None
        return node_record["serial_num"]

    def setFabric(self, record, state=None):
        weave_record = self.getWeaveRecord(state)
        weave_record["fabric"] = record

    def setTunnel(self, record, state=None):
        weave_record = self.getWeaveRecord(state)
        weave_record["tunnel"] = record

    def setWeaveNode(self, node_id, record, state=None):
        weave_nodes = self.getWeaveNodeRecord(state)
        weave_nodes[node_id] = record

    def setWeaveGateway(self, network_id, gateway, state=None):
        network_record = self.getWeaveNetworkRecord(network_id, state)
        network_record["gateway"] = gateway

    def setSerialNum(self, node_id, record, state=None):
        node_record = self.getNodeWeaveRecord(node_id, state)
        node_record["serial_num"] = record

    def removeTunnel(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "tunnel" in weave_record.keys():
            del weave_record["tunnel"]

    def removeFabric(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "fabric" in weave_record.keys():
            del weave_record["fabric"]

    def removeWeaveNode(self, node, state=None):
        weave_record = self.getWeaveRecord(state)
        if "node" in weave_record.keys():
            node_record = weave_record['node']
            if node in node_record.keys():
                del node_record[node]
                if len(node_record) == 0:
                    self.removeWeaveNodes(state)

    def removeWeaveNodes(self, state=None):
        weave_record = self.getWeaveRecord(state)
        if "node" in weave_record.keys():
            del weave_record["node"]

    def removeWeaveGateway(self, network_id, state=None):
        network_record = self.getWeaveNetworkRecord(network_id, state)
        if "gateway" in network_record.keys():
            del network_record["gateway"]

    def __str__(self):
        return self.getWeaveRecord()

    def printState(self):
        """
        State Name: weave
         
        NODES      Name        Weave Node Id    Pairing Code
                 BorderRouter  18B4300000000001          AAA123
                 ThreadNode    18B4300000000002          BBB123
        """
        if self.quiet:
            return

        print "State Name: weave"
        print

        weave_nodes = self.getWeaveNodeRecord()
        node_format = "%-10s%15s%20s%16s"
        print node_format % ("NODES", "Name", "Weave Node Id", "Pairing Code")

        for node in sorted(weave_nodes.keys()):
            node_record = weave_nodes[node]
            print node_format % ("", node, node_record.get('weave_node_id', 'None'), node_record.get('pairing_code', 'None'))
        print

        fabric_record = self.getFabricRecord()
        fabric_format = "%-10s%15s%24s"
        print fabric_format % ("FABRIC", "Fabric Id", "Global Prefix")
        print fabric_format % ("", fabric_record.get('id', ""), fabric_record.get('global_prefix', ""))
        print

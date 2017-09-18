#!/usr/bin/python

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


import os
import sys
import subprocess
import re
import time
try:
    import ipaddress
except ImportError, ex:
    print ex.message
    print 'HINT: You can install the ipaddress package by running "sudo pip install ipaddress"'
    sys.exit(-1)
import shutil
import random
import socket


#===============================================================================
# Global Definitions
#===============================================================================

namePrefix = 'sn-'

defaultIEEEOUI = 0x18B430                           # Nest's OUI
baseMAC48Address = defaultIEEEOUI << 24
baseMAC64Address = defaultIEEEOUI << 40

defaultWeaveFabricId = 1

ulaPrefix = ipaddress.IPv6Network(u'fd00::/8')
llaPrefix = ipaddress.IPv6Network(u'fe80::/64')

defaultHostEntries = '''############################## misc ##############################
127.0.0.1                                localhost
::1                                      ip6-localhost ip6-loopback
fe00::0                                  ip6-localnet
ff00::0                                  ip6-mcastprefix
ff02::1                                  ip6-allnodes
ff02::2                                  ip6-allrouters
127.0.0.1                                %s
''' % (socket.gethostname()) # NOTE: An entry for the hostname is required for certain tools to operate properly (e.g. sudo).

scriptDirName = os.path.dirname(os.path.abspath(os.path.realpath(__file__)))


#===============================================================================
# SimNet Class
#===============================================================================

class SimNet:
    
    def __init__(self):
        self.networks = []
        self.nodes = []
        self.additionalHostsEntries = ''
        self.layoutSearchDirs = [
            '.',
            os.path.join(os.environ['HOME'], '.simnet/layouts'),
            os.path.join(scriptDirName, 'lib/simnet/layouts'),
            os.path.join(scriptDirName, '../lib/simnet/layouts'),
        ]
    
    def loadLayoutFile(self, layoutFileName):

        # If the layout file name does *not* include a path component, search for the
        # file in the configured search directories...        
        if os.path.dirname(layoutFileName) == '':
            for dirName in self.layoutSearchDirs:
                f = os.path.join(dirName, layoutFileName)
                if os.path.exists(f):
                    layoutFileName = f
                    break
                f = f + '.py'
                if os.path.exists(f):
                    layoutFileName = f
                    break
                     
        if not os.path.exists(layoutFileName):
             raise UsageException("Network layout file not found: %s" % layoutFileName)

        print "Loading: %s" % (layoutFileName)
        
        global networks, nodes
        networks = self.networks
        nodes = self.nodes
        
        execfile(layoutFileName)
             
        for node in self.nodes:
            node.configureAddresses()
    
        for node in self.nodes:
            node.configureRoutes()
    
    def buildNetworksAndNodes(self):
        verifyRoot()

        global logActions, logIndent
        logActions = True

        # If one or more of the nodes is implemented by the host, initialize the host's iptables.        
        if True in [ n.isHostNode for n in self.nodes ]:
            logAction('Initializing host iptables')
            logIndent += 1
            initSimnetIPTables()
            logIndent -= 1
        
        for network in self.networks:
            network.buildNetwork()

        hostsTable = self.generateHostsTable()

        for node in self.nodes:
            node.buildNode(hostsTable)
        
        logActions = False

    def clearNetworksAndNodes(self):
        verifyRoot()
        
        global logActions, logIndent
        logActions = True
        
        for node in self.nodes:
            node.clearNode()
        
        for network in self.networks:
            network.clearNetwork()
            
        clearSimnetIPTables()
        
        logActions = False

    def summarizeNetwork(self, networks=True, nodes=True, hosts=True, env=True):
        s = ''
        if networks:
            s += 'Networks:\n'
            for network in self.networks:
                s += network.summary(prefix='  ')
        if nodes or env:
            s += 'Nodes:\n'
            for node in self.nodes:
                if nodes:
                    s += node.summary(prefix='  ')
                else:
                    s += '  Node "%s" (%s):\n' % (node.name, node.__class__.__name__)
                if env:
                    s += '    Environment:\n'
                    envVars = {}
                    node.setEnvironVars(envVars)
                    for name in sorted(envVars.keys()):
                        s += '      %s: %s\n' % (name, envVars[name])
        if hosts:
            s += 'Hosts Table:\n'
            s += re.sub('^', '  ', self.generateHostsTable(), flags=re.M)
        return s
    
    def startShell(self, node, noOverridePrompt=False):

        verifyRoot()

        if not isinstance(node, Node):
            nodeName = node
            node = self.findNode(nodeName)
            if not node:
                raise UsageException('Unknown node: %s' % nodeName)
            if not node.isNodeBuilt():
                raise UsageException('Node currently not active: %s' % nodeName)

        shell = os.path.realpath(os.path.abspath(os.environ['SHELL']))
        
        if os.path.basename(shell) == 'bash' and not noOverridePrompt:
            promptOverride = [ '-c', '''exec $SHELL --rcfile <(cat ~/.bashrc; echo '[ "$SN_NO_OVERRIDE_PROMPT" ] || PS1="\e[1m[\$SN_NODE_NAME]\e[0m $PS1"')''' ]
        else:
            promptOverride = [ ]
                
        cmd = [ 'su', ] + promptOverride + [ os.environ['SUDO_USER'] ]

        if not node.isHostNode:
            cmd = [ 'ip', 'netns', 'exec', node.nsName ] + cmd
            
        cmdEnv = os.environ.copy()
        node.setEnvironVars(cmdEnv)
        
        subprocess.call(cmd, env=cmdEnv)

    def clearAll(self):
        verifyRoot()

        global logActions, logIndent
        logActions = True

        # Clear all node namespaces
        for ns in getNamespaces():
            if ns.startswith(namePrefix):
                deleteNamespace(ns)

        # Clear all node namespace directories
        for name in os.listdir('/etc/netns'):
            if name.startswith(namePrefix):
                name = os.path.join('/etc/netns', name)
                if os.path.isdir(name):
                    print 'Deleting %s' % name
                    shutil.rmtree(name)

        # Clear all network bridges
        for b in getBridges():
            if b.startswith(namePrefix):
                deleteBridge(b)
        
        # Delete any remaining interfaces
        for name in [ i['dev'] for i in getInterfaces() ]:
            if name.startswith(namePrefix):
                deleteInterface(name)
        
        # Clear any simnet iptables configuration on the host.
        clearSimnetIPTables()
        
        # TODO: clear routes

        # Give a little time for async activities to settle.
        time.sleep(0.1)

        logActions = False

    def findNode(self, nodeName):
        nodeName = nodeName.lower()
        for node in self.nodes:
            if node.name.lower() == nodeName:
                return node
        return None 

    def getHostsEntries(self):
        hostsEntriesByNode = {}
        for node in nodes:
            hostsEntriesByNode[node.name] = node.getHostsEntries()
        return hostsEntriesByNode
        
    def generateHostsTable(self, prefix=''):
        hostsEntriesByNode = self.getHostsEntries()
        t = ''
        
        # Add a set of entries for each host.
        for nodeName in sorted(hostsEntriesByNode.keys()):
            t += '%s############################## %s ##############################\n' % (prefix, nodeName)
            hostsEntries = hostsEntriesByNode[nodeName]
            for name in sorted(hostsEntries.keys()):
                addrs = hostsEntries[name]
                if len(addrs) == 0:
                    continue
                for a in addrs:
                    if isinstance(a, ipaddress.IPv4Address):
                        t += '%s%-40s %s\n' % (prefix, a, name)
                for a in addrs:
                    if isinstance(a, ipaddress.IPv6Address):
                        t += '%s%-40s %s\n' % (prefix, a, name)
            t += '\n'
            
        # Add a set of default entries
        t += re.sub('^', prefix, defaultHostEntries + self.additionalHostsEntries, re.M)

        return t


#===============================================================================
# Network Classes
#===============================================================================

class Network():
    def __init__(self, name):
        self.name = name
        self.networkIndex = len(networks) + 1 # NOTE: networkIndexes must be > 0
        self.nsName = namePrefix + name
        self.brName = namePrefix + name
        self.attachedInterfaces = []
        self.ip4Supported = True
        self.ip6Supported = True
        self.usesMAC64 = False
        
        # Add the new network to the list of networks associated with the active simnet object.
        networks.append(self)
        
    def buildNetwork(self):

        global logIndent

        logAction('Building network: %s' % self.name)
        logIndent += 1
        try:
            self._buildNetwork()
        finally:
            logIndent -= 1
    
    def _buildNetwork(self):
        
        # Create a namespace for the network.
        addNamespace(self.nsName)

        # Create a virtual ethernet bridge to simulate the network
        addBridge(self.brName, nsName=self.nsName)

        # Enable the host interface for the bridge
        enableInterface(self.brName, nsName=self.nsName)

    def clearNetwork(self):

        # Delete the namespace for the network.
        deleteNamespace(self.nsName)

        # Delete the network bridge.
        # deleteBridge(self.brName)
        
    def summary(self, prefix=''):
        s  = '%sNetwork "%s" (%s implemented by namespace %s):\n' % (prefix, self.name, self.__class__.__name__, self.nsName)
        s += '%s  Network Index: %d\n' % (prefix, self.networkIndex)
        s += '%s  Virtual Bridge: %s\n' % (prefix, self.brName)
        return s
        
    def requestIP4AutoConfig(self, interface):
        return None
        
    def requestIP6AutoConfig(self, interface):
        return []
    
    def requestIP4AdvertisedRoutes(self, interface):
        return []
        
    def requestIP6AdvertisedRoutes(self, interface):
        return []
        
    @staticmethod
    def findNetwork(name, errMsg=''):
        global networks
        for n in networks:
            if n.name == name:
                return n
        if errMsg:
            raise ConfigException(errMsg + 'No such network: %s' % (name))
        return None

class Internet(Network):
    def __init__(self, ip4Subnet='4.0.0.0/8', ip6Prefix='2000::/3'):
        Network.__init__(self, 'inet')
        self.ip4Subnet = toIP4Subnet(ip4Subnet, 'Unable to initialize inet network: ')
        self.ip6Prefix = toIP6Prefix(ip6Prefix, 'Unable to initialize inet network: ')
        self.ip4Supported = self.ip4Subnet != None
        self.ip6Supported = self.ip6Prefix != None
        self.ip6PrefixRoutes = [ ]

    def requestIP4AutoConfig(self, interface):

        if interface.network != self:
            raise ConfigException('Attempt to auto-configure IPv4 interface %s which is not on network %s' % (interface.ifName, self.name))

        # Return nothing if an IPv4 subnet has not been specified.
        if not self.ip4Subnet:
            return None

        # Within the IPv4 subnet, return a /32 interface address for the node using the node id in the bottom bits.
        # This will be the node's externally-facing IPv4 address on the Internet.
        autoConfig = IP4AutoConfigInfo(
            address = makeIP4IntefaceAddress(self.ip4Subnet, interface.node.nodeIndex, prefixLen=32),
            defaultGateway = None,
            dnsServers = [ ]
        )

        return autoConfig

    def requestIP6AutoConfig(self, interface):
        
        if interface.network != self:
            raise ConfigException('Attempt to auto-configure IPv6 interface %s which is not on network %s' % (interface.ifName, self.name))

        # Return nothing if an IPv6 prefix has not been specified.
        if not self.ip6Prefix:
            return []
        
        # Form a unique /48 prefix for the node based on its node id.  This prefix will be delegated to the node
        # for further use in numbering internal networks.
        nodePrefix = makeIP6Prefix(self.ip6Prefix, networkNum=interface.node.nodeIndex, prefixLen=48)
        
        # Within the node prefix, form an /128 interface address for the node with a subnet of 0 and an IID based on
        # the MAC address of the interface. This is the node's externally-facing IPv6 address on the Internet.
        addr = makeIP6InterfaceAddress(nodePrefix, macAddr=interface.macAddress, prefixLen=128)
        
        # Record a global route that directs traffic for the delegated prefix to the node. 
        self.ip6PrefixRoutes.append(
            IPRoute(
                dest = nodePrefix,
                interface = None,
                via = addr.ip
            )
        )
        
        # Return a single auto-config response containing the node address and delegated prefix. 
        return [ 
            IP6AutoConfigInfo(
                addresses = [ addr ],
                delegatedPrefixes = [ nodePrefix ],
                defaultGateway = None,
                dnsServers = [ ]
            )
        ]

    def requestIP4AdvertisedRoutes(self, interface):
        routes = []

        if self.ip4Subnet != None:
            
            # Add 
            routes.append( 
                IPRoute(
                    dest = self.ip4Subnet, 
                    interface = interface,
                    via = None
                )
            )
        
        return routes
        
    def requestIP6AdvertisedRoutes(self, interface):
        routes = []
        
        if self.ip6Prefix != None:
            
            # Add a global route for the entire Internet that declares it 'on-link' on the requesting
            # node's interface. 
            routes.append(
                IPRoute(
                    dest = self.ip6Prefix,
                    interface = interface,
                    via = None
                )
            )
            
            # Add prefix routes for each of the nodes on the Internet, directing traffic destined to internal
            # networks to the respective gateway. However exclude any routes that point to the node that is
            # making the route request.
            nodeAddresses = [ a.ip for a in interface.ip6Addresses ]
            routes += [ p for p in self.ip6PrefixRoutes if p.via not in nodeAddresses ]

        return routes
        
    def summary(self, prefix=''):
        s = Network.summary(self, prefix)
        if self.ip4Subnet:
            s += '%s  IPv4 Subnet: %s\n' % (prefix, self.ip4Subnet)
        if self.ip6Prefix:
            s += '%s  IPv6 Prefix: %s\n' % (prefix, self.ip6Prefix)
        return s

class WiFiNetwork(Network):

    def __init__(self, name):
        Network.__init__(self, name)

    def requestIP4AutoConfig(self, interface):
        
        if interface.network != self:
            raise ConfigException('Attempt to auto-configure IPv4 interface %s which is not on network %s' % (interface.ifName, self.name))

        # For each node attached to the network...
        for i in self.attachedInterfaces:
            
            # Skip the node if it is the same one that is requesting auto-config.
            if i.node == interface.node:
                continue
            
            # Request auto-config information from the current node.  If successful,
            # return the info to the caller.
            autoConfig = i.node.requestIP4AutoConfig(interface)
            if autoConfig:
                return autoConfig

        return None 

    def requestIP6AutoConfig(self, interface):

        if interface.network != self:
            raise ConfigException('Attempt to auto-configure IPv6 interface %s which is not on network %s' % (interface.ifName, self.name))

        autoConfigs = []
        
        # For each node attached to the network...
        for i in self.attachedInterfaces:
            
            # Skip the node if it is the same one that is requesting auto-config.
            if i.node == interface.node:
                continue
            
            # Request auto-config information from the current node. If successful, add the result
            # to the information collected from other nodes on the network.
            autoConfig = i.node.requestIP6AutoConfig(interface)
            if autoConfig:
                autoConfigs.append(autoConfig)
        
        return autoConfigs

    def requestIP4AdvertisedRoutes(self, interface):
        routes = []
        # TODO: implement this
        return routes

    def requestIP6AdvertisedRoutes(self, interface):
        routes = []
        # TODO: implement this
        return routes

class ThreadNetwork(Network):
    def __init__(self, name, meshLocalPrefix):
        Network.__init__(self, name)
        self.meshLocalPrefix = toIP6Prefix(meshLocalPrefix, 'Unable to initialize thread network %s: ' % name)
        if not self.meshLocalPrefix or self.meshLocalPrefix.prefixlen != 64:
            raise ConfigException('Invalid mesh local prefix specified for network %s' % name)
        self.ip4Supported = False
        self.usesMAC64 = True

    def requestIP6AutoConfig(self, interface):

        if interface.network != self:
            raise ConfigException('Attempt to auto-configure IPv6 interface %s which is not on network %s' % (interface.ifName, self.name))

        # Form an /64 interface address for the node with an IID derived from the node id. This is the
        # node's mesh-local address.
        addr = makeIP6InterfaceAddress(self.meshLocalPrefix, macAddr=interface.macAddress, prefixLen=64, macIs64Bit=True)

        # Return a single auto-config response containing the node address. 
        return [
            IP6AutoConfigInfo(
                addresses = [ addr ],
                delegatedPrefixes = [ ],
                defaultGateway= None,
                dnsServers = [ ]
            )
        ]

    def summary(self, prefix=''):
        s = Network.summary(self, prefix)
        s += '%s  Mesh Local Prefix: %s\n' % (prefix, self.meshLocalPrefix)
        return s
        
class LegacyThreadNetwork(Network):
    def __init__(self, name):
        Network.__init__(self, name)
        self.ip4Supported = False
        self.usesMAC64 = True

class CellularNetwork(WiFiNetwork):
    pass

class HostNetwork(Network):
    def __init__(self, name):
        Network.__init__(self, name)

    def buildNetwork(self):
        # Do nothing.
        pass

    def clearNetwork(self):
        # Do nothing.
        pass
        
    def summary(self, prefix=''):
        # Don't show host networks.  They only exist to give host interface objects
        # something to point at.
        return ''
        

#===============================================================================
# Node Classes
#===============================================================================

class Node():
    def __init__(self, name, isHostNode=False):
        self.name = name
        self.isHostNode = isHostNode
        if isHostNode:
            self.nsName = None
        else:
            self.nsName = namePrefix + name
        self.nodeIndex = len(nodes) + 1 # NOTE: nodeIndexes must be > 0
        self.nextInterfaceIndex = 1
        self.configState = "incomplete"
        self.interfaces = []
        self.ip4Enabled = True
        self.ip4DefaultGateway = None
        self.ip4DefaultInterface = None
        self.ip4Routes = []
        self.ip4DNSServers = None
        self.ip6Enabled = True
        self.ip6DefaultGateway = None
        self.ip6DefaultInterface = None
        self.ip6Routes = []
        self.ip6DNSServers= None
        self.useTapInterfaces = False
        self.hostAliases = []
        
        # Add the new node to the list of nodes associated with the active simnet object.
        nodes.append(self)
        
    def addInterface(self, network, name):
        
        # If a network name was specified (verses an actual Network object), find the associated Network object.
        if isinstance(network, str):
            network = Network.findNetwork(network, 'Unable to add interface to node %s: ' % self.name)

        existingInterface = None

        # If the node is implemented by the host and an network has NOT been specified...
        if self.isHostNode and network == None:
            
            # If the specified interface name is 'host-default', substitute in the name of the host's
            # default interface, as determined by examining the host's default IPv4 route.
            if name == 'host-default':
                name = next((r['dev'] for r in getHostRoutes() if r['dest'] == 'default'), None)
                if not name:
                    raise ConfigException('Unable to determine host default interface')
            
            # Search the list of existing host interfaces for one that matches the specified
            # interface name. If a match is found then this interface will be implemented by the
            # existing host interface, rather than being a virtual interface. 
            existingInterface = next((i for i in getHostInterfaces() if i['dev'] == name), None)

        # Construct the appropriate type of interface.
        if existingInterface:
            i = ExistingHostInterface(self, existingInterface)
        elif self.useTapInterfaces:
            i = TapInterface(self, network, name)
        else:
            i = VirtualInterface(self, network, name)

        return i
    
    def configureAddresses(self):

        if self.configState == "complete":
            return
        
        if self.configState == "inprogress":
            raise ConfigException('Loop detected in automatic address assignment')

        self.configState = "inprogress"
        
        self._configureAddresses()
        
        self.configState = "complete"
        
    def _configureAddresses(self):

        # Perform configuration for each interface...
        for i in self.interfaces:
            
            # If IPv4 is enabled for the interface and the attached network supports IPv4...
            if i.ip4Enabled and i.network.ip4Supported:
                
                # If IPv4 automatic configuration is enabled...
                if i.autoConfigIP4:
            
                    # If an IPv4 address has not been assigned query the associated network for
                    # IPv4 auto-config information and configure the interface to used the returned
                    # address. Fail if unsuccessful.
                    if i.ip4Address == None:
                        i.ip4Address = i.requestIP4AutoConfig().address
                        if i.ip4Address == None:
                            raise ConfigException('Unable to automatically assign IPv4 address to node %s, interface %s' % (self.name, i.ifName))

                    # Auto configure the IPv4 default gateway if necessary. 
                    if self.ip4DefaultGateway == None:
                        self.ip4DefaultGateway = i.requestIP4AutoConfig().defaultGateway
                        self.ip4DefaultInterface = i
    
                    # Auto configure the IPv4 DNS servers if necessary.
                    if self.ip4DNSServers == None:
                        self.ip4DNSServers = i.requestIP4AutoConfig().dnsServers
                    
                # If the assigned IPv4 interface address is a network address (i.e. the node-specific
                # bits of the address are all zeros), then assign a host interface address within
                # specified network based on the node's index.
                if i.ip4Address != None and isNetworkAddress(i.ip4Address):
                    i.ip4Address = makeIP4IntefaceAddress(i.ip4Address.network, self.nodeIndex) 

            # If IPv6 is enabled for the interface and the attached network supports IPv6...
            if i.ip6Enabled and i.network.ip6Supported:

                # Form the interface's link-local address if it hasn't already been specified.
                if i.ip6LinkLocalAddress == None:
                    i.ip6LinkLocalAddress = makeIP6InterfaceAddress(llaPrefix, macAddr=i.macAddress, macIs64Bit=i.usesMAC64, prefixLen=64)

                if i.ip6LinkLocalAddress not in i.ip6Addresses:
                    i.ip6Addresses = [ i.ip6LinkLocalAddress ] + i.ip6Addresses

                # If IPv6 automatic configuration is enabled...
                if i.autoConfigIP6:
                    
                    # Query the associated network for IPv6 auto-config information. Delegate to the node subclass
                    # to filter the responses.  For each remaining response...
                    for autoConfig in self.filterIP6AutoConfig(i, i.requestIP6AutoConfig(i)):
                        
                        # Add the auto-config addresses to the list of addresses for the interface.
                        i.ip6Addresses += autoConfig.addresses
                        
                        # Auto configure the IPv6 default gateway if necessary. 
                        if self.ip6DefaultGateway == None:
                            self.ip6DefaultGateway = autoConfig.defaultGateway
                            self.ip6DefaultInterface = i
        
                        # Auto configure the IPv4 DNS servers if necessary.
                        if self.ip6DNSServers == None:
                            self.ip6DNSServers = autoConfig.dnsServers

                        # Save a list of the prefixes that have been delegated to this interface.                        
                        if autoConfig.delegatedPrefixes:
                            i.delegatedIP6Prefixes += autoConfig.delegatedPrefixes
                        
                        # Save any further routes associated with the auto-config response
                        if autoConfig.furtherRoutes:
                            self.ip6Routes += autoConfig.furtherRoutes

                # If any of the assigned IPv6 addresses are network addresses (i.e. the IID is
                # all zeros), then convert them to host addresses using the interface MAC address
                # as the IID.
                for n in range(len(i.ip6Addresses)):
                    a = i.ip6Addresses[n]
                    if isNetworkAddress(a):
                        a = makeIP6InterfaceAddress(a.network, macAddr=i.macAddress, macIs64Bit=i.usesMAC64) 
                    i.ip6Addresses[n] = a

    def configureRoutes(self):

        # Add default routes...
        if self.ip4DefaultGateway:
            self.ip4Routes.append(
                IPRoute(
                    dest = None,
                    interface = self.ip4DefaultInterface,
                    via = self.ip4DefaultGateway
                )
            )
        if self.ip6DefaultGateway:
            self.ip6Routes.append(
                IPRoute(
                    dest = None,
                    interface = self.ip6DefaultInterface,
                    via = self.ip6DefaultGateway
                )
            )
        
        # Perform route configuration for each interface...
        for i in self.interfaces:
            
            # If IPv4 is enabled for the interface and the attached network supports IPv4
            # request advertised IPv4 routes and add them to the node's route table.
            if i.ip4Enabled and i.network.ip4Supported:
                self.ip4Routes += i.network.requestIP4AdvertisedRoutes(i)
        
            # If IPv6 is enabled for the interface and the attached network supports IPv6
            # request advertised IPv6 routes and add them to the node's route table.
            if i.ip4Enabled and i.network.ip4Supported:
                self.ip6Routes += i.network.requestIP6AdvertisedRoutes(i)

    def requestIP4AutoConfig(self, interface):
        return None

    def requestIP6AutoConfig(self, interface):
        return None

    def requestIP4AdvertisedRoutes(self, interface):
        return []

    def requestIP6AdvertisedRoutes(self, interface):
        return []

    def filterIP6AutoConfig(self, interface, autoConfigs):
        return autoConfigs
    
    def buildNode(self, hostsTable):

        global logIndent

        logAction('Building node: %s' % self.name)        
        logIndent += 1
        try:
            self._buildNode()
            self.installHostsTable(hostsTable)
            self.installResolverConfig()
        finally:
            logIndent -= 1

    def _buildNode(self):

        # If the node is not implemented by the host...        
        if not self.isHostNode:
            
            # Create a network namespace for the node.
            addNamespace(self.nsName)

            # Create a /etc/netns directory for the node.
            etcDirName = os.path.join('/etc/netns', self.nsName)
            if not os.path.exists(etcDirName):
                logAction('Making directory: %s' % etcDirName)
                os.makedirs(etcDirName)
        
        # If the node is not implemented by the host, enable its loopback interface. (If the node
        # is implemented by the host, presumably the loopback interface is already enabled).
        if not self.isHostNode:
            enableInterface('lo', nsName=self.nsName)

        # Build each of the node's interfaces...
        for i in self.interfaces:
            i.buildInterface()

        # Add routes for the node.
        if not self.useTapInterfaces:
            for r in self.ip4Routes + self.ip6Routes:
                addRoute(
                    dest = r.destination,
                    dev = r.interface.ifName if r.interface != None else None, 
                    via = r.via,
                    options = r.options,
                    nsName = self.nsName
                )

    def clearNode(self):

        # Clear the node's interfaces.
        if self.isHostNode:    
            for i in self.interfaces:
                i.clearInterface()

        # Delete the node's namespace.
        if not self.isHostNode:    
            deleteNamespace(self.nsName)

        # Delete the node's namespace /etc directory
        if not self.isHostNode:    
            etcDirName = os.path.join('/etc/netns', self.nsName)
            if os.path.isdir(etcDirName):
                logAction('Deleting directory: %s' % etcDirName)
                shutil.rmtree(etcDirName)

    def isNodeBuilt(self):
        if self.isHostNode:
            return True
        else:
            nsList = getNamespaces()
            return self.nsName in nsList

    def getHostsEntries(self):
        hostEntries = {}
        nodeIP4Addrs = []
        nodeIP6Addrs = []

        for i in self.interfaces:
            if i.ip4Address:
                nodeIP4Addrs.append(i.ip4Address.ip)
                hostEntries[self.name + '-' + i.ifName + '-ip4'] = [ i.ip4Address.ip ]
            if len(i.ip6Addresses) > 0:
                addrs = [ a.ip for a in i.ip6Addresses if not a.is_link_local ]
                nodeIP6Addrs += addrs
                hostEntries[self.name + '-' + i.ifName + '-ip6'] = preferGlobalAddresses(addrs)
        
        # Filter address lists to prefer global addresses over non-global ones.
        nodeIP4Addrs = preferGlobalAddresses(nodeIP4Addrs)
        nodeIP6Addrs = preferGlobalAddresses(nodeIP6Addrs)
        
        hostEntries[self.name] = nodeIP4Addrs + nodeIP6Addrs
        
        for alias in self.hostAliases:
            hostEntries[alias] = nodeIP4Addrs + nodeIP6Addrs
        
        return hostEntries

    def installHostsTable(self, hostsTable):
        if not self.isHostNode:
            etcDirName = os.path.join('/etc/netns', self.nsName)
            hostsFileName = os.path.join(etcDirName, 'hosts')
            logAction('Installing hosts file: %s' % hostsFileName)
            with open(hostsFileName, 'w') as f:
                f.write(hostsTable)

    def installResolverConfig(self):
        if not self.isHostNode:
            etcDirName = os.path.join('/etc/netns', self.nsName)
            resolvConfFileName = os.path.join(etcDirName, 'resolv.conf')
            logAction('Installing resolv.conf file: %s' % resolvConfFileName)
            with open(resolvConfFileName, 'w') as f:
                if self.ip4DNSServers:
                    for a in self.ip4DNSServers:
                        f.write('nameserver %s\n' % a)
                if self.ip6DNSServers:
                    for a in self.ip6DNSServers:
                        f.write('nameserver %s\n' % a)
        
    def setEnvironVars(self, environ):
        environ['SN_NODE_NAME'] = self.name
        if not self.isHostNode:
            environ['SN_NAMESPACE'] = self.nsName
        environ['SN_NODE_INDEX'] = str(self.nodeIndex)

    def getLwIPConfig(self):
        lwipConfig = ''
        for i in self.interfaces:
            if isinstance(i, TapInterface):
                lwipConfig += i.getLwipConfig()
        if self.ip4DefaultGateway:
            lwipConfig += '--ip4-default-gw %s ' % self.ip4DefaultGateway
        if self.ip4DNSServers:
            lwipConfig += '--dns-server %s ' % (','.join(self.ip4DNSServers))
        # TODO: handle routes
        return lwipConfig
        
    def summary(self, prefix=''):
        s  = '%sNode "%s" (%s ' % (prefix, self.name, self.__class__.__name__,)
        if self.isHostNode:
            s += 'implemented by host):\n'
        else:
            s += 'implemented by namespace %s):\n' % self.nsName
        s += '%s  Node Index: %d\n' % (prefix, self.nodeIndex)
        s += '%s  Interfaces (%d):\n' % (prefix, len(self.interfaces))
        for i in self.interfaces:
            s += i.summary(prefix + '    ')
        s += '%s  IPv4 Routes:%s\n' % (prefix, ' None' if len(self.ip4Routes) == 0 else '')
        for r in self.ip4Routes:
            s += r.summary(prefix + '    ')
        s += '%s  IPv6 Routes:%s\n' % (prefix, ' None' if len(self.ip6Routes) == 0 else '')
        for r in self.ip6Routes:
            s += r.summary(prefix + '    ')
        s += '%s  IPv4 DNS Servers: %s\n' % (prefix, 'None' if self.ip4DNSServers == None or len(self.ip4DNSServers) == 0 else ', '.join(self.ip4DNSServers))
        s += '%s  IPv6 DNS Servers: %s\n' % (prefix, 'None' if self.ip6DNSServers == None or len(self.ip6DNSServers) == 0 else ', '.join(self.ip6DNSServers))
        return s

class Gateway(Node):
    def __init__(self, name, 
                 outsideNetwork=None, outsideInterface='outside', outsideIP4Address=None, outsideIP6Addresses=[],
                 insideNetwork=None, insideIP4Subnet=None, insideInterface='inside', insideIP6Subnets=[], 
                 isIP4DefaultGateway=True, isIP6DefaultGateway=True, 
                 ip4DNSServers=['8.8.8.8'], ip6DNSServers=None, hostAliases=[],
                 useHost=False):
        Node.__init__(self, name, isHostNode=useHost)

        if not useHost:
            if not outsideNetwork:
                raise ConfigException('Must specify outside network for gateway %s' % (name))
            if not insideNetwork:
                raise ConfigException('Must specify inside network for gateway %s' % (name))

        self.outsideInterface = self.addInterface(outsideNetwork, name=outsideInterface)
        if not isinstance(self.outsideInterface, ExistingHostInterface):
            if not isinstance(self.outsideInterface.network, Internet) and not isinstance(self.outsideInterface.network, WiFiNetwork):
                raise ConfigException('Incompatible network %s attached to outside interface of gateway %s' % (outsideNetwork, name))
            self.outsideInterface.ip4Address = toIP4InterfaceAddress(outsideIP4Address, 'Unable to assign IPv4 address to node %s, outside interface %s: ' % (name, self.outsideInterface.ifName))
            self.outsideInterface.ip6Addresses = [ toIP6InterfaceAddress(a, 'Unable to assign IPv6 address to node %s, outside interface %s: ' % (name, self.outsideInterface.ifName)) for a in outsideIP6Addresses ]
            self.outsideInterface.autoConfigIP4 = True
            self.outsideInterface.autoConfigIP6 = (self.outsideInterface.ip6Addresses != None and len(self.outsideInterface.ip6Addresses) == 0)
        else:
            if outsideIP4Address != None or len(outsideIP6Addresses) != 0:
                raise ConfigException('Cannot specify addresses for host interface %s on node %s' % (self.outsideInterface.ifName, name))

        self.insideInterface = self.addInterface(insideNetwork, name=insideInterface)
        if not isinstance(self.insideInterface, ExistingHostInterface):
            if not isinstance(self.insideInterface.network, WiFiNetwork):
                raise ConfigException('Incompatible network %s attached to inside interface of gateway %s' % (insideNetwork, name))
            self.insideInterface.ip4Address = toIP4InterfaceAddress(insideIP4Subnet, 'Unable to assign IPv4 address to node %s, inside interface %s: ' % (name, self.insideInterface.ifName))
            self.insideInterface.ip6Addresses = [ toIP6InterfaceAddress(a, 'Unable to assign IPv6 address to node %s, inside interface %s: ' % (name, self.insideInterface.ifName)) for a in insideIP6Subnets ]
            self.insideInterface.autoConfigIP4 = False
            self.insideInterface.autoConfigIP6 = False
        else:
            if insideIP4Subnet != None or len(insideIP6Subnets) != 0:
                raise ConfigException('Cannot specify addresses for host interface %s on node %s' % (self.insideInterface.ifName, name))

        self.isIP4DefaultGateway = isIP4DefaultGateway
        self.isIP6DefaultGateway = isIP6DefaultGateway
        
        if self.insideInterface.ip4Address != None and self.insideInterface.ip4Address.network.prefixlen < 32:
            self.advertisedIP4Subnet = self.insideInterface.ip4Address.network
        else:
            self.advertisedIP4Subnet = None
            
        if self.insideInterface.ip6Addresses != None:
            for a in self.insideInterface.ip6Addresses:
                if a.network.prefixlen < 128:
                    self.insideInterface.advertisedIP6Prefixes.append(a.network)

        self.ip4DNSServers = ip4DNSServers
        self.ip6DNSServers = ip6DNSServers
        
        self.hostAliases = hostAliases

    def _configureAddresses(self):
        Node._configureAddresses(self)
        
        if self.insideInterface.ip6Enabled and self.insideInterface.network.ip6Supported:
            
            # Assign an address to the inside interface for all prefixes that have been delegated to the outside interface. 
            self.insideInterface.ip6Addresses += [ 
                makeIP6InterfaceAddress(p, subnetNum=0, macAddr=self.insideInterface.macAddress, prefixLen=64) for p in self.outsideInterface.delegatedIP6Prefixes
            ]
            
            # On the inside interface, advertise a /64 prefix for all prefixes that have been delegated to the outside interface.
            self.insideInterface.advertisedIP6Prefixes += [
                makeIP6Prefix(p, prefixLen=64) for p in self.outsideInterface.delegatedIP6Prefixes
            ]

    def requestIP4AutoConfig(self, interface):
        if interface.network == self.insideInterface.network and self.advertisedIP4Subnet:
            return IP4AutoConfigInfo(
                address = makeIP4IntefaceAddress(self.advertisedIP4Subnet, interface.node.nodeIndex, self.advertisedIP4Subnet.prefixlen),
                defaultGateway = self.insideInterface.ip4Address.ip if self.isIP4DefaultGateway else None,
                dnsServers = self.ip4DNSServers
            )
        else:
            return None

    def requestIP6AutoConfig(self, interface):
        
        # If the node hasn't been configured, do it now.  This ensures that prefix delegation to the
        # gateway is complete before we attempt to assign addresses to nodes on the inside network.
        if self.configState == "incomplete":
            self.configureAddresses()
        
        if interface.network == self.insideInterface.network:
            return IP6AutoConfigInfo(
                addresses = [ makeIP6InterfaceAddress(p, macAddr=interface.macAddress, prefixLen=64) for p in self.insideInterface.advertisedIP6Prefixes ],
                defaultGateway = self.insideInterface.ip6LinkLocalAddress.ip if self.isIP6DefaultGateway else None,
                furtherRoutes = [
                    IPRoute(
                        dest = a,
                        interface = interface,
                        via = self.insideInterface.ip6LinkLocalAddress.ip
                    ) for a in self.outsideInterface.ip6Addresses if not a.is_link_local
                ],
                dnsServers = self.ip6DNSServers
                # TODO: return delegated prefixes
            )
        else:
            return None

    def requestIP6AdvertisedRoutes(self, interface):
        if interface.network == self.outsideInterface.network:
            return self.outsideInterface.delegatedIP6Prefixes
        else:
            return []

    def _buildNode(self):
        Node._buildNode(self)
        
        # If the node is implemented using a namespace (vs the host) initialize the simnet
        # iptables configuration within the namespace.
        if not self.isHostNode:
            initSimnetIPTables(self.nsName)

        # If the outside interface supports IPv4...
        if self.outsideInterface.ip4Enabled and self.outsideInterface.ip4Address:
            
            # Enable IPv4 NAT between the inside and outside interfaces.
            enableIP4NAT(self.insideInterface.ifName, self.outsideInterface.ifName, self.nsName)
            
            # Enable IPv4 routing for the node
            setSysctl('net.ipv4.conf.all.forwarding', 1, nsName=self.nsName)
            
        # If the outside interface supports IPv6...
        if self.outsideInterface.ip6Enabled and len(self.outsideInterface.ip6Addresses) > 0:

            # Enable Ipv6 forwarding between the inside and outside interfaces.     
            enableIP6Forwarding(self.insideInterface.ifName, self.outsideInterface.ifName, self.nsName)

            # Enable IPv6 routing for the node
            setSysctl('net.ipv6.conf.all.forwarding', 1, nsName=self.nsName)

    def clearNode(self):
        Node.clearNode(self)

        # Clear various configuration if this is a host node...
        # (No need to do this for namespace nodes, since all configuration is cleared
        # when the namespace is deleted).        
        if self.isHostNode:
        
            # Disable IPv4 routing for the node.
            setSysctl('net.ipv4.conf.all.forwarding', 0, nsName=self.nsName)
            
            # Disable IPv6 routing for the node
            setSysctl('net.ipv6.conf.all.forwarding', 0, nsName=self.nsName)

    def getHostsEntries(self):
        hostsEntries = Node.getHostsEntries(self)
        nameEntries = []
        if self.name + '-outside-ip4' in hostsEntries:
            nameEntries += hostsEntries[self.name + '-outside-ip4']
        if self.name + '-outside-ip6' in hostsEntries:
            nameEntries += hostsEntries[self.name + '-outside-ip6']
        if len(nameEntries) > 0:
            hostsEntries[self.name] = nameEntries
        return hostsEntries
        
class WeaveDevice(Node):
    def __init__(self, name, 
                 wifiNetwork=None, wifiInterface=None,
                 threadNetwork=None, threadInterface=None,
                 legacyNetwork=None, legacyInterface=None,
                 cellularNetwork=None, cellularInterface=None,
                 weaveNodeId=None, weaveFabricId=None, isWeaveBorderGateway=False, advertiseWiFiPrefix=True, 
                 hostAliases=[], useLwIP=False, useHost=False):

        if useHost and useLwIP:
            raise ConfigException('Cannot specify both useHost and useLwIP for Weave node %s' % (name))
        
        Node.__init__(self, name, isHostNode=useHost)
        
        self.weaveNodeId = weaveNodeId if weaveNodeId != None else self.nodeIndex
        self.weaveFabricId = weaveFabricId if weaveFabricId != None else defaultWeaveFabricId
        self.fabricAddrGlobalId = self.weaveFabricId % ((1 << 40) - 1) # bottom 40 bits of fabric id
        self.advertiseWiFiPrefix = advertiseWiFiPrefix
        self.useLwIP = useLwIP
        self.useTapInterfaces = useLwIP
        
        if wifiNetwork or wifiInterface:
            if not wifiInterface:
                wifiInterface = 'wifi' if not useLwIP else 'et0'
            self.wifiInterface = self.addInterface(wifiNetwork, name=wifiInterface)
            if not isinstance(self.wifiInterface.network, (WiFiNetwork, HostNetwork)):
                raise ConfigException('Incompatible network %s attached to wifi interface of node %s' % (self.wifiInterface.network.name, name))
        else:
            self.wifiInterface = None
            
        if threadNetwork or threadInterface:
            if not threadInterface:
                threadInterface = 'thread' if not useLwIP else 'th0'
            self.threadInterface = self.addInterface(threadNetwork, name=threadInterface)
            if not isinstance(self.threadInterface.network, (ThreadNetwork, HostNetwork)):
                raise ConfigException('Incompatible network %s attached to thread interface of node %s' % (self.threadInterface.network.name, name))
        else:
            self.threadInterface = None
        
        if legacyNetwork or legacyInterface:
            if not legacyInterface:
                legacyInterface = 'legacy' if not useLwIP else 'al0'
            self.legacyInterface = self.addInterface(legacyNetwork, name=legacyInterface)
            if not isinstance(self.legacyInterface.network, (LegacyThreadNetwork, HostNetwork)):
                raise ConfigException('Incompatible network %s attached to legacy thread interface of node %s' % (self.legacyInterface.network.name, name))
        else:
            self.legacyInterface = None
        
        if cellularNetwork or cellularInterface:
            if not cellularInterface:
                cellularInterface = 'cell' if not useLwIP else 'cl0'
            self.cellularInterface = self.addInterface(cellularNetwork, name=cellularInterface)
            if not isinstance(self.cellularInterface.network, (WiFiNetwork, Internet, HostNetwork)):
                raise ConfigException('Incompatible network %s attached to cell interface of node %s' % (self.cellularInterface.network.name, name))
        else:
            self.cellularInterface = None
            
        self.hostAliases = hostAliases

    def _configureAddresses(self):
        Node._configureAddresses(self)
        
        if self.wifiInterface:
            self.wifiWeaveAddr = self.weaveInterfaceAddress(subnetNum=1)
            self.wifiInterface.ip6Addresses.append(self.wifiWeaveAddr)

        if self.threadInterface:
            self.threadMeshAddr = self.threadInterface.ip6Addresses[1]
            self.threadWeaveAddr = self.weaveInterfaceAddress(subnetNum=6)
            self.threadInterface.ip6Addresses.append(self.threadWeaveAddr)
        
        if self.legacyInterface:
            self.legacyWeaveAddr = self.weaveInterfaceAddress(subnetNum=2)
            self.legacyInterface.ip6Addresses.append(self.legacyWeaveAddr)

    def requestIP6AutoConfig(self, interface):
        if self.wifiInterface and interface.network == self.wifiInterface.network and self.advertiseWiFiPrefix:
            wifiPrefix = self.weaveSubnetPrefix(subnetNum=1)
            return IP6AutoConfigInfo(
                addresses = [ makeIP6InterfaceAddress(wifiPrefix, macAddr=interface.macAddress) ]
            )
        else:
            return None

    def filterIP6AutoConfig(self, interface, autoConfigs):
        # On the WiFi interface, only accept auto-config addresses that are global.  Among other this, 
        # this prevents Weave nodes in one fabric from picking up addresses from nodes in other fabrics.
        if interface == self.wifiInterface:
            for autoConfig in autoConfigs:
                autoConfig.addresses = [ a for a in autoConfig.addresses if a.is_global ]
        return autoConfigs

    def weaveSubnetPrefix(self, subnetNum):
        return makeIP6Prefix(ulaPrefix, networkNum=self.fabricAddrGlobalId, subnetNum=subnetNum, prefixLen=64)
    
    def weaveInterfaceAddress(self, subnetNum):
        subnetPrefix = self.weaveSubnetPrefix(subnetNum)
        
        # As a convenience to testing, interface addresses for Weave nodes with a node id less than 65536
        # are considered 'local', and therefore have their universal/local bit is set to zero.  This simplifies
        # the string representation of the corresponding IPv6 addresses. For example a WiFi ULA for a node
        # with a node id of 10 would be FD00:0:1:1::A, instead of FD00:0:1:1:0200::A.  This behavior matches
        # the behavior of the C++ Weave code.
        if self.weaveNodeId < 65536:
            iid = self.weaveNodeId              # 'test' node id, generate 'local' iid
        else:
            iid = self.weaveNodeId | (1 << 57)  # 'real' node id, generate 'universal' iid

        a = makeIP6InterfaceAddress(subnetPrefix, iid=iid, prefixLen=64)
        return a 

    def getHostsEntries(self):
        hostsEntries = { }
        
        # Add interface-name specific entries (<name>-<interface>-ip4, -ip6, -weave, -mesh).
        if self.wifiInterface:
            if self.wifiInterface.ip4Address:
                hostsEntries[self.name + '-wifi-ip4'] = [ self.wifiInterface.ip4Address.ip ]
            hostsEntries[self.name + '-wifi-ip6'] = [ a.ip for a in self.wifiInterface.ip6Addresses if a.is_global ]
            hostsEntries[self.name + '-wifi-weave'] = [ self.wifiWeaveAddr.ip ]
        if self.threadInterface:
            hostsEntries[self.name + '-thread-weave'] = [ self.threadWeaveAddr.ip ]
            hostsEntries[self.name + '-thread-mesh'] = [ self.threadMeshAddr.ip ]
        if self.legacyInterface:
            hostsEntries[self.name + '-legacy-weave'] = [ self.legacyWeaveAddr.ip ]
        if self.cellularInterface:
            if self.cellularInterface.ip4Address:
                hostsEntries[self.name + '-cell-ip4'] = [ self.cellularInterface.ip4Address.ip ]
            hostsEntries[self.name + '-cell-ip6'] = [ a.ip for a in self.cellularInterface.ip6Addresses if a.is_global ]

        # Add an entry for the primary name of the node (<name>).
        primaryAddrs = []
        if self.wifiInterface:
            if self.wifiInterface.ip4Address:
                primaryAddrs.append(self.wifiInterface.ip4Address.ip)
            primaryAddrs += [ a.ip for a in self.wifiInterface.ip6Addresses if a.is_global ]
        elif self.threadInterface:
            primaryAddrs.append(self.threadWeaveAddr.ip)
        elif self.cellularInterface:
            if self.cellularInterface.ip4Address:
                primaryAddrs.append(self.cellularInterface.ip4Address.ip)
            primaryAddrs += [ a.ip for a in self.cellularInterface.ip6Address if a.is_global ]
        elif self.legacyInterface:
            primaryAddrs.append(self.legacyWeaveAddr.ip)
        hostsEntries[self.name] = primaryAddrs
        for alias in self.hostAliases:
            hostsEntries[alias] = primaryAddrs
            
        # Add an entry for the primary weave name of the node (<name>-weave).
        primaryWeaveAddr = None
        if self.wifiInterface:
            primaryWeaveAddr = self.wifiWeaveAddr
        elif self.threadInterface:
            primaryWeaveAddr = self.threadWeaveAddr
        elif self.legacyInterface:
            primaryWeaveAddr = self.legacyWeaveAddr
        if primaryWeaveAddr:
            hostsEntries[self.name + '-weave'] = [ primaryWeaveAddr.ip ]

        return hostsEntries

    def setEnvironVars(self, environ):
        Node.setEnvironVars(self, environ)
        weaveConfig  = '--node-id %016X ' % self.weaveNodeId
        weaveConfig += '--fabric-id %016X ' % self.weaveFabricId
        if self.wifiInterface:
            weaveConfig += '--default-subnet 1 '
        elif self.threadInterface:
            weaveConfig += '--default-subnet 6 '
        elif self.legacyInterface:
            weaveConfig += '--default-subnet 2 '
        if self.useLwIP:
            weaveConfig += self.getLwIPConfig()
        environ['WEAVE_CONFIG'] = weaveConfig

    def summary(self, prefix=''):
        s = Node.summary(self, prefix)
        s += '%s  Weave Node Id: %016X\n' % (prefix, self.weaveNodeId)
        s += '%s  Weave Fabric Id: %016X\n' % (prefix, self.weaveFabricId)
        return s

class SimpleNode(Node):
    def __init__(self, name, network, ip4Address=None, ip6Addresses=[], autoConfigIP4=True, autoConfigIP6=True, useHost=False, hostAliases=[]):
        Node.__init__(self, name, isHostNode=useHost)
        
        i = self.addInterface(network, name='eth0')
        if not isinstance(i.network, (WiFiNetwork, Internet)):
            raise ConfigException('Incompatible network %s attached to interface of node %s' % (i.network.name, name))
        
        i.ip4Address = toIP4InterfaceAddress(ip4Address, 'Unable to assign IPv4 address to node %s, interface %s: ' % (name, i.ifName))
        i.ip6Addresses = [ toIP6InterfaceAddress(a, 'Unable to assign IPv6 address to node %s, interface %s: ' % (name, i.ifName)) for a in ip6Addresses ]
        i.autoConfigIP4 = autoConfigIP4
        i.autoConfigIP6 = autoConfigIP6
        
        self.hostAliases = hostAliases


#===============================================================================
# NodeInterface Classes
#===============================================================================

class NodeInterface():
    def __init__(self, node, network, name):
        self.node = node
        self.network = network
        self.ifIndex = len(node.interfaces)
        if len(name) > 15:
            raise ConfigException('Interface name for node %s is too long: %s' % (node.name, name))
        self.ifName = name
        self.ip4Enabled = True
        self.ip6Enabled = True
        self.autoConfigIP4 = True
        self.autoConfigIP6 = True
        self.ip4Address = None
        self.ip6Addresses = []
        self.ip6LinkLocalAddress = None
        self.delegatedIP6Prefixes = []
        self.advertisedIP4Subnet = None
        self.advertisedIP6Prefixes = []
        self.ip4AutoConfig = None
        self.ip6AutoConfig = None
        self.isTapInterface = False
        
        # Form a MAC address for the interface from the base MAC address, the node index and the interface index.
        # Use a 48-bit or 64-bit MAC based on the nature of the connected network.
        self.macAddress = (baseMAC64Address if network.usesMAC64 else baseMAC48Address) + node.nodeIndex * 16 + self.ifIndex
        self.usesMAC64 = network.usesMAC64
        
        # Attach the interface to the node and to the network.
        node.interfaces.append(self)
        network.attachedInterfaces.append(self)
    
    def requestIP4AutoConfig(self):
        if self.ip4AutoConfig == None:
            self.ip4AutoConfig = self.network.requestIP4AutoConfig(self)
        if self.ip4AutoConfig == None:
            self.ip4AutoConfig = IP4AutoConfigInfo()
        return self.ip4AutoConfig

    def requestIP6AutoConfig(self, interface):
        if self.ip6AutoConfig == None:
            self.ip6AutoConfig = self.network.requestIP6AutoConfig(self)
        return self.ip6AutoConfig

    def summary(self, prefix=''):
        s  = '%s%s (%s):\n' % (prefix, self.ifName, self.typeSummary())
        s += '%s  MAC Address: %012X\n' % (prefix, self.macAddress)
        if self.ip4Enabled:
            s += '%s  IPv4 Address: %s\n' % (prefix, self.ip4Address if self.ip4Address else 'None')
        if self.ip6Enabled:
            s += '%s  IPv6 Addresses:%s\n' % (prefix, ('' if (self.ip6Addresses != None and len(self.ip6Addresses) > 0) else ' None'))
            for a in self.ip6Addresses:
                s += '%s    %s\n' % (prefix, a)
        return s


class VirtualInterface(NodeInterface):
    def __init__(self, node, network, name):

        # If the node is implemented by the host, include the node index in the interface name to
        # ensure it is unique.
        if node.isHostNode:
            name = name + '-' + str(node.nodeIndex)

        # Initialize the base class.
        NodeInterface.__init__(self, node, network, name)
        
        # Form the peer interface name from the network, node and interface indexes.  (Note that interface
        # names are limited to 16 characters, making it difficult to use a more intuitive naming scheme here).
        self.peerIfName = namePrefix + str(network.networkIndex) + '-' + str(node.nodeIndex) + '-' + str(self.ifIndex)

    def buildInterface(self):
        
        # Create a pair of virtual ethernet interfaces for connecting the node to the target network.
        # Use a temporary name for the first interface.
        tmpInterfaceName = namePrefix + 'tmp-' + str(os.getpid()) + str(random.randint(0, 100))
        createVETHPair(tmpInterfaceName, self.peerIfName)
        
        # Set the MAC address of the node interface.  If we are simulating an interface that uses a 64-bit
        # MAC, form a MAC-48 approximation of the 64-bit value.
        if self.usesMAC64:
            macAddress = ((self.macAddress & 0xFFFFFF0000000000) >> 16) | (self.macAddress & 0xFFFFFF)
        else:
            macAddress = self.macAddress
        setInterfaceMACAddress(tmpInterfaceName, macAddress)
        
        # Attach the second interface (the peer interface) to the network bridge.
        moveInterfaceToNamespace(self.peerIfName, nsName=self.network.nsName)
        attachInterfaceToBridge(self.peerIfName, self.network.brName, nsName=self.network.nsName)

        # Assign the first interface to the node namespace and rename it to the appropriate name.
        if not self.node.isHostNode:
            moveInterfaceToNamespace(tmpInterfaceName, nsName=self.node.nsName)
        renameInterface(tmpInterfaceName, self.ifName, nsName=self.node.nsName)

        # Enable the virtual interfaces. 
        enableInterface(self.ifName, nsName=self.node.nsName)
        enableInterface(self.peerIfName, nsName=self.network.nsName)

        # Flush any IPv6 link-local addresses automatically created by the network. The appropriate
        # LL addresses will be added later (see below).
        runCmd([ 'ip', '-6', 'addr', 'flush', 'dev', self.ifName, 'scope', 'link' ], nsName=self.node.nsName, 
               errMsg='Unable to flush IPv6 LL addresses on interface %s in namespace %s' % (self.ifName, self.node.nsName))

        # Assign the IPv4 address to the node interface.
        if self.ip4Address != None:
            assignAddressToInterface(self.ifName, self.ip4Address, nsName=self.node.nsName)

        # Assign the IPv6 addresses to the node interface.
        if self.ip6Addresses != None:
            for a in self.ip6Addresses:
                try:
                    assignAddressToInterface(self.ifName, a, nsName=self.node.nsName)
                except CommandException, ex:
                    if 'RTNETLINK answers: File exists' not in ex.message:
                        raise

    def clearInterface(self):
        
        # Delete the virtual interfaces.
        deleteInterface(self.ifName, nsName=self.node.nsName)
        deleteInterface(self.peerIfName, nsName=self.network.nsName)

    def typeSummary(self):
        return 'virtual interface to network "%s" via peer interface %s' % (self.network.name, self.peerIfName)
            

class TapInterface(NodeInterface):
    def __init__(self, node, network, name=None):
    
        # If the node is implemented by the host, include the node index in the interface name to
        # ensure it is unique.
        if node.isHostNode:
            name = name + '-' + str(node.nodeIndex)

        # Initialize the base class.
        NodeInterface.__init__(self, node, network, name)

        # Form the name of the tap device from the node index and the name of the network to which it is attached.
        self.tapDevName = namePrefix + network.name + '-' + str(node.nodeIndex)

    def buildInterface(self):
        
        # Create the tap device
        # TODO: set owner and group
        createTapInterface(self.tapDevName)
        
        # Attach the tap interface to the network bridge.
        moveInterfaceToNamespace(self.tapDevName, nsName=self.network.nsName)
        attachInterfaceToBridge(self.tapDevName, self.network.brName, nsName=self.network.nsName)
        
        # Enable the tap interface.
        enableInterface(self.tapDevName, nsName=self.network.nsName)

    def clearInterface(self):
        
        # Delete the tap device.
        deleteInterface(self.tapDevName, nsName=self.network.nsName)
        
    def typeSummary(self):
        return 'tap device connected to network "%s" via interface %s' % (self.network.name, self.tapDevName)

    def getLwIPConfig(self):
        lwipConfig = '--%s-tap-device %s ' % (i.ifName, i.tapDevName) 
        lwipConfig += '--%s-mac-addr %012X ' % (i.ifName, i.macAddress) # TODO: handle case where MAC is 64-bits
        if i.ip4Enabled and i.ip4Address:
            lwipConfig += '--%s-ip4-addr %s ' % (i.ifName, i.ip4Address)
        if i.ip6Enabled and len(i.ip6Addresses) > 0:
            lwipConfig += '--%s-ip6-addrs %s ' % (i.ifName, ','.join([ str(a) for a in i.ip6Addresses ]))
        return lwipConfig;


class ExistingHostInterface(NodeInterface):
    def __init__(self, node, hostInterfaceConfig):
    
        # Use the existing interface's name.
        name = hostInterfaceConfig['dev']

        # Construct a HostNetwork object to represent the network to which the host interface is attached.  
        # Note that this is just a place-holder object so the interface object has something to point to.
        network = HostNetwork(name + '-net')

        # Initialize the base class.
        NodeInterface.__init__(self, node, network, name)

        # Use the MAC address associated with the existing interface.
        self.macAddress = parseMACAddress(hostInterfaceConfig['address'])

        # Disable auto configuration of addresses.
        self.autoConfigIP4 = False
        self.autoConfigIP6 = False

        # Use the host interface's IPv4 address, if any.
        self.ip4Address = next(iter(getInterfaceAddresses(self.ifName, ipv6=False)), None)

        # Search for a IPv6 link-local on the host interface.  If found, use it instead of creating a new one.
        # If not found, disable IPv6 on the interface.
        ip6LinkLocalAddress = next((a for a in getInterfaceAddresses(self.ifName, ipv6=True) if a.is_link_local), None)
        if ip6LinkLocalAddress:
            self.ip6LinkLocalAddress = ip6LinkLocalAddress
        else:
            self.ip6Enabled = False

    def buildInterface(self):
        
        # Assign the IPv6 addresses to the host interface, excluding the link-local address, which is
        # presumed to already exist.
        if self.ip6Enabled and self.ip6Addresses != None:
            for a in self.ip6Addresses:
                if a != self.ip6LinkLocalAddress:
                    assignAddressToInterface(self.ifName, a, nsName=self.node.nsName)

    def clearInterface(self):

        # Remove all assigned IPv6 addresses from the host interface, excluding the link-local
        # address, which existed on the interface to begin with.
        if self.ip6Enabled and self.ip6Addresses != None:
            for a in self.ip6Addresses:
                if a != self.ip6LinkLocalAddress:
                    removeAddressFromInterface(self.ifName, a, nsName=self.node.nsName)
            
    def typeSummary(self):
        return 'existing host interface'


#===============================================================================
# Utility Classes
#===============================================================================

class IP4AutoConfigInfo:
    def __init__(self, address=None, defaultGateway=None, dnsServers=[]):
        self.address = address
        self.defaultGateway = defaultGateway
        self.dnsServers = dnsServers

class IP6AutoConfigInfo:
    def __init__(self, addresses=[], delegatedPrefixes=None, defaultGateway=None, dnsServers=[], furtherRoutes=[]):
        self.addresses = addresses
        self.delegatedPrefixes = delegatedPrefixes
        self.defaultGateway = defaultGateway
        self.dnsServers = dnsServers
        self.furtherRoutes = furtherRoutes

class IPRoute:
    def __init__(self, dest, interface=None, via=None, options=None):
        self.destination = dest
        self.interface = interface
        self.via = via
        self.options = options

    def summary(self, prefix=''):
        s = '%s%s' % (prefix, self.destination if self.destination != None else 'default')
        if self.interface:
            s += ' interface %s' % (self.interface.ifName)
        if self.via:
            s += ' via %s' % (self.via)
        if self.options:
            for option in self.options.keys():
                s += ' %s %s' % (option, self.options[option])
        s += '\n'
        return s

class CommandException(Exception):
    pass

class ConfigException(Exception):
    pass

class UsageException(Exception):
    pass



#===============================================================================
# Utility Functions
#===============================================================================

def runCmd(cmd, errMsg='', ignoreErr=False, nsName=None):
    if nsName:
        cmd = [ 'ip', 'netns', 'exec', nsName ] + cmd
        if errMsg != '':
            errMsg += ' in namespace %s' % (nsName)
    if errMsg != '':
        errMsg += '\n'
    logAction('Running: ' + ' '.join(cmd))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out, err) = p.communicate()
    if p.returncode != 0 and not ignoreErr:
        err = err.strip()
        err = re.sub('^', '>> ', err)
        raise CommandException(errMsg + 'Command failed: %s\n%s' % (' '.join(cmd), err))
    return out

def getBridges(nsName=None):
    out = runCmd([ 'brctl', 'show' ], nsName=nsName, errMsg='Failed to enumerate network bridges')
    for line in re.split('\n', out):
        if line.startswith('bridge name'):
            continue
        m = re.match('\S+', line)
        if m:
            yield m.group(0)

def addBridge(name, nsName=None):
    runCmd([ 'brctl', 'addbr', name ], nsName=nsName, 
           errMsg='Failed to create bridge %s' % (name))
    
def deleteBridge(name, nsName=None):
    try:
        runCmd([ 'ip', 'link', 'set', name, 'down' ], nsName=nsName,
               errMsg='Failed to disable host interface for network bridge %s' % name)
        runCmd([ 'brctl', 'delbr', name ], nsName=nsName,
               errMsg='Failed to delete network bridge %s' % name)
    except Exception, ex:
        if not 'Cannot find device' in ex.message:
            raise
    
def getNamespaces():
    out = runCmd([ 'ip', 'netns', 'list' ], errMsg='Failed to enumerate network namespaces')
    for line in re.split('\n', out):
        line = line.strip()
        if len(line) == 0:
            continue
        yield line

def addNamespace(name):
    runCmd([ 'ip', 'netns', 'add', name ], errMsg='Failed to create network namespace %s' % (name))

def deleteNamespace(name):
    try:
        runCmd([ 'ip', 'netns', 'del', name ], errMsg='Failed to delete network namespace %s' % name)
    except Exception, ex:
        if not 'No such file or directory' in ex.message:
            raise

def enableInterface(name, nsName=None):
    runCmd([ 'ip', 'link', 'set', name, 'up' ], errMsg='Failed to enable interface %s' % (name), nsName=nsName)

def disableInterface(name, nsName=None):
    runCmd([ 'ip', 'link', 'set', name, 'down' ], errMsg='Failed to disable interface %s' % (name), nsName=nsName)

def createVETHPair(name, peerName):
    runCmd([ 'ip', 'link', 'add', 'name', name, 'type', 'veth', 'peer', 'name', peerName ], errMsg='Unable to create virtual ethernet interface pair %s' % (name))
    time.sleep(0.01)

def createTapInterface(name, user='root', group='root'):
    runCmd([ 'tunctl', '-u', user, '-g', group, '-t', name], errMsg='Unable to create tap interface pair %s' % (name))

def moveInterfaceToNamespace(ifName, nsName=None):
    runCmd([ 'ip', 'link', 'set', ifName, 'netns', nsName ], errMsg='Unable to assign virtual ethernet interface %s to namespace %s' % (ifName, nsName))

def renameInterface(name, newName, nsName=None):
    runCmd([ 'ip', 'link', 'set', name, 'name', newName ], errMsg='Unable to rename interface %s to %s' % (name, newName), nsName=nsName)

def deleteInterface(name, nsName=None):
    try:
        runCmd([ 'ip', 'link', 'del', name,  ], errMsg='Unable to delete interface %s' % (name), nsName=nsName)
    except Exception, ex:
        if not 'Cannot find device' in ex.message and not 'Cannot open network namespace: No such file or directory' in ex.message:
            raise

def assignAddressToInterface(ifName, addr, nsName=None):
    runCmd([ 'ip', 'addr', 'add', str(addr), 'dev', ifName ], errMsg='Unable to assign address %s to interface %s in namespace %s' % (addr, ifName, nsName), nsName=nsName)
    
def removeAddressFromInterface(ifName, addr, nsName=None):
    runCmd([ 'ip', 'addr', 'del', str(addr), 'dev', ifName ], errMsg='Unable to remove address %s from interface %s in namespace %s' % (addr, ifName, nsName), nsName=nsName)

def setInterfaceMACAddress(ifName, macAddr, nsName=None):
    macAddr = macAddressToString(macAddr)
    runCmd([ 'ip', 'link', 'set', 'dev', ifName, 'address', macAddr ], nsName=nsName, 
           errMsg='Unable to set MAC address for interface %s in namespace %s' % (ifName, nsName))

def attachInterfaceToBridge(ifName, brName, nsName=None):
    runCmd([ 'brctl', 'addif', brName, ifName ], nsName=nsName,
           errMsg='Unable to assign virtual ethernet interface %s to bridge %s' % (ifName, brName))

def getInterfaces(nsName=None):
    out = runCmd([ 'ip', '-o', 'link', 'show' ], errMsg='Failed to enumerate interfaces', nsName=nsName)
    out = out.replace('\\', '') # move backslashes introduced by -o
    interfaces = []
    for m in re.finditer('^\d+:\s+(\S+):\s+<([^>]*)>(.*)$', out, re.M):
        i = {
             'dev' : m.group(1),
        }
        for flag in m.group(2).split(','):
            i[flag] = True
        attrs = m.group(3)
        for m2 in re.finditer('\s*(\S+)\s+(\S+)', attrs):
            name = m2.group(1)
            if name.startswith('link/'):
                name = 'address'
            if name != 'alias':
                value = m2.group(2)
            else:
                value = attrs[m2.start(2):] 
            i[name] = value
        interfaces.append(i)
    return interfaces

hostInterfaces = None

def getHostInterfaces():
    global hostInterfaces
    if not hostInterfaces:
        hostInterfaces = getInterfaces()
    return hostInterfaces

def getInterfaceAddresses(dev, ipv6=False, nsName=None):
    cmd = [ 'ip', '-o' ]
    if ipv6:
        cmd.append('-6')
    else:
        cmd.append('-4')
    cmd += [ 'addr', 'list', 'dev', str(dev) ]
    out = runCmd(cmd, errMsg='Failed to enumerate address for interface %s' % dev, nsName=nsName)
    addrs = [ m.group(1) for m in re.finditer('^\d+:\s+\S+\s+inet6?\s+(\S+)', out, re.M) ]
    if ipv6:
        addrs = [ ipaddress.IPv6Interface(unicode(a)) for a in addrs ]
    else:
        addrs = [ ipaddress.IPv4Interface(unicode(a)) for a in addrs ]
    return addrs
    
def addRoute(dest, dev=None, via=None, options=None, nsName=None):
    cmd = [ 'ip' ]
    if isinstance(dest, ipaddress.IPv6Address) or isinstance(dest, ipaddress.IPv6Network):
        cmd.append('-6')
    if dest == None:
        dest = 'default'
    cmd += [ 'route', 'add', 'to', str(dest) ]
    if dev != None:
        cmd += [ 'dev', str(dev) ]
    if via != None:
        cmd += [ 'via', str(via) ]
    if options != None:
        for option in options.keys():
            cmd += [ option, str(options[option]) ]
    runCmd(cmd, errMsg='Unable to add route in namespace %s' % (nsName), nsName=nsName)

def getRoutes(ipv6=False, nsName=None):
    cmd = [ 'ip', '-o' ]
    if ipv6:
        cmd.append('-6')
    cmd += [ 'route', 'list',  ]
    out = runCmd(cmd, errMsg='Unable to list routes in namespace %s' % (nsName), nsName=nsName)
    routes = []
    for routeLine in out.split('\n'):
        m = re.match('\s*(\S+)\s+(.*)$', routeLine)
        if m:
            dest = m.group(1)
            args = m.group(2)
            route = { 'dest': dest }
            for m in re.finditer('\s*(\S+)\s+(\S+)', args):
                route[m.group(1)] = m.group(2)
            routes.append(route)
    return routes

hostIP6Routes = None
hostIP4Routes = None

def getHostRoutes(ipv6=False):
    if ipv6:
        global hostIP6Routes
        if not hostIP6Routes:
            hostIP6Routes = getRoutes(ipv6=True)
        return hostIP6Routes
    else:
        global hostIP4Routes
        if not hostIP4Routes:
            hostIP4Routes = getRoutes(ipv6=False)
        return hostIP4Routes

hostIPTablesInitialized = False

def initSimnetIPTables(nsName=None):
    
    # Only initialize the simnet configuration for the host once. 
    if nsName == None:
        global hostIPTablesInitialized
        if hostIPTablesInitialized:
            return
        hostIPTablesInitialized = True        

    # Create the simnet IPv4 FORWARD chain and add it to the default FORWARD chain.
    runCmd([ 'iptables', '-N', namePrefix + 'FORWARD' ], nsName=nsName, 
           errMsg='Unable to create simnet iptables FORWARD chain in namespace %s' % nsName)
    runCmd([ 'iptables', '-A', 'FORWARD', '-j', namePrefix + 'FORWARD' ], nsName=nsName, 
           errMsg='Unable to add simnet iptables FORWARD chain to default FORWARD chain in namespace %s' % nsName)
    
    # Set the policy on the default IPv4 FORWARD chain to DROP.
    runCmd([ 'iptables', '-P', 'FORWARD', 'DROP' ], nsName=nsName, 
           errMsg='Unable to set polify on default iptables FORWARD chain in namespace %s' % nsName)
    
    # Create the simnet IPv4 NAT POSTROUTING chain and and it to the default NAT POSTROUTING chain.
    runCmd([ 'iptables', '-t', 'nat', '-N', namePrefix + 'POSTROUTING' ], nsName=nsName, 
           errMsg='Unable to simnet iptables chain in namespace %s' % nsName)
    runCmd([ 'iptables', '-t', 'nat', '-A', 'POSTROUTING', '-j', namePrefix + 'POSTROUTING' ], nsName=nsName, 
           errMsg='Unable to add simnet iptables POSTROUTING chain to default POSTROUTING chain in namespace %s' % nsName)

    # Create the simnet IPv6 FORWARD chain and add it to the default FORWARD chain.
    runCmd([ 'ip6tables', '-N', namePrefix + 'FORWARD' ], nsName=nsName, 
           errMsg='Unable to create simnet ip6tables FORWARD chain in namespace %s' % nsName)
    runCmd([ 'ip6tables', '-A', 'FORWARD', '-j', namePrefix + 'FORWARD' ], nsName=nsName, 
           errMsg='Unable to add simnet ip6tables FORWARD chain to default FORWARD chain in namespace %s' % nsName)
    
    # Set the policy on the default IPv6 FORWARD chains to DROP.
    runCmd([ 'ip6tables', '-P', 'FORWARD', 'DROP' ], nsName=nsName, 
           errMsg='Unable to set polify on default ip6tables FORWARD chain in namespace %s' % nsName)


hostIPTablesCleared = False

def clearSimnetIPTables(nsName=None):
    
    # Only clear the simnet configuration for the host once. 
    if nsName == None:
        global hostIPTablesCleared
        if hostIPTablesCleared:
            return
        hostIPTablesCleared = True        

    # Remove the simnet IPv4 FORWARD chain from the default FORWARD chain, flush it and delete it.
    runCmd([ 'iptables', '-D', 'FORWARD', '-j', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'iptables', '-F', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'iptables', '-X', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)

    # Remove the simnet IPv6 FORWARD chain from the default FORWARD chain, flush it and delete it.
    runCmd([ 'ip6tables', '-D', 'FORWARD', '-j', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'ip6tables', '-F', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'ip6tables', '-X', namePrefix + 'FORWARD' ], nsName=nsName, ignoreErr=True)

    # Remove the simnet IPv4 NAT POSTROUTING chain from the default NAT POSTROUTING chain
    runCmd([ 'iptables', '-t', 'nat', '-D', 'POSTROUTING', '-j', namePrefix + 'POSTROUTING' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'iptables', '-t', 'nat', '-F', namePrefix + 'POSTROUTING' ], nsName=nsName, ignoreErr=True)
    runCmd([ 'iptables', '-t', 'nat', '-X', namePrefix + 'POSTROUTING' ], nsName=nsName, ignoreErr=True)

def enableIP4NAT(insideIfName, outsideIfName, nsName=None):
    
    # Enable MASQUERADE for IPv4 packets leaving the outside interface.
    runCmd([ 'iptables', '-t', 'nat', '-A', namePrefix + 'POSTROUTING', '-o', outsideIfName, '-j', 'MASQUERADE' ],
           nsName=nsName, errMsg='Unable to enable IPv4 NAT in namespace %s' % nsName)

    # Allow IPv4 packets to pass from the inside interface to the outside interface.
    runCmd([ 'iptables', '-A', namePrefix + 'FORWARD', '-i', insideIfName, '-o', outsideIfName, '-j', 'ACCEPT' ], 
           nsName=nsName, errMsg='Unable to enable IPv4 NAT in namespace %s' % nsName)
    
    # Allow returning IPv4 packets to pass from the outside interface to the inside interface if they are part
    # of an established conntrack session. 
    runCmd([ 'iptables', '-A', namePrefix + 'FORWARD', '-i', outsideIfName, '-o', insideIfName, '-m', 'state', '--state', 'RELATED,ESTABLISHED', '-j', 'ACCEPT' ], 
           nsName=nsName, errMsg='Unable to enable IPv4 NAT in namespace %s' % nsName)

def enableIP6Forwarding(insideIfName, outsideIfName, nsName=None):
    
    # Allow IPv6 packets to pass from the inside interface to the outside interface.
    runCmd([ 'ip6tables', '-A', namePrefix + 'FORWARD', '-i', insideIfName, '-o', outsideIfName, '-j', 'ACCEPT' ], 
           nsName=nsName, errMsg='Unable to enable IPv6 forwarding in namespace %s' % nsName)
    
    # Allow return IPv6 packets to pass from the outside interface to the inside interface if they are part
    # of an established conntrack session. 
    runCmd([ 'ip6tables', '-A', namePrefix + 'FORWARD', '-i', outsideIfName, '-o', insideIfName, '-m', 'state', '--state', 'RELATED,ESTABLISHED', '-j', 'ACCEPT' ], 
           nsName=nsName, errMsg='Unable to enable IPv6 forwarding in namespace %s' % nsName)

def setSysctl(name, value, nsName=None):
    runCmd([ 'sysctl', '-q', name + '=' + str(value) ], "Unabled to set sysctl value %s in namespace %s" % (name, nsName), nsName=nsName)

def toIP4InterfaceAddress(v, errStr):
    if v == None or isinstance(v, ipaddress.IPv4Interface):
        return v
    vStr = str(v)
    if vStr.find('/') == -1:
        vStr = vStr + '/32'
    try:
        v = ipaddress.IPv4Interface(unicode(vStr))
    except ValueError:
        raise ConfigException('%sSpecified IPv4 interface address is invalid: %s' % (errStr, v))
    return v

def toIP6InterfaceAddress(v, errStr):
    if v == None or isinstance(v, ipaddress.IPv6Interface):
        return v
    vStr = str(v)
    if vStr.find('/') == -1:
        vStr = vStr + '/32'
    try:
        v = ipaddress.IPv6Interface(unicode(vStr))
    except ValueError:
        raise ConfigException('%sSpecified IPv6 interface address is invalid: %s' % (errStr, v))
    return v

def toIP4Subnet(v, errStr):
    if v == None or isinstance(v, ipaddress.IPv4Network):
        return v
    try:
        v = ipaddress.IPv4Network(unicode(v))
    except ValueError:
        raise ConfigException('%sSpecified IPv4 subnet is invalid: %s' % (errStr, v))
    return v

def toIP6Prefix(v, errStr):
    if v == None or isinstance(v, ipaddress.IPv6Network):
        return v
    try:
        v = ipaddress.IPv6Network(unicode(v))
    except ValueError:
        raise ConfigException('%sSpecified IPv6 prefix is invalid: %s' % (errStr, v))
    return v

def makeIP4IntefaceAddress(subnet, nodeIndex, prefixLen=-1):
    return ipaddress.IPv4Interface((subnet[nodeIndex], prefixLen if prefixLen >= 0 else subnet.prefixlen))

def makeIP6InterfaceAddress(prefix, subnetNum=0, iid=None, macAddr=None, prefixLen=-1, macIs64Bit=False):
    
    # If IID not specified, derive it from the MAC address.
    if not iid:
        
        # If necessary, convert MAC-48 to MAC-64 according per EUI64 spec
        if not macIs64Bit:
            macAddr = ((macAddr & 0xFFFFFF000000) << 16) | 0xFFFE000000 | (macAddr & 0xFFFFFF)

        # Convert MAC address to IPv6 IID by inverting the u bit per rfc-4291.
        iid = macAddr ^ (1 << 57)

    # Retain the prefix's length if not specified.
    if prefixLen < 0:
        prefixLen = prefix.prefixlen

    # Compute the host address.
    addr = prefix[ (subnetNum << 64) + iid ]
    
    # Return the host address with the network prefix.
    return ipaddress.IPv6Interface((addr, prefixLen))

def makeIP6Prefix(basePrefix, networkNum=0, subnetNum=0, prefixLen=48):
    return ipaddress.IPv6Network((basePrefix[(networkNum << 80) + (subnetNum << 64)], prefixLen))

def isNetworkAddress(addr):
    return addr.network.prefixlen < addr.network.max_prefixlen and addr.ip == addr.network.network_address   

def containsGlobalAddress(addrList):
    for a in addrList:
        if not a.is_private:
            return True
    return False

def preferGlobalAddresses(addrList):
    if containsGlobalAddress(addrList):
        return [ a for a in addrList if not a.is_private ]
    else:
        return addrList

def verifyRoot():
    if os.geteuid() != 0:
        raise UsageException('Operation not permitted: This command requires root privileges.')

def parseMACAddress(addrStr):
    addr = 0
    addrHexStr = addrStr.replace(':', '')
    if len(addrHexStr) != 12 and len(addrHexStr) != 16:
        raise ValueError('Invalid MAC address: ' + addrStr)
    try:
        for v in [ addrHexStr[i:i+2] for i in range(0, len(addrHexStr), 2) ]:
            addr = (addr << 8) + int(v, 16)
    except ValueError:
        raise ValueError('Invalid MAC address: ' + addrStr)
    return addr

def macAddressToString(addr):
    addrHexString = '%012X' % addr
    addrHexString = ':'.join(( addrHexString[i:i+2] for i in range(0, len(addrHexString), 2)))
    return addrHexString

logActions = False
logIndent = 0

def logAction(msg):
    if logActions:
        print '%s%s' % ('  ' * logIndent, msg)
    
#===============================================================================
# Main Code
#===============================================================================

# Main code
#
if __name__ == "__main__":

    toolName = os.path.basename(sys.argv[0])
    usage = '''
Usage:
    [ sudo ] {0} <network-layout-file> <command> [ <arguments> ]
    [ sudo ] {0} clear-all
    [ sudo ] {0} help
'''.format(toolName)
    help = '''
simnet -- an IP network simulator based on Linux namespaces

Usage:
    [ sudo ] {0} <network-layout-file> <command> [ <arguments> ]
    [ sudo ] {0} clear-all
    [ sudo ] {0} help

Available commands:

    build
    
        Build a simulated IP network based on the description contained in the
        specified network layout file.

    shell <node-name>
    
        Invoke a shell in the context of a simulated network node.

    show [ all | networks | nodes | hosts | env ]
    
        Show detailed information about simulated networks and nodes.
        
    clear
    
        Tear down all simulated networks and nodes associated with the specified
        network layout file.
        
    clear-all
    
        Tear down all active simulated networks and nodes.
        
    help
    
        Display this help message.
'''.format(toolName)

    try:
    
        simnet = SimNet()
        
        args = sys.argv[1:]
        if len(args) == 0:
            print usage
            sys.exit(-1)
        
        cmdOrFileName = args[0].lower()
        
        if cmdOrFileName == 'clear-all':
            cmd = 'clear-all'
            layoutFileName = None
        elif cmdOrFileName == 'help' or cmdOrFileName == '-h' or cmdOrFileName == '--help':
            cmd = 'help'
            layoutFileName = None
        else:
            if len(args) < 2:
                print usage
                sys.exit(-1)
            layoutFileName = args.pop(0)
            cmd = args[0].lower()
        
        if layoutFileName:
            simnet.loadLayoutFile(layoutFileName)
    
        if cmd == 'help':
            print help
            
        elif cmd == 'clear-all':
            simnet.clearAll()
            
        elif cmd == 'build':
            simnet.buildNetworksAndNodes()
            
        elif cmd == 'clear':
            simnet.clearNetworksAndNodes()
            
        elif cmd == 'show':
            if len(args) == 1:
                (showNets, showNodes, showHosts, showEnv) = (True, True, False, False)
            else:
                subCmd = args[1].lower()
                if subCmd == 'networks':
                     (showNets, showNodes, showHosts, showEnv) = (True, False, False)
                elif subCmd == 'nodes':
                     (showNets, showNodes, showHosts, showEnv) = (False, True, False, False)
                elif subCmd == 'hosts':
                     (showNets, showNodes, showHosts, showEnv) = (False, False, True, False)
                elif subCmd == 'env':
                     (showNets, showNodes, showHosts, showEnv) = (False, False, False, True)
                elif subCmd == 'all':
                     (showNets, showNodes, showHosts, showEnv) = (True, True, True, True)
                else:
                     raise UsageException('Unknown command: %s' % ' '.join(args))
            print simnet.summarizeNetwork(showNets, showNodes, showHosts, showEnv)
    
        elif cmd == 'shell':
            if len(args) < 2:
                raise UsageException('Please specify node name')
            nodeName = args[1]
            simnet.startShell(nodeName)
    
        else:
            raise UsageException('Unknown command: %s' % args[0])
    
        sys.exit(0)
    
    except CommandException, ex:
        print ex.message
        sys.exit(-1) 
    except ConfigException, ex:
        print ex.message
        sys.exit(-1) 
    except UsageException, ex:
        print ex.message
        sys.exit(-1) 



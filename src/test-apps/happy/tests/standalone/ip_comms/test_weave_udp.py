#!/usr/bin/env python
#
#       Copyright (c) 2015-2017  Nest Labs, Inc.
#       All rights reserved.
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
#    @file
#       Calls Weave Echo between nodes.
#

import os
import sys
import unittest
import psutil
import glob
import shutil
import platform
import itertools

from happy.Utils import *
import happy.HappyNetworkAdd
import happy.HappyNetworkAddress
import happy.HappyNodeAdd
import happy.HappyNodeAddress
import happy.HappyNodeJoin
import happy.HappyProcessStart
import happy.HappyProcessStop
import happy.HappyProcessOutput
import happy.HappyProcessWait
import happy.HappyStateDelete

gWeaveRootDir = os.environ.get('abs_top_srcdir', None)
if gWeaveRootDir == None:
    gWeaveRootDir = os.environ.get('OPENWEAVE_ROOT', None)
if gWeaveRootDir == None:
    gWeaveRootDir = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../../../../../..'))

qWeaveBuildDir = os.environ.get('abs_top_builddir', None)
if qWeaveBuildDir == None:
    CPU = platform.machine()
    OS = platform.system().lower()
    if OS == 'linux':
        OS = 'linux-gnu'
    tripletPattern = '%s-*-%s' % (CPU, OS)
    buildDirPattern = os.path.join(gWeaveRootDir, 'build', tripletPattern)
    qWeaveBuildDir = next(iter(glob.glob(buildDirPattern)), None)

def addNetwork(networkId, type):
    h = happy.HappyNetworkAdd.HappyNetworkAdd()
    h.network_id = networkId
    h.type = type
    h.run()

def addNode(nodeId):
    h = happy.HappyNodeAdd.HappyNodeAdd()
    h.node_id = nodeId
    h.run();

def addNodeAddress(nodeId, interface, address):
    h = happy.HappyNodeAddress.HappyNodeAddress()
    h.node_id = nodeId
    h.interface = interface
    h.add = True
    h.address = address
    h.run()

def joinNodeToNetwork(nodeId, networkId):
    h = happy.HappyNodeJoin.HappyNodeJoin()
    h.node_id = nodeId
    h.network_id = networkId
    h.run();

class test_weave_udp(unittest.TestCase):
    
    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.testName = None
    
    @classmethod
    def setUpClass(self):
        
        try:
        
            if qWeaveBuildDir != None:
                self.weavePingPath = os.path.join(qWeaveBuildDir, 'src/test-apps/weave-ping')
            else:
                self.weavePingPath = shutil.which('weave-ping')
            
            if self.weavePingPath == None or not os.access(self.weavePingPath, os.X_OK):
                raise Exception('Unable to locate weave-ping tool (expected location: %s)' % self.weavePingPath)
        
            test_weave_udp.buildTopology();
        
        except:
            test_weave_udp.tearDownClass()
            raise
        
    @classmethod
    def tearDownClass(self):
        happy.HappyStateDelete.HappyStateDelete().run()

    @classmethod
    def buildTopology(self):
        
        # net1 -- IPv6 and IPv4
        addNetwork(networkId='net1', type='wifi')
        
        # net2 -- IPv4 only
        addNetwork(networkId='net2', type='wifi')
        
        # net3 -- IPv6 only
        addNetwork(networkId='net3', type='wifi')
        
        # node1 -- net1, net2, net3, IPv4 and IPv6 addresses
        addNode(nodeId='node1')
        joinNodeToNetwork(nodeId='node1', networkId='net1')
        joinNodeToNetwork(nodeId='node1', networkId='net2')
        joinNodeToNetwork(nodeId='node1', networkId='net3')
        addNodeAddress(nodeId='node1', interface='wlan0', address='192.168.1.1')
        addNodeAddress(nodeId='node1', interface='wlan0', address='fd00:0:1:1::1')
        addNodeAddress(nodeId='node1', interface='wlan0', address='fe80::1:1')
        addNodeAddress(nodeId='node1', interface='wlan1', address='192.168.2.1')
        addNodeAddress(nodeId='node1', interface='wlan2', address='fd00:0:1:3::1')
        addNodeAddress(nodeId='node1', interface='wlan2', address='fe80::3:1')
        
        # node2 -- net1, IPv4 and IPv6 addresses
        addNode(nodeId='node2')
        joinNodeToNetwork(nodeId='node2', networkId='net1')
        addNodeAddress(nodeId='node2', interface='wlan0', address='192.168.1.2')
        addNodeAddress(nodeId='node2', interface='wlan0', address='fd00:0:1:1::2')
        addNodeAddress(nodeId='node2', interface='wlan0', address='fe80::1:2')
        
        # node3 -- net3, IPv6 addresses only
        addNode(nodeId='node3')
        joinNodeToNetwork(nodeId='node3', networkId='net3')
        addNodeAddress(nodeId='node3', interface='wlan0', address='fd00:0:1:3::3')
        addNodeAddress(nodeId='node3', interface='wlan0', address='fe80::3:3')
        
        # node4 -- net2, IPv4 address only
        addNode(nodeId='node4')
        joinNodeToNetwork(nodeId='node4', networkId='net2')
        addNodeAddress(nodeId='node4', interface='wlan0', address='192.168.2.4')
        
    def getProcessTag(self, node, tag):
        splitTestId = self.id().split('.')
        testClass = splitTestId[-2]
        testName = splitTestId[-1]
        if tag != None:
            return '%s.%s.%s.%s' % (testClass, testName, node, tag)
        else:
            return '%s.%s.%s' % (testClass, testName, node)

    def startWeavePing(self, node, args, tag=None):
        self.startProcess(node, self.weavePingPath + ' ' + args, tag)

    def startProcess(self, node, cmdLine, tag=None):
        h = happy.HappyProcessStart.HappyProcessStart()
        h.node_id = node
        h.tag = self.getProcessTag(node, tag)
        h.command = cmdLine
        h.run()

    def stopProcess(self, node, tag=None):
        h = happy.HappyProcessStop.HappyProcessStop()
        h.node_id = node
        h.tag = self.getProcessTag(node, tag)
        h.run()

    def waitComplete(self, node, tag=None, timeout=10):
        h = happy.HappyProcessWait.HappyProcessWait()
        h.node_id = node
        h.tag = self.getProcessTag(node, tag)
        h.timeout = timeout
        h.run()

    def getOutput(self, node, tag=None):
        h = happy.HappyProcessOutput.HappyProcessOutput()
        h.node_id = node
        h.tag = self.getProcessTag(node, tag)
        return h.run().Data()

    def test_unicast_ula(self):
        '''Send UDP Echo requests to unicast IPv6 ULA'''

        # Start listening server on node 2.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 Echo requests from node 1 to node 2
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 2')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening server on node 2.
        self.stopProcess('node2')

        # Verify the Echo request was received by node2 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fd00:0:1:1::1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 2 (fd00:0:1:1::2)' in self.getOutput('node1'), msg='Echo response not found (node1)')

    def test_unicast_ipv4(self):
        '''Send UDP Echo requests to unicast IPv4 address'''

        # Start listening server on node 2.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 Echo requests from node 1 to node 2
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr 192.168.1.2 2')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening server on node 2.
        self.stopProcess('node2')

        # Verify the Echo request was received by node2 with the correct source address.
        self.assertTrue('Echo Request from node 1 (192.168.1.1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 2 (192.168.1.2)' in self.getOutput('node1'), msg='Echo response not found (node1)')

    def test_unicast_ll(self):
        '''Send UDP Echo requests to IPv6 link-local address'''

        # Start listening server on node 2.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 Echo requests from node 1 to node 2
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr fe80::1:2%wlan0 2')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening server on node 2.
        self.stopProcess('node2')

        # Verify the Echo request was received by node2 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fe80::1:1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 2 (fe80::1:2)' in self.getOutput('node1'), msg='Echo response not found (node1)')

    def test_multicast_ll(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1). Use the sender's link-local as source address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 to the IPv6 all-nodes, link-scope address (ff02::1).
        # Force the source address of the requests to be node1's link-local address by configuring it
        # to NOT be a member of a fabric (i.e. fabric id = 0).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 0 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node2 and node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fe80::1:1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (fe80::3:1)' in self.getOutput('node3'), msg='Echo request not found (node3)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (fe80::1:2)' in node1Output or 'Echo Response from node 3 (fe80::3:3)' in node1Output), 
                        msg='Echo Response not found (node1)')
        
    def test_multicast_ula(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1). Use sender's IPv6 ULA as source address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 to the IPv6 all-nodes, link-scope address (ff02::1).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node2 and node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fd00:0:1:1::1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (fd00:0:1:3::1)' in self.getOutput('node3'), msg='Echo request not found (node3)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (fd00:0:1:1::2)' in node1Output or 'Echo Response from node 3 (fd00:0:1:3::3)' in node1Output), 
                        msg='Echo Response not found (node1)')
        
    def test_broadcast_ipv4(self):
        '''Send UDP Echo requests to IPv4 broadcast address (255.255.255.255).'''

        # Start listening servers on nodes 2 and 4.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node4', '--node-id 4 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 broadcast Echo requests from node1 to the IPv4 broadcast address.
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr 255.255.255.255 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node4')

        # Verify the Echo request was received by node2 and node4 with the correct source address.
        self.assertTrue('Echo Request from node 1 (192.168.1.1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (192.168.2.1)' in self.getOutput('node4'), msg='Echo request not found (node4)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (192.168.1.2)' in node1Output or 'Echo Response from node 4 (192.168.2.4)' in node1Output), 
                        msg='Echo Response not found (node1)')

    def test_intf_multicast_ll(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1) on a specific interface. Use sender's link-local address as source address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 over wlan2 (net3).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 0 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1%wlan2 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fe80::3:1)' in self.getOutput('node3'), msg='Echo request not found (node3)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 3 (fe80::3:3)' in self.getOutput('node1'), msg='Echo response not found (node1)')
        
        # Verify no Echo request was received by node2.
        self.assertFalse('Echo Request from node 1' in self.getOutput('node2'), msg='Unexpected echo request found (node2)')

    def test_intf_multicast_ula(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1) on a specific interface. Use sender's ULA as source address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 over wlan2 (net3).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1%wlan2 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fd00:0:1:3::1)' in self.getOutput('node3'), msg='Echo request not found (node3)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 3 (fd00:0:1:3::3)' in self.getOutput('node1'), msg='Echo response not found (node1)')
        
        # Verify no Echo request was received by node2.
        self.assertFalse('Echo Request from node 1' in self.getOutput('node2'), msg='Unexpected echo request found (node2)')
        
    def test_intf_broadcast_ipv4(self):
        '''Send UDP Echo requests to IPv4 broadcast address (255.255.255.255) over a specific interface.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node4', '--node-id 4 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 over wlan1 (net2).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr 255.255.255.255%wlan1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node4')

        # Verify the Echo request was received by node4 with the correct source address.
        self.assertTrue('Echo Request from node 1 (192.168.2.1)' in self.getOutput('node4'), msg='Echo request not found (node4)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 4 (192.168.2.4)' in self.getOutput('node1'), msg='Echo response not found (node1)')
        
        # Verify no Echo request was received by node2.
        self.assertFalse('Echo Request from node 1' in self.getOutput('node2'), msg='Unexpected echo request found (node2)')
        
    def test_sender_bound_ipv6_multicast(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1) with sender bound to IPv6 listening address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 with sender bound to its wlan2 (net3) IPv6 ULA address.
        self.startWeavePing('node1', '--node-id 1 --fabric-id 0 --subnet 1 --udp --count 5 --interval 200 --node-addr fd00:0:1:3::1 --dest-addr ff02::1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fd00:0:1:3::1)' in self.getOutput('node3'), msg='Echo request not found (node3)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 3 (fd00:0:1:3::3)' in self.getOutput('node1'), msg='Echo response not found (node1)')
        
        # Verify no Echo request was received by node2.
        self.assertFalse('Echo Request from node 1' in self.getOutput('node2'), msg='Unexpected echo request found (node2)')

    def test_sender_bound_ipv4_broadcast(self):
        '''Send UDP Echo requests to IPv4 broadcast address (255.255.255.255) with sender bound to IPv4 listening address.'''

        # Start listening servers on nodes 2 and 3.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --listen')
        self.startWeavePing('node4', '--node-id 4 --fabric-id 1 --subnet 1 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 with sender bound to its wlan2 (net3) IPv4 address.
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --node-addr 192.168.2.1 --dest-addr 255.255.255.255 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node4')

        # Verify the Echo request was received by node4 with the correct source address.
        self.assertTrue('Echo Request from node 1 (192.168.2.1)' in self.getOutput('node4'), msg='Echo request not found (node4)')
        
        # Verify the Echo response was received by node1 with the correct source address.
        self.assertTrue('Echo Response from node 4 (192.168.2.4)' in self.getOutput('node1'), msg='Echo response not found (node1)')
        
        # Verify no Echo request was received by node2.
        self.assertFalse('Echo Request from node 1' in self.getOutput('node2'), msg='Unexpected echo request found (node2)')

    def test_listener_bound_multicast_ll(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1) with listeners bound to IPv6 addresses. Use sender's link-local as source address.'''

        # Start listening servers on nodes 2 and 3 bound to their respective ULAs.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --node-addr fd00:0:1:1::2 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --node-addr fd00:0:1:3::3 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 to the IPv6 all-nodes, link-scope address (ff02::1).
        # Force the source address of the requests to be node1's link-local address by configuring it
        # to NOT be a member of a fabric (i.e. fabric id = 0).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 0 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node2 and node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fe80::1:1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (fe80::3:1)' in self.getOutput('node3'), msg='Echo request not found (node3)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        # Note that, because the listeners are bound to specific IPv6 ULAs, the responses come from those ULAs, rather than 
        # from the node's link-local addresses as would be expected if the node's weren't bound.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (fd00:0:1:1::2)' in node1Output or 'Echo Response from node 3 (fd00:0:1:3::3)' in node1Output), 
                        msg='Echo Response not found (node1)')
        
    def test_listener_bound_multicast_ula(self):
        '''Send UDP Echo requests to IPv6 all-nodes multicast address (ff02::1) with listeners bound to IPv6 addresses. Use sender's IPv6 ULA as source address.'''

        # Start listening servers on nodes 2 and 3 bound to their respective ULAs.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --node-addr fd00:0:1:1::2 --listen')
        self.startWeavePing('node3', '--node-id 3 --fabric-id 1 --subnet 1 --node-addr fd00:0:1:3::3 --listen')
        time.sleep(0.25)
        
        # Send 5 multicast Echo requests from node1 to the IPv6 all-nodes, link-scope address (ff02::1).
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr ff02::1 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node3')

        # Verify the Echo request was received by node2 and node3 with the correct source address.
        self.assertTrue('Echo Request from node 1 (fd00:0:1:1::1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (fd00:0:1:3::1)' in self.getOutput('node3'), msg='Echo request not found (node3)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (fd00:0:1:1::2)' in node1Output or 'Echo Response from node 3 (fd00:0:1:3::3)' in node1Output), 
                        msg='Echo Response not found (node1)')
        
    def test_listener_bound_broadcast_ipv4(self):
        '''Send UDP Echo requests to IPv4 broadcast address (255.255.255.255) with listeners bound to IPv4 addresses.'''

        # Start listening servers on nodes 2 and 4 bount to their respective IPv4 addresses.
        self.startWeavePing('node2', '--node-id 2 --fabric-id 1 --subnet 1 --node-addr 192.168.1.2 --listen')
        self.startWeavePing('node4', '--node-id 4 --fabric-id 1 --subnet 1 --node-addr 192.168.2.4 --listen')
        time.sleep(0.25)
        
        # Send 5 broadcast Echo requests from node1 to the IPv4 broadcast address.
        self.startWeavePing('node1', '--node-id 1 --fabric-id 1 --subnet 1 --udp --count 5 --interval 200 --dest-addr 255.255.255.255 FFFFFFFFFFFFFFFF')
        self.waitComplete('node1')
        
        # Wait for things to settle.
        time.sleep(0.25)
        
        # Stop the listening servers.
        self.stopProcess('node2')
        self.stopProcess('node4')

        # Verify the Echo request was received by node2 and node4 with the correct source address.
        self.assertTrue('Echo Request from node 1 (192.168.1.1)' in self.getOutput('node2'), msg='Echo request not found (node2)')
        self.assertTrue('Echo Request from node 1 (192.168.2.1)' in self.getOutput('node4'), msg='Echo request not found (node4)')

        # Verify that an Echo response was received by node1 from at least one of the two server nodes.
        node1Output = self.getOutput('node1')
        self.assertTrue(('Echo Response from node 2 (192.168.1.2)' in node1Output or 'Echo Response from node 4 (192.168.2.4)' in node1Output), 
                        msg='Echo Response not found (node1)')

if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == 'build-topology':
        test_weave_udp.buildTopology()
    else:
        unittest.main()



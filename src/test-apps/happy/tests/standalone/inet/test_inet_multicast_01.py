#!/usr/bin/env python

#
#    Copyright (c) 2018-2019 Google LLC.
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

#
#    @file
#       Calls Weave Inet Multicast test among sender and receiver nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveInetMulticast
import WeaveUtilities

class test_weave_inet_multicast_01(unittest.TestCase):
    def setUp(self):
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.using_lwip = True
        else:
            self.using_lwip = False

        self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/"

        # The difference between LwIP and non-LwIP exection is that
        # the former will take the LwIP native default interface,
        # 'et1', whereas the latter will take the topology node
        # interface, 'wlan0'. In the former case, LwIP will instead use
        # 'wlan0' as its hosted OS tap network device to proxy itself
        # into the host's network stack.

        if self.using_lwip:
            self.topology_file += "five_nodes_on_tap_wifi.json"
            self.interface = "et1"
            self.tap = "wlan0"
        else:
            self.topology_file += "five_nodes_on_wifi.json"
            self.interface = "wlan0"
            self.tap = None

        self.show_strace = False

        options = WeaveStateLoad.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        ret = setup_network.run()


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_inet_multicast(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        # This test runs four (4) separate invocations / runs:
        #
        #   1) UDP over IPv6
        #   2) UDP over IPv4
        #   3) ICMPv6 over IPv6
        #   4) ICMPv4 over IPv4
        #
        # each with one sender and four receivers. Each receiver
        # varies in the number of multicast groups it participates in.
        #
        # The ipv4-local-addr configuration key-value pairs are only
        # used for LwIP hosted OS topologies where a network tap
        # device is configured.

        configuration = {
            'sender' : {
                '00-WifiNode-TxFourMulticastAddresses' : {
                     'groups' : [
                         {
                             'id' : 1,
                             'send' : 1,
                             'receive' : 0
                         },
                         {
                             'id' : 2,
                             'send' : 2,
                             'receive' : 0
                         },
                         {
                             'id' : 3,
                             'send' : 3,
                             'receive' : 0
                         },
                         {
                             'id' : 4,
                             'send' : 5,
                             'receive' : 0
                         },
                     ],
                    'tap-ipv4-local-addr' : "192.168.10"
                }
            },
            'receivers' : {
                '01-WifiNode-RxOneMulticastAddress' : {
                     'groups' : [
                         {
                             'id' : 1,
                             'send' : 0,
                             'receive' : 1
                         }
                     ],
                    'tap-ipv4-local-addr' : "192.168.11"
                },
                '02-WifiNode-RxTwoMulticastAddresses' : {
                     'groups' : [
                         {
                             'id' : 1,
                             'send' : 0,
                             'receive' : 1
                         },
                         {
                             'id' : 2,
                             'send' : 0,
                             'receive' : 2
                         }
                     ],
                    'tap-ipv4-local-addr' : "192.168.12"
                },
                '03-WifiNode-RxThreeMulticastAddresses' : {
                     'groups' : [
                         {
                             'id' : 1,
                             'send' : 0,
                             'receive' : 1
                         },
                         {
                             'id' : 2,
                             'send' : 0,
                             'receive' : 2
                         },
                         {
                             'id' : 3,
                             'send' : 0,
                             'receive' : 3
                         }
                     ],
                    'tap-ipv4-local-addr' : "192.168.13"
                },
                '04-WifiNode-RxFourMulticastAddresses' : {
                     'groups' : [
                         {
                             'id' : 1,
                             'send' : 0,
                             'receive' : 1
                         },
                         {
                             'id' : 2,
                             'send' : 0,
                             'receive' : 2
                         },
                         {
                             'id' : 3,
                             'send' : 0,
                             'receive' : 3
                         },
                         {
                             'id' : 4,
                             'send' : 0,
                             'receive' : 5
                         }
                     ],
                    'tap-ipv4-local-addr' : "192.168.14"
                }
            }
        }

        # Topology-independent test parameters:

        transports = [ "udp", "raw" ]
        networks = [ "6", "4" ]

        for network in networks:
            for transport in transports:

                # Run the test.
				
                value, data = self.__run_inet_multicast_test(configuration, self.interface, network, transport)
 
                # Process and report the results.
 
                self.__process_result(configuration, self.interface, network, transport, value, data)


    def __process_result(self, configuration, interface, network, transport, value, data):
        nodes = len(configuration['sender']) + len(configuration['receivers'])

        print "Inet multicast test using %sIPv%s w/ device interface: %s (w/%s LwIP) with %u nodes:" % ("UDP/" if transport == "udp" else "", network, "<none>" if interface == None else interface, "" if self.using_lwip else "o", nodes),

        if value:
            print hgreen("PASSED")
        else:
            print hred("FAILED")
            raise ValueError("Weave Inet Multicast Test Failed")


    def __run_inet_multicast_test(self, configuration, interface, network, transport):
        # The default interval is 1 s (1000 ms). This is a good
        # default for interactive test operation; however, for
        # automated continuous integration, we prefer it to run much
        # faster. Consequently, use 250 ms as the interval.
        
        options = WeaveInetMulticast.option()
        options["quiet"] = False
        options["configuration"] = configuration
        options["interface"] = interface
        options["ipversion"] = network
        options["transport"] = transport
        options["interval"] = str(250)
        options["tap"] = self.tap

        weave_inet_multicast = WeaveInetMulticast.WeaveInetMulticast(options)
        ret = weave_inet_multicast.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



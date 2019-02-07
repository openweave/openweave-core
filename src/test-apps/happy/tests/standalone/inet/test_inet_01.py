#!/usr/bin/env python

#
#    Copyright (c) 2018-2019 Google LLC.
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

#
#    @file
#       Calls Weave Inet test between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveInet
import WeaveUtilities

class test_weave_inet_01(unittest.TestCase):
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
            self.topology_file += "two_nodes_on_tap_wifi.json"
            self.interface = "et1"
            self.tap = "wlan0"
        else:
            self.topology_file += "two_nodes_on_wifi.json"
            self.interface = "wlan0"
            self.tap = None

        self.sender = "00-WifiNode-Tx"
        self.receiver = "01-WifiNode-Rx"

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


    def test_weave_inet(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        # This test iterates over several parameters:
        #
        #   1) Network
        #      a) IPv4
        #      b) IPv6
        #         i) Link-local Addresses (LLAs)
        #         ii) Unique-local Addresses (ULAs)
        #         iii) Globally-unique Addresses (GUAs)
        #
        #   2) Transport
        #      a) Raw (ICMP)
        #      b) UDP
        #      c) TCP
        #
        #   3) Send Size*
        #      a) Small, sub-MTU prime value
        #      b) Large, sub-MTU prime value
        #
        #   4) Send / Receive Length
        #      a) Small, sub-MTU prime value
        #      b) Large, sub-MTU prime value
        #      c) Large, super-MTU prime value
        #
        # * Note: In the Inet layer, the maximum send size is limited
        #         by the LwIP implementation of System::PacketBuffer::New
        #         that caps the user send size at 1,408 bytes.

        # Topology-independent test parameters:
        
        rx_tx_lengths = [ 113, 1499, 1523 ]
        tx_sizes = [ 113, 1381 ]
        transports = [ "raw", "udp", "tcp" ]

        # Topology-dependent test parameters:

        networks_and_prefixes = {
                                  "4" : [
                                             "192.168.1.0/24"
                                        ],
                                  "6" : [
                                             "fe80::/64",
                                             "fd00:0000:fab1:0001::/64",
                                             "2001:db8:1:2::/64"
                                        ]
                                }

        # Iterate over each of parameters 1-4 above and invoke the
        # test and process the results for each iteration.

        for network in networks_and_prefixes.keys():
            for prefix in networks_and_prefixes[network]:
                for transport in transports:
                    for tx_size in tx_sizes:
                        for rx_tx_length in rx_tx_lengths:
                            if rx_tx_length >= tx_size:

                                # IPv6 link-local endpoints require a
                                # bound interface for these tests to
                                # work reliably; filter accordingly.

                                if network == "6" and prefix == "fe80::/64":
                                    interface = self.interface
                                else:
                                    interface = None

                                # Run the test.
                                
                                value, data = self.__run_inet_test_between(self.sender, self.receiver, interface, network, transport, prefix, tx_size, rx_tx_length)

                                # Process and report the results.
                                
                                self.__process_result(self.sender, self.receiver, interface, network, transport, prefix, tx_size, rx_tx_length, value, data)


    def __process_result(self, sender, receiver, interface, network, transport, prefix, tx_size, rx_tx_length, value, data):
        print "Inet test using %sIPv%s w/ device interface %s (w/%s LwIP) %s with %u/%u size/length:" % ("UDP/" if transport == "udp" else "TCP/" if transport == "tcp" else "", network, "<none>" if interface == None else interface, "" if self.using_lwip else "o", sender + " to " + receiver + " w/ prefix " + prefix, tx_size, rx_tx_length),

        if value:
            print hgreen("PASSED")
        else:
            print hred("FAILED")
            raise ValueError("Weave Inet Test Failed")


    def __run_inet_test_between(self, sender, receiver, interface, network, transport, prefix, tx_size, rx_tx_length):
        # The default interval is 1 s (1000 ms). This is a good
        # default for interactive test operation; however, for
        # automated continuous integration, we prefer it to run much
        # faster. Consequently, use 250 ms as the interval.
        
        options = WeaveInet.option()
        options["quiet"] = False
        options["sender"] = sender
        options["receiver"] = receiver
        options["interface"] = interface
        options["ipversion"] = network
        options["transport"] = transport
        options["prefix"] = prefix
        options["tx_size"] = str(tx_size)
        options["rx_tx_length"] = str(rx_tx_length)
        options["interval"] = str(250)
        options["tap"] = self.tap

        weave_inet = WeaveInet.WeaveInet(options)
        ret = weave_inet.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()

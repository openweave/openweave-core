#!/usr/bin/env python

#
#    Copyright (c) 2019 Google LLC.
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
#       Implements WeaveInet class that tests Inet Layer unicast
#       between a pair of nodes.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest


options = {}
options["sender"] = None
options["receiver"] = None
options["interface"] = None
options["ipversion"] = None
options["transport"] = None
options["prefix"] = None
options["tx_size"] = None
options["rx_tx_length"] = None
options["interval"] = None
options["quiet"] = False
options["tap"] = None


def option():
    return options.copy()


class WeaveInet(HappyNode, HappyNetwork, WeaveTest):
    """

    Note:
        Supports two networks: IPv4 and IPv6
        Supports three transports: raw IP, TCP, and UDP

    return:
        0   success
        1   failure

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        # Dictionary that will eventually contain 'node' and 'tag' for
        # the sender node process.
        
        self.sender = {}

        # Dictionary that will eventually contain 'node' and 'tag'
        # for the receiver node process.
        
        self.receiver = {}

        self.sender['ip'] = None
        self.sender['node'] = opts["sender"]
        self.receiver['ip'] = None
        self.receiver['node'] = opts["receiver"]
        self.interface = opts["interface"]
        self.ipversion = opts["ipversion"]
        self.transport = opts["transport"]
        self.prefix = opts["prefix"]
        self.tx_size = opts["tx_size"]
        self.rx_tx_length = opts["rx_tx_length"]
        self.interval = opts["interval"]
        self.quiet = opts["quiet"]
        self.tap = opts["tap"]


    def __log_error_and_exit(self, error):
        self.logger.error("[localhost] WeaveInet: %s" % (error))
        sys.exit(1)


    def __checkNodeExists(self, node, description):
        if not self._nodeExists(node):
            emsg = "The %s '%s' does not exist in the test topology." % (description, node)
            self.__log_error_and_exit(emsg)


    # Sanity check the instantiated options and configuration
    def __pre_check(self):
        # Sanity check the transport
        if self.transport != "udp" and self.transport != "tcp" and self.transport != "raw":
            emsg = "Transport type must be one of 'raw', 'tcp', or 'udp'."
            self.__log_error_and_exit(emsg)

        # Sanity check the IP version
        if self.ipversion != "4" and self.ipversion != "6":
            emsg = "The IP version must be one of '4' or '6'."
            self.__log_error_and_exit(emsg)

        # There must be exactly one sender
        if self.sender['node'] == None:
            emsg = "The test must have exactly one sender."
            self.__log_error_and_exit(emsg)

        # There must be exactly one receiver
        if self.sender['node'] == None:
            emsg = "The test must have exactly one receiver."
            self.__log_error_and_exit(emsg)

        # Each specified sender and receiver node must exist in the
        # loaded Happy configuration.

        self.__checkNodeExists(self.sender['node'], "sender")

        self.__checkNodeExists(self.receiver['node'], "receiver")

        # Use the sender and receiver node names, specified prefix,
        # and IP version and attempt to lookup addresses for the
        # sender and receiver.

        # First, sanity check the prefix

        if self.prefix == None:
            emsg = "Please specifiy a valid IPv%s prefix." % (self.ipversion)
            self.__log_error_and_exit(emsg)

        # Second, sanity check the prefix against the IP version.

        if (self.ipversion == "4" and not IP.isIpv4(self.prefix)) or (self.ipversion == "6" and not IP.isIpv6(self.prefix)):
            emsg = "The specified prefix %s is the wrong address family for IPv%s" % (self.prefix, self.ipversion)
            self.__log_error_and_exit(emsg)

        # Finally, find and take the first matching addresses for the
        # node names and specified prefix.

        rips = self.getNodeAddressesOnPrefix(self.prefix, self.receiver['node'])

        if rips != None and len(rips) > 0:
            self.receiver['ip'] = rips[0]

        sips = self.getNodeAddressesOnPrefix(self.prefix, self.sender['node'])

        if sips != None and len(sips) > 0:
            self.sender['ip'] = sips[0]
                
        # Check if all unknowns were found

        if self.receiver['ip'] == None:
            emsg = "Could not find IP address of the receiver node, %s" % (self.receiver['node'])
            self.__log_error_and_exit(emsg)

        if self.sender['ip'] == None:
            emsg = "Could not find IP address of the sender node, %s" % (self.sender['node'])
            self.__log_error_and_exit(emsg)

        if self.tx_size == None or not self.tx_size.isdigit():
            emsg = "Please specify a valid size of data to send per transmission."
            self.__log_error_and_exit(emsg)

        if self.rx_tx_length == None or not self.rx_tx_length.isdigit():
            emsg = "Please specify a valid total length of data to send and receive."
            self.__log_error_and_exit(emsg)

        if not self.interval == None and not self.interval.isdigit():
            emsg = "Please specify a valid send interval in milliseconds."
            self.__log_error_and_exit(emsg)

    # Gather and return the exit status and output as a tuple for the
    # specified process node and tag.
    def __gather_process_results(self, process, quiet):
        node = process['node']
        tag = process['tag']
        
        status, output = self.get_test_output(node, tag, quiet)

        return (status, output)


    # Gather and return the exit status and output as a tuple for the
    # sender process.
    def __gather_sender_results(self, quiet):
        status, output = self.__gather_process_results(self.sender, quiet)

        return (status, output)


    # Gather and return the exit status and output as a tuple for the
    # specified receiver process.
    def __gather_receiver_results(self, quiet):
        status, output = self.__gather_process_results(self.receiver, quiet)

        return (status, output)


    # Gather and return the exit status and output as a dictionary for all
    # sender and receiver processes.
    def __gather_results(self):
        quiet = True
        results = {}
        sender_results = {}
        receiver_results = {}

        sender_results['status'], sender_results['output'] = \
            self.__gather_sender_results(quiet)

        receiver_results['status'], receiver_results['output'] = \
            self.__gather_receiver_results(quiet)

        results['sender'] = sender_results
        results['receiver'] = receiver_results

        return (results)


    def __process_results(self, results):
        status = True
        output = {}
        output['sender'] = ""
        output['receiver'] = ""

        # Iterate through the sender and receivers return status. If
        # all had successful (0) status, then the cumulative return
        # status is successful (True). If any had unsuccessful status,
        # then the cumulative return status is unsuccessful (False).
        #
        # For the output, simply key it by sender and receivers.

        # Sender

        status = (results['sender']['status'] == 0)

        output['sender'] = results['sender']['output']

        # Receiver

        status = (status and (results['receiver']['status'] == 0))

        output['receiver'] = results['receiver']['output']

        return (status, output)


    # Start the test process on the specified node with the provided
    # tag using the provided local address and extra command line
    # options.
    def __start_node(self, node, local_addr, extra, tag):
        cmd = "sudo "
        cmd += self.getWeaveInetLayerPath()

        # If present, generate and accumulate the LwIP hosted OS
        # network tap device interface.

        if self.tap:
            cmd += " --tap-device " + self.tap

        # Generate and accumulate the transport and IP version command
        # line arguments.
        
        cmd += " --ipv" + self.ipversion

        cmd += " --" + self.transport

        cmd += " --local-addr " + local_addr

        # Generate and accumulate, if present, the bound network
        # interface command line option.

        if not self.interface == None:
            cmd += " --interface " + self.interface

        # Accumulate any extra command line options specified.
        
        cmd += extra

        self.logger.debug("[localhost] WeaveInet: Will start process on node '%s' with tag '%s' and command '%s'" % (node, tag, cmd))

        self.start_weave_process(node, cmd, tag, sync_on_output=self.ready_to_service_events_str)


    # Start the receiver test process.
    def __start_receiver(self):
        node = self.receiver['node']
        tag = "WEAVE-INET-RX"

        extra_cmd = " --listen"

        # Accumulate size/length-related parameters. Receivers never
        # send anything.

        extra_cmd += " --expected-rx-size " + str(self.rx_tx_length)
        extra_cmd += " --expected-tx-size 0"

        self.__start_node(node, self.receiver['ip'], extra_cmd, tag)

        self.receiver['tag'] = tag


    # Start the sender test process.
    def __start_sender(self):
        node = self.sender['node']
        tag = "WEAVE-INET-TX"

        extra_cmd = ""

        if not self.interval == None:
            extra_cmd += " --interval " + str(self.interval)

        # Accumulate size/length-related parameters. Senders never
        # receive anything.

        extra_cmd += " --send-size " + str(self.tx_size)
        extra_cmd += " --expected-rx-size 0"
        extra_cmd += " --expected-tx-size " + str(self.rx_tx_length)
        
        extra_cmd += " " + self.receiver['ip']

        self.__start_node(node, self.sender['ip'], extra_cmd, tag)

        self.sender['tag'] = tag


    # Block and wait for the sender test process.
    def __wait_for_sender(self):
        node = self.sender['node']
        tag = self.sender['tag']

        self.logger.debug("[localhost] WeaveInet: Will wait for sender on node %s with tag %s..." % (node, tag))

        self.wait_for_test_to_end(node, tag)


    # Stop the receiver test process.
    def __stop_receiver(self):
        node = self.receiver['node']
        tag = self.receiver['tag']

        self.logger.debug("[localhost] WeaveInet: Will stop receiver on node %s with tag %s..." % (node, tag))

        self.stop_weave_process(node, tag)


    # Run the test.
    def run(self):
        results = {}
        
        self.logger.debug("[localhost] WeaveInet: Run.")

        self.__pre_check()

        self.__start_receiver()

        self.logger.debug("[%s] WeaveInet: receiver should be running." % ("localhost"))

        emsg = "receiver %s should be running." % (self.receiver['tag'])
        self.logger.debug("[%s] WeaveInet: %s" % (self.receiver['node'], emsg))

        self.__start_sender()

        self.__wait_for_sender()

        self.__stop_receiver()

        # Gather results from the sender and receivers

        results = self.__gather_results()

        # Process the results from the sender and receivers into a
        # singular status value and a results dictionary containing
        # process output.

        status, results = self.__process_results(results)

        self.logger.debug("[localhost] WeaveInet: Done.")

        return ReturnMsg(status, results)

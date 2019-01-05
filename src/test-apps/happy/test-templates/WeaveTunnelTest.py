#!/usr/bin/env python


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

#
#    @file
#       Implements WeaveTunnelTest class that executes a set of Weave
#       Tunnel tests between border-gateway and a service.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode

import happy.HappyNodeRoute
import happy.HappyNetworkAddress

import WeaveTunnelStop
from WeaveTest import WeaveTest


options = {}
options["quiet"] = False
options["border_gateway"] = None
options["service"] = None
options["tap"] = None


def option():
    return options.copy()


class WeaveTunnelTest(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-tunnel-test start a tunnel between border-gateway and the service and run all tests

    weave-tunnel-test [-h --help] [-q --quiet] [-b --border_gateway <NODE_ID>]
        [-s --service <NODE_ID> | <IP_ADDR>] [-p --tap <TAP_INTERFACE>]

    Example:
    $ weave-tunnel-test node01 node02
        Start Weave tunnel test between border-gateway node01 and service node02.

    $ weave-tunnel-test node01 184.72.129.8
        Start Weave tunnel between border-gateway node01 and a node with an IP address 184.72.129.8.


    return:
        0    success
        1    fail
    """

    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.border_gateway = opts["border_gateway"]
        self.service = opts["service"]
        self.tap = opts["tap"]

        self.service_process_tag = "WEAVE-TUNNEL-TEST-SERVER"
        self.gateway_process_tag = "WEAVE-TUNNEL-TEST-BORDER-GW"

        self.gateway_node_id = None
        self.service_node_id = None

    def __pre_check(self):
        # Check if border-gateway node was given
        if not self.border_gateway:
            emsg = "Border-gateway node id not specified."
            self.logger.error("[localhost] WeaveTunnelTest: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Tunnel service node is given.
        if not self.service:
            emsg = "Service node id not specified."
            self.logger.error("[localhost] WeaveTunnelTest: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Tunnel border gateway node exists.
        if not self._nodeExists(self.border_gateway):
            emsg = "Border-gateway node %s does not exist." % (self.border_gateway)
            self.logger.error("[localhost] WeaveTunnelTest: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Tunnel service node exists.
        if not self._nodeExists(self.service):
            emsg = "Service node %s does not exist." % (self.service)
            self.logger.error("[localhost] WeaveTunnelTest: %s" % (emsg))
            sys.exit(1)

        self.fabric_id = self.getFabricId()

        # Check if there is no fabric
        if self.fabric_id == None:
            emsg = "There is no Weave fabric."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            sys.exit(1)

        # Get the IPv4 address of the Service to connect to
        self.service_public_ip = self.getNodePublicIPv4Address(self.service)

        # Get the Weave ID for the Service node
        self.service_weave_id = self.getServiceWeaveID("Tunnel")

        # Get the Weave ID for the Border gateway node
        self.gateway_weave_id = self.getWeaveNodeID(self.border_gateway)
 
        # Get the Weave IPv6 ULA address of the Ser\vice node
        self.server_ula = self.getServiceWeaveIPAddress("Tunnel", self.service_weave_id)

        # Check if all unknowns were found

        if self.service_public_ip == None:
            emsg = "Could not find IP address of the service node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            sys.exit(1)

        if self.service_weave_id == None:
            emsg = "Could not find Weave node ID of the service node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            sys.exit(1)

        if self.gateway_weave_id == None:
            emsg = "Could not find Weave node ID of the border-gateway node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            sys.exit(1)

        if self.server_ula == None:
            emsg = "Could not find Weave ULA Address of the Service node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            sys.exit(1)

    def __wait_for_gateway(self):
        self.wait_for_test_to_end(self.border_gateway, self.gateway_process_tag)

    def __stop_server_side(self):
        self.stop_weave_process(self.service, self.service_process_tag)

    def __process_results(self, client_output):
        # search for 'Passed' word
        pass_test = True

        for line in client_output.split("\n"):
            if "'FAILED'" in line or "'PASSED'" in line:
                tokens = line.split("'")
                for str in tokens:
                    if str.startswith('Test', 0, len(str)):
                        testName = str

                if "'FAILED'" in line:
                    print hred(testName + ":" + " FAILED ")
                    pass_test = False
                if "'PASSED'" in line:
                    print hgreen(testName + ":" + " PASSED ")


        return (pass_test, client_output)

    def __start_tunnel_test_server(self):
        cmd = self.getWeaveTunnelTestServerPath()
        if not cmd:
            return

        cmd += " --node-id " + str(self.service_weave_id)
        cmd += " --fabric-id " + str(self.fabric_id)

        if self.tap:
            cmd += " --tap-device " + self.tap

        cmd = self.runAsRoot(cmd)
        self.start_weave_process(self.service, cmd, self.service_process_tag, sync_on_output=self.ready_to_service_events_str)

        max_wait_time = 40

        while not self._nodeInterfaceExists(self.service_tun, self.service):
            time.sleep(0.1)
            max_wait_time -= 1

            if max_wait_time <= 0:
                emsg = "Service-side tunnel interface %s was not created." % (self.service_tun)
                self.logger.error("[%s] WeaveTunnel: %s" % (self.service, emsg))
                sys.exit(1)

            if (max_wait_time) % 10 == 0:
                emsg = "Waiting for interface %s to be created." % (self.service_tun)
                self.logger.debug("[%s] WeaveTunnel: %s" % (self.service, emsg))

        with self.getStateLockManager():
            self.readState()

            new_node_interface = {}
            new_node_interface["link"] = None
            new_node_interface["type"] = self.network_type["tun"]
            new_node_interface["ip"] = {}
            self.setNodeInterface(self.service, self.service_tun, new_node_interface)

            self.writeState()


    def __assign_tunnel_endpoint_ula_at_test_server(self):
        addr = self.getServiceWeaveIPAddress("Tunnel", self.service)

        options = happy.HappyNodeAddress.option()
        options["quiet"] = self.quiet
        options["node_id"] = self.service
        options["interface"] = self.service_tun
        options["address"] = addr
        options["add"] = True

        addAddr = happy.HappyNodeAddress.HappyNodeAddress(options)
        addAddr.run()


    def __start_test_tunnel_border_gateway_client(self):
        cmd = self.getWeaveTunnelTestBRPath()
        if not cmd:
            return

        cmd += " --node-id " + self.gateway_weave_id
        cmd += " --fabric-id " + str(self.fabric_id)

        if self.tap:
            cmd += " --tap-device " + self.tap

        cmd += " -S " + str(self.server_ula) + " --connect-to " + self.service_public_ip + " " + str(self.service_weave_id)
        cmd = self.runAsRoot(cmd)
        self.start_weave_process(self.border_gateway, cmd, self.gateway_process_tag, sync_on_output=self.ready_to_service_events_str)


    def run(self):
        self.logger.debug("[localhost] WeaveTunnelStart: Run.")

        with self.getStateLockManager():
            self.__pre_check()

        self.__start_tunnel_test_server()
        self.__assign_tunnel_endpoint_ula_at_test_server()

        self.__start_test_tunnel_border_gateway_client()

        self.__wait_for_gateway()
        
        gateway_output_value, gateway_output_data = \
            self.get_test_output(self.border_gateway, self.gateway_process_tag, True)
        gateway_strace_value, gateway_strace_data = \
            self.get_test_strace(self.border_gateway, self.gateway_process_tag, True)

        self.__stop_server_side()
        service_output_value, service_output_data = \
            self.get_test_output(self.service, self.service_process_tag, True)
        service_strace_value, service_strace_data = \
            self.get_test_strace(self.service, self.service_process_tag, True)

        status, results = self.__process_results(gateway_output_data)

        data = {}
        data["gateway_output"] = gateway_output_data
        data["gateway_strace"] = gateway_strace_data
        data["service_output"] = service_output_data
        data["service_strace"] = service_strace_data

        self.logger.debug("[localhost] WeaveTunnelTest: Done.")

        return ReturnMsg(status, data)


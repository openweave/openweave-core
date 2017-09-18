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
#       Implements WeaveInet class that tests Weave Inet Layer among Weave Nodes.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from plugin.WeaveTest import WeaveTest


options = {}
options["client"] = None
options["server"] = None
options["count"] = None
options["length"] = None
options["type"] = None
options["quiet"] = False
options["tap"] = None


def option():
    return options.copy()


class WeaveInet(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-inet [-h --help] [-q --quiet] [-o --origin <NAME>] [-s --server <NAME>]
               [-c --count <NUMBER>] [-l --length <LEN>] [-t --type <PROTOCOL_TYPE>]
               [-p --tap <TAP_INTERFACE>]

    command to test inet:
        $ weave-inet -o BorderRouter -s ThreadNode -c 10 -l 50 -t tcp
            send 10 packets from BorderRouter to ThreadNode via tcp

        $ weave-inet -o BorderRouter -s ThreadNode -c 10 -l 50 -t udp
            send 10 packets from quzrtz to ThreadNode via udp

        $ weave-inet -o BorderRouter -s ThreadNode -c 10 -l 50 -t raw6
            send 10 packets from BorderRouter to ThreadNode via raw6

        $ weave-inet -o onhub -s cloud -c 10 -l 50 -t raw4
            send 10 packets from onhub to cloud via raw4

    Note:
        Support four protocols: tcp, udp, raw6, raw4

    return:
        0   success
        1   failure

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.type = opts["type"]
        self.count = opts["count"]
        self.length = opts["length"]
        self.client = opts["client"]
        self.server = opts["server"]
        self.tap = opts["tap"]

        self.no_service = False

        self.server_process_tag = "WEAVE-INET-SERVER"
        self.client_process_tag = "WEAVE-INET-CLIENT"

        self.client_node_id = None
        self.server_node_id = None


    def __pre_check(self):
        # Check if Weave Inet client node is given.
        if self.type != "udp" and self.type != "tcp" and self.type != "raw6" and self.type != "raw4":
            emsg = "Error proto type, support: udp, tcp, raw6, raw4."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        if self.client == None:
            emsg = "Missing name or address of the Weave Inet client node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Inet server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Inet server node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Inet client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave Inet server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        if self.type == "raw4":
            self.client_ip = self.getNodePublicIPv4Address(self.client_node_id)
            self.server_ip = self.getNodePublicIPv4Address(self.server_node_id)
        else:
            self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
            self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveInet: %s" % (emsg))
            sys.exit(1)

        if self.count != None and self.count.isdigit():
            self.count = int(float(self.count))
        else:
            self.count = 5

        if self.length != None and self.length.isdigit():
            self.length = int(float(self.length))
        else:
            self.length = 100


    def __process_results(self, output):

        for line in output.split("\n"):
            if not "Total received data length: " in line:
                continue

            total_len = line.split("length: ")[1].split(" bytes")[0]

        if self.quiet == False:
            print "weave-inet from node %s (%s) to node %s (%s) via %s: " % \
                (self.client_node_id, self.client_ip,
                 self.server_node_id, self.server_ip, self.type),

            if  total_len == str(self.count * self.length) or \
                self.type == "raw4" and total_len == str(self.count * (self.length + 20)):
                # extra 20 bytes ip header for each raw4 packet
                print hgreen("succeed")
                result = True
            else:
                print hred("failed")
                result = False
        return (result, output)
    

    def __start_server_side(self):
        if self.no_service:
            return

        cmd = "sudo "
        cmd += self.getWeaveInetLayerPath()

        cmd += " --local-addr " + self.server_ip
        cmd += " --listen"

        if self.type and self.type != "udp":
            cmd += " --" + self.type

        if self.tap:
            cmd += " --interface " + self.tap

        self.start_weave_process(self.server_node_id, cmd, self.server_process_tag, sync_on_output=self.ready_to_service_events_str)


    def __start_client_side(self):
        cmd = "sudo "
        cmd += self.getWeaveInetLayerPath()

        cmd += " --local-addr " + self.client_ip
        cmd += " " + self.server_ip

        if self.type and self.type != "udp":
            cmd += " --" + self.type

        cmd += " --length " + str(self.length)
        cmd += " --max-send " + str(self.count * self.length)

        if self.tap:
            cmd += " --interface " + self.tap

        self.start_weave_process(self.client_node_id, cmd, self.client_process_tag)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveInet: Run.")

        self.__pre_check()

        self.__start_server_side()

        emsg = "WeaveInet %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveInet: %s" % (self.server_node_id, emsg))

        self.__start_client_side()

        self.__wait_for_client()

        client_output_value, client_output_data = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        client_strace_value, client_strace_data = \
            self.get_test_strace(self.client_node_id, self.client_process_tag, True)


        if self.no_service:
            server_output_data = ""
            server_strace_data = ""
        else:
            self.__stop_server_side()
            server_output_value, server_output_data = \
                self.get_test_output(self.server_node_id, self.server_process_tag, True)
            server_strace_value, server_strace_data = \
                self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        avg, results = self.__process_results(server_output_data)

        data = {}
        data["client_output"] = client_output_data
        data["client_strace"] = client_strace_data
        data["server_output"] = server_output_data
        data["server_strace"] = server_strace_data

        self.logger.debug("[localhost] WeaveInet: Done.")

        return ReturnMsg(avg, data)


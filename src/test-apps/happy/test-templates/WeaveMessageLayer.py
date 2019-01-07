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
#       Implementes WeaveMessageLayer class that tests Weave MessageLayer among Weave Nodes.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest


options = {}
options["client"] = None
options["server"] = None
options["count"] = None
options["quiet"] = False
options["tcp"] = False
options["tap"] = None
options["use_persistent_storage"] = True


def option():
    return options.copy()


class WeaveMessageLayer(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-messagelayer [-h --help] [-q --quiet] [-c --client <NAME>] [-s --server <NAME>]
                        [-n -count <NUMBER>] [-t --tcp] [-p --tap <TAP_INTERFACE>]

    commands:
         $ weave-messagelayer -c node01 -s node02 -n 10
              test weave message send from node01 to node02 via udp

         $ weave-messagelayer -c node01 -s node02 -n 10 -t
              test weave message send from node01 to node02 via tcp

    return:
        0   success
        1   failure

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.count = opts["count"]
        self.client = opts["client"]
        self.server = opts["server"]
        self.tcp = opts["tcp"]
        self.tap = opts["tap"]

        self.server_process_tag = "WEAVE-MESSAGELAYER-SERVER"
        self.client_process_tag = "WEAVE-MESSAGELAYER-CLIENT"
        self.use_persistent_storage = opts["use_persistent_storage"]

        self.client_node_id = None
        self.server_node_id = None


    def __pre_check(self):
        # Check if Weave MessageLayer client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave MessageLayer client node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        # Check if Weave MessageLayer server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave MessageLayer server node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        # Check if Weave MessageLayer client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave MessageLayer server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveMessageLayer: %s" % (emsg))
            sys.exit(1)

        if self.count != None and self.count.isdigit():
            self.count = int(float(self.count))
        else:
            self.count = 5


    def __process_results(self, output):
        for line in output.split("\n"):
            if not "This is weave message " in line:
                continue

            total_count = line.split("message ")[1]

        if self.quiet == False:
            print "weave-messagelayer test from node %s (%s) to node %s (%s) : " % \
                (self.client_node_id, self.client_ip,
                 self.server_node_id, self.server_ip),

            if total_count == str(self.count):
                print hgreen("succeed!")
                result = True
            else:
                print hred("failed!") 
                result = False
        return (result, output)


    def __start_server_side(self):
        cmd = self.getWeaveMessageLayerPath()
        if not cmd:
            return

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_simple_weave_server(cmd, self.server_ip,
             self.server_node_id, self.server_process_tag, self.quiet, False, use_persistent_storage=self.use_persistent_storage)


    def __start_client_side(self):
        cmd = self.getWeaveMessageLayerPath()
        if not cmd:
            return

        cmd += " --count " + str(self.count) + " --interval 200"

        if self.tcp:
            cmd += " --tcp"

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_simple_weave_client(cmd, self.client_ip,
            self.server_ip, self.server_weave_id,
            self.client_node_id, self.client_process_tag, use_persistent_storage=self.use_persistent_storage)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveMessageLayer: Run.")

        self.__pre_check()

        self.__start_server_side()

        emsg = "WeaveMessageLayer %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveMessageLayer: %s" % (self.server_node_id, emsg))

        self.__start_client_side()

        self.__wait_for_client()

        client_output_value, client_output_data = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        client_strace_value, client_strace_data = \
            self.get_test_strace(self.client_node_id, self.client_process_tag, True)


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

        self.logger.debug("[localhost] WeaveMessageLayer: Done.")

        return ReturnMsg(avg, data)


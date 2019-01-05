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
#       Implements WeaveConnectionTunnel class that tests WeaveConnectionTunnel functionality.
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

from WeaveTest import WeaveTest


options = {}
options["quiet"] = False
options["agent"] = None
options["source"] = None
options["destination"] = None
options["tap"] = None


def option():
    return options.copy()


class WeaveConnectionTunnel(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-connection-tunnel tests WeaveConnectionTunnel functionality among three nodes

    weave-connection-tunnel [-h --help] [-q --quiet] [-a --agent <NODE_ID>]
        [-s --source <NODE_ID>] [-d --destination <NODE_ID>] [-p --tap <TAP_INTERFACE>]

    Example:
    $ weave-connection-tunnel -a node01 -s node02 -d node03
        Test WeaveConnectionTunnel functionality among three nodes:
         - node01: Agent, create connections to Tunnel Source and Destination, establish tunnel between them
         - node02: Source, wait for connection from Tunnel Agent, act as sender to verify tunnel link
         - node03: Destination, wait for connection from Tunnel Agent, act as receiver to verify tunnel link

    return:
        0    success
        1    fail
    """

    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.agent = opts["agent"]
        self.source = opts["source"]
        self.destination = opts["destination"]
        self.tap = opts["tap"]

        self.agent_process_tag = "WEAVE-TUNNEL-AGENT"
        self.source_process_tag = "WEAVE-TUNNEL-SOURCE"
        self.dest_process_tag = "WEAVE-TUNNEL-DEST"


    def __pre_check(self):
        # Check if agent node was given
        if self.agent == None:
            emsg = "Agent node id not specified."
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if not self._nodeExists(self.agent):
            emsg = "Agent node %s does not exist." % (self.agent)
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        # Check if source node was given
        if self.source == None:
            emsg = "Source node id not specified."
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if not self._nodeExists(self.source):
            emsg = "Source node %s does not exist." % (self.source)
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        # Check if destination node was given
        if self.destination == None:
            emsg = "Destination node id not specified."
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if not self._nodeExists(self.destination):
            emsg = "Destination node %s does not exist." % (self.destination)
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        self.agent_ip = self.getNodeWeaveIPAddress(self.agent)
        self.agent_weave_id = self.getWeaveNodeID(self.agent)

        self.source_ip = self.getNodeWeaveIPAddress(self.source)
        self.source_weave_id = self.getWeaveNodeID(self.source)

        self.dest_ip = self.getNodeWeaveIPAddress(self.destination)
        self.dest_weave_id = self.getWeaveNodeID(self.destination)


        # Check if all unknowns were found

        if self.agent_ip == None:
            emsg = "Could not find IP address of Tunnel Agent"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if self.agent_weave_id == None:
            emsg = "Could not find Weave node ID of Tunnel Agent"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if self.source_ip == None:
            emsg = "Could not find IP address of Tunnel Source"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if self.source_weave_id == None:
            emsg = "Could not find Weave node ID of Tunnel Source"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if self.dest_ip == None:
            emsg = "Could not find IP address of Tunnel Destination"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)

        if self.dest_weave_id == None:
            emsg = "Could not find Weave node ID of Tunnel Destination"
            self.logger.error("[localhost] WeaveConnectionTunnel: %s" % (emsg))
            sys.exit(1)


    def __start_tunnel_source(self):
        cmd = self.getWeaveConnectionTunnelPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.source_ip
        cmd += " --tunnel-source "

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.source, cmd, self.source_process_tag, sync_on_output = self.ready_to_service_events_str)


    def __stop_tunnel_source(self):
        self.stop_weave_process(self.source, self.source_process_tag)


    def __start_tunnel_dest(self):
        cmd = self.getWeaveConnectionTunnelPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.dest_ip
        cmd += " --tunnel-destination "

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.destination, cmd, self.dest_process_tag, sync_on_output = self.ready_to_service_events_str)


    def __wait_for_tunnel_dest(self):
        self.wait_for_test_to_end(self.destination, self.dest_process_tag)


    def __start_tunnel_agent(self):
        cmd = self.getWeaveConnectionTunnelPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.agent_ip
        cmd += " --tunnel-agent "
        cmd += self.source_weave_id + " " + self.dest_weave_id

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.agent, cmd, self.agent_process_tag)


    def __stop_tunnel_agent(self):
        self.stop_weave_process(self.agent, self.agent_process_tag)


    def __process_results(self, output):
        pass_test = False

        for line in output.split("\n"):
            if "Message from tunnel source node to destination node" in line:
                pass_test = True
                break

        print "weave-connection-tunnel test among Agent %s (%s), Source %s (%s), Destination %s (%s)" % \
                  (self.agent, self.agent_ip, self.source, self.source_ip, self.destination, self.dest_ip)

        if pass_test:
            print hgreen("succeeded")
        else:
            print hred("failed")

        return pass_test


    def run(self):
        self.logger.debug("[localhost] WeaveConnectionTunnel: Run.")

        self.__pre_check()

        self.__start_tunnel_source()
        self.__start_tunnel_dest()
        self.__start_tunnel_agent()

        self.__wait_for_tunnel_dest()
        self.__stop_tunnel_source()
        self.__stop_tunnel_agent()

        dest_output_value, dest_output_data = \
            self.get_test_output(self.destination, self.dest_process_tag, True)
        dest_strace_value, dest_strace_data = \
            self.get_test_strace(self.destination, self.dest_process_tag, True)

        source_output_value, source_output_data = \
            self.get_test_output(self.source, self.source_process_tag, True)
        source_strace_value, source_strace_data = \
            self.get_test_strace(self.source, self.source_process_tag, True)

        agent_output_value, agent_output_data = \
            self.get_test_output(self.agent, self.agent_process_tag, True)
        agent_strace_value, agent_strace_data = \
            self.get_test_strace(self.agent, self.agent_process_tag, True)

        result = self.__process_results(dest_output_data)

        data = {}
        data["dest_output"] = dest_output_data
        data["dest_strace"] = dest_strace_data
        data["source_output"] = source_output_data
        data["source_strace"] = source_strace_data
        data["agent_output"] = agent_output_data
        data["agent_strace"] = agent_strace_data

        self.logger.debug("[localhost] WeaveConnectionTunnel: Done.")
        return ReturnMsg(result, data)


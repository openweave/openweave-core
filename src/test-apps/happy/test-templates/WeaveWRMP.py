#!/usr/bin/env python


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

#
#    @file
#       Implements WeaveWRMP class that tests Weave Echo among Weave Nodes.
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
import plugins.plaid.Plaid as Plaid

options = {
    "client" : None,
    "server" : None,
    "quiet" : False,
    "test" : None,
    "retransmit" : None,
    "tap" : None,
    "strace" : True,
    "plaid" : "auto",
    "use_persistent_storage": True
}


def option():
    return options.copy()


class WeaveWRMP(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-wrmp [-h --help] [-q --quiet] [-c --client <NAME>] [-s --server <NAME>]
           [-t --test] [-r --retransmit <TIME_MICRO>]

    return:
        0    success
        1    fail
    """
    def __init__(self, opts = {}):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        defaultValues = dict(options)
        defaultValues.update(opts)

        self.__dict__.update(defaultValues)

        self.server_process_tag = "WEAVE-WRMP-SERVER" + (opts['test_tag'] if 'test_tag' in opts else '')
        self.client_process_tag = "WEAVE-WRMP-CLIENT" + (opts['test_tag'] if 'test_tag' in opts else '')
        self.plaid_server_process_tag = "PLAID_SERVER" + (opts['test_tag'] if 'test_tag' in opts else '')

        self.client_node_id = None
        self.server_node_id = None
        plaid_opts = Plaid.default_options()
        plaid_opts['quiet'] = self.quiet
        self.plaid_server_node_id = 'node03'
        plaid_opts['server_node_id'] = self.plaid_server_node_id
        plaid_opts['num_clients'] = 2
        plaid_opts['server_ip_address'] = self.getNodeWeaveIPAddress(self.plaid_server_node_id)
        plaid_opts['strace'] = self.strace
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = opts["plaid"]
        if opts["plaid"] == "auto":
            self.use_plaid = self.plaid.isPlaidConfigured()


    def __pre_check(self):
        # Check if Weave WRMP client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave WRMP client node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        # Check if Weave WRMP server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave WRMP server node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        if self.test != None:
            self.test = int(float(self.test))
        else:
            self.test = 1

        # Check if Weave WRMP client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave WRMP server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        # Check if client is provided in a form of IP address
        if IP.isIpAddress(self.client):
            self.client_node_id = self.getNodeIdFromAddress(self.client)

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.server):
            self.server_node_id = self.getNodeIdFromAddress(self.server)

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveWRMP: %s" % (emsg))
            sys.exit(1)

    def __process_results(self, client_output):
        # search for 'Passed' word
        pass_test = False

        for line in client_output.split("\n"):
            if "passed" in line.lower():
                pass_test = True
                break

        if self.quiet == False:
            print "weave-wrmp from node %s (%s) to node %s (%s) : " % \
                (self.client_node_id, self.client_ip,
                 self.server_node_id, self.server_ip),
            if pass_test:
                print hgreen("PASSED")
            else:
                print hred("FAILED")

        return (pass_test, client_output)

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveWdmNext: %s" % (self.plaid_server_node_id, emsg))

    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def __start_server_side(self):
        cmd = self.getWeaveWRMPPath()
        if not cmd:
            return

        if self.tap:
            cmd += " --tap-device " + self.tap

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_simple_weave_server(cmd, self.server_ip,
             self.server_node_id, self.server_process_tag, strace = self.strace, env = custom_env, use_persistent_storage=self.use_persistent_storage)


    def __start_client_side(self):
        cmd = self.getWeaveWRMPPath()
        if not cmd:
            return

        if self.tap:
            cmd += " --tap-device " + self.tap

        # Start WRMP client with test #
        cmd += " --test %d" % (self.test)

        if self.retransmit:
            cmd += " -R %s" % (self.retransmit)

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.client_node_id)

        self.start_simple_weave_client(cmd, self.client_ip,
            self.server_ip, self.server_weave_id,
            self.client_node_id, self.client_process_tag,  strace = self.strace, env = custom_env, use_persistent_storage = self.use_persistent_storage)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveWRMP: Run.")

        self.__pre_check()

        if self.use_plaid:
            self.__start_plaid_server()

        self.__start_server_side()

        emsg = "WeaveWRMP %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveWRMP: %s" % (self.server_node_id, emsg))

        self.__start_client_side()
        self.__wait_for_client()

        client_output_value, client_output_data = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        if self.strace:
            client_strace_value, client_strace_data = \
                                 self.get_test_strace(self.client_node_id, self.client_process_tag, True)

        self.__stop_server_side()

        server_output_value, server_output_data = \
            self.get_test_output(self.server_node_id, self.server_process_tag, True)

        if self.strace:
            server_strace_value, server_strace_data = \
                                 self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        if self.use_plaid:
            self.__stop_plaid_server()

        status, results = self.__process_results(client_output_data)

        data = {}
        data["client_output"] = client_output_data
        data["server_output"] = server_output_data
        if self.strace:
            data["client_strace"] = client_strace_data
            data["server_strace"] = server_strace_data

        self.logger.debug("[localhost] WeaveWRMP: Done.")
        return ReturnMsg(status, data)

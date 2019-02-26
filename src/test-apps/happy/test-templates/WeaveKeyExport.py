#!/usr/bin/env python

#
#    Copyright (c) 2017 Nest Labs, Inc.
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
#       Implements WeaveKeyExport class that tests Weave Key Export protocol among Weave nodes.
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
import WeaveUtilities
import plugins.plaid.Plaid as Plaid


options = {}
options["client"] = None
options["server"] = None
options["quiet"] = False
options["plaid"] = False
options["use_persistent_storage"] = True


def option():
    return options.copy()


class WeaveKeyExport(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-key-export [-h --help] [-q --quiet] [-o --origin <NAME>] [-s --server <NAME>]
           [-c --count <NUMBER>] [-u --udp] [-t --tcp] [-k --key-id <key-id>] [-d --dont-sign-msgs]
           [-p --tap <TAP_INTERFACE>] [--client_faults <fault-injection configuration>]
           [--server_faults <fault-injection configuration>]

    commands:
         $ weave-key-export -o node01 -s node02 -u
               weave key export test between node01 and node02 via UDP with requested default
               Client Root Key (key-id = 0x00010400)

         $ weave-key-export -o node01 -s node02 -t
               weave key export test between node01 and node02 via TCP with requested default
               Client Root Key (key-id = 0x00010400)

         $ weave-key-export -o node01 -s node02 -u --wrmp
               weave key export test between node01 and node02 via WRMP over UDP with requested
               default Client Root Key (key-id = 0x00010400)

         $ weave-key-export -o node01 -s node02 -u --wrmp --key-id 0x00005536
               weave key export test between node01 and node02 via WRMP over UDP with requested
               application key (key-id = 0x00005536)

    return:
        True or False for test

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        default_values = {
            "count": None,
            'udp': True,
            'wrmp': False,
            'tcp': False,
            "sign_msgs": True,
            "key_id": "0x00010400",
            'tap': None,
            'client_faults': None,
            'server_faults': None,
            'iterations': None,
            'test_tag': ""
        }

        default_values.update(opts)

        self.__dict__.update(default_values)

        self.no_service = False

        self.server_process_tag = "WEAVE_KEY_EXPORT_SERVER" + opts["test_tag"]
        self.client_process_tag = "WEAVE_KEY_EXPORT_CLIENT" + opts["test_tag"]
        self.plaid_server_process_tag = "PLAID_SERVER" + opts["test_tag"]

        self.client_node_id = None
        self.server_node_id = None

        plaid_opts = Plaid.default_options()
        plaid_opts['quiet'] = self.quiet
        self.plaid_server_node_id = 'node03'
        plaid_opts['server_node_id'] = self.plaid_server_node_id
        plaid_opts['num_clients'] = 2
        plaid_opts['server_ip_address'] = self.getNodeWeaveIPAddress(self.plaid_server_node_id)
        plaid_opts['interface'] = 'wlan0'
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = opts["plaid"]
        if opts["plaid"] == "auto":
            if self.server == "service":
                # can't use plaid when talking to an external service
                self.use_plaid = False
            else:
                self.use_plaid = self.plaid.isPlaidConfigured()


    def __pre_check(self):
        # Check if Weave Key Export client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave Key Export client node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Key Export server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Key Export server node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if self.count != None and self.count.isdigit():
            self.count = int(float(self.count))
        else:
            self.count = 1

        # Check if Weave Key Export client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave Key Export server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        # Check if client is provided in a form of IP address
        if IP.isIpAddress(self.client):
            self.client_node_id = self.getNodeIdFromAddress(self.client)

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.server):
            self.no_service = True
            self.server_ip = self.server
            self.server_weave_id = self.IPv6toWeaveId(self.server)
        else:
            # Check if server is a true cloud service instance
            if self.getNodeType(self.server) == self.node_type_service:
                self.no_service = True

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if self.getNodeType(self.client_node_id) == "service":
            self.client_ip = self.getServiceWeaveIPAddress("KeyExport", self.client_node_id)
            self.client_weave_id = self.getServiceWeaveID("KeyExport", self.client_node_id)
        else:
            self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
            self.client_weave_id = self.getWeaveNodeID(self.client_node_id)

        if self.getNodeType(self.server_node_id) == "service":
            self.server_ip = self.getServiceWeaveIPAddress("KeyExport", self.server_node_id)
            self.server_weave_id = self.getServiceWeaveID("KeyExport", self.server_node_id)
        else:
            if not self.no_service:
                self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
                self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveKeyExport: %s" % (emsg))
            sys.exit(1)

    def __process_results(self, client_output):
        # search for "Received Key Export Response" phrase
        fail_test = True

        for line in client_output.split("\n"):
            if "Received Key Export Response" in line:
                fail_test = False
                break

        if self.quiet == False:
            print "weave-key-export requested by node %s (%s) from node %s (%s) : " % \
                (self.client_node_id, self.client_ip,
                 self.server_node_id, self.server_ip),

            if fail_test:
                print hred("FAILED")
            else:
                print hgreen("PASSED")

        return (fail_test, client_output)

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveKeyExport: %s" % (self.plaid_server_node_id, emsg))

    def __start_server_side(self):
        if self.no_service:
            return

        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters"

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.server_faults:
            cmd += " --faults " + self.server_faults

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_simple_weave_server(cmd, self.server_ip,
             self.server_node_id, self.server_process_tag, listen = False, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __start_client_side(self, pase_fail = False):
        cmd = self.getWeaveKeyExportPath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters"

        if self.tcp:
            cmd += " --tcp"
        else:
            # default is UDP
            cmd += " --udp"
            if self.wrmp:
                cmd += " --wrmp"

        cmd += " --key-id " + str(self.key_id)

        cmd += " --count " + str(self.count)

        if not self.sign_msgs:
            cmd += " --dont-sign-msgs "

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.client_faults:
            cmd += " --faults " + self.client_faults

        if self.iterations:
            cmd += " --iterations " + str(self.iterations)

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.client_node_id)

        self.start_simple_weave_client(cmd, self.client_ip,
            self.server_ip, self.server_weave_id,
            self.client_node_id, self.client_process_tag, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def __stop_server_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveKeyExport: Run.")

        self.__pre_check()

        if self.use_plaid:
            self.__start_plaid_server()

        self.__start_server_side()

        emsg = "WeaveKeyExport %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveKeyExport: %s" % (self.server_node_id, emsg))

        self.__start_client_side(False)
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
            if self.use_plaid:
                self.__stop_plaid_server()
            server_output_value, server_output_data = \
                self.get_test_output(self.server_node_id, self.server_process_tag, True)
            server_strace_value, server_strace_data = \
                self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        avg, results = self.__process_results(client_output_data)
        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(client_output_data)
        server_parser_error, server_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(server_output_data)

        data = {}
        data["client_output"] = client_output_data
        data["client_strace"] = client_strace_data
        data["server_output"] = server_output_data
        data["server_strace"] = server_strace_data
        data["other_failure"] = client_parser_error or client_leak_detected or server_parser_error or server_leak_detected

        self.logger.debug("[localhost] WeaveKeyExport: Done.")

        return ReturnMsg(avg, data)


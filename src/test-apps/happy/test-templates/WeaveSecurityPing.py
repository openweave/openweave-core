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
#       Implementes WeaveiSecurityPing class that tests Weave Security Ping(CASE, PASE) among Weave Nodes.
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
options["count"] = None
options["udp"] = True
options["wrmp"] = False
options["tcp"] = False
options["quiet"] = False
options["CASE"] = True
options["PASE"] = False
options["group_key"] = False
options["group_key_id"] = "0x00005536"
options["tap"] = None
options["client_faults"] = None
options["server_faults"] = None
options["iterations"] = None
options["plaid"] = False
options["test_tag"] = ""
options["strace"] = False
options["use_persistent_storage"] = True


def option():
    return options.copy()


class WeaveSecurityPing(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-security-ping [-h --help] [-q --quiet] [-o --origin <NAME>] [-s --server <NAME>]
           [-c --count <NUMBER>] [-u --udp] [-t --tcp] [-C --CASE] [-P --PASE] [-G --group-key [--group-key-id <key-id>]]
           [-p --tap <TAP_INTERFACE>] [--client_faults <fault-injection configuration>]
           [--server_faults <fault-injection configuration>]

    commands:
         $ weave-security-ping -o node01 -s node02 -u -C
               weave ping test between node01 and node02 via udp using CASE authenticate

         $ weave-security-ping -o node01 -s node02 -t -C
               weave ping test between node01 and node02 via tcp using CASE authenticate

         $ weave-security-ping -o node01 -s node02 -t -P
               weave ping test between node01 and node02 via tcp using PASE authenticate

         $ weave-security-ping -o node01 -s node02 -u --wrmp -G
               weave ping test between node01 and node02 via wrmp over udp using group-key encryption,
               using the default 0x00005536 key-id

         $ weave-security-ping -o node01 -s node02 -u --wrmp -G --group-key-id 0x00005536
               weave ping test between node01 and node02 via wrmp over udp using group-key encryption

         $ weave-security-ping -o node01 -s node02 -t -C --server_faults Weave_CASEKeyConfirm_s0_f1
               weave ping test between node01 and node02 via tcp using CASE authenticate;
               the server will fail the CASE key confirmation step

    return:
        0-100   percentage of the lost packets

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.tcp = opts["tcp"]
        self.udp = opts["udp"]
        self.wrmp = opts["wrmp"]
        self.case = opts["CASE"]
        self.pase = opts["PASE"]
        self.group_key = opts["group_key"]
        self.group_key_id = opts["group_key_id"]
        self.count = opts["count"]
        self.client = opts["client"]
        self.server = opts["server"]
        self.tap = opts["tap"]
        self.client_faults = opts["client_faults"]
        self.server_faults = opts["server_faults"]
        self.iterations = opts["iterations"]
        self.strace = opts["strace"]
        self.use_persistent_storage = opts["use_persistent_storage"]

        self.no_service = False

        self.server_process_tag = "WEAVE-SECURITY-PING-SERVER" + opts["test_tag"]
        self.client_process_tag = "WEAVE-SECURITY-PING-CLIENT" + opts["test_tag"]
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
        # Check if Weave Ping client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave Ping client node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Ping server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Ping server node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if self.count != None and self.count.isdigit():
            self.count = int(float(self.count))
        else:
            self.count = 1

        # Check if Weave Ping client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave Ping server node exists.
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
            # Check if server is a true clound service instance
            if self.getNodeType(self.server) == self.node_type_service:
                self.no_service = True

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if self.getNodeType(self.client_node_id) == "service":
            self.client_ip = self.getServiceWeaveIPAddress("Echo", self.client_node_id)
            self.client_weave_id = self.getServiceWeaveID("Echo", self.client_node_id)
        else:
            self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
            self.client_weave_id = self.getWeaveNodeID(self.client_node_id)

        if self.getNodeType(self.server_node_id) == "service":
            self.server_ip = self.getServiceWeaveIPAddress("Echo", self.server_node_id)
            self.server_weave_id = self.getServiceWeaveID("Echo", self.server_node_id)
        else:
            if not self.no_service:
                self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
                self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)

        num_security_modes = 0
        if self.case == True:
            num_security_modes += 1
        if self.pase == True:
            num_security_modes += 1
        if self.group_key == True:
            num_security_modes += 1

        if num_security_modes > 1:
            emsg = "Only one security mode can be enabled."
            self.logger.error("[localhost] WeaveSecurityPing: %s" % (emsg))
            sys.exit(1)



    def __process_results(self, output):
        # search for '%' e.g.  1/1(100.00%)
        loss_percentage = None

        # only consider the last iteration
        iterations = output.split("Iteration ")

        output = iterations[-1]

        for line in output.split("\n"):
            if not "%" in line:
                continue

            for token in line.split():
                if "%" in token:
                    loss_percentage = token.strip().split("(")[1]
                    loss_percentage = loss_percentage.translate(None, "()%")

        if loss_percentage == None:
            # hmmm, we haven't found our line
            loss_percentage = 100
        else:
            loss_percentage = float(loss_percentage)
            loss_percentage = 100 - loss_percentage

        if self.quiet == False:
            print "weave-security-ping from node %s (%s) to node %s (%s) : " % \
                (self.client_node_id, self.client_ip,
                 self.server_node_id, self.server_ip),

            if loss_percentage == 0:
                print hgreen("%d%% packet loss" % (loss_percentage))
            else:
                print hred("%d%% packet loss" % (loss_percentage))

        return (loss_percentage, output) # indicate the loss for each client

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveSecurityPing: %s" % (self.plaid_server_node_id, emsg))

    def __start_server_side(self):
        if self.no_service:
            return

        cmd = self.getWeavePingPath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters --idle-session-timeout 15000 --session-establishment-timeout 5000"

        if self.tcp:
            cmd += " --tcp"
        else:
            # default is UDP
            cmd += " --udp"
            if self.wrmp:
                cmd += " --wrmp"

        if self.pase:
            cmd += " --pase --pairing-code test"
        elif self.group_key:
            cmd += " --group-enc --group-enc-key-id " + self.group_key_id

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.server_faults:
            cmd += " --faults " + self.server_faults

        if self.server_faults or self.client_faults:
            cmd += " --extra-cleanup-time 10000"

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_simple_weave_server(cmd, self.server_ip,
             self.server_node_id, self.server_process_tag, strace=self.strace, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __start_client_side(self, pase_fail = False):
        cmd = self.getWeavePingPath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters --session-establishment-timeout 5000"

        if self.tcp:
            cmd += " --tcp"
        else:
            # default is UDP
            cmd += " --udp"
            if self.wrmp:
                cmd += " --wrmp"

        if self.pase:
            if pase_fail:
                cmd += " --pase --pairing-code fail"
            else:
                cmd += " --pase --pairing-code test"
        elif self.case:
            cmd += " --case"
        elif self.group_key:
            cmd += " --group-enc --group-enc-key-id 0x00005536"

        cmd += " --count " + str(self.count)

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.client_faults:
            cmd += " --faults " + self.client_faults

        if self.iterations:
            cmd += " --iterations " + str(self.iterations)

        if self.server_faults or self.client_faults:
            cmd += " --extra-cleanup-time 10000"

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.client_node_id)

        self.start_simple_weave_client(cmd, self.client_ip,
            self.server_ip, self.server_weave_id,
            self.client_node_id, self.client_process_tag, strace=self.strace, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def __stop_server_side(self):
        if self.no_service:
            return

        if (self.udp or self.wrmp) and (self.case):
            # Give enough time to the server to expire the session key at the end of the test.
            # the idle session timer is configured to 15 seconds; it has to fire twice to free the idle key.
            self.wait_for_test_time_secs(self.server_node_id, self.server_process_tag, secs=40)

        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveSecurityPing: Run.")

        self.__pre_check()

        if self.use_plaid:
            self.__start_plaid_server()

        self.__start_server_side()

        emsg = "WeaveSecurityPing %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveSecurityPing: %s" % (self.server_node_id, emsg))

        self.__start_client_side(False)
        self.__wait_for_client()

        client_output_value, client_output_data = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        if self.strace:
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
            if self.strace:
                server_strace_value, server_strace_data = \
                    self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        avg, results = self.__process_results(client_output_data)
        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(client_output_data)
        server_parser_error, server_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(server_output_data)

        data = {}
        data["client_output"] = client_output_data
        data["server_output"] = server_output_data
        if self.strace:
            data["client_strace"] = client_strace_data
            data["server_strace"] = server_strace_data
        data["other_failure"] = client_parser_error or client_leak_detected or server_parser_error or server_leak_detected

        self.logger.debug("[localhost] WeaveSecurityPing: Done.")

        return ReturnMsg(avg, data)


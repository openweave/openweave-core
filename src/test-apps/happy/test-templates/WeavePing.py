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
#       Implements WeavePing class that tests Weave Echo among Weave Nodes.
#

import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest


options = {"count": None,
           "udp": False,
           "tcp": False,
           "wrmp": False,
           "quiet": False,
           "no_service": False,
           "interval": None,
           "tap": None,
           "case": False,
           "case_shared": False,
           "timeout": None,
           "client_process_tag": "WEAVE-ECHO-CLIENT",
           "server_process_tag": "WEAVE-ECHO-SERVER",
           "endpoint": "Echo",
           "clients_info": [],
           "server_node_id": None,
           "strace": True,
           "case_cert_path": None,
           "case_key_path": None,
           "use_persistent_storage": True,
           "plaid_server_env": {},
           "plaid_client_env": {}
           }


def option():
    return options.copy()


class WeavePing(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-ping [-h --help] [-q --quiet] [-o --origin <NAME>] [-s --server <NAME>]
           [-c --count <NUMBER>] [-u --udp] [-t --tcp] [-w --wrmp] [-i --interval <ms>] [-p --tap <TAP_INTERFACE>]
           [-C --case] [--case-shared] [-E --case_cert_path <path>] [-T --case_key_path <path>]
    command to test weave ping over UDP:
        $ weave-ping -o node01 -s node02 -u -i 100 -c 5

    command to test weave ping over TCP:
        $ weave-ping -o node01 -s node02 -t

    command to test weave ping over WRMP:
        $ weave-ping -o node01 -s node02 -w

    command to test weave ping over WRMP and CASE:
        $ weave-ping -o node01 -s node02 -w --case --case_cert_path path --case_key_path path

    command to test weave ping over WRMP with shared CASE session to the Nest Core Router:
        $ weave-ping -o node01 -s node02 -w --case-shared --case_cert_path path --case_key_path path

    return:
        0-100   percentage of the lost packets

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)
        self.__dict__.update(opts)

    def __pre_check(self):
        # clear network info
        self.clients_info = []
        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeavePing: %s" % (emsg))
            sys.exit(1)

        if self.count != None and self.count.isdigit():
            self.count = int(float(self.count))
        else:
            self.count = 1

        for client in self.clients:
            client_node_id = None

            # Check if Weave Ping client node is given.
            if client == None:
                emsg = "Missing name or address of the Weave Ping client node."
                self.logger.error("[localhost] WeavePing: %s" % (emsg))
                sys.exit(1)

            # Check if Weave Ping client node exists.
            if self._nodeExists(client):
                client_node_id = client

            # Check if client is provided in a form of IP address
            if IP.isIpAddress(client):
                client_node_id = self.getNodeIdFromAddress(client)

            if client_node_id == None:
                emsg = "Unknown identity of the client node."
                self.logger.error("[localhost] WeavePing: %s" % (emsg))
                sys.exit(1)

            if self.getNodeType(client_node_id) == "service":
                client_ip = self.getServiceWeaveIPAddress(self.endpoint, client_node_id)
                client_weave_id = self.getServiceWeaveID(self.endpoint, client_node_id)
            else:
                client_ip = self.getNodeWeaveIPAddress(client_node_id)
                client_weave_id = self.getWeaveNodeID(client_node_id)

            if client_ip == None:
                emsg = "Could not find IP address of the client node."
                self.logger.error("[localhost] WeavePing: %s" % (emsg))
                sys.exit(1)

            if client_weave_id == None:
                emsg = "Could not find Weave node ID of the client node."
                self.logger.error("[localhost] WeavePing: %s" % (emsg))
                sys.exit(1)

            self.clients_info.append({'client': client, 'client_node_id': client_node_id, 'client_ip': client_ip,
                          'client_weave_id': client_weave_id, 'client_process_tag': client + "_" + self.client_process_tag + client})


        # Check if Weave Ping server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Ping server node."
            self.logger.error("[localhost] WeavePing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Ping server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.server):
            self.no_service = True
            self.server_ip = self.server
            self.server_weave_id = self.IPv6toWeaveId(self.server)
        elif IP.isDomainName(self.server) or self.server == "service":
            self.no_service = True
            self.server_ip = self.getServiceWeaveIPAddress(self.endpoint)
            self.server_weave_id = self.IPv6toWeaveId(self.server_ip)
        else:
            # Check if server is a true clound service instance
            if self.getNodeType(self.server) == self.node_type_service:
                self.no_service = True

        if not self.no_service and self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeavePing: %s" % (emsg))
            sys.exit(1)

        if self.getNodeType(self.server_node_id) == "service":
            self.server_ip = self.getServiceWeaveIPAddress(self.endpoint, self.server_node_id)
            self.server_weave_id = self.getServiceWeaveID(self.endpoint, self.server_node_id)
        else:
            if not self.no_service:
                self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
                self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeavePing: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeavePing: %s" % (emsg))
            sys.exit(1)


    def __process_results(self, client_info, output):
        # search for '%' e.g.  1/1(100.00%)
        loss_percentage = None

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
            print "weave-ping from node %s (%s) to node %s (%s) : " % \
                (client_info['client_node_id'], client_info['client_ip'],
                 self.server_node_id, self.server_ip),

            if loss_percentage == 0:
                print hgreen("%f%% packet loss" % (loss_percentage))
            else:
                print hred("%f%% packet loss" % (loss_percentage))

        return  loss_percentage # indicate the loss for each client
    

    def __start_server_side(self):
        if self.no_service:
            return

        cmd = self.getWeavePingPath()
        if not cmd:
            return

        if self.tcp:
            cmd += " --tcp"
        elif self.udp:
            # default is UDP
            cmd += " --udp"
        elif self.wrmp:
            cmd += " --wrmp"

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_simple_weave_server(cmd, self.server_ip,
             self.server_node_id, self.server_process_tag, use_persistent_storage = self.use_persistent_storage, env=self.plaid_server_env)


    def __start_client_side(self, client_info):
        cmd = self.getWeavePingPath()
        if not cmd:
            return

        if self.tcp:
            cmd += " --tcp"
        elif self.udp:
            # default is UDP
            cmd += " --udp"
        elif self.wrmp:
            cmd += " --wrmp"

        if self.interval != None:
            cmd += " --interval " + self.interval
    
        cmd += " --count " + str(self.count)

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.case:
            if self.case_shared:
                cmd += " --case-shared"
            else:
                cmd += " --case"

            if not self.case_cert_path == 'default':
                self.cert_file = self.case_cert_path if self.case_cert_path else os.path.join(self.main_conf['log_directory'], client_info["client_weave_id"].upper() + '-cert.weave-b64')
                self.key_file = self.case_key_path if self.case_key_path else os.path.join(self.main_conf['log_directory'], client_info["client_weave_id"].upper() + '-key.weave-b64')
                cmd += ' --node-cert ' + self.cert_file + ' --node-key ' + self.key_file

        self.start_simple_weave_client(cmd, client_info['client_ip'],
            self.server_ip, self.server_weave_id,
            client_info['client_node_id'], client_info['client_process_tag'], use_persistent_storage=self.use_persistent_storage, env=self.plaid_client_env)


    def __wait_for_client(self, client_info):
        self.wait_for_test_to_end(client_info["client_node_id"], client_info["client_process_tag"], timeout=self.timeout)


    def __stop_server_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        all_data = []
        self.logger.debug("[localhost] WeavePing: Run.")

        self.__pre_check()

        self.__start_server_side()

        emsg = "WeavePing %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeavePing: %s" % (self.server_node_id, emsg))

        for client_info in self.clients_info:
            self.__start_client_side(client_info)
        for client_info in self.clients_info:
            self.__wait_for_client(client_info)
        for client_info in self.clients_info:
            client_output_value, client_output_data = \
                self.get_test_output(client_info['client_node_id'], client_info['client_process_tag'], True)
            client_strace_value, client_strace_data = \
                self.get_test_strace(client_info['client_node_id'], client_info['client_process_tag'], True)

            if self.no_service:
                server_output_data = ""
                server_strace_data = ""
            else:
                self.__stop_server_side()
                server_output_value, server_output_data = \
                    self.get_test_output(self.server_node_id, self.server_process_tag, True)
                server_strace_value, server_strace_data = \
                    self.get_test_strace(self.server_node_id, self.server_process_tag, True)

            loss_percentage = self.__process_results(client_info, client_output_data)

            data = {}
            data.update(client_info)
            data["client_output"] = client_output_data
            data["server_output"] = server_output_data
            if self.strace:
                data["client_strace"] = client_strace_data
                data["server_strace"] = server_strace_data
            all_data.append(data)

        self.logger.debug("[localhost] WeavePing: Done.")

        return ReturnMsg(loss_percentage, all_data)


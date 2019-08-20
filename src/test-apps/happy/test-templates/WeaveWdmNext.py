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
#       Implementes WeaveWdmNext Next class that tests Weave WDM Next among Weave Nodes.
#

import os
import re
import sys
import time

from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork
from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP


from functools import reduce
from WeaveTest import WeaveTest
import plugins.plaid.Plaid as Plaid
import WeaveUtilities
import WeaveWdmNextOptions as wwno


def option():
    return wwno.OPTIONS.copy()


class WeaveWdmNext(HappyNode, HappyNetwork, WeaveTest):
    """
     Runs a WDM test  between a client and a server.
    """

    def __init__(self, opts=option()):
        """
        Initialize test parameters and plaid.

        Args:
          options (dict): Test parameters.
        """
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.options = opts
        self.no_service = False
        self.server_process_tag = "WEAVE_WDM_SERVER" + \
            self.options[wwno.TEST].get(wwno.TEST_TAG, "")
        self.client_process_tag = "WEAVE_WDM_CLIENT" + \
            self.options[wwno.TEST].get(wwno.TEST_TAG, "")
        self.plaid_server_process_tag = "PLAID_SERVER" + \
            self.options[wwno.TEST].get(wwno.TEST_TAG, "")
        self.client_node_id = None
        self.server_node_id = None
        self.clients_info = []
        self.wdm_client_option = None
        self.wdm_server_option = None

        plaid_opts = Plaid.default_options()
        plaid_opts["quiet"] = self.options[wwno.TEST][wwno.QUIET]
        self.plaid_server_node_id = "node03"
        plaid_opts["server_node_id"] = self.plaid_server_node_id
        plaid_opts["num_clients"] = 2
        plaid_opts["server_ip_address"] = self.getNodeWeaveIPAddress(self.plaid_server_node_id)
        plaid_opts["strace"] = self.options[wwno.TEST][wwno.STRACE]
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = self.options[wwno.TEST][wwno.PLAID]
        if self.use_plaid == "auto":
            if self.options[wwno.TEST][wwno.SERVER] == "service":
                # Can't use plaid when talking to an external service
                self.use_plaid = False
            else:
                self.use_plaid = self.plaid.isPlaidConfigured()

    def __pre_check(self):
        """
        Perform set of checks to before running Wdm next tests.
        """
        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveWdmNext: %s" % (emsg))
            sys.exit(1)

        # Check if Weave WDM server node is given.
        if not self.options[wwno.TEST][wwno.SERVER]:
            emsg = "Missing name or address of the WeaveWdmNext server node."
            self.logger.error("[localhost] WeaveWdmNext: %s" % (emsg))
            sys.exit(1)

        # Check if WeaveWdmNext server node exists.
        if self._nodeExists(self.options[wwno.TEST][wwno.SERVER]):
            self.server_node_id = self.options[wwno.TEST][wwno.SERVER]
            self.server_process_tag = self.server_node_id + "_" + self.server_process_tag

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.options[wwno.TEST][wwno.SERVER]):
            self.no_service = True
            self.server_ip = self.options[wwno.TEST][wwno.SERVER]
            self.server_weave_id = self.IPv6toWeaveId(self.options[wwno.TEST][wwno.SERVER])
        elif IP.isDomainName(self.options[wwno.TEST][wwno.SERVER]) \
                or self.options[wwno.TEST][wwno.SERVER] == "service":
            self.no_service = True
            self.server_ip = self.getServiceWeaveIPAddress("DataManagement")
            self.server_weave_id = self.IPv6toWeaveId(self.server_ip)
        else:
            # Check if server is a true cloud service instance
            if self.getNodeType(self.options[wwno.TEST][wwno.SERVER]) == self.node_type_service:
                self.no_service = True

        if not self.no_service and self.server_node_id is None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
            sys.exit(1)

        if self.getNodeType(self.server_node_id) == "service":
            self.server_ip = self.getServiceWeaveIPAddress("WeaveWdmNext", self.server_node_id)
            self.server_weave_id = self.getServiceWeaveID("WeaveWdmNext", self.server_node_id)
        else:
            if not self.no_service:
                self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
                self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        if self.server_ip is None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
            sys.exit(1)

        if not self.no_service and self.server_weave_id is None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
            sys.exit(1)

        for client in self.options[wwno.TEST][wwno.CLIENT]:
            client_node_id = None
            client_ip = None
            client_weave_id = None
            # Check if Weave Wdm Next client node is given.
            if client is None:
                emsg = "Missing name or address of the WeaveWdmNext client node."
                self.logger.error("[localhost] WeaveWdmNext: %s" % (emsg))
                sys.exit(1)

            # Check if WeaveWdmNext client node exists.
            if self._nodeExists(client):
                client_node_id = client

            # Check if client is provided in a form of IP address
            if IP.isIpAddress(client):
                client_node_id = self.getNodeIdFromAddress(client)

            if client_node_id is None:
                emsg = "Unknown identity of the client node."
                self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
                sys.exit(1)

            if self.getNodeType(client_node_id) == "service":
                client_ip = self.getServiceWeaveIPAddress("WeaveWdmNext", client_node_id)
                client_weave_id = self.getServiceWeaveID("WeaveWdmNext", client_node_id)
            else:
                client_ip = self.getNodeWeaveIPAddress(client_node_id)
                client_weave_id = self.getWeaveNodeID(client_node_id)

            if client_ip is None:
                emsg = "Could not find IP address of the client node."
                self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
                sys.exit(1)

            if client_weave_id is None:
                emsg = "Could not find Weave node ID of the client node."
                self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
                sys.exit(1)
            self.clients_info.append({"client": client, "client_node_id": client_node_id, "client_ip": client_ip,
                                      "client_weave_id": client_weave_id, "client_process_tag": client + "_" + self.client_process_tag})

        if self.options[wwno.TEST][wwno.WDM_OPTION] == "view":
            self.wdm_client_option = " --wdm-simple-view-client"
            self.wdm_server_option = " --wdm-simple-view-server"
            print "view disabled"
            sys.exit(1)
        elif self.options[wwno.TEST][wwno.WDM_OPTION] == "one_way_subscribe":
            self.wdm_client_option = " --wdm-one-way-sub-client"
            self.wdm_server_option = " --wdm-one-way-sub-publisher"
        elif self.options[wwno.TEST][wwno.WDM_OPTION] == "mutual_subscribe":
            self.wdm_client_option = " --wdm-init-mutual-sub"
            self.wdm_server_option = " --wdm-resp-mutual-sub"
        elif self.options[wwno.TEST][wwno.WDM_OPTION] == "subless_notify":
            self.wdm_client_option = " --wdm-simple-subless-notify-client"
            self.wdm_server_option = " --wdm-simple-subless-notify-server"
        else:
            emsg = "NOT SUPPORTED WDM OPTION"
            self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
            sys.exit(1)

    def __process_results(self, client_output, server_output, client_info):
        """
        Check for parser errors and leaks on client and server.

        Args:
            client_output (str): Log output from client node.
            server_output (str): Log output from server node.
            client_info (dict): Client node information.

        Returns:
            dict: Results of client/server parser error and leaks.
        """
        client_parser_error = None
        client_leak_detected = None
        server_parser_error = None
        server_leak_detected = None
        result = {}
        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(
            client_output)
        result["no_client_parser_error"] = not client_parser_error
        result["no_client_leak_detected"] = not client_leak_detected
        if server_output:
            server_parser_error, server_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(
                server_output)
            result["no_server_parser_error"] = not client_parser_error
            result["no_server_leak_detected"] = not server_leak_detected

        if not self.options[wwno.TEST][wwno.QUIET]:
            print "weave-wdm-next %s from node %s (%s) to node %s (%s) : "\
                  % (self.options[wwno.TEST][wwno.WDM_OPTION],
                     client_info["client_node_id"], client_info["client_ip"],
                     self.server_node_id, self.server_ip)

            if client_parser_error:
                print hred("client parser error")
            if client_leak_detected:
                print hred("client_resource leak detected")
            if server_parser_error:
                print hred("server parser error")
            if server_leak_detected:
                print hred("server resource leak detected")

        return result

    def wdm_next_checksum_check_process(self, log):
        """
        Pull out checksum value list for each trait object sent or received.

        Args:
            log (str): Logs from client/server node.

        Returns:
          list: List of checksum values.
        """
        checksum_list = re.findall("Checksum is (\w+)", log)
        return checksum_list

    def wdm_next_client_event_sequence_process(self, log):
        """
        Pull out the number of events sent in a notification.
        Only notifications with non-zero events are reported.

        Args:
            log (str): Logs from client node.

        Returns:
            dict: Publisher event list data.
        """
        success = True
        client_event_list = re.findall("Fetched (\d+) events", log)
        client_event_process_list = [int(event) for event in client_event_list if int(event) > 0]
        return {"client_event_list": client_event_process_list,
                "length": len(client_event_process_list), "success": success}

    def wdm_next_publisher_event_sequence_process(self, log):
        """
        Pull out the number of events received in a notification.
        All notifications with EventList element are reported.
        It would be considered an error to receive an empty EventList.

        Args:
            log(str): Logs from publisher node node.

        Returns:
            dict: Publisher event list data.
        """
        publisher_event_list = re.finditer("WEAVE:DMG: \tEventList =", log)
        success = True
        event_list_len = []
        for eventListMarker in publisher_event_list:
            startEventList = eventListMarker.end()
            endEventListMarker = re.search(".*WEAVE:DMG: \t],.*", log[startEventList:])
            if endEventListMarker:
                endEventList = endEventListMarker.end()
                event_list_len.append(
                    len(re.findall("WEAVE:DMG: \t\t{", log[startEventList:(startEventList + endEventList)])))
            else:
                success = False
        return {"publisher_event_list": event_list_len,
                "length": len(event_list_len), "success": success}

    def __start_plaid_server(self):
        """
        Start the plaid server.
        """
        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveWdmNext: %s" % (self.plaid_server_node_id, emsg))

    def __start_server_side(self):
        """
        Configure server side node with server test parameters.
        """
        if self.no_service:
            return
        cmd = self.getWeaveWdmNextPath()
        if not cmd:
            return
        self.server_id = self.server_weave_id
        cmd += self.wdm_server_option

        if self.options[wwno.TEST][wwno.WDM_OPTION] == "subless_notify":
            cmd += " --wdm-subless-notify-dest-node " + self.clients_info[0]["client_weave_id"]
            cmd += " --wdm-subnet 6"

        # Loop through server side test parameters
        for key, value in self.options[wwno.SERVER].items():
            if key == wwno.LOG_CHECK:
                continue
            if isinstance(value, bool) and value:
                # parameter is a True/False to add CLI option
                cmd += " --{}".format(key.replace('_', '-'))
            elif not isinstance(value, bool) and value is not None:
                # parameter is a value
                cmd += " --{} {}".format(key.replace('_', '-'), value)

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_simple_weave_server(
            cmd,
            self.server_ip,
            self.server_node_id,
            self.server_process_tag,
            listen=False,
            strace=self.options[wwno.TEST][wwno.STRACE],
            env=custom_env,
            use_persistent_storage=self.options[wwno.TEST][wwno.USE_PERSISTENT_STORAGE])

    def __start_client_side(self, client_info):
        """
        Configure client side node with client test parameters.

        Args:
            client_info(dict): Holds client node information.
        """
        cmd = self.getWeaveWdmNextPath()
        if not cmd:
            return
        cmd += self.wdm_client_option

        if self.options[wwno.TEST][wwno.WDM_OPTION] != "subless_notify":
            cmd += " --wdm-publisher " + self.server_weave_id
        if self.options[wwno.TEST][wwno.SERVER] == "service":
            node_id = client_info["client_weave_id"]
            fabric_id = self.getFabricId()
            client_info["client_ip"] = None
            cmd += " --wdm-subnet 5 --node-id=%s --fabric-id %s " % (node_id, fabric_id)
        if self.options[wwno.CLIENT][wwno.CASE]:
            cert_file = self.options[wwno.CLIENT][wwno.CASE_CERT_PATH] or os.path.join(
                self.main_conf["log_directory"], node_id.upper() + "-cert.weave-b64")
            key_file = self.options[wwno.CLIENT][wwno.CASE_KEY_PATH] or os.path.join(
                self.main_conf["log_directory"], node_id.upper() + "-key.weave-b64")
            cmd += " --{} --{} {} --{} {}".format(
                wwno.CASE, wwno.CASE_CERT_PATH.replace('_', '-'),
                cert_file, wwno.CASE_KEY_PATH.replace('_', '-'), key_file)
        if self.options[wwno.CLIENT][wwno.GROUP_ENC]:
            cmd += ' --{} --{} {}'.format(
                wwno.GROUP_ENC.replace('_', '-'), wwno.GROUP_ENC_KEY_ID.replace('_', '-'),
                self.options[wwno.CLIENT][wwno.GROUP_ENC_KEY_ID])

        # Loop through client side test parameters
        for key, value in self.options[wwno.CLIENT].items():
            if key in [wwno.CASE, wwno.CASE_CERT_PATH, wwno.CASE_KEY_PATH,
                       wwno.GROUP_ENC, wwno.GROUP_ENC_KEY_ID, wwno.LOG_CHECK]:
                    # already dealt with this special case
                continue
            if isinstance(value, bool) and value:
                # parameter is a True/False to add CLI option
                cmd += " --{}".format(key.replace('_', '-'))
            elif not isinstance(value, bool) and value is not None:
                # parameter is a value
                cmd += " --{} {}".format(key.replace('_', '-'), value)

        custom_env = {}

        if False:
            try:
                print "cmd = \n" + cmd
                print "sleeping..."
                time.sleep(60 * 60)
            except:
                print "sleep interrupted"

        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(client_info["client_node_id"])

        self.start_simple_weave_client(cmd, client_info["client_ip"], None, None,
                                       client_info["client_node_id"],
                                       client_info["client_process_tag"],
                                       strace=self.options[wwno.TEST][wwno.STRACE],
                                       env=custom_env,
                                       use_persistent_storage=self.options[wwno.TEST][wwno.USE_PERSISTENT_STORAGE])

    def __wait_for_client(self, client_info):
        """
        Wait for client to finish test execution.

        Args:
            client_info(dict): Holds client node data.
        """
        self.wait_for_test_to_end(
            client_info["client_node_id"],
            client_info["client_process_tag"],
            quiet=False,
            timeout=self.options[wwno.TEST][wwno.TIMEOUT])

    def __stop_server_side(self):
        """
        Stop weave process on server node.
        """
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)

    def __stop_client_side(self):
        """
        Stop weave process on client node.
        """
        if self.no_service:
            return

        self.stop_weave_process(self.client_node_id, self.client_process_tag)

    def __stop_plaid_server(self):
        """
        Stop the plaid server.
        """
        self.plaid.stopPlaidServerProcess()

    def run_client_test(self):
        """
        Run Weave wdm next test with test parameters.

        Returns:
            ReturnMsg: Test run results data.
        """
        all_data = []
        success = True
        self.__pre_check()

        if self.use_plaid:
            self.__start_plaid_server()

        self.__start_server_side()

        emsg = "WeaveWdmNext %s should be running." % self.server_process_tag
        self.logger.debug("[%s] WeaveWdmNext: %s" % (self.server_node_id, emsg))

        for client_info in self.clients_info:
            self.__start_client_side(client_info)
        for client_info in self.clients_info:
            self.__wait_for_client(client_info)
        for client_info in self.clients_info:
            client_output_value, client_output_data = \
                self.get_test_output(
                    client_info["client_node_id"],
                    client_info["client_process_tag"],
                    True)
            if self.options[wwno.TEST][wwno.STRACE]:
                client_strace_value, client_strace_data = \
                    self.get_test_strace(
                        client_info["client_node_id"],
                        client_info["client_process_tag"],
                        True)

            if self.no_service:
                server_output_data = ""
                server_strace_data = ""
            else:
                self.__stop_server_side()
                if self.use_plaid:
                    self.__stop_plaid_server()
                server_output_value, server_output_data = \
                    self.get_test_output(self.server_node_id, self.server_process_tag, True)
                if self.options[wwno.TEST][wwno.STRACE]:
                    server_strace_value, server_strace_data = \
                        self.get_test_strace(self.server_node_id, self.server_process_tag, True)

            success_dic = self.__process_results(
                client_output_data, server_output_data, client_info)

            success = reduce(lambda x, y: x and y, success_dic.values())

            data = {}
            data.update(client_info)
            data["client_output"] = client_output_data
            data["server_output"] = server_output_data
            if self.options[wwno.TEST][wwno.STRACE]:
                data["client_strace"] = client_strace_data
                data["server_strace"] = server_strace_data
            data["success_dic"] = success_dic
            all_data.append(data)

        return ReturnMsg(success, all_data)

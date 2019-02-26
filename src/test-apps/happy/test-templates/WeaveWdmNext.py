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

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest
import WeaveUtilities
import plugins.plaid.Plaid as Plaid

options = { "clients": None,
            "server": None,
            "quiet": False,
            "tap": None,
            "wdm_option": None,
            "strace": True,
            "plaid": "auto",
            "timeout": None,
            "enable_client_stop": True,
            "save_client_perf": False,
            "client_faults": None,
            "server_faults": None,
            "client_event_generator": False,
            "client_inter_event_period": None,
            "enable_server_stop": False,
            "save_server_perf": False,
            "server_event_generator": None,
            "server_inter_event_period": None,
            "wdm_client_liveness_check_period": None,
            "wdm_server_liveness_check_period": None,
            "wdm_subless_notify_dest_node": None,
            "case": False,
            "enable_retry": False,
            "case_cert_path": None,
            "enable_mock_event_timestamp_initial_counter": False,
            "case_key_path": None,
            "group_enc": False,
            "group_enc_key_id": None,
            "use_persistent_storage": True,
            "total_client_count": None,
            "total_server_count": None,
            "final_client_status": None,
            "final_server_status": None,
            "timer_client_period": None,
            "timer_server_period": None,
            "test_client_iterations": None,
            "test_server_iterations": None,
            "test_client_delay": None,
            "test_server_delay": None,
            "enable_client_flip": None,
            "enable_server_flip": None,
            "client_update_mutation": None,
            "client_update_conditionality": None,
            "client_update_timing": None,
            "client_update_num_mutations": None,
            "client_update_num_repeated_mutations": None,
            "client_update_num_traits": None,
            "client_update_discard_on_error": False,
          }


def option():
    return options.copy()


class WeaveWdmNext(HappyNode, HappyNetwork, WeaveTest):
    """
     Runs a WDM test  between a client and a server. The following options are supported.

     test_client_case, test_server_case:

       Maps to --test-case
       Controls which traits are published and subscribed to.
       The same value needs to be specified for both, for the subscriptions to work


     total_client_count, total_server_count:

       Maps to --total-count
       The number of times the node will mutate the trait instance per iteration;
       The frequency of the mutations is controlled by timer_server_period and timer_client_period
       Note that the server sends a custom command at the same time it mutates the trait instance.
       The command type alternates between 1 and 2. Command 1 is served immediately by the client;
       Command 2 is executed with a delay (the client sends a kMsgType_InProgress).


     timer_server_period, timer_client_period:

       Maps to --timer-period
       The period of the trait instance mutations.


     final_client_status, final_server_status:

       Maps to --final-status, with the following values:
       0(client cancel), 1(publisher cancel), 2(client abort), 3(Publisher abort), 4(Idle)
       This parameter controls what ends the test iteration. Both the client and the server have
       a subscription client and a subscription handler (publisher). The node will terminate the
       iteration with a Cancel or Abort, after either its publisher or its client have performed
       all mutations and have become Idle.
       A value of 4(Idle) means that the node will just wait for the other node to act.


     test_client_iterations:

       Maps to --test-iterations on the client node
       The number of times the test sequence (establish subscriptions, mutate N times,
       cancel or abort) should be repeated.


     test_server_iterations:

       Maps to --test-iterations on the server node
       This is 1 by default and our tests never change it, since the tests are driven from
       the client side and the server node is passive.


     test_client_delay, test_server_delay:

       Maps to --test-delay.
       The nodes will sleep for the given number of milliseconds between iterations
       Currently, this needs to be more than 2 seconds if the server is sending commands, since the
       client executes command type 2 with a delay of 2 seconds.


     client_clear_state_between_iterations, server_clear_state_between_iterations:

       Maps to --clear-state-between-iterations
       Resets the state of the data sink after each iteration


     enable_client_stop, enable_server_stop:

       Maps to --enable-stop
       Controls whether the process will exit the iteration or not at the end of the iteration
       This is always enabled on the client, and disabled on the server; the caller should
       not need to change these values.


     client_event_generator, server_event_generator:

       Maps to --event-generator
       Controls the event generator (None | Debug | Livenesss | Security | Telemetry | TestTrait)
       running in the node's background.
       See the option help string in TestWdmNext.cpp. The frequency at which events are generated
       is controlled by client_inter_event_period and server_inter_event_period


     client_inter_event_period, server_inter_event_period:

       Maps to --inter-event-period
       The period used by the event generator in milliseconds
    """

    def __init__(self, opts=options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.__dict__.update(opts)

        self.no_service = False

        self.server_process_tag = "WEAVE_WDM_SERVER" + (opts["test_tag"] if "test_tag" in opts else "")
        self.client_process_tag = "WEAVE_WDM_CLIENT" + (opts["test_tag"] if "test_tag" in opts else "")
        self.plaid_server_process_tag = "PLAID_SERVER" + (opts["test_tag"] if "test_tag" in opts else "")

        self.client_node_id = None
        self.server_node_id = None

        self.clients_info = []

        self.wdm_client_option = None
        self.wdm_server_option = None

        plaid_opts = Plaid.default_options()
        plaid_opts["quiet"] = self.quiet
        self.plaid_server_node_id = "node03"
        plaid_opts["server_node_id"] = self.plaid_server_node_id
        plaid_opts["num_clients"] = 2
        plaid_opts["server_ip_address"] = self.getNodeWeaveIPAddress(self.plaid_server_node_id)
        plaid_opts["strace"] = self.strace
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = opts["plaid"]
        if opts["plaid"] == "auto":
            if self.server == "service":
                # can't use plaid when talking to an external service
                self.use_plaid = False
            else:
                self.use_plaid = self.plaid.isPlaidConfigured()


    def __pre_check(self):
        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveWdmNext: %s" % (emsg))
            sys.exit(1)

        # Check if Weave WDM server node is given.
        if self.server == None:
            emsg = "Missing name or address of the WeaveWdmNext server node."
            self.logger.error("[localhost] WeaveWdmNext: %s" % (emsg))
            sys.exit(1)

        # Check if WeaveWdmNext server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server
            self.server_process_tag = self.server_node_id + "_" + self.server_process_tag

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.server):
            self.no_service = True
            self.server_ip = self.server
            self.server_weave_id = self.IPv6toWeaveId(self.server)
        elif IP.isDomainName(self.server) or self.server == "service":
            self.no_service = True
            self.server_ip = self.getServiceWeaveIPAddress("DataManagement")
            self.server_weave_id = self.IPv6toWeaveId(self.server_ip)
        else:
            # Check if server is a true cloud service instance
            if self.getNodeType(self.server) == self.node_type_service:
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

        for client in self.clients:
            client_node_id = None
            client_ip = None
            client_weave_id = None
            # Check if Weave Wdm Next client node is given.
            if client == None:
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

        if self.wdm_option == "view":
            self.wdm_client_option = " --wdm-simple-view-client"
            self.wdm_server_option = " --wdm-simple-view-server"
            print "view disabled"
            sys.exit(1)
        elif self.wdm_option == "one_way_subscribe":
            self.wdm_client_option = " --wdm-one-way-sub-client"
            self.wdm_server_option = " --wdm-one-way-sub-publisher"
        elif self.wdm_option == "mutual_subscribe":
            self.wdm_client_option = " --wdm-init-mutual-sub"
            self.wdm_server_option = " --wdm-resp-mutual-sub"
        elif self.wdm_option == "update":
            self.wdm_client_option = " --wdm-simple-update-client"
            self.wdm_server_option = " --wdm-simple-update-server"
        elif self.wdm_option == "subless_notify":
            self.wdm_client_option = " --wdm-simple-subless-notify-client"
            self.wdm_server_option = " --wdm-simple-subless-notify-server"
        else:
            emsg = "NOT SUPPORTED WDM OPTION"
            self.logger.error("[localhost] WeaveWdmNext: %s" % emsg)
            sys.exit(1)

    def __process_results(self, client_output, server_output, client_info):
        kevents = re.findall(r"WEAVE:.+kEvent_\w+", client_output)
        smoke_check = False
        client_parser_error = None
        client_leak_detected = None
        server_parser_error = None
        server_leak_detected = None
        result = {}
        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(client_output)
        result["no_client_parser_error"] = not client_parser_error
        result["no_client_leak_detected"] = not client_leak_detected
        if server_output is not "":
            server_parser_error, server_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(server_output)
            result["no_server_parser_error"] = not client_parser_error
            result["no_server_leak_detected"] = not server_leak_detected
        if self.wdm_option == "view":
            if any("kEvent_ViewResponseReceived" in s for s in kevents) and \
                            kevents[-1] == "kEvent_ViewResponseConsumed":
                smoke_check = True
            else:
                smoke_check = False
        elif self.wdm_option == "one_way_subscribe":
            if any("Client->kEvent_OnSubscriptionEstablished" in s for s in kevents) \
                    and "Closing endpoints" in client_output:
                smoke_check = True
            else:
                smoke_check = False
        elif self.wdm_option == "mutual_subscribe":
            if any("Publisher->kEvent_OnSubscriptionEstablished" in s for s in kevents) \
                    and "Closing endpoints" in client_output:
                smoke_check = True
            else:
                smoke_check = False
        elif self.wdm_option == "subless_notify":
            if any("kEvent_SubscriptionlessNotificationProcessingComplete" in s for s in kevents):
                smoke_check = True
            else:
                smoke_check = False

        result["smoke_check"] = smoke_check

        if self.quiet is False:
            print "weave-wdm-next %s from node %s (%s) to node %s (%s) : "\
                  % (self.wdm_option, client_info["client_node_id"], client_info["client_ip"], self.server_node_id, self.server_ip)
            if smoke_check is True:
                for kevent in kevents:
                    print hgreen(kevent)
                print hgreen("weave-wdm-next %s success" % self.wdm_option)
            else:
                for kevent in kevents:
                    print hred(kevent)
                print hred("weave-wdm-next %s failure" % self.wdm_option)

            if client_parser_error is True:
                print hred("client parser error")
            if client_leak_detected is True:
                print hred("client_resource leak detected")
            if server_parser_error is True:
                print hred("server parser error")
            if server_leak_detected is True:
                print hred("server resource leak detected")

        return result


    def wdm_next_checksum_check_process(self, log):
        """Pull out checksum value list for each trait object sent or received."""
        checksum_list= re.findall("Checksum is (\w+)", log)
        return checksum_list


    def wdm_next_client_event_sequence_process(self, log):
        """Pull out the number of events sent in a notification.  Only notifications with non-zero events are reported"""
        success = True
        client_event_list= re.findall("Fetched (\d+) events", log)
        client_event_process_list = [int(event) for event in client_event_list if int(event) > 0]
        return {"client_event_list":client_event_process_list, "length": len(client_event_process_list), "success": success}


    def wdm_next_publisher_event_sequence_process(self, log):
        """Pull out the number of events received in a notification. All notifications with EventList element are reported.  It would be considered an error to receive an empty EventList"""
        publisher_event_list = re.finditer("WEAVE:DMG: \tEventList =", log)
        success = True
        event_list_len = []
        for eventListMarker in publisher_event_list:
            startEventList = eventListMarker.end()
            endEventListMarker = re.search(".*WEAVE:DMG: \t],.*", log[startEventList:])
            if endEventListMarker:
                endEventList = endEventListMarker.end()
                event_list_len.append(len(re.findall("WEAVE:DMG: \t\t{", log[startEventList:(startEventList+endEventList)])))
            else:
                success = False
        return {"publisher_event_list":event_list_len, "length": len(event_list_len), "success": success}

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveWdmNext: %s" % (self.plaid_server_node_id, emsg))

    def __start_server_side(self):
        if self.no_service:
            return
        cmd = self.getWeaveWdmNextPath()
        if not cmd:
            return
        self.server_id = self.server_weave_id
        cmd += self.wdm_server_option

        if self.wdm_option == "subless_notify":
            cmd += " --wdm-subless-notify-dest-node " + self.clients_info[0]["client_weave_id"]
            cmd += " --wdm-subnet 6"

        if self.enable_server_stop is True:
            cmd += " --enable-stop "

        if self.save_server_perf is True:
            cmd += " --save-perf"

        if self.wdm_subless_notify_dest_node is not None:
            cmd += " --wdm-subless-notify-dest-node " + str(self.wdm_subless_notify_dest_node)

        if self.wdm_server_liveness_check_period is not None:
            cmd += " --wdm-liveness-check-period " + str(self.wdm_server_liveness_check_period)

        if self.test_server_case is not None:
            cmd += " --test-case " + str(self.test_server_case)

        if self.total_server_count is not None:
            cmd += " --total-count " + str(self.total_server_count)

        if self.final_server_status is not None:
            cmd += " --final-status " + str(self.final_server_status)

        if self.timer_server_period is not None:
            cmd += " --timer-period " + str(self.timer_server_period)

        if self.test_server_iterations is not None:
            cmd += " --test-iterations " + str(self.test_server_iterations)

        if self.test_server_delay is not None:
            cmd += " --test-delay " + str(self.test_server_delay)

        if self.enable_server_flip is not None:
            cmd += " --enable-flip " + str(self.enable_server_flip)

        if self.server_faults != None:
            cmd += " --faults " + self.server_faults

        if self.__dict__.get("server_clear_state_between_iterations", False):
            cmd += " --clear-state-between-iterations"

        if self.server_event_generator is not None:
            cmd += " --event-generator " + str(self.server_event_generator)

        if self.server_inter_event_period is not None:
            cmd += " --inter-event-period " + str(self.server_inter_event_period)

        if self.tap:
            cmd += " --tap-device " + self.tap

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_simple_weave_server(cmd, self.server_ip, self.server_node_id, self.server_process_tag, listen=False, strace=self.strace, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __start_client_side(self, client_info):
        cmd = self.getWeaveWdmNextPath()
        if not cmd:
            return
        cmd += self.wdm_client_option

        if self.wdm_option != "subless_notify":
            cmd += " --wdm-publisher " + self.server_weave_id

        if self.enable_client_stop is True:
            cmd += " --enable-stop "

        if self.save_client_perf is True:
            cmd += " --save-perf"

        if self.wdm_client_liveness_check_period is not None:
            cmd += " --wdm-liveness-check-period " + str(self.wdm_client_liveness_check_period)

        if self.enable_client_dictionary_test is not None:
            cmd += " --enable-dictionary-test "

        if self.test_client_case is not None:
            cmd += " --test-case " + str(self.test_client_case)

        if self.total_client_count is not None:
            cmd += " --total-count " + str(self.total_client_count)

        if self.final_client_status is not None:
            cmd += " --final-status " + str(self.final_client_status)

        if self.timer_client_period is not None:
            cmd += " --timer-period " + str(self.timer_client_period)

        if self.test_client_iterations is not None:
            cmd += " --test-iterations " + str(self.test_client_iterations)

        if self.test_client_delay is not None:
            cmd += " --test-delay " + str(self.test_client_delay)

        if self.enable_client_flip is not None:
            cmd += " --enable-flip " + str(self.enable_client_flip)

        if self.__dict__.get("client_clear_state_between_iterations", False):
            cmd += " --clear-state-between-iterations"

        if self.client_faults != None:
            cmd += " --faults " + self.client_faults

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.server == "service":
            node_id = client_info["client_weave_id"]
            fabric_id = self.getFabricId()
            client_info["client_ip"] = None
            cmd += " --wdm-subnet 5 --node-id=%s --fabric-id %s " % (node_id, fabric_id)

        if self.client_event_generator is not None:
            cmd += " --event-generator " + str(self.client_event_generator)

        if self.client_inter_event_period is not None:
            cmd += " --inter-event-period " + str(self.client_inter_event_period)

        if self.case:
            self.cert_file = self.case_cert_path if self.case_cert_path else os.path.join(self.main_conf["log_directory"], node_id.upper() + "-cert.weave-b64")
            self.key_file = self.case_key_path if self.case_key_path else os.path.join(self.main_conf["log_directory"], node_id.upper() + "-key.weave-b64")
            cmd += " --case " + " --node-cert " + self.cert_file + " --node-key " + self.key_file

        if self.group_enc:
            cmd += ' --group-enc ' + ' --group-enc-key-id ' + self.group_enc_key_id

        if self.enable_retry:
            cmd += " --enable-retry"

        if self.enable_mock_event_timestamp_initial_counter:
            cmd += " --enable-mock-event-timestamp-initial-counter"

        if self.client_update_mutation:
            cmd += " --wdm-update-mutation " + self.client_update_mutation

        if self.client_update_conditionality:
            cmd += " --wdm-update-conditionality " + self.client_update_conditionality

        if self.client_update_timing:
            cmd += " --wdm-update-timing " + self.client_update_timing

        if self.client_update_num_mutations:
            cmd += " --wdm-update-number-of-mutations " + str(self.client_update_num_mutations)

        if self.client_update_num_repeated_mutations:
            cmd += " --wdm-update-number-of-repeated-mutations " + str(self.client_update_num_repeated_mutations)

        if self.client_update_num_traits:
            cmd += " --wdm-update-number-of-traits " + str(self.client_update_num_traits)

        if self.client_update_discard_on_error:
            cmd += " --wdm-update-discard-on-error"

        custom_env = {}

        if False:
            try:
                print "cmd = \n" + cmd
                print "sleeping..."
                time.sleep(60*60)
            except:
                print "sleep interrupted"

        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(client_info["client_node_id"])

        self.start_simple_weave_client(cmd, client_info["client_ip"], None, None, client_info["client_node_id"],
                                       client_info["client_process_tag"], strace=self.strace, env=custom_env, use_persistent_storage=self.use_persistent_storage)


    def __wait_for_client(self, client_info):
        self.wait_for_test_to_end(client_info["client_node_id"], client_info["client_process_tag"], quiet=False, timeout=self.timeout)


    def __stop_server_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def __stop_client_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.client_node_id, self.client_process_tag)


    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def run_client_test(self):
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
                self.get_test_output(client_info["client_node_id"], client_info["client_process_tag"], True)
            if self.strace:
                client_strace_value, client_strace_data = \
                        self.get_test_strace(client_info["client_node_id"], client_info["client_process_tag"], True)

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

            success_dic = self.__process_results(client_output_data, server_output_data, client_info)

            smoke_check_ghost = success_dic["smoke_check"]
            if self.client_faults or self.server_faults:
                success_dic["smoke_check"] = True
            success = reduce(lambda x, y: x and y, success_dic.values())
            success_dic["smoke_check"] = smoke_check_ghost

            data = {}
            data.update(client_info)
            data["client_output"] = client_output_data
            data["server_output"] = server_output_data
            if self.strace:
                data["client_strace"] = client_strace_data
                data["server_strace"] = server_strace_data
            data["success_dic"] = success_dic
            all_data.append(data)

        return ReturnMsg(success, all_data)

    def run(self):
        return self.run_client_test()

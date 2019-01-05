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
#       Implements WeaveTime class that tests Weave Time sync among Weave Nodes.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork
from WeaveTest import WeaveTest
import WeaveUtilities as WeaveUtilities


options = {
    "client": None,
    "coordinator": None,
    "server": None,
    "mode": "auto",
    "quiet": False,
    "tap": None,
    "skip_service_end": False,
    "skip_coordinator_end": False,
    "client_faults": False,
    "server_faults": False,
    "coordinator_faults": False,
    "iterations": None,
    "test_tag": "",
    "strace": False,
    "plaid_server_env": {},
    "plaid_coordinator_env": {},
    "plaid_client_env": {}
}

gsync_succeeded_str = "Sync Succeeded: Reliable: Y"

def option():
    return options.copy()


class WeaveTime(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-time [-h --help] [-q --quiet] [-c --client <NAME>] [-o --coordinator <NAME>]
               [-s --server <NAME>] [-m --mode <MODE>] [-p --tap <TAP_INTERFACE>]
               [--client_faults <fault-injection configuration>]
               [--server_faults <fault-injection configuration>]
               [--coordinator_faults <fault-injection configuration>]

    commands to test Time profile:
        $ weave-time -c node01 -o node02 -s node03 -m auto
            Time sync test among client(node01), coordinator(node02) and server(node03) with Auto mode (multicast)

        $ weave-time -c node01 -o node02 -s node03 -m local
            Time sync test among client(node01), coordinator(node02) and server(node03) with Local mode (udp)

        $ weave-time -c node01 -o node02 -s node03 -m service
            Time sync test among client(node01), coordinator(node02) and server(node03) with Service mode (tcp)

    Note:
        There are four time sync modes:
         - auto: time sync via Multicast (default)
         - local: time sync with local nodes via UDP
         - service: time sync with Service via TCP
         - service-over-tunnel: time sync with service via WRM over a weave tunnel

    return:
        0   success
        1   failure

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.__dict__.update(options)
        self.__dict__.update(opts)

        self.client_process_tag = "WEAVE-TIME-CLIENT-" + self.mode + self.test_tag
        self.coordinator_process_tag = "WEAVE-TIME-COORDINATOR-" + self.mode + self.test_tag
        self.server_process_tag = "WEAVE-TIME-SERVER-" + self.mode + self.test_tag

        self.client_node_id = None
        self.coordinator_node_id = None
        self.server_node_id = None


    def __pre_check(self):
        # Check if Weave Time client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave Time client node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Time coordinator node is given.
        if not self.skip_coordinator_end and self.coordinator == None:
            emsg = "Missing name or address of the Weave Time coordinator node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Time server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Time server node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)


        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Time client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave Time coordinator node exists.
        if self._nodeExists(self.coordinator):
            self.coordinator_node_id = self.coordinator

        # Check if Weave Time server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if not self.skip_coordinator_end and self.coordinator_node_id == None:
            emsg = "Unknown identity of the coordinator node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None and self.server != "service":
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)

        if not self.skip_coordinator_end:
            self.coordinator_ip = self.getNodeWeaveIPAddress(self.coordinator_node_id)
            self.coordinator_weave_id = self.getWeaveNodeID(self.coordinator_node_id)

        if self.server == "service":
            self.skip_service_end = True
            self.server_weave_id = self.getServiceWeaveID("Time")
            self.server_ip = self.getServiceWeaveIPAddress("Time")
        else:
            self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
            self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if not self.skip_coordinator_end and self.coordinator_ip == None:
            emsg = "Could not find IP address of the coordinator node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if not self.skip_coordinator_end and self.coordinator_weave_id == None:
            emsg = "Could not find Weave node ID of the coordinator node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveTime: %s" % (emsg))
            sys.exit(1)

        if self.mode in ["service"]:
            # in this case, the client does not talk to the coordinator node at all
            self.skip_coordinator_end = True


    def __process_results(self, logs):
        pass_test = False
        found_leaks = []
        parser_errors = []

        for line in logs["client_output"].split("\n"):
            if gsync_succeeded_str in line:
                pass_test = True
                break

        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(logs["client_output"])
        if client_parser_error:
            parser_errors.append("client")
        if client_leak_detected:
            found_leaks.append("client")

        if not self.skip_service_end:
            server_parser_error, server_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(logs["server_output"])
            if server_parser_error:
                parser_errors.append("server")
            if server_leak_detected:
                found_leaks.append("server")

        if not self.skip_coordinator_end:
            coordinator_parser_error, coordinator_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(logs["coordinator_output"])
            if coordinator_parser_error:
                parser_errors.append("coordinator")
            if coordinator_leak_detected:
                found_leaks.append("coordinator")

            print "weave-time test among server %s (%s), client %s (%s), coordinator %s (%s) with %s mode: " % \
                        (self.server_node_id, self.server_ip,
                         self.client_node_id, self.client_ip,
                         self.coordinator_node_id, self.coordinator_ip, self.mode)
        else:
            print "weave-time test among server %s (%s), client %s (%s) with %s mode: " % \
                        (self.server_node_id, self.server_ip,
                         self.client_node_id, self.client_ip, self.mode)

        if self.quiet == False:
            if parser_errors:
                print hred("Parser errors on " + str(parser_errors))
            if found_leaks:
                print hred("Found leaks on " + str(found_leaks))
            if pass_test:
                print hgreen("Time sync succeeded")
            else:
                print hred("Time sync failed")

        return pass_test and not parser_errors and not found_leaks


    def __start_client_side(self, block_until_sync_succeeds=False):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters"
        cmd += " --node-addr " + self.client_ip
        cmd += " --time-sync-client"

        if self.mode == "local":
            cmd += " --time-sync-mode-local" 

        elif self.mode == "service":                 ## TCP
            cmd += " --time-sync-mode-service"

        elif self.mode == "service-over-tunnel":     ## WRM over tunnel
            cmd += " --time-sync-mode-service-over-tunnel"
            cmd += " --ts-server-node-id " + self.server_weave_id
            cmd += " --ts-server-node-addr " + self.server_ip

        else: 
            cmd += " --time-sync-mode-auto" 

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.client_faults:
            cmd += " --faults " + self.client_faults

        if self.iterations:
            cmd += " --iterations " + str(self.iterations)

        if block_until_sync_succeeds:
            wait_str = gsync_succeeded_str
        else:
            # block until client weave node is ready to service weave events
            wait_str = self.ready_to_service_events_str

        self.start_weave_process(self.client_node_id, cmd, self.client_process_tag, sync_on_output=wait_str, strace=self.strace, env=self.plaid_client_env)


    def __stop_client_side(self):
        self.stop_weave_process(self.client_node_id, self.client_process_tag)

  
    def __start_coordinator_side(self):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters"
        cmd += " --node-addr " + self.coordinator_ip
        cmd += " --time-sync-coordinator"

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.coordinator_faults:
            cmd += " --faults " + self.coordinator_faults

        # block until client weave node is ready to service weave events
        wait_str = self.ready_to_service_events_str

        self.start_weave_process(self.coordinator_node_id, cmd, self.coordinator_process_tag, sync_on_output=wait_str, strace=self.strace, env=self.plaid_coordinator_env)

    def __stop_coordinator_side(self):
        self.stop_weave_process(self.coordinator_node_id, self.coordinator_process_tag)


    def __start_server_side(self):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --debug-resource-usage --print-fault-counters"
        cmd += " --node-addr " + self.server_ip
        cmd += " --time-sync-server"

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.server_faults:
            cmd += " --faults " + self.server_faults

        self.start_weave_process(self.server_node_id, cmd, self.server_process_tag, sync_on_output=self.ready_to_service_events_str, strace=self.strace, env=self.plaid_server_env)

    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveTime: Run.")

        self.__pre_check()

        # Note: in all the cases below, we don't need any synchronous and
        # deterministic wait time to allow for a time sync operation to complete
        # because the __start_client_code will block until it performs a
        # successful time sync operation (if block_until_sync_succeeds=True)

        # Note: in all cases below that includes a server simulated by a happy
        # weave node, a server restart is implicity performed as part of
        # fault-injection. This will prompt the server node to send a time
        # change notification and test if clients handle it successfully. There
        # is, hence, no need to explicitly run a server-restart test
        # (regardless of the sync mode).

        # Note: in cases where __start_client_side is invoked with
        # block_until_sync_succeeds=True, the order in which the nodes are
        # created is important. Be sure to start the server and coordinator
        # nodes before the client node is initialized, otherwise, the test will
        # fail because the client node won't get any responses from the server
        # and/or coordinator nodes (because they would not have been created).

        if self.mode == "service-over-tunnel":
            # this test presumes that the service node (mock or real), the 
            # border router and the tunnel between them have already been
            # created and initialized successfully.
            self.__start_client_side(block_until_sync_succeeds=True)
        else:
            # mode in ["auto", "local", "service"]
            self.__start_server_side()
            if not self.skip_coordinator_end:
                self.__start_coordinator_side()
            self.__start_client_side(block_until_sync_succeeds=True)

        self.__stop_client_side()

        if not self.skip_coordinator_end:
            self.__stop_coordinator_side()

        if not self.skip_service_end:
            self.__stop_server_side()

        client_output_value, client_output_log = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        client_strace_log = ""
        client_strace_value = 0
        if self.strace:
            client_strace_value, client_strace_log = \
                self.get_test_strace(self.client_node_id, self.client_process_tag, True)

        coordinator_strace_log = None
        coordinator_strace_value = 0
        if self.skip_coordinator_end:
            coordinator_output_log = None
            coordinator_output_value = 0
        else:
            coordinator_output_value, coordinator_output_log = \
                self.get_test_output(self.coordinator_node_id, self.coordinator_process_tag, True)
            if self.strace:
                coordinator_strace_value, coordinator_strace_log = \
                    self.get_test_strace(self.coordinator_node_id, self.coordinator_process_tag, True)

        server_strace_log = None 
        server_strace_value = 0
        if self.skip_service_end:
            server_output_log = None 
            server_output_value = 0
        else:
            server_output_value, server_output_log = \
                    self.get_test_output(self.server_node_id, self.server_process_tag, True)
            if self.strace:
                server_strace_value, server_strace_log = \
                        self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        data = {}
        data["client_output"] = client_output_log
        data["client_strace"] = client_strace_log
        data["coordinator_output"] = coordinator_output_log
        data["coordinator_strace"] = coordinator_strace_log
        data["server_output"] = server_output_log
        data["server_strace"] = server_strace_log

        passed = self.__process_results(data)

        self.logger.debug("[localhost] WeaveTime: Done.")

        return ReturnMsg(passed, data)


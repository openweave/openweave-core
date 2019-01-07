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
#       Implementes WeaveServiceDir class that test Weave Service Directory profile
#       between a node and a service.
#

import os
import sys
import time
import pprint

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode

import happy.HappyNodeRoute
import happy.HappyNetworkAddress

from WeaveTest import WeaveTest
import WeaveUtilities
import plugins.plaid.Plaid as Plaid


options = {}
options["quiet"] = False
options["client"] = None
options["service"] = None
options["tap"] = None
options["test_tag"] = ""
options["iterations"] = 1
options["client_faults"] = None
options["service_faults"] = None
options["plaid"] = False
options["strace"] = False


def option():
    return options.copy()


class WeaveServiceDir(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-service-dir test Weave Service Directory Profile between a client and a service

    weave-service-dir [-h --help] [-q --quiet] [-c --client <NODE_ID>]
                      [-s --service <NODE_ID>] [-p --tap <TAP_INTERFACE>]

    Example:
    $ weave-service-dir BorderRouter cloud
        Run Weave Service Directory Profile test between client BorderRouter and service cloud.


    return:
        0    success
        1    fail
    """

    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.__dict__.update(opts)

        self.client_process_tag = "WEAVE-SD-CLIENT" + self.test_tag
        self.service_process_tag = "WEAVE-SD-SERVICE" + self.test_tag

        plaid_opts = Plaid.default_options()
        plaid_opts['num_clients'] = 2
        plaid_opts['strace'] = self.strace
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = opts["plaid"]
        if opts["plaid"] == "auto":
            self.use_plaid = self.plaid.isPlaidConfigured()


    def __pre_check(self):
        # Check if client was given
        if not self.client:
            emsg = "client id not specified."
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        if not self._nodeExists(self.client):
            emsg = "client %s does not exist." % (self.client)
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        # Check if service was given
        if not self.service:
            emsg = "Service node not specified."
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        if not self._nodeExists(self.service):
            emsg = "Service %s does not exist." % (self.service)
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        self.fabric_id = self.getFabricId()

        # Check if there is no fabric
        if self.fabric_id == None:
            emsg = "There is not Weave fabric."
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        #  Service Directory Endpoint ID: 0x18b4300200000001
        self.service_weave_id = self.getServiceWeaveID("Directory")
        self.service_ip = self.getNodePublicIPv4Address(self.service)
        self.client_ip = self.getNodeWeaveIPAddress(self.client)

        # Check if all unknowns were found

        if self.service_ip == None:
            emsg = "Could not find IP address of the service."
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()

        if self.client_ip == None:
            emsg = "Could not find IP address of the client."
            self.logger.error("[localhost] WeaveServiceDir: %s" % (emsg))
            self.exit()


    def __process_results(self, client_output, service_output):
        result = {}
        result["connection_completed"] = False
        
        client_parser_error, client_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(client_output)
        result["no_client_parser_error"] = not client_parser_error
        result["no_client_leak_detected"] = not client_leak_detected

        service_parser_error, service_leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(service_output)
        result["no_service_parser_error"] = not service_parser_error
        result["no_service_leak_detected"] = not service_leak_detected

        for line in client_output.split("\n"):
            if "AppConnectionComplete" in line:
                result["connection_completed"] = True
                break

        final_result = reduce((lambda x, y: x and y), result.values())

        print "weave-service-dir test from client %s to service %s: " % (self.client, self.service)

        if self.quiet == False:
            if final_result: 
                print hgreen("succeeded")
            else:
                print hred("failed")

        if not final_result:
            pprint.pprint(result)

        return result


    def __assing_directory_endpoint_ula_at_service(self):
        addr = self.getServiceWeaveIPAddress("Directory", self.service)

        options = happy.HappyNodeAddress.option()
        options["quiet"] = self.quiet
        options["node_id"] = self.service
        options["interface"] = "eth0"
        options["address"] = addr
        options["add"] = True

        addAddr = happy.HappyNodeAddress.HappyNodeAddress(options)
        addAddr.run()


    def __start_client_side(self):
        cmd = self.getWeaveServiceDirPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.client_ip
        cmd += " --service-dir-url " + self.service_ip
        cmd += " --debug-resource-usage"
        cmd += " --print-fault-counters"

        if self.client_faults:
            cmd += " --faults " + self.client_faults

        cmd += " --iterations " + str(self.iterations)

        if self.tap:
            cmd += " --tap-device " + self.tap

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.client)

        self.start_weave_process(self.client, cmd, self.client_process_tag, strace=self.strace, env=custom_env)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client, self.client_process_tag)


    def __stop_client_side(self):
        self.stop_weave_process(self.client, self.client_process_tag)

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("WeaveServiceDir: %s" % (emsg))

    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def __start_service_side(self):
        cmd = self.getWeaveServiceDirPath()
        if not cmd:
            return

        cmd += " --node-id " + str(self.service_weave_id)
        cmd += " --fabric-id " + str(self.fabric_id)

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.service_faults:
            cmd += " --faults " + self.service_faults

        cmd += " --debug-resource-usage"
        cmd += " --print-fault-counters"

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.service)

        self.start_weave_process(self.service, cmd, self.service_process_tag, strace=self.strace, env=custom_env, sync_on_output = self.ready_to_service_events_str)


    def __stop_service_side(self):
        self.stop_weave_process(self.service, self.service_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveServiceDir: Run.")

        self.__pre_check()

        if self.use_plaid:
            self.__start_plaid_server()

        self.__start_service_side()
        self.__assing_directory_endpoint_ula_at_service()

        self.__start_client_side()
        self.__wait_for_client()

        self.__stop_service_side()

        if self.use_plaid:
            self.__stop_plaid_server()

        data = {}

        client_output_value, client_output_data = \
        self.get_test_output(self.client, self.client_process_tag, True)
        if self.strace:
            client_strace_value, client_strace_data = \
            self.get_test_strace(self.client, self.client_process_tag, True)
            data["client_strace"] = client_strace_data

        service_output_value, service_output_data = \
        self.get_test_output(self.service, self.service_process_tag, True)
        if self.strace:
            service_strace_value, service_strace_data = \
            self.get_test_strace(self.service, self.service_process_tag, True)
            data["service_strace"] = service_strace_data

        result = self.__process_results(client_output_data, service_output_data)
        data["client_output"] = client_output_data
        data["service_output"] = service_output_data
        data["result_details"] = result

        final_result = reduce((lambda x, y: x and y), result.values())

        self.logger.debug("[localhost] WeaveServiceDir: Done.")
        return ReturnMsg(final_result, data)


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
#       Implements WeaveInet class that tests Weave Inet Layer among Weave Nodes.
#

import os
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest


# Q: what are the parameters need to specify?
options = {}
options["quiet"] = False
options["node_id"] = None
options["tap_if"] = None
options["node_ip"] = None
options["ipv4_gateway"] = None
options["dns"] = None
options["use_lwip"] = False


def option():
    return options.copy()


class WeaveInetDNS(HappyNode, HappyNetwork, WeaveTest):
    
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.node_id = opts["node_id"]
        self.tap_if = opts["tap_if"]
        self.prefix = opts["prefix"]
        self.ipv4_gateway =opts["ipv4_gateway"]
        self.dns = opts["dns"]
        self.use_lwip = opts["use_lwip"]

        self.node_process_tag = "WEAVE-INET-NODE"

    def __log_error_and_exit(self, error):
        self.logger.error("[localhost] WeaveInetDNS: %s" % (error))
        sys.exit(1)

    def __checkNodeExists(self, node, description):
        if not self._nodeExists(node):
            emsg = "The %s '%s' does not exist in the test topology." % (description, node)
            self.__log_error_and_exit(emsg)


    def __pre_check(self):
       # Check if the name of the new node is given
        if not self.node_id:
            emsg = "Missing name of the virtual node that should start shell."
            self.__log_error_and_exit(emsg)

        # Check if virtual node exists
        if not self._nodeExists():
            emsg = "virtual node %s does not exist." % (self.node_id)
            self.__log_error_and_exit(emsg)

        # check if prefix 
        if self.prefix == None:
            emsg = "prefix is None, Please specifiy a valid prefix."
            self.__log_error_and_exit(emsg)

    def __gather_results(self):
        """
        gather result from get_test_output()
        """
        quiet = True
        results = {}

        results['status'], results['output'] = self.get_test_output(self.node_id, self.node_process_tag, quiet)

        return (results)


    def __process_results(self, results):
        """
        process results from gather_results()
        """
        status = False
        output = ""

        status = (results['status'] == 0)

        output = results['output']

        return (status, output)

    def __start_node_dnscheck(self):
        """
        lwip and socket use different command for now
        """
        cmd = "sudo "
        cmd += self.getWeaveInetLayerDNSPath()
        node_ip = self.getNodeAddressesOnPrefix(self.prefix, self.node_id)[0]

        if node_ip == None:
            emsg = "Could not find IP address of the node, %s" % (self.node_id)
            self.__log_error_and_exit(emsg)

        if self.use_lwip:
            cmd += " --tap-device " + self.tap_if + " -a " + node_ip + " --ipv4-gateway " + self.ipv4_gateway + \
                    " --dns-server " + self.dns

        print "dns check command : {}".format(cmd)
        self.start_weave_process(self.node_id, cmd, self.node_process_tag, sync_on_output=self.ready_to_service_events_str)


    def __stop_node(self):

        self.stop_weave_process(self.node_id, self.node_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveInetDNS: Run.")

        self.__pre_check()

        self.__start_node_dnscheck()

        emsg = "WeaveInet %s should be running." % (self.node_process_tag)
        self.logger.debug("[%s] WeaveInet: %s" % (self.node_id, emsg))

        self.__stop_node()
        node_output_value, node_output_data = \
            self.get_test_output(self.node_id, self.node_process_tag, True)
        node_strace_value, node_strace_data = \
            self.get_test_strace(self.node_id, self.node_process_tag, True)

        results = self.__gather_results()

        result, output = self.__process_results(results)

        data = {}
        data["node_output"] = node_output_data
        data["node_strace"] = node_strace_data

        self.logger.debug("[localhost] WeaveInetDNSTest: Done.")

        return ReturnMsg(result, data)

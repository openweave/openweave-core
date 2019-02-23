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
        self.tap_if = options["tap_if"]
        self.node_ip = options["node_ip"]
        self.ipv4_gateway =options["ipv4_gateway"]
        self.dns = options["dns"]
        self.use_lwip = options["use_lwip"]

        self.node_process_tag = "WEAVE-INET-NODE"

    def __pre_check(self):
       # Check if the name of the new node is given
        if not self.node_id:
            emsg = "Missing name of the virtual node that should start shell."
            self.logger.error("[%s] HappyShell: %s" % (self.node_id, emsg))
            self.exit()

        # Check if virtual node exists
        if not self._nodeExists():
            emsg = "virtual node %s does not exist." % (self.node_id)
            self.logger.error("[%s] HappyShell: %s" % (self.node_id, emsg))
            self.exit()


    def __process_results(self, output):
        """
        process test output from DNS resolution test
        """
        
        print "test DNS resolution output=====>"
        print output

        for line in output.split("\n"):
            if "Fail" in line:
                result = False

        if self.quiet == False:
            print "weave-inet dns resolution test on node {}".format(self.node_id)
            if 'result' in locals() and result is False:
                print hred("failed")
            else:
                print hgreen("passed")
                result = True
        return (result, output)
    

    def __start_node_dnscheck(self):
        """
        lwip and socket use different command for now
        TODO: fix lwip and socket to use similar command future.
        """
        cmd = "sudo "
        cmd += self.getWeaveInetLayerDNSPath()
        if self.use_lwip:
            cmd += " --tap-device " + self.tap_if + " -a " + self.node_ip + " --ipv4-gateway " + self.ipv4_gateway + \
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

        result, output = self.__process_results(node_output_data)

        data = {}
        data["node_output"] = node_output_data
        data["node_strace"] = node_strace_data

        self.logger.debug("[localhost] WeaveInetDNSTest: Done.")

        return ReturnMsg(result, data)

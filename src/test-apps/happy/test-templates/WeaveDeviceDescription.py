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
#       Implements WeaveDeviceDescription class that tests Weave DEVICE-DESCRIPTION among two Weave Nodes.
#

import sys
from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork
from WeaveTest import WeaveTest


options = {}
options["quiet"] = False
options["server"] = None
options["client"] = None
options["tap"] = None


def option():
    return options.copy()


class WeaveDeviceDescription(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-device-description [-h --help] [-q --quiet] [-s --server <NAME>] [-c --client <NAME>] [-p --tap <TAP_INTERFACE>]
    

    command to test swu-v0:
        $ weave-device-description --server node01 --client node02

    return:
        0   success
        1   failure

    """
    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)

        self.quiet = opts["quiet"]
        self.client = opts["client"]
        self.server = opts["server"]
        self.tap = opts["tap"]

        self.server_process_tag = "WEAVE-DEVICE-DESCRIPTION-SERVER"
        self.client_process_tag = "WEAVE-DEVICE-DESCRIPTION-CLIENT"

        self.client_node_id = None
        self.server_node_id = None


    def __pre_check(self):
        # Check if Weave DEVICE-DESCRIPTION client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave DEVICE-DESCRIPTION client node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        # Check if Weave DEVICE-DESCRIPTION server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave DEVICE-DESCRIPTION server node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        # Check if Weave DEVICE-DESCRIPTION client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave DEVICE-DESCRIPTION server node exists.
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
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveDeviceDescription: %s" % (emsg))
            sys.exit(1)


    def __start_client_side(self):
        cmd = self.getWeaveDeviceDescriptionClientPath()
        if not cmd:
            return

        cmd += " --node-id " + self.client_weave_id + " --fabric-id " + self.getFabricId()
        cmd += " --dest-addr " + self.server_ip + " "+ self.getWeaveNodeID(self.server)

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.client_node_id, cmd, self.client_process_tag)


    def __start_server_side(self):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --node-addr " + self.server_ip + " --node-id " + self.server_weave_id

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.server_node_id, cmd, self.server_process_tag, sync_on_output = self.ready_to_service_events_str)


    def __process_results(self, client_output):
        pass_test = False

        for line in client_output.split("\n"):
            # if "subscribe confirm - success status + data list" in line:
            if "Device Description Operation Completed" in line:
                pass_test = True
                break

        if self.quiet == False:
            print "weave-device-description from server %s (%s) to client %s (%s) : " % \
                (self.server_node_id, self.server_ip,
                 self.client_node_id, self.client_ip)
            if pass_test:
                print hgreen("Device description operation is completed")
            else:
                print hred("Device description operation is not completed")

        return (pass_test, client_output)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def __stop_client_side(self):
        self.stop_weave_process(self.client_node_id, self.client_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveDeviceDescription: Run.")

        self.__pre_check()

        self.__start_server_side()
        time.sleep(0.1)
        emsg = "WeaveDeviceDescription %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveDeviceDescription: %s" % (self.server_node_id, emsg))

        self.__start_client_side()

        self.__wait_for_client()
        client_output_value, client_output_data = \
            self.get_test_output(self.client_node_id, self.client_process_tag, True)
        client_strace_value, client_strace_data = \
            self.get_test_strace(self.client_node_id, self.client_process_tag, True)

        self.__stop_server_side()
        server_output_value, server_output_data = \
            self.get_test_output(self.server_node_id, self.server_process_tag, True)
        server_strace_value, server_strace_data = \
            self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        status, results = self.__process_results(client_output_data)

        data = {}
        data["client_output"] = client_output_data
        data["client_strace"] = client_strace_data
        data["server_output"] = server_output_data
        data["server_strace"] = server_strace_data

        self.logger.debug("[localhost] WeaveDeviceDescription: Done.")
        return ReturnMsg(status, data)


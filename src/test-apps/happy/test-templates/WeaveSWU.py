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
#       Implements WeaveSWU class that tests Weave SWU among two Weave Nodes.
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
options["file_designator"] = None
options["integrity_type"] = None
options["update_scheme"] = None
options["announceable"] = False
options["tap"] = None


def option():
    return options.copy()


class WeaveSWU(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-swu [-h --help] [-q --quiet] [-s --server <NAME>] [-c --client <NAME>]
           [-f --file-designator <SERVER_TMP_PATH>] [-i --integrity-type <Number, Default is 0>] [-u --update-scheme <Number, Default is 3>]
           [-a --announceable]

    command to test swu-v0:
        $ weave-swu --server node01 --client node02 --file-designator /server_files/test_image_file --integrity-type 2 --update-scheme 3 --announceable False
        or
        $ weave-swu -s node01 -c node02 -f /server_files/test_image_file -i 2 -u 3 -a True

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
        self.file_designator = opts["file_designator"]
        self.integrity_type = opts["integrity_type"]
        self.update_scheme = opts["update_scheme"]
        self.announceable = opts["announceable"]
        self.tap = opts["tap"]

        self.vendor_id = '9050'
        self.sw_version = '2.0'
        self.server_process_tag = "WEAVE-SWU-SERVER"
        self.client_process_tag = "WEAVE-SWU-CLIENT"

        self.client_node_id = None
        self.server_node_id = None


    def __pre_check(self):
        # Check if Weave SWU client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave SWU client node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        # Check if Weave SWU server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave SWU server node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        # Check if Weave SWU client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave SWU server node exists.
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
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        # Check file_designator,update_scheme,integrity_type

        if self.integrity_type == None:
            emsg = "Missing integrity type that is used by SWU server."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.update_scheme == None:
            emsg = "Missing update scheme that is used by SWU server."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.file_designator == None:
            emsg = "Missing path to a temporaty directory that is used by SWU server."
            self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
            sys.exit(1)

        if self.integrity_type != None:
            self.integrity_type = str(self.integrity_type)
            if not self.integrity_type.isdigit():
                emsg = "File integrity type must be a number, not %s." % (self.integrity_type)
                self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
                if int(self.integrity_type) not in [0, 1, 2]:
                    emsg = "File integrity type must be 0, or 1, or 2, not %s." % (self.integrity_type)
                    self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
                sys.exit(1)

        if self.update_scheme != None:
            self.update_scheme = str(self.update_scheme)
            if not self.update_scheme.isdigit():
                emsg = "File update scheme must be a number, not %s." % (self.update_scheme)
                self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
                if int(self.update_scheme) not in [0, 1, 2, 3]:
                    emsg = "File update scheme must be 0, or 1, or 2, or 3 not %s." % (self.update_scheme)
                    self.logger.error("[localhost] WeaveSWU: %s" % (emsg))
                sys.exit(1)


    def __start_server_side(self):
        cmd = self.getWeaveSWUServerPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.server_ip
        cmd += " --file-designator " + self.file_designator + " --vendor-id " + self.vendor_id
        cmd += " --sw-version " + str(self.sw_version) + " --integrity-type " + self.integrity_type
        cmd += " --update-scheme " + self.update_scheme

        if self.announceable:
            cmd += " --dest-addr " + self.client_ip + " --dest-node-id " + self.client_weave_id
        else:
            cmd += " --listen"

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.server_node_id, cmd, self.server_process_tag, sync_on_output = self.ready_to_service_events_str)


    def __start_client_side(self):
        cmd = self.getWeaveSWUClientPath()
        if not cmd:
            return

        cmd += " --node-addr " + self.client_ip

        if self.announceable:
            cmd += " --listen"
        else:
            cmd += " --dest-addr " + self.server_ip + " " + self.server_weave_id

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.client_node_id, cmd, self.client_process_tag)


    def __process_results(self, client_output):
        pass_test = False

        for line in client_output.split("\n"):
            if "Completed the SWU interactive protocol test" in line:
                pass_test = True
                break

        if self.quiet == False:
            print "weave-swu from server %s (%s) to client %s (%s) : " % \
                (self.server_node_id, self.server_ip,
                 self.client_node_id, self.client_ip)
            if pass_test:
                print hgreen("SWU interaction is completed")
            else:
                print hred("SWU interaction is not completed")

        return (pass_test, client_output)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def run(self):
        self.logger.debug("[localhost] WeaveSWU: Run.")

        self.__pre_check()
        if self.announceable:
            self.__start_client_side()
            delayExecution(0.5)
            emsg = "WeaveSWU %s should be running." % (self.client_process_tag)
            self.logger.debug("[%s] WeaveSWU: %s" % (self.client_node_id, emsg))

            self.__start_server_side()
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
        else:
            self.__start_server_side()

            emsg = "WeaveSWU %s should be running." % (self.server_process_tag)
            self.logger.debug("[%s] WeaveSWU: %s" % (self.server_node_id, emsg))
            delayExecution(0.5)
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

        self.logger.debug("[localhost] WeaveSWU: Done.")
        return ReturnMsg(status, data)


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
#       Implements WeaveBDX class that tests Weave BDX among Weave Nodes.
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
options["quiet"] = False
options["server"] = None
options["client"] = None
options["tmp"] = None
options["receive"] = None
options["download"] = None
options["upload"] = None
options["offset"] = None
options["length"] = None
options["tap"] = None
options["server_version"] = 1
options["client_version"] = 1
options["client_faults"] = None
options["server_faults"] = None
options["strace"] = True
options["test_tag"] = ""
options["iterations"] = 1
options["plaid"] = False


def option():
    return options.copy()


class WeaveBDX(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-bdx [-h --help] [-q --quiet] [-s --server <NAME>] [-c --client <NAME>]
           [-t --tmp <SERVER_TMP_PATH>] [-r --receive <DESTINATION_PATH>]
           [-u --upload <SOURCE_PATH>] [-d --download <SOURCE_PATH>]
           [-o --offset <SIZE>] [-l --length <SIZE>] [-p --tap <TAP_INTERFACE>]
           [-S --server-version <bdx_version>] [-C --client-version <bdx_version>]


    commands to test bdx-v0:
        $ weave-bdx -s node01 -c node02 -S 0 -C 0 --upload /client_files/test_file.txt --tmp /server/tmp --receive /server_files
            Sends /client_files/test_file.txt from node02 to folder /server_files at node01.

        $ weave-bdx -s node01 -c node02 -S 0 -C 0 --download /server_files/test_file.txt --tmp /server/tmp --receive /client_files
            Receives /server_files/test_file.txt from node01 at /client_files of node02.

    commands to test bdx-v1:
        $ weave-bdx -s node01 -c node02 -S 1 -C 1 --upload /client_files/test_file.txt --tmp /server/tmp --receive /server_files
            Sends /client_files/test_file.txt from node02 to folder /server_files at node01.

        $ weave-bdx -s node01 -c node02 -S 1 -C 1 --download /server_files/test_file.txt --tmp /server/tmp --receive /client_files
            Receives /server_files/test_file.txt from node01 at /client_files of node02.

    commands to test bdx-v0 and bdx-v1 interop:
        $ weave-bdx -s node01 -c node02 -S 0 -C 1 --upload /client_files/test_file.txt --tmp /server/tmp --receive /server_files
            Sends /client_files/test_file.txt from node02 to folder /server_files at node01.

        $ weave-bdx -s node01 -c node02 -S 0 -C 1 --download /server_files/test_file.txt --tmp /server/tmp --receive /client_files
            Receives /server_files/test_file.txt from node01 at /client_files of node02.

        $ weave-bdx -s node01 -c node02 -S 1 -C 0 --upload /client_files/test_file.txt --tmp /server/tmp --receive /server_files
            Sends /client_files/test_file.txt from node02 to folder /server_files at node01.

        $ weave-bdx -s node01 -c node02 -S 1 -C 0 --download /server_files/test_file.txt --tmp /server/tmp --receive /client_files
            Receives /server_files/test_file.txt from node01 at /client_files of node02.


    Note:
        special the BDX server and client version with '-S/-C' option, and BDX-v1 would be the default version

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
        self.tmp = opts["tmp"]
        self.download = opts["download"]
        self.upload = opts["upload"]
        self.receive = opts["receive"]
        self.offset = opts["offset"]
        self.length = opts["length"]
        self.tap = opts["tap"]
        self.server_version = opts["server_version"]
        self.client_version = opts["client_version"]
        self.server_faults = opts["server_faults"]
        self.client_faults = opts["client_faults"]
        self.iterations = opts["iterations"]

        self.server_process_tag = "WEAVE-BDX-SERVER" + opts["test_tag"]
        self.client_process_tag = "WEAVE-BDX-CLIENT" + opts["test_tag"]

        self.client_node_id = None
        self.server_node_id = None

        self.strace = opts["strace"]

        plaid_opts = Plaid.default_options()
        plaid_opts['quiet'] = self.quiet
        self.plaid_server_node_id = 'node03'
        plaid_opts['server_node_id'] = self.plaid_server_node_id
        plaid_opts['num_clients'] = 2
        plaid_opts['server_ip_address'] = self.getNodeWeaveIPAddress(self.plaid_server_node_id)
        plaid_opts['strace'] = self.strace
        self.plaid = Plaid.Plaid(plaid_opts)

        self.use_plaid = opts["plaid"]
        if opts["plaid"] == "auto":
            if self.server == "service":
                # can't use plaid when talking to an external service
                self.use_plaid = False
            else:
                self.use_plaid = self.plaid.isPlaidConfigured()


    def __pre_check(self):
        # Check if Weave BDX client node is given.
        if self.client == None:
            emsg = "Missing name or address of the Weave BDX client node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Check if Weave BDX server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave BDX server node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Check if Weave BDX server version is correct.
        if self.server_version != 0 and self.server_version != 1:
            emsg = "BDX server version is not correct."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Check if Weave BDX client version is correct.
        if self.client_version != 0 and self.client_version != 1:
            emsg = "BDX client version is not correct."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Check if Weave BDX client node exists.
        if self._nodeExists(self.client):
            self.client_node_id = self.client

        # Check if Weave BDX server node exists.
        if self._nodeExists(self.server):
            self.server_node_id = self.server

        # Check if client is provided in a form of IP address
        if IP.isIpAddress(self.client):
            self.client_node_id = self.getNodeIdFromAddress(self.client)

        # Check if server is provided in a form of IP address
        if IP.isIpAddress(self.server):
            self.server_node_id = self.getNodeIdFromAddress(self.server)
        elif self.server == "service":
            if self.download is not None:
                self.server_ip = self.getServiceWeaveIPAddress("FileDownload")
            elif self.upload is not None:
                self.server_ip = self.getServiceWeaveIPAddress("LogUpload")
            self.server_weave_id = self.IPv6toWeaveId(self.server_ip)
            self.server_node_id = "service"

        if self.client_node_id == None:
            emsg = "Unknown identity of the client node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if self.server_node_id == None:
            emsg = "Unknown identity of the server node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        self.client_ip = self.getNodeWeaveIPAddress(self.client_node_id)
        self.client_weave_id = self.getWeaveNodeID(self.client_node_id)
        if self.server is not "service":
            self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
            self.server_weave_id = self.getWeaveNodeID(self.server_node_id)

        # Check if all unknowns were found

        if self.client_ip == None:
            emsg = "Could not find IP address of the client node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if self.server_ip == None:
            emsg = "Could not find IP address of the server node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if self.client_weave_id == None:
            emsg = "Could not find Weave node ID of the client node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        # Check tmp, upload, download, receive paths
        if self.server is not "service":
            if self.tmp == None:
                emsg = "Missing path to a temporaty directory that is used by BDX server."
                self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
                sys.exit(1)

            if not os.path.isdir(self.tmp):
                emsg = "The server BDX temp %s is not a directory." % (self.tmp)
                self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
                sys.exit(1)

        if self.receive == None:
            emsg = "Missing path to receiving directory."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if not os.path.isdir(self.receive):
            emsg = "The receiving path %s is not a directory." % (self.receive)
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if (self.download == None and self.upload == None) or \
            (self.download != None and self.upload != None):
            emsg = "Either download or upload path must be specified."
            self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
            sys.exit(1)

        if self.offset != None:
            self.offset = str(self.offset)
            if not self.offset.isdigit():
                emsg = "File offset must be a number, not %s." % (self.offset)
                self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
                sys.exit(1)

        if self.length != None:
            self.length = str(self.length)
            if not self.length.isdigit():
                emsg = "File length must be a number, not %s." % (self.length)
                self.logger.error("[localhost] WeaveBDX: %s" % (emsg))
                sys.exit(1)

    def __start_plaid_server(self):

        self.plaid.startPlaidServerProcess()

        emsg = "plaid-server should be running."
        self.logger.debug("[%s] WeaveWdmNext: %s" % (self.plaid_server_node_id, emsg))


    def __start_server_side(self):
        if self.server_version == 0:
            cmd = self.getWeaveBDXv0ServerPath()
            if not cmd:
                return;

            cmd += " -a " + self.server_ip

            if self.download != None:
                cmd += " -r " + self.download
            else:
                cmd += " -R " + self.receive

        else:
            cmd = self.getWeaveBDXServerPath()
            if not cmd:
                return

            cmd += " -a " + self.server_ip \
                + " -T " + self.tmp

            if self.download != None:
                # Client downloads file specified in self.download
                cmd += " -R " + os.path.dirname(self.download)
            else:
                cmd += " -R " + self.receive

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.server_faults != None:
            cmd += " --faults " + self.server_faults

        if self.server_faults or self.client_faults:
            cmd += " --extra-cleanup-time 10000"

        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.server_node_id)

        self.start_weave_process(self.server_node_id, cmd, self.server_process_tag, strace=self.strace, env=custom_env, sync_on_output="Weave Node ready to service events")


    def __start_client_side(self):
        if self.client_version == 0:
            cmd = self.getWeaveBDXv0ClientPath()
            if not cmd:
                return;

            cmd += " " + self.server_weave_id + "@" + self.server_ip \
                + " -a " + self.client_ip + " -R " + self.receive + " -T" \
                + " --iterations " + str(self.iterations)

            if self.download != None:
                if self.server_version == 1:
                    cmd += " -r" + " file://" + self.download
                else:
                    cmd += " -r " + self.download
            else:
                cmd += " -r " + self.upload + " -p"

        else:
            cmd = self.getWeaveBDXClientPath()
            if not cmd:
                return
            if self.server is not "service":
                cmd += " " + self.server_weave_id + "@" + self.server_ip \
                    + " -a " + self.client_ip + " -T"
            else:
                cmd += " " + self.server_weave_id + "@" + self.server_ip \
                    + " -a " + self.client_ip

            cmd += " --iterations " + str(self.iterations)

            if self.download != None:
                if self.server_version == 1:
                    if self.server is not "service":
                        cmd += " -r" + " file://" + self.download + " -R " + self.receive + " -u "
                    else:
                        cmd +=" -r" + " weave-bdx://" + self.download + " -R " + self.receive + " -u "
                else:
                    cmd += " -r " + self.download + " -R " + self.receive
            else:
                if self.server is not "service":
                    cmd += " -r " + self.upload + " -p"
                else:
                    cmd += " -r " + self.upload + " -p -u"

        if self.offset != None:
                cmd += " -s " + self.offset

        if self.length != None:
                cmd += " -l " + self.length

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.client_faults != None:
            cmd += " --faults " + self.client_faults

        if self.server_faults or self.client_faults:
            cmd += " --extra-cleanup-time 10000"


        custom_env = {}
        if self.use_plaid:
            custom_env = self.plaid.getPlaidClientLibEnv(self.client_node_id)

        self.start_weave_process(self.client_node_id, cmd, self.client_process_tag, strace=self.strace, env=custom_env)


    def __process_results(self, client_output, server_output):
        parser_error = False
        leak_detected = False
        transfer_done = False

        parser_error, leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(client_output)

        for line in client_output.split("\n"):
            if "WEAVE:BDX: Transfer complete!" in line:
                transfer_done = True
                break

        if self.quiet == False:
            if self.download:
                print "weave-bdx from server v%s %s (%s) to client v%s %s (%s) : " % \
                    (self.server_version, self.server_node_id, self.server_ip,
                     self.client_version, self.client_node_id, self.client_ip)
                if transfer_done:
                    print hgreen("downloaded")
                else:
                    print hred("not downloaded")
            else:
                print "weave-bdx from client %s (%s) to server %s (%s) : " % \
                    (self.client_node_id, self.client_ip,
                    self.server_node_id, self.server_ip)
                if transfer_done:
                    print hgreen("uploaded")
                else:
                    print hred("not uploaded")
            if leak_detected:
                print hred("resource leak detected in the client's ouput")
            if parser_error:
                print hred("parser error in the client's output")


        pass_test = transfer_done and not leak_detected and not parser_error

        if self.server is not "service":
            # check the server for leaks and parser errors
            parser_error, leak_detected = WeaveUtilities.scan_for_leaks_and_parser_errors(server_output)
            if self.quiet == False:
                if leak_detected:
                    print hred("resource leak detected in the server's ouput")
                if parser_error:
                    print hred("parser error in the server's output")

        pass_test = pass_test and not parser_error and not leak_detected

        return (pass_test, client_output)


    def __wait_for_client(self):
        self.wait_for_test_to_end(self.client_node_id, self.client_process_tag)


    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)


    def __stop_plaid_server(self):
        self.plaid.stopPlaidServerProcess()


    def run(self):
        self.logger.debug("[localhost] WeaveBDX: Run.")

        self.__pre_check()

        if self.server is "service":
            self.__start_client_side()
            self.__wait_for_client()

            client_output_value, client_output_data = \
                self.get_test_output(self.client_node_id, self.client_process_tag, True)
            if self.strace:
                client_strace_value, client_strace_data = \
                    self.get_test_strace(self.client_node_id, self.client_process_tag, True)

            status, results = self.__process_results(client_output_data, '')
            # status = 0
            data = {}
            data["client_output"] = client_output_data
        else:
            if self.use_plaid:
                self.__start_plaid_server()

            self.__start_server_side()

            emsg = "WeaveBDX %s should be running." % (self.server_process_tag)
            self.logger.debug("[%s] WeaveBDX: %s" % (self.server_node_id, emsg))

            self.__start_client_side()
            self.__wait_for_client()

            client_output_value, client_output_data = \
                self.get_test_output(self.client_node_id, self.client_process_tag, True)
            if self.strace:
                client_strace_value, client_strace_data = \
                    self.get_test_strace(self.client_node_id, self.client_process_tag, True)

            self.__stop_server_side()

            if self.use_plaid:
                self.__stop_plaid_server()

            server_output_value, server_output_data = \
                self.get_test_output(self.server_node_id, self.server_process_tag, True)
            if self.strace:
                server_strace_value, server_strace_data = \
                    self.get_test_strace(self.server_node_id, self.server_process_tag, True)

            status, results = self.__process_results(client_output_data, server_output_data)

            data = {}
            data["client_output"] = client_output_data
            data["server_output"] = server_output_data
            if self.strace:
                data["client_strace"] = client_strace_data
                data["server_strace"] = server_strace_data

        self.logger.debug("[localhost] WeaveBDX: Done.")
        return ReturnMsg(status, data)


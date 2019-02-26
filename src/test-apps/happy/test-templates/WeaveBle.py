#!/usr/bin/env python


#
#    Copyright (c) 2015-2018 Nest Labs, Inc.
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
#       Implements WeaveBle class that tests Weave Echo among Weave Nodes.
#

import os
import psutil
import re
import subprocess
import sys
from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork
from happy.HappyProcess import HappyProcess
import happy.HappyProcessStart
from WeaveTest import WeaveTest



options = {"quiet": False,
           "no_service": False,
           "tap": None,
           "timeout": None,
           "client_process_tag": "WEAVE-DEVICE-MGR",
           "server_process_tag": "WEAVE-BLUEZ-PERIPHERAL",
           "clients_info": [],
           "server_node_id": None,
           "strace": True,
           "use_persistent_storage": True
           }


def option():
    return options.copy()


class WeaveBle(WeaveTest, HappyNode, HappyNetwork, HappyProcess):
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
            self.logger.error("[localhost] WeaveBle: %s" % (emsg))
            sys.exit(1)

        for client in self.clients:
            client_node_id = None

            # Check if Weave Ble client node is given.
            if client == None:
                emsg = "Missing name or address of the Weave Ble client node."
                self.logger.error("[localhost] WeaveBle: %s" % (emsg))
                sys.exit(1)

            # Check if Weave Ble client node exists.
            if self._nodeExists(client):
                client_node_id = client

            # Check if client is provided in a form of IP address
            if IP.isIpAddress(client):
                client_node_id = self.getNodeIdFromAddress(client)

            if client_node_id == None:
                emsg = "Unknown identity of the client node."
                self.logger.error("[localhost] WeaveBle: %s" % (emsg))
                sys.exit(1)

            if self.getNodeType(client_node_id) == "service":
                client_ip = self.getServiceWeaveIPAddress(self.endpoint, client_node_id)
                client_weave_id = self.getServiceWeaveID(self.endpoint, client_node_id)
            else:
                client_ip = self.getNodeWeaveIPAddress(client_node_id)
                client_weave_id = self.getWeaveNodeID(client_node_id)

            if client_ip == None:
                emsg = "Could not find IP address of the client node."
                self.logger.error("[localhost] WeaveBle: %s" % (emsg))
                sys.exit(1)

            if client_weave_id == None:
                emsg = "Could not find Weave node ID of the client node."
                self.logger.error("[localhost] WeaveBle: %s" % (emsg))
                sys.exit(1)

            self.clients_info.append({'client': client, 'client_node_id': client_node_id, 'client_ip': client_ip,
                          'client_weave_id': client_weave_id, 'client_process_tag': client + "_" + self.client_process_tag + client})


        # Check if Weave Ble server node is given.
        if self.server == None:
            emsg = "Missing name or address of the Weave Ble server node."
            self.logger.error("[localhost] WeaveBle: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Ble server node exists.
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
            self.logger.error("[localhost] WeaveBle: %s" % (emsg))
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
            self.logger.error("[localhost] WeaveBle: %s" % (emsg))
            sys.exit(1)

        if not self.no_service and self.server_weave_id == None:
            emsg = "Could not find Weave node ID of the server node."
            self.logger.error("[localhost] WeaveBle: %s" % (emsg))
            sys.exit(1)

    def __start_server_side(self):
        if self.no_service:
            return

        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --enable-bluez-peripheral --peripheral-name N0001 --peripheral-address " + self.interfaces[0]["bd_address"]
        cmd += " --print-fault-counters"
        self.start_weave_process(node_id=self.server_node_id, cmd=cmd, tag=self.server_process_tag, rootMode=True)

    def resetBluez(self):
        self.disableBluetoothService()
        processBluetoothd = self.GetProcessByName('bluetoothd')
        self.logger.debug("WeaveBle: [%s]" % ', '.join(map(str, processBluetoothd)))

        processBtvirt = self.GetProcessByName('btvirt')
        self.logger.debug("WeaveBle: [%s]" % ', '.join(map(str, processBtvirt)))

        clearProcesslist = []
        if processBluetoothd:
            clearProcesslist += processBluetoothd
        if processBtvirt:
            clearProcesslist += processBtvirt
        if clearProcesslist:
            self.TerminateProcesses(clearProcesslist)

    def disableBluetoothService(self):
        cmd = "systemctl disable bluetooth"
        self.start_weave_process(node_id=None, cmd=cmd, tag='BluetoothService', rootMode=True)
        cmd = "/etc/init.d/bluetooth stop "
        self.start_weave_process(node_id=None, cmd=cmd, tag='BluetoothService', rootMode=True)

    def initializeBluetoothd(self):
        # bluetoothd need to reside in root namespace, it cannot be put in container.
        cmd = self.getBluetoothdPath()
        if not cmd:
            return
        cmd += ' --debug --experimental'
        self.start_weave_process(node_id=None, cmd=cmd, tag='wobleBluetoothd', rootMode=True)

    def initializeBtvirt(self):
        # TODO: Temporarily we put btvirt in root namespace. Later, we need to design one node that manage btvirt in container.
        cmd = self.getBtvirtPath()
        if not cmd:
            return
        cmd += ' -L -l2'
        self.start_weave_process(node_id=None, cmd=cmd, tag='wobleBtvirt', rootMode=True)
        delayExecution(1)

    def extract(self, data):
        result = re.search(r'^(?P<interface>hci\d+):.+(?P<Bus>Virtual)\s+'  +
                       r'(\s+BD Address: (?P<bd_address>\S+)) ' ,
                       data, re.MULTILINE )
        if result:
            info = result.groupdict()
            return info

        return {}

    def parseBtvirtInfo(self):
        # TODO: add output extraction outside of container in happy
        process = subprocess.Popen(['sudo','hciconfig'], stdout=subprocess.PIPE)
        out, err = process.communicate()
        print out
        self.interfaces = [self.extract(interface) for interface in out.split('\n\n') if interface.strip()]
        print self.interfaces

    def initializeBluez(self):
        self.resetBluez()
        self.initializeBluetoothd()
        self.initializeBtvirt()
        self.parseBtvirtInfo()

    def __start_client_side(self, client_info):
        dir_path = os.path.dirname(os.path.realpath(__file__))
        cmd = "python " + dir_path + "/WeaveBleCentral.py " + self.interfaces[1]["bd_address"] + " " + self.interfaces[0]["bd_address"]
        self.start_weave_process(node_id=client_info['client_node_id'], cmd=cmd, tag=client_info['client_process_tag'], rootMode=True)

    def __wait_for_client(self, client_info):
        self.wait_for_test_to_end(client_info["client_node_id"], tag=client_info['client_process_tag'], timeout=self.timeout)

    def __stop_server_side(self):
        if self.no_service:
            return

        self.stop_weave_process(self.server_node_id, self.server_process_tag)

    def run(self):
        all_data = []
        result_list = []
        self.logger.debug("[localhost] WeaveBle: Run.")

        self.__pre_check()

        self.initializeBluez()

        delayExecution(5)

        self.__start_server_side()

        emsg = "WeaveBle Peripheral %s should be running." % (self.server_process_tag)
        self.logger.debug("[%s] WeaveBle: %s" % (self.server_node_id, emsg))

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

            data = {}
            data.update(client_info)
            data["client_output"] = client_output_data
            data["server_output"] = server_output_data

            if "WoBLE central is good to go" in client_output_data:
                result_list.append(True)
            else:
                result_list.append(False)

            if self.strace:
                data["client_strace"] = client_strace_data
                data["server_strace"] = server_strace_data
            all_data.append(data)


        self.resetBluez()
        self.logger.debug("[localhost] WeaveBle: Done.")

        return ReturnMsg(result_list, all_data)


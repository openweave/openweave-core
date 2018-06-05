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

##
#    @file
#       Implements WeaveStateLoad class that sets up virtual network topology
#       together with Weave fabric configuration.
#

import json
import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *

import happy.HappyStateLoad

import WeaveFabricAdd
import WeaveNodeConfigure
import WeaveNetworkGateway
from Weave import Weave
from WeaveState import WeaveState

options = {}
options["quiet"] = False
options["json_file"] = None


def option():
    return options.copy()


class WeaveStateLoad(WeaveState):
    """
    Loads a Weave-enabled virtual network topology from a JSON file.

    weave-state-load [-h --help] [-q --quiet] [-f --file <JSON_FILE>]

        -f --file   Required. A valid JSON file with the topology to load.

    Example:
    $ weave-state-load myweavestate.json
        Creates a virtual network topology based on the state described
        in myweavestate.json.

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        WeaveState.__init__(self)

        self.quiet = opts["quiet"]
        self.new_json_file = opts["json_file"]

    def __pre_check(self):
        # Check if the name of the new node is given
        if self.new_json_file is None:
            emsg = "Missing name of file that specifies virtual network topology."
            self.logger.error("[localhost] WeaveStateLoad: %s" % (emsg))
            self.exit()

        # Check if json file exists
            if not os.path.exists(self.new_json_file):
                emsg = "Cannot find the configuration file %s" % (self.new_json_file)
                self.logger.error("[localhost] WeaveStateLoad: %s" % emsg)
                self.exit()

        self.new_json_file = os.path.realpath(self.new_json_file)

        emsg = "Loading Weave Fabric from file %s." % (self.new_json_file)
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

    def __load_JSON(self):
        emsg = "Import state file %s." % (self.new_json_file)
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

        try:
            with open(self.new_json_file, 'r') as jfile:
                json_data = jfile.read()

            self.weave_topology = json.loads(json_data)

        except Exception:
            emsg = "Failed to load JSON state file: %s" % (self.new_json_file)
            self.logger.error("[localhost] WeaveStateLoad: %s" % emsg)
            self.exit()

    def __load_network_topology(self):
        emsg = "Loading network topology."
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

        options = happy.HappyStateLoad.option()
        options["quiet"] = self.quiet
        options["json_file"] = self.new_json_file

        happyLoad = happy.HappyStateLoad.HappyStateLoad(options)
        happyLoad.run()

        self.readState()

    def __create_fabric(self):
        emsg = "Creating Weave Fabric."
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

        options = WeaveFabricAdd.option()
        options["fabric_id"] = self.getFabricId(self.weave_topology)
        options["quiet"] = self.quiet

        if options["fabric_id"] is not None:
            addFabric = WeaveFabricAdd.WeaveFabricAdd(options)
            addFabric.run()

        self.readState()

    def __configure_weave_nodes(self):
        emsg = "Configuring weave nodes"
        self.logger.debug("[localhost] WeaveStateLoad: %s" % emsg)

        weave_nodes = self.getWeaveNodeRecord(self.weave_topology)

        for node in weave_nodes.keys():
            options = WeaveNodeConfigure.option()
            options['quiet'] = self.quiet

            options['node_name'] = node

            node_record = weave_nodes[node]
            options['weave_node_id'] = node_record.get('weave_node_id', None)
            options['weave_certificate'] = node_record.get('weave_node_certificate', "")
            options['private_key'] = node_record.get('private_key', "")
            options['pairing_code'] = node_record.get('pairing_code', None)

            wnc = WeaveNodeConfigure.WeaveNodeConfigure(options)
            wnc.run()

        self.readState()

    def __configure_network_gateway(self):
        emsg = "Configuring Weave gateway."
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

        for network_id in self.getWeaveNetworkIds(self.weave_topology):
            gateways = self.getWeaveNetworkGatewayIds(network_id, self.weave_topology)
            for gateway in gateways:

                options = WeaveNetworkGateway.option()
                options["quiet"] = self.quiet
                options["add"] = True

                options["network_id"] = network_id
                options["gateway"] = gateway

                wsg = WeaveNetworkGateway.WeaveNetworkGateway(options)
                wsg.run()

                self.readState()

    def __post_check(self):
        emsg = "Loading Weave Fabric completed."
        self.logger.debug("[localhost] WeaveStateLoad: %s" % (emsg))

    def run(self):
        with self.getStateLockManager():

            self.__pre_check()

            self.__load_JSON()

            self.__load_network_topology()

            self.__create_fabric()

            self.__configure_weave_nodes()

            self.__configure_network_gateway()

            self.__post_check()

        return ReturnMsg(0)

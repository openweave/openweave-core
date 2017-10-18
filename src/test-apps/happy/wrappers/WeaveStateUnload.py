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
#       Implements WeaveStateUnload class that tears down virtual network topology
#       together with Weave fabric configuration.
#

import json
import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *

import happy.HappyStateUnload

from WeaveState import WeaveState
import WeaveFabricDelete
import WeaveNetworkGateway
import WeaveNodeConfigure

options = {}
options["quiet"] = False
options["json_file"] = None


def option():
    return options.copy()


class WeaveStateUnload(WeaveState):
    """
    weave-state-unload loads virtual weave network from a file.

    weave-state-unload [-h --help] [-q --quiet] [-f --file <JSON_FILE>]

    Example:
    $ weave-state-unload <file>.json
        Deletes virtual network topology based on description specified in <file>.json.

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        WeaveState.__init__(self)

        self.quiet = opts["quiet"]
        self.old_json_file = opts["json_file"]

    def __pre_check(self):
        # Check if the name of the new node is given
        if self.old_json_file is None:
            emsg = "Missing name of file that specifies virtual network topology."
            self.logger.error("[localhost] HappyStateUnload: %s" % (emsg))
            self.exit()

        # Check if json file exists
            if not os.path.exists(self.old_json_file):
                emsg = "Cannot find the configuration file %s" % (self.old_json_file)
                self.logger.error("[localhost] HappyStateUnload: %s" % emsg)
                self.exit()

        self.old_json_file = os.path.realpath(self.old_json_file)

        emsg = "Unloading Weave Fabric from file %s." % (self.old_json_file)
        self.logger.debug("[localhost] HappyStateUnload: %s" % emsg)

    def __load_JSON(self):
        emsg = "Import state file %s." % (self.old_json_file)
        self.logger.debug("[localhost] WeaveStateUnload: %s" % (emsg))

        try:
            with open(self.old_json_file, 'r') as jfile:
                json_data = jfile.read()

            self.weave_topology = json.loads(json_data)

        except Exception:
            emsg = "Failed to load JSON state file: %s" % (self.old_json_file)
            self.logger.error("[localhost] HappyStateUnload: %s" % emsg)
            self.exit()

    def __unload_network_topology(self):
        emsg = "Unloading network topology."
        self.logger.debug("[localhost] WeaveStateUnload: %s" % (emsg))

        options = happy.HappyStateUnload.option()
        options["quiet"] = self.quiet
        options["json_file"] = self.old_json_file

        happyUnload = happy.HappyStateUnload.HappyStateUnload(options)
        happyUnload.run()

    def __delete_network_gateway(self):
        emsg = "Deleting Weave gateway."
        self.logger.debug("[localhost] WeaveStateUnload: %s" % (emsg))

        for network_id in self.getWeaveNetworkIds(self.weave_topology):
            gateways = self.getWeaveNetworkGatewayIds(network_id, self.weave_topology)
            for gateway in gateways:
                options = WeaveNetworkGateway.option()
                options["quiet"] = self.quiet
                options["delete"] = True

                options["network_id"] = network_id
                options["gateway"] = gateway

                wsg = WeaveNetworkGateway.WeaveNetworkGateway(options)
                wsg.run()

                self.readState()

    def __delete_fabric(self):
        emsg = "Deleting Weave Fabric."
        self.logger.debug("[localhost] WeaveStateUnload: %s" % (emsg))

        fabric_id = self.getFabricId(self.weave_topology)

        if fabric_id is None:
            return

        options = WeaveFabricDelete.option()
        options["fabric_id"] = fabric_id
        options["quiet"] = self.quiet

        delFabric = WeaveFabricDelete.WeaveFabricDelete(options)
        delFabric.run()

        self.readState()

    def __delete_weave_nodes(self):
        emsg = "Deleting Weave Nodes"
        self.logger.debug("[localhost] WeaveStateUnload: %s" % emsg)

        options = WeaveNodeConfigure.option()
        options['delete'] = True
        options['quiet'] = self.quiet

        wnc = WeaveNodeConfigure.WeaveNodeConfigure(options)

        self.readState()

    def __post_check(self):
        emsg = "Unloading Weave Fabric completed."
        self.logger.debug("[localhost] WeaveStateUnload: %s" % (emsg))

    def run(self):
        with self.getStateLockManager():

            self.__pre_check()

            self.__load_JSON()

            self.__delete_network_gateway()

            self.__delete_fabric()

            self.__post_check()

        self.__unload_network_topology()

        return ReturnMsg(0)

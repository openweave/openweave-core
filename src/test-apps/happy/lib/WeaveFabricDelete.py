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

##
#    @file
#       Implements WeaveFabricDelete class that removes Weave Fabric.
#

import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNetwork import HappyNetwork
import happy.HappyNetworkAddress

from Weave import Weave

options = {}
options["quiet"] = False
options["fabric_id"] = None


def option():
    return options.copy()


class WeaveFabricDelete(HappyNetwork, Weave):
    """
    Deletes a Weave Fabric. If there are any nodes or networks, their lose
    their ULA addresses on Weave fabric.

    weave-fabric-delete [-h --help] [-q --quiet] [-i --id <FABRIC_ID>]

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        HappyNetwork.__init__(self)
        Weave.__init__(self)

        self.quiet = opts["quiet"]
        self.fabric_id = opts["fabric_id"]
        self.done = False

    def __pre_check(self):
        # Check if the name of the fabric is given
        if not self.fabric_id:
            self.fabric_id = self.getFabricId()
            if self.fabric_id is not None:
                emsg = "Weave fabric is not provided. Assuming Fabric %s" % (self.fabric_id)
                self.logger.warning("[localhost] WeaveFabricDelete: %s" % (emsg))

        if self.getFabricId() is None:
            emsg = "Weave fabric does not exists."
            self.logger.warning("[localhost] WeaveFabricDelete: %s" % (emsg))
            self.done = True

        try:
            self.fabric_id = "%x" % (int(str(self.fabric_id), 16))
        except Exception:
            emsg = "Cannot convert fabric id %s into a number." % (self.fabric_id)
            self.logger.warning("[localhost] WeaveFabricDelete: %s" % (emsg))
            self.exit()

    def __post_check(self):
        pass

    def __delete_fabric_state(self):
        self.removeFabric()

    def __delete_nodes_state(self):
        self.removeWeaveNodes()

    def __delete_node_weave_fabric_address(self, node_id, interface_id, addr):
        options = happy.HappyNodeAddress.option()
        options["quiet"] = self.quiet
        options["node_id"] = node_id
        options["interface"] = interface_id
        options["address"] = addr
        options["delete"] = True

        addAddr = happy.HappyNodeAddress.HappyNodeAddress(options)
        addAddr.run()

    def __delete_nodes_addresses(self):
        for node_id in self.getNodeIds():
            if node_id not in self.getWeaveNodes():
                continue

            for interface_id in self.getNodeInterfaceIds(node_id):
                interface_type = self.getNodeInterfaceType(interface_id, node_id)
                subnet = self.typeToWeaveSubnet(interface_type)
                if subnet is None:
                    continue
                subnet_hex = "%04x" % (subnet)

                iid = self.getWeaveNodeIID(node_id)
                self.global_prefix_addr, _ = self.global_prefix.split("::")
                addr = self.global_prefix_addr + ":" + subnet_hex + ":" + iid

                self.__delete_node_weave_fabric_address(node_id, interface_id, addr)

    def run(self):
        with self.getStateLockManager():
            self.__pre_check()

        if self.done:
            return ReturnMsg(0)

        self.global_prefix = self.getWeaveGlobalPrefix(self.fabric_id)
        self.__delete_nodes_addresses()

        with self.getStateLockManager():
            self.readState()

            self.__delete_nodes_state()
            self.__delete_fabric_state()

            self.writeState()

        return ReturnMsg(0)

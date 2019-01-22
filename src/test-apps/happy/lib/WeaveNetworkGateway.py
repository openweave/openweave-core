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
#       Implements WeaveNetworkGateway class that controls network gateway.
#
#       A virtual network is logical representation of a virtual ethernet
#       bridge that acts like a hub.
#

import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode
import happy.HappyNetworkRoute

from Weave import Weave

options = {}
options["quiet"] = False
options["network_id"] = None
options["add"] = False
options["delete"] = False
options["gateway"] = None


def option():
    return options.copy()


class WeaveNetworkGateway(HappyNetwork, HappyNode, Weave):
    """
    Manages Weave network IP routes in a Happy topology and assigns a gateway
    node for the Weave Fabric.

    weave-network-gateway [-h --help] [-q --quiet] [-a --add] [-d --delete]
                          [-i --id <NETWORK_NAME>] [-g --gateway <NODE>]

        -i --id         Required. Network to add routes to. Find using happy-network-list
                        or happy-state.
        -g --gateway    Required. Weave node to serve as the Weave Fabric gateway.

    Examples:
    $ weave-network-gateway HomeThread BorderRouter
        Adds routes for all Weave nodes in the HomeThread network to the default
        routes of all nodes accessible via the BorderRouter gateway.

    $ weave-network-gateway -d -i HomeThread -g BorderRouter
        Delete routes for all Weave nodes in the HomeThread network to the default
        routes of all nodes accessible via the BorderRouter gateway.

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        HappyNetwork.__init__(self)
        HappyNode.__init__(self)
        Weave.__init__(self)

        self.quiet = opts["quiet"]
        self.network_id = opts["network_id"]
        self.add = opts["add"]
        self.delete = opts["delete"]
        self.gateway = opts["gateway"]

    def __pre_check(self):
        # Check if the name of the new network is given
        if not self.network_id:
            emsg = "Missing name of the virtual network that should be configured with a route."
            self.logger.error("[localhost] WeaveNetworkGateway: %s" % (emsg))
            self.exit()

        # Check if the name of new network is not a duplicate (that it does not already exists).
        if not self._networkExists():
            emsg = "virtual network %s does not exist." % (self.network_id)
            self.logger.error("[%s] WeaveNetworkGateway: %s" % (self.network_id, emsg))
            self.exit()

        if not self.delete:
            self.add = True

        self.fabric_id = self.getFabricId()

        if not self.fabric_id:
            emsg = "weave fabric id is unknown."
            self.logger.error("[%s] WeaveNetworkGateway: %s" % (self.network_id, emsg))
            self.exit()

        self.global_prefix = self.getWeaveGlobalPrefix(self.fabric_id)

        if not self.global_prefix:
            emsg = "weave fabric prefix is unknown."
            self.logger.error("[%s] WeaveNetworkGateway: %s" % (self.network_id, emsg))
            self.exit()

        # Check if 'gateway' is given
        if not self.gateway:
            emsg = "Missing gateway address for weave fabric."
            self.logger.error("[%s] WeaveNetworkGateway: %s" % (self.network_id, emsg))
            self.exit()

        if not self._nodeExists(self.gateway):
            emsg = "Proposed gateway node %s does not exist." % (self.gateway)
            self.logger.error("[%s] WeaveNetworkGateway: %s" % (self.network_id, emsg))
            self.exit()

        if IP.isIpAddress(self.gateway):
            self.gateway = IP.paddingZeros(self.gateway)

        self.to = IP.paddingZeros(self.global_prefix)

    def __configure_network_route(self):
        options = happy.HappyNetworkRoute.option()
        options["network_id"] = self.network_id
        options["to"] = self.to
        options["via"] = self.gateway
        options["prefix"] = self.global_prefix
        options["add"] = self.add
        options["delete"] = self.delete
        options["record"] = False

        weaveRoute = happy.HappyNetworkRoute.HappyNetworkRoute(options)
        weaveRoute.start()

        self.readState()

    def __post_check(self):
        pass

    def __update_state(self):
        if self.add:
            self.setWeaveGateway(self.network_id, self.gateway)
        else:
            self.removeWeaveGateway(self.network_id)

    def run(self):
        with self.getStateLockManager():

            self.__pre_check()

            self.__configure_network_route()

            self.__post_check()

            self.__update_state()

            self.writeState()

        return ReturnMsg(0)

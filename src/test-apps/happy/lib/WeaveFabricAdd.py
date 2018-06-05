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
#       Implements WeaveFabricAdd class that creates Weave Fabric.
#

import os
import sys
import uuid

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode
import happy.HappyNetworkAddress

from Weave import Weave
import WeaveFabricDelete

options = {}
options["quiet"] = False
options["fabric_id"] = None


def option():
    return options.copy()


class WeaveFabricAdd(HappyNetwork, HappyNode, Weave):
    """
    Creates a Weave Fabric for a Happy topology.

    weave-fabric-add [-h --help] [-q --quiet] [-i --id <FABRIC_ID>]

        -i --id 	Optional. Fabric ID. Limit 10 characters in
                    hexadecimal format. Find using weave-state. Omit
                    to generate a random Fabric ID.

    Examples:
    $ weave-fabric-delete fab1
        Creates a Weave Fabric with an ID of fab1 and a Weave
        Global Prefix of fd00:0000:fab1::/48.

    $ weave-fabric-add
        Creates a Weave Fabric with a randomly-generated ID.

    return:
        0    success
        1    fail
    """

    def __init__(self, opts=options):
        HappyNetwork.__init__(self)
        HappyNode.__init__(self)
        Weave.__init__(self)

        self.quiet = opts["quiet"]
        self.fabric_id = opts["fabric_id"]
        self.weave_nodes = {}

    def __deleteExistingFabric(self):
        options = WeaveFabricDelete.option()
        options["fabric_id"] = self.getFabricId()
        options["quiet"] = self.quiet
        delFabric = WeaveFabricDelete.WeaveFabricDelete(options)
        delFabric.run()

        self.readState()

    def __pre_check(self):
        # Check if the name of the new fabric is given
        if not self.fabric_id:
            emsg = "Missing name of the new Weave fabric that should be created."
            self.logger.warning("[localhost] WeaveFabricAdd: %s" % (emsg))

            self.fabric_id = "%x" % (uuid.uuid4().int >> 64)

            emsg = "Generated random Weave fabric ID %s." % (self.fabric_id)
            self.logger.warning("[localhost] WeaveFabricAdd: %s" % (emsg))

        # Check if the there is a fabric
        if self.getFabricId():
            emsg = "There is already a Weave fabric %s." % (self.getFabricId())
            self.logger.warning("[localhost] WeaveFabricAdd: %s" % (emsg))
            self.__deleteExistingFabric()

        try:
            self.fabric_id = "%x" % (int(str(self.fabric_id), 16))
        except Exception:
            emsg = "Cannot convert fabric id %s into a number." % (self.fabric_id)
            self.logger.error("[localhost] WeaveFabricAdd: %s" % (emsg))
            self.exit()

    def __post_check(self):
        pass

    def __add_fabric_state(self):
        new_fabric = {}
        new_fabric["id"] = self.fabric_id
        new_fabric["global_prefix"] = self.global_prefix

        self.setFabric(new_fabric)

    def run(self):
        with self.getStateLockManager():
            self.__pre_check()
            self.global_prefix = self.getWeaveGlobalPrefix(self.fabric_id)
            self.__add_fabric_state()
            self.writeState()

        self.__post_check()
        return ReturnMsg(0)

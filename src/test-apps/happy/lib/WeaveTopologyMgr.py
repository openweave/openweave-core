#!/usr/bin/env python

#
#    Copyright (c) 2017 Nest Labs, Inc.
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
#       Implements WeaveTopologyMgr class that provides API to create weave topology
#

import WeaveFabricAdd
import WeaveFabricDelete
import WeaveNetworkGateway
import WeaveNodeConfigure


class WeaveTopologyMgr(object):
    def WeaveFabricAdd(self, fabric_id=None, quiet=False):
        options = WeaveFabricAdd.option()
        options["quiet"] = quiet
        options["fabric_id"] = fabric_id

        cmd = WeaveFabricAdd.WeaveFabricAdd(options)
        cmd.start()

    def WeaveFabricDelete(self, fabric_id=None, quiet=False):
        options = WeaveFabricDelete.option()
        options["quiet"] = quiet
        options["fabric_id"] = fabric_id
        cmd = WeaveFabricDelete.WeaveFabricDelete(options)
        cmd.start()

    def WeaveNetworkGateway(self, network_id=None, add=False, delete=False, gateway=None, quiet=False):
        options = WeaveNetworkGateway.option()
        options["quiet"] = quiet
        options["network_id"] = network_id
        options["add"] = add
        options["delete"] = delete
        options["gateway"] = gateway

        cmd = WeaveNetworkGateway.WeaveNetworkGateway(options)
        cmd.start()

    def WeaveNodeConfigure(self, opts=None):
        options = WeaveNodeConfigure.option()
        options.update(opts)

        cmd = WeaveNodeConfigure.WeaveNodeConfigure(options)
        cmd.run()

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

#
#    @file
#       Implements WeaveTunnelStart class that start Weave Tunnel
#       between border-gateway and a service.
#

import os
import sys

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode

from WeaveTest import WeaveTest


options = {"border_gateway": None,
           "service": None,
           "service_dir": False,
           "service_dir_server": None,
           "service_process_tag": "WEAVE-SERVICE-TUNNEL",
           "gateway_process_tag": "WEAVE-GATEWAY-TUNNEL",
           "skip_service_end": False,
           "strace": False,
           "test_tag": ""
           }


def option():
    return options.copy()


class WeaveTunnelStop(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-tunnel-stop stops tunnel connection between border-gateway and a service

    weave-tunnel-stop [-h --help] [-q --quiet] [-b --border_gateway <NODE_ID>]
        [-s --service <NODE_ID>]

    Example:
    $ weave-tunnel-stop node01 node02
        Stops Weave tunnel between border-gateway node01 and service node02.

    $ weave-tunnel-stop
        Attempts to guess border gateway and service and then stop Weave tunnel between them.


    return:
        0    success
        1    fail
    """

    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)
        self.__dict__.update(opts)
        self.gateway_process_tag += self.test_tag
        self.service_process_tag += self.test_tag

    def __deleteExistingFabric(self):
        options = WeaveTunnelStop.option()
        options["fabric_id"] = self.getFabricId()
        delFabric = WeaveTunnelStop.WeaveTunnelStop(options)
        delFabric.run()

        self.readState()


    def __pre_check(self):
        if self.border_gateway:
            if self.border_gateway != self.getTunnelGatewayNodeId():
                emsg = "Incorrect border-gateway node id. Entered %s vs %s on the record." % \
                    (self.border_gateway, self.getTunnelGatewayNodeId())
                self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                self.exit()
        else:
            self.border_gateway = self.getTunnelGatewayNodeId()

        # Check if border-gateway node was given
        if not self.border_gateway:
            emsg = "Border-gateway node id not specified and there is none on the record."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        if not self._nodeExists(self.border_gateway):
            emsg = "Border-gateway node %s does not exist." % (self.border_gateway)
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        # Check if service node was given in the environment
        if not self.service:
            self.service = self.getTunnelServiceNodeId()

        if not self.service and self.service_dir:
            if self.service_dir_server:
                self.skip_service_end = True
                self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)
            else:
                self.service_dir_server = self.getTunnelServiceDir()
                if not self.service_dir_server:
                    # Check if service node was given
                    emsg = "Service node id (or IP address or service directory) not specified."
                    self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                    self.exit()
                else:
                    self.skip_service_end = True
                    self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)

        if self.service:
            # If service is a domain name, convert it to IP
            if IP.isDomainName(self.service):
                ip = IP.getHostByName(self.service)
                self.service = ip

            if not IP.isIpAddress(self.service):
                if self.service:
                    if self.service != self.getTunnelServiceNodeId():
                        emsg = "Incorrect service node id. Entered %s vs %s on the record." % \
                        (self.service, self.getTunnelServiceNodeId())
                        self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                        self.exit()

            if not IP.isIpAddress(self.service) and not self._nodeExists(self.service):
                emsg = "Service node %s does not exist." % (self.service)
                self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                self.exit()

            if IP.isIpAddress(self.service):
                self.skip_service_end = True

        # Check if there is no fabric
        if not self.getFabricId():
            emsg = "There is not Weave fabric."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        # Check if there is a tunnel
        if not self.getTunnelServiceNodeId() and not self.getTunnelServiceDir() :
            emsg = "There is no Weave tunnel on the record."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()


    def __stop_tunnel_at_service(self):
        if self.skip_service_end:
            return
        self.stop_weave_process(self.service, self.service_process_tag)


    def __stop_tunnel_at_gateway(self):
        self.stop_weave_process(self.border_gateway, self.gateway_process_tag)
 

    def __post_check(self):
        pass


    def __delete_tunnel_state(self):
        self.removeTunnel()


    def run(self):
        self.logger.debug("[localhost] WeaveTunnelStop: Run.")

        with self.getStateLockManager():
            self.__pre_check()

        self.__stop_tunnel_at_gateway()
        self.__stop_tunnel_at_service()

        with self.getStateLockManager():
            self.readState()

            self.__post_check()

            self.__delete_tunnel_state()

            self.writeState()

        data = {}

        gateway_output_value, gateway_output_data = \
            self.get_test_output(self.border_gateway, self.gateway_process_tag, quiet=True)
        if self.strace:
            gateway_strace_value, gateway_strace_data = \
                self.get_test_strace(self.border_gateway, self.gateway_process_tag)
            data["gateway_strace"] = gateway_strace_data

        if self.skip_service_end:
            service_output_data = ""
            data["service_strace"] = ""
            service_output_value = 0
        else:
            service_output_value, service_output_data = \
                self.get_test_output(self.service, self.service_process_tag, quiet=True)
            if self.strace:
                service_strace_value, service_strace_data = \
                    self.get_test_strace(self.service, self.service_process_tag)
                data["service_strace"] = service_strace_data

        data["gateway_output"] = gateway_output_data
        data["service_output"] = service_output_data
        result = gateway_output_value or service_output_value # 0 == SUCCESS

        self.logger.debug("[localhost] WeaveTunnelStop: Done.")
        return ReturnMsg(result, data)


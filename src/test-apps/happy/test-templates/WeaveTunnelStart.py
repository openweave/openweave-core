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
import time

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
from happy.HappyNetwork import HappyNetwork
from happy.HappyNode import HappyNode

import happy.HappyNodeRoute
import happy.HappyNetworkAddress

import WeaveTunnelStop
from WeaveTest import WeaveTest
import WeaveUtilities


options = {"border_gateway": None,
           "service": None,
           "tap": None,
           "primary": None,
           "backup": None,
           "customized_tunnel_port": None,
           "service_dir": False,
           "skip_service_end": False,
           "tunnel_log": True,
           "case": False,
           "service_dir_server": None,
           "service_process_tag": "WEAVE-SERVICE-TUNNEL",
           "gateway_process_tag": "WEAVE-GATEWAY-TUNNEL",
           "case_cert_path": None,
           "case_key_path": None,
           "test_tag": "",
           "iterations": 1,
           "gateway_faults": None,
           "service_faults": None,
           "strace": False,
           "sync_on_gateway_output": "",
           "sync_on_service_output": "",
           "plaid_gateway_env": {},
           "plaid_service_env": {}
           }


def option():
    return options.copy()


class WeaveTunnelStart(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-tunnel-start start a tunnel between border-gateway and a service

    weave-tunnel-start [-h --help] [-q --quiet] [-b --border_gateway <NODE_ID>]
        [-s --service <NODE_ID> | <IP_ADDR>] [-p --tap <TAP_INTERFACE>] [-P --primary <PRIMARY_INTERFACE>] [-B --backup <BACKUP_INTERFACE>]
        [-d --service_dir] [-i --service_dir_server <SERVICE_HOST>] [-C --case] [-E --case_cert_path <path>] [-T --case_key_path <path>]

    Example:
    $ weave-tunnel-start node01 node02
        Start Weave tunnel between border-gateway node01 and service node02.

    $ weave-tunnel-start node01 184.72.129.8
        Start Weave tunnel between border-gateway node01 and a node with an IP address 184.72.129.8.

    $ weave-tunnel-start --border_gateway node01 --service_dir --service_dir_server  "frontdoor.integration.nestlabs.com" --case --case_cert_path path --case_key_path path
        Start Weave tunnel with case encryption between border-gateway node01 and a node with the service loopup directory frontdoor.integration.nestlabs.com.

    $ weave-tunnel-start --border_gateway node01 --service 184.72.129.8 -P wlan0 -B cellular0
        Start Weave tunnel between border-gateway node01 and a node with an IP address 184.72.129.8, the primary tunnel go through wlan0, the backup tunnel
        go through cellular0

    return:
        0    success
        1    fail
    """

    def __init__(self, opts = options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)
        self.__dict__.update(opts)
        emsg = "starting with opts: %s" %opts
        self.logger.debug("[localhost] WeaveTunnelStart: %s" %emsg)

        self.gateway_process_tag += self.test_tag
        self.service_process_tag += self.test_tag

    def __stopExistingTunnel(self):
        options = WeaveTunnelStop.option()
        options["border_gateway"] = self.border_gateway
        options["service"] = self.service

        stopTunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        stopTunnel.run()

        self.readState()


    def __pre_check(self):
        # Check if border-gateway node was given
        if not self.border_gateway:
            emsg = "Border-gateway node id not specified."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        if not self._nodeExists(self.border_gateway):
            emsg = "Border-gateway node %s does not exist." % (self.border_gateway)
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        # Check if service node was given in the environment
        if not self.service and not self.service_dir:
            if "weave_service_address" in os.environ.keys():
                self.service = os.environ['weave_service_address']
                emsg = "Found weave_service_address %s." % (self.service)
                self.logger.debug("[localhost] Weave: %s" % (emsg))

        if not self.service and self.service_dir:
            if self.service_dir_server:
                self.skip_service_end = True
                self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)
            else:
                if "weave_service_address" in os.environ.keys():
                    self.skip_service_end = True
                    self.service_dir_server = os.environ['weave_service_address']
                    self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)
                else:
                    # Check if service node was given
                    emsg = "Service node id (or IP address or service directory) not specified."
                    self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                    self.exit()

        if self.service:
            # If service is a domain name, convert it to IP
            if IP.isDomainName(self.service):
                ip = IP.getHostByName(self.service)
                self.service = ip

            if not IP.isIpAddress(self.service) and not self._nodeExists(self.service):
                emsg = "Service node %s does not exist." % (self.service)
                self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                self.exit()

            if IP.isIpAddress(self.service):
                self.service_ip = self.service
                self.skip_service_end = True
            else:
                self.service_ip = self.getNodePublicIPv4Address(self.service)

            if self.service_ip == None:
                emsg = "Could not find IP address of the service node."
                self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
                self.exit()

        self.fabric_id = self.getFabricId()

        # Check if there is no fabric
        if self.fabric_id == None:
            emsg = "There is not Weave fabric."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.exit()

        # Check if there is a tunnel
        if self.getTunnelServiceNodeId() or self.getTunnelServiceDir():
            emsg = "There already exist a Weave tunnel."
            self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
            self.__stopExistingTunnel()


        self.service_weave_id = self.getServiceWeaveID("Tunnel")
        self.gateway_weave_id = self.getWeaveNodeID(self.border_gateway)

        if self.service_weave_id == None:
            emsg = "Could not find Weave node ID of the service node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            self.exit()

        if self.gateway_weave_id == None:
            emsg = "Could not find Weave node ID of the border-gateway node."
            self.logger.error("[localhost] WeaveTunnel: %s" % (emsg))
            self.exit()


    def __start_tunnel_at_service(self):
        cmd = self.getWeaveTunnelTestServerPath()
        if not cmd:
            return

        cmd += " --node-id " + str(self.service_weave_id)
        cmd += " --fabric-id " + str(self.fabric_id)

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.service_faults:
            cmd += " --faults " + self.service_faults
            cmd += " --iterations " + str(self.iterations)

        if self.gateway_faults or self.service_faults:
            cmd += " --extra-cleanup-time 10000"

        cmd += " --debug-resource-usage"
        cmd += " --print-fault-counters"

        cmd = self.runAsRoot(cmd)
        self.start_weave_process(self.service, cmd, self.service_process_tag, strace=self.strace, sync_on_output=self.sync_on_service_output, env=self.plaid_service_env)

        max_wait_time = 40

        while not self._nodeInterfaceExists(self.service_tun, self.service):
            time.sleep(0.1)
            max_wait_time -= 1

            if max_wait_time <= 0:
                emsg = "Service-side tunnel interface %s was not created." % (self.service_tun)
                self.logger.error("[%s] WeaveTunnel: %s" % (self.service, emsg))
                self.exit()

            if (max_wait_time) % 10 == 0:
                emsg = "Waiting for interface %s to be created." % (self.service_tun)
                self.logger.debug("[%s] WeaveTunnel: %s" % (self.service, emsg))

        with self.getStateLockManager():
            self.readState()

            new_node_interface = {}
            new_node_interface["link"] = None
            new_node_interface["type"] = self.network_type["tun"]
            new_node_interface["ip"] = {}
            self.setNodeInterface(self.service, self.service_tun, new_node_interface)

            self.writeState()


    def __assing_tunnel_endpoint_ula_at_service(self):
        addr = self.getServiceWeaveIPAddress("Tunnel", self.service)

        options = happy.HappyNodeAddress.option()
        options["node_id"] = self.service
        options["interface"] = self.service_tun
        options["address"] = addr
        options["add"] = True

        addAddr = happy.HappyNodeAddress.HappyNodeAddress(options)
        addAddr.run()


    def __start_tunnel_at_gateway(self):
        cmd = self.getWeaveTunnelBorderGatewayPath()
        if not cmd:
            return

        cmd += " --node-id " + self.gateway_weave_id
        cmd += " --fabric-id " + str(self.fabric_id)

        if self.tap:
            cmd += " --tap-device " + self.tap
        if self.primary:
            cmd += " --primary-intf " + self.primary
        if self.backup:
            cmd += " --backup-intf " + self.backup + " --enable-backup "
        if self.tunnel_log:
            cmd += " --tunnel-log"

        if self.case:
            cmd += ' --case'
            if not self.case_cert_path == 'default':
                self.cert_file = self.case_cert_path if self.case_cert_path else os.path.join(self.main_conf['log_directory'], self.gateway_weave_id.upper() + '-cert.weave-b64')
                self.key_file = self.case_key_path if self.case_key_path else os.path.join(self.main_conf['log_directory'], self.gateway_weave_id.upper() + '-key.weave-b64')
                cmd += ' --node-cert ' + self.cert_file + ' --node-key ' + self.key_file

            if self.service_dir:
                cmd += " --service-dir " + str(self.service_weave_id) + ' --service-dir-server ' + self.service_dir_server
            else:
                cmd += " --connect-to " + self.service_ip + " " + str(self.service_weave_id)
        elif self.service:
            if self.customized_tunnel_port:
                self.service_ip = self.service_ip + ":%d" % self.customized_tunnel_port
            cmd += " --connect-to " + self.service_ip + " " + str(self.service_weave_id)

        # fault-injection related
        cmd += " --debug-resource-usage"
        cmd += " --print-fault-counters"

        if self.gateway_faults:
            cmd += " --faults " + self.gateway_faults
            cmd += " --iterations " + str(self.iterations)

        if self.gateway_faults or self.service_faults:
            cmd += " --extra-cleanup-time 10000"

        if self.jitter_distribution_curve is not None:
            self.jitter(self.jitter_distribution_curve)

        cmd = self.runAsRoot(cmd)
        self.start_weave_process(self.border_gateway, cmd, self.gateway_process_tag, strace=self.strace, sync_on_output=self.sync_on_gateway_output, env=self.plaid_gateway_env)


    def __post_check(self):
        tries = 100
        while not self._nodeInterfaceExists(self.gateway_tun, self.border_gateway):
            self.logger.debug("[localhost] WeaveTunnelStart: sleeping in __post_check")
            time.sleep(1)
            tries -= 1
            if tries < 0:
                emsg = "Tunnel interface %s cannot be found at node %s." % (self.gateway_tun, self.border_gateway)
                self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                self.exit()


    def __add_tunnel_state(self):
        new_tunnel = {}
        new_tunnel["gateway"] = self.border_gateway
        new_tunnel["service"] = self.service
        new_tunnel["service_dir"] = self.service_dir_server
        self.setTunnel(new_tunnel)


    def __add_tunnel_route(self):
        self.global_prefix = self.getFabricGlobalPrefix()

        options = happy.HappyNodeRoute.option()
        options["node_id"] = self.border_gateway
        options["to"] = self.global_prefix
        options["via"] = self.gateway_tun
        options["prefix"] = self.global_prefix
        options["add"] = True

        self.logger.debug("[localhost] WeaveTunnelStart: adding route with options: " + str(options) )
        weaveServiceRoute = happy.HappyNodeRoute.HappyNodeRoute(options)
        weaveServiceRoute.start()


    def run(self):
        self.logger.debug("[localhost] WeaveTunnelStart: Run.")

        with self.getStateLockManager():
            self.__pre_check()

        if not self.skip_service_end:
            self.__start_tunnel_at_service()
            self.__assing_tunnel_endpoint_ula_at_service()

        self.__start_tunnel_at_gateway()

        with self.getStateLockManager():
            self.readState()

            self.__post_check()

            self.__add_tunnel_state()

            self.writeState()

        self.__add_tunnel_route()

        data = {}

        gateway_output_value, gateway_output_data = \
            self.get_test_output(self.border_gateway, self.gateway_process_tag, quiet=True)
        if self.strace:
            gateway_strace_value, gateway_strace_data = \
                self.get_test_strace(self.border_gateway, self.gateway_process_tag)
            data["gateway_strace"] = gateway_strace_data

        if self.skip_service_end:
            service_output_data = ""
            service_strace_data = ""
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
        result = gateway_output_value or service_output_value  # 0 == success, hence 'or'

        self.logger.debug("[localhost] WeaveTunnelStart: Done.")
        return ReturnMsg(result, data)


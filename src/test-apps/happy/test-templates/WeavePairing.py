#!/usr/bin/env python


#
#    Copyright (c) 2020 Google, LLC.
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
#       Implements WeavePairing class that tests Weave Pairing among Weave node and device manager.
#

import os
import random
import re
import sys
import time

from happy.ReturnMsg import ReturnMsg
from happy.utils.IP import IP
from happy.HappyNode import HappyNode
from happy.HappyNetwork import HappyNetwork

from WeaveTest import WeaveTest


options = {"quiet": False,
           "service": None,
           "mobile": None,
           "device": None,
           "server": None,
           "service_dir": False,
           "woca_server": None,
           "tun_server": None,
           "service_dir_server": None,
           "case": False,
           "case_cert_path": None,
           "case_key_path": None,
           "tap": None,
           "tier": None,
           "username": None,
           "password": None,
           "devices_info": [],
           'mobile_process_tag': "WEAVE-PAIRING-MOBILE",
           'device_process_tag': "WEAVE-PAIRING-DEVICE",
           'server_process_tag': "WEAVE-PAIRING-SERVER",
           'mobile_node_id': None,
           'server_node_id': None,
           'woca_node_id': None,
           'register_cmd': None}


def option():
    return options.copy()


class WeavePairing(HappyNode, HappyNetwork, WeaveTest):
    """
    weave-pairing [-h --help] [-q --quiet] [-m --mobile <NAME>] [-d --device <NAME>] [-s --server <NAME>] [-w --woca_server <NAME>]
        [-C --case] [-E --case_cert_path <path>] [-T --case_key_path <path>]

    command to test pairing using a local mock server:
        $ weave-pairing --mobile node01 --device node02 --server node03 --woca_server node03

    command to test pairing using the default Weave ServiceProvisioning node-id and WOCA node-id over the internet:
        $ weave-pairing --mobile node01 --device node02 --server service --woca_server service

    command to test pairing using the default Weave ServiceProvisioning node-id and WOCA node-id over the internet with CASE session:
        $ weave-pairing --mobile node01 --device node02 --server service --woca_server service --case --case_cert_path path --case_key_path path

    command to test pairing using a custom Weave ServiceProvisioning server and WOCA server over the internet:
        $ weave-pairing --mobile node01 --device node02 --server <ip address> --woca_server <ip address>

    return:
        0   success
        1   failure

    """

    def __init__(self, opts=options):
        HappyNode.__init__(self)
        HappyNetwork.__init__(self)
        WeaveTest.__init__(self)
        self.__dict__.update(opts)

    def __pre_check(self):
        device_node_id = None
        # Set the produce resource that mock-device is paired to
        resourceDictionaries = self.getResourceIds()
        resourceIndexList = os.environ.get("RESOURCE_IDS", "thd1").split(" ")
        self.resources = [resourceDictionaries[resourceIndex]
                          for resourceIndex in resourceIndexList]

        # Check if Weave Pairing device node is given.
        if self.devices is None:
            emsg = "Missing name or address of the Weave Pairing device node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Pairing mobile node is given.
        if self.mobile is None:
            emsg = "Missing name or address of the Weave Pairing mobile node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Pairing server info is given.
        if self.server is None:
            emsg = "Missing name or address of the Weave Pairing server node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Operational CA server info is given.
        if self.woca_server is None:
            emsg = "Missing name or address of the Weave Operational CA server node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Make sure that fabric was created
        if self.getFabricId() == None:
            emsg = "Weave Fabric has not been created yet."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Check if Weave Pairing mobile node exists.
        if self._nodeExists(self.mobile):
            self.mobile_node_id = self.mobile

        # Check if mobile is provided in a form of IP address
        if IP.isIpAddress(self.mobile):
            self.mobile_node_id = self.getNodeIdFromAddress(self.mobile)

        if self.mobile_node_id is None:
            emsg = "Unknown identity of the mobile node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        # Find out whether to use a local mock server or a server
        # reachable over the internet
        if self._nodeExists(self.server):
            self.server_node_id = self.server
            self.server_ip = self.getNodeWeaveIPAddress(self.server_node_id)
            self.server_weave_id = self.getWeaveNodeID(self.server_node_id)
        elif IP.isIpAddress(self.server):
            self.server_ip = self.server
            self.server_weave_id = self.IPv6toWeaveId(self.server)
        elif IP.isDomainName(self.server) or self.server == "service":
            self.server_ip = self.getServiceWeaveIPAddress("ServiceProvisioning")
            self.server_weave_id = self.IPv6toWeaveId(self.server_ip)

        if self._nodeExists(self.woca_server):
            self.woca_node_id = self.woca_server
            self.woca_ip = self.getNodeWeaveIPAddress(self.woca_node_id)
            self.woca_weave_id = self.getWeaveNodeID(self.woca_node_id)
        elif IP.isIpAddress(self.woca_server):
            self.woca_ip = self.woca_server
            self.woca_weave_id = self.IPv6toWeaveId(self.woca_ip)
        elif IP.isDomainName(self.woca_server) or self.woca_server == "service":
            self.woca_ip = self.getServiceWeaveIPAddress("WOCA")
            self.woca_weave_id = self.IPv6toWeaveId(self.woca_ip)

        if self.tun_server is not None:
            if self._nodeExists(self.tun_server):
                self.tun_node_id = self.tun_server
                self.tun_ip = self.getNodeWeaveIPAddress(self.tun_node_id)
                self.tun_weave_id = self.getWeaveNodeID(self.tun_node_id)
            elif IP.isIpAddress(self.tun_server):
                self.tun_ip = self.tun_server
                self.tun_weave_id = self.IPv6toWeaveId(self.tun_ip)
            elif IP.isDomainName(self.tun_server) or self.tun_server == "service":
                self.tun_ip = self.getServiceWeaveIPAddress("Tunnel")
                self.tun_weave_id = self.IPv6toWeaveId(self.tun_ip)

            if self.tun_ip is None:
                emsg = "Could not find IP address of the Tunnel server."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            if self.tun_weave_id is None:
                emsg = "Could not find Weave node ID of the Tunnel server."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            # Check if service node was given in the environment
            if not self.service and not self.service_dir:
                if "weave_service_address" in os.environ.keys():
                    self.service = os.environ['weave_service_address']
                    emsg = "Found weave_service_address %s." % (self.service)
                    self.logger.debug("[localhost] Weave: %s" % (emsg))

            if not self.service and self.service_dir:
                if self.service_dir_server:
#                    self.skip_service_end = True
                    self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)
                else:
                    if "weave_service_address" in os.environ.keys():
#                        self.skip_service_end = True
                        if "unstable" not in os.environ["weave_service_address"]:
                            self.service_dir_server = os.environ["weave_service_address"]
                        else:
                            self.customized_tunnel_port = 20895
                            self.service_dir_server = os.environ["weave_service_address"] + ":%d" % self.customized_tunnel_port
                        self.logger.debug("[localhost] WeaveTunnelStart against tier %s." % self.service_dir_server)
                    else:
                        # Check if service node was given
                        emsg = "Service node id (or IP address or service directory) not specified."
                        self.logger.error("[localhost] WeaveTunnelStart: %s" % (emsg))
                        self.exit()

        self.mobile_ip = self.getNodeWeaveIPAddress(self.mobile_node_id)

        self.mobile_weave_id = self.getWeaveNodeID(self.mobile_node_id)

        for device, resource in zip(self.devices, self.resources):
            # Check if Weave Pairing device node exists.
            # TODO: fix
            device = "BorderRouter"
            if self._nodeExists(device):
                device_node_id = device
            # Check if device is provided in a form of IP address
            if IP.isIpAddress(device):
                device_node_id = self.getNodeIdFromAddress(device)

            if device_node_id is None:
                emsg = "Unknown identity of the device node."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            device_ip = self.getNodeWeaveIPAddress(device_node_id)
            if device_ip is None:
                emsg = "Could not find IP address of the device node."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            device_weave_id = self.getWeaveNodeID(device_node_id)
            if device_weave_id is None:
                emsg = "Could not find Weave node ID of the device node."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            device_serial_num = self.getSerialNum(device_node_id)
            if device_serial_num is None:
                emsg = "Could not find serial number of the device node."
                self.logger.error("[localhost] WeavePairing: %s" % (emsg))
                sys.exit(1)

            self.devices_info.append({'device': device,
                                      'device_node_id': device_node_id,
                                      'device_ip': device_ip,
                                      'device_weave_id': device_weave_id,
                                      'device_serial_num': device_serial_num,
                                      'device_process_tag': device + "_" + self.device_process_tag,
                                      'resource': resource})

        if self.mobile_ip is None:
            emsg = "Could not find IP address of the mobile node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        if self.mobile_weave_id is None:
            emsg = "Could not find Weave node ID of the mobile node."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        if self.server_ip is None:
            emsg = "Could not find IP address of the pairing server."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        if self.server_weave_id is None:
            emsg = "Could not find Weave node ID of the pairing server."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        if self.woca_ip is None:
            emsg = "Could not find IP address of the WOCA server."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

        if self.woca_weave_id is None:
            emsg = "Could not find Weave node ID of the WOCA server."
            self.logger.error("[localhost] WeavePairing: %s" % (emsg))
            sys.exit(1)

    def getResourceIds(self):
        resource_ids = {}

        resource_ids['np1'] = {}
        resource_ids['np1']['vendor_id'] = '9050'
        resource_ids['np1']['software_id'] = '1.0'
        resource_ids['np1']['product_id'] = '8'
        resource_ids['np1']['id'] = 'np1'

        resource_ids['nf1'] = {}
        resource_ids['nf1']['vendor_id'] = '9050'
        resource_ids['nf1']['software_id'] = '1.0'
        resource_ids['nf1']['product_id'] = '12'
        resource_ids['nf1']['id'] = 'nf1'

        resource_ids['nan1'] = {}
        resource_ids['nan1']['vendor_id'] = '9050'
        resource_ids['nan1']['software_id'] = '1.0'
        resource_ids['nan1']['product_id'] = '22'
        resource_ids['nan1']['id'] = 'nan1'

        resource_ids['ntl2'] = {}
        resource_ids['ntl2']['vendor_id'] = '9050'
        resource_ids['ntl2']['software_id'] = '1.0'
        resource_ids['ntl2']['product_id'] = '32'
        resource_ids['ntl2']['id'] = 'ntl2'

        resource_ids['ntb2'] = {}
        resource_ids['ntb2']['vendor_id'] = '9050'
        resource_ids['ntb2']['software_id'] = '1.0'
        resource_ids['ntb2']['product_id'] = '33'
        resource_ids['ntb2']['id'] = 'ntb2'

        resource_ids['gn1'] = {}
        resource_ids['gn1']['vendor_id'] = '57600'
        resource_ids['gn1']['software_id'] = '1.0'
        resource_ids['gn1']['product_id'] = '1'
        resource_ids['gn1']['id'] = 'gn1'

        resource_ids['gv1'] = {}
        resource_ids['gv1']['vendor_id'] = '57600'
        resource_ids['gv1']['software_id'] = '1.0'
        resource_ids['gv1']['product_id'] = '3'
        resource_ids['gv1']['id'] = 'gv1'

        resource_ids['gm1'] = {}
        resource_ids['gm1']['vendor_id'] = '57600'
        resource_ids['gm1']['software_id'] = '1.0'
        resource_ids['gm1']['product_id'] = '4'
        resource_ids['gm1']['id'] = 'gm1'

        resource_ids['gsl1'] = {}
        resource_ids['gsl1']['vendor_id'] = '57600'
        resource_ids['gsl1']['software_id'] = '1.0'
        resource_ids['gsl1']['product_id'] = '65024'
        resource_ids['gsl1']['id'] = 'gsl1'

        resource_ids['gsrbr1'] = {}
        resource_ids['gsrbr1']['vendor_id'] = '57600'
        resource_ids['gsrbr1']['software_id'] = '1.0'
        resource_ids['gsrbr1']['product_id'] = '65025'
        resource_ids['gsrbr1']['id'] = 'gsrbr1'

        resource_ids['thd1'] = {}
        resource_ids['thd1']['vendor_id'] = '9050'
        resource_ids['thd1']['software_id'] = '1.0'
        resource_ids['thd1']['product_id'] = '65534'
        resource_ids['thd1']['id'] = 'thd1'

        resource_ids['tst1'] = {}
        resource_ids['tst1']['vendor_id'] = '9050'
        resource_ids['tst1']['software_id'] = '1.0'
        resource_ids['tst1']['product_id'] = '65024'
        resource_ids['tst1']['id'] = 'tst1'

        return resource_ids

    def __start_server(self):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --node-addr " + self.server_ip
        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(
            self.server_node_id,
            cmd,
            self.server_process_tag,
            sync_on_output=self.ready_to_service_events_str)

    def __start_mobile_side(self, device_info, mobile_process_tag):
        os.environ['WEAVE_DEVICE_MGR_PATH'] = self.getWeaveDeviceMgrPath()
        os.environ['WEAVE_DEVICE_MGR_LIB_PATH'] = self.getWeaveDeviceMgrLibPath()
        cmd = "/usr/bin/env python " + \
            os.path.dirname(os.path.realpath(__file__)) + "/../lib/WeaveDeviceManager.py"
        if not cmd:
            return

        cmd += " " + device_info['device_ip'] + " " + device_info['device_weave_id']
        cmd += " --pairing-code TEST"

        if self.server is not None and not self.server_node_id:
            if not self.register_cmd:
                import ServiceAccountManager
                options = ServiceAccountManager.option()
                options["tier"] = self.tier
                options["username"] = self.username
                options["password"] = self.password

                registration = ServiceAccountManager.ServiceAccountManager(self.logger, options)
                self.register_cmd = registration.run()

            if self.register_cmd:
                cmd += self.register_cmd
            else:
                raise ValueError('register_cmd is empty')

        if self.tap:
            cmd += " --tap-device " + self.tap

        self.start_weave_process(self.mobile_node_id, cmd, mobile_process_tag, env=os.environ)

    def __start_device_side(self, device_info):
        cmd = self.getWeaveMockDevicePath()
        if not cmd:
            return

        cmd += " --node-addr " + device_info['device_ip'] + " --pairing-code TEST"
        cmd += " --wdm-resp-mutual-sub --test-case 10 --total-count 0 --wdm-update-timing NoSub "

        if self.server is not None:
            cmd += " --pairing-server " + self.server_ip \
                + " --wrm-pairing" \
                + " --vendor-id " + device_info['resource']['vendor_id'] \
                + " --software-version " + '"' + device_info['resource']['software_id'] + '"' \
                + " --product-id " + device_info['resource']['product_id'] \
                + " --suppress-ac" \
                + " --serial-num " + device_info['device_serial_num']

            if self.server_node_id is not None:
                # if the server is a local mock, we need to override the default endpoint id
                cmd += " --pairing-endpoint-id " + self.server_weave_id

        if self.woca_server is not None:
            cmd += " --woca-server " + self.woca_ip

            if self.woca_node_id is not None:
                # if the WOCA server is a local mock, we need to override the default endpoint id
                cmd += " --woca-endpoint-id " + self.woca_weave_id

        if self.tun_server is not None:
            cmd += " --tun-border-gw"
            cmd += " --tun-dest-node-id " + str(self.tun_weave_id)
            if self.service_dir:
                cmd += " --service-dir" + " --service-dir-server " + self.service_dir_server
            else:
                cmd += " --tun-connect-to " + self.tun_service_ipv4_addr

        if self.tap:
            cmd += " --tap-device " + self.tap

        if self.case:
            cmd += ' --case'
            if not self.case_cert_path == 'default' and self.woca_server is None:
                self.cert_file = self.case_cert_path if self.case_cert_path else os.path.join(self.main_conf['log_directory'], device_info['device_weave_id'] + '-cert.weave-b64')
                self.key_file = self.case_key_path if self.case_key_path else os.path.join(self.main_conf['log_directory'], device_info['device_weave_id'] + '-key.weave-b64')
                cmd += ' --node-cert ' + self.cert_file + ' --node-key ' + self.key_file

        cmd = self.runAsRoot(cmd)
        self.start_weave_process(
            device_info['device_node_id'],
            cmd,
            device_info['device_process_tag'],
            sync_on_output=self.ready_to_service_events_str)

    def __process_results(self, mobiles_output, devices_info):
        result_list = []
        for mobile_output in mobiles_output:
            if "Shutdown complete" in mobile_output:
                result_list.append(True)
            else:
                result_list.append(False)

        for device_info, result in zip(devices_info, result_list):
            print " %s weave-pairing from mobile %s (%s) to device %s (%s) : " % \
                ("Success for" if result else "Fail for", self.mobile_node_id,
                 self.mobile_ip, device_info['device_node_id'], device_info['device_ip'])

        return result_list

    def __wait_for_mobile(self, mobile_process_tag):
        self.wait_for_test_to_end(self.mobile_node_id, mobile_process_tag)

    def __stop_device_side(self, device_info):
        self.stop_weave_process(device_info['device_node_id'], device_info['device_process_tag'])

    def __stop_server_side(self):
        self.stop_weave_process(self.server_node_id, self.server_process_tag)

    def run(self):
        self.logger.debug("[localhost] WeavePairing: Run.")

        self.__pre_check()

        devices_output_data = []
        devices_strace_data = []
        mobiles_output_data = []
        mobiles_strace_data = []

        for device_info in self.devices_info:
            self.__start_device_side(device_info)
            # delay Execution
            time.sleep(0.5)

            emsg = "WeavePairing %s should be running." % (device_info['device_process_tag'])
            self.logger.debug("[%s] WeavePairing: %s" % (device_info['device_node_id'], emsg))

            if self.server_node_id:
                self.__start_server()
            mobile_process_tag = self.mobile_process_tag + device_info['device']
            self.__start_mobile_side(device_info, mobile_process_tag)
            self.__wait_for_mobile(mobile_process_tag)

            mobile_output_value, mobile_output_data = \
                self.get_test_output(self.mobile_node_id, mobile_process_tag, True)
            mobile_strace_value, mobile_strace_data = \
                self.get_test_strace(self.mobile_node_id, mobile_process_tag, True)

            self.__stop_device_side(device_info)

            device_output_value, device_output_data = \
                self.get_test_output(
                    device_info['device_node_id'],
                    device_info['device_process_tag'],
                    True)
            device_strace_value, device_strace_data = \
                self.get_test_strace(
                    device_info['device_node_id'],
                    device_info['device_process_tag'],
                    True)

            devices_output_data.append(device_output_data)
            devices_strace_data.append(device_strace_data)
            mobiles_output_data.append(mobile_output_data)
            mobiles_strace_data.append(mobile_strace_data)
            # delay execution
            time.sleep(3)
        server_output_value = None
        server_output_data = None
        server_strace_value = None
        server_strace_data = None

        if self.server_node_id:
            self.__stop_server_side()

            server_output_value, server_output_data = \
                self.get_test_output(self.server_node_id, self.server_process_tag, True)
            server_strace_value, server_strace_data = \
                self.get_test_strace(self.server_node_id, self.server_process_tag, True)

        result_list = self.__process_results(mobiles_output_data, self.devices_info)

        data = {}
        data["devices_output"] = devices_output_data
        data["devices_strace"] = devices_strace_data
        data["mobiles_output"] = mobiles_output_data
        data["mobiles_strace"] = mobiles_strace_data
        data["server_output"] = server_output_data
        data["server_strace"] = server_strace_data
        data["devices_info"] = self.devices_info

        self.logger.debug("[localhost] WeavePairing: Done.")
        return ReturnMsg(result_list, data)

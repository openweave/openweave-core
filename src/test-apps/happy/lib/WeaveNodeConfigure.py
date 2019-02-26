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
#       Implements WeaveNodeConfigure class which is used to configure
#       Weave parameters for a Happy node.
#

import os
import glob
import re
import random
import json
import uuid

from happy.ReturnMsg import ReturnMsg
from happy.Utils import *
from happy.utils.IP import IP
import happy.HappyNodeAddress as HappyNodeAddress

from Weave import Weave
from WeaveState import WeaveState
import WeaveCerts

options = {}
options['quiet'] = True
options['node_name'] = None
options['weave_node_id'] = None
options['weave_certificate'] = ""
options['private_key'] = ""
options['pairing_code'] = None
options['delete'] = False
options['customized_eui64'] = None


def option():
    return options.copy()


class WeaveNodeConfigure():
    """
    Configures Weave-related parameters for Happy nodes.

    weave-node-configure [-h --help] [-d --delete] [-w --weave-node-id <WEAVE_ID>]
                         [-c --weave-certificate </PATH/TO/WEAVE_CERITIFICATE>]
                         [-k --private-key </PATH/TO/PRIVATE_KEY>]
                         [-p --pairing_code <PAIRING_CODE>] <NODE_NAME>

        -w --weave-node-id      The EUI64 to use as a Weave Node ID, derived from a
                                node's MAC address. Omit to generate a random Weave
                                Node ID.
        -c --weave-certificate  Optional. Path to a Weave certificate. A dummy Weave
                                certificate is used in test scripts as needed.
        -k --private-key        Optional. Path to a Weave private key. A dummy Weave
                                private key is used in test scripts as needed.
        -p --pairing-code       Weave pairing code, a six character alphanumeric
                                string. Omit to generate a random pairing code.
        <NODE_NAME>             Optional. Node to configure for Weave. Find using
                                happy-node-list or happy-state. Omit to configure
                                all Weave-capable nodes in the topology.

    Examples:
    $ weave-node-configure
        Configures all Weave-capable nodes with randomly-generated parameters.

    $ weave-node-configure -w 18B4300000000001 BorderRouter
        Configures the BorderRouter node with a Weave Node ID of 18B4300000000001.

    $ weave-node-configure -w 18B4300000000001 -c /tmp/node.cert -k /tmp/node.key -p ABC123 BorderRouter
        Configures the BorderRouter node with all Weave-related parameters.

    $ weave-node-configure -d
        Deletes all Weave-related parameters from all Weave nodes in a Happy
        topology.

    $ weave-node-configure --delete BorderRouter
        Deletes all Weave-related parameters from the BorderRouter node.

    return:
        0   success
        1   fail
    """

    def __init__(self, opts=options):
        # TODO: this should instantiate a WeaveState object instead
        self.weave_state = Weave()
        self.logger = self.weave_state.logger

        emsg = "Starting WeaveNodeConfigure with options: %s" % opts
        self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)

        self.quiet = opts['quiet']
        self.delete = opts['delete']

        self.node_name = opts.get('node_name', "")
        if self.node_name:
            self.node_params = {}
            self.node_params['weave_node_id'] = opts['weave_node_id']
            self.node_params['weave_certificate'] = opts['weave_certificate']
            self.node_params['private_key'] = opts['private_key']
            self.node_params['pairing_code'] = opts['pairing_code']
            self.node_params['customized_eui64'] = opts['customized_eui64']

            # if weave node id is empty, but customized_eui64 is present;
            # use latter to generate a weave node id
            if not self.node_params['weave_node_id'] and self.node_params['customized_eui64']:
                self.node_params['weave_node_id'] = self.weave_state.EUI64toWeaveId(self.node_params['customized_eui64'])

        o = WeaveCerts.option()
        o['weave_cert_path'] = self.weave_state.getWeaveCertPath()
        self.weave_certs = WeaveCerts.WeaveCerts(o)
        self.weave_device_certs = self.weave_certs.getDeviceCerts()

    def __pre_check(self):
        if (self.node_params['weave_certificate'] or
                self.node_params['private_key'] or
                self.node_params['pairing_code']):
            if not (self.node_params['weave_node_id'] and
                    self.node_name):
                emsg = "weave-node-id and node_name must be specified to use cert/key/pairing-code"
                self.logger.error("[localhost] WeaveNodeConfigure: %s" % (emsg))
                self.weave_state.exit()

    def __generate_random_weave_node_id(self):
        """
        generate random 64-bit int, but make sure the 57th bit is 0 (which
        is the convention used for a 'local' weave node id.
        """
        return format(uuid.uuid1().int >> 64 & ~pow(2, 57), 'x')

    def __get_available_weave_node_id(self):
        """
        Picks a weave_node_id from the test pool that is not already present in
        WeaveState. If none is found, generates a random weave_node_id.
        """
        ids_in_use = self.weave_state.getWeaveNodeIds()
        ids_with_certs = self.weave_device_certs.keys()
        available_ids = list(set(ids_with_certs) - set(ids_in_use))

        if available_ids:
            return random.choice(available_ids)
        else:
            emsg = "__get_available_weave_node_id: no weave_id with certs available, generating random weave_id"
            self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)

            while (True):
                id = self.__generate_random_weave_node_id()
                if id not in self.weave_state.getWeaveNodeIds():
                    return id

    def __get_weave_params(self, node_name=None, node_params={}):
        """
        Returns weave params for a node. If weave_node_id is empty, it will
        pick one at random from a list of test weave node ids.
        """
        weave_node_id = node_params.get('weave_node_id', None)
        cert = node_params.get('weave_certificate', None)
        key = node_params.get('private_key', None)
        pairing_code = node_params.get('pairing_code', None)

        if weave_node_id:
            # bail if it is already in use by a different node
            nodes = self.weave_state.getWeaveNodeRecord()
            for node in nodes.keys():
                if node == node_name:
                    continue
                if nodes[node].get("weave_node_id", None) == weave_node_id:
                    emsg = "weave_node_id (%s) already in use." % weave_node_id
                    self.logger.error("[localhost] WeaveNodeConfigure: %s" % emsg)
                    self.weave_state.exit()
        else:
            weave_node_id = self.__get_available_weave_node_id()

        if not (cert and key):
            # check if a cert/key are available and use them
            node_cert = self.weave_device_certs.get(weave_node_id, None)
            if node_cert:
                cert = self.weave_device_certs[weave_node_id].get('cert', None)
                key = self.weave_device_certs[weave_node_id].get('key', None)
                # NOTE: self.weave_device_certs[weave_node_id]['cert-256'] is also available
            else:
                emsg = "WARNING: could not find weave cert/key for id=%s" % (weave_node_id)
                self.weave_state.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)
                # while we don't sys.exit() at this point, note that tests
                # that require cert/key will fail if they are not configured.

        params = {}
        params['weave_node_id'] = weave_node_id
        params['eui64'] = self.weave_state.WeaveIdtoEUI64(weave_node_id)
        params['iid'] = IP.EUI64toIID(params['eui64'])

        if cert and key:
            # these keys are only written if they are available
            params['weave_certificate'] = cert
            params['private_key'] = key

        params['pairing_code'] = self.__get_pairing_code(pairing_code)

        return params

    def __get_pairing_code(self, code=None):
        if code:
            return code
        else:
            # return a random 6-digit pairing code
            X = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
            return ''.join([random.choice(X) for i in range(6)])

    def __check_weave_params(self, node_params):
        """
        Checks if cert/key files exist and are readable
        """

        if not node_params:
            return False

        if not node_params.get('weave_node_id', None):
            return False

        if 'weave_certificate' in node_params:
            expanded_path = os.path.expandvars(node_params['weave_certificate'])
            if not os.path.isfile(expanded_path):
                return False

        if 'private_key' in node_params:
            expanded_path = os.path.expandvars(node_params['private_key'])
            if not os.path.isfile(expanded_path):
                return False

        return True

    def __configure_node(self, node_name, weave_params=None):
        """
        Writes weave config to WeaveState. Make sure weave_params is well-formed
        or be sure to call __get_weave_params() to initialize it first.
        """

        if not node_name:
            emsg = "node_name is empty, can't configure weave params"
            self.logger.error("[localhost] WeaveNodeConfigure: %s" % (emsg))
            return False

        emsg = "configuring weave params for node: %s" % (node_name)
        self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)

        if weave_params and not self.__check_weave_params(weave_params):
            emsg = "Bailing because weave params [%s] seem off for [%s]" \
                    % (weave_params, node_name)
            self.logger.error("[localhost] WeaveNodeConfigure: %s" % (emsg))
            self.weave_state.exit()

        emsg = "setting weave node (%s, %s)" % (node_name, weave_params)
        self.logger.debug("[localhost] WeaveNodeConfigure: %s" % (emsg))
        self.weave_state.setWeaveNode(node_name, weave_params)
        self.weave_state.writeState()

    def __add_node_address(self, node):
        if not node:
            return

        for interface_id in self.weave_state.getNodeInterfaceIds(node):
            interface_type = self.weave_state.getNodeInterfaceType(interface_id, node)
            subnet = self.weave_state.typeToWeaveSubnet(interface_type)
            if subnet is None:
                continue
            subnet_hex = "%04x" % (subnet)

            node_record = self.weave_state.getNodeWeaveRecord(node)

            iid = node_record['iid']

            fabric_id = self.weave_state.getFabricId()
            global_prefix = self.weave_state.getWeaveGlobalPrefix(fabric_id)
            global_prefix_addr, _ = global_prefix.split("::")

            emsg = "node: %s, global_prefix_addr: %s, subnet_hex: %s, iid: %s" % \
                (node, global_prefix_addr, subnet_hex, iid)
            self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)

            addr = global_prefix_addr + ":" + subnet_hex + ":" + iid

            self.__add_node_weave_fabric_address(node, interface_id, addr)

    def __add_nodes_addresses(self):
        for node in self.getWeaveNodes():
            self.__add_node_address(node)

    def __add_node_weave_fabric_address(self, node, interface_id, addr):
        if addr in self.weave_state.getNodeInterfaceAddresses(interface_id, node):
            emsg = "virtual node %s already has weave-preifx addresses %s." % (node, addr)
            self.logger.warning("[localhost] WeaveFabricAdd: %s" % (emsg))
            return

        options = HappyNodeAddress.option()
        options["quiet"] = self.quiet
        options["node_id"] = node
        options["interface"] = interface_id
        options["address"] = addr
        options["add"] = True

        emsg = "calling HappyNodeAddress with options: %s" % (json.dumps(options, indent=4))
        self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)

        addAddr = HappyNodeAddress.HappyNodeAddress(options)
        addAddr.run()

        self.weave_state.readState()

    def __init_node(self, node, node_params={}):
        if not node:
            emsg = "no node name specified, can't initialize"
            self.logger.error("[localhost] WeaveNodeConfigure: %s" % emsg)
            return

        if self.weave_state.getNodeType(node) in ['ap', 'service']:
            emsg = "ignoring initialization for nodes types 'ap' and 'service'"
            self.logger.debug("[localhost] WeaveNodeConfigure: %s" % emsg)
            return

        weave_params = self.__get_weave_params(node, node_params)
        self.__configure_node(node, weave_params)
        self.__add_node_address(node)

    def __init_nodes(self):
        """
        configure weave params for all nodes
        """
        for node in self.weave_state.getNodeIds():
            self.__init_node(node)

    def __delete_node(self, node):
        if not node:
            return

        emsg = "deleting node: %s" % node
        self.logger.debug("[localhost] WeaveStateUnload: %s" % emsg)

        self.weave_state.removeWeaveNode(node)

    def __delete_nodes(self):
        """
        delete all weave nodes
        """
        for node in self.weave_state.getWeaveNodes():
            self.__delete_node(node)

    def __post_check(self):
        pass

    def run(self):
        with self.weave_state.getStateLockManager():

            if self.delete:
                if self.node_name:
                    self.__delete_node(self.node_name)
                else:
                    self.__delete_nodes()
            else:
                if self.node_name:
                    self.__init_node(self.node_name, self.node_params)
                else:
                    self.__init_nodes()

            self.weave_state.writeState()
        self.__post_check()
        return ReturnMsg(0)

if __name__ == '__main__':

    print "\nTEST01: no node_name, configure all nodes"
    opts = option()
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST02: node_name: BorderRouter. Random weave_node_id assigned."
    opts = option()
    opts['node_name'] = 'BorderRouter'
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST03: node_name: BorderRouter. weave_node_id: 18B4300000000003. certs: None"
    opts = option()
    opts['node_name'] = 'BorderRouter'
    opts['weave_node_id'] = '18B4300000000003'
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST04: node_name: ThreadNode. weave_node_id: 18B430000000111. certs don't exist."
    opts = option()
    opts['node_name'] = 'ThreadNode'
    opts['weave_node_id'] = '18B4300000000111'
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST05: node_name: BorderRouter, delete=True"
    opts = option()
    opts['node_name'] = 'BorderRouter'
    opts['delete'] = True
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST06: delete=True, deletes all nodes"
    opts = option()
    opts['delete'] = True
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST07: node_name: BorderRouter, customized_eui64=18-B4-30-00-00-00-00-05"
    opts = option()
    opts['node_name'] = 'BorderRouter'
    opts['customized_eui64'] = '18-B4-30-00-00-00-00-05'
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

    print "\nTEST08: node_name: BorderRouter, customized_eui64=18-B4-30-00-AA-00-00-05"
    opts = option()
    opts['node_name'] = 'BorderRouter'
    opts['customized_eui64'] = '18-B4-30-00-AA-00-00-05'
    wnc = WeaveNodeConfigure(opts)
    wnc.run()
    print json.dumps(wnc.weave_state.getWeaveNodeRecord(), indent=4)

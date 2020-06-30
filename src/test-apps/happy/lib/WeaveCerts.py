#!/usr/bin/env python3

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
#       Implements WeaveCerts class which is used to manage Weave certs
#       and private keys for test Weave devices and service endpoints.
#

from __future__ import absolute_import
from __future__ import print_function
import os
import glob
import re
import json
import sys

options = {}
options['quiet'] = True
options['weave_cert_path'] = ""
options['weave_certs_dir'] = "certs/development"

options['devices_path'] = "/device"
options['devices_regex'] = "test-dev-(\w*)-([\w-]*).*weave"

options['service_endpoint_path'] = "/service-endpoint"
options['service_endpoint_regex'] = ".*endpoint-(.*).weave"


def option():
    return options.copy()


class WeaveCerts():
    """
    Acts as a store for Weave certs/keys and stores in the following format:

    certs = {
                'devices': {
                    '18B4300000000008': {
                        'cert': '/path/to/cert',
                        'key' : '/path/to/key',
                        'cert-256' : /path/to/cert-256'
                    },
                    '18B4300000000009': {
                        'cert': '/path/to/cert',
                        'key' : '/path/to/key',
                        'cert-256' : /path/to/cert-256'
                    }
                }
                'service_endpoint' {
                    'tunnel': {
                        'cert': '/path/to/cert',
                        'key' : '/path/to/key',
                        'cert-256' : /path/to/cert-256'
                    },
                    'log-upload': {
                        'cert': '/path/to/cert',
                        'key' : '/path/to/key',
                        'cert-256' : /path/to/cert-256'
                    }
                }
            }

    For convenience, separate dictionaries for each of the following is also
    available: (devices, service_endpoint)

    They can be accessed using class variables as follows:
        device_certs['18B4300000000008']['cert']
        service_endpoint_certs['tunnel']['cert']
    """

    def __init__(self, opts=options):
        self.quiet = opts['quiet']

        self.cert_path = opts['weave_cert_path']
        if not self.__check_cert_path():
            # print "invalid weave_cert_path: %s. [ignoring; no certs will be available; CASE tests might fail]" %self.cert_path
            self.cert_path = None

        self.certs = {}

        if self.cert_path:
            self.device_cert_path = self.cert_path + opts["devices_path"]
            self.service_endpoint_cert_path = self.cert_path + opts["service_endpoint_path"]
        else:
            self.device_cert_path = None
            self.service_endpoint_cert_path = None

        self.devices_regex = options["devices_regex"]
        self.service_endpoint_regex = options["service_endpoint_regex"]

        self.device_certs = {}
        self.service_endpoint_certs = {}

        self.loadCerts()

    def __check_cert_path(self):
        if self.cert_path:
            expanded_path = os.path.expandvars(self.cert_path)
            return os.path.isdir(expanded_path)

        return False

    def __fetch_certs_from_dir(self, path, fname_regex):
        certs = {}
        fname_regex = self.service_endpoint_regex
        pattern = re.compile(fname_regex)

        expanded_path = os.path.expandvars(path)
        for fname in [x for x in os.listdir(expanded_path) if re.search(fname_regex, x)]:
            match = pattern.match(fname)

            # match.group() = 'nest-weave-dev-tunnel-endpoint-cert.weave'
            abs_path = path + "/" + match.group()

            # match.groups() = ('cert',)
            file_type = match.groups()[0]

            certs[file_type] = abs_path
        return certs

    def __fetch_certs_from_nested_dir(self, path, fname_regex):
        """
        Load certs from 2-level dir structure and return dictionary.
        Example: for the service-endpoint dir, it returns the foll. dict::

        {
          "log-upload": {
            "cert": "/path/to/weave/certs/development/service-endpoint/log-upload/nest-weave-dev-log-upload-endpoint-cert.weave",
            "key": "/path/to/weave/certs/development/service-endpoint/log-upload/nest-weave-dev-log-upload-endpoint-key.weave",
            "cert-256": "/path/to/weave/certs/development/service-endpoint/log-upload/nest-weave-dev-log-upload-endpoint-cert-256.weave"
          },
          "tunnel": {
            "cert": "/path/to/weave/certs/development/service-endpoint/tunnel/nest-weave-dev-tunnel-endpoint-cert.weave",
            "key": "/path/to/weave/certs/development/service-endpoint/tunnel/nest-weave-dev-tunnel-endpoint-key.weave"
            "cert-256": "/path/to/weave/certs/development/service-endpoint/tunnel/nest-weave-dev-tunnel-endpoint-cert-256.weave",
          },
          [..]
        }
        """
        certs = {}
        fname_regex = self.service_endpoint_regex
        pattern = re.compile(fname_regex)

        expanded_path = os.path.expandvars(path)
        for f in os.listdir(expanded_path):
            subdir = path + "/" + f
            expanded_subdir = os.path.expandvars(subdir)
            if os.path.isdir(expanded_subdir):
                # retaining any variables as is in 'path' and 'subdir'
                certs[f] = self.__fetch_certs_from_dir(subdir, fname_regex)
        return certs

    def loadDeviceCerts(self):
        """
        searches device_cert_path for all available cert/key pairs and returns
        the following:

        {
            'id1': {'cert': '/path/to/cert1', 'key': '/path/to/key1'},
            'id2': {'cert': '/path/to/cert2', 'key': '/path/to/key2'},
            [..]
        }
        """
        # TODO: consider grouping certs in the devices dir by weave_node_id,
        #      that way, we can then re-use the above functions to fetch
        #      certs. Also, it will clean up the devices dir some.

        if not self.cert_path:
            # print "WeaveCerts: loadDeviceCerts: self.cert_path is empty [continuing]"
            return

        expanded_path = os.path.expandvars(self.device_cert_path)
        if not os.path.isdir(expanded_path):
            emsg = "device_cert_path (%s) is invalid." % self.device_cert_path
            raise ValueError(emsg)

        self.device_certs = {}
        fname_regex = self.devices_regex
        pattern = re.compile(".*" + fname_regex)

        for f in [x for x in os.listdir(expanded_path) if re.search(fname_regex, x)]:
            match = pattern.match(f)
            if not match:
                continue

            abs_path = self.device_cert_path + "/" + match.group()

            id = match.groups()[0]
            file_type = match.groups()[1]

            if id not in list(self.device_certs.keys()):
                self.device_certs[id] = {}
            self.device_certs[id][file_type] = abs_path

    def loadServiceEndpointCerts(self):
        """
        searches se_cert_path for all service endpoints certs and returns the foll:
        {
            "tunnel": { "cert": "/path/to/cert", key: "/path/to/key" },
            "software-update": { "cert: "/path/to/cert", key: "/path/to/key" },
            [..]
        }

        """
        if not self.cert_path:
            return

        se_cert_path = os.path.expandvars(self.service_endpoint_cert_path)
        if not os.path.isdir(se_cert_path):
            emsg = "weave service endpoint cert path (%s) is invalid." % se_cert_path
            raise ValueError(emsg)

        self.service_endpoint_certs = {}
        self.service_endpoint_certs = self.__fetch_certs_from_nested_dir(
                                            self.service_endpoint_cert_path,
                                            self.service_endpoint_regex)

    def loadCerts(self):
        self.loadDeviceCerts()
        self.loadServiceEndpointCerts()

    def getServiceEndpointCerts(self, reload=False):
        if reload:
            self.loadServiceEndpointCerts()
        return self.service_endpoint_certs.copy()

    def getDeviceCerts(self, reload=False):
        if reload:
            self.loadDeviceCerts()
        return self.device_certs.copy()

    def getCerts(self):
        self.certs = {}
        self.certs["devices"] = self.device_certs.copy()
        self.certs["service_endpoint"] = self.service_endpoint_certs.copy()
        return self.certs

if __name__ == '__main__':
    # sample code
    import random

    opts = option()
    opts["weave_cert_path"] = "${WEAVE_HOME}/certs/development"

    weave_certs = WeaveCerts(opts)

    device_certs = weave_certs.getDeviceCerts()
    if device_certs:
        node_id = random.choice(list(device_certs))
        node_cert = device_certs[node_id]
        print("id: %s\ncert: %s\nkey: %s" % (node_id, node_cert['cert'], node_cert['key']))
        print(node_id, ": ", json.dumps(node_cert, indent=4))

    se_certs = weave_certs.getServiceEndpointCerts()
    if se_certs:
        tunnel_cert = se_certs["tunnel"]
        print("tunnel: ", json.dumps(tunnel_cert, indent=4))

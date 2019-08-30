#!/usr/bin/env python
#
#       Copyright (c) 2015-2017  Nest Labs, Inc.
#       All rights reserved.
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
#       Calls Weave Tunnel between nodes.
#

import itertools
import os
import sys
import unittest
import set_test_path
import time
import subprocess

from happy.Utils import *
import happy.HappyNodeList
import WeaveStateLoad
import WeaveStateUnload
import WeaveTunnelStart
import WeaveTunnelStop
import WeaveUtilities

class test_weave_tunnel_01(unittest.TestCase):
    def setUp(self):
        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.use_lwip = True
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_on_tap_ap_service.sh"
            # tap interface, ipv4 gateway and node addr should be provided if device is tap device
            # both BorderRouter and cloud node are tap devices here
            self.BR_tap = "wlan0"
            self.cloud_tap = "eth0"
        else:
            self.use_lwip = False
            topology_shell_script = os.path.dirname(os.path.realpath(__file__)) + \
                "/../../../topologies/standalone/thread_wifi_ap_service.sh"
        output = subprocess.call([topology_shell_script])

        # Wait for a second to ensure that Weave ULA addresses passed dad
        # and are no longer tentative
        time.sleep(2)

    def tearDown(self):
        # cleaning up
        subprocess.call(["happy-state-delete"])

    def test_weave_tunnel(self):
        # topology has nodes: ThreadNode, BorderRouter, onhub and cloud
        # we run tunnel between BorderRouter and cloud

        # Start tunnel
        value, data = self.__start_tunnel_between("BorderRouter", "cloud")

        time.sleep(1)

        # Stop tunnel
        value, data = self.__stop_tunnel_between("BorderRouter", "cloud")

    def __start_tunnel_between(self, gateway, service):
        options = WeaveTunnelStart.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["service"] = service
        options["use_lwip"] = self.use_lwip
        if self.use_lwip:
            options["client_tap"] = self.BR_tap
            options["service_tap"] = self.cloud_tap

        weave_tunnel = WeaveTunnelStart.WeaveTunnelStart(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data

    def __stop_tunnel_between(self, gateway, service):
        options = WeaveTunnelStop.option()
        options["quiet"] = False
        options["border_gateway"] = gateway
        options["service"] = service

        weave_tunnel = WeaveTunnelStop.WeaveTunnelStop(options)
        ret = weave_tunnel.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()



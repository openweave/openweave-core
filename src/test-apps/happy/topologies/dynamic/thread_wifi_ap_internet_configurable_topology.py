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
#    @file thread_wifi_ap_internet_configurable_topology.
## Example of Thead, WiFi, and WAN networks, which have:
# - one thread-only device ThreadNode
# - one thread-wifi-cellular border_gateway device BorderRouter
# - one access-point router onhub
# - the onhub router is connected to the Internet
#

import getopt
import os
import set_test_path
from happy.Utils import *
import happy.HappyTopologyMgr
import lib.WeaveTopologyMgr
import sys
import uuid


default_quiet = False
default_tap = False


class thread_wifi_ap_internet_configurable_topology(object):
    """
    thread_wifi_ap_internet_configurable_topology provides topology creation and destroy functionality.

    python weave_wdm_next_test_service_topology [-h --help] [-q --quiet] [-a --action] [-f --fabric_id]
                                                [-e --customized_eui64_seed] [-t --tap] [-c --cellular]
                                                [-m --mobile] [-n --device_numbers]

    note: please pair weave node id in service before testing
    Example:
    $ python thread_wifi_ap_internet_configurable_topology.py --action="create" --fabric_id="00000" --customized_eui64_seed="18:B4:30:00:00:00:00:0" --cellular --mobile --enable_random_fabric --device_numbers=3 --initial_device_index=1
    $ python thread_wifi_ap_internet_configurable_topology.py --action="destroy" --fabric_id="00000" --customized_eui64_seed="18:B4:30:00:00:00:00:0" --cellular --mobile --enable_random_fabric --device_numbers=3 --initial_device_index=1

    fabric_id in test is a postfix  which is set as only 5 hex, for example, 00000, then random 4 hex digit would append with fabric_id, which is used to test load balancer of service
    customized_eui64_seed in test is set as only 15 hex, for example, 18:B4:30:00:00:00:00:0
    cellular is set when cellular topology is applied
    there are three devices per fabric
    initial_device_index is set as 1
    return:
        0    success
        1    fail
    """
    def __init__(self, quiet=False, fabric_id=None, device_numbers=None, customized_eui64_seed=None, tap=False, dns=None, cellular=False, mobile=False, initial_device_index=1, enable_random_fabric=False):
        if device_numbers is None:
            self.device_numbers = 1
        else:
            self.device_numbers = device_numbers
        self.quiet = quiet

        if enable_random_fabric:
            self.fabric_id = uuid.uuid4().hex[0:4] + fabric_id
        else:
            self.fabric_id = fabric_id

        self.customized_eui64_seed = customized_eui64_seed
        self.customized_eui64_BorderRouter = self.customized_eui64_seed + "0"
        self.tap = tap
        self.dns = dns
        self.cellular = cellular
        self.topology = happy.HappyTopologyMgr.HappyTopologyMgr()
        self.weave_topology = lib.WeaveTopologyMgr.WeaveTopologyMgr()
        self.mobile = mobile
        self.initial_device_index = initial_device_index


    def createTopology(self):
        ISPPrefix = self.getISPPrefix()

        self.topology.HappyNetworkAdd(network_id="HomeThread", type="thread", quiet=self.quiet)
        self.topology.HappyNetworkAddress(network_id="HomeThread", address="2001:db8:111:1::", quiet=self.quiet)
        self.topology.HappyNetworkAdd(network_id="HomeWiFi", type="wifi", quiet=self.quiet)
        self.topology.HappyNetworkAddress(network_id="HomeWiFi", address="2001:db8:222:2::", quiet=self.quiet)
        self.topology.HappyNetworkAddress(network_id="HomeWiFi", address="10.0.1.0", quiet=self.quiet)

        self.weave_topology.WeaveFabricAdd(fabric_id=self.fabric_id, quiet=self.quiet)

        for device_index in range(self.device_numbers):
            ThreadNodeId = "ThreadNode%s" % str(device_index + self.initial_device_index).zfill(2)
            customized_eui64_ThreadNode = self.customized_eui64_seed + "%s" % format(device_index + self.initial_device_index, 'x')
            self.topology.HappyNodeAdd(node_id=ThreadNodeId, quiet=self.quiet)
            self.topology.HappyNodeJoin(node_id=ThreadNodeId, network_id="HomeThread", customized_eui64=customized_eui64_ThreadNode,
                                        tap=self.tap, quiet=self.quiet)
            # set up ThreadNode weave params
            opts = {}
            opts['quiet'] = self.quiet
            opts['node_name'] = ThreadNodeId
            opts['customized_eui64'] = customized_eui64_ThreadNode.replace(':', '-')

            self.weave_topology.WeaveNodeConfigure(opts)


        self.topology.HappyNodeAdd(node_id="BorderRouter", quiet=self.quiet)
        self.topology.HappyNodeJoin(node_id="BorderRouter", network_id="HomeThread", customized_eui64=self.customized_eui64_BorderRouter, tap=self.tap, quiet=self.quiet)
        self.topology.HappyNodeJoin(node_id="BorderRouter", network_id="HomeWiFi", customized_eui64=self.customized_eui64_BorderRouter, tap= self.tap, quiet=self.quiet)

        self.topology.HappyNodeAdd(node_id="onhub", type="ap", quiet=self.quiet)
        self.topology.HappyNodeJoin(node_id="onhub", network_id="HomeWiFi", quiet=self.quiet)

        self.topology.HappyNetworkRoute(network_id="HomeThread", to="default", via="BorderRouter", quiet=self.quiet)

        if self.cellular:
            self.topology.HappyNetworkRoute(network_id="HomeWiFi", to="default", via="onhub", prefix="10.0.1.0", quiet=self.quiet, isp=ISPPrefix + "eth", seed=249)
        else:
            self.topology.HappyNetworkRoute(network_id="HomeWiFi", to="default", via="onhub", prefix="10.0.1.0", quiet=self.quiet)

        if self.mobile:
            self.topology.HappyNetworkAdd(network_id="TMobile", type="cellular", quiet=self.quiet)
            self.topology.HappyNetworkAddress(network_id="TMobile", address="2001:db8:333:3::", quiet=self.quiet)
            self.topology.HappyNodeAdd(node_id="mobile", quiet=self.quiet)
            self.topology.HappyNodeJoin(node_id="mobile", network_id="TMobile", quiet=self.quiet)
            self.topology.HappyNodeJoin(node_id="BorderRouter", network_id="TMobile", quiet=self.quiet)
            self.topology.HappyNetworkRoute(network_id="TMobile", to="default", via="BorderRouter", quiet=self.quiet)
            # set up mobile weave params
            opts = {}
            opts['quiet'] = self.quiet
            opts['node_name'] = "mobile"
            self.weave_topology.WeaveNodeConfigure(opts)

        self.topology.HappyNetworkRoute(network_id="HomeWiFi", to="default", via="onhub", prefix="2001:db8:222:2::", quiet=self.quiet)

        self.topology.HappyInternet(add=True, node_id="onhub", quiet=self.quiet, isp=ISPPrefix + "eth", seed="249")
        if self.cellular:
            self.topology.HappyInternet(add=True, node_id="BorderRouter", quiet=self.quiet, isp=ISPPrefix + "cell", seed="250")
        self.topology.HappyDNS(add=True, quiet=self.quiet, dns=self.dns)

        # set up BorderRouter weave params
        opts = {}
        opts['quiet'] = self.quiet
        opts['node_name'] = 'BorderRouter'
        opts['customized_eui64'] = self.customized_eui64_BorderRouter.replace(':', '-')
        self.weave_topology.WeaveNodeConfigure(opts)

        self.weave_topology.WeaveNetworkGateway(network_id="HomeThread", gateway="BorderRouter", quiet=self.quiet)


    def destroyTopology(self):
        self.topology.HappyDNS(dns=self.dns, delete=True, quiet=self.quiet)
        self.weave_topology.WeaveNetworkGateway(network_id="HomeThread", gateway="BorderRouter", delete=True, quiet=self.quiet)
        self.weave_topology.WeaveFabricDelete(fabric_id=self.fabric_id, quiet=self.quiet)
        self.topology.HappyStateDelete(quiet=self.quiet)

    def getISPPrefix(self):
        if "weave_service_address" in os.environ.keys():
            prefix = os.environ['weave_service_address'].split(".")[-3][0:3]
        else:
            prefix = "test"

        return prefix

if __name__ == "__main__":
    options = {"quiet":False, "tap": False, "cellular": False, "mobile": False, "enable_random_fabric": False}
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ha:qf:e:t:cmn:i:r",
            ["help", "action=", "quiet", "fabric_id=", "customized_eui64_seed=", "tap=", "cellular", "mobile", "device_numbers=", "initial_device_index=", "enable_random_fabric"])

    except getopt.GetoptError as err:
        print thread_wifi_ap_internet_configurable_topology.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-a", "--action"):
            options["action"] = a

        elif o in ("-f", "--fabric_id"):
            options["fabric_id"] = a

        elif o in ("-e", "--customized_eui64_seed"):
            options["customized_eui64_seed"] = a

        elif o in ("-t", "--tap"):
            options["tap"] = True

        elif o in ("-c", "--cellular"):
            options["cellular"] = True

        elif o in ("-m", "--mobile"):
            options["mobile"] = True

        elif o in ("-n", "--device_numbers"):
            options["device_numbers"] = a

        elif o in ("-i", "--initial_device_index"):
            options["initial_device_index"] = a

        elif o in ("-r", "--enable_random_fabric"):
            options["enable_random_fabric"] = True

        else:
            assert False, "unhandled option"

    if "action" not in options:
        print thread_wifi_ap_internet_configurable_topology.__doc__
        sys.exit(0)

    if options["action"] not in ["create", "destroy"]:
        print thread_wifi_ap_internet_configurable_topology.__doc__
        sys.exit(0)

    if ("fabric_id" in options) and ("customized_eui64_seed" in options):
        fabric_id = options["fabric_id"]
        customized_eui64_seed = options["customized_eui64_seed"]

    else:
        print thread_wifi_ap_internet_configurable_topology.__doc__
        sys.exit(0)

    if "device_numbers" in options:
        device_numbers = int(options["device_numbers"])
    else:
        device_numbers = 1

    if "initial_device_index" in options:
        initial_device_index = int(options["initial_device_index"])
    else:
        initial_device_index = 1

    topology = thread_wifi_ap_internet_configurable_topology(quiet=options["quiet"],
                                                             fabric_id=fabric_id,
                                                             customized_eui64_seed=customized_eui64_seed,
                                                             tap=options["tap"],
                                                             dns=["8.8.8.8", "172.16.255.1", "172.16.255.153", "172.16.255.53"],
                                                             cellular=options["cellular"],
                                                             device_numbers=device_numbers,
                                                             mobile=options["mobile"],
                                                             initial_device_index=initial_device_index,
                                                             enable_random_fabric = options["enable_random_fabric"])
    if options["action"] == "create":
        topology.createTopology()
    else:
        topology.destroyTopology()






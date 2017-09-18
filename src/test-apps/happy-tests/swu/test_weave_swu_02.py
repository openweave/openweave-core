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
#       Calls Weave SWU Test between two nodes.
#           server ===> client (image announce)
#           server <=== client (image request)
#           client ===> server (image response)
#

import os
import unittest
import set_test_path
import random
import shutil
import string

from happy.Utils import *
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveSWU as WeaveSWU
import plugin.WeaveTunnelStart as WeaveTunnelStart
import plugin.WeaveTunnelStop as WeaveTunnelStop
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_swu_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_thread_weave.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_thread_weave.json"

        self.show_strace = False

        # setting Mesh for thread test
        options = WeaveStateLoad.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        ret = setup_network.run()


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_swu(self):
        # topology has nodes: node01(server), node02(client) and node03

        # SWU
        data = self.__weave_swu_between("node01", "node02", 10)


    def __weave_swu_between(self, nodeA, nodeB, file_size):
        test_folder_path = "/tmp/happy_%08d" % (int(os.getpid()))
        test_folder = self.__create_test_folder(test_folder_path)

        test_file = self.__create_file(test_folder, file_size)

        options = WeaveSWU.option()
        options["quiet"] = False
        options["server"] = nodeA
        options["client"] = nodeB
        options["file_designator"] = test_file
        options["integrity_type"] = 1
        options["update_scheme"] = 3
        options["announceable"] = True
        options["tap"] = self.tap

        weave_swu = WeaveSWU.WeaveSWU(options)
        ret = weave_swu.run()

        value = ret.Value()
        data = ret.Data()
        self.__delete_test_folder(test_folder)

        return data


    def __create_test_folder(self, path):
        os.mkdir(path)
        return path


    def __delete_test_folder(self, path):
        shutil.rmtree(path)


    def __create_file(self, test_folder, size = 10):
        path = test_folder + "/test_file.txt"
        random_content = ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(size))

        with open(path, 'w+') as test_file:
            test_file.write(random_content)

        return path

if __name__ == "__main__":
    WeaveUtilities.run_unittest()



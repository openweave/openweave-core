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
#       Calls Weave BDX between nodes using fault injection to test failure handling
#

import filecmp
import itertools
import os
import random
import shutil
import string
import unittest
import set_test_path
import getopt
import sys

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveBDX as WeaveBDX
import plugin.WeaveUtilities as WeaveUtilities

gFaultopts = WeaveUtilities.FaultInjectionOptions()
gOptions = {"direction": None, "versions": None}
gDirections = ["download", "upload"]

class test_weave_bdx_faults_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if os.environ.get("WEAVE_SYSTEM_CONFIG_USE_LWIP") == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_thread_weave.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_thread_weave.json"

        self.strace = False
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


    def test_weave_bdx_download(self):
        self.test_num = 0
        if (gOptions["versions"] == None):
            bdx_version = [0, 1]
            pairs_of_versions = list(itertools.product(bdx_version, bdx_version))
        else:
            pairs_of_versions = [map(int, gOptions["versions"].split("_"))]


        directions = [ gOptions["direction"] ]
        if (directions[0] == None):
            directions = gDirections

        file_sizes = [1000]

        num_failed_tests = 0
        failed_tests = []

        for file_size in file_sizes:
            for version in pairs_of_versions:
                for direction in directions:
                    # run the test once withtout faults to populate the fault injection counters
                    test_tag = "_" + direction + "_v" + str(version[0]) + "_v" + str(version[1]) + "_" + str(self.test_num)
                    failed = self.__weave_bdx(direction, file_size, version[0], version[1], num_iterations = 1, faults = {}, test_tag = test_tag)
                    if failed:
                        raise ValueError("The happy sequence failed")

                    for node in gFaultopts.nodes:
                        restart = True

                        fault_configs = gFaultopts.generate_fault_config_list(node, self.happy_seq_output_logs[node], restart)

                        for fault_config in fault_configs:
                            test_tag = "_" + direction + "_v" + str(version[0]) + "_v" + str(version[1]) + "_" + str(self.test_num) + "_" + node + "_" + fault_config
                            print test_tag
                            failed = self.__weave_bdx(direction, file_size, version[0], version[1], num_iterations = 3, faults = {node: fault_config}, test_tag = test_tag)
                            if failed:
                                num_failed_tests += 1
                                failed_tests.append(test_tag)

        print "executed %d cases" % self.test_num
        print "failed %d cases:" % num_failed_tests
        if num_failed_tests > 0:
            for failed_test in failed_tests:
                print "    " + failed_test
            raise ValueError("Fix it!")
                   

    def __weave_bdx(self, direction, file_size, server_version, client_version, num_iterations, faults = {}, test_tag = ""):

        test_folder_path = "/tmp/happy_%08d_%s_%03d" % (int(os.getpid()), direction, self.test_num)
        test_folder = self.__create_test_folder(test_folder_path)

        server_temp_path = self.__create_server_temp(test_folder)
        test_file = self.__create_file(test_folder, file_size)
        receive_path = self.__create_receive_folder(test_folder)

        options = WeaveBDX.option()
        options["plaid"] = "auto"
        options["quiet"] = False
        options["server"] = "node01" 
        options["client"] = "node02"
        options["tmp"] = server_temp_path
        options[direction] = test_file
        options["receive"] = receive_path
        options["tap"] = self.tap
        options["server_version"] = server_version
        options["client_version"] = client_version
        options["server_faults"] = faults.get('server')
        options["client_faults"] = faults.get('client')
        options["strace"] = self.strace
        options["test_tag"] = test_tag
        options["iterations"] = num_iterations

        weave_bdx = WeaveBDX.WeaveBDX(options)
        ret = weave_bdx.run()

        value = ret.Value()
        data = ret.Data()
        copy_success = self.__file_copied(test_file, receive_path)
        self.__delete_test_folder(test_folder)

        failed = self.__process_result("node01", "node02", value, data, direction, file_size, server_version, client_version, faults)

        if (len(faults) == 0):
            self.happy_seq_output_logs = {}
            self.happy_seq_output_logs['client'] = data["client_output"]
            self.happy_seq_output_logs['server'] = data["server_output"]

        self.test_num += 1

        return failed


    def __process_result(self, nodeA, nodeB, value, data, direction, file_size, server_version, client_version, faults):

        if direction == "download":
            print "bdx download of " + str(file_size) + "B from " + nodeA + "(BDX-v" + str(server_version) + ") to "\
                  + nodeB + "(BDX-v" + str(client_version) + ") ",
        else:
            print "bdx upload of " + str(file_size) + "B from " + nodeA + "(BDX-v" + str(server_version) + ") to "\
                  + nodeB + "(BDX-v" + str(client_version) + ") ",

        if faults.get('server') != None:
            print "server fault: " + faults['server'] + " "

        if faults.get('client') != None:
            print "client fault: " + faults['client'] + " "

        if value:
            print hgreen("Passed")
        else:
            print hred("Failed")

        failed = not value

        return failed



    def __file_copied(self, test_file_path, receive_path):
        file_name = os.path.basename(test_file_path)
        receive_file_path = receive_path + "/" + file_name

        if not os.path.exists(receive_file_path):
            print "A copy of a file does not exists"
            return False

        return filecmp.cmp(test_file_path, receive_file_path)


    def __create_test_folder(self, path):
        os.mkdir(path)
        return path


    def __delete_test_folder(self, path):
        shutil.rmtree(path)


    def __create_server_temp(self, test_folder):
        path = test_folder + "/server_tmp"
        os.mkdir(path)
        return path


    def __create_file(self, test_folder, size = 10):
        path = test_folder + "/test_file.txt"
        random_content = ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(size))

        with open(path, 'w+') as test_file:
            test_file.write(random_content)

        return path


    def __create_receive_folder(self, test_folder):
        path = test_folder + "/receive"
        os.mkdir(path)
        return path


if __name__ == "__main__":

    help_str = """usage: 
    --direction {" + ", ".join(gDirections) + "} --versions {0,1}_{0,1}"""

    help_str += gFaultopts.help_string

    longopts = ["help", "direction=", "versions="]
    longopts.extend(gFaultopts.getopt_config)

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h", longopts)

    except getopt.GetoptError as err:
        print help_str
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    opts = gFaultopts.process_opts(opts)

    for o, a in opts:
        if o in ("-h", "--help"):
            print help_str
            sys.exit(0)

        elif o in ("--versions"):
            gOptions["versions"] = a
        elif o in ("--direction"): 
            if a in gDirections:
                gOptions["direction"] = a
            else:
                print help_str
                sys.exit(1)

    sys.argv = [sys.argv[0]]

    WeaveUtilities.run_unittest()


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
#       This file calls Weave BDX-v1 test between nodes with offset parameter.
#

import filecmp
import itertools
import os
import random
import shutil
import string
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugin.WeaveBDX as WeaveBDX
import plugin.WeaveUtilities as WeaveUtilities

class test_weave_bdx_02(unittest.TestCase):
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

        self.file_size = 1000


    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_bdx_download(self):
        self.test_num = 0

        for total_size in [6]:
            for file_offset in [1]:
                copy_size = min(self.file_size - file_offset, total_size)
                self.__weave_bdx("download", file_offset, copy_size)

            copy_size = min(self.file_size, total_size)
            self.__weave_bdx("upload", 0, copy_size)


    def __weave_bdx(self, direction, file_offset, copy_size):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        if direction == "download":
            value, data = self.__run_bdx_download_between("node01", "node02", file_offset, copy_size)
        else:
            value, data = self.__run_bdx_upload_between("node01", "node02", file_offset, copy_size)

        self.__process_result("node01", "node02", value, data, direction, copy_size)
        self.test_num += 1


    def __process_result(self, nodeA, nodeB, value, data, direction, copy_size):

        if direction == "download":
            print "bdx download of " + str(copy_size) + "B from " + nodeA + " to " + nodeB + " ",
        else:
            print "bdx upload of " + str(copy_size) + "B from " + nodeB + " to " + nodeA + " ",

        if value:
            print hgreen("Passed")
        else:
            print hred("Failed")

        try:
            self.assertTrue(value == True, "File Copied: " + str(value))
        except AssertionError, e:
            print str(e)
            print "Captured experiment result:"

            print "Client Output: "
            for line in data["client_output"].split("\n"):
                print "\t" + line

            print "Server Output: "
            for line in data["server_output"].split("\n"):
                print "\t" + line

            if self.show_strace:
                print "Server Strace: "
                for line in data["server_strace"].split("\n"):
                    print "\t" + line

                print "Client Strace: "
                for line in data["client_strace"].split("\n"):
                    print "\t" + line

        if not value:
            raise ValueError("Weave BDX Failed")


    def __run_bdx_download_between(self, nodeA, nodeB, file_offset, copy_size):
        test_folder_path = "/tmp/happy_%08d_down_%03d" % (int(os.getpid()), self.test_num)
        test_folder = self.__create_test_folder(test_folder_path)

        server_temp_path = self.__create_server_temp(test_folder)
        test_file = self.__create_file(test_folder, self.file_size)
        receive_path = self.__create_receive_folder(test_folder)
        expected_file = self.__create_expected_content_file(test_folder, test_file, file_offset, copy_size)

        options = WeaveBDX.option()
        options["quiet"] = False
        options["server"] = nodeA
        options["client"] = nodeB
        options["tmp"] = server_temp_path
        options["download"] = test_file
        options["receive"] = receive_path
        options["offset"] = file_offset
        options["length"] = copy_size
        options["tap"] = self.tap
        options["server_version"] = 1
        options["client_version"] = 1

        weave_bdx = WeaveBDX.WeaveBDX(options)
        ret = weave_bdx.run()

        value = ret.Value()
        data = ret.Data()
        self.__delete_test_folder(test_folder)

        return value, data


    def __run_bdx_upload_between(self, nodeA, nodeB, file_offset, copy_size):
        test_folder_path = "/tmp/happy_%08d_up_%03d" % (int(os.getpid()), self.test_num)
        test_folder = self.__create_test_folder(test_folder_path)

        server_temp_path = self.__create_server_temp(test_folder)
        test_file = self.__create_file(test_folder, self.file_size)
        receive_path = self.__create_receive_folder(test_folder)
        expected_file = self.__create_expected_content_file(test_folder, test_file, file_offset, copy_size)

        options = WeaveBDX.option()
        options["quiet"] = False
        options["server"] = nodeA
        options["client"] = nodeB
        options["tmp"] = server_temp_path
        options["upload"] = test_file
        options["receive"] = receive_path
        options["offset"] = file_offset
        options["length"] = copy_size
        options["tap"] = self.tap
        options["server_version"] = 1
        options["client_version"] = 1

        weave_bdx = WeaveBDX.WeaveBDX(options)
        ret = weave_bdx.run()

        value = ret.Value()
        data = ret.Data()
        self.__delete_test_folder(test_folder)

        return value, data


    def __file_copied(self, expected_file_path, test_file_path, receive_path, file_offset, copy_size):
        file_name = os.path.basename(test_file_path)
        receive_file_path = receive_path + "/" + file_name

        if not os.path.exists(receive_file_path):
            print "A copy of a file does not exists"
            return False

        return filecmp.cmp(expected_file_path, receive_file_path)


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


    def __create_expected_content_file(self, test_folder, test_file, file_offset, copy_size):
        path = test_folder + "/expected_file.txt"

        with open(test_file, 'r') as content_source:
            content_source.seek(file_offset)
            content = content_source.read(copy_size)

        with open(path, 'w+') as expected_file:
            expected_file.write(content)

        return path


if __name__ == "__main__":
    WeaveUtilities.run_unittest()


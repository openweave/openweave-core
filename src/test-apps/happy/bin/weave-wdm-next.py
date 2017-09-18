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
#       A Happy command line utility that tests Weave Wdm Next among Weave nodes.
#
#       The command is executed by instantiating and running WeaveWdmNext class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveWdmNext as WeaveWdmNext

helpstring = \
    """
    weave-wdm-next [-h --help] [-q --quiet] [-o --origin <NAME>] [-s --server <NAME>] [-t --tap <TAP_INTERFACE>]
    [-w --wdm_option <wdm_option>] [-t --test_case <test_case>] [--test_client_case <case_id>] [--total_client_count <count>):
    [--final_client_status <status>][--timer_client_period <period>] [--enable_client_stop][--test_client_iterations <iterations>]
    [--test_client_delay <delay_time>] [--enable_client_flip][--save_client_perf][--test_server_case <case_id>]
    [--total_server_count <count>][--final_server_status <status>]
    [--timer_server_period] <period>[--enable_server_stop][--test_server_iterations <iterations>]
    [--test_server_delay <delay_time>][--enable_server_flip][--save_server_perf][--test_focus_client]
    [--case][--case_cert_path <path>][--case_key_path <path>][--group_enc][--group_enc_key_id <key>]

    return:
        True or False for test

    """

if __name__ == "__main__":
    options = WeaveWdmNext.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "ho:s:qt:w:",
            ["help", "origin=", "server=", "quiet", "tap=", "wdm_option=",
             "test_client_case=", "total_client_count=", "final_client_status=", "timer_client_period=",
             "enable_client_stop=", "test_client_iterations=", "test_client_delay=","enable_client_flip=",
             "save_client_perf="
             "test_server_case=", "total_server_count=", "final_server_status=", "timer_server_period=",
             "enable_server_stop=", "test_server_iterations=", "test_server_delay=", "enable_server_flip=",
             "save_server_perf=", "swap_role=", "case", "case_cert_path=", "case_key_path=", "group_enc", "group_enc_key_id="])

    except getopt.GetoptError as err:
        print WeaveWdmNext.WeaveWdmNext.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed server parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print helpstring
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-o", "--origin"):
            options["client"] = a

        elif o in ("-s", "--server"):
            options["server"] = a

        elif o in ("-t", "--tap"):
            options["tap"] = a

        elif o in ("-w", "--wdm_option"):
            options["wdm_option"] = a

        elif o in ("--test_client_case"):
            options["test_client_case"] = int(a)

        elif o in ("--total_client_count"):
            options["total_client_count"] = int(a)

        elif o in ("--final_client_status"):
            options["final_client_status"] = int(a)

        elif o in ("--timer_client_period"):
            options["timer_client_period"] = int(a)

        elif o in ("--enable_client_stop"):
            options["enable_client_stop"] = int(a)

        elif o in ("--test_client_iterations"):
            options["test_client_iterations"] = int(a)

        elif o in ("--test_client_delay"):
            options["test_client_delay"] = int(a)

        elif o in ("--enable_client_flip"):
            options["enable_client_flip"] = int(a)

        elif o in ("--save_client_perf"):
            options["save_client_perf"] = int(a)

        elif o in ("--test_server_case"):
            options["test_server_case"] = int(a)

        elif o in ("--total_server_count"):
            options["total_server_count"] = int(a)

        elif o in ("--final_server_status"):
            options["final_server_status"] = int(a)

        elif o in ("--timer_server_period"):
            options["timer_server_period"] = int(a)

        elif o in ("--enable_server_stop"):
            options["enable_server_stop"] = int(a)

        elif o in ("--test_server_iterations"):
            options["test_server_iterations"] = int(a)

        elif o in ("--test_server_delay"):
            options["test_server_delay"] = int(a)

        elif o in ("--enable_server_flip"):
            options["enable_server_flip"] = int(a)

        elif o in ("--save_server_perf"):
            options["save_server_perf"] = int(a)

        elif o in ("--test_focus_client"):
            options["swap_role"] = int(a)

        elif o in ("--case"):
            options["case"] = True

        elif o in ("--case_cert_path"):
            options["case_cert_path"] = a

        elif o in ("--case_key_path"):
            options["case_key_path"] = a

        elif o in ("--group_enc"):
            options["group_enc"] = True

        elif o in ("--group_enc_key_id"):
            options["group_enc_key_id"] = a

        else:
            print hred(str(o) + " cannot be recognized")
            assert False, "unhandled option"

        if len(args) == 1:
                options["origin"] = args[0]

        if len(args) == 2:
                options["client"] = args[0]
                options["server"] = args[1]

    cmd = WeaveWdmNext.WeaveWdmNext(options)
    cmd.start()


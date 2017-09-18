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

#
#    @file
#       A Happy command line utility that tests Weave Key Export among Weave nodes.
#
#       The command is executed by instantiating and running WeaveKeyExport class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveKeyExport as WeaveKeyExport

if __name__ == "__main__":
        options = WeaveKeyExport.option()

        try:
                opts, args = getopt.getopt(sys.argv[1:], "ho:s:c:k:dtuwqp:",
                        ["help", "origin=", "server=", "count=", "tcp", "udp", "wrmp",
                         "key-id=", "dont-sign-msgs", "quiet", "tap=",
                         "server_faults=", "client_faults="])
        except getopt.GetoptError as err:
                print WeaveKeyExport.WeaveKeyExport.__doc__
                print hred(str(err))
                sys.exit(hred("%s: Failed server parse arguments." % (__file__)))

        for o, a in opts:
                if o in ("-h", "--help"):
                        print WeaveKeyExport.WeaveKeyExport.__doc__
                        sys.exit(0)

                elif o in ("-q", "--quiet"):
                        options["quiet"] = True

                elif o in ("-t", "--tcp"):
                        options["tcp"] = True

                elif o in ("-u", "--udp"):
                        options["udp"] = True

                elif o in ("-w", "--wrmp"):
                        options["wrmp"] = True

                elif o in ("-d", "--dont-sign-msgs"):
                        options["sign-msgs"] = False

                elif o in ("-k", "--key-id"):
                        options["key-id"] = a

                elif o in ("-o", "--origin"):
                        options["client"] = a

                elif o in ("-s", "--server"):
                        options["server"] = a

                elif o in ("-c", "--count"):
                        options["count"] = a

                elif o in ("-p", "--tap"):
                        options["tap"] = a

                elif o in ("--server_faults"):
                        options["server_faults"] = a

                elif o in ("--client_faults"):
                        options["client_faults"] = a

                else:
                        assert False, "unhandled option"

        if len(args) == 1:
                options["origin"] = args[0]

        if len(args) == 2:
                options["client"] = args[0]
                options["server"] = args[1]

        cmd = WeaveKeyExport.WeaveKeyExport(options)
        cmd.start()


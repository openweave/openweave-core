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
#       A Happy command line utility that creates a record for a Weave Node
#

import getopt
import sys
import set_test_path

import WeaveNodeConfigure
from happy.Utils import *

if __name__ == "__main__":
    options = WeaveNodeConfigure.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hw:c:k:p:qd",
                                   ["help", "weave-node-id=", "weave-certificate=", "private-key=",
                                    "pairing-code=", "quiet", "delete"])

    except getopt.GetoptError as err:
        print WeaveNodeConfigure.WeaveNodeConfigure.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveNodeConfigure.WeaveNodeConfigure.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-w", "--weave-node-id"):
            options["weave_node_id"] = a

        elif o in ("-c", "--weave-certificate"):
            options["weave_certificate"] = a

        elif o in ("-k", "--private-key"):
            options["private_key"] = a

        elif o in ("-p", "--pairing-code"):
            options["pairing_code"] = a

        elif o in ("-d", "--delete"):
            options["delete"] = True

        else:
            assert False, "unhandled option"

    if len(args) == 1:
        options["node_name"] = args[0]

    cmd = WeaveNodeConfigure.WeaveNodeConfigure(options)
    cmd.run()

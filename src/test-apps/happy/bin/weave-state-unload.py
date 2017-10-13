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

##
#    @file
#       Implements WeaveStateUnoad class that tears down virtual network topology
#       together with Weave fabric configuration.
#

import getopt
import sys
import set_test_path

import wrappers.WeaveStateUnload as WeaveStateUnload
from happy.Utils import *

if __name__ == "__main__":
    options = WeaveStateUnload.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:q",
                                   ["help", "file=", "quiet"])

    except getopt.GetoptError as err:
        print WeaveStateUnload.WeaveStateUnload.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveStateUnload.WeaveStateUnload.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-i", "--file"):
            options["json_file"] = a

        else:
            assert False, "unhandled option"

    if len(args) == 1:
        options["json_file"] = args[0]

    cmd = WeaveStateUnload.WeaveStateUnload(options)
    cmd.start()

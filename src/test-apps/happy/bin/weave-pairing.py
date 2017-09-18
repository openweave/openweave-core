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
#       A Happy command line utility that tests Weave pairing.
#
#       The command is executed by instantiating and running WeavePairing class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeavePairing as WeavePairing

if __name__ == "__main__":
    options = WeavePairing.option()


    try:
        opts, args = getopt.getopt(sys.argv[1:], "hqm:d:f:i:u:a:p",
            ["help", "quiet", "mobile=", "device=", "tap="])

    except getopt.GetoptError as err:
        print WeavePairing.WeavePairing.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed destination parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeavePairing.WeavePairing.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-m", "--mobile"):
            options["mobile"] = a

        elif o in ("-d", "--device"):
            options["device"] = a

        elif o in ("-p", "--tap"):
            options["tap"] = a
        else:
            assert False, "unhandled option"

    cmd = WeavePairing.WeavePairing(options)
    cmd.start()


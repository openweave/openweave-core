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
#       A Happy command line utility that tests Weave Service Directory Profile.
#
#       The command is executed by instantiating and running WeaveServiceDir class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveServiceDir as WeaveServiceDir

if __name__ == "__main__":
    options = WeaveServiceDir.option()


    try:
        opts, args = getopt.getopt(sys.argv[1:], "hqc:s:p",
            ["help", "quiet", "client=", "service=", "tap="])

    except getopt.GetoptError as err:
        print WeaveServiceDir.WeaveServiceDir.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed destination parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveServiceDir.WeaveServiceDir.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-c", "--client"):
            options["client"] = a

        elif o in ("-s", "--service"):
            options["service"] = a

        elif o in ("-p", "--tap"):
            options["tap"] = a
        else:
            assert False, "unhandled option"

    if len(args) == 2:
        options["client"] = args[0]
        options["service"] = args[1]


    cmd = WeaveServiceDir.WeaveServiceDir(options)
    cmd.start()


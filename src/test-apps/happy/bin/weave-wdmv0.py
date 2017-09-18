#!/usr/bin/env python


#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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
#       A Happy command line utility that tests Weave wdmv0.
#
#       The command is executed by instantiating and running WeaveWDMv0 class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveWDMv0 as WeaveWDMv0

if __name__ == "__main__":
    options = WeaveWDMv0.option()


    try:
        opts, args = getopt.getopt(sys.argv[1:], "hqs:c:f:i:u:a:p",
            ["help", "quiet", "server=", "client=", "tap="])

    except getopt.GetoptError as err:
        print WeaveWDMv0.WeaveWDMv0.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed destination parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveWDMv0.WeaveWDMv0.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-s", "--server"):
            options["server"] = a

        elif o in ("-c", "--client"):
            options["client"] = a

        elif o in ("-p", "--tap"):
            options["tap"] = a
        else:
            assert False, "unhandled option"

    cmd = WeaveWDMv0.WeaveWDMv0(options)
    cmd.start()


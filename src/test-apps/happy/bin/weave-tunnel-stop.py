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
#       A Happy command line utility that stops Weave Tunnel between border-gateway and a service.
#
#       The command is executed by instantiating and running WeaveTunnelStop class.
#

import getopt
import sys

import plugin.WeaveTunnelStop as WeaveTunnelStop
from happy.Utils import *

if __name__ == "__main__":
    options = WeaveTunnelStop.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hb:s:q",
            ["help", "border_gateway=", "service=", "quiet"])

    except getopt.GetoptError as err:
        print WeaveTunnelStop.WeaveTunnelStop.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveTunnelStop.WeaveTunnelStop.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-b", "--border_gateway"):
            options["border_gateway"] = a

        elif o in ("-s", "--service"):
            options["service"] = a

        else:
            assert False, "unhandled option"

    if len(args) == 2:
        options["border_gateway"] = args[0]
        options["service"] = args[1]

    cmd = WeaveTunnelStop.WeaveTunnelStop(options)
    cmd.start()


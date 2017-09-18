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
#       A Happy command line utility that tests WeaveConnectionTunnel functionality.
#
#       The command is executed by instantiating and running WeaveConnectionTunnel class.
#

import getopt
import sys

import plugin.WeaveConnectionTunnel as WeaveConnectionTunnel
from happy.Utils import *

if __name__ == "__main__":
    options = WeaveConnectionTunnel.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "ha:s:d:qp:",
            ["help", "agent=", "source=", "destination=", "quiet", "tap="])

    except getopt.GetoptError as err:
        print WeaveConnectionTunnel.WeaveConnectionTunnel.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveConnectionTunnel.WeaveConnectionTunnel.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-a", "--agent"):
            options["agent"] = a

        elif o in ("-s", "--source"):
            options["source"] = a

        elif o in ("-d", "--destination"):
            options["destination"] = a

        elif o in ("-p", "--tap"):
            options["tap"] = a

        else:
            assert False, "unhandled option"

    cmd = WeaveConnectionTunnel.WeaveConnectionTunnel(options)
    cmd.start()


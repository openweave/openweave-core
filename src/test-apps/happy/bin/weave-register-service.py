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
#       A Happy command line utility that generates the service registration cmd.
#
#       The command is executed by instantiating and running WeaveRegisterService class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveRegisterService as WeaveRegisterService

if __name__ == "__main__":
    options = WeaveRegisterService.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:t:u:p:",
            ["help", "tier=", "username=", "password=" "quiet"])
    except getopt.GetoptError as err:
        print WeaveRegisterService.WeaveRegisterService.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed server parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveRegisterService.WeaveRegisterService.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        elif o in ("-t", "--tier"):
            options["tier"] = a

        elif o in ("-u", "--username"):
            options["username"] = a

        elif o in ("-p", "--password"):
            options["password"] = a

    cmd = WeaveRegisterService.WeaveRegisterService(options)
    cmd.start()


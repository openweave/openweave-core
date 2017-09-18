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
#       A Happy command line utility that tests Weave BDX among Weave nodes.
#
#       The command is executed by instantiating and running WeaveBDX class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveBDX as WeaveBDX

if __name__ == "__main__":
	options = WeaveBDX.option()

	try:
		opts, args = getopt.getopt(sys.argv[1:], "hqs:c:t:r:u:d:o:l:p:S:C:",
			["help", "quiet", "server=", "client=", "tmp=", "receive=","upload=",
                        "download=", "offset=", "length=", "tap=", "server-version=", "client-version="])

	except getopt.GetoptError as err:
		print WeaveBDX.WeaveBDX.__doc__
		print hred(str(err))
                sys.exit(hred("%s: Failed destination parse arguments." % (__file__)))

	for o, a in opts:
		if o in ("-h", "--help"):
			print WeaveBDX.WeaveBDX.__doc__
			sys.exit(0)

		elif o in ("-q", "--quiet"):
			options["quiet"] = True

		elif o in ("-s", "--server"):
			options["server"] = a

		elif o in ("-c", "--client"):
			options["client"] = a

		elif o in ("-t", "--tmp"):
			options["tmp"] = a

		elif o in ("-r", "--receive"):
			options["receive"] = a

		elif o in ("-u", "--upload"):
			options["upload"] = a

		elif o in ("-d", "--download"):
			options["download"] = a

		elif o in ("-o", "--offset"):
			options["offset"] = a

		elif o in ("-l", "--length"):
			options["length"] = a

		elif o in ("-p", "--tap"):
			options["tap"] = a

		elif o in ("-S", "--server-version"):
			options["server_version"] = int(a)

		elif o in ("-C", "--client-version"):
			options["client_version"] = int(a)

		else:
			assert False, "unhandled option"

	cmd = WeaveBDX.WeaveBDX(options)
	cmd.start()


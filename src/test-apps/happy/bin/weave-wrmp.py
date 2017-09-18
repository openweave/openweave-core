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
#       A Happy command line utility that tests Weave WRMP among Weave nodes.
#
#       The command is executed by instantiating and running WeaveWRMP class.
#

import getopt
import sys

from happy.Utils import *
import plugin.WeaveWRMP as WeaveWRMP

if __name__ == "__main__":
	options = WeaveWRMP.option()

	try:
		opts, args = getopt.getopt(sys.argv[1:], "hc:s:r:t:uq",
			["help", "client=", "server=",
				"retransmit=", "test=", "quiet"])
	except getopt.GetoptError as err:
		print WeaveWRMP.WeaveWRMP.__doc__
		print hred(str(err))
                sys.exit(hred("%s: Failed server parse arguments." % (__file__)))

	for o, a in opts:
		if o in ("-h", "--help"):
			print WeaveWRMP.WeaveWRMP.__doc__
			sys.exit(0)

		elif o in ("-q", "--quiet"):
			options["quiet"] = True

		elif o in ("-t", "--test"):
			options["test"] = a

		elif o in ("-c", "--client"):
			options["client"] = a

		elif o in ("-s", "--server"):
			options["server"] = a

		elif o in ("-r", "--retransmit"):
			options["retransmit"] = a

		else:
			assert False, "unhandled option"

	if len(args) == 1:
		options["client"] = args[0]

	if len(args) == 2:
		options["client"] = args[0]
		options["server"] = args[1]

	cmd = WeaveWRMP.WeaveWRMP(options)
	cmd.start()


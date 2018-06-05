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
#       Display Weave node and fabric configuration
#

import getopt
import sys
import set_test_path

import WeaveState


if __name__ == "__main__":

    options = WeaveState.option()

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hq",
                                   ["help", "quiet"])

    except getopt.GetoptError as err:
    	print WeaveState.WeaveState.__doc__
        print hred(str(err))
        sys.exit(hred("%s: Failed to parse arguments." % (__file__)))

    for o, a in opts:
        if o in ("-h", "--help"):
            print WeaveState.WeaveState.__doc__
            sys.exit(0)

        elif o in ("-q", "--quiet"):
            options["quiet"] = True

        else:
            assert False, "unhandled option"

    weave_state = WeaveState.WeaveState(options)
    weave_state.printState()

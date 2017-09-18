#! /usr/bin/python


#
#    Copyright (c) 2013-2017 Nest Labs, Inc.
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
#      This file implements a Python script that reads a "big number"
#      from standard input and generates the required OpenSSL C
#      language "BN" (big number) code necessary for handling the
#      number on standard output.
#

# NOTE: This only handles positive values.

import sys
import re

valName = "BN"

if len(sys.argv) > 1:
    valName = sys.argv[1] 

valStr = sys.stdin.read()

valStr = re.sub(r"""0x""", '', valStr)
valStr = re.sub(r"""[:,]""", '', valStr)
valStr = re.sub(r"""\s+""", '', valStr)
valStr = valStr.upper()

if re.search(r"""[^0-9A-F]""", valStr):
    sys.stderr.write("Invalid characters in input value\n")
    sys.exit(-1)

while valStr[:2] == "00":
    valStr = valStr[2:]
    
bnVals = []
while (len(valStr) > 0):
    bnVals.append(valStr[-8:]);
    valStr = valStr[:-8]

sys.stdout.write("""
static BN_ULONG s{0}_Data[] =
{{
    """.format(valName))

i = 0
for bnVal in bnVals:
    if i != 0:
        if i % 8 == 0:
            sys.stdout.write(",\n    ")
        else:
            sys.stdout.write(", ")
    sys.stdout.write("0x")
    sys.stdout.write(bnVal)
    i += 1

sys.stdout.write("""
}};

BIGNUM s{0} =
{{
    s{0}_Data,
    sizeof(s{0}_Data)/sizeof(BN_ULONG),
    sizeof(s{0}_Data)/sizeof(BN_ULONG),
    0,
    BN_FLG_STATIC_DATA
}};

""".format(valName))

sys.exit(0)

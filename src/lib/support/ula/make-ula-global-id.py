#!/usr/bin/env python

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
#      This file implements a Python script for making unique
#      pseudo-random global identifiers (Global IDs) for IPv6 unique
#      local addresses (ULAs) according to Section 3.2.2 of RFC 4193.
#

"""Tool for making unique pseudo-random global identifiers (Global IDs) for IPv6 unique local addresses (ULAs) according to Section 3.2.2 of RFC 4193.
"""

import sys
import os
import re
import hashlib
import binascii
import time
import struct
import math
from datetime import datetime

__all__ = [ 'MakeULAGlobalId' ]


def NormalizeMAC(macAddr):
        macAddr = macAddr.replace(':', '')
        macAddr = macAddr.replace('-', '')
        macAddr = macAddr.upper()

        return macAddr

def MakeULAGlobalId(macAddr, timeStamp=datetime.utcnow()):
        # Normalized the string representation of the MAC.
        macAddr = NormalizeMAC(macAddr)

        # If the MAC address is a MAC-48 convert it to a EUI-64 as specified in RFC-4291.
        if len(macAddr) == 12:
                macAddr = macAddr[:6] + "FFFE" + macAddr[6:]

        # Throw an error if an invalid MAC was specified.
        if len(macAddr) != 16:
                raise ValueError('Invalid MAC address')

        # Convert the MAC to binary.
        macAddr = binascii.unhexlify(macAddr)

        # Convert the time stamp to a TimeDelta since the NTP epoch.
        timeStamp = timeStamp - datetime(1900, 1, 1)

        # Convert the the TimeDelta to integral and fractional seconds in 64-bit NTP format.
        timeStampSecs = timeStamp.days * 86400 + timeStamp.seconds
        timeStampFractSecs = int(timeStamp.microseconds * (2**32) // 1000000)

        # Create a byte string containing the 32-bit seconds value, the 32-bit
        # fractional seconds value and the binary MAC address in network order.
        hashInput = struct.pack('!LL8s', timeStampSecs, timeStampFractSecs, macAddr)

        # Hash the byte string using SHA1.
        hash = hashlib.sha1(hashInput).digest()

        # Extract the lower 40 bits as an 8 bit integer and 2 16 bit integers.
        (field1, field2, field3) = struct.unpack('!15xBHH', hash)

        # Combine the field values with the ULA prefix (FD00::/8) to create the
        # ULA prefix.
        return "%x:%x:%x" % (field1 + 0xfd00, field2, field3)

# Testing code
#
if __name__ == "__main__":

        usage = """
  make-ula-gloabl-id.py <mac-addr> [ <date-time>  | <date> <time> ]

  where:
        <mac-addr> is a 48 or 64-bit MAC address in hex (XX:XX:XX:XX:XX:XX)

        <date-time> is a date and time (YYYY/MM/DD HH:MM:SS)
 """

        if len(sys.argv) < 2:
                print usage
                sys.exit(-1)

        if len(sys.argv) > 4:
                print "Unexpected argument: %s" % (sys.argv[3])
                sys.exit(-1)

        macAddr = sys.argv[1]

        # Handle either the date-time argument as a single, quoted
        # argument (e.g. "2010/08/30 12:34:04") or as two, separate
        # arguments (e.g. 2010/08/30 12:34:04).

        if len(sys.argv) == 3:
                timeStamp = datetime.strptime(sys.argv[2], '%Y/%m/%d %H:%M:%S')
        elif len(sys.argv) == 4:
                timeStamp = datetime.strptime(sys.argv[2] + ' ' + sys.argv[3], '%Y/%m/%d %H:%M:%S')
        else:
                timeStamp = datetime.utcnow()

        print ("%s::/48") % MakeULAGlobalId(macAddr, timeStamp)

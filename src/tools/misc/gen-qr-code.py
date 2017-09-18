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
#      This file implements a Python script to read binary data from
#      standard input and generates a QR code.
#
#      If the output is to a console, the QR code is displayed
#      directly on the terminal using alternating background
#      colors. If the output is to a file, the QR code is written as a
#      Portable Network Graphic (PNG) file.
#

import sys
import optparse
import qrcode
from qrcode import constants, exceptions, util
from optparse import OptionParser

usage = """gen-qr-code.py [options] < input [ > output-file ]

Converts binary data read from stdin into a QR code. If output is to a console,
the QR code is display directly on the terminal using alternating background
colors. If the output is to a file the QR code is written to the file as a PNG."""

optParser = OptionParser(usage=usage)
optParser.add_option("-v", "--version", action="store", dest="version", help="QR code version", metavar="INTEGER")
optParser.add_option("-e", "--error-correction", action="store", dest="errCorrection", help="Error correction mode", metavar="[LMQH]", default="M")
optParser.add_option("-s", "--box-size", action="store", dest="boxSize", help="Box size", metavar="PIXELS", default=10)
optParser.add_option("-b", "--border-size", action="store", dest="borderSize", help="Boarder size", metavar="PIXELS", default=4)
optParser.add_option("-f", "--fit", action="store", dest="fit", help="disable auto fit", metavar="BOOL", default=False)
optParser.add_option("-m", "--mode", action="store", dest="mode", help="QR code mode", metavar="[NAB]", default="B")

(options, remainingArgs) = optParser.parse_args(sys.argv[1:])
if len(remainingArgs) != 0:
    print 'Unexpected argument: %s' % remainingArgs[0]
    sys.exit(-1)

options.errCorrection = options.errCorrection.upper()
if (options.errCorrection == 'L'):
    options.errCorrection = constants.ERROR_CORRECT_L
elif (options.errCorrection == 'M'):
    options.errCorrection = constants.ERROR_CORRECT_M
elif (options.errCorrection == 'Q'):
    options.errCorrection = constants.ERROR_CORRECT_Q
elif (options.errCorrection == 'H'):
    options.errCorrection = constants.ERROR_CORRECT_H
else:
    print 'Unrecognized error correction level: %s' % options.errCorrection
    sys.exit(-1)

options.mode = options.mode.upper()
if (options.mode == 'N'):
    options.mode = util.MODE_NUMBER
elif (options.mode == 'A'):
    options.mode = util.MODE_ALPHA_NUM
elif (options.mode == 'B'):
    options.mode = util.MODE_8BIT_BYTE
else:
    print 'Unrecognized encoding mode: %s' % options.mode
    sys.exit(-1)

qr = qrcode.QRCode(version=options.version, box_size=int(options.boxSize), border=int(options.borderSize), error_correction=options.errCorrection)

data = sys.stdin.read()

data = util.QRData(data, mode=options.mode)

qr.add_data(data)

if options.fit:
    qr.make(fit=False)

if sys.stdout.isatty():
    qr.print_tty()

else:
    img = qr.make_image(image_factory=None)
    img.save(sys.stdout)

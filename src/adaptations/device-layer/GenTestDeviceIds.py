#
#    Copyright (c) 2018-2020 Google, LLC.
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

import os
import sys
import re
import base64

preamble = '''\
/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceInternal.h>
#include <Weave/DeviceLayer/ConfigurationManager.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

'''

postamble = '''

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

const uint8_t TestDeviceIntermediateCACert[] =
{
    0xD6, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x15, 0x30, 0x01, 0x08, 0x6A, 0x76, 0xA7, 0xB9, 0x50,
    0x3C, 0x34, 0x7A, 0x24, 0x02, 0x05, 0x37, 0x03, 0x27, 0x13, 0x01, 0x00, 0x00, 0xEE, 0xEE, 0x30,
    0xB4, 0x18, 0x18, 0x26, 0x04, 0x80, 0x62, 0xA9, 0x19, 0x26, 0x05, 0x7F, 0xDE, 0x72, 0x79, 0x37,
    0x06, 0x27, 0x13, 0x02, 0x00, 0x00, 0xEE, 0xEE, 0x30, 0xB4, 0x18, 0x18, 0x24, 0x07, 0x02, 0x26,
    0x08, 0x15, 0x00, 0x5A, 0x23, 0x30, 0x0A, 0x31, 0x04, 0x3B, 0x9D, 0xC4, 0xE8, 0xCA, 0xC8, 0x33,
    0xA0, 0x2E, 0x7B, 0x5D, 0xB5, 0x29, 0xF4, 0xA6, 0xD5, 0xF8, 0x82, 0x26, 0x4C, 0xD2, 0xFB, 0x31,
    0x21, 0xE6, 0x84, 0xA5, 0x1C, 0xC9, 0x58, 0x13, 0x72, 0x36, 0x4A, 0x05, 0xA9, 0xC6, 0x27, 0x65,
    0xDD, 0x20, 0xDB, 0x30, 0xD4, 0x6B, 0xF8, 0xAD, 0x31, 0x35, 0x83, 0x29, 0x01, 0x29, 0x02, 0x18,
    0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x60, 0x18, 0x35, 0x81, 0x30, 0x02, 0x08, 0x44, 0xE3, 0x40,
    0x38, 0xA9, 0xD4, 0xB5, 0xA7, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x42, 0x0C, 0xAC, 0xF6, 0xB4,
    0x64, 0x71, 0xE6, 0x18, 0x35, 0x0C, 0x30, 0x01, 0x18, 0x57, 0x63, 0xAA, 0xD5, 0x6A, 0x91, 0xCE,
    0x35, 0xAB, 0x2A, 0x44, 0x77, 0x31, 0x3C, 0xBA, 0xFC, 0x77, 0x5F, 0x3E, 0xFE, 0xCB, 0xA2, 0x65,
    0x4B, 0x30, 0x02, 0x19, 0x00, 0xF4, 0x54, 0x79, 0x8C, 0xAA, 0x07, 0x13, 0x0B, 0xAF, 0xA8, 0x8F,
    0xCB, 0x0B, 0x2F, 0x80, 0x8D, 0xA3, 0x57, 0xBB, 0xC7, 0xA0, 0xFF, 0x54, 0xD5, 0x18, 0x18, 0x18,
};

const uint16_t TestDeviceCertLength = sizeof(TestDeviceCert);
const uint16_t TestDeviceIntermediateCACertLength = sizeof(TestDeviceIntermediateCACert);
const uint16_t TestDevicePrivateKeyLength = sizeof(TestDevicePrivateKey);

#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
'''



def toArrayInit(data):
    res = ''
    i = 0
    for ch in data:
        if i != 0 and i % 16 == 0:
            res += '\n'
        res += '0x%02X, ' % ord(ch)
        i += 1
    res += '\n'
    return res

def indent(s, indentStr='    '):
    return re.sub(r'''^''', indentStr, s, flags=re.MULTILINE)

inFileName = sys.argv[1]
outFileName = sys.argv[2]

isFirst = True

with open(inFileName, 'r') as inFile:
    with open(outFileName, 'w') as outFile:

        outFile.write(preamble);

        for line in inFile:
            line = line.strip()

            if line == "":
                continue

            (macAddr, certificate, privateKey) = [ x.strip() for x in line.split(',')[:3]]

            if macAddr == 'MAC' and certificate == 'Certificate' and privateKey == 'Private Key':
                continue

            certificate = base64.b64decode(certificate)
            privateKey = base64.b64decode(privateKey)

            idNum = int(macAddr[-2:], 16)

            outFile.write('''\
#%s WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY == %d

''' % ('if' if isFirst else 'elif', idNum))

            isFirst = False

            outFile.write('''\
const uint64_t TestDeviceId = 0x%sULL;

const uint8_t TestDeviceCert[] =
{
''' % (macAddr))

            outFile.write(indent(toArrayInit(certificate).strip()))

            outFile.write('''
};

const uint8_t TestDevicePrivateKey[] =
{
''')

            outFile.write(indent(toArrayInit(privateKey).strip()))

            outFile.write('''
};

''')

        outFile.write('''\
#endif // WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY == %d
''' % (idNum))

        outFile.write(postamble);

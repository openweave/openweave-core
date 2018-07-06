import os
import sys
import re
import base64

preamble = '''\
/*
 *
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

#include <internal/WeaveDeviceInternal.h>
#include <ConfigurationManager.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

'''

postamble = '''

#if WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY

const uint16_t TestDeviceCertLength = sizeof(TestDeviceCert);
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

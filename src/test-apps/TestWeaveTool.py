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
#      Unit tests for the weave command line tool.
#

import os
import sys
import unittest
import tempfile
import shutil
import subprocess
import base64
import binascii
import re
import hashlib
import argparse
import itertools


# ================================================================================
# Test Data
# ================================================================================

args = None

allCurves = [ 'prime192v1', 'secp224r1', 'prime256v1' ]

allSignatureHashes = [ 'sha1', 'sha256' ]

deviceCert_Weave = base64.b64decode('''
1QAABAABADABCEiqbwLPvS7MJAIFNwMnEwIAAO7uMLQYGCYEABhzGiYFfxKaLTcGJxEBAAAAADC0
GBgkBwImCCUAWiMwCjkE72edUwyZ/51yQrH5tmAgjiWfNXLwo+eD5lYUk/loRWWLJDFeh4xkNSWH
GQOZzUWhJPp2CxKeOX41gykBGDWCKQEkAgUYNYQpATYCBAIEARgYNYEwAghO/0dR5MZjmxg1gDAC
CETjQDip1LWnGDUMMAEYILdaKd7BcqnLAeFhSRoELsUBOTW7DAUYMAIZAKKWYMG5uOD6TfpmKbEQ
qlVFjrRd9t+4lhgY
''')

deviceCert_PEM = '''\
-----BEGIN CERTIFICATE-----
MIIBiTCCAT+gAwIBAgIISKpvAs+9LswwCgYIKoZIzj0EAwIwIjEgMB4GCisGAQQB
gsMrAQMMEDE4QjQzMEVFRUUwMDAwMDIwHhcNMTMxMDIyMDAwMDAwWhcNMjMxMDIw
MjM1OTU5WjAiMSAwHgYKKwYBBAGCwysBAQwQMThCNDMwMDAwMDAwMDAwMTBOMBAG
ByqGSM49AgEGBSuBBAAhAzoABO9nnVMMmf+dckKx+bZgII4lnzVy8KPng+ZWFJP5
aEVliyQxXoeMZDUlhxkDmc1FoST6dgsSnjl+o2owaDAMBgNVHRMBAf8EAjAAMA4G
A1UdDwEB/wQEAwIFoDAgBgNVHSUBAf8EFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEw
EQYDVR0OBAoECE7/R1HkxmObMBMGA1UdIwQMMAqACETjQDip1LWnMAoGCCqGSM49
BAMCAzgAMDUCGCC3WinewXKpywHhYUkaBC7FATk1uwwFGAIZAKKWYMG5uOD6Tfpm
KbEQqlVFjrRd9t+4lg==
-----END CERTIFICATE-----
'''

deviceCert_DER = base64.b64decode('''
MIIBiTCCAT+gAwIBAgIISKpvAs+9LswwCgYIKoZIzj0EAwIwIjEgMB4GCisGAQQBgsMrAQMMEDE4
QjQzMEVFRUUwMDAwMDIwHhcNMTMxMDIyMDAwMDAwWhcNMjMxMDIwMjM1OTU5WjAiMSAwHgYKKwYB
BAGCwysBAQwQMThCNDMwMDAwMDAwMDAwMTBOMBAGByqGSM49AgEGBSuBBAAhAzoABO9nnVMMmf+d
ckKx+bZgII4lnzVy8KPng+ZWFJP5aEVliyQxXoeMZDUlhxkDmc1FoST6dgsSnjl+o2owaDAMBgNV
HRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIFoDAgBgNVHSUBAf8EFjAUBggrBgEFBQcDAgYIKwYBBQUH
AwEwEQYDVR0OBAoECE7/R1HkxmObMBMGA1UdIwQMMAqACETjQDip1LWnMAoGCCqGSM49BAMCAzgA
MDUCGCC3WinewXKpywHhYUkaBC7FATk1uwwFGAIZAKKWYMG5uOD6TfpmKbEQqlVFjrRd9t+4lg==
''')

deviceKey_Weave = base64.b64decode('''
1QAABAACACQBJTACHKmH22bJe2mLomgimLLt70dguW98M6EsKajVuO4wAzkE72edUwyZ/51yQrH5
tmAgjiWfNXLwo+eD5lYUk/loRWWLJDFeh4xkNSWHGQOZzUWhJPp2CxKeOX4Y
''')

deviceKey_PKCS8_PEM = '''\
-----BEGIN PRIVATE KEY-----
MHgCAQAwEAYHKoZIzj0CAQYFK4EEACEEYTBfAgEBBByph9tmyXtpi6JoIpiy7e9H
YLlvfDOhLCmo1bjuoTwDOgAE72edUwyZ/51yQrH5tmAgjiWfNXLwo+eD5lYUk/lo
RWWLJDFeh4xkNSWHGQOZzUWhJPp2CxKeOX4=
-----END PRIVATE KEY-----
'''

deviceKey_PKCS8_DER = base64.b64decode('''
MHgCAQAwEAYHKoZIzj0CAQYFK4EEACEEYTBfAgEBBByph9tmyXtpi6JoIpiy7e9H
YLlvfDOhLCmo1bjuoTwDOgAE72edUwyZ/51yQrH5tmAgjiWfNXLwo+eD5lYUk/lo
RWWLJDFeh4xkNSWHGQOZzUWhJPp2CxKeOX4=
''')

deviceKey_SEC1_PEM = '''\
-----BEGIN EC PRIVATE KEY-----
MGgCAQEEHKmH22bJe2mLomgimLLt70dguW98M6EsKajVuO6gBwYFK4EEACGhPAM6
AATvZ51TDJn/nXJCsfm2YCCOJZ81cvCj54PmVhST+WhFZYskMV6HjGQ1JYcZA5nN
RaEk+nYLEp45fg==
-----END EC PRIVATE KEY-----
'''

deviceKey_SEC1_DER = base64.b64decode('''
MGgCAQEEHKmH22bJe2mLomgimLLt70dguW98M6EsKajVuO6gBwYFK4EEACGhPAM6
AATvZ51TDJn/nXJCsfm2YCCOJZ81cvCj54PmVhST+WhFZYskMV6HjGQ1JYcZA5nN
RaEk+nYLEp45fg==
''')

ecPrivKey_prime192v1 = '''\
-----BEGIN EC PRIVATE KEY-----
MF8CAQEEGCRNKztjGPy1ZaOBN0PNPhZWNYzwG+jGHKAKBggqhkjOPQMBAaE0AzIA
BB0B2y4GG4vi83oeDCKEH5dRBsfrDJIN8eOK99tW1APHjAlD+dk6bjd87hAWbYtB
0A==
-----END EC PRIVATE KEY-----
'''

ecPrivKey_secp224r1 = '''\
-----BEGIN EC PRIVATE KEY-----
MGgCAQEEHJwfkRY712nw9ZorpyuD0BWWVJE4qDZTH8gJWoKgBwYFK4EEACGhPAM6
AATZb9qPAwHhPQYog7jzwcBJ+jeItUwc+gWy8y4h3ElQftCIt+E5vLgZ/eJtDBE2
nLM8N1XVSXbdyA==
-----END EC PRIVATE KEY-----
'''

ecPrivKey_prime256v1 = '''\
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIIa9Aa2nUwi5evHrP6k6wtW2Ej202GOPjmbqPjozbnlZoAoGCCqGSM49
AwEHoUQDQgAE5T4DC5tEevCM02PZRQeh+e9DEAQzXoz97xN8Lx9eHq4EE7XHhqf9
cKxcN1Wowx24MlwHnqycOTOvPnLq2lQ0eA==
-----END EC PRIVATE KEY-----
'''

ecPrivKeys = {
    'prime192v1': ecPrivKey_prime192v1,
    'secp224r1':  ecPrivKey_secp224r1,
    'prime256v1': ecPrivKey_prime256v1,
}

rootCACert_prime256v1_sha256 = '''\
-----BEGIN CERTIFICATE-----
MIIBhTCCASugAwIBAgIIVNWe4R2MEuwwCgYIKoZIzj0EAwIwIjEgMB4GCisGAQQB
gsMrAQMMEDE4QjQzMEVFRUUwMDAwMDEwHhcNMTgwMTAxMDAwMDAwWhcNMTgxMjMx
MjM1OTU5WjAiMSAwHgYKKwYBBAGCwysBAwwQMThCNDMwRUVFRTAwMDAwMTBZMBMG
ByqGSM49AgEGCCqGSM49AwEHA0IABHRsXKwVm44lx1XonvFe6NN2FC0EHxrURPkF
UD4HITXZvCJuvuhts9omej+dK5xKykeeboFg+nFfigXBWe+6q8CjSzBJMA8GA1Ud
EwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMBEGA1UdDgQKBAhDIkjqBK5b0jAT
BgNVHSMEDDAKgAhDIkjqBK5b0jAKBggqhkjOPQQDAgNIADBFAiEAyepSVsXgc3YS
v9UrGrlV5yRh/YKehKkd/qqtXIoHtdYCIHTccVIhRP2luMS74x2P7yeBTB9m3/Fi
+AVuB/DSuE+K
-----END CERTIFICATE-----
'''

rootCAKey_prime256v1 = '''\
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIBKjAhVUhrZv9epNU/TOgrMnidr6kQTrIzc5zdmRbcNzoAoGCCqGSM49
AwEHoUQDQgAEdGxcrBWbjiXHVeie8V7o03YULQQfGtRE+QVQPgchNdm8Im6+6G2z
2iZ6P50rnErKR55ugWD6cV+KBcFZ77qrwA==
-----END EC PRIVATE KEY-----
'''

rootCAKeyId_prime256v1 = binascii.unhexlify('432248EA04AE5BD2')

weaveSig_ecdsaWithSHA1_signingCert = base64.b64decode('''\
1QAABAAFADUBMAEdAMaLcZBPltcc7efJQrbZUGPj1D73DWaf+scPKd0wAh0AizhyI08p7rmHuodn
LJtP/0eswUpU5QulTAXl2Rg2BBUwAQgaUkJcquuMVCQCBFcDACcTBAAA7u4wtBgYJgQIrWUaJgWI
oxgtVwYAJxQBAADuAzC0GBgkBwIkCCUwCjkEyGxXmW/tdZwqQFBDdK6rV0JuWRhMM4W4kEteNaVG
+pYEetbpTVk/xQOGR8GTiHO5zUzGBqORpxk1gykBGDWCKQEkAgEYNYQpATYCBAMYGDWBMAIIRybd
iJ776L8YNYAwAghMjpcZLrz47Rg1DDABHCPtQBA1kYR/qhLlvZ/88vkCFo/aB6yZS4O6cekwAh0A
qcjqr71PG/EoC0rjT8jK+tEw07cLJ8/d59szuhgYFTABCGRW+nvGNJkUJAIEVwMAJxMBAADu7jC0
GBgmBOGIZRomBWEE5VNXBgAnEwQAAO7uMLQYGCQHAiQIJTAKOQQ3boDGKBoAVSfJn1CGq3F6mWzd
25VCwiQ3fHabganwrjBOEGLnWBxz0o5nrEG15D0ZBlBYhwFVzDWDKQEpAhg1gikBJAJgGDWBMAII
TI6XGS68+O0YNYAwAghKqnukemFLLRg1DDABHQCrMcDH5OYW1me01XfsZwTG3igFS/XJKlTtetvA
MAIdAPC4MHMAwN3fk0W17E0aeFrtovEgcsJ8GrfNDAAYGBgY
''')

weaveSig_ecdsaWithSHA256_certRef = base64.b64decode('''\
1QAABAAFACUFBQI1ATABHE0Px2EANL9tD9G4K82MeSUHihoqi9nhqJxa0JwwAhwtgeTZTnZpiX/+
eZ71UhRhnjKfRp/8bH+ipYZMGDUDMAIIRybdiJ776L8YGA==
''')




# ================================================================================
# Utility Classes and Functions
# ================================================================================

def computeTruncatedKeyId(pubKey):
    
    # Generate "truncated" SHA-1 hash. Per RFC5280:
    # 
    #     "(2) The keyIdentifier is composed of a four-bit type field with the value 0100 followed
    #     by the least significant 60 bits of the SHA-1 hash of the value of the BIT STRING
    #     subjectPublicKey (excluding the tag, length, and number of unused bits)."
    # 
    pubKeyHash = hashlib.sha1(pubKey).digest()
    keyId = chr(0x40 | ord(pubKeyHash[12]) & 0xF) + pubKeyHash[13:]
    
    return keyId

def splitSimpleCSV(line):
    return [ v.strip() for v in line.split(',') ]

def parseOpenSSLTextField(text, startMarker):
    m = re.search(r'''^\s+%s(.*)$''' % startMarker, text, re.MULTILINE)
    if m:
        return m.group(1).strip()
    else:
        return None

def parseOpenSSLDNField(text, startMarker):
    v = parseOpenSSLTextField(text, startMarker)
    if v != None:
        v = re.sub(r'''\s+=\s+''', '=', v)
    return v

def parseOpenSSLHexField(text, startMarker):
    m = re.search(r'''^\s*%s\s*(([0-9A-Fa-f]{2}[\s:]+)+)''' % (startMarker), text, re.MULTILINE)
    if m:
        valueText = m.group(1)
        valueHex = re.sub(r'''[^0-9A-Fa-f]''', '', valueText)
        return binascii.unhexlify(valueHex)
    else:
        return None

def prefixLines(text, prefix):
    return re.sub(r'''^''', prefix, text, flags=re.MULTILINE)

def isPrintable(s):
    return re.search(r'''[^\x09-\x0d\x20-\x7f]''', s) == None

class FileArg:
    def __init__(self, name):
        self.name = name
        self.baseName = name
    
    def prepare(self, dirName):
        self.name = os.path.join(dirName, self.baseName)

    def __str__(self):
        return self.name

class InFileArg(FileArg):
    def __init__(self, name, contents):
        FileArg.__init__(self, name)
        self.contents = contents

    def prepare(self, dirName):
        FileArg.prepare(self, dirName)
        with open(self.name, 'w+') as f:
            f.write(self.contents)

class OutFileArg(FileArg):
    def __init__(self, name):
        self.name = name
        self.baseName = name

    @property
    def contents(self):
        if os.access(self.name, os.R_OK):
            with open(self.name, 'rb') as f:
                self.contents = f.read()
                return self.contents
        else:
            return None

class CommandRecord:
    def __init__(self, cmd, args, files, res):
        self.cmd = cmd
        self.args = args
        self.files = files
        self.res = res
    
    def __str__(self):
        s = '''Command: %s %s\n  Exit Code: %d''' %  (self.cmd, ' '.join(self.args), self.res)
        for name, contents in self.files:
            if contents != None:
                if not isPrintable(contents):
                    contents = contents.encode('string-escape')
                s += '\n  %s:\n%s' % (name, prefixLines(contents, '    |'))
        return s


class WeaveToolTestCase(unittest.TestCase):
    '''Base class for all weave tool test cases'''
    
    def id(self):
        testName = re.sub(r'''^TEST\d\d_?''', '', type(self).__name__)
        methodName = re.sub(r'''^test_?''', '', self._testMethodName)
        if methodName != '':
            return "%s[%s]" % (testName, methodName)
        else:
            return testName
    
    def setUp(self):
        unittest.TestCase.setUp(self)

        # Create a temporary directory to hold test input/output files.
        parentDir = '/dev/shm'
        if not os.path.isdir(parentDir):
            parentDir = None
        self.tmpDir = tempfile.mkdtemp(dir=parentDir)
        
        self.clearTestContext()
        
        print '- %s' % (self.id())

    def tearDown(self):
        unittest.TestCase.tearDown(self)
        
        # Remove the temporary directory, and any files it contains.
        if os.path.isdir(self.tmpDir):
            shutil.rmtree(self.tmpDir)

    def getCommandSummary(self):
        return 'Commands executed: %d\n\n%s' % (
            len(self.commandRecords),
            '\n\n'.join((str(cmdRec) for cmdRec in self.commandRecords))
        )
        
    def clearTestContext(self):
        self.commandRecords = []
        
    def clearTmpFiles(self):
        if os.path.isdir(self.tmpDir):
            for name in os.listdir(self.tmpDir):
                pathName = os.path.join(self.tmpDir, name)
                if os.path.isfile(pathName):
                    os.unlink(pathName)

    def runCommand(self, cmd, args, stdin=None):
        
        fileArgs = [ arg for arg in args if isinstance(arg, FileArg) ]

        for fileArg in fileArgs:
            fileArg.prepare(self.tmpDir)
            
        args = [ str(arg) for arg in args ]

        proc = subprocess.Popen([ cmd ] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (stdout, stderr) = proc.communicate(stdin)
        res = proc.wait()
        
        self.commandRecords.append(
            CommandRecord(
                cmd=cmd,
                args=args,
                files=[ ('stdin', stdin), ('stdout', stdout), ('stderr', stderr) ] + [ (fileArg.name, fileArg.contents) for fileArg in fileArgs ],
                res=res
            )
        )
        
        return (res, stdout, stderr)

    def parseWeaveCert(self, cert):
        
        (res, stdout, stderr) = self.runCommand(args.weaveTool, 
            args=[ 'convert-cert', '--x509', '-', '-' ],
            stdin=cert)
        
        self.assertEqual(res, 0, 'Failed to convert weave certificate')
        
        return self.parseX509Cert(stdout)
    
    def parseX509Cert(self, cert):
        
        if re.match(r'''(-+)[\w\s]+\1''', cert):
            inForm = 'pem'
        else:
            inForm = 'der'
        
        (res, certText, stderr) = self.runCommand(args.opensslTool, 
            args=[ 'x509', '-text', '-noout', '-inform', inForm ],
            stdin=cert)
        
        self.assertEqual(res, 0, 'Failed to dump X.509 certificate')
        
        certProps = { }
        certProps['Text'] = certText
        certProps['Version'] = parseOpenSSLTextField(certText, 'Version:')
        certProps['SerialNumber'] = parseOpenSSLTextField(certText, 'Serial Number:')
        certProps['SignatureAlgorithm'] = parseOpenSSLTextField(certText, 'Signature Algorithm:')
        certProps['Issuer'] = parseOpenSSLDNField(certText, 'Issuer:')
        certProps['NotBefore'] = parseOpenSSLTextField(certText, 'Not Before:')
        certProps['NotAfter'] = parseOpenSSLTextField(certText, 'Not After :')
        certProps['Subject'] = parseOpenSSLDNField(certText, 'Subject:')
        certProps['Curve'] = parseOpenSSLTextField(certText, 'ASN1 OID:')
        certProps['PublicKeyAlgorithm'] = parseOpenSSLTextField(certText, 'Public Key Algorithm:')
        certProps['PublicKey'] = parseOpenSSLHexField(certText, 'pub:')
        certProps['SubjectKeyId'] = parseOpenSSLHexField(certText, 'X509v3 Subject Key Identifier:')
        certProps['AuthorityKeyId'] = parseOpenSSLHexField(certText, r'''X509v3 Authority Key Identifier:\s+keyid:''')
        
        m = re.search(r'''^\s+X509v3 Basic Constraints: critical\n([^\n]+)$''', certText, re.MULTILINE)
        if m:
            certProps['BasicConstraints'] = [ v.strip() for v in m.group(1).split(',') ]
        else:
            certProps['BasicConstraints'] = None

        m = re.search(r'''^\s+X509v3 Key Usage: critical\n([^\n]+)$''', certText, re.MULTILINE)
        if m:
            certProps['KeyUsage'] = [ v.strip() for v in m.group(1).split(',') ]
        else:
            certProps['KeyUsage'] = None
        
        m = re.search(r'''^\s+X509v3 Extended Key Usage: critical\n([^\n]+)$''', certText, re.MULTILINE)
        if m:
            certProps['ExtendedKeyUsage'] = [ v.strip() for v in m.group(1).split(',') ]
        else:
            certProps['ExtendedKeyUsage'] = None
            
        return certProps

    def privateKeyToPublicKey(self, privKey, keyFormat):
        
        if keyFormat == 'weave':
            (res, privKeyPEM, stderr) = self.runCommand(args.weaveTool, 
                args=[ 'convert-key', '--pem', '-', '-' ],
                stdin=privKey)
            self.assertEqual(res, 0, 'Failed to convert Weave private key to PEM format')
            privKey = privKeyPEM
            keyFormat = 'pem'
            
        if keyFormat == 'pkcs8':
            (res, privKeyPEM, stderr) = self.runCommand(args.opensslTool, 
                args=[ 'pkcs8', '-nocrypt', '-inform', 'der', '-outform', 'pem' ],
                stdin=privKey)
            self.assertEqual(res, 0, 'Failed to convert PKCS#8 private key to PEM format')
            privKey = privKeyPEM
            keyFormat = 'pem'
        
        (res, privKeyText, stderr) = self.runCommand(args.opensslTool, 
            args=[ 'ec', '-text', '-noout', '-inform', keyFormat ],
            stdin=privKey)
        self.assertEqual(res, 0, 'Failed to extract public key from private key')
        
        return parseOpenSSLHexField(privKeyText, 'pub:')

    def verifyProvisioningDataFile(self, fileName, deviceCount, curve, hash, certFormat, keyFormat):

        # Read the provisioning data file...
        with open(fileName, 'r') as provData:
            
            # Read and parse the header line
            columnNames = splitSimpleCSV(provData.readline())
            
            certColName = 'Certificate' + (' ' + certFormat.upper() if certFormat != 'weave' else '')
            privKeyColName = 'Private Key' + (' ' + keyFormat.upper() if keyFormat != 'weave' else '')

            # Verify the correct columns are present
            expectedColumnNames = [
                'MAC',
                certColName,
                privKeyColName,
                'Permissions',
                'Pairing Code',
                'Certificate Type'
            ]
            self.assertItemsEqual(columnNames, expectedColumnNames, 'Invalid column names in provisioning data file')

            macCol = columnNames.index('MAC')
            certCol = columnNames.index(certColName)
            privKeyCol = columnNames.index(privKeyColName)
            pairingCodeCol = columnNames.index('Pairing Code')
            certTypeCol = columnNames.index('Certificate Type')
            
            count = 0
            
            # Read the provisioning data lines...
            for line in provData:
                
                # Parse the provisioning data
                values = splitSimpleCSV(line)
                mac = values[macCol]
                cert = values[certCol]
                privKey = values[privKeyCol]
                pairingCode = values[pairingCodeCol]
                certType = values[certTypeCol]
                
                # Verify the MAC
                expectedMAC = "%16X" % (0x18B4300000000001 + count)
                self.assertEqual(mac, expectedMAC, 'Invalid MAC value')
                
                # Verify the certificate's construction
                if certFormat == 'weave':
                    certProps = self.parseWeaveCert(cert)
                else:
                    cert = base64.b64decode(cert)
                    certProps = self.parseX509Cert(cert)
                self.verifyDeviceCert(certProps, mac, curve, hash)

                # Verify the CertificateType
                self.assertEqual(certType, 'ECDSAWith' + hash.upper(), 'Invalid Certificate Type value')
                
                # Verify the private key matches the public key in the certificate.
                privKey = base64.b64decode(privKey)
                expectedPubKey = self.privateKeyToPublicKey(privKey, keyFormat)
                self.assertEqual(certProps['PublicKey'], expectedPubKey, 'Public key in certificate does not match generated private key')
                
                # Verify the pairing code
                self.verifyPairingCode(pairingCode)
                
                count += 1

            self.assertEqual(count, deviceCount, 'Incorrect number of lines in generate provisioning data file')

    def verifyPairingCode(self, pairingCode):
        for ch in pairingCode:
            self.assertTrue(ch in Verhoeff.CharSet_Base32, 'Invalid character in pairing code')
        self.assertTrue(Verhoeff.VerifyCheckChar32(pairingCode), 'Invalid check character in pairing code')
        
    def verifyDeviceCert(self, certProps, mac, curve, hash):
        self.assertEqual(certProps['Version'], '3 (0x2)', 'Invalid certificate version')
        self.assertEqual(certProps['Subject'], '1.3.6.1.4.1.41387.1.1=%s' % mac, 'Invalid certificate subject')
        self.assertEqual(certProps['Issuer'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate issuer')
        self.assertEqual(certProps['Curve'], curve, 'Invalid public key curve');
        self.assertEqual(certProps['SignatureAlgorithm'], 'ecdsa-with-%s' % (hash.upper()), 'Invalid signature algo');
        self.assertEqual(certProps['NotBefore'], 'Jan  1 00:00:00 2018 GMT', 'Invalid NotBefore date');
        self.assertEqual(certProps['NotAfter'], 'Dec 31 23:59:59 2018 GMT', 'Invalid NotAfter date');
        self.assertIn('CA:FALSE', certProps['BasicConstraints'], 'Invalid CA constraint')
        self.assertItemsEqual(['Digital Signature', 'Key Encipherment' ], certProps['KeyUsage'], 'Invalid Key Usage constraint')
        self.assertItemsEqual(['TLS Web Client Authentication', 'TLS Web Server Authentication' ], certProps['ExtendedKeyUsage'], 'Invalid Extended Key Usage constraint')
        self.assertEqual(len(certProps['SubjectKeyId']), 8, 'Invalid Subject Key Id length (expected 8)');
        self.assertEqual(certProps['SubjectKeyId'], computeTruncatedKeyId(certProps['PublicKey']), 'SubjectKeyId does not match public key');
        self.assertEqual(rootCAKeyId_prime256v1, certProps['AuthorityKeyId'], 'AuthorityKeyId does not match root certificate');


class WeaveToolTestResult(unittest.TestResult):
    
    def _exc_info_to_string(self, err, test):
        return "%s\n%s\n" % (
            unittest.TestResult._exc_info_to_string(self, err, test),
            test.getCommandSummary()
        )


# ================================================================================
# Test Cases
# ================================================================================

class TEST01_PrintCert(WeaveToolTestCase):

    def test(self):
        '''Test the weave print-cert command'''
        inFile = InFileArg('cert.weave', deviceCert_Weave)
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'print-cert', inFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertRegexpMatches(stdout, r'''Subject:\s+WeaveDeviceId=18B4300000000001''')
        self.assertRegexpMatches(stdout, r'''Issuer:\s+WeaveCAId=18B430EEEE000002''')
        self.assertRegexpMatches(stdout, r'''Subject\s+Key\s+Id:\s+4EFF4751E4C6639B''')
        self.assertRegexpMatches(stdout, r'''Authority\s+Key\s+Id:\s+44E34038A9D4B5A7''')
        self.assertRegexpMatches(stdout, r'''Not\s+Before:\s+2013/10/22''')
        self.assertRegexpMatches(stdout, r'''Not\s+After:\s+2023/10/20''')
        self.assertRegexpMatches(stdout, r'''Key\s+Usage:\s+DigitalSignature\s+KeyEncipherment''')
        self.assertRegexpMatches(stdout, r'''Key\s+Purpose:\s+ServerAuth\s+ClientAuth''')
        self.assertRegexpMatches(stdout, r'''Public\s+Key\s+Algorithm:\s+ECPublicKey''')
        self.assertRegexpMatches(stdout, r'''Signature\s+Algorithm:\s+ECDSAWithSHA256''')
        self.assertRegexpMatches(stdout, r'''Curve\s+Id:\s+secp224r1''')
        self.assertRegexpMatches(stdout, r'''Public\s+Key:\s+04 EF 67 9D 53 0C 99 FF 9D 72 42 B1 F9 B6 60 20''')
        self.assertRegexpMatches(stdout, r'''r:\s+20 B7 5A 29 DE C1 72 A9 CB 01 E1 61 49 1A 04 2E''')
        self.assertRegexpMatches(stdout, r'''s:\s+00 A2 96 60 C1 B9 B8 E0 FA 4D FA 66 29 B1 10 AA''')

class TEST02_ConvertCert(WeaveToolTestCase):
    '''Test the weave convert-cert command'''

    def test_WeaveToPEM(self):
        '''Test certificate conversion: Weave to PEM'''
        inFile = InFileArg('cert.weave', deviceCert_Weave)
        outFile = OutFileArg('cert.pem')
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-cert', '--x509', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceCert_PEM)

    def test_WeaveToDER(self):
        '''Test certificate conversion: Weave to DER'''
        inFile = InFileArg('cert.weave', deviceCert_Weave)
        outFile = OutFileArg('cert.der') 
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-cert', '--x509-der', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceCert_DER)

    def test_PEMToWeave(self):
        '''Test certificate conversion: PEM to Weave'''
        inFile = InFileArg('cert.pem', deviceCert_PEM)
        outFile = OutFileArg('cert.weave')
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-cert', '--weave', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceCert_Weave)

    def test_DERToWeave(self):
        '''Test certificate conversion: DER to Weave'''
        inFile = InFileArg('cert.pem', deviceCert_DER)
        outFile = OutFileArg('cert.weave')
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-cert', '--weave', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceCert_Weave)

class TEST03_ConvertKey(WeaveToolTestCase):
    '''Test the weave convert-key command'''

    def test_WeaveToPKCS8PEM(self):
        '''Test private key conversion: Weave to PKCS8 PEM'''
        inFile = InFileArg('key.weave', deviceKey_Weave)
        outFile = OutFileArg('key.pem') 
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-key', '--pkcs8-pem', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceKey_PKCS8_PEM)

    def test_WeaveToPKCS8DER(self):
        '''Test private key conversion: Weave to PKCS8 DER'''
        inFile = InFileArg('key.weave', deviceKey_Weave)
        outFile = OutFileArg('key.pem') 
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-key', '--pkcs8-der', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceKey_PKCS8_DER)

    def test_WeaveToSEC1PEM(self):
        '''Test private key conversion: Weave to SEC1 PEM'''
        inFile = InFileArg('key.weave', deviceKey_Weave)
        outFile = OutFileArg('key.pem') 
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-key', '--pem', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceKey_SEC1_PEM)

    def test_WeaveToSEC1DER(self):
        '''Test private key conversion: Weave to SEC1 DER'''
        inFile = InFileArg('key.weave', deviceKey_Weave)
        outFile = OutFileArg('key.pem') 
        (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'convert-key', '--der', inFile, outFile ])
        self.assertEqual(res, 0, 'Command returned %d' % res)
        self.assertEqual(stderr, '', 'Text in stderr')
        self.assertEqual(outFile.contents, deviceKey_SEC1_DER)
  
class TEST04_GenCACert(WeaveToolTestCase):
    '''Test the weave gen-ca-cert command'''

    def test_SelfSigned(self):
        '''Test generation of self-signed CA certificates'''
        
        for curve, hash in itertools.product(allCurves, allSignatureHashes):

            self.clearTestContext()
            self.clearTmpFiles()
            
            # Invoke weave gen-ca-cert command to generate a self signed CA cert
            keyFile = InFileArg('key.pem', ecPrivKeys[curve])
            certFile = OutFileArg('cert.pem')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-ca-cert',
                    '--id', '18B430EEEE000001',
                    '--self',
                    '--' + hash,
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--key', keyFile,
                    '--out', certFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            # Decode the certificate                
            certProps = self.parseX509Cert(certFile.contents)
            
            # Verify the certificate's construction
            self.assertEqual(certProps['Version'], '3 (0x2)', 'Invalid certificate version')
            self.assertEqual(certProps['Subject'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate subject')
            self.assertEqual(certProps['Issuer'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate issuer')
            self.assertEqual(certProps['Curve'], curve, 'Invalid public key curve');
            self.assertEqual(certProps['SignatureAlgorithm'], 'ecdsa-with-%s' % (hash.upper()), 'Invalid signature algo');
            self.assertEqual(certProps['NotBefore'], 'Jan  1 00:00:00 2018 GMT', 'Invalid NotBefore date');
            self.assertEqual(certProps['NotAfter'], 'Dec 31 23:59:59 2018 GMT', 'Invalid NotAfter date');
            self.assertIn('CA:TRUE', certProps['BasicConstraints'], 'Invalid CA constraint')
            self.assertItemsEqual(['Certificate Sign', 'CRL Sign' ], certProps['KeyUsage'], 'Invalid Key Usage constraint')
            self.assertIsNone(certProps['ExtendedKeyUsage'], 'Unexpected Extended Key Usage constraint')
            self.assertEqual(len(certProps['SubjectKeyId']), 8, 'Invalid Subject Key Id length (expected 8)');
            self.assertEqual(certProps['SubjectKeyId'], computeTruncatedKeyId(certProps['PublicKey']), 'SubjectKeyId does not match public key');
            self.assertEqual(certProps['SubjectKeyId'], certProps['AuthorityKeyId'], 'SubjectKeyId does not match AuthorityKeyId');

    def test_RootSigned(self):
        '''Test generation of CA certificates signed by a root certificate'''
        
        for curve, hash in itertools.product(allCurves, allSignatureHashes):

            self.clearTestContext()
            self.clearTmpFiles()
            
            # Invoke weave gen-ca-cert command to generate a CA cert signed by a root cert/key
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            keyFile = InFileArg('key.pem', ecPrivKeys[curve])
            certFile = OutFileArg('cert.pem')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-ca-cert',
                    '--id', '18B430EEEE000002',
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--key', keyFile,
                    '--' + hash,
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--out', certFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            # Decode the certificate                
            certProps = self.parseX509Cert(certFile.contents)
            
            # Verify the certificate's construction
            self.assertEqual(certProps['Version'], '3 (0x2)', 'Invalid certificate version')
            self.assertEqual(certProps['Subject'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000002', 'Invalid certificate subject')
            self.assertEqual(certProps['Issuer'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate issuer')
            self.assertEqual(certProps['Curve'], curve, 'Invalid public key curve');
            self.assertEqual(certProps['SignatureAlgorithm'], 'ecdsa-with-%s' % (hash.upper()), 'Invalid signature algo');
            self.assertEqual(certProps['NotBefore'], 'Jan  1 00:00:00 2018 GMT', 'Invalid NotBefore date');
            self.assertEqual(certProps['NotAfter'], 'Dec 31 23:59:59 2018 GMT', 'Invalid NotAfter date');
            self.assertIn('CA:TRUE', certProps['BasicConstraints'], 'Invalid CA constraint')
            self.assertItemsEqual(['Certificate Sign', 'CRL Sign' ], certProps['KeyUsage'], 'Invalid Key Usage constraint')
            self.assertIsNone(certProps['ExtendedKeyUsage'], 'Unexpected Extended Key Usage constraint')
            self.assertEqual(len(certProps['SubjectKeyId']), 8, 'Invalid Subject Key Id length (expected 8)');
            self.assertEqual(certProps['SubjectKeyId'], computeTruncatedKeyId(certProps['PublicKey']), 'SubjectKeyId does not match public key');
            self.assertEqual(rootCAKeyId_prime256v1, certProps['AuthorityKeyId'], 'AuthorityKeyId does not match root certificate');

class TEST05_GetDeviceCert(WeaveToolTestCase):
    '''Test the weave gen-device-cert command'''

    def test_ExistingKey(self):
        '''Test generation of a device certificate using an existing private key file'''
        
        for curve, hash in itertools.product(allCurves, allSignatureHashes):

            self.clearTestContext()
            self.clearTmpFiles()
            
            # Invoke weave gen-device-cert command to generate a device certificate
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            keyFile = InFileArg('key.pem', ecPrivKeys[curve])
            certFile = OutFileArg('cert.weave.b64')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-device-cert',
                    '--dev-id', '18B430EEEE000002',
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--' + hash,
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--key', keyFile,
                    '--out', certFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            # Decode the certificate                
            certProps = self.parseWeaveCert(certFile.contents)
            
            # Verify the certificate's construction
            self.assertEqual(certProps['Version'], '3 (0x2)', 'Invalid certificate version')
            self.assertEqual(certProps['Subject'], '1.3.6.1.4.1.41387.1.1=18B430EEEE000002', 'Invalid certificate subject')
            self.assertEqual(certProps['Issuer'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate issuer')
            self.assertEqual(certProps['Curve'], curve, 'Invalid public key curve');
            self.assertEqual(certProps['SignatureAlgorithm'], 'ecdsa-with-%s' % (hash.upper()), 'Invalid signature algo');
            self.assertEqual(certProps['NotBefore'], 'Jan  1 00:00:00 2018 GMT', 'Invalid NotBefore date');
            self.assertEqual(certProps['NotAfter'], 'Dec 31 23:59:59 2018 GMT', 'Invalid NotAfter date');
            self.assertIn('CA:FALSE', certProps['BasicConstraints'], 'Invalid CA constraint')
            self.assertItemsEqual(['Digital Signature', 'Key Encipherment' ], certProps['KeyUsage'], 'Invalid Key Usage constraint')
            self.assertItemsEqual(['TLS Web Client Authentication', 'TLS Web Server Authentication' ], certProps['ExtendedKeyUsage'], 'Invalid Extended Key Usage constraint')
            self.assertEqual(len(certProps['SubjectKeyId']), 8, 'Invalid Subject Key Id length (expected 8)');
            self.assertEqual(certProps['SubjectKeyId'], computeTruncatedKeyId(certProps['PublicKey']), 'SubjectKeyId does not match public key');
            self.assertEqual(rootCAKeyId_prime256v1, certProps['AuthorityKeyId'], 'AuthorityKeyId does not match root certificate');

    def test_GenerateKey(self):
        '''Test generation of a device certificate and a corresponding private key'''
        
        for curve, hash in itertools.product(allCurves, allSignatureHashes):

            self.clearTestContext()
            self.clearTmpFiles()
            
            # Invoke weave gen-device-cert command to generate a device certificate
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            certFile = OutFileArg('cert.weave.b64')
            keyFile = OutFileArg('key.weave.b64')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-device-cert',
                    '--dev-id', '18B430EEEE000002',
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--curve', curve,
                    '--' + hash,
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--out-key', keyFile,
                    '--out', certFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            # Decode the certificate                
            certProps = self.parseWeaveCert(certFile.contents)
            
            # Verify the certificate's construction
            self.assertEqual(certProps['Version'], '3 (0x2)', 'Invalid certificate version')
            self.assertEqual(certProps['Subject'], '1.3.6.1.4.1.41387.1.1=18B430EEEE000002', 'Invalid certificate subject')
            self.assertEqual(certProps['Issuer'], '1.3.6.1.4.1.41387.1.3=18B430EEEE000001', 'Invalid certificate issuer')
            self.assertEqual(certProps['Curve'], curve, 'Invalid public key curve');
            self.assertEqual(certProps['SignatureAlgorithm'], 'ecdsa-with-%s' % (hash.upper()), 'Invalid signature algo');
            self.assertEqual(certProps['NotBefore'], 'Jan  1 00:00:00 2018 GMT', 'Invalid NotBefore date');
            self.assertEqual(certProps['NotAfter'], 'Dec 31 23:59:59 2018 GMT', 'Invalid NotAfter date');
            self.assertIn('CA:FALSE', certProps['BasicConstraints'], 'Invalid CA constraint')
            self.assertItemsEqual(['Digital Signature', 'Key Encipherment' ], certProps['KeyUsage'], 'Invalid Key Usage constraint')
            self.assertItemsEqual(['TLS Web Client Authentication', 'TLS Web Server Authentication' ], certProps['ExtendedKeyUsage'], 'Invalid Extended Key Usage constraint')
            self.assertEqual(len(certProps['SubjectKeyId']), 8, 'Invalid Subject Key Id length (expected 8)');
            self.assertEqual(certProps['SubjectKeyId'], computeTruncatedKeyId(certProps['PublicKey']), 'SubjectKeyId does not match public key');
            self.assertEqual(rootCAKeyId_prime256v1, certProps['AuthorityKeyId'], 'AuthorityKeyId does not match root certificate');

            expectedPubKey = self.privateKeyToPublicKey(keyFile.contents, 'weave')
            self.assertEqual(certProps['PublicKey'], expectedPubKey, 'Public Key in certificate does not match generated private key')

class TEST06_GenProvisioningData(WeaveToolTestCase):
    '''Test the weave gen-provisioning-data command'''

    def test(self):
        '''Test generation of device provisioning data'''
        
        allCertFormats = [ 'weave', 'der' ]
        allKeyFormats = [ 'weave', 'der', 'pkcs8' ]
        deviceCount = 5

        for curve, hash, certFormat, keyFormat in itertools.product(allCurves, allSignatureHashes, allCertFormats, allKeyFormats):

            self.clearTestContext()
            self.clearTmpFiles()
            
            # Invoke weave gen-provisioning-data command
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            provDataFile = OutFileArg('prov-data.csv')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-provisioning-data',
                    '--dev-id', '18B4300000000001',
                    '--count', deviceCount,
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--curve', curve,
                    '--' + hash,
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--' + certFormat + '-cert',
                    '--' + keyFormat + '-key',
                    '--out', provDataFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')
            
            # Verify the contents of the generated provisioning data file
            self.verifyProvisioningDataFile(provDataFile.name, deviceCount, curve, hash, certFormat, keyFormat)

class TEST07_ConvertProvisioningData(WeaveToolTestCase):
    '''Test the weave convert-provisioning-data command'''

    def test_CertificateConversion(self):
        '''Test conversion of certificate formats in a device provisioning data file'''
        
        allCertFormats = [ 'weave', 'der' ]
        
        deviceCount = 5
        curve = 'prime256v1'
        hash = 'sha256'
        keyFormat = 'weave'
        
        for fromKeyFormat in allCertFormats:
            
            self.clearTestContext()
            self.clearTmpFiles()

            # Invoke weave gen-provisioning-data command
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            fromProvDataFile = OutFileArg('from-prov-data.csv')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-provisioning-data',
                    '--dev-id', '18B4300000000001',
                    '--count', deviceCount,
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--curve', curve,
                    '--sha256',
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--' + fromKeyFormat + '-cert',
                    '--' + keyFormat + '-key',
                    '--out', fromProvDataFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            for toKeyFormat in allCertFormats:

                # Invoke weave convert-provisioning-data command
                toProvDataFile = OutFileArg('to-prov-data.csv')
                (res, stdout, stderr) = self.runCommand(args.weaveTool,
                    args=[
                        'convert-provisioning-data',
                        '--' + toKeyFormat + '-cert',
                        fromProvDataFile,
                        toProvDataFile
                    ])
                
                # Check for errors or unexpected output.
                self.assertEqual(res, 0, 'Command returned %d' % res)
                self.assertEqual(stderr, '', 'Text in stdout')
                self.assertEqual(stderr, '', 'Text in stderr')
            
                # Verify the contents of the generated provisioning data file
                self.verifyProvisioningDataFile(toProvDataFile.name, deviceCount, curve, hash, toKeyFormat, keyFormat)

    def test_KeyConversion(self):
        '''Test conversion of private key formats in a device provisioning data file'''
        
        allKeyFormats = [ 'weave', 'der', 'pkcs8' ]
        
        deviceCount = 5
        curve = 'prime256v1'
        hash = 'sha256'
        certFormat = 'weave'
        
        for fromKeyFormat in allKeyFormats:
            
            self.clearTestContext()
            self.clearTmpFiles()

            # Invoke weave gen-provisioning-data command
            caCertFile = InFileArg('ca-cert.pem', rootCACert_prime256v1_sha256)
            caKeyFile = InFileArg('ca-key.pem', rootCAKey_prime256v1)
            fromProvDataFile = OutFileArg('from-prov-data.csv')
            (res, stdout, stderr) = self.runCommand(args.weaveTool,
                args=[
                    'gen-provisioning-data',
                    '--dev-id', '18B4300000000001',
                    '--count', deviceCount,
                    '--ca-cert', caCertFile,
                    '--ca-key', caKeyFile,
                    '--curve', curve,
                    '--sha256',
                    '--valid-from', '2018/01/01',
                    '--lifetime', '365',
                    '--' + certFormat + '-cert',
                    '--' + fromKeyFormat + '-key',
                    '--out', fromProvDataFile
                ])
            
            # Check for errors or unexpected output.
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stdout')
            self.assertEqual(stderr, '', 'Text in stderr')

            for toKeyFormat in allKeyFormats:

                # Invoke weave convert-provisioning-data command
                toProvDataFile = OutFileArg('to-prov-data.csv')
                (res, stdout, stderr) = self.runCommand(args.weaveTool,
                    args=[
                        'convert-provisioning-data',
                        '--' + toKeyFormat + '-key',
                        fromProvDataFile,
                        toProvDataFile
                    ])
                
                # Check for errors or unexpected output.
                self.assertEqual(res, 0, 'Command returned %d' % res)
                self.assertEqual(stderr, '', 'Text in stdout')
                self.assertEqual(stderr, '', 'Text in stderr')
            
                # Verify the contents of the generated provisioning data file
                self.verifyProvisioningDataFile(toProvDataFile.name, deviceCount, curve, hash, certFormat, toKeyFormat)

class TEST08_PrintSig(WeaveToolTestCase):

    def findField(self, fieldNamePattern, inStr, searchStart=0, searchEnd=None):
        if searchEnd == None:
            searchEnd = len(inStr)
        fieldHeaderRE = re.compile(r'''^(\s+)''' + fieldNamePattern + r''':\s*''', re.MULTILINE)
        fieldHeader = fieldHeaderRE.search(inStr, searchStart, searchEnd)
        if fieldHeader:
            fieldIndent = str(fieldHeader.group(1))
            fieldEndRE = re.compile(r'''^''' + fieldIndent + r'''[^\s]''', re.MULTILINE)
            fieldEnd = fieldEndRE.search(inStr, fieldHeader.end())
            if fieldEnd:
                return (fieldHeader.start(), fieldEnd.start())
            else:
                return (fieldHeader.start(), searchEnd)
        else:
            return (None, None)
        
    def extractRelatedCerts(self, output):
        
        (relatedCertsStart, relatedCertsEnd) = self.findField('Related Certificates', output)
        if relatedCertsStart != None:
            remainingOutput = output[:relatedCertsStart] + output[relatedCertsEnd:]
            relatedCerts = []
            nextSearchStart = relatedCertsStart
            while True:
                (certStart, certEnd) = self.findField('Certificate \d+', output, nextSearchStart, relatedCertsEnd)
                if certStart != None:
                    relatedCerts.append(output[certStart:certEnd])
                    nextSearchStart = certEnd
                else:
                    break
        else:
            remainingOutput = output
            relatedCerts = None
        
        return (relatedCerts, remainingOutput)
        
    def test_SigningCert(self):
        '''Test the weave print-sig command on signature object that includes a set of related signing certificates'''
        
        inFormats = [ 'tlv', 'base64' ]
        
        for inFormat in inFormats:
            
            if inFormat == 'base64':
                inFile = InFileArg('sig.b64', base64.b64encode(weaveSig_ecdsaWithSHA1_signingCert))
            else:
                inFile = InFileArg('sig.weave', weaveSig_ecdsaWithSHA1_signingCert)
        
            (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'print-sig', inFile ])
            
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stderr')
            
            (relatedCerts, remainingOutput) = self.extractRelatedCerts(stdout)
            
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+Signature\s+Algorithm:\s+ECDSAWithSHA1\s+\(implicit\)\s*$''')
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+r:\s+00 C6 8B 71 90 4F 96 D7 1C ED E7 C9 42 B6 D9 50\s+63 E3 D4 3E F7 0D 66 9F FA C7 0F 29 DD\s*$''')
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+s:\s+00 8B 38 72 23 4F 29 EE B9 87 BA 87 67 2C 9B 4F\s+FF 47 AC C1 4A 54 E5 0B A5 4C 05 E5 D9\s*$''')

            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Subject:\s+WeaveSoftwarePublisherId=18B43003EE000001''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Issuer:\s+WeaveCAId=18B430EEEE000004''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Subject\s+Key\s+Id:\s+4726DD889EFBE8BF\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Authority\s+Key\s+Id:\s+4C8E97192EBCF8ED\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Not\s+Before:\s+2013/10/11\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Not\s+After:\s+2023/07/15\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Key\s+Usage:\s+DigitalSignature\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Key\s+Purpose:\s+CodeSigning\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Public\s+Key\s+Algorithm:\s+ECPublicKey\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Signature\s+Algorithm:\s+ECDSAWithSHA1\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Curve\s+Id:\s+secp224r1\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+Public\s+Key:\s+04 C8 6C 57 99 6F ED 75 9C 2A 40 50 43 74 AE AB\s*57 42 6E 59 18 4C 33 85 B8 90 4B 5E 35 A5 46 FA\s*96 04 7A D6 E9 4D 59 3F C5 03 86 47 C1 93 88 73\s*B9 CD 4C C6 06 A3 91 A7 19\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+r:\s+23 ED 40 10 35 91 84 7F AA 12 E5 BD 9F FC F2 F9\s*02 16 8F DA 07 AC 99 4B 83 BA 71 E9\s*$''')
            self.assertRegexpMatches(relatedCerts[0], r'''(?m)^\s+s:\s+00 A9 C8 EA AF BD 4F 1B F1 28 0B 4A E3 4F C8 CA\s*FA D1 30 D3 B7 0B 27 CF DD E7 DB 33 BA\s*$''')

            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Subject:\s+WeaveCAId=18B430EEEE000004''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Issuer:\s+WeaveCAId=18B430EEEE000001''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Subject\s+Key\s+Id:\s+4C8E97192EBCF8ED\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Authority\s+Key\s+Id:\s+4AAA7BA47A614B2D\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Not\s+Before:\s+2013/10/11\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Not\s+After:\s+2043/10/16\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Is\s+CA:\s+true\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Key\s+Usage:\s+KeyCertSign\s+CRLSign\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Public\s+Key\s+Algorithm:\s+ECPublicKey\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Signature\s+Algorithm:\s+ECDSAWithSHA1\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Curve\s+Id:\s+secp224r1\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+Public\s+Key:\s+04 37 6E 80 C6 28 1A 00 55 27 C9 9F 50 86 AB 71\s*7A 99 6C DD DB 95 42 C2 24 37 7C 76 9B 81 A9 F0\s*AE 30 4E 10 62 E7 58 1C 73 D2 8E 67 AC 41 B5 E4\s*3D 19 06 50 58 87 01 55 CC\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+r:\s+00 AB 31 C0 C7 E4 E6 16 D6 67 B4 D5 77 EC 67 04\s*C6 DE 28 05 4B F5 C9 2A 54 ED 7A DB C0\s*$''')
            self.assertRegexpMatches(relatedCerts[1], r'''(?m)^\s+s:\s+00 F0 B8 30 73 00 C0 DD DF 93 45 B5 EC 4D 1A 78\s*5A ED A2 F1 20 72 C2 7C 1A B7 CD 0C 00\s*$''')

    def test_CertRef(self):
        '''Test the weave print-sig command on signature object that includes a certificate reference'''
        
        inFormats = [ 'tlv', 'base64' ]
        
        for inFormat in inFormats:
            
            if inFormat == 'base64':
                inFile = InFileArg('sig.b64', base64.b64encode(weaveSig_ecdsaWithSHA256_certRef))
            else:
                inFile = InFileArg('sig.weave', weaveSig_ecdsaWithSHA256_certRef)
        
            (res, stdout, stderr) = self.runCommand(args.weaveTool, [ 'print-sig', inFile ])
            
            self.assertEqual(res, 0, 'Command returned %d' % res)
            self.assertEqual(stderr, '', 'Text in stderr')
            
            (relatedCerts, remainingOutput) = self.extractRelatedCerts(stdout)
            
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+Signature\s+Algorithm:\s+ECDSAWithSHA256\s*$''')
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+r:\s+4D 0F C7 61 00 34 BF 6D 0F D1 B8 2B CD 8C 79 25\s*07 8A 1A 2A 8B D9 E1 A8 9C 5A D0 9C\s*$''')
            self.assertRegexpMatches(remainingOutput, r'''(?m)^\s+s:\s+2D 81 E4 D9 4E 76 69 89 7F FE 79 9E F5 52 14 61\s*9E 32 9F 46 9F FC 6C 7F A2 A5 86 4C\s*$''')

            
if __name__ == '__main__':
    
    argParser = argparse.ArgumentParser(description='Script for testing the weave tool')
    argParser.add_argument('--run', required=False, dest='runTests', help='List of tests to be run, separated by commas.  Defaults to all.')
    argParser.add_argument('--weave-root', required=False, dest='weaveRoot', help='Path to the root of the weave source tree')
    argParser.add_argument('--weave-tool', required=False, dest='weaveTool', help='Path to the weave tool executable to be tested')
    argParser.add_argument('--openssl', required=False, dest='opensslTool', help='Path to the openssl executable.')
    
    args = argParser.parse_args()
    
    # Locate the root of the Weave source tree.
    # If specified on the command line, use that.
    # If not, look for the env var 'abs_top_srcdir', passed by the Weave test-apps
    # Makefile.
    # Finally, infer the location of the source directory based on the location
    # of the TestWeaveTool script.
    if not args.weaveRoot:
        args.weaveRoot = os.environ.get('abs_top_srcdir', None)
    if not args.weaveRoot:
        scriptDir = os.path.dirname(os.path.abspath(os.path.realpath(__file__)))
        args.weaveRoot = os.path.abspath(os.path.join(scriptDir, '../..'))

    # Locate the target-specific Weave build directory. 
    buildDir = os.environ.get('abs_top_builddir', None)
    if not buildDir:
        configGuessTool = os.path.join(args.weaveRoot, './third_party/nlbuild-autotools/repo/third_party/autoconf/config.guess')
        targetDirName = subprocess.check_output([ configGuessTool ]).strip()
        buildDir = os.path.join(args.weaveRoot, 'build', targetDirName)
    
    # Locate the weave tool executable.  If not specified on the command line,
    # look for it in the target-specific Weave build directory. 
    if not args.weaveTool:
        args.weaveTool = os.path.join(buildDir, 'src/tools/weave/weave')
    if not os.path.isfile(args.weaveTool) or not os.access(args.weaveTool, os.X_OK):
        sys.stderr.write('ERROR: weave tool not found\n')
        sys.stderr.write('Expected location: %s\n' % args.weaveTool)
        sys.exit(-1)
    
    # If not specified on the command line, locate the openssl tool in the user's PATH.
    if not args.opensslTool:
        args.opensslTool = next(
            (fileName for fileName in
                (os.path.join(path, 'openssl') for path in os.environ["PATH"].split(os.pathsep))
                if os.path.isfile(fileName) and os.access(fileName, os.X_OK)),
            None)
    if not args.opensslTool or not os.path.isfile(args.opensslTool) or not os.access(args.opensslTool, os.X_OK):
        sys.stderr.write('ERROR: openssl tool not found\n')
        sys.exit(-1)

    # Import the Verhoeff module from the Weave source tree
    verhoeffDir = os.path.join(args.weaveRoot, 'src/lib/support/verhoeff')
    if os.path.isdir(verhoeffDir):
        sys.path.append(verhoeffDir)
    import Verhoeff
    
    print 'TestWeaveTool'
    
    tests = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    
    testResult = WeaveToolTestResult()
    testResult.failfast = True
    
    testResult.startTestRun()
    try:
        tests(testResult)
    finally:
        testResult.stopTestRun()

    for test, error in testResult.errors:
        sys.stderr.write('TEST ERROR: %s\n%s' % (test, error))
        
    for test, error in testResult.failures:
        sys.stderr.write('TEST FAILED: %s (%s)\n%s' % (test.id(), test.shortDescription(), error))

    sys.exit(0 if testResult.wasSuccessful() else 1)

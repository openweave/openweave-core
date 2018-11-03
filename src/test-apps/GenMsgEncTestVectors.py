#!/usr/bin/python

""" Generate test vectors for testing Weave message encoding/decoding.
"""

import sys
import os
import struct
import binascii
import itertools
import random
from Cryptodome.Cipher import AES
from Cryptodome.Hash import HMAC, SHA1, SHA256

kMsgHeaderField_FlagsMask               = 0x0F0F
kMsgHeaderField_FlagsShift              = 0
kMsgHeaderField_EncryptionTypeMask      = 0x00F0
kMsgHeaderField_EncryptionTypeShift     = 4
kMsgHeaderField_MessageVersionMask      = 0xF000
kMsgHeaderField_MessageVersionShift     = 12

kWeaveHeaderFlag_DestNodeId             = 0x0100
kWeaveHeaderFlag_SourceNodeId           = 0x0200
kWeaveHeaderFlag_TunneledData           = 0x0400
kWeaveHeaderFlag_MsgCounterSyncReq      = 0x0800

kMsgHeaderField_IntegrityCheckMask      = ~((kWeaveHeaderFlag_DestNodeId |
                                             kWeaveHeaderFlag_SourceNodeId |
                                             kWeaveHeaderFlag_MsgCounterSyncReq) << kMsgHeaderField_FlagsShift)

kWeaveEncryptionType_None               = 0
kWeaveEncryptionType_AES128CTRSHA1      = 1
kWeaveEncryptionType_AES128EAX64        = 2
kWeaveEncryptionType_AES128EAX128       = 3

kWeaveMessageVersion_V1                 = 1
kWeaveMessageVersion_V2                 = 2

kWeaveKeyType_Session                   = 0x00002000

kNodeIdNotSpecified                     = 0
kAnyNodeId                              = 0xFFFFFFFFFFFFFFFF

def toCArrayInitializer(str, indent=''):
    formattedStr = ''
    i = 0
    for ch in str:
        if i == 0:
            formattedStr = indent
        elif i % 16 == 0:
            formattedStr += ('\n' + indent)
        else:
            formattedStr += ' '
        formattedStr += ('0x%02x,' % (ord(ch)))
        i += 1
    return formattedStr

def messageFlagsToCExpr(flags):
    flagNames = [
        (kWeaveHeaderFlag_DestNodeId, 'kWeaveHeaderFlag_DestNodeId'),
        (kWeaveHeaderFlag_SourceNodeId, 'kWeaveHeaderFlag_SourceNodeId'),
        (kWeaveHeaderFlag_TunneledData, 'kWeaveHeaderFlag_TunneledData'),
        (kWeaveHeaderFlag_MsgCounterSyncReq, 'kWeaveHeaderFlag_MsgCounterSyncReq')
    ]
    flagsExp = []
    for (flag, name) in flagNames:
        if (flag & flags) != 0:
            flagsExp.append(name)
    return '|'.join(flagsExp) if len(flagsExp) != 0 else '0'

def messageVersionToCExpr(version):
    if version == kWeaveMessageVersion_V1:
        return 'kWeaveMessageVersion_V1'
    if version == kWeaveMessageVersion_V2:
        return 'kWeaveMessageVersion_V2'
    raise ValueError('Invalid argument')

def messageEncTypeToCExpr(encType):
    if encType == kWeaveEncryptionType_None:
        return 'kWeaveEncryptionType_None'
    if encType == kWeaveEncryptionType_AES128CTRSHA1:
        return 'kWeaveEncryptionType_AES128CTRSHA1'
    if encType == kWeaveEncryptionType_AES128EAX128:
        return 'kWeaveEncryptionType_AES128EAX128'
    if encType == kWeaveEncryptionType_AES128EAX64:
        return 'kWeaveEncryptionType_AES128EAX64'
    raise ValueError('Invalid argument')

class UnsupportedMessageEncoding(Exception):
    pass

class MessageEncodingTestVector():
    def __init__(self, sourceNodeId=0x18B4300000000001, destNodeId=0x18B4300000000002, flags=0,
                 msgId=None, version=kWeaveMessageVersion_V2, encType=kWeaveEncryptionType_None,
                 keyId=None, msgPayloadLen=16):
        self.sourceNodeId = sourceNodeId
        self.destNodeId = destNodeId
        if msgId == None:
            msgId = random.randint(0, 0xFFFFFFFF)
        self.msgId = msgId
        self.flags = flags
        self.version = version
        self.encType = encType
        if keyId == None:
            if encType != kWeaveEncryptionType_None:
                keyId = kWeaveKeyType_Session | random.randint(0, 0xFFF)
            else:
                keyId = 0
        self.keyId = keyId
        
        if encType == kWeaveEncryptionType_AES128CTRSHA1:
            self.dataKey = os.urandom(16)
            self.integrityKey = os.urandom(20)
        else:
            self.dataKey = os.urandom(16)

        self.msgPayload = os.urandom(msgPayloadLen)
        
    def encodeMessage(self):
    
        if self.version == kWeaveMessageVersion_V1:
            if (self.flags & kWeaveHeaderFlag_TunneledData) != 0:
                raise UnsupportedMessageEncoding('Unsupported message format')
        
        if self.sourceNodeId == kAnyNodeId:
            raise UnsupportedMessageEncoding('Invalid source node id')
        
        if self.sourceNodeId == self.destNodeId:
            raise UnsupportedMessageEncoding('Invalid source/destination node id combination')
            
        if (self.flags & kWeaveHeaderFlag_TunneledData) != 0 and (self.flags & kWeaveHeaderFlag_MsgCounterSyncReq) != 0:
            raise UnsupportedMessageEncoding('Unsupported message format')
        
        headerVal = (((self.flags   & 0xF0F) << 0) |
                     ((self.encType & 0xF)   << 4) |
                     ((self.version & 0xF)   << 12))
        
        self.encodedMsg = struct.pack('<H', headerVal)
        self.encodedMsg += struct.pack('<I', self.msgId)
        
        if (self.flags & kWeaveHeaderFlag_SourceNodeId) != 0:
            self.encodedMsg += struct.pack('<Q', self.sourceNodeId)
        if (self.flags & kWeaveHeaderFlag_DestNodeId) != 0:
            self.encodedMsg += struct.pack('<Q', self.destNodeId)
    
        if self.encType != kWeaveEncryptionType_None:
            self.encodedMsg += struct.pack('<H', self.keyId)
    
        encryptedDataStart = len(self.encodedMsg)
            
        self.encodedMsg += self.msgPayload
            
        if self.encType == kWeaveEncryptionType_AES128CTRSHA1:
            
            pseudoHeader = self.encodeIntegrityCheckPseudoHeader(headerVal)

            hmac = HMAC.new(self.integrityKey, digestmod=SHA1)
            hmac.update(pseudoHeader)
            hmac.update(self.msgPayload)
            tag = hmac.digest()
            
            self.encodedMsg += tag
    
            counter  = struct.pack('>Q', self.sourceNodeId)
            counter += struct.pack('>I', self.msgId)
            counter += struct.pack('>I', 0)
            
            cypher = AES.new(self.dataKey, AES.MODE_CTR, nonce='', initial_value=counter)
            
            encryptedData = cypher.encrypt(self.encodedMsg[encryptedDataStart:])
            
            self.encodedMsg = self.encodedMsg[0:encryptedDataStart] + encryptedData
            
        elif self.encType == kWeaveEncryptionType_AES128EAX128 or self.encType == kWeaveEncryptionType_AES128EAX64:
            
            pseudoHeader = self.encodeIntegrityCheckPseudoHeader(headerVal)
    
            nonce  = struct.pack('>Q', self.sourceNodeId)
            nonce += struct.pack('>I', self.msgId)
            
            tagLen = 16 if self.encType == kWeaveEncryptionType_AES128EAX128 else 8
            
            cypher = AES.new(self.dataKey, AES.MODE_EAX, nonce=nonce, mac_len=tagLen)
            cypher.update(pseudoHeader)
            (encryptedData, tag) = cypher.encrypt_and_digest(self.encodedMsg[encryptedDataStart:])
            
            self.encodedMsg = self.encodedMsg[0:encryptedDataStart] + encryptedData + tag
        
    def encodeIntegrityCheckPseudoHeader(self, headerVal):
        
        pseudoHeader = struct.pack('<Q', self.sourceNodeId)
        pseudoHeader += struct.pack('<Q', self.destNodeId)
        
        if self.version != kWeaveMessageVersion_V1 or self.encType != kWeaveEncryptionType_AES128CTRSHA1:
            
            maskedHeaderVal = headerVal & kMsgHeaderField_IntegrityCheckMask
            pseudoHeader += struct.pack('<H', maskedHeaderVal)
            pseudoHeader += struct.pack('<I', self.msgId)
            
        return pseudoHeader
        
        
    def toCDecl(self, declPrefix):
        res = ''
        if self.encType == kWeaveEncryptionType_AES128CTRSHA1:
            res += (
                'static const WeaveEncryptionKey_AES128CTRSHA1 ' + declPrefix + '_EncryptionKey =\n' +
                '{\n' +
                '    {\n' +
                         toCArrayInitializer(self.dataKey, indent='        ') + '\n' +
                '    },\n' +
                '    {\n' +
                         toCArrayInitializer(self.integrityKey, indent='        ') + '\n' +
                '    }\n' +
                '};\n'
            )
        elif self.encType == kWeaveEncryptionType_AES128EAX128 or self.encType == kWeaveEncryptionType_AES128EAX64:
            res += (
                'static const WeaveEncryptionKey_AES128EAX '+ declPrefix + '_EncryptionKey =\n' +
                '{\n' +
                '    {\n' +
                         toCArrayInitializer(self.dataKey, indent='        ') + '\n' +
                '    }\n' +
                '};\n'
            )
        res += ('static const uint8_t ' + declPrefix + '_Payload[] =\n' +
                '{\n' +
                     toCArrayInitializer(self.msgPayload, indent='    ') + '\n' +
                '};\n\n'
        )
        res += ('static const uint8_t ' + declPrefix + '_ExpectedEncoding[] =\n' +
                '{\n' +
                     toCArrayInitializer(self.encodedMsg, indent='    ') + '\n' +
                '};\n\n'
        )
        res += ('static const MessageEncodingTestVector ' + declPrefix + ' =\n' +
                '{\n' +
                '    {\n' +
                '        0x%016X,\n' % (self.sourceNodeId) +
                '        0x%016X,\n' % (self.destNodeId) +
                '        0x%08X,\n' % (self.msgId) +
                '        %s,\n' % (messageFlagsToCExpr(self.flags)) +
                '        0x%04X,\n' % (self.keyId) +
                '        %s,\n' % (messageVersionToCExpr(self.version)) +
                '        %s,\n' % (messageEncTypeToCExpr(self.encType)) +
                '        0,\n' +
                '        NULL,\n' +
                '        NULL\n' +
                '    },\n'
        )
        if self.encType != kWeaveEncryptionType_None:
            res += (
                '    (const WeaveEncryptionKey *)&' + declPrefix + '_EncryptionKey,\n'
            )
        else:
            res += (
                '    NULL,\n'
            )
        res += ('    ' + declPrefix + '_Payload,\n' +
                '    sizeof(' + declPrefix + '_Payload),\n' +
                '    ' + declPrefix + '_ExpectedEncoding,\n' +
                '    sizeof(' + declPrefix + '_ExpectedEncoding)\n' +
                '};\n\n'
        )
        return res


testedVersions = [
    kWeaveMessageVersion_V1,
    kWeaveMessageVersion_V2
]

testedEncTypes = [
    kWeaveEncryptionType_None,
    kWeaveEncryptionType_AES128CTRSHA1,
    kWeaveEncryptionType_AES128EAX64,
    kWeaveEncryptionType_AES128EAX128
]

testedFlags = [ 
    (x[0] | x[1] | x[2] | x[3]) for x in itertools.product([ kWeaveHeaderFlag_DestNodeId, 0 ],
                                                           [ kWeaveHeaderFlag_SourceNodeId, 0 ],
                                                           [ kWeaveHeaderFlag_TunneledData, 0 ],
                                                           [ kWeaveHeaderFlag_MsgCounterSyncReq, 0 ])
]

testedNodeIds = [
    0x18B4300000000001,
    0x18B4300000000042,
    kAnyNodeId
]

testedKeyIds = [
    kWeaveKeyType_Session | 0x001,
    kWeaveKeyType_Session | 0x010,
    kWeaveKeyType_Session | 0x800,
    kWeaveKeyType_Session | 0x8FF,
    kWeaveKeyType_Session | 0xFFF,
]

testedMessageLengths = [
    1,
    10,
    50,
    128,
    1024
]

testedMessageIds = [
    0,
    1,
    1024,
    0xFFFFFFFF
]

testNum = 0

testVectorDeclName = 'sMessageEncodingTest%04d'

print '''\
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

/**
 *    @file
 *      Weave Message Encryption Test Vectors
 */

/*
 *    !!! WARNING !!! AUTO-GENERATED FILE !!! DO NOT EDIT !!!
 *
 *    This file has been generated by the gen-msg-enc-test-vectors.py script.
 */

struct MessageEncodingTestVector
{
    WeaveMessageInfo MsgInfo;
    const WeaveEncryptionKey * EncKey;
    const uint8_t * MsgPayload;
    uint16_t MsgPayloadLen;
    const uint8_t * ExpectedEncodedMsg;
    uint16_t ExpectedEncodedMsgLen;
};

'''


# Generate test vectors for various combinations of source / destination node ids,
# flags, message versions and encryption types. 
testInputs = itertools.product(testedNodeIds, testedNodeIds, testedFlags, testedVersions, testedEncTypes) 
for (sourceNodeId, destNodeId, flags, version, encType) in testInputs:

    testVector = MessageEncodingTestVector(
        sourceNodeId = sourceNodeId,
        destNodeId = destNodeId,
        flags = flags,
        version = version,
        encType = encType
    )

    try:
        testVector.encodeMessage()
    except UnsupportedMessageEncoding, ex:
        continue

    print testVector.toCDecl(testVectorDeclName % (testNum))
    
    testNum += 1

# Generate test vectors for various key ids. 
testInputs = itertools.product(testedKeyIds, testedEncTypes) 
for (keyId, encType) in testInputs:

    if encType == kWeaveEncryptionType_None:
        continue
    
    testVector = MessageEncodingTestVector(
        keyId = keyId,
        encType = encType
    )

    try:
        testVector.encodeMessage()
    except UnsupportedMessageEncoding, ex:
        continue

    print testVector.toCDecl(testVectorDeclName % (testNum))
    
    testNum += 1

# Generate test vectors for messages of various lengths. 
testInputs = itertools.product(testedMessageLengths, testedFlags, testedEncTypes) 
for (msgPayloadLen, flags, encType) in testInputs:

    testVector = MessageEncodingTestVector(
        msgPayloadLen = msgPayloadLen,
        flags = flags,
        encType = encType
    )

    try:
        testVector.encodeMessage()
    except UnsupportedMessageEncoding, ex:
        continue

    print testVector.toCDecl(testVectorDeclName % (testNum))
    
    testNum += 1

print 'static const size_t sNumMessageEncodingTestVectors = %d;\n' % (testNum)

print 'static const MessageEncodingTestVector * sMessageEncodingTestVectors[] ='
print '{'
for i in xrange(testNum):
    print '    &' + (testVectorDeclName % (i)) + ','
print '};'

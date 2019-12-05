#!/usr/bin/env python3

#
#   Copyright (c) 2019 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   @file
#         This file contains definitions for working with data encoded in Weave TLV format.
#

import sys
import struct
from collections import Mapping, Sequence, OrderedDict


TLVType_SignedInteger                   = 0x00
TLVType_UnsignedInteger                 = 0x04
TLVType_Boolean                         = 0x08
TLVType_FloatingPointNumber             = 0x0A
TLVType_UTF8String                      = 0x0C
TLVType_ByteString                      = 0x10
TLVType_Null                            = 0x14
TLVType_Structure                       = 0x15
TLVType_Array                           = 0x16
TLVType_Path                            = 0x17

TLVTagControl_Anonymous                 = 0x00
TLVTagControl_ContextSpecific           = 0x20
TLVTagControl_CommonProfile_2Bytes      = 0x40
TLVTagControl_CommonProfile_4Bytes      = 0x60
TLVTagControl_ImplicitProfile_2Bytes    = 0x80
TLVTagControl_ImplicitProfile_4Bytes    = 0xA0
TLVTagControl_FullyQualified_6Bytes     = 0xC0
TLVTagControl_FullyQualified_8Bytes     = 0xE0

TLVBoolean_False                        = TLVType_Boolean
TLVBoolean_True                         = TLVType_Boolean + 1

TLVEndOfContainer                       = 0x18                  

INT8_MIN                                = -128
INT16_MIN                               = -32768
INT32_MIN                               = -2147483648
INT64_MIN                               = -9223372036854775808
 
INT8_MAX                                = 127
INT16_MAX                               = 32767
INT32_MAX                               = 2147483647
INT64_MAX                               = 9223372036854775807
 
UINT8_MAX                               = 255
UINT16_MAX                              = 65535
UINT32_MAX                              = 4294967295
UINT64_MAX                              = 18446744073709551615

class TLVWriter(object):

    def __init__(self, encoding=None, implicitProfile=None):
        self._encoding = encoding if encoding is not None else bytearray()
        self._implicitProfile = implicitProfile
        self._containerStack = []

    @property
    def encoding(self):
        '''The object into which encoded TLV data is written.
           
           By default this is a bytearray object. 
        '''
        return self._encoding
    
    @encoding.setter
    def encoding(self, val):
        self._encoding = val

    @property
    def implicitProfile(self):
        '''The Weave profile id used when encoding implicit profile tags.
        
           Setting this value will result in an implicit profile tag being encoded
           whenever the profile of the tag to be encoded matches the specified implicit
           profile id.
           
           Setting this value to None (the default) disabled encoding of implicit
           profile tags.
        '''
        return self._implicitProfile
    
    @implicitProfile.setter
    def implicitProfile(self, val):
        self._implicitProfile = val

    def put(self, tag, val):
        '''Write a value in TLV format with the specified TLV tag.
        
           val can be a Python object which will be encoded as follows: 
           - Python bools, floats and strings are encoded as their respective TLV types.
           - Python ints are encoded as unsigned TLV integers if zero or positive; signed TLV
             integers if negative.
           - None is encoded as a TLV Null.
           - bytes and bytearray objects are encoded as TVL byte strings.
           - Mapping-like objects (e.g. dict) are encoded as TLV structures.  The keys of the
             map object are expected to be tag values, as described below for the tag argument.
             Map values are encoded recursively, using the same rules as defined for the val
             argument. The encoding order of elements depends on the type of the map object. 
             Elements within a dict are automatically encoded tag numerical order. Elements
             within other forms of mapping object (e.g. OrderedDict) are encoded in the
             object's natural iteration order.
           - Sequence-like objects (e.g. arrays) are written as TLV arrays. Elements within
             the array are encoded recursively, using the same rules as defined for the val
             argument.

           tag can be a small int (0-255), a tuple of two integers, or None.
           If tag is an integer, it is encoded as a TLV context-specific tag.
           If tag is a two-integer tuple, it is encoded as a TLV profile-specific tag, with
             the first integer encoded as the profile id and the second as the tag number.
           If tag is None, it is encoded as a TLV anonymous tag.
        '''
        if val == None:
            self.putNull(tag)
        elif isinstance(val, bool):
            self.putBool(tag, val)
        elif isinstance(val, int):
            if val < 0:
                self.putSignedInt(tag, val)
            else:
                self.putUnsignedInt(tag, val)
        elif isinstance(val, float):
            self.putFloat(tag, val)
        elif isinstance(val, str):
            self.putString(tag, val)
        elif isinstance(val, bytes) or isinstance(val, bytearray):
            self.putBytes(tag, val)
        elif isinstance(val, Mapping):
            self.startStructure(tag)
            if type(val) == dict:
                val = OrderedDict(sorted(val.items(), key=lambda item: tlvTagToSortKey(item[0])))
            for containedTag, containedVal in val.items():
                self.put(containedTag, containedVal)
            self.endContainer()
        elif isinstance(val, Sequence):
            self.startArray(tag)
            for containedVal in val:
                self.put(None, containedVal)
            self.endContainer()
        else:
            raise ValueError('Attempt to TLV encode unsupported value')

    def putSignedInt(self, tag, val):
        '''Write a value as a TLV signed integer with the specified TLV tag.'''
        if val >= INT8_MIN and val <= INT8_MAX:
            format = '<b'
        elif val >= INT16_MIN and val <= INT16_MAX:
            format = '<h'
        elif val >= INT32_MIN and val <= INT32_MAX:
            format = '<l'
        elif val >= INT64_MIN and val <= INT64_MAX:
            format = '<q'
        else:
            raise ValueError('Integer value out of range')
        val = struct.pack(format, val)
        controlAndTag = self._encodeControlAndTag(TLVType_SignedInteger, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def putUnsignedInt(self, tag, val):
        '''Write a value as a TLV unsigned integer with the specified TLV tag.'''
        val = self._encodeUnsignedInt(val)
        controlAndTag = self._encodeControlAndTag(TLVType_UnsignedInteger, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def putFloat(self, tag, val):
        '''Write a value as a TLV float with the specified TLV tag.'''
        val = struct.pack('d', val)
        controlAndTag = self._encodeControlAndTag(TLVType_FloatingPointNumber, tag, lenOfLenOrVal=len(val))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(val)

    def putString(self, tag, val):
        '''Write a value as a TLV string with the specified TLV tag.'''
        val = val.encode('utf-8')
        valLen = self._encodeUnsignedInt(len(val))
        controlAndTag = self._encodeControlAndTag(TLVType_UTF8String, tag, lenOfLenOrVal=len(valLen))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(valLen)
        self._encoding.extend(val)

    def putBytes(self, tag, val):
        '''Write a value as a TLV byte string with the specified TLV tag.'''
        valLen = self._encodeUnsignedInt(len(val))
        controlAndTag = self._encodeControlAndTag(TLVType_ByteString, tag, lenOfLenOrVal=len(valLen))
        self._encoding.extend(controlAndTag)
        self._encoding.extend(valLen)
        self._encoding.extend(val)

    def putBool(self, tag, val):
        '''Write a value as a TLV boolean with the specified TLV tag.'''
        if val:
            type = TLVBoolean_True
        else:
            type = TLVBoolean_False
        controlAndTag = self._encodeControlAndTag(type, tag)
        self._encoding.extend(controlAndTag)

    def putNull(self, tag):
        '''Write a TLV null with the specified TLV tag.'''
        controlAndTag = self._encodeControlAndTag(TLVType_Null, tag)
        self._encoding.extend(controlAndTag)

    def startContainer(self, tag, containerType):
        '''Start writing a TLV container with the specified TLV tag.
        
           containerType can be one of TLVType_Structure, TLVType_Array or
           TLVType_Path.
        '''
        self._verifyValidContainerType(containerType)
        controlAndTag = self._encodeControlAndTag(containerType, tag)
        self._encoding.extend(controlAndTag)
        self._containerStack.insert(0, containerType)

    def startStructure(self, tag):
        '''Start writing a TLV structure with the specified TLV tag.'''
        self.startContainer(tag, containerType=TLVType_Structure)

    def startArray(self, tag):
        '''Start writing a TLV array with the specified TLV tag.'''
        self.startContainer(tag, containerType=TLVType_Array)

    def startPath(self, tag):
        '''Start writing a TLV path with the specified TLV tag.'''
        self.startContainer(tag, containerType=TLVType_Path)

    def endContainer(self):
        '''End writing the current TLV container.'''
        self._containerStack.pop(0)
        controlAndTag = self._encodeControlAndTag(TLVEndOfContainer, None)
        self._encoding.extend(controlAndTag)

    def _encodeControlAndTag(self, type, tag, lenOfLenOrVal=0):
        controlByte = type
        if lenOfLenOrVal == 2:
            controlByte |= 1
        elif lenOfLenOrVal == 4:
            controlByte |= 2
        elif lenOfLenOrVal == 8:
            controlByte |= 3
        if tag == None:
            if type != TLVEndOfContainer and len(self._containerStack) != 0 and self._containerStack[0] == TLVType_Structure:
                raise ValueError('Attempt to encode anonymous tag within TLV structure')
            controlByte |= TLVTagControl_Anonymous
            return struct.pack('<B', controlByte)
        if isinstance(tag, int):
            if tag < 0 or tag > UINT8_MAX:
                ValueError('Context-specific TLV tag number out of range')
            if len(self._containerStack) == 0:
                raise ValueError('Attempt to encode context-specific TLV tag at top level')
            if self._containerStack[0] == TLVType_Array:
                raise ValueError('Attempt to encode context-specific tag within TLV array')
            controlByte |= TLVTagControl_ContextSpecific
            return struct.pack('<BB', controlByte, tag)
        if isinstance(tag, tuple):
            (profile, tagNum) = tag
            if not isinstance(tagNum, int):
                raise ValueError('Invalid object given for TLV tag')
            if tagNum < 0 or tagNum > UINT32_MAX:
                raise ValueError('TLV tag number out of range')
            if profile != None:
                if not isinstance(profile, int):
                    raise ValueError('Invalid object given for TLV profile id')
                if profile < 0 or profile > UINT32_MAX:
                    raise ValueError('TLV profile id value out of range')
            if len(self._containerStack) != 0 and self._containerStack[0] == TLVType_Array:
                raise ValueError('Attempt to encode profile-specific tag within TLV array')
            if profile == None or profile == self._implicitProfile:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_ImplicitProfile_2Bytes
                    return struct.pack('<BH', controlByte, tagNum)
                else:
                    controlByte |= TLVTagControl_ImplicitProfile_4Bytes
                    return struct.pack('<BL', controlByte, tagNum)
            elif profile == 0:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_CommonProfile_2Bytes
                    return struct.pack('<BH', controlByte, tagNum)
                else:
                    controlByte |= TLVTagControl_CommonProfile_4Bytes
                    return struct.pack('<BL', controlByte, tagNum)
            else:
                if tagNum <= UINT16_MAX:
                    controlByte |= TLVTagControl_FullyQualified_6Bytes
                    return struct.pack('<BLH', controlByte, profile, tagNum)
                else:
                    controlByte |= TLVTagControl_FullyQualified_8Bytes
                    return struct.pack('<BLL', controlByte, profile, tagNum)
        raise ValueError('Invalid object given for TLV tag')
    
    @staticmethod
    def _encodeUnsignedInt(val):
        if val < 0:
            raise ValueError('Integer value out of range')
        if val <= UINT8_MAX:
            format = '<B'
        elif val <= UINT16_MAX:
            format = '<H'
        elif val <= UINT32_MAX:
            format = '<L'
        elif val <= UINT64_MAX:
            format = '<Q'
        else:
            raise ValueError('Integer value out of range')
        return struct.pack(format, val)
        
    @staticmethod
    def _verifyValidContainerType(containerType):
        if containerType != TLVType_Structure and containerType != TLVType_Array and containerType != TLVType_Path:
            raise ValueError('Invalid TLV container type')

def tlvTagToSortKey(tag):
    if tag == None:
        return -1
    if isinstance(tag, int):
        majorOrder = 0;
    elif isinstance(tag, tuple):
        (profileId, tag) = tag
        if profileId == None:
            majorOrder = 1
        else:
            majorOrder = profileId + 2
    else:
        raise ValueError('Invalid TLV tag')
    return (majorOrder << 32) + tag

if __name__ == '__main__':
    val = dict([
        (1, 0),
        (2, 65536),
        (3, True),
        (4, None),
        (5, "Hello!"),
        (6, bytearray([ 0xDE, 0xAD, 0xBE, 0xEF ])),
        (7, [
            "Goodbye!",
            71024724507,
            False
        ]),
        ((0x235A0000, 42), "FOO"),
        ((None, 42), "BAR"),
    ])
    writer = TLVWriter()
    encodedVal = writer.put(None, val)
    sys.stdout.write(writer.encoding)

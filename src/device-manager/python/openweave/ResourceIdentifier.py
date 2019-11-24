#
#    Copyright (c) 2019 Google, LLC.
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
#      Python interface for ResourceIdentifier
#

from ctypes import *
from WeaveStack import *
import struct

__all__ = [ 'ResourceIdentifier' , 'ResourceIdentifierStruct' ]

SELF_NODE_ID = -2

class ResourceIdentifier:
    def __init__(self, resourceType=0, resourceId=SELF_NODE_ID):
        self.ResourceType = resourceType
        self.ResourceId = resourceId

    def Print(self, prefix=""):
        if self.ResourceType != None:
            print "%sResource Type: %s" % (prefix, self.ResourceType)
        if self.ResourceId != None:
            print "%sResource Id: \"%d\"" % (prefix, self.ResourceId)

    @staticmethod
    def MakeResTypeIdInt(type, idInt):
        return ResourceIdentifier(type, idInt)

    @staticmethod
    def MakeResTypeIdBytes(type, idBytes):
        ResourceId = struct.unpack("<Q", idBytes)[0]
        return ResourceIdentifier(type, ResourceId)

class _ResourceIdUnion(Union):
    _fields_ = [
        ('ResourceId', c_uint64),               # resource id, uint64 value
        ('ResourceIdBytes', c_ubyte * 8),       # resource id, 8 bytes contain the object id, also encoded as integer in little-endian format.
    ]

class ResourceIdentifierStruct(Structure):
    _fields_ = [
        ('ResourceType', c_uint16),             # The type of resource id.
        ('ResourceIdUnion', _ResourceIdUnion),  # The resource id.
    ]

    @classmethod
    def fromResourceIdentifier(cls, resourceIdentifier):
        resourceIdentifierStruct = cls()
        resourceIdentifierStruct.ResourceType = resourceIdentifier.ResourceType if resourceIdentifier.ResourceType != None else -1
        resourceIdentifierStruct.ResourceIdUnion = _ResourceIdUnion(resourceIdentifier.ResourceId) if resourceIdentifier.ResourceId != None else -1
        return resourceIdentifierStruct

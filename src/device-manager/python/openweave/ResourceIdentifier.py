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

__all__ = [ 'ResourceIdentifier' , 'ResourceIdentifierStruct' ]

ResourcesTypes = {
    "RESOURCE_TYPE_RESERVED" : 0,
    "RESOURCE_TYPE_DEVICE" : 1,
    "RESOURCE_TYPE_USER" : 2,
    "RESOURCE_TYPE_ACCOUNT" : 3,
    "RESOURCE_TYPE_AREA" : 4,
    "RESOURCE_TYPE_FIXTURE" : 5,
    "RESOURCE_TYPE_GROUP" : 6,
    "RESOURCE_TYPE_ANNOTATION" : 7,
    "RESOURCE_TYPE_STRUCTURE" : 8,
    "RESOURCE_TYPE_GUEST" : 9,
    "RESOURCE_TYPE_SERVICE" : 10,
}

class ResourceIdentifier:
    def __init__(self, resourceType=ResourcesTypes["RESOURCE_TYPE_RESERVED"], resourceIdInt64=-2, resourceIdBytes=None):
        self.ResourceType = resourceType
        self.ResourceIdInt64 = resourceIdInt64
        self.ResourceIdBytes = resourceIdBytes

    def Print(self, prefix=""):
        if self.ResourceType != None:
            print "%sResource Type: %s" % (prefix, self.ResourceType)
        if self.ResourceIdInt64 != None:
            print "%sResource Id Uint64: \"%d\"" % (prefix, self.ResourceIdInt64)
        if self.ResourceIdBytes != None:
            print "%sResource Id Bytes: \"%s\"" % (prefix, WeaveStack.ByteArrayToHex(self.ResourceIdBytes))

    def SetField(self, name, val):
        name = name.lower()
        if (name == 'resourceType'):
            self.ResourceType = self._parseResourceType(val)
        elif (name == 'resourceIdInt64'):
            self.ResourceIdInt64 = int(val)
        elif (name == 'resourceIdBytes'):
            self.ResourceIdBytes = val

    # resourceIdentifierStr's format would be something like DEVICE_18B4300000000001, RESERVED_FFFFFFFFFFFFFFFE
    @staticmethod
    def MakeResString(resourceIdentifierStr):
        splitedIdentifier = resourceIdentifierStr.split("_")
        resourceType = "RESOURCE_TYPE_" + splitedIdentifier[0]
        if resourceType not in ResourcesTypes.keys():
            print "unexpected resourceIdentifierStr, ResourceType is not applicable"
            return None
        ResourceType = ResourcesTypes[resourceType]
        ResourceIdInt64 = int(splitedIdentifier[1], 16)
        return ResourceIdentifier(ResourceType, ResourceIdInt64)

    @staticmethod
    def MakeResTypeIdString(type, idStr):
        resourceType = type
        if resourceType not in ResourcesTypes.keys():
            print "unexpected resourceIdentifierStr, ResourceType is not applicable"
            return None
        ResourceType = ResourcesTypes[resourceType]
        ResourceIdInt64 = int(idStr, 16)
        return ResourceIdentifier(ResourceType, ResourceIdInt64)

    @staticmethod
    def MakeResTypeIdInt(type, idInt):
        resourceType = type
        if resourceType not in ResourcesTypes.keys():
            print "unexpected resourceIdentifierStr, ResourceType is not applicable"
            return None
        ResourceType = ResourcesTypes[resourceType]
        ResourceIdInt64 = idInt
        return ResourceIdentifier(ResourceType, ResourceIdInt64)

    @staticmethod
    def MakeResTypeIdBytes(type, idBytes):
        resourceType = type
        if resourceType not in ResourcesTypes.keys():
            print "unexpected resourceIdentifierStr, ResourceType is not applicable"
            return None
        ResourceType = ResourcesTypes[resourceType]
        ResourceIdBytes = idBytes
        return ResourceIdentifier(ResourceType, 0, ResourceIdBytes)

    def _parseResourceType(self, name):
        if name in ResourcesTypes.keys():
            return ResourcesTypes[name]
        else:
            print "UNKNOWN (" + str(name)+ ")"
            return None

class _ResourceIdUnion(Union):
    _fields_ = [
        ('ResourceId', c_uint64),         # resource id, uint64 value
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
        if resourceIdentifier.ResourceIdBytes != None:
            raw_bytes = (c_ubyte * 8).from_buffer_copy(resourceIdentifier.ResourceIdBytes)
            resourceIdentifierStruct.ResourceIdUnion = _ResourceIdUnion(ResourceIdBytes=raw_bytes)
        elif resourceIdentifier.ResourceIdInt64 != None:
            resourceIdentifierStruct.ResourceIdUnion = _ResourceIdUnion(resourceIdentifier.ResourceIdInt64)
        return resourceIdentifierStruct

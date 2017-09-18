#!/usr/bin/python

#
#    Copyright (c) 2016-2017 Nest Labs, Inc.
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
import struct

# Directory entry types
DirectoryEntryType_SingleNode = 0
DirectoryEntryType_HostPortList = 1


class SingleNodeDirectoryEntry():
    def __init__(self, serviceEndpointId=None, nodeId=None):
        self.serviceEndpointId = serviceEndpointId
        self.nodeId = nodeId
        
    def __str__(self):
        return self.toStr()
    
    def toStr(self, prefix='', indent='    '):
        return '%sService Endpoint Id: %016X\n%sNode Id: %016X\n' % (prefix, self.serviceEndpointId, prefix, self.nodeId)
        

class HostPortListDirectoryEntry():
    def __init__(self, serviceEndpointId=None):
        self.serviceEndpointId = serviceEndpointId
        self.hostPortList = []

    def __str__(self):
        return self.toStr()
    
    def toStr(self, prefix='', indent='    '):
        res = '%sService Endpoint Id: %016X\n' % (prefix, self.serviceEndpointId)
        
        res += '%sHost/Port List (length %d):\n' % (prefix, len(self.hostPortList))
        for i in range(0, len(self.hostPortList)):
            hostPortElem = self.hostPortList[i]
            res += '%s%d:\n' % (prefix + indent, i)
            res += hostPortElem.toStr(prefix=prefix + indent * 2, indent=indent)
        
        return res

class HostPortListElement():
    def __init__(self, hostName=None, port=None, suffixIndex=None):
        self.hostName = hostName
        self.port = port
        self.suffixIndex = suffixIndex

    def __str__(self):
        return self.toStr()
    
    def toStr(self, prefix='', indent='    '):
        res = '%sHost Name: "%s"\n' % (prefix, self.hostName)
        res += '%sPort: %s\n' % (prefix, str(self.port) if self.port != None else "Not Specified")
        res += '%sSuffix Index: %s\n' % (prefix, str(self.suffixIndex) if self.suffixIndex != None else "Not Specified")
        return res
 
class ServiceDirectoryResponse():
    
    def __init__(self):
        self.dirList = []
        self.suffixTable = None
        self.isRedirect = False
        self.queryReceiptTime = None
        self.processingTime = None 
        pass

    @staticmethod
    def decode(encodedServiceDir):

        self = ServiceDirectoryResponse()

        parseOffset = 0
        
        # Extract the response header field from the response.
        respHeader = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
        parseOffset += 1
        
        # Decode the various subfields within the response header.
        dirEntryCount = respHeader & 0x0F
        self.isRedirect = (respHeader & 0x10) != 0
        suffixTablePresent = (respHeader & 0x20) != 0 
        timeFieldsPresent = (respHeader & 0x40) != 0 
    
        # Verify that the reserve bit is 0.
        if (respHeader & 0x80) != 0:
            raise ValueError('Invalid response header value: reserved bit not zero')

        # Decode the list of directory entries.
        parseOffset = self._decodeDirList(encodedServiceDir, parseOffset, dirEntryCount)
        
        # Decode the suffix table if present.
        if suffixTablePresent:
            parseOffset = self._decodeSuffixTable(encodedServiceDir, parseOffset)
        
        # Decode the time fields, if present.
        if timeFieldsPresent:
            
            self.queryReceiptTime = struct.unpack_from('<Q', encodedServiceDir, parseOffset)[0]
            parseOffset += 8
            
            self.processingTime = struct.unpack_from('<L', encodedServiceDir, parseOffset)[0]
            parseOffset += 4
        
        # Verify the length of the encoded service directory.
        if parseOffset != len(encodedServiceDir):
            raise ValueError('Unexpected data at end of Service Directory response')

        # Verify any suffix indexes in the host/port directory entries have corresponding positions
        # in the suffix table.
        for i in range(0, len(self.dirList)):
            dirEntry = self.dirList[i]
            if isinstance(dirEntry, HostPortListDirectoryEntry):
                for hostPortEntry in dirEntry.hostPortList:
                    if hostPortEntry.suffixIndex != None:
                        if self.suffixTable == None or hostPortEntry.suffixIndex >= len(self.suffixTable):
                            raise ValueError('Invalid directory entry (entry %d): reference to non-existant suffix table entry' % i)

        return self

    def _decodeDirList(self, encodedServiceDir, parseOffset, dirEntryCount):

        # Decode each of the directory entries    
        for i in range(0, dirEntryCount):
            
            # Extract the header for the current directory entry.
            dirEntryHeader = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
            parseOffset += 1
            
            # Decode the subfields within the directory entry header.
            hostPortListLen = dirEntryHeader & 0x7
            entryType = (dirEntryHeader & 0xC0) >> 6
            
            # Verify that the reserved bits are zero.
            if (dirEntryHeader & 0x38) != 0:
                raise ValueError('Invalid directory entry header value (entry %d): reserved bits not zero' % (i))
            
            # Verify the entry type.
            if entryType != DirectoryEntryType_SingleNode and entryType != DirectoryEntryType_HostPortList:
                raise ValueError('Invalid directory entry type (entry %d): %d' % (i, entryType))

            # Verify the hostPortListLen.
            if entryType == DirectoryEntryType_SingleNode and hostPortListLen > 0:
                raise ValueError('Invalid single node directory entry (entry %d): host/port list length is %d, but should be zero' % (i, hostPortListLen))
            if entryType == DirectoryEntryType_HostPortList and hostPortListLen == 0:
                raise ValueError('Invalid host/port list directory entry (entry %d): host/port list length is 0' % (i))

            # Extract the service endpoint id for the directory entry.
            serviceEndpointId = struct.unpack_from('<Q', encodedServiceDir, parseOffset)[0]
            parseOffset += 8
            
            # If this this a 'single node' directory entry...
            if entryType == DirectoryEntryType_SingleNode:
                
                # Extract the node id for the directory entry.
                nodeId = struct.unpack_from('<Q', encodedServiceDir, parseOffset)[0]
                parseOffset += 8

                # Form a single node directory entry object                
                dirEntry = SingleNodeDirectoryEntry(serviceEndpointId, nodeId)

            # Otherwise, this is a Host/Port list directory entry...                
            else:

                # Form a Host/Port list directory entry object.
                dirEntry = HostPortListDirectoryEntry(serviceEndpointId=serviceEndpointId)
                
                # For each host/port element in the current directory entry...
                for j in range(0, hostPortListLen):
                
                    # Extract the host/port entry header
                    hostPortEntryHeader = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
                    parseOffset += 1
                    
                    # Decode subfields within the host/port entry header.
                    hostIdType = (hostPortEntryHeader & 0x03)
                    suffixIndexPresent = (hostPortEntryHeader & 0x04) != 0
                    portPresent = (hostPortEntryHeader & 0x08) != 0
                    
                    # Verify that the reserved bits are zero.
                    if (hostPortEntryHeader & 0xF0) != 0:
                        raise ValueError('Invalid host/port directory entry header value (entry %d, element %d): reserved bits not zero' % (i, j))
            
                    # Extract and verify the host name length.
                    hostNameLen = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
                    parseOffset += 1
                    if (hostNameLen == 0):
                        raise ValueError('Invalid host/port directory entry (entry %d, element %d): host name length is zero' % (i, j))
                    
                    # Extract the host name.
                    hostName = struct.unpack_from('<' + str(hostNameLen) + 's', encodedServiceDir, parseOffset)[0]
                    parseOffset += hostNameLen
                    
                    # If present, extract the suffix index.
                    if suffixIndexPresent:
                        suffixIndex = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
                        parseOffset += 1 
                    else:
                        suffixIndex = None
                        
                    # If present, extract the port.
                    if portPresent:
                        port = struct.unpack_from('<H', encodedServiceDir, parseOffset)[0]
                        parseOffset += 2
                    else:
                        port = None
                    
                    # Form a host/port list element and add it to the directory entry.
                    elem = HostPortListElement(hostName=hostName, port=port, suffixIndex=suffixIndex)
                    dirEntry.hostPortList.append(elem)

            # Add the directory entry to entry list.
            self.dirList.append(dirEntry)
        
        return parseOffset
    
    def _decodeSuffixTable(self, encodedServiceDir, parseOffset):

        self.suffixTable = []

        # Extract the suffix table length.
        suffixTableLen = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
        parseOffset += 1 

        # Decode each of the suffix table entries...    
        for i in range(0, suffixTableLen):
            
            # Extract and verify the suffix string length.
            suffixLen = struct.unpack_from('<B', encodedServiceDir, parseOffset)[0]
            parseOffset += 1
            if (suffixLen == 0):
                raise ValueError('Invalid suffix table entry (entry %d): suffix length is zero' % (i))
            
            # Extract the suffix string.
            suffix = struct.unpack_from('<' + str(suffixLen) + 's', encodedServiceDir, parseOffset)[0]
            parseOffset += suffixLen
            
            # Add the suffix string to the table array.
            self.suffixTable.append(suffix)

        return parseOffset
    
    def __str__(self):
        return self.toStr()
    
    def toStr(self, prefix='', indent='    '):
        res = ''
        res += '%sDirectory List (length: %d):\n' % (prefix, len(self.dirList))
        for i in range(0, len(self.dirList)):
            dirEntry = self.dirList[i]
            res += '%sElem %d (%s):\n' % (prefix + indent, i, "Host/Port List" if isinstance(dirEntry, HostPortListDirectoryEntry) else "Single Node")
            res += dirEntry.toStr(prefix=prefix + indent * 2, indent=indent)
        
        if self.suffixTable != None:
            res += '%sSuffix Table (length %d):\n' % (prefix, len(self.suffixTable))
            for i in range(0, len(self.suffixTable)):
                res += '%s%d: "%s"\n' % (prefix + indent, i, self.suffixTable[i])
        
        res += '%sIs Redirect: %s\n' % (prefix, self.isRedirect)

        if self.queryReceiptTime != None:
            res += '%sQuery Receipt Time: %d\n' % (prefix, self.queryReceiptTime)
        else:
            res += '%sQuery Receipt Time: Not Specified\n' % (prefix)
            
        if self.processingTime != None:
            res += '%sProcessing Time: %d\n' % (prefix, self.processingTime)
        else:
            res += '%sProcessing Time: Not Specified\n' % (prefix)

        return res
            

usage = """
decode-service-dir.py -- Decode a Weave Service Directory response. 

Usage:

  decode-service-dir.py [ <file-name> ]

Input to the tool should be the payload of a Weave Service Directory response
message, in binary form.  If no input file is given, the tool reads from stdin.

The following command can be used to decode a response in hex format:

  xxd -r -p <hex-input-file> | decode-service-dir.py

"""

# Main code
#
if __name__ == "__main__":

    if len(sys.argv) == 1:
        inFile = sys.stdin
    elif len(sys.argv) > 2:
        sys.stderr.write("Unexpected argument: %s\n" % sys.argv[2])
        sys.exit(-1)
    else:
        arg = sys.argv[1].lower()
        if arg == 'help' or arg == '-h' or arg == '--help':
            sys.stderr.write(usage)
            sys.exit(-1)
        inFile = open(sys.argv[1], 'rb')
      
    encodedServiceDir = inFile.read()
    
    sd = ServiceDirectoryResponse.decode(encodedServiceDir)
    
    print sd
